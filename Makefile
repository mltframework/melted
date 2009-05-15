SUBDIRS = src/mvcp \
		  src/melted \
		  src/melted++ \
		  src/mvcp-client \
		  src/mvcp-console \
		  src/modules

all clean:
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -s -C $$subdir depend || exit 1; \
		$(MAKE) -C $$subdir $@ || exit 1; \
	done

distclean:
	rm mlt-config packages.dat; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) -C $$subdir $@ || exit 1; \
	done; \
	rm config.mak;

dist-clean: distclean

include config.mak

install:
	install -d "$(DESTDIR)$(prefix)/bin"
	install -d "$(DESTDIR)$(prefix)/include"
	install -d "$(DESTDIR)$(libdir)"
	install -d "$(DESTDIR)$(libdir)/mlt"
	install -d "$(DESTDIR)$(libdir)/pkgconfig"
	install -d "$(DESTDIR)$(prefix)/share/mlt"
	install -c -m 644 *.pc "$(DESTDIR)$(libdir)/pkgconfig"
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
	done; \
	if test -z "$(DESTDIR)"; then \
	  /sbin/ldconfig -n "$(DESTDIR)$(libdir)" 2> /dev/null || true; \
	fi

uninstall:
	rm -f "$(DESTDIR)$(libdir)"/pkgconfig/mlt-*.pc
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		$(MAKE) DESTDIR=$(DESTDIR) -C $$subdir $@ || exit 1; \
	done

dist:
	git archive --format=tar --prefix=melted-$(version)/ HEAD | gzip >melted-$(version).tar.gz
