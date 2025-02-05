/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2022, Advanced Micro Devices, Inc. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name(s) of the copyright holder(s) nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <immintrin.h>
#include <string.h>

#include "blis.h"
#include "lpgemm_kernels.h"
#include "lpgemm_f32_kern_macros.h"

#ifdef BLIS_KERNELS_ZEN4
// 6xlt16 bf16 fringe kernel
LPGEMM_N_LT_NR0_FRINGE_KERN(bfloat16, bfloat16, float, bf16bf16f32of32_6xlt16)
{
	static void* post_ops_labels[] =
						{
						  &&POST_OPS_6xLT16_DISABLE,
						  &&POST_OPS_BIAS_6xLT16,
						  &&POST_OPS_RELU_6xLT16,
						  &&POST_OPS_RELU_SCALE_6xLT16,
						  &&POST_OPS_DOWNSCALE_6xLT16
						};
	dim_t MR = 6;
	dim_t m_full_pieces = m0 / MR;
	dim_t m_full_pieces_loop_limit = m_full_pieces * MR;
	dim_t m_partial_pieces = m0 % MR;

	dim_t k_full_pieces = k0 / 2;
	dim_t k_partial_pieces = k0 % 2;

	int32_t a_kfringe_buf = 0;

    // B matrix storage bfloat type
	__m512bh b0;

	// A matrix storage bfloat type
	__m512bh a_bf16_0;

	// For corner cases.
	float buf0[16];
	float buf1[16];
	float buf2[16];
	float buf3[16];
	float buf4[16];
	float buf5[16];

	for ( dim_t ir = 0; ir < m_full_pieces_loop_limit; ir += MR )
	{
		// Registers to use for accumulating C.
		__m512 c_float_0p0 = _mm512_setzero_ps();

		__m512 c_float_1p0 = _mm512_setzero_ps();

		__m512 c_float_2p0 = _mm512_setzero_ps();
		
		__m512 c_float_3p0 = _mm512_setzero_ps();

		__m512 c_float_4p0 = _mm512_setzero_ps();

		__m512 c_float_5p0 = _mm512_setzero_ps();

		for ( dim_t kr = 0; kr < k_full_pieces; kr += 1 )
		{
			// Load 2 rows with 16 extended elements each from B to 1 ZMM
			// registers. It is to be noted that the B matrix is packed for use
			// in bf16 instructions and each load to ZMM register will have 2
			// elements along k direction and 16 elements across n directions,
			// so 2x16 elements to a ZMM register.
			b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * kr ) + ( cs_b * 0 ) );

			// Broadcast a[0,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 0 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-15] = a[0,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			
			// Broadcast a[1,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 1 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-15] = a[1,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			
			// Broadcast a[2,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 2 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-15] = a[2,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			
			// Broadcast a[3,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 3 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-15] = a[3,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			
			// Broadcast a[4,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 4 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-15] = a[4,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			
			// Broadcast a[5,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 5 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-15] = a[5,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
		}
        
		// Handle k remainder.
		if ( k_partial_pieces > 0 )
		{
			b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * k_full_pieces ) + ( cs_b * 0 ) );

			// Broadcast a[0,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 0 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-15] = a[0,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			
			// Broadcast a[1,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 1 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-15] = a[1,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			
			// Broadcast a[2,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 2 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-15] = a[2,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			
			// Broadcast a[3,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 3 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-15] = a[3,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			
			// Broadcast a[4,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 4 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-15] = a[4,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			
			// Broadcast a[5,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 5 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-15] = a[5,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
		}
        
		// Load alpha and beta
		__m512 selector1 = _mm512_set1_ps( alpha );
		__m512 selector2 = _mm512_set1_ps( beta );

		// Scale by alpha
		c_float_0p0 = _mm512_mul_ps( selector1, c_float_0p0 );

		c_float_1p0 = _mm512_mul_ps( selector1, c_float_1p0 );
		
		c_float_2p0 = _mm512_mul_ps( selector1, c_float_2p0 );
		
		c_float_3p0 = _mm512_mul_ps( selector1, c_float_3p0 );
		
		c_float_4p0 = _mm512_mul_ps( selector1, c_float_4p0 );
		
		c_float_5p0 = _mm512_mul_ps( selector1, c_float_5p0 );

		// Scale C by beta.
		if ( beta != 0 )
		{
			memcpy( buf0, ( c + ( rs_c * ( ir + 0 ) ) ), ( n0_rem * sizeof( float ) ) );
			memcpy( buf1, ( c + ( rs_c * ( ir + 1 ) ) ), ( n0_rem * sizeof( float ) ) );
			memcpy( buf2, ( c + ( rs_c * ( ir + 2 ) ) ), ( n0_rem * sizeof( float ) ) );
			memcpy( buf3, ( c + ( rs_c * ( ir + 3 ) ) ), ( n0_rem * sizeof( float) ) );
			memcpy( buf4, ( c + ( rs_c * ( ir + 4 ) ) ), ( n0_rem * sizeof( float ) ) );
			memcpy( buf5, ( c + ( rs_c * ( ir + 5 ) ) ), ( n0_rem * sizeof( float ) ) );
			
			// c[0,0-15]
			selector1 = _mm512_loadu_ps( buf0 );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

			// c[1,0-15]
			selector1 = _mm512_loadu_ps( buf1 );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

			// c[2,0-15]
			selector1 = _mm512_loadu_ps( buf2 );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

			// c[3,0-15]
			selector1 = _mm512_loadu_ps( buf3 );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

			// c[4,0-15]
			selector1 = _mm512_loadu_ps( buf4 );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

			// c[5,0-15]
			selector1 = _mm512_loadu_ps( buf5  );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );
		}
		// Post Ops
		lpgemm_post_op* post_ops_list_temp = post_ops_list;
		POST_OP_LABEL_LASTK_SAFE_JUMP
