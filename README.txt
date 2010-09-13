% Retinex PDE

# ABOUT

* Author    : Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
* Copyright : (C) 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
* Licence   : GPL v3+, see GPLv3.txt

# OVERVIEW

This source code provides an implementation of the Retinex theory by a
Poisson equation, as described in IPOL
    http://www.ipol.im/pub/algo/lmps_retinex_poisson_equation/.

This program reads a PNG image, computes its laplacian, performs a
DFT, then solves the Poisson equation in the Fourier space and
performs an inverse DFT.
The result is normalized and written as a PNG image.

The same normalization is also performed on a unmodified image, for
comparison. The normalization method is the "simplest color balance",
as described in IPOL
    http://www.ipol.im/pub/algo/lmps_simplest_color_balance/.

# REQUIREMENTS

The code is written in ANSI C, and should compile on any system with
an ANSI C compiler.

The libpng header and libraries are required on the system for
compilation and execution. See http://www.libpng.org/pub/png/libpng.html

The fftw3 header and libraries are required on the system for
compilation and execution. See http://www.fftw.org/

# COMPILATION

Simply use the provided makefile, with the command `make`.

Alternatively, you can manually compile
    cc io_png.c normalize_histo_lib.c retinex_pde_lib.c retinex_pde.c \
        -lpng -lfftw3f -o retinex_pde

Multi-threading is possible, with the FFTW_NTHREADS parameter:
    cc io_png.c normalize_histo_lib.c retinex_pde_lib.c retinex_pde.c \
        -DFFTW_NTHREADS -lpng -lfftw3f -lfftw3f_threads -o retinex_pde

You can optionally disable the normalization with `make WITHOUT_NORM=1`
or by adding the `-DWITHOUT_NORM` option to the compiler command.

# USAGE

This program takes 4 parameters: `retinex_pde T in norm rtnx`

* `T`       : retinex threshold [0...255[
* `in`      : input image
* `norm`    : normalized output image
* `rtnx`    : retinex output image

# ABOUT THIS FILE

Copyright 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
