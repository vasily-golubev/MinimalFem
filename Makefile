CC=g++
CFLAGS= -Wall -Os -I eigen
all: main.o
	$(CC) main.o -o MinimalFEM

main.o: main.cpp
	$(CC) -c $(CFLAGS)  main.cpp -DRECT

clean:
	rm -rf *.o MinimalFEM
