CSRC	= io_tiff.c normalize_histo_lib.c retinex_pde_lib.c retinex_pde.c

SRC	= $(CSRC)
OBJ	= $(CSRC:.c=.o)
BIN	= retinex_pde

COPT	= -O3 -funroll-loops -fomit-frame-pointer
CFLAGS	+= -std=c99 -Wall -Wextra -Werror $(COPT)

default: $(BIN)

%.o	: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN)	: $(OBJ)
	$(CC) $(CFLAGS) -o $@ -ltiff -lfftw3f $^

.PHONY	: clean distclean
clean	:
	$(RM) $(OBJ)
distclean	: clean
	$(RM) $(BIN)
