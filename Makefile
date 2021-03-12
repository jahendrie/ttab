#===============================================================================
#		Makefile for ttab.c
#		Simple terminal-based adding machine
#===============================================================================

CC=gcc
PREFIX=/usr
FILES=ttab.c
OPTFLAGS=-g -Wall
OUTPUT=ttab
SRC=src
DOC=doc
MANPAGE=ttab.1.gz
OUTPUTDIR=$(PREFIX)/bin
MANPATH=$(PREFIX)/share/man/man1

all: $(SRC)/ttab.c
	$(CC) $(OPTFLAGS) -o $(OUTPUT) $(SRC)/$(FILES)

install:
	install $(OUTPUT) -D $(OUTPUTDIR)/$(OUTPUT)
	install $(DOC)/$(MANPAGE) -D $(MANPATH)/$(MANPAGE)

uninstall:
	rm -f $(OUTPUTDIR)/$(OUTPUT)
	rm -f $(MANPATH)/$(MANPAGE)

clean:
	rm -f $(OUTPUT)