POST_OPS_BIAS_6xLT16:
		{
			if ( ( *( char* )post_ops_list_temp->op_args2 == 'r' ) ||
				 ( *( char* )post_ops_list_temp->op_args2 == 'R' ) )
			{
				memcpy( buf0, ( ( float* )post_ops_list_temp->op_args1 +
						post_op_c_j ), ( n0_rem * sizeof( float ) ) );
				selector1 = _mm512_loadu_ps( buf0 );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );
			}
			else
			{
				selector1 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 0 ) );
				selector2 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 1 ) );
				__m512 selector3 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 2 ) );
				__m512 selector4 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 3 ) );
				__m512 selector5 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 4 ) );
				__m512 selector6 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 5 ) );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector2, c_float_1p0 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector3, c_float_2p0 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector4, c_float_3p0 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector5, c_float_4p0 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector6, c_float_5p0 );
			}

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_6xLT16:
		{
			selector1 = _mm512_setzero_ps();

			// c[0,0-15]
			c_float_0p0 = _mm512_max_ps( selector1, c_float_0p0 );

			// c[1,0-15]
			c_float_1p0 = _mm512_max_ps( selector1, c_float_1p0 );

			// c[2,0-15]
			c_float_2p0 = _mm512_max_ps( selector1, c_float_2p0 );

			// c[3,0-15]
			c_float_3p0 = _mm512_max_ps( selector1, c_float_3p0 );

			// c[4,0-15]
			c_float_4p0 = _mm512_max_ps( selector1, c_float_4p0 );

			// c[5,0-15]
			c_float_5p0 = _mm512_max_ps( selector1, c_float_5p0 );

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_SCALE_6xLT16:
		{
			selector1 = _mm512_setzero_ps();
			selector2 =
				_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args2 ) );

			__mmask16 relu_cmp_mask;

			// c[0, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_0p0)

			// c[1, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_1p0)

			// c[2, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_2p0)

			// c[3, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_3p0)

			// c[4, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_4p0)

			// c[5, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_5p0)

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_DOWNSCALE_6xLT16:
	{
		// c[0, 0-15]
		CVT_F32_BF16_LT16(c_float_0p0,0,0);

		// c[1, 0-15]
		CVT_F32_BF16_LT16(c_float_1p0,1,0);

		// c[2, 0-15]
		CVT_F32_BF16_LT16(c_float_2p0,2,0);

		// c[3, 0-15]
		CVT_F32_BF16_LT16(c_float_3p0,3,0);

		// c[4, 0-15]
		CVT_F32_BF16_LT16(c_float_4p0,4,0);

		// c[5, 0-15]
		CVT_F32_BF16_LT16(c_float_5p0,5,0);

		POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
	}		
POST_OPS_6xLT16_DISABLE:
		;
		
		// Store the results.
		// c[0,0-15]
		_mm512_storeu_ps( buf0, c_float_0p0 );

		// c[1,0-15]
		_mm512_storeu_ps( buf1, c_float_1p0 );

		// c[2,0-15]
		_mm512_storeu_ps( buf2, c_float_2p0 );

		// c[3,0-15]
		_mm512_storeu_ps( buf3, c_float_3p0 );

		// c[4,0-15]
		_mm512_storeu_ps( buf4, c_float_4p0 );

		// c[5,0-15]
		_mm512_storeu_ps( buf5, c_float_5p0 );

		// Memcpy partial parts.
		// c[0,0-15]
		memcpy( c + ( rs_c * ( ir + 0 ) ) + ( 0*16 ), buf0, ( n0_rem * sizeof( float ) ) );

		// c[1,0-15]
		memcpy( c + ( rs_c * ( ir + 1 ) ) + ( 0*16 ), buf1, ( n0_rem * sizeof( float ) ) );

		// c[2,0-15]
		memcpy( c + ( rs_c * ( ir + 2 ) ) + ( 0*16 ), buf2, ( n0_rem * sizeof( float ) ) );

		// c[3,0-15]
		memcpy( c + ( rs_c * ( ir + 3 ) ) + ( 0*16 ), buf3, ( n0_rem * sizeof( float ) ) );

		// c[4,0-15]
		memcpy( c + ( rs_c * ( ir + 4 ) ) + ( 0*16 ), buf4, ( n0_rem * sizeof( float ) ) );

		// c[5,0-15]
		memcpy( c + ( rs_c * ( ir + 5 ) ) + ( 0*16 ), buf5, ( n0_rem * sizeof( float ) ) );

		a = a + ( MR * ps_a );
		post_op_c_i += MR;
	}
    
	if ( m_partial_pieces > 0 )
	{
		if ( m_partial_pieces == 5 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 5 );
			lpgemm_rowvar_bf16bf16f32of32_5xlt16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta, n0_rem,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 4 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 4 );
			lpgemm_rowvar_bf16bf16f32of32_4xlt16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta, n0_rem,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 3 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 3 );
			lpgemm_rowvar_bf16bf16f32of32_3xlt16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta, n0_rem,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 2 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 2 );
			lpgemm_rowvar_bf16bf16f32of32_2xlt16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta, n0_rem,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 1 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 1 );
			lpgemm_rowvar_bf16bf16f32of32_1xlt16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta, n0_rem,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
	}    
}

// 6x16 bf16 fringe kernel
LPGEMM_N_FRINGE_KERN(bfloat16, bfloat16, float, bf16bf16f32of32_6x16)
{
	static void* post_ops_labels[] =
						{
						  &&POST_OPS_6x16_DISABLE,
						  &&POST_OPS_BIAS_6x16,
						  &&POST_OPS_RELU_6x16,
						  &&POST_OPS_RELU_SCALE_6x16,
						  &&POST_OPS_DOWNSCALE_6x16
						};
	dim_t MR = 6;
	dim_t m_full_pieces = m0 / MR;
	dim_t m_full_pieces_loop_limit = m_full_pieces * MR;
	dim_t m_partial_pieces = m0 % MR;

	dim_t k_full_pieces = k0 / 2;
	dim_t k_partial_pieces = k0 % 2;

	int32_t a_kfringe_buf = 0;
	
	// B matrix storage bfloat type
	__m512bh b0;

	// A matrix storage bfloat type
	__m512bh a_bf16_0;

	for ( dim_t ir = 0; ir < m_full_pieces_loop_limit; ir += MR )
	{
		// Registers to use for accumulating C.
		__m512 c_float_0p0 = _mm512_setzero_ps();

		__m512 c_float_1p0 = _mm512_setzero_ps();

		__m512 c_float_2p0 = _mm512_setzero_ps();
		
		__m512 c_float_3p0 = _mm512_setzero_ps();

		__m512 c_float_4p0 = _mm512_setzero_ps();

		__m512 c_float_5p0 = _mm512_setzero_ps();

		for ( dim_t kr = 0; kr < k_full_pieces; kr += 1 )
		{
			// Load 2 rows with 16 elements each from B to 1 ZMM registers. It
			// is to be noted that the B matrix is packed for use in bf16
			// instructions and each load to ZMM register will have 2 elements
			// along k direction and 16 elements across n directions, so 2x16
			// elements to a ZMM register.
		    b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * kr ) + ( cs_b * 0 ) );

			// Broadcast a[0,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 0 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-15] = a[0,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			
			// Broadcast a[1,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 1 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-15] = a[1,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			
			// Broadcast a[2,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 2 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-15] = a[2,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			
			// Broadcast a[3,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 3 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-15] = a[3,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			
			// Broadcast a[4,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 4 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-15] = a[4,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			
			// Broadcast a[5,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 5 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-15] = a[5,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
		}
		// Handle k remainder.
		
		if ( k_partial_pieces > 0 )
		{
			b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * k_full_pieces ) + ( cs_b * 0 ) );

			// Broadcast a[0,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 0 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-15] = a[0,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			
			// Broadcast a[1,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 1 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-15] = a[1,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			
			// Broadcast a[2,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 2 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-15] = a[2,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			
			// Broadcast a[3,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 3 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-15] = a[3,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			
			// Broadcast a[4,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 4 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-15] = a[4,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			
			// Broadcast a[5,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 5 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-15] = a[5,kr:kr+2]*b[kr:kr+2,0-15]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
		}
		
		// Load alpha and beta
		__m512 selector1 = _mm512_set1_ps( alpha );
		__m512 selector2 = _mm512_set1_ps( beta );

		// Scale by alpha
		c_float_0p0 = _mm512_mul_ps( selector1, c_float_0p0 );

		c_float_1p0 = _mm512_mul_ps( selector1, c_float_1p0 );
		
		c_float_2p0 = _mm512_mul_ps( selector1, c_float_2p0 );
		
		c_float_3p0 = _mm512_mul_ps( selector1, c_float_3p0 );
		
		c_float_4p0 = _mm512_mul_ps( selector1, c_float_4p0 );
		
		c_float_5p0 = _mm512_mul_ps( selector1, c_float_5p0 );

		// Scale C by beta.
		if ( beta != 0 )
		{
			// c[0,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

			// c[1,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

			// c[2,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

			// c[3,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

			// c[4,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

			// c[5,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );
		}
		// Post Ops
		lpgemm_post_op* post_ops_list_temp = post_ops_list;
		POST_OP_LABEL_LASTK_SAFE_JUMP
