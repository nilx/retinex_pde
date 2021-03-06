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
 * @file retinex_pde.c
 * @brief command-line interface
 *
 * The input image is processed by the retinex transform,
 * then normalized to have the same mean and variance as the input
 * image.
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "retinex_pde_lib.h"
#include "io_png.h"
#include "norm.h"
#include "debug.h"

/**
 * @brief main function call
 */
int main(int argc, char *const *argv)
{
    float t;                    /* retinex threshold */
    size_t nx, ny, nc;          /* image size */
    size_t channel, nc_non_alpha;
    float *data, *data_rtnx;

    /* "-v" option : version info */
    if (2 <= argc && 0 == strcmp("-v", argv[1])) {
        fprintf(stdout, "%s version " __DATE__ "\n", argv[0]);
        return EXIT_SUCCESS;
    }
    /* wrong number of parameters : simple help info */
    if (4 != argc) {
        fprintf(stderr, "usage : %s T in.png rtnx.png\n", argv[0]);
        fprintf(stderr, "        T retinex threshold [0,1[\n");
        return EXIT_FAILURE;
    }

    /* retinex threshold */
    t = atof(argv[1]);
    if (0. > t || 1. <= t) {
        fprintf(stderr, "the retinex float threshold must be in [0,1[\n");
        return EXIT_FAILURE;
    }

    /* read the PNG image into data */
    DBG_CLOCK_START(0);
    if (NULL == (data = io_png_read_flt(argv[2], &nx, &ny, &nc))) {
        fprintf(stderr, "the image could not be properly read\n");
        return EXIT_FAILURE;
    }
    DBG_CLOCK_TOGGLE(0);

    /* allocate data_rtnx and fill it with a copy of data */
    if (NULL == (data_rtnx = (float *) malloc(nc * nx * ny * sizeof(float)))) {
        fprintf(stderr, "allocation error\n");
        free(data);
        return EXIT_FAILURE;
    }
    memcpy(data_rtnx, data, nc * nx * ny * sizeof(float));

    /* the image has either 1 or 3 non-alpha channels */
    if (3 <= nc)
        nc_non_alpha = 3;
    else
        nc_non_alpha = 1;

    /*
     * run retinex on each non-alpha channel data_rtnx,
     * normalize mean and standard deviation and save
     */
    for (channel = 0; channel < nc_non_alpha; channel++) {
        if (NULL == retinex_pde(data_rtnx + channel * nx * ny, nx, ny, t)) {
            fprintf(stderr, "the retinex PDE failed\n");
            free(data_rtnx);
            free(data);
            return EXIT_FAILURE;
        }
        normalize_mean_dt(data_rtnx + channel * nx * ny,
                          data + channel * nx * ny, nx * ny);
    }
    DBG_CLOCK_TOGGLE(0);
    io_png_write_flt(argv[3], data_rtnx, nx, ny, nc);
    DBG_CLOCK_TOGGLE(0);
    DBG_PRINTF1("io\t%0.2fs\n", DBG_CLOCK_S(0));

    free(data_rtnx);
    free(data);

    return EXIT_SUCCESS;
}
