CSRC	= io_tiff.c algo_normalize.c algo_retinex_pde.c retinex_pde.c

SRC	= $(CSRC)
OBJ	= $(CSRC:.c=.o)
BIN	= retinex_pde

COPT	= -O3 -funroll-loops -fomit-frame-pointer
CFLAGS	+= -std=c99 -Wall -Wextra -Wno-unknown-pragmas -Werror $(CXXOPT)

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
