/* BROKEN DISCLAIMER */

/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef HAVE_R5XX_3DREGS_H
#define HAVE_R5XX_3DREGS_H 1

/* PLL register i cannot find any documentation on */
#define R500_DYN_SCLK_PWMEM_PIPE                        0x000d /* PLL */

#define R300_DST_PIPE_CONFIG		                0x170c
#       define R300_PIPE_AUTO_CONFIG                    (1 << 31)

#define RADEON_WAIT_UNTIL                   0x1720
#       define RADEON_WAIT_CRTC_PFLIP       (1 << 0)
#       define RADEON_WAIT_RE_CRTC_VLINE    (1 << 1)
#       define RADEON_WAIT_FE_CRTC_VLINE    (1 << 2)
#       define RADEON_WAIT_CRTC_VLINE       (1 << 3)
#       define RADEON_WAIT_DMA_VID_IDLE     (1 << 8)
#       define RADEON_WAIT_DMA_GUI_IDLE     (1 << 9)
#       define RADEON_WAIT_CMDFIFO          (1 << 10) /* wait for CMDFIFO_ENTRIES */
#       define RADEON_WAIT_OV0_FLIP         (1 << 11)
#       define RADEON_WAIT_AGP_FLUSH        (1 << 13)
#       define RADEON_WAIT_2D_IDLE          (1 << 14)
#       define RADEON_WAIT_3D_IDLE          (1 << 15)
#       define RADEON_WAIT_2D_IDLECLEAN     (1 << 16)
#       define RADEON_WAIT_3D_IDLECLEAN     (1 << 17)
#       define RADEON_WAIT_HOST_IDLECLEAN   (1 << 18)
#       define RADEON_CMDFIFO_ENTRIES_SHIFT 10
#       define RADEON_CMDFIFO_ENTRIES_MASK  0x7f
#       define RADEON_WAIT_VAP_IDLE         (1 << 28)
#       define RADEON_WAIT_BOTH_CRTC_PFLIP  (1 << 30)
#       define RADEON_ENG_DISPLAY_SELECT_CRTC0    (0 << 31)
#       define RADEON_ENG_DISPLAY_SELECT_CRTC1    (1 << 31)

#define R300_VAP_CNTL				        0x2080
#       define R300_PVS_NUM_SLOTS_SHIFT                 0
#       define R300_PVS_NUM_CNTLRS_SHIFT                4
#       define R300_PVS_NUM_FPUS_SHIFT                  8
#       define R300_VF_MAX_VTX_NUM_SHIFT                18
#       define R300_GL_CLIP_SPACE_DEF                   (0 << 22)
#       define R300_DX_CLIP_SPACE_DEF                   (1 << 22)
#       define R500_TCL_STATE_OPTIMIZATION              (1 << 23)

#define R500_VAP_INDEX_OFFSET			        0x208c

#define R300_VAP_VTE_CNTL				0x20B0
#       define R300_VPORT_X_SCALE_ENA                   (1 << 0)
#       define R300_VPORT_X_OFFSET_ENA                  (1 << 1)
#       define R300_VPORT_Y_SCALE_ENA                   (1 << 2)
#       define R300_VPORT_Y_OFFSET_ENA                  (1 << 3)
#       define R300_VPORT_Z_SCALE_ENA                   (1 << 4)
#       define R300_VPORT_Z_OFFSET_ENA                  (1 << 5)
#       define R300_VTX_XY_FMT                          (1 << 8)
#       define R300_VTX_Z_FMT                           (1 << 9)
#       define R300_VTX_W0_FMT                          (1 << 10)

#define R300_VAP_CNTL_STATUS				0x2140
#       define R300_PVS_BYPASS                          (1 << 8)

#define R300_VAP_VTX_STATE_CNTL		                0x2180

#define R300_VAP_PSC_SGN_NORM_CNTL		        0x21DC

