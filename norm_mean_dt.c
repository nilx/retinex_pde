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
 * @file norm_brief_dt.c
 * @brief image normalisation
 *
 * @author Jose-Luis Lisani <joseluis.lisani@uib.es>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @brief normalize mean and variance of an image given a reference
 *        image
 *
 * @param out input/output data
 * @param in reference image
 * @param size size of the input/output
 */
void norm_dt(float *out, float *in, size_t size)
{
    double m_in, m_out, dt_in, dt_out; /* means and variances */
    double a, b; /* normalization coefficients */
    size_t i;
    float *ptr_in, *ptr_out;

    /* sanity check */
    if (NULL == out || NULL == in) {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

    /* compute input mean and variance */
    m_in = 0.;
    dt_in = 0.;
    ptr_in = in;
    for (i=0; i< size; i++) {
        m_in += *ptr_in;
        dt_in += (*ptr_in) * (*ptr_in);
        ptr_in++;
    }
    m_in /= (double) size;
    dt_in /= (double) size;
    dt_in -= (m_in * m_in);
    dt_in = sqrt(dt_in);

    /* compute pre-normalization output mean and variance */
    m_out = 0.;
    dt_out = 0.;
    ptr_out = out;
    for (i=0; i<size; i++) {
        m_out += *ptr_out;
        dt_out += (*ptr_out) * (*ptr_out);
        ptr_out++;
    }
    m_out /= (double) size;
    dt_out /= (double) size;
    dt_out -= (m_out * m_out);
    dt_out = sqrt(dt_out);

    /* compute normalization coefficients */
    a = dt_in / dt_out;
    b = m_in - a * m_out;

    /* normalize the output image */
    ptr_out = out;
    for (i=0; i<size; i++) {
        *ptr_out = a * *ptr_out + b;
        ptr_out++;
    }
}
