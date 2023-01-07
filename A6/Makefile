CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=cachesim.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=cachesim

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)