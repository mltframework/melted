include ../../config.mak

TARGET = melted

ifneq ($(targetos), Darwin)
LIBNAME = libmelted$(LIBSUF)
LIBTARGET = $(LIBNAME).$(version)
LIBSONAME = $(LIBNAME).$(soversion)
SHFLAGS += -Wl,-soname,$(LIBSONAME)
else
LIBNAME = libmelted$(LIBSUF)
LIBTARGET = libmelted.$(version)$(LIBSUF)
LIBSONAME = libmelted.$(soversion)$(LIBSUF)
SHFLAGS += -install_name $(libdir)/$(LIBSONAME) -current_version $(version) -compatibility_version $(soversion)
endif

APP_OBJS = melted.o

LIB_OBJS = melted_log.o \
	   melted_server.o \
	   melted_connection.o \
	   melted_local.o \
	   melted_unit.o \
	   melted_commands.o \
	   melted_unit_commands.o

INCS = melted_server.h \
	   melted_local.h \
	   melted_log.h

OBJS = $(APP_OBJS) $(LIB_OBJS)

CFLAGS += -I.. $(RDYNAMIC)

LDFLAGS += -L../mvcp -lmvcp
LDFLAGS += -lpthread

SRCS := $(OBJS:.o=.c)

all: 		$(TARGET)

$(TARGET): 	$(APP_OBJS) $(LIBTARGET)
			$(CC) -o $@ $(APP_OBJS) -L. -lmelted $(LDFLAGS)

$(LIBTARGET):	$(LIB_OBJS)
			$(CC) $(SHFLAGS) -o $@ $(LIB_OBJS) $(LDFLAGS)
			ln -sf $(LIBTARGET) $(LIBNAME)
			ln -sf $(LIBTARGET) $(LIBSONAME)

depend:		$(SRCS)
			$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
			rm -f .depend

clean:	
			rm -f $(OBJS) $(TARGET) $(LIBNAME) $(LIBTARGET) $(LIBSONAME)

install:	all
	install -d "$(DESTDIR)$(bindir)"
	install -c -s -m 755 $(TARGET) "$(DESTDIR)$(bindir)"
	install -m 755 $(LIBTARGET) $(DESTDIR)$(libdir)
	ln -sf $(LIBTARGET) $(DESTDIR)$(libdir)/$(LIBSONAME)
	ln -sf $(LIBTARGET) $(DESTDIR)$(libdir)/$(LIBNAME)
	mkdir -p "$(DESTDIR)$(prefix)/include/mlt/melted"
	install -m 644 $(INCS) "$(DESTDIR)$(prefix)/include/mlt/melted"

uninstall:
	rm -f "$(DESTDIR)$(bindir)/$(TARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(LIBTARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(LIBSONAME)"
	rm -f "$(DESTDIR)$(libdir)/$(LIBNAME)"
	rm -rf "$(DESTDIR)$(prefix)/include/mlt/melted"

ifneq ($(wildcard .depend),)
include .depend
endif
