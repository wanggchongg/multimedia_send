OBJECTS = matrix.o    \
          raptorcode.o

SRCS = $(OBJECTS:.o=.c)
DEPENDENCY = def.h   \
             matrix.h  \
             raptorcode.h

CC = /usr/local/arm/4.3.2/bin/arm-linux-gcc-4.3.2
CFLAGS =-g -c -lm -Os -Wall -std=gnu99
INC = -I../Common

TARGET = app

$(TARGET) : $(OBJECTS)

.c.o :
		$(CC) $(INC) $(CFLAGS) $<

clean :
		rm -rf $(OBJECTS) $(TARGET) core

raptorcode.o : $(DEPENDENCY) raptorcode.c
matrix.o : $(DEPENDENCY) matrix.c

#raptorcode.o :raptorcode.h matrix.o def.h 
#		$(CC) -c -std=gnu99 raptorcode.c 

#matrix.o :matrix.h def.h
#		$(CC) -c -std=gnu99 matrix.c

#clean :
#	rm matrix.o raptorcode.o
