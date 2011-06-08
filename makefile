# Copyright 2009-2011 IPOL Image Processing On Line http://www.ipol.im/
# Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

CSRC	= io_png.c norm.c retinex_pde_lib.c retinex_pde.c

SRC	= $(CSRC)
OBJ	= $(CSRC:.c=.o)
BIN	= retinex_pde

COPT	= -O3 -funroll-loops -fomit-frame-pointer -ffast-math
CFLAGS	+= -ansi -pedantic -Wall -Wextra -Werror $(COPT)
LDFLAGS	+= -lpng -lfftw3f

# uncomment this part to use the multi-threaded DCT
#CPPFLAGS	= -DFFTW_NTHREADS=8
#LDFLAGS	+= -lfftw3f_threads -lpthread

default: $(BIN)

%.o	: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

$(BIN)	: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY	: clean distclean
clean	:
	$(RM) $(OBJ)
distclean	: clean
	$(RM) $(BIN)

# extra tasks
.PHONY	: lint beautify test release
lint	: $(CSRC)
	splint -weak $^;
beautify	: $(CSRC)
	for FILE in $^; do \
		expand $$FILE | sed 's/[ \t]*$$//' > $$FILE.$$$$ \
		&& indent -kr -i4 -l78 -nut -nce -sob -sc \
			$$FILE.$$$$ -o $$FILE \
		&& rm $$FILE.$$$$; \
	done
test	: $(CSRC)
	sh -e test/run.sh && echo SUCCESS || ( echo ERROR; return 1)
release	:
	git archive --format=tar --prefix=retinex_pde/ HEAD \
	        | gzip > ../retinex_pde.tar.gz