#define R300_VAP_PROG_STREAM_CNTL_EXT_0	                0x21e0
#       define R300_SWIZZLE_SELECT_X_0_SHIFT            0
#       define R300_SWIZZLE_SELECT_Y_0_SHIFT            3
#       define R300_SWIZZLE_SELECT_Z_0_SHIFT            6
#       define R300_SWIZZLE_SELECT_W_0_SHIFT            9
#       define R300_SWIZZLE_SELECT_X                    0
#       define R300_SWIZZLE_SELECT_Y                    1
#       define R300_SWIZZLE_SELECT_Z                    2
#       define R300_SWIZZLE_SELECT_W                    3
#       define R300_SWIZZLE_SELECT_FP_ZERO              4
#       define R300_SWIZZLE_SELECT_FP_ONE               5
#       define R300_WRITE_ENA_0_SHIFT                   12
#       define R300_WRITE_ENA_X                         1
#       define R300_WRITE_ENA_Y                         2
#       define R300_WRITE_ENA_Z                         4
#       define R300_WRITE_ENA_W                         8
#       define R300_SWIZZLE_SELECT_X_1_SHIFT            16
#       define R300_SWIZZLE_SELECT_Y_1_SHIFT            19
#       define R300_SWIZZLE_SELECT_Z_1_SHIFT            22
#       define R300_SWIZZLE_SELECT_W_1_SHIFT            25
#       define R300_WRITE_ENA_1_SHIFT                   28
#define R300_VAP_PROG_STREAM_CNTL_EXT_1	                0x21e4
#       define R300_SWIZZLE_SELECT_X_2_SHIFT            0
#       define R300_SWIZZLE_SELECT_Y_2_SHIFT            3
#       define R300_SWIZZLE_SELECT_Z_2_SHIFT            6
#       define R300_SWIZZLE_SELECT_W_2_SHIFT            9
#       define R300_WRITE_ENA_2_SHIFT                   12
#       define R300_SWIZZLE_SELECT_X_3_SHIFT            16
#       define R300_SWIZZLE_SELECT_Y_3_SHIFT            19
#       define R300_SWIZZLE_SELECT_Z_3_SHIFT            22
#       define R300_SWIZZLE_SELECT_W_3_SHIFT            25
#       define R300_WRITE_ENA_3_SHIFT                   28

#define R300_VAP_PVS_VECTOR_INDX_REG		        0x2200
#define R300_VAP_PVS_VECTOR_DATA_REG		        0x2204

