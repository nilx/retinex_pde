% Retinex PDE

# ABOUT

* Author    : Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
              Jose-Luis Lisani <joseluis.lisani@uib.es>
* Copyright : (C) 2009-2011 IPOL Image Processing On Line http://www.ipol.im/
* License   : GPL v3+, see GPLv3.txt

# OVERVIEW

This source code provides an implementation of the Retinex theory by a
Poisson equation, as described in IPOL
    http://www.ipol.im/pub/algo/lmps_retinex_poisson_equation/.

This program reads a PNG image, computes its laplacian, performs a
DFT, then solves the Poisson equation in the Fourier space and
performs an inverse DFT.
The result is normalized to have the same mean and variance as the
input image and written as a PNG image.

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
    cc io_png.c norm.c retinex_pde_lib.c retinex_pde.c \
        -lpng -lfftw3f -o retinex_pde

Multi-threading is possible, with the FFTW_NTHREADS parameter:
    cc io_png.c norm.c retinex_pde_lib.c retinex_pde.c \
        -DFFTW_NTHREADS -lpng -lfftw3f -lfftw3f_threads -o retinex_pde

# USAGE

This program takes 4 parameters: `retinex_pde T in.png rtnx.png`

* `T`        : retinex threshold [0...255[
* `in.png`   : input image
* `rtnx.png` : retinex output image

# ABOUT THIS FILE

Copyright 2009-2011 IPOL Image Processing On Line http://www.ipol.im/
Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
