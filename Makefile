CC = gcc
CGLAGS = -Wall -Wextra

SRCS = file_system.c
OUT = file_system

all: compile

compile:
	$(CC) $(CFLAGS) $(SRCS) -o $(OUT)

build: compile

clean:
	rm -f $(OUT)

.PHONY: all compile build clean
