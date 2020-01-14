threadpool_server: threadpool_server.o threadpool.o
	g++ -o threadpool_server threadpool_server.o threadpool.o -lpthread -l ssl -l crypto

threadpool.o: threadpool.c
	g++ -o threadpool.o -c threadpool.c

threadpool_server.o: threadpool_server.cpp threadpool.h
	g++ -o threadpool_server.o -c threadpool_server.cpp

clean:
	rm -f threadpool_server threadpool.o threadpool_server.o
