include ../../../config.mak

TARGET = ../libmltmvcp$(LIBSUF)

OBJS = factory.o \
	   consumer_mvcp.o

CFLAGS += -I../.. 

LDFLAGS += -L../../mvcp -lmvcp
LDFLAGS += -L../../framework -lmlt

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) 

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(libdir)/mlt"

ifneq ($(wildcard .depend),)
include .depend
endif
