TARGETDIR=/usr/share/prometheus/infos
LANGUAGES=de en

compile:

install:
	@for lang in $(LANGUAGES) ; \
	do \
	    echo "Installing infos for language $$lang..." ; \
	    mkdir -p $(DESTDIR)$(TARGETDIR)/$$lang ; \
	    install -m 644 $$lang/*.inf $(DESTDIR)$(TARGETDIR)/$$lang ; \
	done

distdir:
	cp -pr Makefile $(LANGUAGES) $(DISTDIR)

clean:

mrproper:
	rm -f *~
