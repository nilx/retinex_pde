/**
 * @file lib/src/algo_retinex_pde.c
 *
 * @author Catalina Sbert <catalina.sbert@uib.es>
 * @author Ana Belen Petro <anabelen.petro@uib.es>
 * @author Jean-Michel Morel <jean-michel.morel@cmla.ens-cachan.fr>
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 *
 * @todo adapt nb threads to the number of CPU available (OpenMP?)
 * @todo only multithread large images
 */

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <fftw3.h>

#include "mw4.h"

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
 * If the absolute value of difference is < tmin, 0 is used instead;
 * if it is > tmax, +-tmax is used.
 *
 * @param data_out output array
 * @param data_in input array
 * @param nx, ny array size
 * @param tmin, tmax min/max threshold
 *
 * @return data_out
 *
 * @todo unloop?
 * @todo try using blas?
 */
static float *mw4_discrete_laplacian_threshold(float *data_out,
                                               const float *data_in,
                                               size_t nx, size_t ny,
                                               float tmin, float tmax)
{
    int i, j;
    float *out_ptr;
    const float *in_ptr, *in_ptr_xm1, *in_ptr_xp1, *in_ptr_ym1, *in_ptr_yp1;
    float diff;

    /* check allocation */
    if (NULL == data_in || NULL == data_out)
        MW4_FATAL(MW4_MSG_NULL_PTR);

    /* pointers to the data and neighbour values */
    in_ptr = data_in;
    in_ptr_xm1 = data_in - 1;
    in_ptr_xp1 = data_in + 1;
    in_ptr_ym1 = data_in - nx;
    in_ptr_yp1 = data_in + nx;
    out_ptr = data_out;
    /* iterate on j, i, following the array order */
    for (j = 0; j < (int) ny; j++)
        for (i = 0; i < (int) nx; i++)
        {
            *out_ptr = 0.;
            /* row differences */
            if (0 < i)
            {
                diff = *(in_ptr_xm1) - *in_ptr;
                if (fabs(diff) > tmax)
                    *out_ptr += (diff > 0 ? tmax : -tmax);
                else if (fabs(diff) > tmin)
                    *out_ptr += diff;
            }
            if ((int) nx - 1 > i)
            {
                diff = *(in_ptr_xp1) - *in_ptr;
                if (fabs(diff) > tmax)
                    *out_ptr += (diff > 0 ? tmax : -tmax);
                else if (fabs(diff) > tmin)
                    *out_ptr += diff;
            }
            /* column differences */
            if (0 < j)
            {
                diff = *(in_ptr_ym1) - *in_ptr;
                if (fabs(diff) > tmax)
                    *out_ptr += (diff > 0 ? tmax : -tmax);
                else if (fabs(diff) > tmin)
                    *out_ptr += diff;
            }
            if ((int) ny - 1 > j)
            {
                diff = *(in_ptr_yp1) - *in_ptr;
                if (fabs(diff) > tmax)
                    *out_ptr += (diff > 0 ? tmax : -tmax);
                else if (fabs(diff) > tmin)
                    *out_ptr += diff;
            }
            in_ptr++;
            in_ptr_xm1++;
            in_ptr_xp1++;
            in_ptr_ym1++;
            in_ptr_yp1++;
            out_ptr++;
        }
    return data_out;
}

/**
 * @brief scale the DFT parameters
 *
 * @f$ u(i, j) = F(i, j) / (2 * (cos(PI * i / nx)
 *                               + cos(PI * j / ny)
 *                               - 2. - u / (nx * ny - 1))) @f$
 *
 * @param data the dft complex coefficients, of size nx x ny
 * @param nx, ny data array size
 * @param u multiplication parameter for @f$ \hat{u}() @f$
 * @param m global multiplication parameter (DFT normalization)
 *
 * @return the data array, updated
 */
