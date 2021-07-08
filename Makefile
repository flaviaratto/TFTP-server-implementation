#Makefile for TCP echo client-server service for Team 6 in ECEN602

output: server.o
	gcc server.o -o server

server.o: server.c
	gcc -c server.c

clean:
	rm -f server *.o core