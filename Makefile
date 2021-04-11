all : bin/server bin/client

bin/server : src/server.c
	@mkdir -p bin
	gcc -o bin/server src/server.c -lpthread
bin/client : src/client.c
	gcc -o bin/client src/client.c -lpthread

clean :
	rm -r bin
