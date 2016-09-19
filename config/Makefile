TARGETDIR=/usr/share/prometheus/config
LANGUAGES=de en


compile:

install:
	@for lang in $(LANGUAGES) ; \
	do \
	    echo "Installing game configuration files for language $$lang..." ; \
	    mkdir -p $(DESTDIR)$(TARGETDIR)/$$lang ; \
	    install -m 644 $$lang/*.cfg $(DESTDIR)$(TARGETDIR)/$$lang ; \
	done

distdir:
	cp Makefile $(DISTDIR)
	cp -r $(LANGUAGES) $(DISTDIR)

clean:

mrproper:
	rm -f *~
