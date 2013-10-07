BIN=main
OBJS=main.o lwt_tramp.o lwt.o
CC=gcc
CFLAGS=-g -I. -Wall -Wextra
#DEFINES=

$(BIN):	$(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

%.o:	%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

%.o:	%.S
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

clean:
	rm $(BIN) $(OBJS)

test:
	./main