POST_OPS_BIAS_6x16:
		{
			if ( ( *( char* )post_ops_list_temp->op_args2 == 'r' ) ||
				 ( *( char* )post_ops_list_temp->op_args2 == 'R' ) )
			{
				selector1 =
					_mm512_loadu_ps( ( float* )post_ops_list_temp->op_args1 +
								post_op_c_j );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );
			}
			else
			{
				selector1 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 0 ) );
				selector2 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 1 ) );
				__m512 selector3 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 2 ) );
				__m512 selector4 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 3 ) );
				__m512 selector5 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 4 ) );
				__m512 selector6 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 5 ) );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector2, c_float_1p0 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector3, c_float_2p0 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector4, c_float_3p0 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector5, c_float_4p0 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector6, c_float_5p0 );
			}

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_6x16:
		{
			selector1 = _mm512_setzero_ps();

			// c[0,0-15]
			c_float_0p0 = _mm512_max_ps( selector1, c_float_0p0 );

			// c[1,0-15]
			c_float_1p0 = _mm512_max_ps( selector1, c_float_1p0 );

			// c[2,0-15]
			c_float_2p0 = _mm512_max_ps( selector1, c_float_2p0 );

			// c[3,0-15]
			c_float_3p0 = _mm512_max_ps( selector1, c_float_3p0 );

			// c[4,0-15]
			c_float_4p0 = _mm512_max_ps( selector1, c_float_4p0 );

			// c[5,0-15]
			c_float_5p0 = _mm512_max_ps( selector1, c_float_5p0 );

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_SCALE_6x16:
		{
			selector1 = _mm512_setzero_ps();
			selector2 =
				_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args2 ) );

			__mmask16 relu_cmp_mask;

			// c[0, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_0p0)

			// c[1, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_1p0)

			// c[2, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_2p0)

			// c[3, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_3p0)

			// c[4, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_4p0)

			// c[5, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_5p0)

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_DOWNSCALE_6x16:
	{
		// c[0, 0-15]
		CVT_F32_BF16(c_float_0p0,0,0);

		// c[1, 0-15]
		CVT_F32_BF16(c_float_1p0,1,0);

		// c[2, 0-15]
		CVT_F32_BF16(c_float_2p0,2,0);

		// c[3, 0-15]
		CVT_F32_BF16(c_float_3p0,3,0);

		// c[4, 0-15]
		CVT_F32_BF16(c_float_4p0,4,0);

		// c[5, 0-15]
		CVT_F32_BF16(c_float_5p0,5,0);

		POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
	}		
POST_OPS_6x16_DISABLE:
		;
		
		// Store the results.
		// c[0,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 0*16 ), c_float_0p0 );

		// c[1,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 0*16 ), c_float_1p0 );

		// c[2,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 0*16 ), c_float_2p0 );

		// c[3,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 0*16 ), c_float_3p0 );

		// c[4,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 0*16 ), c_float_4p0 );

		// c[5,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 0*16 ), c_float_5p0 );

		a = a + ( MR * ps_a );
		post_op_c_i += MR;
	}
    
	if ( m_partial_pieces > 0 )
	{
		if ( m_partial_pieces == 5 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 5 );
			lpgemm_rowvar_bf16bf16f32of32_5x16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 4 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 4 );
			lpgemm_rowvar_bf16bf16f32of32_4x16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 3 )
		{
			int cs_a_use = ( cs_a == 2) ? 2 : ( ( cs_a / 6 ) * 3 );
			lpgemm_rowvar_bf16bf16f32of32_3x16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 2 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 2 );
			lpgemm_rowvar_bf16bf16f32of32_2x16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 1 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 1 );
			lpgemm_rowvar_bf16bf16f32of32_1x16
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
	}	
}

