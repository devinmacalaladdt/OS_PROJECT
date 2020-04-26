CC=gcc
CFLAGS=-g -Wall -D_FILE_OFFSET_BITS=64
LDFLAGS=-lfuse

OBJ=tfs.o block.o

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o -lm $@

tfs: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o -lm tfs

.PHONY: clean
clean:
	rm -f *.o tfs