/* PVS instructions */
/* Opcode and dst instruction */
#define R300_PVS_DST_OPCODE(x)                          (x << 0)
/* Vector ops */
#       define R300_VECTOR_NO_OP                        0
#       define R300_VE_DOT_PRODUCT                      1
#       define R300_VE_MULTIPLY                         2
#       define R300_VE_ADD                              3
#       define R300_VE_MULTIPLY_ADD                     4
#       define R300_VE_DISTANCE_VECTOR                  5
#       define R300_VE_FRACTION                         6
#       define R300_VE_MAXIMUM                          7
#       define R300_VE_MINIMUM                          8
#       define R300_VE_SET_GREATER_THAN_EQUAL           9
#       define R300_VE_SET_LESS_THAN                    10
#       define R300_VE_MULTIPLYX2_ADD                   11
#       define R300_VE_MULTIPLY_CLAMP                   12
#       define R300_VE_FLT2FIX_DX                       13
#       define R300_VE_FLT2FIX_DX_RND                   14
/* R500 additions */
#       define R500_VE_PRED_SET_EQ_PUSH                 15
#       define R500_VE_PRED_SET_GT_PUSH                 16
#       define R500_VE_PRED_SET_GTE_PUSH                17
#       define R500_VE_PRED_SET_NEQ_PUSH                18
#       define R500_VE_COND_WRITE_EQ                    19
#       define R500_VE_COND_WRITE_GT                    20
#       define R500_VE_COND_WRITE_GTE                   21
#       define R500_VE_COND_WRITE_NEQ                   22
#       define R500_VE_COND_MUX_EQ                      23
#       define R500_VE_COND_MUX_GT                      24
#       define R500_VE_COND_MUX_GTE                     25
#       define R500_VE_SET_GREATER_THAN                 26
#       define R500_VE_SET_EQUAL                        27
#       define R500_VE_SET_NOT_EQUAL                    28
/* Math ops */
#       define R300_MATH_NO_OP                          0
#       define R300_ME_EXP_BASE2_DX                     1
#       define R300_ME_LOG_BASE2_DX                     2
#       define R300_ME_EXP_BASEE_FF                     3
#       define R300_ME_LIGHT_COEFF_DX                   4
#       define R300_ME_POWER_FUNC_FF                    5
#       define R300_ME_RECIP_DX                         6
#       define R300_ME_RECIP_FF                         7
#       define R300_ME_RECIP_SQRT_DX                    8
#       define R300_ME_RECIP_SQRT_FF                    9
#       define R300_ME_MULTIPLY                         10
#       define R300_ME_EXP_BASE2_FULL_DX                11
#       define R300_ME_LOG_BASE2_FULL_DX                12
#       define R300_ME_POWER_FUNC_FF_CLAMP_B            13
#       define R300_ME_POWER_FUNC_FF_CLAMP_B1           14
#       define R300_ME_POWER_FUNC_FF_CLAMP_01           15
#       define R300_ME_SIN                              16
#       define R300_ME_COS                              17
/* R500 additions */
#       define R500_ME_LOG_BASE2_IEEE                   18
#       define R500_ME_RECIP_IEEE                       19
#       define R500_ME_RECIP_SQRT_IEEE                  20
#       define R500_ME_PRED_SET_EQ                      21
#       define R500_ME_PRED_SET_GT                      22
#       define R500_ME_PRED_SET_GTE                     23
#       define R500_ME_PRED_SET_NEQ                     24
#       define R500_ME_PRED_SET_CLR                     25
#       define R500_ME_PRED_SET_INV                     26
#       define R500_ME_PRED_SET_POP                     27
#       define R500_ME_PRED_SET_RESTORE                 28
/* macro */
#       define R300_PVS_MACRO_OP_2CLK_MADD              0
#       define R300_PVS_MACRO_OP_2CLK_M2X_ADD           1
#define R300_PVS_DST_MATH_INST                          (1 << 6)
#define R300_PVS_DST_MACRO_INST                         (1 << 7)
#define R300_PVS_DST_REG_TYPE(x)                        (x << 8)
#       define R300_PVS_DST_REG_TEMPORARY               0
#       define R300_PVS_DST_REG_A0                      1
#       define R300_PVS_DST_REG_OUT                     2
#       define R500_PVS_DST_REG_OUT_REPL_X              3
#       define R300_PVS_DST_REG_ALT_TEMPORARY           4
#       define R300_PVS_DST_REG_INPUT                   5
#define R300_PVS_DST_ADDR_MODE_1                        (1 << 12)
#define R300_PVS_DST_OFFSET(x)                          (x << 13)
#define R300_PVS_DST_WE_X                               (1 << 20)
#define R300_PVS_DST_WE_Y                               (1 << 21)
#define R300_PVS_DST_WE_Z                               (1 << 22)
#define R300_PVS_DST_WE_W                               (1 << 23)
#define R300_PVS_DST_VE_SAT                             (1 << 24)
#define R300_PVS_DST_ME_SAT                             (1 << 25)
#define R300_PVS_DST_PRED_ENABLE                        (1 << 26)
#define R300_PVS_DST_PRED_SENSE                         (1 << 27)
#define R300_PVS_DST_DUAL_MATH_OP                       (1 << 28)
#define R300_PVS_DST_ADDR_SEL(x)                        (x << 29)
#define R300_PVS_DST_ADDR_MODE_0                        (1 << 31)
/* src operand instruction */
#define R300_PVS_SRC_REG_TYPE(x)                        (x << 0)
#       define R300_PVS_SRC_REG_TEMPORARY               0
#       define R300_PVS_SRC_REG_INPUT                   1
#       define R300_PVS_SRC_REG_CONSTANT                2
#       define R300_PVS_SRC_REG_ALT_TEMPORARY           3
#define R300_SPARE_0                                    (1 << 2)
#define R300_PVS_SRC_ABS_XYZW                           (1 << 3)
#define R300_PVS_SRC_ADDR_MODE_0                        (1 << 4)
#define R300_PVS_SRC_OFFSET(x)                          (x << 5)
#define R300_PVS_SRC_SWIZZLE_X(x)                       (x << 13)
#define R300_PVS_SRC_SWIZZLE_Y(x)                       (x << 16)
#define R300_PVS_SRC_SWIZZLE_Z(x)                       (x << 19)
#define R300_PVS_SRC_SWIZZLE_W(x)                       (x << 22)
#       define R300_PVS_SRC_SELECT_X                    0
#       define R300_PVS_SRC_SELECT_Y                    1
#       define R300_PVS_SRC_SELECT_Z                    2
#       define R300_PVS_SRC_SELECT_W                    3
#       define R300_PVS_SRC_SELECT_FORCE_0              4
#       define R300_PVS_SRC_SELECT_FORCE_1              5
#define R300_PVS_SRC_NEG_X                              (1 << 25)
#define R300_PVS_SRC_NEG_Y                              (1 << 26)
#define R300_PVS_SRC_NEG_Z                              (1 << 27)
#define R300_PVS_SRC_NEG_W                              (1 << 28)
#define R300_PVS_SRC_ADDR_SEL(x)                        (x << 29)
#define R300_PVS_SRC_ADDR_MODE_1                        (1 << 31)

