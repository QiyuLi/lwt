BIN=main
OBJS=main.o lwt_tramp.o lwt.o
CC=gcc
#CFLAGS=-ggdb3 -I. -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
CFLAGS=-O3 -ggdb3 -I. -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
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


