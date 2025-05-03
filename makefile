CC = gcc
CFLAGS = -Wall -Wextra -Ivendor/ncurses/build/include
LDFLAGS = -static -Lvendor/ncurses/build/lib -lncurses
TARGET = main
SRC = main.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