#define R300_VAP_CLIP_CNTL				0x221c
#       define R300_UCP_ENA_0                           (1 << 0)
#       define R300_UCP_ENA_1                           (1 << 1)
#       define R300_UCP_ENA_2                           (1 << 2)
#       define R300_UCP_ENA_3                           (1 << 3)
#       define R300_UCP_ENA_4                           (1 << 4)
#       define R300_UCP_ENA_5                           (1 << 5)
#       define R300_PS_UCP_MODE_SHIFT                   14
#       define R300_CLIP_DISABLE                        (1 << 16)
#       define R300_UCP_CULL_ONLY_ENA                   (1 << 17)
#       define R300_BOUNDARY_EDGE_FLAG_ENA              (1 << 18)

#define R300_VAP_GB_VERT_CLIP_ADJ		        0x2220
#define R300_VAP_GB_VERT_DISC_ADJ		        0x2224
#define R300_VAP_GB_HORZ_CLIP_ADJ		        0x2228
#define R300_VAP_GB_HORZ_DISC_ADJ		        0x222c

#define R300_VAP_PVS_STATE_FLUSH_REG			0x2284

#define R300_VAP_PVS_FLOW_CNTL_OPC		        0x22DC

#define R300_GB_ENABLE				        0x4008

#define R300_GB_MSPOS0				        0x4010
#       define R300_MS_X0_SHIFT                         0
#       define R300_MS_Y0_SHIFT                         4
#       define R300_MS_X1_SHIFT                         8
#       define R300_MS_Y1_SHIFT                         12
#       define R300_MS_X2_SHIFT                         16
#       define R300_MS_Y2_SHIFT                         20
#       define R300_MSBD0_Y_SHIFT                       24
#       define R300_MSBD0_X_SHIFT                       28
#define R300_GB_MSPOS1				        0x4014
#       define R300_MS_X3_SHIFT                         0
#       define R300_MS_Y3_SHIFT                         4
#       define R300_MS_X4_SHIFT                         8
#       define R300_MS_Y4_SHIFT                         12
#       define R300_MS_X5_SHIFT                         16
#       define R300_MS_Y5_SHIFT                         20
#       define R300_MSBD1_SHIFT                         24

#define R300_GB_TILE_CONFIG				0x4018
#       define R300_ENABLE_TILING                       (1 << 0)
#       define R300_PIPE_COUNT_RV350                    (0 << 1)
#       define R300_PIPE_COUNT_R300                     (3 << 1)
#       define R300_PIPE_COUNT_R420_3P                  (6 << 1)
#       define R300_PIPE_COUNT_R420                     (7 << 1)
#       define R300_TILE_SIZE_8                         (0 << 4)
#       define R300_TILE_SIZE_16                        (1 << 4)
#       define R300_TILE_SIZE_32                        (2 << 4)
#       define R300_SUBPIXEL_1_12                       (0 << 16)
#       define R300_SUBPIXEL_1_16                       (1 << 16)

#define R300_GB_SELECT				        0x401c
#define R300_GB_AA_CONFIG				0x4020

#define R400_GB_PIPE_SELECT                             0x402c

