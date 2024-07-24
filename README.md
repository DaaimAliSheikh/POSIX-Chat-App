**DOWNLOAD SFML FOR LINUX**
***ADD THE SFML INCLUDE AND LIB FOLDERS OF SFML IN SFML-config***

In OS-PROJECT-SOURCE directory:

make server  // compiles and runs server program
make client  // compiles and runs client program
make clean      // removes exe files



NOTE: Incase client compilation fails, add this env variable before compiling client: export LD_LIBRARY_PATH=./SFML-config/lib
