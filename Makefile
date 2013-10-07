BIN=main
OBJS=main.o lwt_tramp.o
CC=gcc
CFLAGS=-g -I. -Wall -Wextra
#DEFINES=

all:	$(BIN)

main.o:	main.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

lwt_tramp.o:	lwt_tramp.S
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

clean:
	rm $(BIN) $(OBJS)

test:
	./main
