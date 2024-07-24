client: client.o
	export LD_LIBRARY_PATH=./SFML-config/lib && g++ client.o -pthread -o client.exe -L"./SFML-config/lib" -lsfml-graphics -lsfml-window -lsfml-system && ./client.exe

client.o: client.cpp
	g++ -c client.cpp -pthread -I"./SFML-config/include"

server: server.cpp
	g++ server.cpp -pthread -o server.exe && ./server.exe


clean: 
	rm -rf *.exe *.o