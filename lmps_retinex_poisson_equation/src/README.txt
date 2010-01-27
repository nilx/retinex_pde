# REQUIREMENTS

retinex_pde uses libtiff to read and write TIFF image files, and
libfftw3 to perform a Fourier transform.

On debian-based systems, this is enought to install these libraries
    sudo aptitude install libtiff4-dev libfftw3-dev

retinex_pde can also use OpenMP for parallel computation of the
retinex transform on the RGB channels. OpenMP support is included in
the gcc compiler, no other installation is required.


# COMPILATION

simple way:
 
    cc -O3 -ltiff -lfftw3f \
    io_tiff.c algo_normalize.c algo_retinex_pde.c retinex_pde.c \
    -o retinex_pde

with OpenMP support:

    cc -fopenmp -O3 -ltiff -lfftw3f \
    io_tiff.c algo_normalize.c algo_retinex_pde.c retinex_pde.c \
    -o retinex_pde

with N FFT multi-threads:

    cc -fopenmp -DFFTW_NTHREADS=4 -O3 -ltiff -lfftw3f \
    io_tiff.c algo_normalize.c algo_retinex_pde.c retinex_pde.c \
    -o retinex_pde

FFT multi-threading can be combined with OpenMP, but it will only be
faster with large images, and enough processing units (processors or cores):
* 3 processors for OpenMP
* N processors with N FFT multi-threads
* 3N processors for both

# USE

Options and details are available with the --help option:

    ./retinex_pde -h

For OpenMP multi-processing, you need to set the number of threads to
use before starting retinex_pde:

    export OMP_MAX_THREADS=3
