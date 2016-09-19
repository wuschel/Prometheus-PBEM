LANGUAGE = en

include ../Makefile.rulebook

anleitung.mhtml: ../anleitung.mhtml
	egrep '^@' $< | sed 's/^@//' > $@
