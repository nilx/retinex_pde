/*
 * Copyright 2009-2011 IPOL Image Processing On Line http://www.ipol.im/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file retinex_pde_lib.c
 * @brief laplacian, DFT and Poisson routines
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <fftw3.h>

#include "debug.h"

/* ensure consistency */
#include "retinex_pde_lib.h"

/* M_PI is a POSIX definition */
#ifndef M_PI
/** macro definition for Pi */
#define M_PI 3.14159265358979323846
#endif                          /* !M_PI */

#ifndef NDEBUG
#define LAPLACE 1
#define POISSON 2
#define FOURIER 3
#endif

/*
 * number of threads to use for libfftw
 * define to enable parallel FFT multi-threading
 */
/* #define FFTW_NTHREADS 4 */

/**
 * @brief compute the discrete laplacian of a 2D array with a threshold
 *
 * This function computes the discrete laplacian, ie
 * @f$ (F_{i - 1, j} - F_{i, j})
 *     + (F_{i + 1, j} - F_{i, j})
 *     + (F_{i, j - 1} - F_{i, j})
 *     + (F_{i, j + 1} - F_{i, j}) \f$.
 * On the border, differences with "outside of the array" are 0.
 * If the absolute value of difference is < t, 0 is used instead.
 *
 * This step takes a significant part of the computation time, and
 * needs to be fast. In that case, we observed that (with our compiler
 * and architecture):
 * - pointer arithmetic is faster than data[i]
 * - if() is faster than ( ? : )
 *
 * @param data_out output array
 * @param data_in input array
 * @param nx, ny array size
 * @param t threshold
 *
 * @return data_out
 *
 * @todo split corner/border/inner
 */
