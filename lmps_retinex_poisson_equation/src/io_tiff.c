/**
 * @file lib/src/io_tiff.c
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 *
 * tiff io handling
 *
 * @todo always deinterlace
 */

#include <stdlib.h>

#include <tiffio.h>

#include "mw4.h"

/**
 * @brief load a TIFF image file data as RGB 8 bits values
 *
 * The array is allocated by this function
 *
 * @param fname the file name to read
 * @param nx, ny storage space for the image size
 * @return the data array pointer, NULL if an error occured
 *
 * @todo handle 16 bit int and float tiff images
 */
unsigned char *mw4_read_tiff(const char *fname,
                             unsigned int *nx, unsigned int *ny)
{
    TIFF *tiffp = NULL;

    uint32 *data = NULL;
    uint32 width, height;

    /* no warning messages */
    (void) TIFFSetWarningHandler(NULL);

    /* open the TIFF file and structure */
    if (NULL == (tiffp = TIFFOpen(fname, "r")))
        return NULL;

    /* read width and height and allocate the storage raster */
    if (1 != TIFFGetField(tiffp, TIFFTAG_IMAGEWIDTH, &width)
        || 1 != TIFFGetField(tiffp, TIFFTAG_IMAGELENGTH, &height)
        || NULL == (data = (uint32 *) malloc(width * height
                                             * sizeof(uint32))))
    {
        TIFFClose(tiffp);
        return NULL;
    }

    if (NULL != nx)
        *nx = (unsigned int) width;
    if (NULL != ny)
        *ny = (unsigned int) height;

    /* read the image data */
    if (1 != TIFFReadRGBAImageOriented(tiffp, width, height, data,
                                       ORIENTATION_TOPLEFT, 1))
    {
        free(data);
        TIFFClose(tiffp);
        return NULL;
    }

    /* close the TIFF file and structure */
    TIFFClose(tiffp);

    return (unsigned char *) data;
}

/**
 * @brief save an array into a TIFF file
 *
 * @return 0 if OK, != 0 if an error occured
 *
 * @todo handle 16 bit int and float tiff images
 */
int mw4_write_tiff(const char *fname, unsigned char *data,
                   unsigned int nx, unsigned int ny)
{
    TIFF *tiffp = NULL;

    /* ensure the data is allocated
     * and the width and height are within the limits
     * (tiff uses uint32, 2^32 - 1 = 4294967295)
     * and open the TIFF file and structure
     */
    if (NULL == data
        || 4294967295. < (double) nx || 4294967295. < (double) ny)
        return -1;

    /* no warning messages */
    (void) TIFFSetWarningHandler(NULL);

    /* open the TIFF file and structure */
    if (NULL == (tiffp = TIFFOpen(fname, "w")))
        return -1;

    /* insert tags into the TIFF structure */
    if (1 != TIFFSetField(tiffp, TIFFTAG_IMAGEWIDTH, nx)
        || 1 != TIFFSetField(tiffp, TIFFTAG_IMAGELENGTH, ny)
        || 1 != TIFFSetField(tiffp, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT)
        || 1 != TIFFSetField(tiffp, TIFFTAG_BITSPERSAMPLE, 8)
        || 1 != TIFFSetField(tiffp, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)
        || 1 != TIFFSetField(tiffp, TIFFTAG_ROWSPERSTRIP, ny)
        || 1 != TIFFSetField(tiffp, TIFFTAG_SAMPLESPERPIXEL, 4)
        || 1 != TIFFSetField(tiffp, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB))
    {
        TIFFClose(tiffp);
        return -1;
    }

    /* write the image as one single tile */
    if (4 * nx * ny != (unsigned int)
        TIFFWriteEncodedStrip(tiffp, (tstrip_t) 0, (tdata_t) data,
                              (tsize_t) (4 * nx * ny)))
    {
        TIFFClose(tiffp);
        return -1;
    }

    /* free the TIFF and structure, return success */
    TIFFClose(tiffp);
    return 0;
}

/**
 * @brief deinterlace a TIFF RGBA data array into one or three float RGB arrays
 *
 * @param data_out_rgb array of 3 pointers to 3 output arrays, preallocated
 * @param data_in input array
 * @param size array size
 * @return data_out_rgb or NULL if error
 */
float **mw4_deinterlace(float **data_out_rgb,
                        unsigned char *data_in, size_t size)
{
    uint32 *ptr_in, *ptr_end;
    float *ptr_out_r, *ptr_out_g, *ptr_out_b;

    /* check allocaton */
    if (NULL == data_out_rgb
        || NULL == data_out_rgb[0]
        || NULL == data_out_rgb[1]
        || NULL == data_out_rgb[2] || NULL == data_in)
        return NULL;

    /* setup the pointers */
    ptr_out_r = data_out_rgb[0];
    ptr_out_g = data_out_rgb[1];
    ptr_out_b = data_out_rgb[2];
    ptr_in = (uint32 *) data_in;
    ptr_end = ptr_in + size;

    /* run */
    while (ptr_in < ptr_end)
    {
        *ptr_out_r++ = (float) TIFFGetR(*ptr_in);
        *ptr_out_g++ = (float) TIFFGetG(*ptr_in);
        *ptr_out_b++ = (float) TIFFGetB(*ptr_in);
        ptr_in++;               /* skip the alpha channel */
    }

    return data_out_rgb;
}

/**
 * @brief convert 1 or 3 float arrays to an interlaced RGBA array
 *
 * The data already present on the output array is kept.
 *
 * @param data_out output array, preallocated
 * @param data_in_rgb array of 3 pointers to 3 input arrays
 * @param size array size
 * @return data_out or NULL if errors
 */
unsigned char *mw4_interlace(unsigned char *data_out,
                             float **data_in_rgb, size_t size)
{
    unsigned char *ptr_out, *ptr_end;
    const float *ptr_in_r, *ptr_in_g, *ptr_in_b;

    /* check allocaton */
    if (NULL == data_in_rgb
        || NULL == data_in_rgb[0]
        || NULL == data_in_rgb[1]
        || NULL == data_in_rgb[2] || NULL == data_out)
        return NULL;

    /* setup the pointers */
    ptr_in_r = data_in_rgb[0];
    ptr_in_g = data_in_rgb[1];
    ptr_in_b = data_in_rgb[2];
    ptr_out = data_out;
    ptr_end = ptr_out + 4 * size;

    /* run */
    while (ptr_out < ptr_end)
    {
        /* FIXME: endianness...?? */
        *ptr_out++ = (unsigned char) (*ptr_in_r++);
        *ptr_out++ = (unsigned char) (*ptr_in_g++);
        *ptr_out++ = (unsigned char) (*ptr_in_b++);
        ptr_out += 1;           /* skip the unused alpha channel */
    }

    return data_out;
}

#ifdef CHECK
#include "check_io_tiff.c"
#endif                          /* CHECK */
