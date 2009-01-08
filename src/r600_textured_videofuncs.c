/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Alex Deucher <alexander.deucher@amd.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

#include "exa.h"

#include "rhd.h"
#include "rhd_cs.h"
#include "r6xx_accel.h"
#include "r600_shader.h"
#include "r600_reg.h"
#include "r600_state.h"

#include "rhd_video.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

# ifdef DAMAGE
#  include "damage.h"
# endif

/* seriously ?! @#$%% */
# define uint32_t CARD32
# define uint64_t CARD64

void
R600DisplayTexturedVideo(ScrnInfoPtr pScrn, struct RHDPortPriv *pPriv)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    PixmapPtr pPixmap = pPriv->pPixmap;
    BoxPtr pBox = REGION_RECTS(&pPriv->clip);
    int nBox = REGION_NUM_RECTS(&pPriv->clip);
    int dstxoff, dstyoff;
    int i = 0;
    uint32_t vs[16];
    uint32_t ps[48];
    cb_config_t     cb_conf;
    tex_resource_t  tex_res;
    tex_sampler_t   tex_samp;
    shader_config_t vs_conf, ps_conf;
    uint64_t vs_addr, ps_addr;
    int src_pitch, dst_pitch;
    draw_config_t   draw_conf;
    vtx_resource_t  vtx_res;
    uint64_t vb_addr;

    static float ps_alu_consts[] = {
	1.0,  0.0,      1.13983,  -1.13983/2, // r - c[0]
	1.0, -0.39465, -0.5806,  (0.39465+0.5806)/2, // g - c[1]
	1.0,  2.03211,  0.0,     -2.03211/2, // b - c[2]
    };

    //0
    vs[i++] = CF_DWORD0(ADDR(4));
    vs[i++] = CF_DWORD1(POP_COUNT(0),
			CF_CONST(0),
			COND(SQ_CF_COND_ACTIVE),
			I_COUNT(2),
			CALL_COUNT(0),
			END_OF_PROGRAM(0),
			VALID_PIXEL_MODE(0),
			CF_INST(SQ_CF_INST_VTX),
			WHOLE_QUAD_MODE(0),
			BARRIER(1));
    //1
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_POS0),
				      TYPE(SQ_EXPORT_POS),
				      RW_GPR(1),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(0));
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(0),
					   END_OF_PROGRAM(0),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(1));
    //2
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(0),
				      TYPE(SQ_EXPORT_PARAM),
				      RW_GPR(0),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(0));
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(0),
					   END_OF_PROGRAM(1),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(0));
    //3
    vs[i++] = 0x00000000;
    vs[i++] = 0x00000000;
    //4/5
    vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			 FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			 FETCH_WHOLE_QUAD(0),
			 BUFFER_ID(0),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 SRC_SEL_X(SQ_SEL_X),
			 MEGA_FETCH_COUNT(16));
    vs[i++] = VTX_DWORD1_GPR(DST_GPR(1),
			     DST_REL(0),
			     DST_SEL_X(SQ_SEL_X),
			     DST_SEL_Y(SQ_SEL_Y),
			     DST_SEL_Z(SQ_SEL_0),
			     DST_SEL_W(SQ_SEL_1),
			     USE_CONST_FIELDS(0),
			     DATA_FORMAT(FMT_32_32_FLOAT), //xxx
			     NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
			     FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
			     SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
    vs[i++] = VTX_DWORD2(OFFSET(0),
			 ENDIAN_SWAP(ENDIAN_NONE),
			 CONST_BUF_NO_STRIDE(0),
			 MEGA_FETCH(1));
    vs[i++] = VTX_DWORD_PAD;
    //6/7
    vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			 FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			 FETCH_WHOLE_QUAD(0),
			 BUFFER_ID(0),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 SRC_SEL_X(SQ_SEL_X),
			 MEGA_FETCH_COUNT(8));
    vs[i++] = VTX_DWORD1_GPR(DST_GPR(0),
			     DST_REL(0),
			     DST_SEL_X(SQ_SEL_X),
			     DST_SEL_Y(SQ_SEL_Y),
			     DST_SEL_Z(SQ_SEL_0),
			     DST_SEL_W(SQ_SEL_1),
			     USE_CONST_FIELDS(0),
			     DATA_FORMAT(FMT_32_32_FLOAT), //xxx
			     NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
			     FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
			     SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
    vs[i++] = VTX_DWORD2(OFFSET(8),
			 ENDIAN_SWAP(ENDIAN_NONE),
			 CONST_BUF_NO_STRIDE(0),
			 MEGA_FETCH(0));
    vs[i++] = VTX_DWORD_PAD;

    i = 0;

    // 0
    ps[i++] = CF_DWORD0(ADDR(20));
    ps[i++] = CF_DWORD1(POP_COUNT(0),
			CF_CONST(0),
			COND(SQ_CF_COND_ACTIVE),
			I_COUNT(2),
			CALL_COUNT(0),
			END_OF_PROGRAM(0),
			VALID_PIXEL_MODE(0),
			CF_INST(SQ_CF_INST_TEX),
			WHOLE_QUAD_MODE(0),
			BARRIER(0));
    // 1
    ps[i++] = CF_ALU_DWORD0(ADDR(3),
			    KCACHE_BANK0(0),
			    KCACHE_BANK1(0),
			    KCACHE_MODE0(SQ_CF_KCACHE_NOP));
    ps[i++] = CF_ALU_DWORD1(KCACHE_MODE1(SQ_CF_KCACHE_NOP),
			    KCACHE_ADDR0(0),
			    KCACHE_ADDR1(0),
			    I_COUNT(16),
			    USES_WATERFALL(0),
			    CF_INST(SQ_CF_INST_ALU),
			    WHOLE_QUAD_MODE(0),
			    BARRIER(1));
    // 2
    ps[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_PIXEL_MRT0),
				      TYPE(SQ_EXPORT_PIXEL),
				      RW_GPR(3),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(3));
    ps[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(0),
					   END_OF_PROGRAM(1),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(1));
    // 3 - alu 0
    // DP4 gpr[2].x gpr[1].x c[0].x
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_X),
			 SRC0_NEG(0),
			 SRC1_SEL(256),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_X),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(1),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_X),
			     CLAMP(0));
    // 4 - alu 1
    // DP4 gpr[2].y gpr[1].y c[0].y
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Y),
			 SRC0_NEG(0),
			 SRC1_SEL(256),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_Y),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Y),
			     CLAMP(0));
    // 5 - alu 2
    // DP4 gpr[2].z gpr[1].z c[0].z
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Z),
			 SRC0_NEG(0),
			 SRC1_SEL(256),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_Z),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Z),
			     CLAMP(0));
    // 6 - alu 3
    // DP4 gpr[2].w gpr[1].w c[0].w
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_W),
			 SRC0_NEG(0),
			 SRC1_SEL(256),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_W),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(1));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_021),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_W),
			     CLAMP(0));
    // 7 - alu 4
    // DP4 gpr[2].x gpr[1].x c[1].x
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_X),
			 SRC0_NEG(0),
			 SRC1_SEL(257),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_X),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_X),
			     CLAMP(0));
    // 8 - alu 5
    // DP4 gpr[2].y gpr[1].y c[1].y
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Y),
			 SRC0_NEG(0),
			 SRC1_SEL(257),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_Y),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(1),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Y),
			     CLAMP(0));
    // 9 - alu 6
    // DP4 gpr[2].z gpr[1].z c[1].z
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Z),
			 SRC0_NEG(0),
			 SRC1_SEL(257),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_Z),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Z),
			     CLAMP(0));
    // 10 - alu 7
    // DP4 gpr[2].w gpr[1].w c[1].w
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_W),
			 SRC0_NEG(0),
			 SRC1_SEL(257),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_W),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(1));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_021),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_W),
			     CLAMP(0));
    // 11 - alu 8
    // DP4 gpr[2].x gpr[1].x c[2].x
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_X),
			 SRC0_NEG(0),
			 SRC1_SEL(258),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_X),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_X),
			     CLAMP(0));
    // 12 - alu 9
    // DP4 gpr[2].y gpr[1].y c[2].y
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Y),
			 SRC0_NEG(0),
			 SRC1_SEL(258),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_Y),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Y),
			     CLAMP(0));
    // 13 - alu 10
    // DP4 gpr[2].z gpr[1].z c[2].z
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Z),
			 SRC0_NEG(0),
			 SRC1_SEL(258),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_Z),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(1),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_102),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Z),
			     CLAMP(0));
    // 14 - alu 11
    // DP4 gpr[2].w gpr[1].w c[2].w
    ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_W),
			 SRC0_NEG(0),
			 SRC1_SEL(258),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_W),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(1));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(0),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_DOT4),
			     BANK_SWIZZLE(SQ_ALU_VEC_021),
			     DST_GPR(2),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_W),
			     CLAMP(0));
    // 15 - alu 12
    // MOV gpr[3].x gpr[2].x
    ps[i++] = ALU_DWORD0(SRC0_SEL(2),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_X),
			 SRC0_NEG(0),
			 SRC1_SEL(0),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_X),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(1),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_MOV),
			     BANK_SWIZZLE(SQ_ALU_VEC_210),
			     DST_GPR(3),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_X),
			     CLAMP(0));
    // 16 - alu 13
    // MOV gpr[3].y gpr[2].y
    ps[i++] = ALU_DWORD0(SRC0_SEL(2),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Y),
			 SRC0_NEG(0),
			 SRC1_SEL(0),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_X),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(1),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_MOV),
			     BANK_SWIZZLE(SQ_ALU_VEC_210),
			     DST_GPR(3),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Y),
			     CLAMP(0));
    // 17 - alu 14
    // MOV gpr[3].z gpr[2].z
    ps[i++] = ALU_DWORD0(SRC0_SEL(2),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_Z),
			 SRC0_NEG(0),
			 SRC1_SEL(0),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_X),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(0));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(1),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_MOV),
			     BANK_SWIZZLE(SQ_ALU_VEC_210),
			     DST_GPR(3),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_Z),
			     CLAMP(0));
    // 18 - alu 15
    // MOV gpr[3].w gpr[2].w
    ps[i++] = ALU_DWORD0(SRC0_SEL(2),
			 SRC0_REL(ABSOLUTE),
			 SRC0_ELEM(ELEM_W),
			 SRC0_NEG(0),
			 SRC1_SEL(0),
			 SRC1_REL(ABSOLUTE),
			 SRC1_ELEM(ELEM_X),
			 SRC1_NEG(0),
			 INDEX_MODE(SQ_INDEX_LOOP),
			 PRED_SEL(SQ_PRED_SEL_OFF),
			 LAST(1));
    ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
			     SRC0_ABS(0),
			     SRC1_ABS(0),
			     UPDATE_EXECUTE_MASK(0),
			     UPDATE_PRED(0),
			     WRITE_MASK(1),
			     FOG_MERGE(0),
			     OMOD(SQ_ALU_OMOD_OFF),
			     ALU_INST(SQ_OP2_INST_MOV),
			     BANK_SWIZZLE(SQ_ALU_VEC_012),
			     DST_GPR(3),
			     DST_REL(ABSOLUTE),
			     DST_ELEM(ELEM_W),
			     CLAMP(0));
    // 19 - alignment
    ps[i++] = 0x00000000;
    ps[i++] = 0x00000000;
    // 20/21 - tex 0
    ps[i++] = TEX_DWORD0(TEX_INST(SQ_TEX_INST_SAMPLE),
			 BC_FRAC_MODE(0),
			 FETCH_WHOLE_QUAD(0),
			 RESOURCE_ID(0),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 R7xx_ALT_CONST(0));
    ps[i++] = TEX_DWORD1(DST_GPR(1),
			 DST_REL(ABSOLUTE),
			 DST_SEL_X(SQ_SEL_X),
			 DST_SEL_Y(SQ_SEL_MASK),
			 DST_SEL_Z(SQ_SEL_MASK),
			 DST_SEL_W(SQ_SEL_1),
			 LOD_BIAS(0),
			 COORD_TYPE_X(TEX_NORMALIZED),
			 COORD_TYPE_Y(TEX_NORMALIZED),
			 COORD_TYPE_Z(TEX_NORMALIZED),
			 COORD_TYPE_W(TEX_NORMALIZED));
    ps[i++] = TEX_DWORD2(OFFSET_X(0),
			 OFFSET_Y(0),
			 OFFSET_Z(0),
			 SAMPLER_ID(0),
			 SRC_SEL_X(SQ_SEL_X),
			 SRC_SEL_Y(SQ_SEL_Y),
			 SRC_SEL_Z(SQ_SEL_0),
			 SRC_SEL_W(SQ_SEL_1));
    ps[i++] = TEX_DWORD_PAD;
    // 22/23 - tex 1
    ps[i++] = TEX_DWORD0(TEX_INST(SQ_TEX_INST_SAMPLE),
			 BC_FRAC_MODE(0),
			 FETCH_WHOLE_QUAD(0),
			 RESOURCE_ID(1),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 R7xx_ALT_CONST(0));
    ps[i++] = TEX_DWORD1(DST_GPR(1),
			 DST_REL(ABSOLUTE),
			 DST_SEL_X(SQ_SEL_MASK),
			 DST_SEL_Y(SQ_SEL_X),
			 DST_SEL_Z(SQ_SEL_Y),
			 DST_SEL_W(SQ_SEL_MASK),
			 LOD_BIAS(0),
			 COORD_TYPE_X(TEX_NORMALIZED),
			 COORD_TYPE_Y(TEX_NORMALIZED),
			 COORD_TYPE_Z(TEX_NORMALIZED),
			 COORD_TYPE_W(TEX_NORMALIZED));
    ps[i++] = TEX_DWORD2(OFFSET_X(0),
			 OFFSET_Y(0),
			 OFFSET_Z(0),
			 SAMPLER_ID(1),
			 SRC_SEL_X(SQ_SEL_X),
			 SRC_SEL_Y(SQ_SEL_Y),
			 SRC_SEL_Z(SQ_SEL_0),
			 SRC_SEL_W(SQ_SEL_1));
    ps[i++] = TEX_DWORD_PAD;

    CLEAR (cb_conf);
    CLEAR (tex_res);
    CLEAR (tex_samp);
    CLEAR (vs_conf);
    CLEAR (ps_conf);
    CLEAR (draw_conf);
    CLEAR (vtx_res);

    dst_pitch = exaGetPixmapPitch(pPixmap) / (pPixmap->drawable.bitsPerPixel / 8);
    src_pitch = pPriv->BufferPitch;

    // bad pitch
    if (src_pitch & 63)
	return;
    if (dst_pitch & 63)
	return;

