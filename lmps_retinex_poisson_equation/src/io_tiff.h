/* io_tiff.c */
float *read_tiff_rgba_f32(const char *fname, size_t *nx, size_t *ny);
unsigned char *read_tiff_rgba_u8(const char *fname, size_t *nx, size_t *ny);
int write_tiff_rgba_f32(const char *fname, const float *data, size_t nx, size_t ny);
int write_tiff_rgba_u8(const char *fname, const unsigned char *data, size_t nx, size_t ny);
