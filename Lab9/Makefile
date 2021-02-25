#######################################################################
#
# Copyright (C) 2018 David C. Harrison. All right reserved.
#
# You may not use, distribute, publish, or modify this code without 
# the express written permission of the copyright holder.
#
#######################################################################

CFLAGS = -Wall -Isrc -lpthread -lcrypt -g
CC = g++ -std=c++17

SRC=$(wildcard src/*.cc)

all: crack

crack: $(SRC) crack.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	@rm crack

submit: clean 
	@tar czvf ~/CMPS109-Lab9.tar.gz \
	    Makefile src/*.cc src/*.h 
