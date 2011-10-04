# Copyright 2009-2011 Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

# source code
SRC	= io_png.c norm.c retinex_pde_lib.c retinex_pde.c
# headers-only code
HDR	= debug.h
# object files (partial compilation)
OBJ	= $(SRC:.c=.o)
# binary executable programs
BIN	= retinex_pde

# standard C compiler optimization options
COPT	= -O3 -DNDEBUG
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
# dev tasks
PROJECT	= retinex_pde
-include	makefile.dev
