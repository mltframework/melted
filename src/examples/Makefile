include ../../config.mak

CXXFLAGS += -Wall -g  -I.. -I../melted++ `pkg-config mlt++ --cflags`
LDFLAGS  += -L../melted++ -lmelted++ -L../melted -lmelted -L../mvcp -lmvcp `pkg-config mlt++ --libs`
CC=c++

all:		server

server:		server.o

server.o:	server.cpp

clean:
	$(RM) server server.o

distclean: clean

install:

uninstall:
