# Scramble Makefile
#
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.

CC		= gcc 
CFLAGS		= -Wall -Wstrict-prototypes  # DON'T USE OPTIMIZATION HERE!!!
OBJS		= getopt.o scramble.o
SRCS		= getopt.c scramble.c

scramble: $(OBJS)
	$(CC) -o scramble $(OBJS)

clean:
	rm -f *.o

obj: $(OBJS)

$(OBJS): 