static float *discrete_laplacian_threshold(float *data_out,
                                           const float *data_in,
                                           size_t nx, size_t ny, float t)
{
    size_t i, j;
    float *ptr_out;
    float diff;
    /* pointers to the current and neighbour values */
    const float *ptr_in, *ptr_in_xm1, *ptr_in_xp1, *ptr_in_ym1, *ptr_in_yp1;

    /* sanity check */
    if (NULL == data_in || NULL == data_out) {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

    DBG_CLOCK_TOGGLE(LAPLACE);

    /* pointers to the data and neighbour values */
    /*
     *                 y-1
     *             x-1 ptr x+1
     *                 y+1
     *    <---------------------nx------->
     */
    ptr_in = data_in;
    ptr_in_xm1 = data_in - 1;
    ptr_in_xp1 = data_in + 1;
    ptr_in_ym1 = data_in - nx;
    ptr_in_yp1 = data_in + nx;
    ptr_out = data_out;
    /* iterate on j, i, following the array order */
    for (j = 0; j < ny; j++) {
        for (i = 0; i < nx; i++) {
            *ptr_out = 0.;
            /* row differences */
            if (0 < i) {
                diff = *ptr_in - *ptr_in_xm1;
                if (fabs(diff) > t)
                    *ptr_out += diff;
            }
            if (nx - 1 > i) {
                diff = *ptr_in - *ptr_in_xp1;
                if (fabs(diff) > t)
                    *ptr_out += diff;
            }
            /* column differences */
            if (0 < j) {
                diff = *ptr_in - *ptr_in_ym1;
                if (fabs(diff) > t)
                    *ptr_out += diff;
            }
            if (ny - 1 > j) {
                diff = *ptr_in - *ptr_in_yp1;
                if (fabs(diff) > t)
                    *ptr_out += diff;
            }
            ptr_in++;
            ptr_in_xm1++;
            ptr_in_xp1++;
            ptr_in_ym1++;
            ptr_in_yp1++;
            ptr_out++;
        }
    }

    DBG_CLOCK_TOGGLE(LAPLACE);

    return data_out;
}

/**
 * @brief compute a cosinus table
 *
 * Allocate and fill a table of n values cos(i Pi / n) for i in [0..n[.
 *
 * @param size the table size
 *
 * @return the table, allocated and filled
 */
static double *cos_table(size_t size)
{
    double *table = NULL;
    double pi_size;
    size_t i;

    /* allocate the cosinus table */
    if (NULL == (table = (double *) malloc(sizeof(double) * size))) {
        fprintf(stderr, "allocation error\n");
        abort();
    }

    /*
     * fill the cosinus table,
     * table[i] = cos(i Pi / n) for i in [0..n[
     */
    pi_size = M_PI / size;
    for (i = 0; i < size; i++)
        table[i] = cos(pi_size * i);

    return table;
}

/**
 * @brief perform a Poisson PDE in the Fourier DCT space
 *
 * @f$ u(i, j) = F(i, j) * m / (4 - 2 cos(i PI / nx)
 *                              - 2 cos(j PI / ny)) @f$
 * if @f$ (i, j) \neq (0, 0) @f$,
 * @f$ u(0, 0) = 0 @f$
 *
 * When this function is successively used on arrays of identical
 * size, the trigonometric computation is redundant and could be kept
 * in memory for a faster code. However, in our use case, the speedup
 * is marginal and we prefer to recompute this data and keep the code
 * simple.
 *
 * @param data the dct complex coefficients, of size nx x ny
 * @param nx, ny data array size
 * @param m global multiplication parameter (DCT normalization)
 *
 * @return the data array, updated
 */
static float *retinex_poisson_dct(float *data, size_t nx, size_t ny, double m)
{
    double *cosx = NULL, *cosy = NULL;
    size_t i;
    double m2;

    DBG_CLOCK_TOGGLE(POISSON);

    /*
     * get the cosinus tables
     * cosx[i] = cos(i Pi / nx) for i in [0..nx[
     * cosy[i] = cos(i Pi / ny) for i in [0..ny[
     */
    cosx = cos_table(nx);
    cosy = cos_table(ny);

    /*
     * we will now multiply data[i, j] by
     * m / (4 - 2 * cosx[i] - 2 * cosy[j]))
     * and set data[0, 0] to 0
     */
    m2 = m / 2.;
    /*
     * handle the first value, data[0, 0] = 0
     * after that, by construction, we always have
     * cosx[] + cosy[] != 2.
     */
    data[0] = 0.;
    /*
     * continue with all the array:
     * i % nx is the position on the x axis (column number)
     * i / nx is the position on the y axis (row number)
     */
    for (i = 1; i < nx * ny; i++)
        data[i] *= m2 / (2. - cosx[i % nx] - cosy[i / nx]);

    free(cosx);
    free(cosy);

    DBG_CLOCK_TOGGLE(POISSON);

    return data;
}

/*
 * RETINEX
 */

/**
 * @brief retinex PDE implementation
 *
 * This function solves the Retinex PDE equation with forward and
 * backward DCT.
 *
 * The input array is processed as follow:
 *
 * @li a discrete laplacian is computed with a threshold;
 * @li this discrete laplacian array is symmetrised in both directions;
 * @li this data is transformed by forward DFT (both steps can be
 *     handled by a simple DCT);
 * @li the DFT data is modified by
 * @f$ \hat{u}(i, j) = \frac{\hat{F}(i, j)}
 *                           {4 - 2 \cos(\frac{i \pi}{n_x})
 *                           - 2 \cos(\frac{j \pi}{n_y})} @f$;
 * @li this data is transformed by backward DFT.
 *
 * @param data input/output array
 * @param nx, ny dimension
 * @param t retinex threshold
 *
 * @return data, or NULL if an error occured
 */
float *retinex_pde(float *data, size_t nx, size_t ny, float t)
{
    fftwf_plan dct_fw, dct_bw;
    float *data_fft, *data_tmp;

    DBG_CLOCK_RESET(LAPLACE);
    DBG_CLOCK_RESET(POISSON);
    DBG_CLOCK_RESET(FOURIER);

    /* check allocaton */
    if (NULL == data) {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

    /* allocate the float tmp array */
    if (NULL == (data_tmp = (float *) malloc(sizeof(float) * nx * ny))) {
        fprintf(stderr, "allocation error\n");
        abort();
    }

    /* compute the laplacian : data -> data_tmp */
    (void) discrete_laplacian_threshold(data_tmp, data, nx, ny, t);

    /* allocate the float-complex FFT array */
    if (NULL == (data_fft = (float *) malloc(sizeof(float) * nx * ny))) {
        fprintf(stderr, "allocation error\n");
        abort();
    }

    /* start threaded fftw if FFTW_NTHREADS is defined */
#ifdef FFTW_NTHREADS
    if (0 == fftwf_init_threads()) {
        fprintf(stderr, "fftw initialisation error\n");
        abort();
    }
    fftwf_plan_with_nthreads(FFTW_NTHREADS);
#endif                          /* FFTW_NTHREADS */

    /* create the DFT forward plan and run the DCT : data_tmp -> data_fft */
    DBG_CLOCK_TOGGLE(FOURIER);
    dct_fw = fftwf_plan_r2r_2d((int) ny, (int) nx,
                               data_tmp, data_fft,
                               FFTW_REDFT10, FFTW_REDFT10,
                               FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
    fftwf_execute(dct_fw);
    DBG_CLOCK_TOGGLE(FOURIER);
    free(data_tmp);

    /* solve the Poisson PDE in Fourier space */
    /* 1. / (float) (nx * ny)) is the DCT normalisation term, see libfftw */
    (void) retinex_poisson_dct(data_fft, nx, ny, 1. / (double) (nx * ny));

    /* create the DFT backward plan and run the iDCT : data_fft -> data */
    DBG_CLOCK_TOGGLE(FOURIER);
    dct_bw = fftwf_plan_r2r_2d((int) ny, (int) nx,
                               data_fft, data,
                               FFTW_REDFT01, FFTW_REDFT01,
                               FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
    fftwf_execute(dct_bw);
    DBG_CLOCK_TOGGLE(FOURIER);

    /* cleanup */
    fftwf_destroy_plan(dct_fw);
    fftwf_destroy_plan(dct_bw);
    fftwf_free(data_fft);
    fftwf_cleanup();
#ifdef FFTW_NTHREADS
    fftwf_cleanup_threads();
#endif                          /* FFTW_NTHREADS */

    DBG_PRINTF1("laplace\t%0.2fs\n", DBG_CLOCK_S(LAPLACE));
    DBG_PRINTF1("poisson\t%0.2fs\n", DBG_CLOCK_S(POISSON));
    DBG_PRINTF1("fourier\t%0.2fs\n", DBG_CLOCK_S(FOURIER));

    return data;
}
