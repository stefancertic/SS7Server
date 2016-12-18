server2: server2.o stabla.o
	gcc -pthread -o server2 server2.o  -lsctp stabla.o `mysql_config --libs`
	
server2.o: server2.c stabla.h
	gcc -c -o server2.o `mysql_config --cflags` -lsctp server2.c 

server: server.o stabla.o
	gcc -pthread -o server server.o  -lsctp stabla.o `mysql_config --libs`
	
server.o: server.c stabla.h
	gcc -c -o server.o `mysql_config --cflags` -lsctp server.c 

stabla.o: stabla.c stabla.h
	gcc  -c -o stabla.o stabla.c

