/**
********************************************************************************
*
* @file: adi_sharcfx_maxpool.c
*
* @brief: Contains optimized maxpool function
*
* @details: Contains optimized maxpool function for 8bit integer input
*
*******************************************************************************
 Copyright(c) 2024 Analog Devices, Inc. All Rights Reserved. This software is
 proprietary & confidential to Analog Devices, Inc. and its licensors. By using
 this software you agree to the terms of the associated Analog Devices License
 Agreement.
*******************************************************************************
*/

/*============= I N C L U D E S =============*/
#include "adi_sharcfx_nn.h"

/*============= C O D E =============*/

/**
*******************************************************************************
* Function: adi_sharcfx_maxpool_int8
* @brief optimized maxpool function for 8-bit integer input 
*
* @details optimized maxpool function for 8-bit integer input 
*
* Parameters:
* @param [in] input_y - input height
* @param [in] input_x - input width
* @param [in] output_y - output height
* @param [in] output_x - output width
* @param [in] stride_y - stride height
* @param [in] stride_x - stride width
* @param [in] kernel_y - kernel height
* @param [in] kernel_x - kernel width
* @param [in] pad_y - padding height 
* @param [in] pad_x - padding width
* @param [in] act_min - min value after activation function
* @param [in] act_max - max value after activation function
* @param [in] ch_src - input depth (channels)
* @param [in] src - input data
* 
* @param [out] dst - output data
*
* @return None
*
*
*******************************************************************************
*/ 
void adi_sharcfx_maxpool_int8(
							const int32_t input_y,
							const int32_t input_x,
							const int32_t output_y,
							const int32_t output_x,
							const int32_t stride_y,
							const int32_t stride_x,
							const int32_t kernel_y,
							const int32_t kernel_x,
							const int32_t pad_y,
							const int32_t pad_x,
							const int32_t act_min,
							const int32_t act_max,
							const int32_t ch_src,
						    const int8_t *src,
						    int8_t *dst)
{
	xb_vec4Mx8 in_vec0;
	xb_vec4Mx8 act_ll = act_min;
	xb_vec4Mx8 act_hl = act_max;
	xb_vec4Mx8* __restrict pvOut  = (xb_vec4Mx8 *)(dst);
	for (int i_y = 0; i_y < output_y; i_y++)
	{
		for (int i_x = 0; i_x < output_x; i_x++ )//process upto 4*PDX_M at a time
		{

			//kernel X and Y axis starting and end points
			const int32_t k_y_start = MAX(0, i_y * stride_y - pad_y);
			const int32_t k_y_end = MIN(i_y * stride_y - pad_y + kernel_y, input_y);
			const int32_t k_x_start = MAX(0, i_x * stride_x - pad_x);
			const int32_t k_x_end = MIN(i_x * stride_x - pad_x + kernel_x, input_x);

			//input and output
			const int8_t *src_in = src + i_y * stride_y * input_x * ch_src + i_x * stride_x * ch_src;

			//process for all channels
			int nBytesLeft = ch_src;
			for (int nc = 0; nc < ch_src;nc += 4*PDX_M)
			{
				valign pvOuta = PDX_LA_4MX8_PP(pvOut);
				xb_vec4Mx8 out = 0xFFffFF80;
				for (int k_y = 0; k_y < k_y_end - k_y_start; k_y++)
				{
					for (int k_x = 0; k_x < k_x_end - k_x_start; k_x++)
					{
						xb_vec4Mx8 *in_ptr = (xb_vec4Mx8 *)(src_in + ( (k_x * ch_src + k_y * input_x * ch_src)) + nc); //src_in modified at end
						valign in_ptra = PDX_LA_4MX8_PP (in_ptr);
						PDX_LA_4MX8_XP(in_vec0, in_ptra, in_ptr, 4*PDX_M);
						//find max
						out = PDX_MAX_4MX8(out,in_vec0);
					}
				}

				out = PDX_MAX_4MX8(out,act_ll);
				out = PDX_MIN_4MX8(out,act_hl);

				int nBytesWrite = MIN(nBytesLeft,4*PDX_M);
				PDX_SAV_4MX8_XP (out, pvOuta, pvOut, nBytesWrite);
				PDX_SAPOS_4MX8_FP (pvOuta, pvOut);
				nBytesLeft -= nBytesWrite;
			}
		}
	}
}






