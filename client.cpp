#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

/// threads, mutexes, named pipes, signals

/// pictures
#define fontLocation "./SFML-config/Poppins-Regular.ttf"
#define logoLocation "./SFML-config/fifo-net-logo.jpeg"
#define loginLocation "./SFML-config/login-bg.jpg"
#define chatLocation "./SFML-config/chat-bg.jpg"

using namespace std;

enum class State { CHAT, LOGIN };
State state = State::LOGIN;
string client_name;
string server_name;
int server_fd;
pthread_mutex_t mt;

typedef struct {
  char username[30];
  bool exit;
  int fd; /// no need in client but need in server
} User;

class Login {
private:
  sf::Font font;
  sf::Text usernameText;
  sf::Text serverIPText;
  sf::Text joinButton;
  sf::Text errorText;
  sf::RectangleShape joinButtonBox;
  sf::RectangleShape usernameInputBox;
  sf::RectangleShape serverIPInputBox;
  sf::Text usernameInput;
  sf::Text serverIPInput;
  std::string username;
  std::string serverIP;
  sf::Texture logo_tex;
  sf::Sprite logo_sp;
  sf::Texture login_tex;
  sf::Sprite login_sp;

  bool isUsernameSelected = true;
  bool isServerIPSelected = false;

public:
  Login() : username(""), serverIP("") {
    font.loadFromFile(fontLocation);
    logo_tex.loadFromFile(logoLocation);
    logo_sp.setTexture(logo_tex);
    logo_sp.setPosition(210, 30);
    logo_sp.setScale(0.3f, 0.3f);

    login_tex.loadFromFile(loginLocation);
    login_sp.setTexture(login_tex);
    login_sp.setScale(0.45f, 0.45f);

    // sf::Color color = sprite.getColor();
    // color.a = 128; // Set alpha to 128 (range 0 - 255, where 0 is fully
    //                // transparent and 255 is fully opaque)
    // sprite.setColor(color);

    usernameText.setFont(font);
    usernameText.setString("Username ");
    usernameText.setCharacterSize(16);
    usernameText.setFillColor(sf::Color::White);
    usernameText.setStyle(sf::Text::Bold);
    usernameText.setPosition(150, 300);

    serverIPText.setFont(font);
    serverIPText.setString("Server IP ");
    serverIPText.setCharacterSize(16);
    serverIPText.setStyle(sf::Text::Bold);
    serverIPText.setFillColor(sf::Color::White);
    serverIPText.setPosition(150, 340);

    joinButton.setFont(font);
    joinButton.setStyle(sf::Text::Bold);
    joinButton.setString("Join");
    joinButton.setCharacterSize(16);
    joinButton.setFillColor(sf::Color::White);
    joinButton.setPosition(310, 400);

    errorText.setFont(font);
    errorText.setString("");
    errorText.setStyle(sf::Text::Bold);
    errorText.setCharacterSize(17);
    errorText.setFillColor(sf::Color(243, 53, 53));
    errorText.setPosition(180, 440);

    joinButtonBox.setSize(sf::Vector2f(100, 30));
    joinButtonBox.setFillColor(sf::Color::Blue);
    joinButtonBox.setPosition(280, 395);

    usernameInputBox.setSize(sf::Vector2f(180, 30));
    usernameInputBox.setFillColor(sf::Color::White);
    usernameInputBox.setOutlineColor(sf::Color::Green);
    usernameInputBox.setOutlineThickness(3);
    usernameInputBox.setPosition(275, 290);
    // usernameInputBox.setOutlineThickness(1);

    serverIPInputBox.setSize(sf::Vector2f(180, 30));
    serverIPInputBox.setFillColor(sf::Color::White);
    serverIPInputBox.setOutlineColor(sf::Color::Green);
    serverIPInputBox.setPosition(275, 330);

    usernameInput.setFillColor(sf::Color::Black);
    usernameInput.setPosition(288, 293);
    usernameInput.setCharacterSize(16);
    usernameInput.setFont(font);

    serverIPInput.setFillColor(sf::Color::Black);
    serverIPInput.setPosition(288, 333);
    serverIPInput.setCharacterSize(16);
    serverIPInput.setFont(font);
  }
  void render(sf::RenderWindow &window) {

    window.clear(sf::Color::Cyan);
    window.draw(login_sp);
    window.draw(logo_sp);
    window.draw(usernameText);
    window.draw(serverIPText);
    window.draw(joinButtonBox);
    window.draw(joinButton);
    window.draw(usernameInputBox);
    window.draw(serverIPInputBox);
    window.draw(serverIPInput);
    window.draw(usernameInput);
    window.draw(errorText);

    window.display();
  }