#define R500_RS_IP_0					0x4074
#define R500_RS_IP_1					0x4078
#   define R500_RS_IP_PTR_K0				62
#   define R500_RS_IP_PTR_K1 				63
#   define R500_RS_IP_TEX_PTR_S_SHIFT 			0
#   define R500_RS_IP_TEX_PTR_T_SHIFT 			6
#   define R500_RS_IP_TEX_PTR_R_SHIFT 			12
#   define R500_RS_IP_TEX_PTR_Q_SHIFT 			18
#   define R500_RS_IP_COL_PTR_SHIFT 			24
#   define R500_RS_IP_COL_FMT_SHIFT 			27
#   define R500_RS_IP_COL_FMT_RGBA			(0 << 27)
#   define R500_RS_IP_OFFSET_EN 			(1 << 31)

#define R300_GA_ENHANCE				        0x4274
#       define R300_GA_DEADLOCK_CNTL                    (1 << 0)
#       define R300_GA_FASTSYNC_CNTL                    (1 << 1)

#define R300_GA_COLOR_CONTROL			        0x4278
#       define R300_RGB0_SHADING_SOLID                  (0 << 0)
#       define R300_RGB0_SHADING_FLAT                   (1 << 0)
#       define R300_RGB0_SHADING_GOURAUD                (2 << 0)
#       define R300_ALPHA0_SHADING_SOLID                (0 << 2)
#       define R300_ALPHA0_SHADING_FLAT                 (1 << 2)
#       define R300_ALPHA0_SHADING_GOURAUD              (2 << 2)
#       define R300_RGB1_SHADING_SOLID                  (0 << 4)
#       define R300_RGB1_SHADING_FLAT                   (1 << 4)
#       define R300_RGB1_SHADING_GOURAUD                (2 << 4)
#       define R300_ALPHA1_SHADING_SOLID                (0 << 6)
#       define R300_ALPHA1_SHADING_FLAT                 (1 << 6)
#       define R300_ALPHA1_SHADING_GOURAUD              (2 << 6)
#       define R300_RGB2_SHADING_SOLID                  (0 << 8)
#       define R300_RGB2_SHADING_FLAT                   (1 << 8)
#       define R300_RGB2_SHADING_GOURAUD                (2 << 8)
#       define R300_ALPHA2_SHADING_SOLID                (0 << 10)
#       define R300_ALPHA2_SHADING_FLAT                 (1 << 10)
#       define R300_ALPHA2_SHADING_GOURAUD              (2 << 10)
#       define R300_RGB3_SHADING_SOLID                  (0 << 12)
#       define R300_RGB3_SHADING_FLAT                   (1 << 12)
#       define R300_RGB3_SHADING_GOURAUD                (2 << 12)
#       define R300_ALPHA3_SHADING_SOLID                (0 << 14)
#       define R300_ALPHA3_SHADING_FLAT                 (1 << 14)
#       define R300_ALPHA3_SHADING_GOURAUD              (2 << 14)

#define R300_GA_POLY_MODE				0x4288
#       define R300_FRONT_PTYPE_POINT                   (0 << 4)
#       define R300_FRONT_PTYPE_LINE                    (1 << 4)
#       define R300_FRONT_PTYPE_TRIANGE                 (2 << 4)
#       define R300_BACK_PTYPE_POINT                    (0 << 7)
#       define R300_BACK_PTYPE_LINE                     (1 << 7)
#       define R300_BACK_PTYPE_TRIANGE                  (2 << 7)
#define R300_GA_ROUND_MODE				0x428c
#       define R300_GEOMETRY_ROUND_TRUNC                (0 << 0)
#       define R300_GEOMETRY_ROUND_NEAREST              (1 << 0)
#       define R300_COLOR_ROUND_TRUNC                   (0 << 2)
#       define R300_COLOR_ROUND_NEAREST                 (1 << 2)

#define R300_GA_OFFSET				        0x4290

#define R300_SU_TEX_WRAP				0x42a0
#define R300_SU_POLY_OFFSET_ENABLE		        0x42b4
#define R300_SU_CULL_MODE				0x42b8
#       define R300_CULL_FRONT                          (1 << 0)
#       define R300_CULL_BACK                           (1 << 1)
#       define R300_FACE_POS                            (0 << 2)
#       define R300_FACE_NEG                            (1 << 2)
#define R300_SU_DEPTH_SCALE				0x42c0
#define R300_SU_DEPTH_OFFSET			        0x42c4

