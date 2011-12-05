CC = gcc
CFLAGS = -g -Wall -DDEBUG 
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

crc32.o: crc32.c
	$(CC) $(CFLAGS) -c crc32.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

sbuf.o: sbuf.c sbuf.h
	$(CC) $(CFLAGS) -c sbuf.c

proxy: proxy.o csapp.o crc32.o sbuf.o cache.o

submit:
	(make clean; cd ..; tar czvf proxylab.tar.gz proxylab-handout)

clean:
	rm -f *~ *.o proxy core