static float *mw4_retinex_update_dft(float *data,
                                     size_t nx, size_t ny, float u, float m)
{
    float *data_ptr, *data_ptr_end;
    float *cosi_ptr, *cosi_ptr_end, *cosj_ptr, *cosj_ptr_end;
    float *factor_ptr;
    int i, j;
    float cst;
    /* static data, kept for a potential re-use */
    static float s_u = 0.;
    static float s_m = 0.;
    static size_t s_nx = 0, s_ny = 0;
    static float *s_factor = NULL, *s_cosi = NULL, *s_cosj = NULL;

    MW4_DEBUGF("saved params: u=%g, m=%g, nx=%u, ny=%u",
               s_u, s_m, (unsigned int) s_nx, (unsigned int) s_ny);
    MW4_DEBUGF("new params  : u=%g, m=%g, nx=%u, ny=%u",
               u, m, (unsigned int) nx, (unsigned int) ny);

    /* if needed, recompute the factors */
    /*
     * fabs(u - s_u) is a trick to avoid float comparison,
     * with negligible cost
     */
#pragma omp critical
    if (nx != s_nx || ny != s_ny || 0. < fabs(u - s_u) || 0. < fabs(m - s_m))
    {
        /* (re) allocate the factors table */
        if (NULL != s_factor)
            free(s_factor);
        if (NULL == (s_factor = (float *) malloc(sizeof(float) * nx * ny)))
            MW4_FATAL(MW4_MSG_ALLOC_ERR);
        /* allocate and fill the cosinus table arrays */
        if (s_nx != nx)
        {
            if (NULL != s_cosi)
                free(s_cosi);
            if (NULL == (s_cosi = (float *) malloc(sizeof(float) * nx)))
                MW4_FATAL(MW4_MSG_ALLOC_ERR);
            cosi_ptr = s_cosi;
            cosi_ptr_end = cosi_ptr + nx;
            i = 0;
            while (cosi_ptr < cosi_ptr_end)
                *cosi_ptr++ = 2. * cos((M_PI * i++) / nx);
        }
        s_nx = nx;
        if (s_ny != ny)
        {
            if (NULL != s_cosj)
                free(s_cosj);
            if (NULL == (s_cosj = (float *) malloc(sizeof(float) * ny)))
                MW4_FATAL(MW4_MSG_ALLOC_ERR);
            cosj_ptr = s_cosj;
            cosj_ptr_end = cosj_ptr + ny;
            j = 0;
            while (cosj_ptr < cosj_ptr_end)
                *cosj_ptr++ = 2. * cos((M_PI * j++) / ny);
        }
        s_ny = ny;

        /* (re)compute the factors */
        s_u = u;
        s_m = m;
        cst = (2. * s_u) / (s_nx * s_ny - 1);
        MW4_DEBUGF("DCT coefficients *="
                   " %g / (cos(i * PI / %u) + cos(j * PI / %u) - 4. - %g)",
                   s_m, (unsigned int) s_nx, (unsigned int) s_ny, cst);
        factor_ptr = s_factor;
        cosj_ptr = s_cosj;
        cosj_ptr_end = s_cosj + ny;
        cosi_ptr_end = s_cosi + nx;
        while (cosj_ptr < cosj_ptr_end)
        {
            cosi_ptr = s_cosi;
            while (cosi_ptr < cosi_ptr_end)
                *factor_ptr++ = s_m / (*cosi_ptr++ + *cosj_ptr - 4. - cst);
            cosj_ptr++;
        }
        if (FLT_MIN > fabs(s_u))
            *s_factor = 0.;
    }

    data_ptr = data;
    data_ptr_end = data_ptr + nx * ny;
    factor_ptr = s_factor;
    /* scale the dct coefficients */
    while (data_ptr < data_ptr_end)
        *data_ptr++ *= *factor_ptr++;

    return data;
}

/*
 * RETINEX
 */

/**
 * @brief retinex PDE implementation
 *
 * This function solves the Redinex PDE equation with forward and
 * backward DFT.
 *
 * The input array is processed as follow:
 *
 * @li a discrete laplacian is computed with a threshold;
 * @li this discrete laplacian array is symmetrised in both directions;
 * @li this data is transformed by forward DFT;
 * @li the DFT data is modified by
 * @f$ \hat{u}(i, j) = \frac{\hat{F}(i, j)}
                            {2 \times (\cos(\frac{i \pi}{n_x})
 *                           + \cos(\frac{j \pi}{n_y})
 *                           - 2 - 2 / (n_x \times n_y - 1))} @f$;
 * @li this data is transformed by backward DFT.
 *
 * @param data_out output array, retinex-corrected, already allocated
 * @param data_in array to process
 * @param nx dimension
 * @param ny dimension
 * @param tmin, tmax retinex min/max threshold
 * @param u multiplication parameter for @f$ \hat{u}() @f$
 *
 * @return data_out, or NULL if an error occured
 *
 * @todo unroll loops
 * @todo split pre/run/post for multi-threading
 */
float *mw4_retinex_pde(float *data_out, const float *data_in,
                       size_t nx, size_t ny, float tmin, float tmax, float u)
{
    fftwf_plan dct_fw, dct_bw;
    float *data_fft;

    /*
     * checks and initialisation
     */

    /* check allocaton */
    if (NULL == data_in || NULL == data_out)
        MW4_FATAL(MW4_MSG_NULL_PTR);
    /* start threaded fftw if FFTW_NTHREADS is defined */
#ifdef FFTW_NTHREADS
    if (0 == fftwf_init_threads())
        MW4_FATAL("fftw initialisation error");
    fftwf_plan_with_nthreads(FFTW_NTHREADS);
#endif                          /* FFTW_NTHREADS */
    /* allocate the float-complex FFT array */
    if (NULL == (data_fft = (float *) malloc(sizeof(float) * nx * ny)))
        MW4_FATAL(MW4_MSG_ALLOC_ERR);

    /*
     * step one : fill the DFT input with symmetrised discrete laplacian
     */

    /* compute the laplacian and store it in data_out */
    (void) mw4_discrete_laplacian_threshold(data_out, data_in,
                                            nx, ny, tmin, tmax);

    /*
     * step two : DCT
     */

    /* create the DFT forward plan and run the DCT */
#pragma omp critical
    dct_fw = fftwf_plan_r2r_2d((int) ny, (int) nx,
                               data_out, data_fft,
                               FFTW_REDFT10, FFTW_REDFT10,
                               FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
    fftwf_execute(dct_fw);

    /*
     * step three : update the DCT result
     */

    (void) mw4_retinex_update_dft(data_fft, nx, ny,
                                  u, 1. / (float) (nx * ny));

    /*
     * step four : backward DCT
     */

    /* create the DFT backward plan and run the DCT */
#pragma omp critical
    dct_bw = fftwf_plan_r2r_2d((int) ny, (int) nx,
                               data_fft, data_out,
                               FFTW_REDFT01, FFTW_REDFT01,
                               FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
    fftwf_execute(dct_bw);

    /*
     * cleanup
     */

    /* destroy the FFT plan and data */
    fftwf_destroy_plan(dct_fw);
    fftwf_destroy_plan(dct_bw);
    fftwf_free(data_fft);
    fftwf_cleanup();
#ifdef FFTW_NTHREADS
    fftwf_cleanup_threads();
#endif                          /* FFTW_NTHREADS */
    return data_out;
}

#ifdef CHECK
#include "check_algo_retinex_pde.c"
#endif                          /* CHECK */
