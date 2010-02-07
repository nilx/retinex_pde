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
 * @file normalization.c
 * @brief normalization routines for float arrays
 *
 * The core routine, flatten_minmax_nb_f32(), implicitly assumes that
 * the float values can be converted to int.
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 *
 * @todo intelligent merge with the u8 versions
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

/**
 * @brief get the min/max of a float array
 *
 * @param data input array
 * @param size array size
 * @param ptr_min, ptr_max pointers to the returned values, ignored if NULL
 */
static void minmax_f32(const float *data, size_t size,
		       float *ptr_min, float *ptr_max)
{
    const float *ptr_data, *ptr_end;
    float min, max;

    /* sanity check */
    if (NULL == data)
    {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }

    /* compute min and max */
    ptr_data = data;
    ptr_end = ptr_data + size;
    min = *ptr_data;
    max = *ptr_data;
    ptr_data++;
    while (ptr_data < ptr_end)
    {
        if (*ptr_data < min)
            min = *ptr_data;
        if (*ptr_data > max)
            max = *ptr_data;
        ptr_data++;
    }

    /* save min and max to the returned pointers if available */
    if (NULL != ptr_min)
        *ptr_min = min;
    if (NULL != ptr_max)
        *ptr_max = max;
    return;
}

/**
 * @brief remove extremal pixels from a float array
 *
 * This function operates in-place. It flattens (saturates) the values
 * < flat_min (resp. > flat_max) to flat_min (resp. flat_max).
 *
 * @param data input/output
 * @param size data array size
 * @param flat_min, flat_max flattening limits
 */
static void flatten_minmax_f32(float *data, size_t size,
			       float flat_min, float flat_max)
{
    float *ptr_data, *ptr_end;

    /* sanity check */
    if (NULL == data)
    {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }
    if (flat_max < flat_min)
    {
	float tmp;

	tmp = flat_min;
	flat_min = flat_max;
	flat_max = tmp;
	fprintf(stderr, "flat_min > flat_max\n");
	fprintf(stderr, "switching flat_min and flat_max\n");
    }

    /* flatten the values */
    ptr_data = data;
    ptr_end = ptr_data + size;
    while (ptr_data < ptr_end)
    {
        if (*ptr_data < flat_min)
            *ptr_data++ = flat_min;
        else if (*ptr_data > flat_max)
            *ptr_data++ = flat_max;
        else
            ptr_data++;
    }
    return;
}

/**
 * @brief flatten extremal pixels from a float array for a total
 * number of flattened pixels
 *
 * This function implicitly assumes that the float values can be
 * rounded to int, ie float values are in [-2^31..2^31[ (for typical
 * 32bit systems). The histogram is an approximation and built on this
 * assumption.
 *
 * @todo implement an exact float version
 *
 * This function operates in-place. It flattens the values < flat_min
 * (resp. > flat_max) to flat_min (resp. flat_max) with (flat_min,
 * flat_max) such that the number of flattened pixels is inferior or
 * equal to flat_nb_min and flat_nb_max.
 *
 * @param data input/output
 * @param size data array size
 * @param flat_nb_min, flat_nb_max number of pixels to flatten
 * @param ptr_newmin, ptr_newmax new data min/max output, ignored if NULL
 */