#define R500_SU_REG_DEST                                0x42c8

#define R300_RS_COUNT				        0x4300
#	define R300_RS_COUNT_IT_COUNT_SHIFT		0
#	define R300_RS_COUNT_IC_COUNT_SHIFT		7
#	define R300_RS_COUNT_HIRES_EN			(1 << 18)

#define R300_RS_INST_COUNT				0x4304
#	define R300_INST_COUNT_RS(x)		        (x << 0)
#	define R300_RS_W_EN			        (1 << 4)
#	define R300_TX_OFFSET_RS(x)		        (x << 5)

#define R300_RS_IP_0				        0x4310
#define R300_RS_IP_1				        0x4314
#	define R300_RS_TEX_PTR(x)		        (x << 0)
#	define R300_RS_COL_PTR(x)		        (x << 6)
#	define R300_RS_COL_FMT(x)		        (x << 9)
#	define R300_RS_COL_FMT_RGBA		        0
#	define R300_RS_COL_FMT_RGB0		        2
#	define R300_RS_COL_FMT_RGB1		        3
#	define R300_RS_COL_FMT_000A		        4
#	define R300_RS_COL_FMT_0000		        5
#	define R300_RS_COL_FMT_0001		        6
#	define R300_RS_COL_FMT_111A		        8
#	define R300_RS_COL_FMT_1110		        9
#	define R300_RS_COL_FMT_1111		        10
#	define R300_RS_SEL_S(x)		                (x << 13)
#	define R300_RS_SEL_T(x)		                (x << 16)
#	define R300_RS_SEL_R(x)		                (x << 19)
#	define R300_RS_SEL_Q(x)		                (x << 22)
#	define R300_RS_SEL_C0		                0
#	define R300_RS_SEL_C1		                1
#	define R300_RS_SEL_C2		                2
#	define R300_RS_SEL_C3		                3
#	define R300_RS_SEL_K0		                4
#	define R300_RS_SEL_K1		                5

#define R300_RS_INST_0				        0x4330
#define R300_RS_INST_1				        0x4334
#	define R300_INST_TEX_ID(x)		        (x << 0)
#       define R300_RS_INST_TEX_CN_WRITE		(1 << 3)
#	define R300_INST_TEX_ADDR(x)		        (x << 6)

#define R500_RS_INST_0					0x4320
#define R500_RS_INST_1					0x4324
#   define R500_RS_INST_TEX_ID_SHIFT			0
#   define R500_RS_INST_TEX_CN_WRITE			(1 << 4)
#   define R500_RS_INST_TEX_ADDR_SHIFT			5
#   define R500_RS_INST_COL_ID_SHIFT			12
#   define R500_RS_INST_COL_CN_NO_WRITE			(0 << 16)
#   define R500_RS_INST_COL_CN_WRITE			(1 << 16)
#   define R500_RS_INST_COL_CN_WRITE_FBUFFER		(2 << 16)
#   define R500_RS_INST_COL_CN_WRITE_BACKFACE		(3 << 16)
#   define R500_RS_INST_COL_COL_ADDR_SHIFT		18
#   define R500_RS_INST_TEX_ADJ				(1 << 25)
#   define R500_RS_INST_W_CN				(1 << 26)

#define R300_SC_CLIP_0_A				0x43b0
#define R300_SC_CLIP_0_B				0x43b4
#       define R300_CLIP_X_SHIFT                        0
#       define R300_CLIP_Y_SHIFT                        13
#define R300_SC_CLIP_RULE				0x43d0

#define R300_SC_EDGERULE				0x43a8
#define R300_SC_SCISSOR0				0x43e0
#define R300_SC_SCISSOR1				0x43e4
#       define R300_SCISSOR_X_SHIFT                     0
#       define R300_SCISSOR_Y_SHIFT                     13

#define R300_SC_SCREENDOOR				0x43e8

