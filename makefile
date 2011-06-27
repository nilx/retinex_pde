# Copyright 2009-2011 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

# source code, C language
CSRC	= io_png.c norm.c retinex_pde_lib.c retinex_pde.c
# source code, all languages (only C here)
SRC	= $(CSRC)
# object files (partial compilation)
OBJ	= $(CSRC:.c=.o)
# binary executable programs
BIN	= retinex_pde

# standard C compiler optimization options
COPT	= -O3 -funroll-loops -fomit-frame-pointer -ffast-math
# complete C compiler options
CFLAGS	+= -ansi -pedantic -Wall -Wextra -Werror -pipe $(COPT)
# linker options
LDFLAGS	+= -lpng -lfftw3f

# uncomment this part to use the multi-threaded DCT
#CPPFLAGS	= -DFFTW_NTHREADS=8
#LDFLAGS	+= -lfftw3f_threads -lpthread

# default target: the binary executable programs
default: $(BIN)

# partial C compilation xxx.c -> xxx.o
%.o	: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

# final link
retinex_pde	: $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

# cleanup
.PHONY	: clean distclean
clean	:
	$(RM) $(OBJ)
distclean	: clean
	$(RM) $(BIN)
	$(RM) -r srcdoc

################################################
# extra tasks

PROJECT	= retinex_pde
DATE	= $(shell date -u +%Y%m%d)
RELEASE_TAG   = 0.$(DATE)

.PHONY	: srcdoc lint beautify test release
# source documentation
srcdoc	: $(SRC)
	doxygen doc/doxygen.conf
# code cleanup
beautify	: $(CSRC)
	for FILE in $^; do \
		expand $$FILE | sed 's/[ \t]*$$//' > $$FILE.$$$$ \
		&& indent -kr -i4 -l78 -nut -nce -sob -sc \
			$$FILE.$$$$ -o $$FILE \
		&& rm $$FILE.$$$$; \
	done
# static code analysis
lint	: $(CSRC)
	for FILE in $^; do \
		clang --analyze -ansi -I. $$FILE || exit 1; done;
	for FILE in $^; do \
		splint -ansi-lib -weak -I. $$FILE || exit 1; done;
	$(RM) *.plist
# code tests
test	: $(CSRC)
	sh -e test/run.sh && echo SUCCESS || ( echo ERROR; return 1)
# release tarball
release	: beautify lint test distclean
	git archive --format=tar --prefix=$(PROJECT)-$(RELEASE_TAG)/ HEAD \
	        | gzip > ../$(PROJECT)-$(RELEASE_TAG).tar.gz
