/*
 * Copyright (c) 2014 by Cadence Design Systems Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Cadence Design Systems Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Cadence Design Systems Inc.
 */
#include <stdio.h>
#include <math.h>
#include <complex.h>

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

double W_re(int N, int M)
{
  return cos (2*M_PI*M/N);
}

double W_im(int N, int M)
{
  return sin (2*M_PI*M/N);
}

/*-------------------------------------------------------------------------
Calculate FFT twiddle table size
Input:
  fft_size	size in elements

Restrictions:

  fft_size must be a power of 2

Returned value:
  Size in bytes
---------------------------------------------------------------------------*/
int size_fft_twiddle_table (
    int fft_size)          /**< [in] table size in complex elements */		
{
    return (sizeof (complex float) * 3 * fft_size / 4);
}

/*-------------------------------------------------------------------------
Precomputes twiddle factor table for a given fft size.
Input:
  fft_size	size in elements

Output:
  p_twiddles	twiddle table

Restrictions:

  fft_size must be a power of 2

Returned value:
  Size in elements
---------------------------------------------------------------------------*/
int generate_fft_twiddle_table (
    int fft_size,         /**< [in] table size in complex elements */
    float *p_twiddles)    /**< [out] table output.  Size in floats is 2 * 3 * fft_size / 4 */
{
    float *p_tw = p_twiddles;
    int size = fft_size;
    int i, j;

    for (i=0; i < size/4; i++) { /* this loop corresponds to vectorized tensor loop */
        for (j=1; j < 4; j++) {  /* this loop corresponds to radix kernel */
        	int p = -1 * j * i;
        	*p_tw++ = W_re (size, p);
        	*p_tw++ = W_im (size, p);
        }
    }
    return (p_tw - p_twiddles)/2;
}