#define R300_US_CONFIG				        0x4600
#       define R300_NLEVEL_SHIFT                        0
#       define R300_FIRST_TEX                           (1 << 3)
#       define R500_ZERO_TIMES_ANYTHING_EQUALS_ZERO     (1 << 1)
#define R300_US_PIXSIZE				        0x4604
#define R300_US_CODE_OFFSET				0x4608
#       define R300_ALU_CODE_OFFSET(x)                  (x << 0)
#       define R300_ALU_CODE_SIZE(x)                    (x << 6)
#       define R300_TEX_CODE_OFFSET(x)                  (x << 13)
#       define R300_TEX_CODE_SIZE(x)                    (x << 18)
#define R300_US_CODE_ADDR_0				0x4610
#       define R300_ALU_START(x)                        (x << 0)
#       define R300_ALU_SIZE(x)                         (x << 6)
#       define R300_TEX_START(x)                        (x << 12)
#       define R300_TEX_SIZE(x)                         (x << 17)
#       define R300_RGBA_OUT                            (1 << 22)
#       define R300_W_OUT                               (1 << 23)
#define R300_US_CODE_ADDR_1				0x4614
#define R300_US_CODE_ADDR_2				0x4618
#define R300_US_CODE_ADDR_3				0x461c
#define R300_US_TEX_INST_0				0x4620
#define R300_US_TEX_INST_1				0x4624
#define R300_US_TEX_INST_2				0x4628
#       define R300_TEX_SRC_ADDR(x)                     (x << 0)
#       define R300_TEX_DST_ADDR(x)                     (x << 6)
#       define R300_TEX_ID(x)                           (x << 11)
#       define R300_TEX_INST(x)                         (x << 15)
#       define R300_TEX_INST_NOP                        0
#       define R300_TEX_INST_LD                         1
#       define R300_TEX_INST_TEXKILL                    2
#       define R300_TEX_INST_PROJ                       3
#       define R300_TEX_INST_LODBIAS                    4

#define R500_US_FC_CTRL					0x4624
#define R500_US_CODE_ADDR				0x4630
#define R500_US_CODE_RANGE 				0x4634
#define R500_US_CODE_OFFSET 				0x4638

#define R300_US_OUT_FMT_0				0x46a4
#define R300_US_OUT_FMT_1				0x46a8
#define R300_US_OUT_FMT_2				0x46ac
#define R300_US_OUT_FMT_3				0x46b0
#       define R300_OUT_FMT_C4_8                        (0 << 0)
#       define R300_OUT_FMT_C4_10                       (1 << 0)
#       define R300_OUT_FMT_C4_10_GAMMA                 (2 << 0)
#       define R300_OUT_FMT_C_16                        (3 << 0)
#       define R300_OUT_FMT_C2_16                       (4 << 0)
#       define R300_OUT_FMT_C4_16                       (5 << 0)
#       define R300_OUT_FMT_C_16_MPEG                   (6 << 0)
#       define R300_OUT_FMT_C2_16_MPEG                  (7 << 0)
#       define R300_OUT_FMT_C2_4                        (8 << 0)
#       define R300_OUT_FMT_C_3_3_2                     (9 << 0)
#       define R300_OUT_FMT_C_5_6_5                     (10 << 0)
#       define R300_OUT_FMT_C_11_11_10                  (11 << 0)
#       define R300_OUT_FMT_C_10_11_11                  (12 << 0)
#       define R300_OUT_FMT_C_2_10_10_10                (13 << 0)
#       define R300_OUT_FMT_UNUSED                      (15 << 0)
#       define R300_OUT_FMT_C_16_FP                     (16 << 0)
#       define R300_OUT_FMT_C2_16_FP                    (17 << 0)
#       define R300_OUT_FMT_C4_16_FP                    (18 << 0)
#       define R300_OUT_FMT_C_32_FP                     (19 << 0)
#       define R300_OUT_FMT_C2_32_FP                    (20 << 0)
#       define R300_OUT_FMT_C4_32_FP                    (21 << 0)
#       define R300_OUT_FMT_C0_SEL_ALPHA                (0 << 8)
#       define R300_OUT_FMT_C0_SEL_RED                  (1 << 8)
#       define R300_OUT_FMT_C0_SEL_GREEN                (2 << 8)
#       define R300_OUT_FMT_C0_SEL_BLUE                 (3 << 8)
#       define R300_OUT_FMT_C1_SEL_ALPHA                (0 << 10)
#       define R300_OUT_FMT_C1_SEL_RED                  (1 << 10)
#       define R300_OUT_FMT_C1_SEL_GREEN                (2 << 10)
#       define R300_OUT_FMT_C1_SEL_BLUE                 (3 << 10)
#       define R300_OUT_FMT_C2_SEL_ALPHA                (0 << 12)
#       define R300_OUT_FMT_C2_SEL_RED                  (1 << 12)
#       define R300_OUT_FMT_C2_SEL_GREEN                (2 << 12)
#       define R300_OUT_FMT_C2_SEL_BLUE                 (3 << 12)
#       define R300_OUT_FMT_C3_SEL_ALPHA                (0 << 14)
#       define R300_OUT_FMT_C3_SEL_RED                  (1 << 14)
#       define R300_OUT_FMT_C3_SEL_GREEN                (2 << 14)
#       define R300_OUT_FMT_C3_SEL_BLUE                 (3 << 14)
#define R300_US_W_FMT				        0x46b4

