CC=gcc
CFLAGS=-lpthread -Wall -Werror -g 
SOURCES=kindel_dave_final.c
EXECUTABLE=final

all:
	$(CC) $(SOURCES) $(CFLAGS) -o $(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE)
