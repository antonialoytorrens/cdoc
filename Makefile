.POSIX:
.SUFFIXES:
.PHONY: format clean

CC = c99
CFLAGS = -O0 -g
OBJS = cdoc.o

cdoc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

format:
	clang-format -i *.c

clean:
	rm -f cdoc $(OBJS)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $<