  void handleLeftClick(sf::RenderWindow &window) {
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    if (usernameInputBox.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
      isUsernameSelected = true;
      isServerIPSelected = false;
      usernameInputBox.setOutlineThickness(3);
      serverIPInputBox.setOutlineThickness(0);
    } else if (serverIPInputBox.getGlobalBounds().contains(mousePos.x,
                                                           mousePos.y)) {
      isUsernameSelected = false;
      isServerIPSelected = true;
      usernameInputBox.setOutlineThickness(0);
      serverIPInputBox.setOutlineThickness(3);
    } else if (joinButtonBox.getGlobalBounds().contains(mousePos.x,

                                                        mousePos.y)) {
      if (username.size() == 0)
        errorText.setString("Please provide a username");
      else if (connect_to_server()) {
        state = State::CHAT;
        window.setTitle("Server: " + server_name);
      }
    } else {
      isUsernameSelected = false;
      isServerIPSelected = false;
      usernameInputBox.setOutlineThickness(0);
      serverIPInputBox.setOutlineThickness(0);
    }
  }

  void handleLoginInput(sf::RenderWindow &window, sf::Event &event) {

    if (event.type == sf::Event::Closed) {
      pthread_mutex_destroy(&mt);
      window.close();
      _exit(0);
    } else if (event.type == sf::Event::TextEntered) {
      if (event.text.unicode == '\b') { // Backspace
        if (isUsernameSelected && !username.empty()) {
          username.pop_back();
        } else if (isServerIPSelected && !serverIP.empty()) {
          serverIP.pop_back();
        }
      } else if (event.text.unicode < 128 &&
                 event.text.unicode != 13) { // ASCII printable characters
        if (isUsernameSelected && username.size() < 30) {
          username += static_cast<char>(event.text.unicode);

        } else if (isServerIPSelected) {
          serverIP += static_cast<char>(event.text.unicode);
        }
      } else if (event.text.unicode == 13)
        if (isUsernameSelected || isServerIPSelected) {
          if (username.size() == 0)
            errorText.setString("Please provide a username");
          else if (serverIP.size() == 0)
            errorText.setString("Please provide a Server IP Address");
          else if (connect_to_server()) {
            state = State::CHAT;
            window.setTitle("Server: " + server_name);
          }
        }

      /// simulate click event

      usernameInput.setString(username);
      serverIPInput.setString(serverIP);

    } else if (event.type == sf::Event::MouseButtonPressed)
      if (event.mouseButton.button == sf::Mouse::Left)
        handleLeftClick(window);
  }
  bool connect_to_server() {

    int login_fd = open((serverIP + "login").c_str(), O_RDWR);
    if (login_fd < 0) {
      errorText.setString("Server with server IP: " + serverIP +
                          " does not exist");

      return false;
    }

    server_fd = open((serverIP + "server").c_str(),
                     O_RDWR); /// server_fd is declared globally
    if (server_fd < 0) {
      errorText.setString("Server with server IP: " + serverIP +
                          " does not exist");
      return false;
    }
    server_name = serverIP; /// to use in send_message of Chat class

    if (mkfifo(username.c_str(), 0666) < 0) { /// create client fifo
      errorText.setString("Username '" + username + "' already exists");
      return false;
    }

    User user;
    client_name =
        username; /// sets clientname size to > 0 after validation, allowing the
                  /// await message thread to continue execution
    strcpy(user.username, username.c_str());
    user.exit = false;
    write(login_fd, &user, sizeof(User)); // register client on the server
    close(login_fd);

    return true;
  }
};

class Messages {

public:
  vector<sf::Text> messages;
  sf::Font font;

  Messages() { font.loadFromFile(fontLocation); }

