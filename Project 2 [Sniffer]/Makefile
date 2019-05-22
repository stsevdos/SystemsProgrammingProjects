OBJS 	= main.o dataTypes.o dataTypesTLD.o handlers.o parser.o
SOURCE	= main.c dataTypes.c dataTypesTLD.c handlers.c parser.c
HEADER  = dataTypes.h dataTypesTLD.h handlers.h parser.h
OUT  	= sniffer
CC	= gcc
CFLAGS   = -g -c 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT)

main.o: main.c
	$(CC) $(CFLAGS) main.c

dataTypes.o: dataTypes.c
	$(CC) $(CFLAGS) dataTypes.c
	
dataTypesTLD.o: dataTypesTLD.c
	$(CC) $(CFLAGS) dataTypesTLD.c

handlers.o: handlers.c
	$(CC) $(CFLAGS) handlers.c

parser.o: parser.c
	$(CC) $(CFLAGS) parser.c

clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)
