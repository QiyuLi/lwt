BIN=main
OBJS=main.o lwt_tramp.o lwt.o
CC=gcc
CFLAGS=-I. -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
#DEFINES=

all:	$(BIN)
	@echo "======  Makefile Finished  ======"

$(BIN):	$(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

%.o:	%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

%.o:	%.S
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

clean:
	rm $(BIN) $(OBJS)

test:	$(BIN)
	@./main