  void add_message(string text) {

    sf::Text message;
    int gap = 25;

    size_t lastColonPos = string(text).find_last_of(':');
    if (lastColonPos == std::string::npos) {
      if (string(text) == "exit") {

        cout << "Server: " << server_name << " was shut down, graceful exit."
             << endl;
        _exit(0);
      } else {
        // announcement
        message.setString("--------------------------------------  " + text +
                          "  -----------------------------------------");
        message.setFillColor(sf::Color::White);
        message.setCharacterSize(16);
        message.setFont(font);

        message.setPosition(0, (messages.size() > 0)
                                   ? messages.back().getPosition().y + gap
                                   : gap);
      }
    }

    else {
      // dialogue

      message.setCharacterSize(16);
      message.setFont(font);

      string sender = text.substr(lastColonPos + 1);
      string content = text.substr(0, lastColonPos);

      if (sender == client_name) {
        message.setString(content);
        message.setPosition(800 - message.getGlobalBounds().width - 50,
                            (messages.size() > 0)
                                ? messages.back().getPosition().y + gap
                                : gap);
        message.setFillColor(sf::Color::Green);
      } else {
        message.setString("> " + sender + " :    " + content);
        message.setPosition(40, (messages.size() > 0)
                                    ? messages.back().getPosition().y + gap
                                    : gap);
        message.setFillColor(sf::Color::Cyan);
      }
    }
    pthread_mutex_lock(&mt);
    messages.push_back(message);
    pthread_mutex_unlock(&mt);
  }
  void draw(sf::RenderWindow &window) {
    for (int i = 0; i < messages.size(); i++) {

      pthread_mutex_lock(&mt);
      window.draw(messages[i]);
      pthread_mutex_unlock(&mt);
    }
  }
};

Messages global_messages;

class Chat {
public:
  sf::Font font;
  sf::Text chatInput;
  sf::Text PlaceHolder;
  sf::RectangleShape chatInputBox;
  sf::RectangleShape scrollContainer;
  sf::View view;
  sf::Texture chat_tex;
  sf::Sprite chat_sp;
  string message;

  Chat() {

    view = sf::View(sf::FloatRect(0, 0, 800, 500));
    font.loadFromFile(fontLocation);

    chat_tex.loadFromFile(chatLocation);
    chat_sp.setTexture(chat_tex);
    chat_sp.setScale(1.2f, 1.2f);

    scrollContainer.setSize(sf::Vector2f(800, 500 - 37));
    scrollContainer.setFillColor(sf::Color::Cyan);
    scrollContainer.setOutlineColor(sf::Color::Black);
    scrollContainer.setOutlineThickness(1);

    chatInputBox.setSize(sf::Vector2f(800, 37));
    chatInputBox.setFillColor(sf::Color::White);
    chatInputBox.setOutlineColor(sf::Color::Black);
    chatInputBox.setPosition(0, 460);
    chatInputBox.setOutlineThickness(3);

    chatInput.setFillColor(sf::Color(85, 85, 85));
    chatInput.setCharacterSize(16);
    chatInput.setFont(font);
    chatInput.setPosition(20, 465);
  }
  void handleChatInput(sf::RenderWindow &window, sf::Event &event) {

    if (event.type == sf::Event::Closed) {
      int login_fd = open((server_name + "login").c_str(), O_RDWR);
      User user;
      user.exit = true;
      strcpy(user.username, client_name.c_str());
      if (write(login_fd, &user, sizeof(User)) <
          0) /// signal server to remove this user from its list
        cout << "Disconnected from server: " << server_name << endl;

      else
        cout << "graceful exit from server: " << server_name << endl;

      close(login_fd);

      pthread_mutex_destroy(&mt);
      /// delete client fifo incase server shuts down
      unlink(client_name.c_str());
      close(server_fd);
      window.close();
      _exit(0);

    } else if (event.type == sf::Event::MouseWheelScrolled) {
      if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {

        if (event.mouseWheelScroll.delta < 0 &&
            (scrollContainer.getSize().y - view.getCenter().y) >=
                (view.getSize().y / 2) - 35) {

          view.move(0, -event.mouseWheelScroll.delta *
                           10); // Adjust scroll speed as needed
        }

        // fingers down
        else if (event.mouseWheelScroll.delta > 0 &&
                 view.getCenter().y > (scrollContainer.getGlobalBounds().top +
                                       2 + view.getSize().y / 2)) {
          view.move(0, -event.mouseWheelScroll.delta *
                           10); // Adjust scroll speed as needed
        }
        chatInputBox.setPosition(0, view.getCenter().y + 210);
        chatInput.setPosition(20, view.getCenter().y + 215);
        chat_sp.setPosition(chat_sp.getPosition().x, view.getCenter().y - 250);

        // global_messages.add_message(to_string(view.getCenter().y));
      }
    } else if (event.type == sf::Event::TextEntered) {

      if (event.text.unicode == '\b') { // Backspace
        if (message.size() > 0)
          message.pop_back();

      } else if (event.text.unicode < 128 &&
                 event.text.unicode != 13) { // ASCII printable characters
        message += static_cast<char>(event.text.unicode);
      } else if (event.text.unicode == 13) { // ASCII printable characters
        if (message.size()) {
          send_message(message);
          message = "";
        }
      }
      if (message.size()) {

        chatInput.setFillColor(sf::Color::Black);
        chatInput.setString("> " + client_name + " :   " + message);
      } else {
        chatInput.setFillColor(sf::Color(85, 85, 85));
        chatInput.setString("> " + client_name + " :   " +
                            "Enter message to send");
      }
    }
  }