static void flatten_minmax_nb_f32(float *data, size_t size,
				  size_t flat_nb_min, size_t flat_nb_max,
				  float *ptr_newmin, float *ptr_newmax)
{
    float *data_ptr, *data_end;
    size_t *histo;
    size_t *histo_ptr, *histo_end;
    size_t histo_size;
    int histo_offset;
    float min, max;
    float flat_min, flat_max;

    /* sanity check */
    if (NULL == data)
    {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }
    if (flat_nb_min + flat_nb_max >= size)
    {
	flat_nb_min = (size - 1) / 2;
	flat_nb_max = (size - 1) / 2;
        fprintf(stderr, "the number of pixels to flatten is too large\n");
        fprintf(stderr, "using (size - 1) / 2\n");
    }

    /* compute min/max and the size and offset of the histogram */
    /*
     * for example, with values in [-510.7, 312.7]
     * - min = -510.7 will be -511 when rounded
     * - max =  312.7 will be  313 when rounded
     * - the histogram needs 824 bins
     * - the histogram will be indexed [0.. 823]
     * - data values are rounded and an offset value of 511 is added
     *   before filling the histogram
     */
    minmax_f32(data, size, &min, &max);
    histo_size = (size_t) ((int) (max + .5) - (int) (min + .5)) + 1;
    /* the offset can be <0, it must not be size_t */
    histo_offset = (int) (min + .5);

    /* create the histogram and set to 0 */
    if (NULL == (histo = (size_t *) malloc(histo_size * sizeof(size_t))))
    {
        fprintf(stderr, "allocation error\n");
        abort();
    }
    histo_ptr = histo;
    histo_end = histo_ptr + histo_size;
    while (histo_ptr < histo_end)
        *histo_ptr++ = 0;
    /* fill the histogram */
    data_ptr = data;
    data_end = data_ptr + size;
    min = *data_ptr;
    max = *data_ptr;
    while (data_ptr < data_end)
    {
        histo[(size_t) ((int) (*data_ptr + .5) - histo_offset)] += 1;
        if (*data_ptr < min)
            min = *data_ptr;
        if (*data_ptr > max)
            max = *data_ptr;
        data_ptr++;
    }
    /* reset the histogram pointer to the second histogram value */
    histo_ptr = histo + 1;
    /* convert the histogram to a cumulative histogram */
    while (histo_ptr < histo_end)
    {
        *histo_ptr += *(histo_ptr - 1);
        histo_ptr++;
    }

    /* get the new min/max */
    if (0 == flat_nb_min)
        flat_min = min;
    else
    {
	/* simple forward traversal of the cumulative histogram */
	/* search the first value > flat_nb_min */
        histo_ptr = histo;
        while (histo_ptr < histo_end && *histo_ptr <= flat_nb_min)
            histo_ptr++;
	/* the corresponding histogram value is the current cell indice */
        flat_min = histo_ptr - histo;
	/* get the original float data value */
        flat_min += (float) histo_offset;
    }
    if (0 == flat_nb_max)
        flat_max = max;
    else
    {
	/* simple backward traversal of the cumulative histogram */
	/* search the first value <= size - flat_nb_max */
        histo_ptr = histo_end - 1;
        while (histo_ptr >= histo && *histo_ptr > (size - flat_nb_max))
            histo_ptr--;
	/*
	 * if we are not at the end of the histogram,
	 * get to the next cell,
	 * ie the last (backward) value > size - flat_nb_max
	 */
	if (histo_ptr < histo_end - 1)
	    histo_ptr++;
        flat_max = histo_ptr - histo;
	/* get the original float data value */
        flat_max += (float) histo_offset;
    }

    /* flatten */
    flatten_minmax_f32(data, size, flat_min, flat_max);
    if (NULL != ptr_newmin)
        *ptr_newmin = flat_min;
    if (NULL != ptr_newmax)
        *ptr_newmax = flat_max;
    return;
}

/**
 * @brief normalize a float array
 *
 * This function operates in-place. It computes the minimum and
 * maximum values of the data, and rescales the data to the target
 * minimum and maximum, with optionnally flattening some extremal
 * pixels.
 *
 * @param data input/output array
 * @param size array size
 * @param target_min, target_max target min/max values
 * @param flat_nb_min, flat_nb_max number extremal pixels flattened
 *
 * @return data
 */
float *normalize_f32(float *data, size_t size,
		     float target_min, float target_max,
		     size_t flat_nb_min, size_t flat_nb_max)
{
    float *data_ptr, *data_end;
    float min, max;
    double scale;
    float target_mid;

    /* sanity checks */
    if (NULL == data)
    {
        fprintf(stderr, "a pointer is NULL and should not be so\n");
        abort();
    }
    if (flat_nb_min + flat_nb_max >= size)
    {
	flat_nb_min = (size - 1) / 2;
	flat_nb_max = (size - 1) / 2;
        fprintf(stderr, "the number of pixels to flatten is too large\n");
        fprintf(stderr, "using (size - 1) / 2\n");
    }

    /* setup iteration pointers */
    data_ptr = data;
    data_end = data_ptr + size;

    /* target_max == target_min : shortcut */
    if (target_max == target_min)
    {
        while (data_ptr < data_end)
            *data_ptr++ = target_min;
        return data;
    }

    if (0 != flat_nb_min || 0 != flat_nb_max)
        /* flatten extremal pixels and get the min/max */
        flatten_minmax_nb_f32(data, size, flat_nb_min, flat_nb_max,
			      &min, &max);
    else
        minmax_f32(data, size, &min, &max);

    /* rescale */
    /* max == min : constant output */
    if (max == min)
    {
        target_mid = (target_max + target_min) / 2;
        while (data_ptr < data_end)
            *data_ptr++ = target_mid;
    }
    else
    {
        /*
	 * normalize the data
	 * such that norm(min) = target_min
	 *           norm(max) = target_max
	 * norm(x) = (x - min) * (t_max - t_min) / (max - min) + t_min
	 */
        scale = (double) (target_max - target_min) / (double) (max - min);
        while (data_ptr < data_end)
        {
            *data_ptr = (float) ((*data_ptr - min) * scale + target_min);
            data_ptr++;
        }
    }
    return data;
}
