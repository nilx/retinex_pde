/**
 * @file lib/src/algo_normalize.c
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 *
 * Normalization tools.
 *
 * @todo use more size_t/ptrdiff_t?
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "mw4.h"

/**
 * @brief get the min/max of an float array
 *
 * @param data input array
 * @param size array size
 * @param min, max retunred values
 */
static void mw4_minmax(const float *data, size_t size, float *min, float *max)
{
    const float *ptr_data, *ptr_end;
    float _min, _max;

    /* sanity check */
    if (NULL == data)
        MW4_FATAL(MW4_MSG_NULL_PTR);

    /* compute min and max */
    ptr_data = data;
    ptr_end = ptr_data + size;
    _min = *ptr_data;
    _max = *ptr_data;
    ptr_data++;
    while (ptr_data < ptr_end)
    {
        if (*ptr_data < _min)
            _min = *ptr_data;
        if (*ptr_data > _max)
            _max = *ptr_data;
        ptr_data++;
    }
    *min = _min;
    *max = _max;
    return;
}

/**
 * @brief remove extremal pixels from a float array
 *
 * This function operates in-place. It flattens the values <= flat_min
 * (resp. >= flat_max) to flat_min (resp. flat_max).
 *
 * @param data input/output
 * @param size data array size
 * @param flat_min, flat_max flattening limits
 */
static void mw4_flatten_minmax(float *data, size_t size,
                               float flat_min, float flat_max)
{
    float *ptr_data, *ptr_end;

    /* sanity check */
    if (NULL == data)
        MW4_FATAL(MW4_MSG_NULL_PTR);
    if (flat_max < flat_min)
        MW4_FATAL(MW4_MSG_BAD_PARAM);

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
 * number of pixels
 *
 * This function operates in-place. It flattens the values < flat_min
 * (resp. > flat_max) to flat_min (resp. flat_max) with (flat_min,
 * flat_max) such that the number of flattened pixels is superior or
 * equal to flat_nb_min and flat_nb_max.
 *
 * @param data input/output
 * @param size data array size
 * @param min, max data min/max
 * @param flat_nb_min, flat_nb_max number of pixels to flatten
 * @param newmin, newmax new data min/max output, ignored if NULL
 */
static void mw4_flatten_minmax_nb(float *data, size_t size,
                                  float min, float max,
                                  size_t flat_nb_min, size_t flat_nb_max,
                                  float *newmin, float *newmax)
{
    size_t *histo;
    float *data_ptr, *data_end;
    float flat_min, flat_max;
    double imin, imax;
    size_t i, nb;
    double size_t_max = (double) (size_t) - 1;

    /* sanity check */
    if (NULL == data)
        MW4_FATAL(MW4_MSG_NULL_PTR);
    if (flat_nb_min >= size || flat_nb_max >= size)
        MW4_FATAL(MW4_MSG_BAD_PARAM);

    /* make an cumulative histogram */
    imin = floor(min + .5);
    imax = floor(max + .5);
    if (size_t_max < imax - imin + 1)
        MW4_FATALF("the data range is too wide for an integer histogram :"
                   " %f - %f", imin, imax);
    if (NULL == (histo = (size_t *) calloc((size_t) (imax - imin + 1),
                                           sizeof(size_t))))
        MW4_FATAL(MW4_MSG_ALLOC_ERR);
    data_ptr = data;
    data_end = data_ptr + size;
    while (data_ptr < data_end)
        histo[(size_t) (floor(*data_ptr++ + .5) - imin)] += 1;

    /* get the new min/max */
    if (0 < flat_nb_min)
    {
        nb = 0;
        i = 0;
        while (nb < flat_nb_min)
            nb += histo[i++];
        if (nb > flat_nb_min)
            i--;
        flat_min = imin + i;
    }
    else
        flat_min = imin;
    if (0 < flat_nb_max)
    {
        nb = 0;
        i = (size_t) (imax - imin);
        while (nb < flat_nb_max)
            nb += histo[i--];
        if (nb > flat_nb_max)
            i++;
        flat_max = imin + i;
    }
    else
        flat_max = imax;
    if (flat_max < flat_min)
    {
        flat_min = (flat_min + flat_max) / 2;
        flat_max = flat_min;
    }

    /* flatten */
    mw4_flatten_minmax(data, size, (float) flat_min, (float) flat_max);
    if (NULL != newmin)
        *newmin = (float) flat_min;
    if (NULL != newmax)
        *newmax = (float) flat_max;
    free(histo);
    return;
}

/**
 * @brief perform x<-a * x + b on an array
 *
 * This function operates in-place.
 *
 * @param data input/output array
 * @param size array size
 * @param a
 * @param b
 */
static void mw4_axpb(float *data, size_t size, float a, float b)
{
    float *ptr_data, *ptr_end;

    if (NULL == data)
        MW4_FATAL(MW4_MSG_NULL_PTR);

    ptr_data = data;
    ptr_end = data + size;
    while (ptr_data < ptr_end)
    {
        *ptr_data *= a;
        *ptr_data++ += b;
    }
    return;
}

/**
 * @brief normalize a float array
 *
 * This function operates in-place. It computes the minimum and
 * maximum values of the data, and rescales the data to the target
 * minimum and maximum, with optionnally ignoring some extremal
 * pixels.
 *
 * @param data input/output array
 * @param size array size
 * @param target_min, target_max target min/max values
 * @param flatten_min, flatten_max proportion of extremal pixels
 *        flattened (in [0..1], sum <= 1)
 *
 * @return data
 *
 * @todo fix max=min
 */
float *mw4_normalize(float *data, size_t size,
                     float target_min, float target_max,
                     float flatten_min, float flatten_max)
{
    float min, max;
    float scale, offset;
    float flat_nb_min, flat_nb_max;

    /* sanity checks */
    if (NULL == data)
        MW4_FATAL(MW4_MSG_NULL_PTR);
    if (1. < flatten_min + flatten_max
        || 0. > flatten_min
        || 1. < flatten_min || 0. > flatten_max || 1. < flatten_max)
        MW4_FATAL(MW4_MSG_BAD_PARAM);

    /* target_max == target_min : shortcut */
    if (FLT_MIN > fabs(target_max - target_min))
    {
        mw4_axpb(data, size, 0., target_min);
        return data;
    }

    /* compute min and max */
    mw4_minmax(data, size, &min, &max);
    MW4_DEBUGF("min=%g, max=%g", min, max);

    flat_nb_min = floor(flatten_min * (float) size + .5);
    flat_nb_max = floor(flatten_max * (float) size + .5);

    if (0 < flat_nb_min || 0 < flat_nb_max)
        /* flatten extremal pixels and update min/max */
        mw4_flatten_minmax_nb(data, size, min, max,
                              (size_t) flat_nb_min, (size_t) flat_nb_max,
                              &min, &max);

    MW4_DEBUGF("min=%g, max=%g", min, max);
    /* check possible precision loss due to float arithmetics */
    if (0. > target_min * target_max)
        MW4_WARN(MW4_MSG_PRECISION_LOSS);

    /* rescale */
    /* max == min : do nothing */
    if (0. < fabs(max - min))
    {
        scale = (target_max - target_min) / (max - min);
        offset = target_min - min * scale;
        mw4_axpb(data, size, scale, offset);
    }
    MW4_DEBUGF("min=%g, max=%g", target_min, target_max);
    return data;
}

#ifdef CHECK
#include "check_algo_normalize.c"
#endif                          /* CHECK */
