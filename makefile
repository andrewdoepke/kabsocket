.PHONY: server client

all:
	make server
	make client

server:
	g++ -std=c++17 server_async.cpp base64.cpp -o server -lboost_system -lpthread

client:
	g++ client_async.cpp base64.cpp -o client -lboost_system -lpthread

clean:
	rm server client