// 6x32 bf16 fringe kernel
LPGEMM_N_FRINGE_KERN(bfloat16, bfloat16, float, bf16bf16f32of32_6x32)
{
	static void* post_ops_labels[] =
						{
						  &&POST_OPS_6x32_DISABLE,
						  &&POST_OPS_BIAS_6x32,
						  &&POST_OPS_RELU_6x32,
						  &&POST_OPS_RELU_SCALE_6x32,
						  &&POST_OPS_DOWNSCALE_6x32
						};
	dim_t MR = 6; 
	dim_t m_full_pieces = m0 / MR;
	dim_t m_full_pieces_loop_limit = m_full_pieces * MR;
	dim_t m_partial_pieces = m0 % MR;

	dim_t k_full_pieces = k0 / 2;
	dim_t k_partial_pieces = k0 % 2;

	int32_t a_kfringe_buf = 0;

	// B matrix storage bfloat type
	__m512bh b0;
	__m512bh b1;

	// A matrix storage bfloat type
	__m512bh a_bf16_0;

	for ( dim_t ir = 0; ir < m_full_pieces_loop_limit; ir += MR )
	{
		// Registers to use for accumulating C.
		__m512 c_float_0p0 = _mm512_setzero_ps();
		__m512 c_float_0p1 = _mm512_setzero_ps();

		__m512 c_float_1p0 = _mm512_setzero_ps();
		__m512 c_float_1p1 = _mm512_setzero_ps();

		__m512 c_float_2p0 = _mm512_setzero_ps();
		__m512 c_float_2p1 = _mm512_setzero_ps();
		
		__m512 c_float_3p0 = _mm512_setzero_ps();
		__m512 c_float_3p1 = _mm512_setzero_ps();

		__m512 c_float_4p0 = _mm512_setzero_ps();
		__m512 c_float_4p1 = _mm512_setzero_ps();

		__m512 c_float_5p0 = _mm512_setzero_ps();
		__m512 c_float_5p1 = _mm512_setzero_ps();

		for ( dim_t kr = 0; kr < k_full_pieces; kr += 1 )
		{
			// Load 2 rows with 32 elements each from B to 2 ZMM registers. It
			// is to be noted that the B matrix is packed for use in bf16
			// instructions and each load to ZMM register will have 2 elements
			// along k direction and 32 elements across n directions, so 2x16
			// elements to a ZMM register.
			b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * kr ) + ( cs_b * 0 ) );
			b1 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * kr ) + ( cs_b * 1 ) );

			// Broadcast a[0,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 0 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-31] = a[0,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			c_float_0p1 = _mm512_dpbf16_ps( c_float_0p1, a_bf16_0, b1 );
			
			// Broadcast a[1,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 1 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-31] = a[1,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			c_float_1p1 = _mm512_dpbf16_ps( c_float_1p1, a_bf16_0, b1 );
			
			// Broadcast a[2,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 2 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-31] = a[2,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			c_float_2p1 = _mm512_dpbf16_ps( c_float_2p1, a_bf16_0, b1 );
			
			// Broadcast a[3,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 3 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-31] = a[3,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			c_float_3p1 = _mm512_dpbf16_ps( c_float_3p1, a_bf16_0, b1 );
			
			// Broadcast a[4,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 4 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-31] = a[4,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			c_float_4p1 = _mm512_dpbf16_ps( c_float_4p1, a_bf16_0, b1 );
			
			// Broadcast a[5,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 5 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-31] = a[5,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
			c_float_5p1 = _mm512_dpbf16_ps( c_float_5p1, a_bf16_0, b1 );
		}		
		// Handle k remainder.
		if ( k_partial_pieces > 0 )
		{
			b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * k_full_pieces ) + ( cs_b * 0 ) );
			b1 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * k_full_pieces ) + ( cs_b * 1 ) );

			// Broadcast a[0,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 0 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-31] = a[0,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			c_float_0p1 = _mm512_dpbf16_ps( c_float_0p1, a_bf16_0, b1 );
			
			// Broadcast a[1,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 1 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-31] = a[1,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			c_float_1p1 = _mm512_dpbf16_ps( c_float_1p1, a_bf16_0, b1 );
			
			// Broadcast a[2,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 2 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-31] = a[2,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			c_float_2p1 = _mm512_dpbf16_ps( c_float_2p1, a_bf16_0, b1 );
			
			// Broadcast a[3,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 3 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-31] = a[3,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			c_float_3p1 = _mm512_dpbf16_ps( c_float_3p1, a_bf16_0, b1 );
			
			// Broadcast a[4,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 4 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-31] = a[4,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			c_float_4p1 = _mm512_dpbf16_ps( c_float_4p1, a_bf16_0, b1 );
			
			// Broadcast a[5,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 5 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-31] = a[5,kr:kr+2]*b[kr:kr+2,0-31]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
			c_float_5p1 = _mm512_dpbf16_ps( c_float_5p1, a_bf16_0, b1 );
		}      
		// Load alpha and beta
		__m512 selector1 = _mm512_set1_ps( alpha );
		__m512 selector2 = _mm512_set1_ps( beta );

		// Scale by alpha
		c_float_0p0 = _mm512_mul_ps( selector1, c_float_0p0 );
		c_float_0p1 = _mm512_mul_ps( selector1, c_float_0p1 );

		c_float_1p0 = _mm512_mul_ps( selector1, c_float_1p0 );
		c_float_1p1 = _mm512_mul_ps( selector1, c_float_1p1 );
		
		c_float_2p0 = _mm512_mul_ps( selector1, c_float_2p0 );
		c_float_2p1 = _mm512_mul_ps( selector1, c_float_2p1 );
		
		c_float_3p0 = _mm512_mul_ps( selector1, c_float_3p0 );
		c_float_3p1 = _mm512_mul_ps( selector1, c_float_3p1 );
		
		c_float_4p0 = _mm512_mul_ps( selector1, c_float_4p0 );
		c_float_4p1 = _mm512_mul_ps( selector1, c_float_4p1 );
		
		c_float_5p0 = _mm512_mul_ps( selector1, c_float_5p0 );
		c_float_5p1 = _mm512_mul_ps( selector1, c_float_5p1 );

		// Scale C by beta.
		if ( beta != 0 )
		{
			// c[0,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

			// c[0, 16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_0p1 = _mm512_add_ps( selector1, c_float_0p1 );

			// c[1,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

			// c[1,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_1p1 = _mm512_add_ps( selector1, c_float_1p1 );

			// c[2,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

			// c[2,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_2p1 = _mm512_add_ps( selector1, c_float_2p1 );

			// c[3,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

			// c[3,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_3p1 = _mm512_add_ps( selector1, c_float_3p1 );

			// c[4,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

			// c[4,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_4p1 = _mm512_add_ps( selector1, c_float_4p1 );

			// c[5,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );

			// c[5,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_5p1 = _mm512_add_ps( selector1, c_float_5p1 );
		}
		// Post Ops
		lpgemm_post_op* post_ops_list_temp = post_ops_list;
		POST_OP_LABEL_LASTK_SAFE_JUMP
