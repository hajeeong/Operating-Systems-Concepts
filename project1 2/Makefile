CC = g++
CFLAGS = -Wall -std=c++11 -g

all: driver encryption logger

driver: driver.cpp
	$(CC) $(CFLAGS) -o driver driver.cpp

encryption: encryption.cpp
	$(CC) $(CFLAGS) -o encryption encryption.cpp

logger: logger.cpp
	$(CC) $(CFLAGS) -o logger logger.cpp

clean:
	rm -f driver encryption logger *.o

.PHONY: all clean