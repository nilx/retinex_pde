/*
 * Copyright 2011 IPOL Image Processing On Line http://www.ipol.im/
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
 * @file norm_mean_dt.c
 * @brief array normalization
 *
 * @author Jose-Luis Lisani <joseluis.lisani@uib.es>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @brief compute mean and variance of a float array
 *
 * @param data float array
 * @param size array size
 * @param mean_p, dt_p addresses to store the mean and variance
 */
static void mean_dt(const float *data, size_t size,
                    double *mean_p, double *dt_p)
{
    double mean, dt;
    float *ptr_data;
    size_t i;

    mean = 0.;
    dt = 0.;
    ptr_data = data;
    for (i = 0; i < size; i++) {
        mean += *ptr_data;
        dt += (*ptr_data) * (*ptr_data);
        ptr_data++;
    }
    mean /= (double) size;
    dt /= (double) size;
    dt -= (mean * mean);
    dt = sqrt(dt);

    *mean_p = mean;
    *dt_p = dt;

    return;
}

/**
 * @brief normalize mean and variance of a float array given a reference
 *        array
 *
 * The normalized array is normalized by an affine transformation
 * to adjust its mean and variance to a reference array.
 *
 * @param data normalized array
 * @param ref reference array
 * @param size size of the arrays
 */
void norm_dt(float *data, const float *ref, size_t size)
{
    double mean_ref, mean_data, dt_ref, dt_data;        /* means and variances */
    double a, b;                /* normalization coefficients */
    size_t i;
    float *ptr_data;

    /* sanity check */
    if (NULL == data || NULL == ref) {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

    /* compute mean and variance of the two arrays */
    mean_dt(ref, size, &mean_ref, &dt_ref);
    mean_dt(data, size, &mean_data, &dt_data);

    /* compute the normalization coefficients */
    a = dt_ref / dt_data;
    b = mean_ref - a * mean_data;

    /* normalize the array */
    ptr_data = data;
    for (i = 0; i < size; i++) {
        *ptr_data = a * *ptr_data + b;
        ptr_data++;
    }

    return;
}
