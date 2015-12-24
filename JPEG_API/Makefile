
OBJECTS = JPGApi.o

SRCS = $(OBJECTS:.o=.c)
DEPENDENCY = JPGApi.h

CC = /usr/local/arm/4.3.2/bin/arm-linux-gcc-4.3.2
CFLAGS = -g -c -Os -Wall
INC = -I../Common
#COMPILER_NEW = y 
ifdef COMPILER_NEW
CC = /usr/local/arm/4.3.2/bin/arm-linux-gcc-4.3.2
else 
#CC = /usr/local/arm/4.3.2/bin/arm-linux-gcc-4.3.2
endif

TARGET = app

$(TARGET) : $(OBJECTS)
		#$(CC) -o $(TARGET) $(OBJECTS)

.c.o :
		$(CC) $(INC) $(CFLAGS) $<

clean :
		rm -rf $(OBJECTS) $(TARGET) core

JPGApi.o : $(DEPENDENCY) JPGApi.c

