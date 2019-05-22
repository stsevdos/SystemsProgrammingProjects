OBJS 	= main.o oracleFunctions.o netFunctions.o
SOURCE	= main.c oracleFunctions.c netFunctions.c
HEADER  = oracleFunctions.h netFunctions.h hash.h oracle.h
LIBS = liboracle_v3.a libhash.a
OUT  	= invoke-oracle-multithreaded
CC	= gcc
CFLAGS   = -g -c
LIBFLAGS =  -L . -loracle_v3 -lhash -lpthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LIBFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) main.c

oracleFunctions.o: oracleFunctions.c
	$(CC) $(CFLAGS) oracleFunctions.c

netFunctions.o: netFunctions.c
	$(CC) $(CFLAGS) netFunctions.c

clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)