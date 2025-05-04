CC = gcc
CFLAGS = -Wall -Wextra -Ivendor/ncurses/build/include
LDFLAGS = -static -Lvendor/ncurses/build/lib -lncursesw -ltinfow
TARGET = main
SRC = main.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "Compiling with command: $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
