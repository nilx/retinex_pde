/* io_tiff.c */
float *read_tiff_rgba_float(const char *fname, size_t *nx, size_t *ny);
int write_tiff_rgba_float(const char *fname, const float *data, size_t nx, size_t ny);
