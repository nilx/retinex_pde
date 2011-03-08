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
void norm_dt(float *data, float *ref, size_t size)
{
    double m_ref, m_data, dt_ref, dt_data; /* means and variances */
    double a, b; /* normalization coefficients */
    size_t i;
    float *ptr_ref, *ptr_data;

    /* sanity check */
    if (NULL == data || NULL == ref) {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

    /* compute reference mean and variance */
    m_ref = 0.;
    dt_ref = 0.;
    ptr_ref = ref;
    for (i=0; i< size; i++) {
        m_ref += *ptr_ref;
        dt_ref += (*ptr_ref) * (*ptr_ref);
        ptr_ref++;
    }
    m_ref /= (double) size;
    dt_ref /= (double) size;
    dt_ref -= (m_ref * m_ref);
    dt_ref = sqrt(dt_ref);

    /* compute pre-normalization mean and variance */
    m_data = 0.;
    dt_data = 0.;
    ptr_data = data;
    for (i=0; i<size; i++) {
        m_data += *ptr_data;
        dt_data += (*ptr_data) * (*ptr_data);
        ptr_data++;
    }
    m_data /= (double) size;
    dt_data /= (double) size;
    dt_data -= (m_data * m_data);
    dt_data = sqrt(dt_data);

    /* compute normalization coefficients */
    a = dt_ref / dt_data;
    b = m_ref - a * m_data;

    /* normalize the array */
    ptr_data = data;
    for (i=0; i<size; i++) {
        *ptr_data = a * *ptr_data + b;
        ptr_data++;
    }
}