#define R300_FG_FOG_BLEND				0x4bc0
#define R300_FG_ALPHA_FUNC				0x4bd4
#define R300_FG_DEPTH_SRC				0x4bd8

#define R300_RB3D_CCTL				        0x4e00
#define R300_RB3D_BLENDCNTL				0x4e04
#       define R300_ALPHA_BLEND_ENABLE                  (1 << 0)
#       define R300_SEPARATE_ALPHA_ENABLE               (1 << 1)
#       define R300_READ_ENABLE                         (1 << 2)
#define R300_RB3D_ABLENDCNTL			        0x4e08
#define R300_RB3D_COLOR_CHANNEL_MASK	                0x4e0c
#       define R300_BLUE_MASK_EN                        (1 << 0)
#       define R300_GREEN_MASK_EN                       (1 << 1)
#       define R300_RED_MASK_EN                         (1 << 2)
#       define R300_ALPHA_MASK_EN                       (1 << 3)

#define R300_RB3D_COLOR_CLEAR_VALUE                     0x4e14
#define R300_RB3D_ROPCNTL				0x4e18

#define R300_RB3D_COLOROFFSET0			        0x4e28
#define R300_RB3D_COLORPITCH0			        0x4e38
#       define R300_COLORTILE                           (1 << 16)
#       define R300_COLORENDIAN_WORD                    (1 << 19)
#       define R300_COLORENDIAN_DWORD                   (2 << 19)
#       define R300_COLORENDIAN_HALF_DWORD              (3 << 19)
#       define R300_COLORFORMAT_ARGB1555                (3 << 21)
#       define R300_COLORFORMAT_RGB565                  (4 << 21)
#       define R300_COLORFORMAT_ARGB8888                (6 << 21)
#       define R300_COLORFORMAT_ARGB32323232            (7 << 21)
#       define R300_COLORFORMAT_I8                      (9 << 21)
#       define R300_COLORFORMAT_ARGB16161616            (10 << 21)
#       define R300_COLORFORMAT_VYUY                    (11 << 21)
#       define R300_COLORFORMAT_YVYU                    (12 << 21)
#       define R300_COLORFORMAT_UV88                    (13 << 21)
#       define R300_COLORFORMAT_ARGB4444                (15 << 21)

#define R300_RB3D_DSTCACHE_CTLSTAT		        0x4e4c
#       define R300_DC_FLUSH_3D                         (2 << 0)
#       define R300_DC_FREE_3D                          (2 << 2)
#       define R300_RB3D_DC_FLUSH_ALL                   (R300_DC_FLUSH_3D | R300_DC_FREE_3D)
#       define R300_DC_FINISH_3D                        (1 << 4)

#define R300_RB3D_DITHER_CTL			        0x4e50

#define R300_RB3D_AARESOLVE_CTL			        0x4e88

#define R300_RB3D_ZCNTL				        0x4f00
#define R300_RB3D_ZSTENCILCNTL			        0x4f04

#define R300_RB3D_ZTOP				        0x4f14
#define R300_RB3D_ZCACHE_CTLSTAT			0x4f18
#       define R300_ZC_FLUSH                            (1 << 0)
#       define R300_ZC_FREE                             (1 << 1)
#       define R300_ZC_FLUSH_ALL                        0x3
#define R300_RB3D_BW_CNTL				0x4f1c

#endif /* HAVE_R5XX_3DREGS_H */
