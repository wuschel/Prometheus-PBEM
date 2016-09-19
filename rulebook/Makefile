TARGETDIR=/usr/share/prometheus/rulebook
LANGUAGES=de en

compile:
	@for lang in $(LANGUAGES) ; \
	do \
		$(MAKE) -C $$lang --no-print-directory ; \
	done

install:
	@for lang in $(LANGUAGES) ; \
	do \
	    echo "Installing html rulebook for language $$lang..." ; \
	    mkdir -p $(DESTDIR)$(TARGETDIR)/$$lang ; \
	    install -m 644 $$lang/*.htm $(DESTDIR)$(TARGETDIR)/$$lang ; \
	    install -m 644 images/*.png $(DESTDIR)$(TARGETDIR)/$$lang ; \
	done

distdir:
	cp -pr Makefile Makefile.rulebook anleitung.mhtml index-??.mhtml \
	  mhtml2html mhtml2html.sed zerlege_in_befehle zerlege_in_kapitel images $(DISTDIR)
	mkdir -p $(DISTDIR)/de
	mkdir -p $(DISTDIR)/en
	$(MAKE) -C de DISTDIR=$(DISTDIR)/de distdir
	$(MAKE) -C en DISTDIR=$(DISTDIR)/en distdir

clean:

mrproper:
	rm -f *~
