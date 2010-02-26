% Retinex PDE

# ABOUT

* Author    : Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
* Copyright : (C) 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
* Licence   : GPL v3+, see GPLv3.txt

# OVERVIEW

This source code provides an implementation of the Retinex theory by a
Poisson equation, as described on
    http://www.ipol.im/pub/algo/lmps_retinex_poisson_equation/.

This program reads a TIFF image, performs a DFT, then solved the
Poisson equation in the Fourier space and performs an inverse DFT.
The result is normalized and written as a TIFF image.

The same normalization is also performed on a unmodified image, for
comparison. The normalization method is the "simplest color balance",
as described on
    http://www.ipol.im/pub/algo/lmps_simplest_color_balance/.

Only 8bit RGB TIFF images are handled. Other TIFF files are implicitly
converted to 8bit color RGB.

# REQUIREMENTS

The code is written in ANSI C, and should compile on any system with
an ANSI C compiler.

The libtiff header and libraries are required on the system for
compilation and execution. See http://www.remotesensing.org/libtiff/ 

The fftw3 header and libraries are required on the system for
compilation and execution. See http://www.fftw.org/

# COMPILATION

Simply use the provided makefile, with the command `make`.
Alternatively, you can manually compile
    cc io_tiff.c normalize_histo_lib.c retinex_pde_lib.c retinex_pde.c \
        -ltiff -lfftw3f -o retinex_pde

Multi-threading is possible, with the FFTW_NTHREADS parameter:
    cc io_tiff.c normalize_histo_lib.c retinex_pde_lib.c retinex_pde.c \
        -DFFTW_NTHREADS -ltiff -lfftw3f -o retinex_pde

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
