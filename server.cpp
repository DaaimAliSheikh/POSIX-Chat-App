#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

typedef struct {
  char username[30];
  bool exit;
  int fd;
} User;

string server_fifo;
string login_fifo;
int login_fd;
int server_fd;

vector<User> clients;

void *login_handler(void *) {
  login_fd = open(login_fifo.c_str(), O_RDWR);
  User user;
  while (true) {

    read(login_fd, &user, sizeof(User));

    if (user.exit == false) {
      user.fd = open(user.username, O_RDWR);
      clients.push_back(user);

      string user_joined = "'" + string(user.username) +
                           "' has joined the server"; ////announcements are not
                                                      /// prefixed with :xyz
      cout << user_joined << endl;
      for (int i = 0; i < clients.size(); i++)
        write(clients[i].fd, user_joined.c_str(), user_joined.size() + 1);

    } else if (user.exit == true) {
      for (int i = 0; i < clients.size(); i++) {
        if (string(clients[i].username) == string(user.username)) {

          string user_left = "'" + string(clients[i].username) +
                             "' left the server"; ////announcements are not
                                                  /// prefixed with :xyz
          cout << user_left << endl;
          for (int i = 0; i < clients.size(); i++)
            write(clients[i].fd, user_left.c_str(), user_left.size() + 1);

          close(clients[i].fd);
          clients.erase(clients.begin() + i);
          break;
        }
      }
    }
  }
}

void *server_handler(void *) {
  server_fd = open(server_fifo.c_str(), O_RDWR);
  char buff[100];
  while (true) {

    read(server_fd, buff, sizeof(buff));

    for (int i = 0; i < clients.size(); i++)
      write(clients[i].fd, buff, sizeof(buff));
  }
}

void cleanup(int sign) {
  /// close all open clients
  for (int i = 0; i < clients.size(); i++) {

    write(clients[i].fd, "exit", sizeof("exit"));
    cout << "removing " << string(clients[i].username) << endl;
    close(clients[i].fd);
    unlink(clients[i].username);
  }

  close(server_fd);
  close(login_fd);
  unlink(server_fifo.c_str());
  unlink(login_fifo.c_str());

  cout << "shutting down server, Good Bye!" << endl;
  _exit(0);
}

int main() {
  string ip;
  cout << "Enter the IP address you want to designate to this server:" << endl;

  signal(SIGINT, cleanup);  // ctrl+c from terminal
  signal(SIGHUP, cleanup);  // killing process by closing terminal close
  signal(SIGABRT, cleanup); // abort call
  signal(SIGSEGV, cleanup); // segmentation fault

  while (true) {

    cin >> ip;

    server_fifo = ip + "server";
    login_fifo = ip + "login";

    if (mkfifo(server_fifo.c_str(), 0666) == -1) {
      cout << "This IP is already designated to another running "
              "server,\nplease enter "
              "another IP address:"
           << endl;
      continue;
    } else if (mkfifo(login_fifo.c_str(), 0666) == -1) {
      cout << "This IP is already designated to another running "
              "server,\nplease enter "
              "another IP address:"
           << endl;
      unlink(server_fifo.c_str());
    }

    else
      break;
  }

  pthread_t login_thread;
  pthread_t server_thread;
  pthread_create(&login_thread, NULL, login_handler, NULL);
  pthread_create(&server_thread, NULL, server_handler, NULL);

  cout << "server started at IP address: " << ip
       << ", type 'exit' to shutdown server" << endl;
  string s;

  while (s != "exit")
    cin >> s;

  cleanup(0);

  return 0;
}