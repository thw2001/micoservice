CC = gcc
CFLAGS = -fPIC
INCLUDE = -I../include

rpcclient = librpcclient.so
rpcserver = librpcserver.so

%.o : %.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

all: $(rpcclient) $(rpcserver)

$(rpcclient) : context.o net.o serial.o client.o
	$(CC) $(CFLAGS) -shared $^ -o $@

$(rpcserver) : context.o evloop.o hash_table.o net.o serial.o server.o
	$(CC) $(CFLAGS) -shared $^ -o $@

clean:
	rm -f *.o
	rm -f *.so
