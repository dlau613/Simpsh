CC = gcc
CLAGS = -g -Wall -Wextra -Wno-unused - Werror
LAB = 1
DISTDIR = lab1-$(USER)

all: simpsh

TESTS = $(wildcard test*.sh)
TEST_BASES = $(subst .sh,,$(TESTS))

Simpsh_SOURCES = \
	simpsh.c

Simpsh_OBJECTS = $(subst .c,.o,$(Simpsh_SOURCES))

DIST_SOURCES = \
	$(Simpsh_SOURCES) makefile \
	$(TESTS) README

simpsh: $(Simpsh_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(Simpsh_OBJECTS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES) 
	rm -fr $(DISTDIR)
	tar -czf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	mv $@.tmp $@

check: $(TEST_BASES)

$(TEST_BASES): simpsh
	./$@.sh

clean:
	rm -fr *.o *~ *.bak *.tar.gz core *.core *.tmp simpsh $(DISTDIR)

.PHONY: all dist check $(TEST_BASES) clean


