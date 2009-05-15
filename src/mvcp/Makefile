include ../../config.mak

ifneq ($(targetos), Darwin)
NAME = libmvcp$(LIBSUF)
TARGET = $(NAME).$(version)
SONAME = $(NAME).$(soversion)
SHFLAGS += -Wl,-soname,$(SONAME)
else
NAME = libmvcp$(LIBSUF)
TARGET = libmvcp.$(version)$(LIBSUF)
SONAME = libmvcp.$(soversion)$(LIBSUF)
SHFLAGS += -install_name $(libdir)/$(SONAME) -current_version $(version) -compatibility_version $(soversion)
endif

OBJS = mvcp.o \
	   mvcp_notifier.o \
	   mvcp_parser.o \
	   mvcp_response.o \
	   mvcp_status.o \
	   mvcp_tokeniser.o \
	   mvcp_util.o \
	   mvcp_remote.o \
	   mvcp_socket.o

INCS = mvcp.h \
	   mvcp_notifier.h \
	   mvcp_parser.h \
	   mvcp_remote.h \
	   mvcp_response.h \
	   mvcp_socket.h \
	   mvcp_status.h \
	   mvcp_tokeniser.h \
	   mvcp_util.h

SRCS := $(OBJS:.o=.c)

CFLAGS += -I.. $(RDYNAMIC)

LDFLAGS += -L../framework -lmlt -lpthread

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)
		ln -sf $(TARGET) $(NAME)
		ln -sf $(TARGET) $(SONAME)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET) $(NAME) $(SONAME)

install: 	all
	install -m 755 $(TARGET) $(DESTDIR)$(libdir)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(SONAME)
	ln -sf $(TARGET) $(DESTDIR)$(libdir)/$(NAME)
	mkdir -p "$(DESTDIR)$(prefix)/include/mlt/mvcp"
	install -m 644 $(INCS) "$(DESTDIR)$(prefix)/include/mlt/mvcp"

uninstall:
	rm -f "$(DESTDIR)$(libdir)/$(TARGET)"
	rm -f "$(DESTDIR)$(libdir)/$(SONAME)"
	rm -f "$(DESTDIR)$(libdir)/$(NAME)"
	rm -rf "$(DESTDIR)$(prefix)/include/mlt/mvcp"

ifneq ($(wildcard .depend),)
include .depend
endif
