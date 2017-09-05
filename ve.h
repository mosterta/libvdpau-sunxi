/*
 * Copyright (c) 2013 Jens Kuske <jenskuske@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __CEDARV_H__
#define __CEDARV_H__

#include <stdint.h>
#include <string.h>

int cedarv_open(void);
void cedarv_close(void);
int cedarv_get_version(void);
int cedarv_wait(int timeout);
void *cedarv_get(int engine, uint32_t flags);
void cedarv_put(void);
void* cedarv_get_regs();

#if USE_UMP
  #include <ump/ump.h>
  #include <ump/ump_ref_drv.h>

  typedef struct _CEDARV_MEMORY {
      ump_handle mem_id;
  }CEDARV_MEMORY;
#else
  typedef void* CEDARV_MEMORY;
  
#endif

CEDARV_MEMORY cedarv_malloc(int size);
int cedarv_isValid(CEDARV_MEMORY mem);
void cedarv_free(CEDARV_MEMORY mem);
uint32_t cedarv_virt2phys(CEDARV_MEMORY mem);
void cedarv_flush_cache(CEDARV_MEMORY mem, int len);
void cedarv_memcpy(CEDARV_MEMORY dst, size_t offset, const void * src, size_t len);
void cedarv_memset(CEDARV_MEMORY dst, unsigned char value, size_t len);
void* cedarv_getPointer(CEDARV_MEMORY mem);
size_t cedarv_getSize(CEDARV_MEMORY mem);
unsigned char cedarv_byteAccess(CEDARV_MEMORY mem, size_t offset);
void cedarv_setBufferInvalid(CEDARV_MEMORY mem);
int cedarv_allocateEngine(int engine);
int cedarv_freeEngine();
int cedarv_VeReset();

static inline void writel(uint32_t val, void *addr)
{
	*((volatile uint32_t *)addr) = val;
}

static inline uint32_t readl(void *addr)
{
	return *((volatile uint32_t *) addr);
}

static inline void writeb(uint8_t val, void *addr)
{
   *((volatile uint8_t *)addr) = val;
}


#define CEDARV_ENGINE_MPEG			0x0
#define CEDARV_ENGINE_H264			0x1
#define CEDARV_ENGINE_HEVC			0x4

#define CEDARV_CTRL				0x000
#define CEDARV_RESET				0x004
#define CEDARV_TIMEOUT				0x00c
#define CEDARV_IPD_DBLK_BUF_CTRL    		0x050
#define CEDARV_IPD_BUF              		0x054
#define CEDARV_DBLK_BUF             		0x05c
#define CEDARV_OUTPUT_CHROMA_OFFSET             0x0c4
#define CEDARV_OUTPUT_STRIDE                    0x0c8
#define CEDARV_EXTRA_OUT_STRIDE			0x0cc
#define CEDARV_EXTRA_OUT_FMT_OFFSET             0x0e8
#define CEDARV_OUTPUT_FORMAT                    0x0ec
#define CEDARV_VERSION                          0x0f0


#define CEDARV_MPEG_PIC_HDR			0x100
#define CEDARV_MPEG_VOP_HDR         0x104
#define CEDARV_MPEG_SIZE			0x108
#define CEDARV_MPEG_FRAME_SIZE		0x10c
#define CEDARV_MPEG_MBA             0x110
#define CEDARV_MPEG_CTRL			0x114
#define CEDARV_MPEG_TRIGGER			0x118
#define CEDARV_MPEG_STATUS			0x11c
#define CEDARV_MPEG_TRBTRD_FIELD	0x120
#define CEDARV_MPEG_TRBTRD_FRAME	0x124
#define CEDARV_MPEG_VLD_ADDR		0x128
#define CEDARV_MPEG_VLD_OFFSET		0x12c
#define CEDARV_MPEG_VLD_LEN			0x130
#define CEDARV_MPEG_VLD_END			0x134
#define CEDARV_MPEG_MBH_ADDR        0x138
#define CEDARV_MPEG_DCAC_ADDR       0x13c
#define CEDARV_MPEG_NCF_ADDR        0x144
#define CEDARV_MPEG_REC_LUMA		0x148
#define CEDARV_MPEG_REC_CHROMA		0x14c
#define CEDARV_MPEG_FWD_LUMA		0x150
#define CEDARV_MPEG_FWD_CHROMA		0x154
#define CEDARV_MPEG_BACK_LUMA		0x158
#define CEDARV_MPEG_BACK_CHROMA		0x15c
#define CEDARV_MPEG_SOCX            0x160
#define CEDARV_MPEG_SOCY            0x164
#define CEDARV_MPEG_SOL             0x168
#define CEDARV_MPEG_SDLX            0x16c
#define CEDARV_MPEG_SDLY            0x170
#define CEDARV_MPEG_SPRITESHIFT     0x174
#define CEDARV_MPEG_SDCX            0x178
#define CEDARV_MPEG_SDCY            0x17c
#define CEDARV_MPEG_IQ_MIN_INPUT	0x180
#define CEDARV_MPEG_QP_INPUT        0x184
#define CEDARV_MPEG_MSMPEG4_HDR     0x188
#define CEDARV_MPEG_MV5             0x1A8
#define CEDARV_MPEG_MV6             0x1AC
#define CEDARV_MPEG_JPEG_SIZE		0x1b8
#define CEDARV_MPEG_JPEG_MCU        0x1bc
#define CEDARV_MPEG_JPEG_RES_INT		0x1c0

#define CEDARV_MPEG_ERROR           0x1c4
#define CEDARV_MPEG_CTR_MB          0x1c8
#define CEDARV_MPEG_ROT_LUMA		0x1cc
#define CEDARV_MPEG_ROT_CHROMA		0x1d0
#define CEDARV_MPEG_SDROT_CTRL		0x1d4
#define CEDARV_MPEG_JPEG_MCU_START  0x1d8
#define CEDARV_MPEG_JPEG_MCU_END    0x1dc
#define CEDARV_MPEG_RAM_WRITE_PTR		0x1e0
#define CEDARV_MPEG_RAM_WRITE_DATA		0x1e4


#define CEDARV_H264_FRAME_SIZE		0x200
#define CEDARV_H264_PIC_HDR			0x204
#define CEDARV_H264_SLICE_HDR		0x208
#define CEDARV_H264_SLICE_HDR2		0x20c
#define CEDARV_H264_PRED_WEIGHT		0x210
#define CEDARV_H264_QP_PARAM		0x21c
#define CEDARV_H264_CTRL			0x220
#define CEDARV_H264_TRIGGER			0x224
#define CEDARV_H264_STATUS			0x228
#define CEDARV_H264_CUR_MB_NUM		0x22c
#define CEDARV_H264_VLD_ADDR		0x230
#define CEDARV_H264_VLD_OFFSET		0x234
#define CEDARV_H264_VLD_LEN			0x238
#define CEDARV_H264_VLD_END			0x23c
#define CEDARV_H264_SDROT_CTRL		0x240
#define CEDARV_H264_SDROT_LUMA                0x244
#define CEDARV_H264_SDROT_CHROMA              0x248

#define CEDARV_H264_OUTPUT_FRAME_IDX	0x24c
#define CEDARV_H264_FIELD_INTRA_INFO_BUF	0x250
#define CEDARV_H264_NEIGHBOR_INFO_BUF		0x254
#define CEDARV_H264_MB_ADDR             0x260
#define CEDARV_H264_ERROR           0x2b8
#define CEDARV_H264_BASIC_BITS		0x2dc
#define CEDARV_H264_RAM_WRITE_PTR		0x2e0
#define CEDARV_H264_RAM_WRITE_DATA		0x2e4

#define CEDARV_SRAM_H264_PRED_WEIGHT_TABLE	0x000
#define CEDARV_SRAM_H264_FRAMEBUFFER_LIST	0x400
#define CEDARV_SRAM_H264_REF_LIST0		0x640
#define CEDARV_SRAM_H264_REF_LIST1		0x664
#define CEDARV_SRAM_H264_SCALING_LISTS	0x800

#define CEDARV_HEVC_NAL_HDR                 0x500
#define CEDARV_HEVC_SPS                     0x504
#define CEDARV_HEVC_PIC_SIZE                0x508
#define CEDARV_HEVC_PCM_HDR                 0x50c
#define CEDARV_HEVC_PPS0                    0x510
#define CEDARV_HEVC_PPS1                    0x514
#define CEDARV_HEVC_SCALING_LIST_CTRL       0x518
#define CEDARV_HEVC_SLICE_HDR0              0x520
#define CEDARV_HEVC_SLICE_HDR1              0x524
#define CEDARV_HEVC_SLICE_HDR2              0x528
#define CEDARV_HEVC_CTB_ADDR                0x52c
#define CEDARV_HEVC_CTRL                    0x530
#define CEDARV_HEVC_TRIG                    0x534
#define CEDARV_HEVC_STATUS                  0x538
#define CEDARV_HEVC_CTU_NUM                 0x53c
#define CEDARV_HEVC_BITS_ADDR               0x540
#define CEDARV_HEVC_BITS_OFFSET             0x544
#define CEDARV_HEVC_BITS_LEN                0x548
#define CEDARV_HEVC_BITS_END_ADDR           0x54c
#define CEDARV_HEVC_EXTRA_OUT_CTRL          0x550
#define CEDARV_HEVC_EXTRA_OUT_LUMA_ADDR     0x554
#define CEDARV_HEVC_EXTRA_OUT_CHROMA_ADDR   0x558
#define CEDARV_HEVC_REC_BUF_IDX             0x55c
#define CEDARV_HEVC_NEIGHBOR_INFO_ADDR      0x560
#define CEDARV_HEVC_TILE_LIST_ADDR          0x564
#define CEDARV_HEVC_TILE_START_CTB          0x568
#define CEDARV_HEVC_TILE_END_CTB            0x56c
#define CEDARV_HEVC_SCALING_LIST_DC_COEF0   0x578
#define CEDARV_HEVC_SCALING_LIST_DC_COEF1   0x57c
#define CEDARV_HEVC_BITS_DATA               0x5dc
#define CEDARV_HEVC_SRAM_ADDR               0x5e0
#define CEDARV_HEVC_SRAM_DATA               0x5e4

#define CEDARV_SRAM_HEVC_PRED_WEIGHT_LUMA_L0        0x000
#define CEDARV_SRAM_HEVC_PRED_WEIGHT_CHROMA_L0      0x020
#define CEDARV_SRAM_HEVC_PRED_WEIGHT_LUMA_L1        0x060
#define CEDARV_SRAM_HEVC_PRED_WEIGHT_CHROMA_L1      0x080
#define CEDARV_SRAM_HEVC_PIC_LIST                   0x400
#define CEDARV_SRAM_HEVC_SCALING_LISTS              0x800
#define CEDARV_SRAM_HEVC_REF_PIC_LIST0              0xc00
#define CEDARV_SRAM_HEVC_REF_PIC_LIST1              0xc10

#define CEDARV_ISP_PIC_SIZE 		0x0a00 	//ISP source picture size in macroblocks (16x16)
#define CEDARV_ISP_PIC_STRIDE 		0x0a04 	//ISP source picture stride
#define CEDARV_ISP_CTRL 			0x0a08 	//ISP IRQ Control
#define CEDARV_ISP_TRIG 			0x0a0c 	//ISP Trigger
#define CEDARV_ISP_SCALER_SIZE 		0x0a2c 	//ISP scaler frame size/16
#define CEDARV_ISP_SCALER_OFFSET_Y 		0x0a30 	//ISP scaler picture offset for luma
#define CEDARV_ISP_SCALER_OFFSET_C 		0x0a34 	//ISP scaler picture offset for chroma
#define CEDARV_ISP_SCALER_FACTOR 		0x0a38 	//ISP scaler picture scale factor
//#define CEDARV_ISP_BUF??? 	0x0a44 	4B 	ISP PHY Buffer offset
//#define CEDARV_ISP_BUF??? 	0x0a48 	4B 	ISP PHY Buffer offset
//#define CEDARV_ISP_BUF??? 	0x0a4C 	4B 	ISP PHY Buffer offset
//#define CEDARV_ISP_?? 	0x0a74 	4B 	ISP ??
#define CEDARV_ISP_OUTPUT_LUMA 		0x0a70 	//ISP Output LUMA Address
#define CEDARV_ISP_OUTPUT_CHROMA 		0x0a74 	//ISP Output CHROMA Address
#define CEDARV_ISP_WB_THUMB_LUMA 		0x0a78 	//ISP THUMB WriteBack PHY LUMA Address
#define CEDARV_ISP_WB_THUMB_CHROMA 		0x0a7c 	//ISP THUMB WriteBack PHY CHROMA Adress
#define CEDARV_ISP_SRAM_INDEX 		0x0ae0 	//ISP VE SRAM Index
#define CEDARV_ISP_SRAM_DATA 		0x0ae4 	//ISP VE SRAM Data

#define CEDARV_AVC_PARAM			0xb04
#define CEDARV_AVC_QP			    0xb08
#define CEDARV_AVC_MOTION_EST		0xb10
#define CEDARV_AVC_CTRL			    0xb14
#define CEDARV_AVC_TRIGGER			0xb18
#define CEDARV_AVC_STATUS			0xb1c
#define CEDARV_AVC_BASIC_BITS		0xb20
#define CEDARV_AVC_VLE_ADDR			0xb80
#define CEDARV_AVC_VLE_END			0xb84
#define CEDARV_AVC_VLE_OFFSET		0xb88
#define CEDARV_AVC_VLE_MAX			0xb8c
#define CEDARV_AVC_VLE_LENGTH		0xb90
#define CEDARV_AVC_REF_LUMA			0xba0
#define CEDARV_AVC_REF_CHROMA		0xba4
#define CEDARV_AVC_REC_LUMA			0xbb0
#define CEDARV_AVC_REC_CHROMA		0xbb4
#define CEDARV_AVC_REF_SLUMA		0xbb8
#define CEDARV_AVC_REC_SLUMA		0xbbc
#define CEDARV_AVC_MB_INFO			0xbc0
#define CEDARV_AVC_SDRAM_INDEX		0xbe0
#define CEDARV_AVC_SDRAM_DATA		0xbe4



#define CEDARV_MPEG_TRIG_FORMAT                     24
#define CEDARV_MPEG_TRIG_FORMAT_SIZE                0x7
#define CEDARV_MPEG_TRIG_FORMAT_RESERVED            0x0

#define CEDARV_MPEG_TRIG_COLOR_FORMAT               27
#define CEDARV_MPEG_TRIG_COLOR_FORMAT_SIZE          0x7
#define CEDARV_MPEG_TRIG_COLOR_FORMAT_YUV_4_2_0     0x0
#define CEDARV_MPEG_TRIG_COLOR_FORMAT_YUV_4_1_1     0x1
#define CEDARV_MPEG_TRIG_COLOR_FORMAT_YUV_4_2_2_HOR 0x2
#define CEDARV_MPEG_TRIG_COLOR_FORMAT_YUV_4_4_4     0x3
#define CEDARV_MPEG_TRIG_COLOR_FORMAT_YUV_4_2_2_VER 0x4

#define CEDARV_MPEG_TRIG_ERROR_DISABLE_BIT  31
#define CEDARV_MPEG_TRIG_ERROR_DISABLE_SIZE  0x1

#define CEDARV_MPEG_TRIG_VE_START_TYPE(val)          (((val) & 0xf) << 0)
#define CEDARV_MPEG_TRIG_STCD_TYPE(val)              (((val) & 0x3) << 4)
#define CEDARV_MPEG_TRIG_ISGETBIT(val)               (((val) & 0x1) << 7)
#define CEDARV_MPEG_TRIG_NUM_MB_IN_GOB(val)          (((val) & ((1<<16)-1)) << 8)
#define CEDARV_MPEG_TRIG_DEC_FORMAT(val)             (((val) & 0x7) << 24)
#define CEDARV_MPEG_TRIG_CHROM_FORMAT(val)           (((val) & 0x7) << 27)
#define CEDARV_MPEG_TRIG_MB_BOUNDARY(val)            (((val) & 0x1) << 31)

#define CEDARV_MPEG_TRIG_ERROR_DISABLE(err_dis)      ((err_dis & CEDARV_MPEG_TRIG_ERROR_DISABLE_SIZE) << CEDARV_MPEG_TRIG_ERROR_DISABLE_BIT)

#define CEDARV_MPEG_CTRL_CEDARV_FINISH_INT_EN(val)       (((val) & 0x1) << 3)
#define CEDARV_MPEG_CTRL_CEDARV_ERROR_INT_EN(val)        (((val) & 0x1) << 4)
#define CEDARV_MPEG_CTRL_VLD_MEM_REQ_INT_EN(val)     (((val) & 0x1) << 5)
#define CEDARV_MPEG_CTRL_NOT_WRITE_RECONS_FLAG(val)  (((val) & 0x1) << 7)
#define CEDARV_MPEG_CTRL_WRITE_ROTATE_PIC(val)       (((val) & 0x1) << 8)
#define CEDARV_MPEG_CTRL_OUTPUT_EN(val)              (((val) & 0x1) << 12)
#define CEDARV_MPEG_CTRL_OUTLOOP_DBLK_EN(val)        (((val) & 0x1) << 13)
#define CEDARV_MPEG_CTRL_QP_AC_DC_OUT_EN(val)        (((val) & 0x1) << 14)
#define CEDARV_MPEG_CTRL_HISTOGRAM_OUTPUT_EN(val)    (((val) & 0x1) << 16)
#define CEDARV_MPEG_CTRL_BYPASS_IQIS(val)            (((val) & 0x1) << 17)
#define CEDARV_MPEG_CTRL_MVCS_FLD_HM(val)            (((val) & 0x1) << 19)
#define CEDARV_MPEG_CTRL_MVCS_FLD_QM(val)            (((val) & 0x3) << 20)
#define CEDARV_MPEG_CTRL_MVCS_MV1_QM(val)            (((val) & 0x3) << 22)
#define CEDARV_MPEG_CTRL_MVCS_MV4_QM(val)            (((val) & 0x3) << 24)
#define CEDARV_MPEG_CTRL_SWVLD_FLAG(val)             (((val) & 0x1) << 27)
#define CEDARV_MPEG_CTRL_SW_CHROM_MV_SEL(val)        (((val) & 0x1) << 28)
#define CEDARV_MPEG_CTRL_FDC_QAC_IN_DRAM(val)        (((val) & 0x1) << 30)
#define CEDARV_MPEG_CTRL_MC_CACHE_EN(val)            (((val) & 0x1) << 31)

#define CEDARV_MPEG_MVOPHDR_VOP_FCODE_B(val)         (((val) & 0x7) << 0)
#define CEDARV_MPEG_MVOPHDR_VOP_FCODE_F(val)         (((val) & 0x7) << 3)
#define CEDARV_MPEG_MVOPHDR_ALTER_V_SCAN(val)        (((val) & 0x1) << 6)
#define CEDARV_MPEG_MVOPHDR_TOP_FIELD_FIRST(val)     (((val) & 0x1) << 7)
#define CEDARV_MPEG_MVOPHDR_INTRA_DC_VLC_THR(val)    (((val) & 0x7) << 8)
#define CEDARV_MPEG_MVOPHDR_IS_H263_UMV(val)         (((val) & 0x1) << 12)
#define CEDARV_MPEG_MVOPHDR_EN_ADV_INTRA_PRED(val)   (((val) & 0x1) << 13)
#define CEDARV_MPEG_MVOPHDR_EN_MODI_QUANT(val)       (((val) & 0x1) << 14)
#define CEDARV_MPEG_MVOPHDR_IS_H263_PMV(val)         (((val) & 0x1) << 15)
#define CEDARV_MPEG_MVOPHDR_USE_H263_ESCAPE(val)     (((val) & 0x1) << 16)
#define CEDARV_MPEG_MVOPHDR_VOP_ROUNDING_TYPE(val)   (((val) & 0x1) << 17)
#define CEDARV_MPEG_MVOPHDR_VOP_CODING_TYPE(val)     (((val) & 0x3) << 18)
#define CEDARV_MPEG_MVOPHDR_NO_WRAPPING_POINTS(val)  (((val) & 0x3) << 20)
#define CEDARV_MPEG_MVOPHDR_RESYNC_MARKER_DIS(val)   (((val) & 0x1) << 22)
#define CEDARV_MPEG_MVOPHDR_QUARTER_SAMPLE(val)      (((val) & 0x1) << 23)
#define CEDARV_MPEG_MVOPHDR_QUANT_TYPE(val)          (((val) & 0x1) << 24)
#define CEDARV_MPEG_MVOPHDR_SPRITE_WRAP_ACCURACY(val) (((val) & 0x3) << 25)
#define CEDARV_MPEG_MVOPHDR_CO_LOCATED_VOP_TYPE(val) (((val) & 0x3) << 28)
#define CEDARV_MPEG_MVOPHDR_INTERLACED(val)          (((val) & 0x1) << 30)
#define CEDARV_MPEG_MVOPHDR_SHORT_VIDEO_HEADER(val)  (((val) & 0x1) << 31)

//H264 Status values
#define VLD_BUSY                (1 << 8)
#define VLD_DATA_REQ_INTERRUPT  (1 << 2)

//CEDARV_OUTPUT_FORMAT
#define OUTPUT_FORMAT(x)		((x) << 4)
#define OUTPUT_FORMAT_TILE32x32 	(OUTPUT_FORMAT(0x0))
#define OUTPUT_FORMAT_TILE128x32	(OUTPUT_FORMAT(0x1))
#define OUTPUT_FORMAT_I420		(OUTPUT_FORMAT(0x2))
#define OUTPUT_FORMAT_YV12		(OUTPUT_FORMAT(0x3))
#define OUTPUT_FORMAT_NV12		(OUTPUT_FORMAT(0x4))
#define OUTPUT_FORMAT_NV21		(OUTPUT_FORMAT(0x5))

#define EXTRA_OUTPUT_FORMAT(x)                ((x) << 0)
#define EXTRA_OUTPUT_FORMAT_TILE32x32         (EXTRA_OUTPUT_FORMAT(0x0))
#define EXTRA_OUTPUT_FORMAT_TILE128x32        (EXTRA_OUTPUT_FORMAT(0x1))
#define EXTRA_OUTPUT_FORMAT_I420              (EXTRA_OUTPUT_FORMAT(0x2))
#define EXTRA_OUTPUT_FORMAT_YV12              (EXTRA_OUTPUT_FORMAT(0x3))
#define EXTRA_OUTPUT_FORMAT_NV12              (EXTRA_OUTPUT_FORMAT(0x4))
#define EXTRA_OUTPUT_FORMAT_NV21              (EXTRA_OUTPUT_FORMAT(0x5))

//CEDARV_HVEC_TRIG
#define HEVC_TRIG_FUNCTION(x)		((x))
#define HEVC_TRIG_FUNCTION_U		(HEVC_TRIG_FUNCTION(0x2))
#define HEVC_TRIG_FUNCTION_SKIP         (HEVC_TRIG_FUNCTION(0x3))
#define HEVC_TRIG_FUNCTION_SE		(HEVC_TRIG_FUNCTION(0x4))
#define HEVC_TRIG_FUNCTION_UE		(HEVC_TRIG_FUNCTION(0x5))
#define HEVC_TRIG_FUNCTION_SYNC		(HEVC_TRIG_FUNCTION(0x7))
#define HEVC_TRIG_FUNCTION_DECODE	(HEVC_TRIG_FUNCTION(0x8))
#define HEVC_TRIG_PARA(x)		((x) << 8)

//CEDARV_HEVC_STATUS
#define HEVC_STATUS_VLD_BUSY		(1 << 8)
#define HEVC_STATUS_ERR			(1 << 1)
#define HEVC_STATUS_DONE		(1 << 0)

//CEDARV_HEVC_CTB_ADDR
#define HEVC_CTB_ADDR_X(x)		(((x) & 0xFF))
#define HEVC_CTB_ADDR_Y(x)		(((x) & 0xFF) << 16)

#endif
