CC     = gcc
C_FILE = chatclient.c
TARGET = $(patsubst %.c, %, $(C_FILE))
CFLAGS = -g -Wall -Werror -pedantic-errors

.PHONY: all clean
all: 
	$(CC) $(CFLAGS) $(C_FILE) -o $(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
