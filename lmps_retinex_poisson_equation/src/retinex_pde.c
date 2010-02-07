/*
 * Copyright 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
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
 * @file lib/src/algo_retinex_pde.c
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 *
 * @todo adapt nb threads to the number of CPU available
 * @todo only multithread large images
 */

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <fftw3.h>

/* M_PI is a POSIX definition */
#ifndef M_PI
/** macro definition for Pi */
#define M_PI 3.14159265358979323846
#endif                          /* !M_PI */

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
 * If the absolute value of difference is < t, 0 is used instead;
 *
 * @param data_out output array
 * @param data_in input array
 * @param nx, ny array size
 * @param t threshold
 *
 * @return data_out
5A *
 * @todo full iteration and switch()
 * @todo try using blas?
 */
static float *discrete_laplacian_threshold(float *data_out,
					   const float *data_in,
					   size_t nx, size_t ny, float t)
{
    int i, j;
    float *ptr_out;
    float diff;
    /* pointers to the current and neighbour values */
    const float *ptr_in, *ptr_in_xm1, *ptr_in_xp1, *ptr_in_ym1, *ptr_in_yp1;

    /* sanity check */
    if (NULL == data_in || NULL == data_out)
    {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

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
    for (j = 0; j < (int) ny; j++)
        for (i = 0; i < (int) nx; i++)
        {
            *ptr_out = 0.;
            /* row differences */
            if (0 < i)
            {
                diff = *(ptr_in_xm1) - *ptr_in;
                if (fabs(diff) > t)
                    *ptr_out += diff;
            }
            if ((int) nx - 1 > i)
            {
                diff = *(ptr_in_xp1) - *ptr_in;
                if (fabs(diff) > t)
                    *ptr_out += diff;
            }
            /* column differences */
            if (0 < j)
            {
                diff = *(ptr_in_ym1) - *ptr_in;
                if (fabs(diff) > t)
                    *ptr_out += diff;
            }
            if ((int) ny - 1 > j)
            {
                diff = *(ptr_in_yp1) - *ptr_in;
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
    return data_out;
}

/**
 * @brief perform a Poisson PDE in the Fourier DCT space
 *
 * @f$ u(i, j) = F(i, j) / (2 * (cos(PI * i / nx)
 *                               + cos(PI * j / ny)
 *                               - 2. - 2. / (nx * ny - 1))) @f$
 *
 * The trigonometric data is only computed once if the function is
 * called many times with the same nx, ny and m parameters (RGB planes).
 *
 * @param data the dct complex coefficients, of size nx x ny
 * @param nx, ny data array size
 * @param m global multiplication parameter (DCT normalization)
 *
 * @return the data array, updated
 *
 * @todo handle nx=ny
 */
static float *retinex_poisson_dct(float *data,
				  size_t nx, size_t ny, float m)
{
    float *ptr_data, *ptr_end;
    float *ptr_coef;
    float *ptr_cosi, *ptr_cosi_end, *ptr_cosj, *ptr_cosj_end;
    int i, j;
    float cst;
    /* static data, kept for a potential re-use */
    /*
     * we typically perform this on 3 RGB planes, and wand to keep the
     * trigonometric data because its computation is expensive
     */
    static float s_m = 0.;
    static size_t s_nx = 0, s_ny = 0;
    static float *s_coef = NULL, *s_cosi = NULL, *s_cosj = NULL;

    /*
     * if needed, recompute the coefs
     * this computation is only needed if nx, ny and m are not the
     * same as the previous call
     */
    /*
     * fabs(u - s_u) is a trick to avoid float comparison,
     * with negligible cost
     */
    if (nx != s_nx || ny != s_ny || 0. < fabs(m - s_m))
    {
        s_m = m;

        if (s_nx != nx)
        {
	    /* (re) allocate the cosinus table */
            if (NULL != s_cosi)
                free(s_cosi);
            if (NULL == (s_cosi = (float *) malloc(sizeof(float) * nx)))
	    {
		fprintf(stderr, "allocation error\n");
		abort();
	    }
	    /* fill the cosinus table */
            ptr_cosi = s_cosi;
            ptr_cosi_end = ptr_cosi + nx;
            i = 0;
            while (ptr_cosi < ptr_cosi_end)
                *ptr_cosi++ = 2. * cos((M_PI * i++) / nx);
        }
        s_nx = nx;

        if (s_ny != ny)
        {
	    /* (re) allocate the sinus table */
            if (NULL != s_cosj)
                free(s_cosj);
            if (NULL == (s_cosj = (float *) malloc(sizeof(float) * ny)))
	    {
		fprintf(stderr, "allocation error\n");
		abort();
	    }
	    /* fill the sinus table */
            ptr_cosj = s_cosj;
            ptr_cosj_end = ptr_cosj + ny;
            j = 0;
            while (ptr_cosj < ptr_cosj_end)
                *ptr_cosj++ = 2. * cos((M_PI * j++) / ny);
        }
        s_ny = ny;

        /* (re) allocate the coefs table */
        if (NULL != s_coef)
            free(s_coef);
        if (NULL == (s_coef = (float *) malloc(sizeof(float) * nx * ny)))
	{
	    fprintf(stderr, "allocation error\n");
		abort();
	}

        /* (re) compute the coefs */
	/*
	 * coef(i, j) = 1 / (cos(i) + cos(j) - 4 - 4 / (nx * ny - 1))
 	 */
        cst = 4. / (s_nx * s_ny - 1);
        ptr_coef = s_coef;
        ptr_cosj = s_cosj;
        ptr_cosj_end = s_cosj + s_ny;
        ptr_cosi_end = s_cosi + s_nx;
        while (ptr_cosj < ptr_cosj_end)
        {
            ptr_cosi = s_cosi;
            while (ptr_cosi < ptr_cosi_end)
                *ptr_coef++ = s_m / (*ptr_cosi++ + *ptr_cosj - 4. - cst);
            ptr_cosj++;
        }
    }

    /* scale the dct coefficients */
    ptr_data = data;
    ptr_end = ptr_data + nx * ny;
    ptr_coef = s_coef;
    while (ptr_data < ptr_end)
        *ptr_data++ *= *ptr_coef++;

    return data;
}

/*
 * RETINEX
 */

/**
 * @brief retinex PDE implementation
 *
 * This function solves the Redinex PDE equation with forward and
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
                            {2 \times (\cos(\frac{i \pi}{n_x})
 *                           + \cos(\frac{j \pi}{n_y})
 *                           - 2 - 2 / (n_x \times n_y - 1))} @f$;
 * @li this data is transformed by backward DFT.
 *
 * @param data input/output array
 * @param nx, ny dimension
 * @param t retinex threshold
 *
 * @return data, or NULL if an error occured
 *
 * @todo inplace fft
 * @todo split pre/run/post for multi-threading
 */
float *retinex_pde(float *data, size_t nx, size_t ny, float t)
{
    fftwf_plan dct_fw, dct_bw;
    float *data_fft, *data_tmp;

    /*
     * checks and initialisation
     */

    /* check allocaton */
    if (NULL == data)
    {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

    /* start threaded fftw if FFTW_NTHREADS is defined */
#ifdef FFTW_NTHREADS
    if (0 == fftwf_init_threads())
    {
        fprintf(stderr, "fftw initialisation error\n");
        abort();
    }
    fftwf_plan_with_nthreads(FFTW_NTHREADS);
#endif                          /* FFTW_NTHREADS */

    /* allocate the float-complex FFT array and the float tmp array */
    if (NULL == (data_fft = (float *) malloc(sizeof(float) * nx * ny))
	|| NULL == (data_tmp = (float *) malloc(sizeof(float) * nx * ny)))
    {
	fprintf(stderr, "allocation error\n");
	abort();
    }

    /*
     * retinex PDE
     */

    /* compute the laplacian : data -> data_tmp */
    (void) discrete_laplacian_threshold(data_tmp, data,
                                            nx, ny, t);
    /* create the DFT forward plan and run the DCT : data_tmp -> data_fft */
    dct_fw = fftwf_plan_r2r_2d((int) ny, (int) nx,
                               data_tmp, data_fft,
                               FFTW_REDFT10, FFTW_REDFT10,
                               FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
    fftwf_execute(dct_fw);
    free(data_tmp);

    /* solve the Poisson PDE in Fourier space */
    (void) retinex_poisson_dct(data_fft, nx, ny, 1. / (float) (nx * ny));

    /* create the DFT forward plan and run the iDCT : data_fft -> data */
    dct_bw = fftwf_plan_r2r_2d((int) ny, (int) nx,
                               data_fft, data,
                               FFTW_REDFT01, FFTW_REDFT01,
                               FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
    fftwf_execute(dct_bw);

    /* cleanup */
    fftwf_destroy_plan(dct_fw);
    fftwf_destroy_plan(dct_bw);
    fftwf_free(data_fft);
    fftwf_cleanup();
#ifdef FFTW_NTHREADS
    fftwf_cleanup_threads();
#endif                          /* FFTW_NTHREADS */
    return data;
}
