/**
 * @file main.c
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include "retinex_pde.h"
#include "normalization.h"
#include "io_tiff.h"

#include "mw4.h"

/*
 * INFORMATION STRINGS
 */

/** program name */
#define NAME "retinex_pde"
/** program version */
#define VERSION "2.04.20090613"
/** brief description of the purpose of the program */
#define BRIEF "PDE implementation of the Land Retinex theory"
/** long description of this program */
#define DESCRIPTION "\
The input image is first normalized to [0-255], ignoring a percentage \n\
of pixels (parameters --flatten-min and	--flatten-max) at the \n\
beginning and end of the histogram. This defines the normalized data, \n\
saved into a file and used for the following operations. \n\
Then this data is modified according to the Retinex PDE, with two \n\
possible thresholds in the discrete laplacian (parameters \n\
--threshold-min and --threshold-max) and a flexible parameter (u) in \n\
the Fourier coefficients update. Finally, the same normalization is \n\
processed again on the computed PDE solution.\n\
"
/** authors of this program */
#define AUTHOR "\
Catalina Sbert <catalina.sbert@uib.es>, \
Ana Belen Petro <anabelen.petro@uib.es>, \
Jean-Michel Morel <jean-michel.morel@cmla.ens-cachan.fr>, \
Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>"

/* long strings are split because of
 * the 509 characters limit on strings for ANSI C */
/** version and data information */
#define VERSION_STR_DATE "\
" NAME " version " VERSION "\n\
compiled on " __DATE__ " " __TIME__ "\n\
"
#ifdef _OPENMP
/** OpenMP support information */#
define VERSION_STR_OMP "\
compiled with OpenMP support\n\
"
#else
/** OpenMP support information */
#define VERSION_STR_OMP ""
#endif                          /* _OPENMP */

#ifdef FFTW_NTHREADS
/** threads support information */
#define VERSION_STR_FFTW "\
compiled for %i parallel FFT threads\n\
", FFTW_NTHREADS
#else
/** threads support information */
#define VERSION_STR_FFTW ""
#endif                          /* FFTW_NTHREADS */

/** detailed version information */
#define VERSION_STR VERSION_STR_DATE VERSION_STR_OMP VERSION_STR_FFTW

/** usage text */
#define USAGE_STR_1 "\
" NAME " version " VERSION "\n\
\n\
" NAME " - " BRIEF "\n\
\n\
Synopsis:\n\
    " NAME " [-d] [-t N] [-c C] input_image norm_image rtnx_image\n\
    " NAME " [-h] [-v]\n\