#ifdef COMPOSITE
    dstxoff = -pPixmap->screen_x + pPixmap->drawable.x;
    dstyoff = -pPixmap->screen_y + pPixmap->drawable.y;
#else
    dstxoff = 0;
    dstyoff = 0;
#endif

    accel_state->ib = RHDDRMCPBuffer(pScrn->scrnIndex);

    /* Init */
    start_3d(pScrn, accel_state->ib);

    cp_set_surface_sync(pScrn, accel_state->ib);

    set_default_state(pScrn, accel_state->ib);

    /* Scissor / viewport */
    ereg  (accel_state->ib, PA_CL_VTE_CNTL,                      VTX_XY_FMT_bit);
    ereg  (accel_state->ib, PA_CL_CLIP_CNTL,                     CLIP_DISABLE_bit);

    //vs_addr = upload (pScrn, vs, sizeof(vs), 0);
    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 512, vs, sizeof(vs));
    vs_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 512;
    //ps_addr = upload (pScrn, ps, sizeof(ps), 4096);
    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 256, ps, sizeof(ps));
    ps_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 256;

    /* Shader */
    vs_conf.shader_addr         = vs_addr;
    vs_conf.num_gprs            = 2;
    vs_conf.stack_size          = 0;
    vs_setup                    (pScrn, accel_state->ib, &vs_conf);

    ps_conf.shader_addr         = ps_addr;
    ps_conf.num_gprs            = 4;
    ps_conf.stack_size          = 0;
    ps_conf.uncached_first_inst = 1;
    ps_conf.clamp_consts        = 0;
    ps_conf.export_mode         = 2;
    ps_setup                    (pScrn, accel_state->ib, &ps_conf);

    // PS alu constants
    set_alu_consts(pScrn, accel_state->ib, 0, sizeof(ps_alu_consts) / SQ_ALU_CONSTANT_offset, ps_alu_consts);

    /* Texture */
    // Y texture
    tex_res.id                  = 0;
    tex_res.w                   = pPriv->w;
    tex_res.h                   = pPriv->h >> 1;
    tex_res.pitch               = src_pitch * 2;
    tex_res.depth               = 0;
    tex_res.dim                 = SQ_TEX_DIM_2D;
    tex_res.base                = pPriv->BufferOffset + rhdPtr->FbIntAddress;
    tex_res.mip_base            = pPriv->BufferOffset + rhdPtr->FbIntAddress;

    tex_res.format              = FMT_8;
    tex_res.dst_sel_x           = SQ_SEL_X;
    tex_res.dst_sel_y           = SQ_SEL_0;
    tex_res.dst_sel_z           = SQ_SEL_0;
    tex_res.dst_sel_w           = SQ_SEL_0;

    tex_res.request_size        = 1;
    tex_res.base_level          = 0;
    tex_res.last_level          = 0;
    tex_res.perf_modulation     = 0;
    tex_res.interlaced          = 0;
    set_tex_resource            (pScrn, accel_state->ib, &tex_res);

    // UV texture
    tex_res.id                  = 1;
    tex_res.format              = FMT_8_8;
    tex_res.w                   = pPriv->w >> 1;
    tex_res.h                   = pPriv->h >> 2;
    tex_res.pitch               = src_pitch;
    tex_res.dst_sel_x           = SQ_SEL_X;
    tex_res.dst_sel_y           = SQ_SEL_Y;
    tex_res.dst_sel_z           = SQ_SEL_0;
    tex_res.dst_sel_w           = SQ_SEL_0;
    tex_res.interlaced          = 0;
    // XXX tex bases need to be 256B aligned
    tex_res.base                = pPriv->BufferOffset + rhdPtr->FbIntAddress + (src_pitch * pPriv->h);
    tex_res.mip_base            = pPriv->BufferOffset + rhdPtr->FbIntAddress + (src_pitch * pPriv->h);
    set_tex_resource            (pScrn, accel_state->ib, &tex_res);

    // Y sampler
    tex_samp.id                 = 0;
    tex_samp.clamp_x            = SQ_TEX_CLAMP_LAST_TEXEL;
    tex_samp.clamp_y            = SQ_TEX_CLAMP_LAST_TEXEL;
    tex_samp.clamp_z            = SQ_TEX_WRAP;

    // xxx: switch to bicubic
    tex_samp.xy_mag_filter      = SQ_TEX_XY_FILTER_BILINEAR;
    tex_samp.xy_min_filter      = SQ_TEX_XY_FILTER_BILINEAR;

    tex_samp.z_filter           = SQ_TEX_Z_FILTER_NONE;
    tex_samp.mip_filter         = 0;			/* no mipmap */
    set_tex_sampler             (pScrn, accel_state->ib, &tex_samp);

    // UV sampler
    tex_samp.id                 = 1;
    set_tex_sampler             (pScrn, accel_state->ib, &tex_samp);

    /* Render setup */
    ereg  (accel_state->ib, CB_SHADER_MASK,                      (0x0f << OUTPUT0_ENABLE_shift));
    ereg  (accel_state->ib, R7xx_CB_SHADER_CONTROL,              (RT0_ENABLE_bit));
    ereg  (accel_state->ib, CB_COLOR_CONTROL,                    (0xcc << ROP3_shift)); /* copy */

    cb_conf.id = 0;

    cb_conf.w = dst_pitch;
    cb_conf.h = pPixmap->drawable.height;
    cb_conf.base = exaGetPixmapOffset(pPixmap) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    switch (pPixmap->drawable.bitsPerPixel) {
    case 16:
	if (pPixmap->drawable.depth == 15)
	    cb_conf.format = COLOR_1_5_5_5;
	else
	    cb_conf.format = COLOR_5_6_5;
	break;
    case 32:
	cb_conf.format = COLOR_8_8_8_8;
	break;
    default:
	return;
    }

    cb_conf.comp_swap = 1;
    cb_conf.source_format = 1;
    cb_conf.blend_clamp = 1;
    set_render_target(pScrn, accel_state->ib, &cb_conf);

    ereg  (accel_state->ib, PA_SU_SC_MODE_CNTL,                  (FACE_bit			|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_FRONT_PTYPE_shift)	|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_BACK_PTYPE_shift)));
    ereg  (accel_state->ib, DB_SHADER_CONTROL,                   ((1 << Z_ORDER_shift)		| /* EARLY_Z_THEN_LATE_Z */
						 DUAL_EXPORT_ENABLE_bit)); /* Only useful if no depth export */

    /* Interpolator setup */
    // export tex coords from VS
    ereg  (accel_state->ib, SPI_VS_OUT_CONFIG, ((1 - 1) << VS_EXPORT_COUNT_shift));
    ereg  (accel_state->ib, SPI_VS_OUT_ID_0, (0 << SEMANTIC_0_shift));

    /* Enabling flat shading needs both FLAT_SHADE_bit in SPI_PS_INPUT_CNTL_x
     * *and* FLAT_SHADE_ENA_bit in SPI_INTERP_CONTROL_0 */
    ereg  (accel_state->ib, SPI_PS_IN_CONTROL_0,                 ((1 << NUM_INTERP_shift)));
    ereg  (accel_state->ib, SPI_PS_IN_CONTROL_1,                 0);
    ereg  (accel_state->ib, SPI_PS_INPUT_CNTL_0 + (0 <<2),       ((0    << SEMANTIC_shift)	|
									     (0x03 << DEFAULT_VAL_shift)	|
									     SEL_CENTROID_bit));
    ereg  (accel_state->ib, SPI_INTERP_CONTROL_0,                0);


    accel_state->vb_index = 0;

    while (nBox--) {
	int srcX, srcY, srcw, srch;
	int dstX, dstY, dstw, dsth;
	struct r6xx_copy_vertex *xv_vb = (pointer)(char*)accel_state->ib->address + (accel_state->ib->total / 2);
	struct r6xx_copy_vertex vertex[3];

	dstX = pBox->x1 + dstxoff;
	dstY = pBox->y1 + dstyoff;
	dstw = pBox->x2 - pBox->x1;
	dsth = pBox->y2 - pBox->y1;

	srcX = ((pBox->x1 - pPriv->drw_x) *
		pPriv->src_w) / pPriv->dst_w;
	srcY = ((pBox->y1 - pPriv->drw_y) *
		pPriv->src_h) / pPriv->dst_h;

	srcw = (pPriv->src_w * dstw) / pPriv->dst_w;
	srch = (pPriv->src_h * dsth) / pPriv->dst_h;

	vertex[0].x = (float)dstX;
	vertex[0].y = (float)dstY;
	vertex[0].s = (float)srcX / pPriv->w;
	vertex[0].t = (float)srcY / pPriv->h;

	vertex[1].x = (float)dstX;
	vertex[1].y = (float)(dstY + dsth);
	vertex[1].s = (float)srcX / pPriv->w;
	vertex[1].t = (float)(srcY + srch) / pPriv->h;

	vertex[2].x = (float)(dstX + dstw);
	vertex[2].y = (float)(dstY + dsth);
	vertex[2].s = (float)(srcX + srcw) / pPriv->w;
	vertex[2].t = (float)(srcY + srch) / pPriv->h;

	ErrorF("vertex 0: %f, %f, %f, %f\n", vertex[0].x, vertex[0].y, vertex[0].s, vertex[0].t);
	ErrorF("vertex 1: %f, %f, %f, %f\n", vertex[1].x, vertex[1].y, vertex[1].s, vertex[1].t);
	ErrorF("vertex 2: %f, %f, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].s, vertex[2].t);

	// append to vertex buffer
	xv_vb[accel_state->vb_index++] = vertex[0];
	xv_vb[accel_state->vb_index++] = vertex[1];
	xv_vb[accel_state->vb_index++] = vertex[2];

	pBox++;
    }

    if (accel_state->vb_index == 0) {
	R600CPFlushIndirect(pScrn, accel_state->ib);
	DamageDamageRegion(pPriv->pDraw, &pPriv->clip);
	return;
    }

    vb_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2);

    /* Vertex buffer setup */
    vtx_res.id              = SQ_VTX_RESOURCE_vs;
    vtx_res.vtx_size_dw     = 16 / 4;
    vtx_res.vtx_num_entries = accel_state->vb_index * 16 / 4;
    vtx_res.mem_req_size    = 1;
    vtx_res.vb_addr         = vb_addr;
    set_vtx_resource        (pScrn, accel_state->ib, &vtx_res);

    draw_conf.prim_type          = DI_PT_RECTLIST;
    draw_conf.vgt_draw_initiator = DI_SRC_SEL_AUTO_INDEX;
    draw_conf.num_instances      = 1;
    draw_conf.num_indices        = vtx_res.vtx_num_entries / vtx_res.vtx_size_dw;
    draw_conf.index_type         = DI_INDEX_SIZE_16_BIT;

    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_0,            0);	/* ? */
    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_1,            0);

    ereg  (accel_state->ib, VGT_MAX_VTX_INDX,                    draw_conf.num_indices);
    ereg  (accel_state->ib, VGT_MIN_VTX_INDX,                    0);
    ereg  (accel_state->ib, VGT_INDX_OFFSET,                     0);

    draw_auto(pScrn, accel_state->ib, &draw_conf);

    wait_3d_idle_clean(pScrn, accel_state->ib);

    R600CPFlushIndirect(pScrn, accel_state->ib);

    DamageDamageRegion(pPriv->pDraw, &pPriv->clip);
}
