CC=g++
CFLAGS= -Wall -Os -I eigen
all: main.o
	$(CC) main.o -o MinimalFEM -lginac

main.o: main.cpp
	$(CC) -c $(CFLAGS)  main.cpp -DRECT -DCURVE

clean:
	rm -rf *.o MinimalFEM
