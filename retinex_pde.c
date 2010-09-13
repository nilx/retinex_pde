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
 * @mainpage Retinex Poisson Equation : a Model for Color Perception
 *
 * README.txt:
 * @verbinclude README.txt
 */

/**
 * @file retinex_pde.c
 * @brief command-line interface
 *
 * The input image processed by the retinex transform, then normalized
 * to [0-255], ignoring 3% pixels at the beginning and end of the
 * histogram.
 *
 * For comparison, the same input is also normalized by the same
 * normalization process, without retinex transform.
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "retinex_pde_lib.h"
#include "io_png.h"

#ifndef WITHOUT_NORM
#include "normalize_histo_lib.h"
#endif /* !WITHOUT_NORM */

/**
 * @brief main function call
 */
int main(int argc, char *const *argv)
{
    float t;                    /* retinex threshold */
    size_t nx, ny, nc;
    size_t channel, nc_non_alpha;
    float *data, *data_rtnx;

    /* "-v" option : version info */
    if (2 <= argc && 0 == strcmp("-v", argv[1]))
    {
        fprintf(stdout, "%s version " __DATE__ "\n", argv[0]);
        return EXIT_SUCCESS;
    }
    /* wrong number of parameters : simple help info */
    if (5 != argc)
    {
        fprintf(stderr, "usage : %s T in.png norm.png rtnx.png\n", argv[0]);
        fprintf(stderr, "        T retinex threshold [0...255]\n");
        return EXIT_FAILURE;
    }

    /* retinex threshold */
    t = atof(argv[1]);
    if (0. > t || 255. < t)
    {
        fprintf(stderr, "the retinex threshold must be in [0..255]\n");
        return EXIT_FAILURE;
    }

    /* read the PNG image into data */
    if (NULL == (data = read_png_f32(argv[2], &nx, &ny, &nc)))
    {
        fprintf(stderr, "the image could not be properly read\n");
        return EXIT_FAILURE;
    }

    /* allocate data_rtnx and fill it with a copy of data */
    if (NULL == (data_rtnx = (float *) malloc(nc * nx * ny * sizeof(float))))
    {
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

    /* normalize data with 3% saturation and save */
#ifndef WITHOUT_NORM
    for (channel = 0; channel < nc_non_alpha; channel++)
        (void) normalize_histo_f32(data + channel * nx * ny,
                                   nx * ny, 0., 255.,
                                   (size_t) (0.015 * nx * ny),
                                   (size_t) (0.015 * nx * ny));
#endif /* !WITHOUT_NORM */
    write_png_f32(argv[3], data, nx, ny, nc);
    free(data);

    /* run retinex on data_rtnx, normalize with 3% saturation and save */
    for (channel = 0; channel < nc_non_alpha; channel++)
    {
        if (NULL == retinex_pde(data_rtnx + channel * nx * ny, nx, ny, t))
        {
            fprintf(stderr, "the retinex PDE failed\n");
            free(data_rtnx);
            return EXIT_FAILURE;
        }
#ifndef WITHOUT_NORM
        (void) normalize_histo_f32(data_rtnx + channel * nx * ny,
                                   nx * ny, 0., 255.,
                                   (size_t) (0.015 * nx * ny),
                                   (size_t) (0.015 * nx * ny));
#endif /* !WITHOUT_NORM */
    }
    write_png_f32(argv[4], data_rtnx, nx, ny, nc);
    free(data_rtnx);

    return EXIT_SUCCESS;
}
