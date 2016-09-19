TARGETDIR=/usr/share/prometheus/graphics

compile:

install:
	mkdir -p $(DESTDIR)$(TARGETDIR)
	install -m 644 ra *.gra $(DESTDIR)$(TARGETDIR)

distdir:
	cp -p Makefile ra *.gra $(DISTDIR)

clean:

mrproper:
	rm -f *~
