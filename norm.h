#ifndef _NORM_H
#define _NORM_H

#ifdef __cplusplus
extern "C" {
#endif

/* norm.c */
void normalize_mean_dt(float *data, const float *ref, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* !_NORM_H */