POST_OPS_BIAS_6x32:
		{
			if ( ( *( char* )post_ops_list_temp->op_args2 == 'r' ) ||
				 ( *( char* )post_ops_list_temp->op_args2 == 'R' ) )
			{
				selector1 =
					_mm512_loadu_ps( ( float* )post_ops_list_temp->op_args1 +
								post_op_c_j + ( 0 * 16 ) );
				selector2 =
					_mm512_loadu_ps( ( float* )post_ops_list_temp->op_args1 +
								post_op_c_j + ( 1 * 16 ) );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[0, 16-31]
				c_float_0p1 = _mm512_add_ps( selector2, c_float_0p1 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

				// c[1, 16-31]
				c_float_1p1 = _mm512_add_ps( selector2, c_float_1p1 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

				// c[2, 16-31]
				c_float_2p1 = _mm512_add_ps( selector2, c_float_2p1 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

				// c[3, 16-31]
				c_float_3p1 = _mm512_add_ps( selector2, c_float_3p1 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

				// c[4, 16-31]
				c_float_4p1 = _mm512_add_ps( selector2, c_float_4p1 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );

				// c[5, 16-31]
				c_float_5p1 = _mm512_add_ps( selector2, c_float_5p1 );
			}
			else
			{
				selector1 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 0 ) );
				selector2 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 1 ) );
				__m512 selector3 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 2 ) );
				__m512 selector4 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 3 ) );
				__m512 selector5 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 4 ) );
				__m512 selector6 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 5 ) );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[0, 16-31]
				c_float_0p1 = _mm512_add_ps( selector1, c_float_0p1 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector2, c_float_1p0 );

				// c[1, 16-31]
				c_float_1p1 = _mm512_add_ps( selector2, c_float_1p1 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector3, c_float_2p0 );

				// c[2, 16-31]
				c_float_2p1 = _mm512_add_ps( selector3, c_float_2p1 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector4, c_float_3p0 );

				// c[3, 16-31]
				c_float_3p1 = _mm512_add_ps( selector4, c_float_3p1 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector5, c_float_4p0 );

				// c[4, 16-31]
				c_float_4p1 = _mm512_add_ps( selector5, c_float_4p1 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector6, c_float_5p0 );

				// c[5, 16-31]
				c_float_5p1 = _mm512_add_ps( selector6, c_float_5p1 );
			}

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_6x32:
		{
			selector1 = _mm512_setzero_ps();

			// c[0,0-15]
			c_float_0p0 = _mm512_max_ps( selector1, c_float_0p0 );

			// c[0, 16-31]
			c_float_0p1 = _mm512_max_ps( selector1, c_float_0p1 );

			// c[1,0-15]
			c_float_1p0 = _mm512_max_ps( selector1, c_float_1p0 );

			// c[1,16-31]
			c_float_1p1 = _mm512_max_ps( selector1, c_float_1p1 );

			// c[2,0-15]
			c_float_2p0 = _mm512_max_ps( selector1, c_float_2p0 );

			// c[2,16-31]
			c_float_2p1 = _mm512_max_ps( selector1, c_float_2p1 );

			// c[3,0-15]
			c_float_3p0 = _mm512_max_ps( selector1, c_float_3p0 );

			// c[3,16-31]
			c_float_3p1 = _mm512_max_ps( selector1, c_float_3p1 );

			// c[4,0-15]
			c_float_4p0 = _mm512_max_ps( selector1, c_float_4p0 );

			// c[4,16-31]
			c_float_4p1 = _mm512_max_ps( selector1, c_float_4p1 );

			// c[5,0-15]
			c_float_5p0 = _mm512_max_ps( selector1, c_float_5p0 );

			// c[5,16-31]
			c_float_5p1 = _mm512_max_ps( selector1, c_float_5p1 );

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_SCALE_6x32:
		{
			selector1 = _mm512_setzero_ps();
			selector2 =
				_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args2 ) );

			__mmask16 relu_cmp_mask;

			// c[0, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_0p0)

			// c[0, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_0p1)

			// c[1, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_1p0)

			// c[1, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_1p1)

			// c[2, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_2p0)

			// c[2, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_2p1)

			// c[3, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_3p0)

			// c[3, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_3p1)

			// c[4, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_4p0)

			// c[4, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_4p1)

			// c[5, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_5p0)

			// c[5, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_5p1)

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_DOWNSCALE_6x32:
	{
		// c[0, 0-15]
		CVT_F32_BF16(c_float_0p0,0,0);

		// c[0, 16-31]
		CVT_F32_BF16(c_float_0p1,0,1);

		// c[1, 0-15]
		CVT_F32_BF16(c_float_1p0,1,0);

		// c[1, 16-31]
		CVT_F32_BF16(c_float_1p1,1,1);

		// c[2, 0-15]
		CVT_F32_BF16(c_float_2p0,2,0);

		// c[2, 16-31]
		CVT_F32_BF16(c_float_2p1,2,1);

		// c[3, 0-15]
		CVT_F32_BF16(c_float_3p0,3,0);

		// c[3, 16-31]
		CVT_F32_BF16(c_float_3p1,3,1);

		// c[4, 0-15]
		CVT_F32_BF16(c_float_4p0,4,0);

		// c[4, 16-31]
		CVT_F32_BF16(c_float_4p1,4,1);

		// c[5, 0-15]
		CVT_F32_BF16(c_float_5p0,5,0);

		// c[5, 16-31]
		CVT_F32_BF16(c_float_5p1,5,1);

		POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
	}		
POST_OPS_6x32_DISABLE:
		;
		
		// Store the results.
		// c[0,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 0*16 ), c_float_0p0 );

		// c[0, 16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 1*16 ), c_float_0p1 );

		// c[1,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 0*16 ), c_float_1p0 );

		// c[1,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 1*16 ), c_float_1p1 );

		// c[2,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 0*16 ), c_float_2p0 );

		// c[2,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 1*16 ), c_float_2p1 );

		// c[3,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 0*16 ), c_float_3p0 );

		// c[3,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 1*16 ), c_float_3p1 );

		// c[4,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 0*16 ), c_float_4p0 );

		// c[4,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 1*16 ), c_float_4p1 );

		// c[5,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 0*16 ), c_float_5p0 );

		// c[5,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 1*16 ), c_float_5p1 );

		a = a + ( MR * ps_a );
		post_op_c_i += MR;
	}
    
	if ( m_partial_pieces > 0 )
	{
		if ( m_partial_pieces == 5 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 5 );
			lpgemm_rowvar_bf16bf16f32of32_5x32
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 4 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 4 );
			lpgemm_rowvar_bf16bf16f32of32_4x32
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 3 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 3 );
			lpgemm_rowvar_bf16bf16f32of32_3x32
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 2 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 2 );
			lpgemm_rowvar_bf16bf16f32of32_2x32
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 1 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 1 );
			lpgemm_rowvar_bf16bf16f32of32_1x32
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
	}	
}

