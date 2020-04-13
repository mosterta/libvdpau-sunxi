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

#include <string.h>
#include "vdpau_private.h"
#include "ve.h"
#include <time.h>
#include <stdio.h>

#define TIMEMEAS 0

extern uint64_t get_time();
static const uint8_t zigzag_scan[64] =
{
	 0,  1,  5,  6, 14, 15, 27, 28,
	 2,  4,  7, 13, 16, 26, 29, 42,
	 3,  8, 12, 17, 25, 30, 41, 43,
	 9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};


static int mpeg_find_startcode(CEDARV_MEMORY mem, int len)
{
	int pos = 0;
	uint8_t *data = cedarv_getPointer(mem);
	while (pos < len)
	{
		int zeros = 0;
		for ( ; pos < len; pos++)
		{
			if (data[pos] == 0x00)
				zeros++;
			else if (data[pos] == 0x01 && zeros >= 2)
			{
				pos++;
				break;
			}
			else
				zeros = 0;
		}

		uint8_t marker = data[pos++];

		if (marker >= 0x01 && marker <= 0xaf)
			return pos - 4;
	}
	return 0;
}
static unsigned long num_pics=0;
static unsigned long num_longs=0;

static VdpStatus mpeg12_decode(decoder_ctx_t *decoder, VdpPictureInfo const *_info, const int len, video_surface_ctx_t *output)
{
	VdpPictureInfoMPEG1Or2 const *info = (VdpPictureInfoMPEG1Or2 const *)_info;
	int start_offset = mpeg_find_startcode(decoder->data, len);

	int i;

	// activate MPEG engine
	void *cedarv_regs = cedarv_get(CEDARV_ENGINE_MPEG, 0);

	output->source_format = INTERNAL_YCBCR_FORMAT;

	// set quantisation tables
	for (i = 0; i < 64; i++)
		writel((uint32_t)(64 + zigzag_scan[i]) << 8 | info->intra_quantizer_matrix[i], cedarv_regs + CEDARV_MPEG_IQ_MIN_INPUT);
	for (i = 0; i < 64; i++)
		writel((uint32_t)(zigzag_scan[i]) << 8 | info->non_intra_quantizer_matrix[i], cedarv_regs + CEDARV_MPEG_IQ_MIN_INPUT);

	// set size
    uint16_t width = (decoder->width + 15) / 16;
    uint16_t height = (decoder->height + 15) / 16;
	output->vidWidth = width * 16;
	output->vidHeight = height * 16;
	writel((width << 8) | height, cedarv_regs + CEDARV_MPEG_SIZE);
	writel(((width * 16) << 16) | (height * 16), cedarv_regs + CEDARV_MPEG_FRAME_SIZE);

	// set picture header
	uint32_t pic_header = 0;
	pic_header |= ((info->picture_coding_type & 0x7) << 28);
	pic_header |= ((info->f_code[0][0] & 0xf) << 24);
	pic_header |= ((info->f_code[0][1] & 0xf) << 20);
	pic_header |= ((info->f_code[1][0] & 0xf) << 16);
	pic_header |= ((info->f_code[1][1] & 0xf) << 12);
	pic_header |= ((info->intra_dc_precision & 0x3) << 10);
	pic_header |= ((info->picture_structure & 0x3) << 8);
	pic_header |= ((info->top_field_first & 0x1) << 7);
	pic_header |= ((info->frame_pred_frame_dct & 0x1) << 6);
	pic_header |= ((info->concealment_motion_vectors & 0x1) << 5);
	pic_header |= ((info->q_scale_type & 0x1) << 4);
	pic_header |= ((info->intra_vlc_format & 0x1) << 3);
	pic_header |= ((info->alternate_scan & 0x1) << 2);
	pic_header |= ((info->full_pel_forward_vector & 0x1) << 1);
	pic_header |= ((info->full_pel_backward_vector & 0x1) << 0);
	if (decoder->profile == VDP_DECODER_PROFILE_MPEG1)
		pic_header |= 0x000003c0;
	writel(pic_header, cedarv_regs + CEDARV_MPEG_PIC_HDR);

	// ??
	writel(0x80000138 | ((cedarv_get_version() < 0x1680) << 7), cedarv_regs + CEDARV_MPEG_CTRL);
        if (cedarv_get_version() >= 0x1680)
                writel((0x1 << 30) | (0x1 << 28) , cedarv_regs + CEDARV_EXTRA_OUT_FMT_OFFSET);

	// set forward/backward predicion buffers
	if (info->forward_reference != VDP_INVALID_HANDLE)
	{
		video_surface_ctx_t *forward = handle_get(info->forward_reference);
                if(forward)
                {
		   writel(cedarv_virt2phys(forward->dataY), cedarv_regs + CEDARV_MPEG_FWD_LUMA);
		   writel(cedarv_virt2phys(forward->dataU)/* + forward->plane_size */, cedarv_regs + CEDARV_MPEG_FWD_CHROMA);
                   handle_release(info->forward_reference);
                }
	}
	if (info->backward_reference != VDP_INVALID_HANDLE)
	{
		video_surface_ctx_t *backward = handle_get(info->backward_reference);
                if(backward)
                {
		   writel(cedarv_virt2phys(backward->dataY), cedarv_regs + CEDARV_MPEG_BACK_LUMA);
		   writel(cedarv_virt2phys(backward->dataU)/* + backward->plane_size*/, cedarv_regs + CEDARV_MPEG_BACK_CHROMA);
                   handle_release(info->backward_reference);
                }
	}

	// set output buffers (Luma / Croma)
	writel(cedarv_virt2phys(output->dataY), cedarv_regs + CEDARV_MPEG_REC_LUMA);
	writel(cedarv_virt2phys(output->dataU)/* + output->plane_size*/, cedarv_regs + CEDARV_MPEG_REC_CHROMA);
	writel(cedarv_virt2phys(output->dataY), cedarv_regs + CEDARV_MPEG_ROT_LUMA);
	writel(cedarv_virt2phys(output->dataU)/* + output->plane_size*/, cedarv_regs + CEDARV_MPEG_ROT_CHROMA);

        if(cedarv_get_version() >= 0x1680)
        {
            writel(OUTPUT_FORMAT_NV12 | EXTRA_OUTPUT_FORMAT_NV12, cedarv_regs + CEDARV_OUTPUT_FORMAT);
            output->source_format = VDP_YCBCR_FORMAT_NV12;
        }

	// set input offset in bits
	writel(start_offset * 8, cedarv_regs + CEDARV_MPEG_VLD_OFFSET);

	// set input length in bits
	writel((len - start_offset) * 8, cedarv_regs + CEDARV_MPEG_VLD_LEN);

	// input end
	uint32_t input_addr = cedarv_virt2phys(decoder->data);
	writel(input_addr + VBV_SIZE - 1, cedarv_regs + CEDARV_MPEG_VLD_END);

	// set input buffer
	writel((input_addr & 0x0ffffff0) | (input_addr >> 28) | (0x7 << 28), cedarv_regs + CEDARV_MPEG_VLD_ADDR);

	// trigger
	writel((((decoder->profile == VDP_DECODER_PROFILE_MPEG1) ? 1 : 2) << 24) | 0x8000000f, cedarv_regs + CEDARV_MPEG_TRIGGER);

	// wait for interrupt
	++num_pics;
#if TIMEMEAS
	uint64_t tv, tv2;
	tv = get_time();
#endif
	cedarv_wait(1);
#if TIMEMEAS
	tv2 = get_time();
	if (tv2-tv > 10000000) {
		printf("cedarv_wait, longer than 10ms:%lld, pics=%ld, longs=%ld\n", tv2-tv, num_pics, ++num_longs);
		}
#endif	

	// clean interrupt flag
	writel(0x0000c00f, cedarv_regs + CEDARV_MPEG_STATUS);

	// stop MPEG engine
	cedarv_put();
        output->frame_decoded = 1;
        
	return VDP_STATUS_OK;
}

VdpStatus new_decoder_mpeg12(decoder_ctx_t *decoder)
{
	decoder->decode = mpeg12_decode;
	return VDP_STATUS_OK;
}