\n\
"
/** usage text, continued */
#define USAGE_STR_2 "\
Options:\n\
    -t, --threshold-min N   set the retinex min threshold to N\n\
                            default : 0, limits : [0..255]\n\
    -T, --threshold-max N   set the retinex max threshold to N\n\
                            default : 255, limits : [0..255]\n\
    -f, --flatten-min N     set the min flattening percentage to N\n\
                            default : 0, limits : [0..100]\n\
    -F, --flatten-max N     set the retinex max threshold to N (in [0..255])\n\
                            default : 0, limits : [0..100]\n\
    -u N                    multiplication parameter for u() (in [0..100]\n\
                            default : 2\n\
"
/** usage text, continued */
#define USAGE_STR_3 "\
    -h, --help              print this usage information\n\
    -v, --version           print the version information\n\
    -d, --debug             print debugging informations\n\
\n\
"
/** usage text, continued */
#define USAGE_STR_4 "\
Parameters:\n\
    input_image         image file read\n\
    norm_image          image file written with the normalized values\n\
    rtnx_image          image file written with the retinex values\n\
\n\
    " NAME " only reads and writes TIFF image files\n\
\n\
\n\
"
/** usage text, continued */
#define USAGE_STR_5 "\
" DESCRIPTION "\
\n\
Written by " AUTHOR ".\n\
See http://mw.cmla/ens-cachan.fr/megawave/algo/retinex_pde/ for details.\n\
"

/**
 * @brief main function call
 *
 * Detailed command-line usage information is displayed with the "-h" option.
 */
int main(int argc, char *const *argv)
{
    float tmin, tmax;             /* retinex min/max thresholds */
    float fmin, fmax;             /* flattening proportion */
    float u;
    int channel;
    char *fname_in, *fname_norm, *fname_rtnx;   /* input/output file names */
    unsigned int nx, ny;        /* data size */
    float *data_norm, *data_rtnx;       /* output data */

    /* default values initialisation */
    tmin = 0.;
    tmax = 255.;
    fmin = 0.;
    fmax = 0.;
    u = 2.;

    /*
     * process the options and parameters
     */
    while (1)
    {
        int c;                  /* getopt_long flag */
        int option_index = 0;   /* getopt_long stores the option index here */
        static struct option long_options[] = {
            {"help", no_argument, NULL, 'h'},
            {"version", no_argument, NULL, 'v'},
            {"debug", no_argument, NULL, 'd'},
            {"threshold-min", required_argument, NULL, 't'},
            {"threshold-max", required_argument, NULL, 'T'},
            {"flatten-min", required_argument, NULL, 'f'},
            {"flatten-max", required_argument, NULL, 'F'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "hvdt:T:f:F:u:", 
			long_options, &option_index);
        /* end of the options. */
        if (c == -1)
            break;
        /* pick the defined options */
        switch (c)
        {
        case 'h':
            fprintf(stdout, USAGE_STR_1);
            fprintf(stdout, USAGE_STR_2);
            fprintf(stdout, USAGE_STR_3);
            fprintf(stdout, USAGE_STR_4);
            fprintf(stdout, USAGE_STR_5);
            return 0;
        case 'v':
            fprintf(stdout, VERSION_STR);
            return 0;
        case 'd':
            mw4_debug_flag = 1;
            break;
        case 't':
            tmin = atof(optarg);
            if (0. > tmin || 255. < tmin)
                MW4_FATAL("the retinex thresholds must be in [0..255]");
            break;
        case 'T':
            tmax = atof(optarg);
            if (0. > tmax || 255. < tmax)
                MW4_FATAL("the retinex thresholds must be in [0..255]");
            break;
        case 'f':
            fmin = atof(optarg) / 100.;
            if (0. > fmin || 1. < fmin)
                MW4_FATAL("the flattening percentage must be in [0..100]");
            break;
        case 'F':
            fmax = atof(optarg) / 100.;
            if (0. > fmax || 1. < fmax)
                MW4_FATAL("the flattening percentage must be in [0..100]");
            break;
        case 'u':
            u = atof(optarg);
            if (0. > u || 100. < u)
                MW4_FATAL("the u parameter must be in [0..100]");
            break;
        default:
            abort();
        }
    }

    /* process the non-option parameters */
    if (3 > (argc - optind))
        /* parameters are missing */
        MW4_FATAL("the image file names are missing,"
                  " see details with the '-h' option");
    if (3 < (argc - optind))
        /* extra parameters */
        MW4_DEBUG("unhandled extra parameters");

    fname_in = argv[optind++];
    fname_norm = argv[optind++];
    fname_rtnx = argv[optind];

    MW4_DEBUGF("params: tmin=%f tmax=%f fmin=%f fmax=%f u=%f",
	       tmin, tmax, fmin, fmax, u);
    MW4_DEBUGF("files : in=%s norm=%s rtnx=%s",
	       fname_in, fname_norm, fname_rtnx);

    /*
     * initialisation and load input
     */

    /* read the TIFF image */
    if (NULL == (data_norm = read_tiff_rgba_f32(fname_in, &nx, &ny)))
        MW4_FATAL(MW4_MSG_FILE_READ_ERR);
    MW4_DEBUG("input image file read");

    /* allocate data_rtnx */
    if (NULL == (data_rtnx =
		 (float *) malloc(4 * nx * ny * sizeof(float))))
	MW4_FATAL(MW4_MSG_ALLOC_ERR);

    /* copy data_norm to data_rtnx */
    memcpy(data_rtnx, data_norm, 4 * nx * ny * sizeof(float));

    /*
     * do normalization 3% on original data and save
     */

    for (channel = 0; channel < 3; channel++)
	normalize_f32(data_norm + channel * nx * ny, nx * ny,
		      0., 255., 0.015 * nx * ny, 0.015 * nx * ny);
    write_tiff_rgba_f32(fname_norm, data_norm, nx, ny);
    free(data_norm);

    /*
     * do retinex and normalize 3% and save
     */

    for (channel = 0; channel < 3; channel++)
    {
	if (NULL == mw4_retinex_pde(data_rtnx + channel * nx * ny,
				    nx, ny, tmin, tmax, u))
	    MW4_FATAL("error while processing the FFT PDE resolution");
	MW4_DEBUG("retinex PDE solved");
	normalize_f32(data_rtnx + channel * nx * ny, nx * ny, 
		      0., 255., 0.015 * nx * ny, 0.015 * nx * ny);
    }
    write_tiff_rgba_f32(fname_rtnx, data_rtnx, nx, ny);
    free(data_rtnx);

    return EXIT_SUCCESS;
}
