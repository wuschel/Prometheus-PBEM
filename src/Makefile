include Makefile.config

# Optimierung ein oder ausschalten geht hier
ifeq ($(DEBUG),yes)
OPTIMIZE        = -g
else
OPTIMIZE        = -O9 -fomit-frame-pointer -fstrength-reduce -w
endif

LIBS            = -lm -lncurses -lstdc++
CXX		= g++
LD		= g++
INCDIRS		= -I.
CXXFLAGS 	= $(INCDIRS) $(OPTIMIZE) -Wall "-DL(de,en)=$(lang)" "-DNUR_TESTVERSION=$(TESTVERSION)"
LDFLAGS		= $(LIBDIRS) $(LIBS)
SRCS		= $(shell ls *.cc)
OBJS		= $(patsubst %.cc,temp-$(lang)/%.o,$(SRCS))
DEPS		= $(patsubst %.cc,temp-$(lang)/%.d,$(SRCS))

compile:
	@for lang in $(LANGUAGES) ; \
	do \
		$(MAKE) --no-print-directory lang=$$lang prometheus-$$lang; \
	done

alles-$(lang).cxx: *.cc
	cat *.cc > $@

prometheus-$(lang): alles-$(lang).cxx
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

install:
	mkdir -p $(DESTDIR)/usr/bin
	install prometheus-* -m 755 $(DESTDIR)/usr/bin

distdir:
	cp Makefile.config Makefile.dos Makefile *.cc *.h $(DISTDIR)

clean:
	rm -f alles-*.cxx

mrproper: clean
	rm -f prometheus-* *~