// 6x48 bf16 fringe kernel
LPGEMM_N_FRINGE_KERN(bfloat16, bfloat16, float, bf16bf16f32of32_6x48)
{
	static void* post_ops_labels[] =
						{
						  &&POST_OPS_6x48_DISABLE,
						  &&POST_OPS_BIAS_6x48,
						  &&POST_OPS_RELU_6x48,
						  &&POST_OPS_RELU_SCALE_6x48,
						  &&POST_OPS_DOWNSCALE_6x48
						};
	dim_t MR = 6;
	dim_t m_full_pieces = m0 / MR;
	dim_t m_full_pieces_loop_limit = m_full_pieces * MR;
	dim_t m_partial_pieces = m0 % MR;

	dim_t k_full_pieces = k0 / 2;
	dim_t k_partial_pieces = k0 % 2;

	int32_t a_kfringe_buf = 0;

	// B matrix storage bfloat type
	__m512bh b0;
	__m512bh b1;
	__m512bh b2;

	// A matrix storage bfloat type
	__m512bh a_bf16_0;
    
	for ( dim_t ir = 0; ir < m_full_pieces_loop_limit; ir += MR )
	{
		// Registers to use for accumulating C.
		__m512 c_float_0p0 = _mm512_setzero_ps();
		__m512 c_float_0p1 = _mm512_setzero_ps();
		__m512 c_float_0p2 = _mm512_setzero_ps();

		__m512 c_float_1p0 = _mm512_setzero_ps();
		__m512 c_float_1p1 = _mm512_setzero_ps();
		__m512 c_float_1p2 = _mm512_setzero_ps();

		__m512 c_float_2p0 = _mm512_setzero_ps();
		__m512 c_float_2p1 = _mm512_setzero_ps();
		__m512 c_float_2p2 = _mm512_setzero_ps();
		
		__m512 c_float_3p0 = _mm512_setzero_ps();
		__m512 c_float_3p1 = _mm512_setzero_ps();
		__m512 c_float_3p2 = _mm512_setzero_ps();

		__m512 c_float_4p0 = _mm512_setzero_ps();
		__m512 c_float_4p1 = _mm512_setzero_ps();
		__m512 c_float_4p2 = _mm512_setzero_ps();

		__m512 c_float_5p0 = _mm512_setzero_ps();
		__m512 c_float_5p1 = _mm512_setzero_ps();
		__m512 c_float_5p2 = _mm512_setzero_ps();

		for ( dim_t kr = 0; kr < k_full_pieces; kr += 1 )
		{
			
			// Load 2 rows with 48 elements each from B to 3 ZMM registers. It
			// is to be noted that the B matrix is packed for use in bf16
			// instructions and each load to ZMM register will have 2 elements
			// along k direction and 16 elements across n directions, so 2x16
			// elements to a ZMM register.
			b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * kr ) + ( cs_b * 0 ) );
			b1 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * kr ) + ( cs_b * 1 ) );
			b2 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * kr ) + ( cs_b * 2 ) );

			// Broadcast a[0,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 0 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-47] = a[0,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			c_float_0p1 = _mm512_dpbf16_ps( c_float_0p1, a_bf16_0, b1 );
			c_float_0p2 = _mm512_dpbf16_ps( c_float_0p2, a_bf16_0, b2 );
			
			// Broadcast a[1,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 1 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-47] = a[1,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			c_float_1p1 = _mm512_dpbf16_ps( c_float_1p1, a_bf16_0, b1 );
			c_float_1p2 = _mm512_dpbf16_ps( c_float_1p2, a_bf16_0, b2 );
			
			// Broadcast a[2,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 2 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-47] = a[2,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			c_float_2p1 = _mm512_dpbf16_ps( c_float_2p1, a_bf16_0, b1 );
			c_float_2p2 = _mm512_dpbf16_ps( c_float_2p2, a_bf16_0, b2 );
			
			// Broadcast a[3,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 3 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-47] = a[3,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			c_float_3p1 = _mm512_dpbf16_ps( c_float_3p1, a_bf16_0, b1 );
			c_float_3p2 = _mm512_dpbf16_ps( c_float_3p2, a_bf16_0, b2 );
			
			// Broadcast a[4,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 4 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-47] = a[4,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			c_float_4p1 = _mm512_dpbf16_ps( c_float_4p1, a_bf16_0, b1 );
			c_float_4p2 = _mm512_dpbf16_ps( c_float_4p2, a_bf16_0, b2 );
			
			// Broadcast a[5,kr:kr+2].
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( *( int32_t* )( a + ( rs_a * 5 ) + ( cs_a * kr ) ) );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-47] = a[5,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
			c_float_5p1 = _mm512_dpbf16_ps( c_float_5p1, a_bf16_0, b1 );
			c_float_5p2 = _mm512_dpbf16_ps( c_float_5p2, a_bf16_0, b2 );

		}
		// Handle k remainder.		
		if ( k_partial_pieces > 0 )
		{
			b0 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * k_full_pieces ) + ( cs_b * 0 ) );
			b1 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * k_full_pieces ) + ( cs_b * 1 ) );
			b2 = (__m512bh)_mm512_loadu_epi16( b + ( rs_b * k_full_pieces ) + ( cs_b * 2 ) );

			// Broadcast a[0,kr:kr+4].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 0 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[0,0-47] = a[0,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_0p0 = _mm512_dpbf16_ps( c_float_0p0, a_bf16_0, b0 );
			c_float_0p1 = _mm512_dpbf16_ps( c_float_0p1, a_bf16_0, b1 );
			c_float_0p2 = _mm512_dpbf16_ps( c_float_0p2, a_bf16_0, b2 );
			
			// Broadcast a[1,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 1 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[1,0-47] = a[1,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_1p0 = _mm512_dpbf16_ps( c_float_1p0, a_bf16_0, b0 );
			c_float_1p1 = _mm512_dpbf16_ps( c_float_1p1, a_bf16_0, b1 );
			c_float_1p2 = _mm512_dpbf16_ps( c_float_1p2, a_bf16_0, b2 );
			
			// Broadcast a[2,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 2 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[2,0-47] = a[2,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_2p0 = _mm512_dpbf16_ps( c_float_2p0, a_bf16_0, b0 );
			c_float_2p1 = _mm512_dpbf16_ps( c_float_2p1, a_bf16_0, b1 );
			c_float_2p2 = _mm512_dpbf16_ps( c_float_2p2, a_bf16_0, b2 );
			
			// Broadcast a[3,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 3 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[3,0-47] = a[3,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_3p0 = _mm512_dpbf16_ps( c_float_3p0, a_bf16_0, b0 );
			c_float_3p1 = _mm512_dpbf16_ps( c_float_3p1, a_bf16_0, b1 );
			c_float_3p2 = _mm512_dpbf16_ps( c_float_3p2, a_bf16_0, b2 );
			
			// Broadcast a[4,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 4 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[4,0-47] = a[4,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_4p0 = _mm512_dpbf16_ps( c_float_4p0, a_bf16_0, b0 );
			c_float_4p1 = _mm512_dpbf16_ps( c_float_4p1, a_bf16_0, b1 );
			c_float_4p2 = _mm512_dpbf16_ps( c_float_4p2, a_bf16_0, b2 );
			
			// Broadcast a[5,kr:kr+2].
			memcpy
			(
			  &a_kfringe_buf,
			  ( a + ( rs_a * 5 ) + ( cs_a * k_full_pieces ) ),
			  ( k_partial_pieces * sizeof( bfloat16 ) )
			);
			a_bf16_0 = (__m512bh)_mm512_set1_epi32( a_kfringe_buf );

			// Perform column direction mat-mul with k = 2.
			// c[5,0-47] = a[5,kr:kr+2]*b[kr:kr+2,0-47]
			c_float_5p0 = _mm512_dpbf16_ps( c_float_5p0, a_bf16_0, b0 );
			c_float_5p1 = _mm512_dpbf16_ps( c_float_5p1, a_bf16_0, b1 );
			c_float_5p2 = _mm512_dpbf16_ps( c_float_5p2, a_bf16_0, b2 );
		}
        
		// Load alpha and beta
		__m512 selector1 = _mm512_set1_ps( alpha );
		__m512 selector2 = _mm512_set1_ps( beta );

		// Scale by alpha
		c_float_0p0 = _mm512_mul_ps( selector1, c_float_0p0 );
		c_float_0p1 = _mm512_mul_ps( selector1, c_float_0p1 );
		c_float_0p2 = _mm512_mul_ps( selector1, c_float_0p2 );

		c_float_1p0 = _mm512_mul_ps( selector1, c_float_1p0 );
		c_float_1p1 = _mm512_mul_ps( selector1, c_float_1p1 );
		c_float_1p2 = _mm512_mul_ps( selector1, c_float_1p2 );
		
		c_float_2p0 = _mm512_mul_ps( selector1, c_float_2p0 );
		c_float_2p1 = _mm512_mul_ps( selector1, c_float_2p1 );
		c_float_2p2 = _mm512_mul_ps( selector1, c_float_2p2 );
		
		c_float_3p0 = _mm512_mul_ps( selector1, c_float_3p0 );
		c_float_3p1 = _mm512_mul_ps( selector1, c_float_3p1 );
		c_float_3p2 = _mm512_mul_ps( selector1, c_float_3p2 );
		
		c_float_4p0 = _mm512_mul_ps( selector1, c_float_4p0 );
		c_float_4p1 = _mm512_mul_ps( selector1, c_float_4p1 );
		c_float_4p2 = _mm512_mul_ps( selector1, c_float_4p2 );
		
		c_float_5p0 = _mm512_mul_ps( selector1, c_float_5p0 );
		c_float_5p1 = _mm512_mul_ps( selector1, c_float_5p1 );
		c_float_5p2 = _mm512_mul_ps( selector1, c_float_5p2 );

		// Scale C by beta.
		if ( beta != 0 )
		{
			// c[0,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

			// c[0, 16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_0p1 = _mm512_add_ps( selector1, c_float_0p1 );

			// c[0,32-47]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 2*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_0p2 = _mm512_add_ps( selector1, c_float_0p2 );

			// c[1,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

			// c[1,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_1p1 = _mm512_add_ps( selector1, c_float_1p1 );

			// c[1,32-47]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 2*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_1p2 = _mm512_add_ps( selector1, c_float_1p2 );

			// c[2,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

			// c[2,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_2p1 = _mm512_add_ps( selector1, c_float_2p1 );

			// c[2,32-47]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 2*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_2p2 = _mm512_add_ps( selector1, c_float_2p2 );

			// c[3,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

			// c[3,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_3p1 = _mm512_add_ps( selector1, c_float_3p1 );

			// c[3,32-47]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 2*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_3p2 = _mm512_add_ps( selector1, c_float_3p2 );

			// c[4,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

			// c[4,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_4p1 = _mm512_add_ps( selector1, c_float_4p1 );

			// c[4,32-47]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 2*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_4p2 = _mm512_add_ps( selector1, c_float_4p2 );

			// c[5,0-15]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 0*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );

			// c[5,16-31]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 1*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_5p1 = _mm512_add_ps( selector1, c_float_5p1 );

			// c[5,32-47]
			selector1 = _mm512_loadu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 2*16 ) );
			selector1 = _mm512_mul_ps( selector2, selector1 );
			c_float_5p2 = _mm512_add_ps( selector1, c_float_5p2 );
		}
		// Post Ops
		lpgemm_post_op* post_ops_list_temp = post_ops_list;
		POST_OP_LABEL_LASTK_SAFE_JUMP
