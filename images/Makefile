SUBDIRS = icons full-size half-size roads
TARGETDIR = /usr/share/prometheus/images

compile:

install:
	mkdir -p $(DESTDIR)$(TARGETDIR)
	cp -pr $(SUBDIRS) $(DESTDIR)$(TARGETDIR)

distdir:
	cp -pr Makefile $(SUBDIRS) $(DISTDIR)

clean:

mrproper:
	rm -f *~
