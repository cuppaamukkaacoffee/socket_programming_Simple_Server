CC=gcc
CFLAGS=-o
TARGET=server
all: $(TARGET)
.PHONY: all
%:
	$(CC) -o $@ $@.c

clean:
	rm -f $(TARGET)