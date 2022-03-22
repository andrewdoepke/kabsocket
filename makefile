.PHONY: server client

all:
	make server
	make client

server:
	g++ srv_new.cpp -o server -lboost_system -lpthread

client:
	g++ cli_new.cpp -o client -lboost_system -lpthread

clean:
	rm server client