  void render(sf::RenderWindow &window) {

    window.clear();
    // Set view to the size of the window
    window.setView(view);

    window.draw(scrollContainer);
    window.draw(chat_sp);
    global_messages.draw(window);
    window.draw(chatInputBox);
    window.draw(chatInput);

    window.display();
  }

  void send_message(string message) {

    if (write(server_fd, (message + ":" + client_name).c_str(),
              message.size() + client_name.size() + 2) < 0) {
      cout << "Disconnected from server: " << server_name << endl;
      unlink(client_name.c_str());
      exit(0);
    }
  }
};

void *await_messages(void *param) {
  /// while loop implementaion, using processes

  while (client_name.size() == 0) {
  }; /// block execution untill username is validated, and client fifo created

  Chat *chat = (Chat *)param;
  chat->chatInput.setString("> " + client_name + " :   " +
                            "Enter message to send");

  int fd = open(client_name.c_str(), O_RDWR);

  char buff[100];

  while (1) {

    read(fd, buff, sizeof(buff));
    global_messages.add_message(string(buff));

    // scroll behaviour

    if (global_messages.messages.size() > 17) {
      sf::Vector2f newSize = chat->scrollContainer.getSize();
      newSize.y += 25;
      chat->scrollContainer.setSize(newSize);

      if (chat->scrollContainer.getSize().y - chat->view.getCenter().y <=
          (chat->view.getSize().y / 2) + 5) {

        chat->view.move(0, 25);
        chat->chatInputBox.setPosition(0, chat->view.getCenter().y + 210);
        chat->chatInput.setPosition(20, chat->view.getCenter().y + 215);
        chat->chat_sp.setPosition(chat->chat_sp.getPosition().x,
                                  chat->view.getCenter().y - 250);
      }
    }
  }

  return NULL;
}

void cleanup(int sig) { /// cleanup function
  int login_fd = open((server_name + "login").c_str(), O_RDWR);
  User user;
  user.exit = true;
  strcpy(user.username, client_name.c_str());
  if (write(login_fd, &user, sizeof(User)) <
      0) /// signal server to remove this user from its list
    cout << "Disconnected from server: " << server_name << endl;

  else
    cout << "graceful exit from server: " << server_name << endl;

  close(login_fd);
  close(server_fd);
  unlink(client_name.c_str());
  pthread_mutex_destroy(&mt);
  _exit(0);
}

int main() {

  sf::RenderWindow window(sf::VideoMode(700, 500), "CLIENT");

  Chat chat;
  Login login;

  signal(SIGABRT, cleanup); // abort call
  signal(SIGINT, cleanup);  // ctrl+c from terminal
  signal(SIGHUP, cleanup);  // killing process by closing terminal close
  signal(SIGSEGV, cleanup); // segmentation fault

  pthread_t message_thread;

  pthread_mutex_init(&mt, NULL);

  pthread_create(&message_thread, NULL, await_messages, (void *)&chat);

  while (window.isOpen()) {
    sf::Event event;

    while (window.pollEvent(event)) {
      if (state == State::LOGIN)
        login.handleLoginInput(window, event);
      else
        chat.handleChatInput(window, event);
    }
    if (state == State::LOGIN)
      login.render(window);
    else
      chat.render(window);
  }

  return 0;
}
