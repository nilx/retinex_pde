#ifndef _RETINEX_PDE_LIB_H
#define _RETINEX_PDE_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* retinex_pde_lib.c */
float *retinex_pde(float *data, size_t nx, size_t ny, float t);

#ifdef __cplusplus
}
#endif

#endif /* !_RETINEX_PDE_LIB_H */
