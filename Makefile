CC		:= gcc
CFLAGS	:= -Wall -g
#LFLAGS	:= -lpthread

OBJS	+= main.o
TARGET	:= logbuf-praser

.PHONY: clean

all:	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LFLAGS)

clean:
	rm -rf $(OBJS) $(TARGET)
