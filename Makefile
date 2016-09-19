SUBDIRS = src images config infos rulebook graphics
DOCFILES = COPYING README.LICENSE README TODO 
compile install clean mrproper distdir:
	@$(MAKE) --no-print-directory $@-local
	@for subdir in $(SUBDIRS) ; \
	do \
	    [ -z "$(DISTDIR)" ] || mkdir -p $(DISTDIR)/$$subdir ; \
	    $(MAKE) DISTDIR=$(DISTDIR)/$$subdir -C $$subdir $@ ; \
	done

compile-local:

install-local:
	@for lang in de en ; do \
		echo "Creating link structur for html printouts language $$lang..." ; \
		htmldir=$(DESTDIR)/usr/share/prometheus/html/$$lang ; \
		install -d $$htmldir ; \
		ln -sf ../../images/full-size $$htmldir/f ; \
		ln -sf ../../images/half-size $$htmldir/s ; \
		ln -sf ../../images/icons     $$htmldir/i ; \
		ln -sf ../../images/roads     $$htmldir/r ; \
		ln -sf ../../rulebook/$$lang  $$htmldir/a ; \
	done
	install -m 644 $(DOCFILES) $(DESTDIR)/usr/share/prometheus

distdir-local:
	cp Makefile $(DOCFILES) $(DISTDIR)

clean-local:
	rm -rf prometheus-*.*.*

mrproper-local: clean-local
	rm -f prometheus-*.*.*.tar.gz
	rm -f prometheus-*.*.*-*.*.rpm *~

rpm: compile
	@VERSION="$$(sed -n '/Version:/s/.*: *\(.*\)/\1/p' prometheus.spec)" && \
	 RELEASE="$$(sed -n '/Release:/s/.*: *\(.*\)/\1/p' prometheus.spec)" && \
	 DISTDIR=prometheus-$$VERSION && \
	 rm -rf $$DISTDIR && \
	 mkdir -p $$DISTDIR && \
	 make DISTDIR=$(CURDIR)/$$DISTDIR distdir && \
	 tar czvf $$DISTDIR.tar.gz $$DISTDIR && \
	 cp $$DISTDIR.tar.gz /usr/src/packages/SOURCES && \
	 cp prometheus.spec /usr/src/packages/SPECS && \
	 rpm -ba prometheus.spec && \
	 cp -pv /usr/src/packages/RPMS/*/prometheus-$$VERSION-$$RELEASE.*.rpm . && \
	 cp -pv /usr/src/packages/SRPMS/prometheus-$$VERSION-$$RELEASE.src.rpm .