POST_OPS_BIAS_6x48:
		{
			__m512 selector3;

			if ( ( *( char* )post_ops_list_temp->op_args2 == 'r' ) ||
				 ( *( char* )post_ops_list_temp->op_args2 == 'R' ) )
			{
				selector1 =
					_mm512_loadu_ps( ( float* )post_ops_list_temp->op_args1 +
								post_op_c_j + ( 0 * 16 ) );
				selector2 =
					_mm512_loadu_ps( ( float* )post_ops_list_temp->op_args1 +
								post_op_c_j + ( 1 * 16 ) );
				selector3 =
					_mm512_loadu_ps( ( float* )post_ops_list_temp->op_args1 +
								post_op_c_j + ( 2 * 16 ) );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[0, 16-31]
				c_float_0p1 = _mm512_add_ps( selector2, c_float_0p1 );

				// c[0,32-47]
				c_float_0p2 = _mm512_add_ps( selector3, c_float_0p2 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector1, c_float_1p0 );

				// c[1, 16-31]
				c_float_1p1 = _mm512_add_ps( selector2, c_float_1p1 );

				// c[1,32-47]
				c_float_1p2 = _mm512_add_ps( selector3, c_float_1p2 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector1, c_float_2p0 );

				// c[2, 16-31]
				c_float_2p1 = _mm512_add_ps( selector2, c_float_2p1 );

				// c[2,32-47]
				c_float_2p2 = _mm512_add_ps( selector3, c_float_2p2 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector1, c_float_3p0 );

				// c[3, 16-31]
				c_float_3p1 = _mm512_add_ps( selector2, c_float_3p1 );

				// c[3,32-47]
				c_float_3p2 = _mm512_add_ps( selector3, c_float_3p2 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector1, c_float_4p0 );

				// c[4, 16-31]
				c_float_4p1 = _mm512_add_ps( selector2, c_float_4p1 );

				// c[4,32-47]
				c_float_4p2 = _mm512_add_ps( selector3, c_float_4p2 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector1, c_float_5p0 );

				// c[5, 16-31]
				c_float_5p1 = _mm512_add_ps( selector2, c_float_5p1 );

				// c[5,32-47]
				c_float_5p2 = _mm512_add_ps( selector3, c_float_5p2 );
			}
			else
			{
				selector1 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 0 ) );
				selector2 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 1 ) );
				selector3 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 2 ) );
				__m512 selector4 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 3 ) );
				__m512 selector5 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 4 ) );
				__m512 selector6 =
					_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args1
								+ post_op_c_i + 5 ) );

				// c[0,0-15]
				c_float_0p0 = _mm512_add_ps( selector1, c_float_0p0 );

				// c[0, 16-31]
				c_float_0p1 = _mm512_add_ps( selector1, c_float_0p1 );

				// c[0,32-47]
				c_float_0p2 = _mm512_add_ps( selector1, c_float_0p2 );

				// c[1,0-15]
				c_float_1p0 = _mm512_add_ps( selector2, c_float_1p0 );

				// c[1, 16-31]
				c_float_1p1 = _mm512_add_ps( selector2, c_float_1p1 );

				// c[1,32-47]
				c_float_1p2 = _mm512_add_ps( selector2, c_float_1p2 );

				// c[2,0-15]
				c_float_2p0 = _mm512_add_ps( selector3, c_float_2p0 );

				// c[2, 16-31]
				c_float_2p1 = _mm512_add_ps( selector3, c_float_2p1 );

				// c[2,32-47]
				c_float_2p2 = _mm512_add_ps( selector3, c_float_2p2 );

				// c[3,0-15]
				c_float_3p0 = _mm512_add_ps( selector4, c_float_3p0 );

				// c[3, 16-31]
				c_float_3p1 = _mm512_add_ps( selector4, c_float_3p1 );

				// c[3,32-47]
				c_float_3p2 = _mm512_add_ps( selector4, c_float_3p2 );

				// c[4,0-15]
				c_float_4p0 = _mm512_add_ps( selector5, c_float_4p0 );

				// c[4, 16-31]
				c_float_4p1 = _mm512_add_ps( selector5, c_float_4p1 );

				// c[4,32-47]
				c_float_4p2 = _mm512_add_ps( selector5, c_float_4p2 );

				// c[5,0-15]
				c_float_5p0 = _mm512_add_ps( selector6, c_float_5p0 );

				// c[5, 16-31]
				c_float_5p1 = _mm512_add_ps( selector6, c_float_5p1 );

				// c[5,32-47]
				c_float_5p2 = _mm512_add_ps( selector6, c_float_5p2 );
			}

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_6x48:
		{
			//printf("relu\n");
			selector1 = _mm512_setzero_ps();

			// c[0,0-15]
			c_float_0p0 = _mm512_max_ps( selector1, c_float_0p0 );

			// c[0, 16-31]
			c_float_0p1 = _mm512_max_ps( selector1, c_float_0p1 );

			// c[0,32-47]
			c_float_0p2 = _mm512_max_ps( selector1, c_float_0p2 );

			// c[1,0-15]
			c_float_1p0 = _mm512_max_ps( selector1, c_float_1p0 );

			// c[1,16-31]
			c_float_1p1 = _mm512_max_ps( selector1, c_float_1p1 );

			// c[1,32-47]
			c_float_1p2 = _mm512_max_ps( selector1, c_float_1p2 );

			// c[2,0-15]
			c_float_2p0 = _mm512_max_ps( selector1, c_float_2p0 );

			// c[2,16-31]
			c_float_2p1 = _mm512_max_ps( selector1, c_float_2p1 );

			// c[2,32-47]
			c_float_2p2 = _mm512_max_ps( selector1, c_float_2p2 );

			// c[3,0-15]
			c_float_3p0 = _mm512_max_ps( selector1, c_float_3p0 );

			// c[3,16-31]
			c_float_3p1 = _mm512_max_ps( selector1, c_float_3p1 );

			// c[3,32-47]
			c_float_3p2 = _mm512_max_ps( selector1, c_float_3p2 );

			// c[4,0-15]
			c_float_4p0 = _mm512_max_ps( selector1, c_float_4p0 );

			// c[4,16-31]
			c_float_4p1 = _mm512_max_ps( selector1, c_float_4p1 );

			// c[4,32-47]
			c_float_4p2 = _mm512_max_ps( selector1, c_float_4p2 );

			// c[5,0-15]
			c_float_5p0 = _mm512_max_ps( selector1, c_float_5p0 );

			// c[5,16-31]
			c_float_5p1 = _mm512_max_ps( selector1, c_float_5p1 );

			// c[5,32-47]
			c_float_5p2 = _mm512_max_ps( selector1, c_float_5p2 );

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_RELU_SCALE_6x48:
		{
			selector1 = _mm512_setzero_ps();
			selector2 =
				_mm512_set1_ps( *( ( float* )post_ops_list_temp->op_args2 ) );

			__mmask16 relu_cmp_mask;

			// c[0, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_0p0)

			// c[0, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_0p1)

			// c[0, 32-47]
			RELU_SCALE_OP_F32_AVX512(c_float_0p2)

			// c[1, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_1p0)

			// c[1, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_1p1)

			// c[1, 32-47]
			RELU_SCALE_OP_F32_AVX512(c_float_1p2)

			// c[2, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_2p0)

			// c[2, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_2p1)

			// c[2, 32-47]
			RELU_SCALE_OP_F32_AVX512(c_float_2p2)

			// c[3, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_3p0)

			// c[3, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_3p1)

			// c[3, 32-47]
			RELU_SCALE_OP_F32_AVX512(c_float_3p2)

			// c[4, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_4p0)

			// c[4, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_4p1)

			// c[4, 32-47]
			RELU_SCALE_OP_F32_AVX512(c_float_4p2)

			// c[5, 0-15]
			RELU_SCALE_OP_F32_AVX512(c_float_5p0)

			// c[5, 16-31]
			RELU_SCALE_OP_F32_AVX512(c_float_5p1)

			// c[5, 32-47]
			RELU_SCALE_OP_F32_AVX512(c_float_5p2)

			POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
		}
POST_OPS_DOWNSCALE_6x48:
	{
		// c[0, 0-15]
		CVT_F32_BF16(c_float_0p0,0,0);

		// c[0, 16-31]
		CVT_F32_BF16(c_float_0p1,0,1);

		// c[0, 32-47]
		CVT_F32_BF16(c_float_0p2,0,2);

		// c[1, 0-15]
		CVT_F32_BF16(c_float_1p0,1,0);

		// c[1, 16-31]
		CVT_F32_BF16(c_float_1p1,1,1);

		// c[1, 32-47]
		CVT_F32_BF16(c_float_1p2,1,2);

		// c[2, 0-15]
		CVT_F32_BF16(c_float_2p0,2,0);

		// c[2, 16-31]
		CVT_F32_BF16(c_float_2p1,2,1);

		// c[2, 32-47]
		CVT_F32_BF16(c_float_2p2,2,2);

		// c[3, 0-15]
		CVT_F32_BF16(c_float_3p0,3,0);

		// c[3, 16-31]
		CVT_F32_BF16(c_float_3p1,3,1);

		// c[3, 32-47]
		CVT_F32_BF16(c_float_3p2,3,2);

		// c[4, 0-15]
		CVT_F32_BF16(c_float_4p0,4,0);

		// c[4, 16-31]
		CVT_F32_BF16(c_float_4p1,4,1);

		// c[4, 32-47]
		CVT_F32_BF16(c_float_4p2,4,2);

		// c[5, 0-15]
		CVT_F32_BF16(c_float_5p0,5,0);

		// c[5, 16-31]
		CVT_F32_BF16(c_float_5p1,5,1);

		// c[5, 32-47]
		CVT_F32_BF16(c_float_5p2,5,2);

		POST_OP_LABEL_LASTK_SAFE_JUMP_WITH_NEXT_PTR
	}		
POST_OPS_6x48_DISABLE:
		;
		
		// Store the results.
		// c[0,0-15]	
		_mm512_storeu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 0*16 ), c_float_0p0 );

		// c[0, 16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 1*16 ), c_float_0p1 );
       
		// c[0,32-47]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 0 ) ) + ( 2*16 ), c_float_0p2 );
        
		// c[1,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 0*16 ), c_float_1p0 );
        
		// c[1,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 1*16 ), c_float_1p1 );

		// c[1,32-47]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 1 ) ) + ( 2*16 ), c_float_1p2 );

		// c[2,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 0*16 ), c_float_2p0 );

		// c[2,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 1*16 ), c_float_2p1 );

		// c[2,32-47]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 2 ) ) + ( 2*16 ), c_float_2p2 );

		// c[3,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 0*16 ), c_float_3p0 );

		// c[3,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 1*16 ), c_float_3p1 );

		// c[3,32-47]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 3 ) ) + ( 2*16 ), c_float_3p2 );

		// c[4,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 0*16 ), c_float_4p0 );

		// c[4,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 1*16 ), c_float_4p1 );

		// c[4,32-47]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 4 ) ) + ( 2*16 ), c_float_4p2 );

		// c[5,0-15]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 0*16 ), c_float_5p0 );

		// c[5,16-31]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 1*16 ), c_float_5p1 );

		// c[5,32-47]
		_mm512_storeu_ps( c + ( rs_c * ( ir + 5 ) ) + ( 2*16 ), c_float_5p2 );

		a = a + ( MR * ps_a );
		post_op_c_i += MR;
		
	}
    
	if ( m_partial_pieces > 0 )
	{
		if ( m_partial_pieces == 5 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 5 );
			lpgemm_rowvar_bf16bf16f32of32_5x48
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 4 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 4 );
			lpgemm_rowvar_bf16bf16f32of32_4x48
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 3 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 3 );
			lpgemm_rowvar_bf16bf16f32of32_3x48
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 2 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 2 );
			lpgemm_rowvar_bf16bf16f32of32_2x48
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
		else if ( m_partial_pieces == 1 )
		{
			int cs_a_use = ( cs_a == 2 ) ? 2 : ( ( cs_a / 6 ) * 1 );
			lpgemm_rowvar_bf16bf16f32of32_1x48
			(
			  k0,
			  a, rs_a, cs_a_use,
			  b, rs_b, cs_b,
			  ( c + ( rs_c * m_full_pieces_loop_limit ) ), rs_c,
			  alpha, beta,
			  is_last_k,
			  post_op_c_i, post_op_c_j,
			  post_ops_list, rs_c_downscale
			);
		}		
	}	
}
#endif
