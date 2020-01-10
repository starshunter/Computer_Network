threadpool_client: threadpool_server.o threadpool.o
	g++ -o threadpool_client threadpool_server.o threadpool.o -lpthread

threadpool.o: threadpool.c
	g++ -o threadpool.o -c threadpool.c

threadpool_server.o: client.c threadpool.h
	g++ -o threadpool_server.o -c client.c

clean:
	rm -f threadpool_client threadpool.o threadpool_server.o
