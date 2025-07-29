.POSIX:
.SUFFIXES:
.PHONY: format clean

CC = c99
BUILD_TYPE = debug
OBJS = cdoc.o

ifeq ($(BUILD_TYPE),release)
	CFLAGS = -O2 -Wall -Wextra -std=c99
else ifeq ($(BUILD_TYPE),debug)
	CFLAGS = -g -O0 -Wall -Werror -Wextra -std=c99
endif

cdoc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

format:
	clang-format -i cdoc.c

clean:
	rm -f cdoc $(OBJS) *.html

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $<
