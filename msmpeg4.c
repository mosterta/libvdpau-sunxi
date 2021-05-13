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
#include <assert.h>
#include "bitstream.h"
#include "mpeg4.h"
#include "stdlib.h"
#include "mp4_vars.h"
#include <stdio.h>

#define MBAC_BITRATE 50*1024
#define TIMEMEAS 0

extern uint32_t show_bits(bitstream *bs, int n);
extern uint32_t get_bits(bitstream *bs, int n);
extern int bytealign(bitstream *bs);
extern int  bytealigned(bitstream *bs, int nbit);
extern void flush_bits(bitstream *bs, int nbit);
extern int bits_left(bitstream *bs);
extern int nextbits_bytealigned(bitstream *bs, int nbit);

static void dumpData(char* data)
{
    int pos=0;
    printf("data[%d]=%X data[+1]=%X data[+2]=%X data[+3]=%X data[+4]=%X\n",
        pos, data[pos], data[pos+1], data[pos+2], data[pos+3],data[pos+4]);
}

static void msmpeg4_private_free(decoder_ctx_t *decoder)
{
	mp4_private_t *decoder_p = (mp4_private_t *)decoder->private;
	cedarv_free(decoder_p->mbh_buffer);
	cedarv_free(decoder_p->dcac_buffer);
	cedarv_free(decoder_p->ncf_buffer);
	free(decoder_p);
}

static int decode_vop_header(bitstream *bs, VdpPictureInfoMPEG4Part2 const *info, mp4_private_t *priv)
{
    int dummy;
    vop_header_t *h = &priv->vop_header;
    int code;
    video_packet_header_t *vh = &priv->pkt_hdr;


    if(h->msmpeg4_version==1){
        int start_code = get_bits(bs, 32);
        if(start_code!=0x00000100){
            //av_log(s->avctx, AV_LOG_ERROR, "invalid startcode\n");
            return -1;
        }

        get_bits(bs, 5); // frame number */
    }

    h->vop_coding_type = get_bits(bs, 2) ;
    if (h->vop_coding_type != VOP_I &&
        h->vop_coding_type != VOP_P){
        printf("invalid picture type\n");
        return -1;
    }

    h->vop_quant = get_bits(bs, 5);
    if(h->vop_quant==0){
        printf( "invalid qscale\n");
        return -1;
    }

    if (h->vop_coding_type == VOP_I) {
        code = get_bits(bs, 5);
        if(h->msmpeg4_version==1){
#if 0
			if(code==0 || code>s->mb_height){
                av_log(s->avctx, AV_LOG_ERROR, "invalid slice height %d\n", code);
                return -1;
            }
#endif
            h->slice_height = code;
        }else{
            /* 0x17: one slice, 0x18: two slices, ... */
            if (code < 0x17){
                printf("error, slice code was %X\n", code);
                return -1;
            }
			h->slice_height = vh->mb_height / (code - 0x16);
        }

        switch(h->msmpeg4_version){
        case 0:
            {
                int code = show_bits(bs, 2);
                switch(code) {
                case 0:
                case 1:
                    flush_bits(bs, 1);
                    h->rl_chroma_table_index = 0;
                    //vld_intra chroma functions insert here
                    break;
                case 2:
                    h->rl_chroma_table_index = 1;
                    //vld_intra chroma functions insert here
                    flush_bits(bs, 2);
                    break;
                case 3:
                    h->rl_chroma_table_index = 2;
                    //vld_intra chroma functions insert here
                    flush_bits(bs, 2);
                    break;
                default:
                        break;
                }
                code = show_bits(bs, 2);
                switch(code) {
                case 0:
                case 1:
                    h->rl_table_index = 0;
                    flush_bits(bs, 1);
                    //vld_intra lum functions insert here
                    break;
                case 2:
                    h->rl_table_index = 1;
                    flush_bits(bs, 2);
                    //vld_intra lum functions insert here
                    break;
                case 3:
                    h->rl_table_index = 2;
                    flush_bits(bs, 2);
                    //vld_intra lum functions insert here
                    break;
                }
                int dc_value = get_bits(bs, 1);
                h->dc_table_index = dc_value;
                if(dc_value == 0) {
                        //getDC chroma and lum ptr insert here
                }
                else {
                        //getDC chroma and lum ptr insert here 
                }
                //getCBP ptr insert here
            }
            break;
        case 1:
        case 2:
            h->rl_chroma_table_index = 2;
            h->rl_table_index = 2;

            h->dc_table_index = 0; //not used
            break;
        case 3:
            h->rl_chroma_table_index = decode012(bs);
            h->rl_table_index = decode012(bs);

            h->dc_table_index = get_bits(bs,1);
            break;
        case 4:
            //ff_msmpeg4_decode_ext_header(s, (2+5+5+17+7)/8);

            if(h->bit_rate > MBAC_BITRATE) h->per_mb_rl_table= get_bits(bs,1);
            else                           h->per_mb_rl_table= 0;

            if(!h->per_mb_rl_table){
                h->rl_chroma_table_index = decode012(bs);
                h->rl_table_index = decode012(bs);
            }

            h->dc_table_index = get_bits(bs,1);
            h->inter_intra_pred= 0;
            break;
        }
        h->no_rounding = 1;
    } else {
        switch(h->msmpeg4_version){
        case 0:
            {
                h->use_skip_mb_code = get_bits(bs, 1);
                int code = show_bits(bs, 2);
                switch(code) {
                case 0:
                case 1:
                    flush_bits(bs, 1);
                    h->rl_table_index = 0;
                    h->rl_chroma_table_index = 0;
                    //vld_intra chroma functions insert here
                    //and inter function
                    break;
                case 2:
                    h->rl_table_index = 1;
                    h->rl_chroma_table_index = 1;
                    //vld_intra chroma functions insert here
                    flush_bits(bs, 2);
                    break;
                case 3:
                    h->rl_table_index = 2;
                    h->rl_chroma_table_index = 2;
                    //vld_intra chroma functions insert here
                    flush_bits(bs, 2);
                    break;
                default:
                    break;
                }
                int dc_table_index = get_bits(bs, 1);
                if(dc_table_index == 0) {
                    //insert getDC function pointers
                    h->dc_table_index = dc_table_index;
                }
                else {
                    //insert getDC function pointers
                    h->dc_table_index = 1;
                }
                int mv_table_index = get_bits(bs, 1);
                if(mv_table_index == 0) {
                    //insert getMVdata function pointers, getvophdr_311
                    h->mv_table_index = mv_table_index;
                }
                else {
                    //insert getMVdata function pointers, getvophdr_311
                    h->mv_table_index = 1;
                }
                //insert getCBP function pointer here
            }
            break;
        case 1:
        case 2:
            if(h->msmpeg4_version==1)
                h->use_skip_mb_code = 1;
            else
                h->use_skip_mb_code = get_bits(bs, 1);
            h->rl_table_index = 2;
            h->rl_chroma_table_index = h->rl_table_index;
            h->dc_table_index = 0; //not used
            h->mv_table_index = 0;
            break;
        case 3:
            h->use_skip_mb_code = get_bits(bs, 1);
            h->rl_table_index = decode012(bs);
            h->rl_chroma_table_index = h->rl_table_index;

            h->dc_table_index = get_bits(bs, 1);

            h->mv_table_index = get_bits(bs, 1);
            break;
        case 4:
            h->use_skip_mb_code = get_bits(bs, 1);

            if(h->bit_rate > MBAC_BITRATE) h->per_mb_rl_table= get_bits(bs, 1);
            else                           h->per_mb_rl_table= 0;

            if(!h->per_mb_rl_table){
                h->rl_table_index = decode012(bs);
                h->rl_chroma_table_index = h->rl_table_index;
            }

            h->dc_table_index = get_bits(bs, 1);

            h->mv_table_index = get_bits(bs, 1);
            //h->inter_intra_pred= (h->width*h->height < 320*240 && h->bit_rate<=II_BITRATE);
            break;
        }

	//if(h->vop_coding_type != VOP_I)
	{
           if(h->flipflop_rounding){
              h->no_rounding ^= 1;
           }else{
              h->no_rounding = 0;
           }
	}
    }

    return 1;
}

static unsigned long num_pics=0;
static unsigned long num_longs=0;
int msmpeg4_decode(decoder_ctx_t *decoder, VdpPictureInfoMPEG4Part2 const *_info, 
			const int len, video_surface_ctx_t *output,
			uint32_t * bitstream_pos_returned)
{
    VdpPictureInfoMPEG4Part2 const *info = (VdpPictureInfoMPEG4Part2 const *)_info;
    mp4_private_t *decoder_p = (mp4_private_t *)decoder->private;
    vop_header_t *h = &decoder_p->vop_header;

    uint32_t   mp4mbaAddr_reg = 0;
    int error=0;

    int i;
    void *cedarv_regs = cedarv_get_regs();
    bitstream bs = { .data = cedarv_getPointer(decoder->data), .length = len, .bitpos = 0 };

    if (!decode_vop_header(&bs, info, decoder_p))
            return 0;

    if(decoder_p->vop_header.vop_coding_type == VOP_I) {
            //check change in picture height/width
    }
    // activate MPEG engine
    //writel((readl(cedarv_regs + CEDARV_CTRL) & ~0xf) | 0x0, cedarv_regs + CEDARV_CTRL);
    cedarv_regs = cedarv_get(CEDARV_ENGINE_MPEG, 0);
    
    writel(0xffffffff, cedarv_regs + CEDARV_MPEG_STATUS);
    writel(0x0, cedarv_regs + CEDARV_MPEG_CTR_MB);
    writel(0x0, cedarv_regs + CEDARV_MPEG_ERROR);

    //if(info->quant_type)
    {
            // set quantisation tables
            for (i = 0; i < 64; i++)
                    writel((uint32_t)(64 + i) << 8 | info->intra_quantizer_matrix[i], cedarv_regs + CEDARV_MPEG_IQ_MIN_INPUT);
            for (i = 0; i < 64; i++)
                    writel((uint32_t)(i) << 8 | info->non_intra_quantizer_matrix[i], cedarv_regs + CEDARV_MPEG_IQ_MIN_INPUT);
    }

    // set forward/backward predicion buffers
    if (info->forward_reference != VDP_INVALID_HANDLE)
    {
            video_surface_ctx_t *forward = handle_get(info->forward_reference);
            if(forward)
            {
               writel(cedarv_virt2phys(forward->dataY), cedarv_regs + CEDARV_MPEG_FWD_LUMA);
               writel(cedarv_virt2phys(forward->dataU), cedarv_regs + CEDARV_MPEG_FWD_CHROMA);
               handle_release(info->forward_reference);
            }
    }
    if (info->backward_reference != VDP_INVALID_HANDLE)
    {
            video_surface_ctx_t *backward = handle_get(info->backward_reference);
            if(backward)
            {
               writel(cedarv_virt2phys(backward->dataY), cedarv_regs + CEDARV_MPEG_BACK_LUMA);
               writel(cedarv_virt2phys(backward->dataU), cedarv_regs + CEDARV_MPEG_BACK_CHROMA);
               handle_release(backward);
            }
    }
    else
    {
            writel(0x0, cedarv_regs + CEDARV_MPEG_BACK_LUMA);
            writel(0x0, cedarv_regs + CEDARV_MPEG_BACK_CHROMA);
    }

    // set trb/trd
    if (decoder_p->vop_header.vop_coding_type == VOP_B)
    {
            writel((info->trb[0] << 16) | (info->trd[0] << 0), cedarv_regs + CEDARV_MPEG_TRBTRD_FRAME);
            // unverified:
            writel((info->trb[1] << 16) | (info->trd[1] << 0), cedarv_regs + CEDARV_MPEG_TRBTRD_FIELD);
    }
    // set size
    uint16_t width  = (decoder_p->vol_header.video_object_layer_width + 15) / 16;
    uint16_t height = (decoder_p->vol_header.video_object_layer_height + 15) / 16;
    if(width == 0 || height == 0) {
            //some videos do not have a VOL, at least at the right time
            //try with the following parameters
            width = ((decoder->width + 15) / 16);
            height = ((decoder->height + 15) / 16);
    }

    uint32_t mpeg_size = 0;
    mpeg_size |= ((width & 1) ? width + 1 : width) << 16;
    mpeg_size |= width << 8;
    mpeg_size |= height;
    writel(mpeg_size, cedarv_regs + CEDARV_MPEG_SIZE);
    writel(((width * 16) << 16) | (height * 16), cedarv_regs + CEDARV_MPEG_FRAME_SIZE);

    // set buffers
    writel(cedarv_virt2phys(decoder_p->mbh_buffer), cedarv_regs + CEDARV_MPEG_MBH_ADDR);
    writel(cedarv_virt2phys(decoder_p->dcac_buffer), cedarv_regs + CEDARV_MPEG_DCAC_ADDR);
    writel(cedarv_virt2phys(decoder_p->ncf_buffer), cedarv_regs + CEDARV_MPEG_NCF_ADDR);

    // set output buffers (Luma / Croma)
    writel(cedarv_virt2phys(output->dataY), cedarv_regs + CEDARV_MPEG_REC_LUMA);
    writel(cedarv_virt2phys(output->dataU), cedarv_regs + CEDARV_MPEG_REC_CHROMA);
    writel(cedarv_virt2phys(output->dataY), cedarv_regs + CEDARV_MPEG_ROT_LUMA);
    writel(cedarv_virt2phys(output->dataU), cedarv_regs + CEDARV_MPEG_ROT_CHROMA);

    if(output->source_format == VDP_YCBCR_FORMAT_NV12)
    {
      int align = output->alignment[0];
      writel(OUTPUT_FORMAT_NV12 | EXTRA_OUTPUT_FORMAT_NV12, cedarv_regs + CEDARV_OUTPUT_FORMAT);
      writel((0x1 << 30) | (0x1 << 28) , cedarv_regs + CEDARV_EXTRA_OUT_FMT_OFFSET);
      writel((ALIGN(output->width, align)/2 << 16) | ALIGN(output->width, align), cedarv_regs + CEDARV_OUTPUT_STRIDE);
      writel((ALIGN(output->width, align)/2 << 16) | ALIGN(output->width, align), cedarv_regs + CEDARV_EXTRA_OUT_STRIDE);
    }

    uint32_t rotscale = 0;
    //bit 0-3: rotate_angle
    //bit 4-5: horiz_scale_ratio
    //bit 6-7: vert_scale_ratio
    //bit 8-11: scale_down_enable
    //bit 12-14: ?
    //bit 16-19: ?
    //bit 20-24: value 6
    //bit 30: 1 
    //bit 31: 0
    const int no_scale = 2;
    const int no_rotate = 6;
    rotscale |= 0x40620000;
    writel(rotscale, cedarv_regs + CEDARV_MPEG_SDROT_CTRL);

                            // ??
    uint32_t cedarv_control = 0;
    cedarv_control |= CEDARV_MPEG_CTRL_CEDARV_FINISH_INT_EN(1);
    cedarv_control |= CEDARV_MPEG_CTRL_CEDARV_ERROR_INT_EN(1);
    cedarv_control |= CEDARV_MPEG_CTRL_QP_AC_DC_OUT_EN(1);

#if 0
    if(decoder_p->vop_header.vop_coding_type == VOP_B)
      cedarv_control |= CEDARV_MPEG_CTRL_SWVLD_FLAG(1);
#endif

    if(decoder_p->vop_header.vop_coding_type == VOP_P)
      cedarv_control |= CEDARV_MPEG_CTRL_OUTPUT_EN(1);
            
    cedarv_control |= CEDARV_MPEG_CTRL_MVCS_FLD_HM(1);
    cedarv_control |= CEDARV_MPEG_CTRL_MC_CACHE_EN(1);
    cedarv_control |= CEDARV_MPEG_CTRL_WRITE_ROTATE_PIC(1);
    cedarv_control |= CEDARV_MPEG_CTRL_NOT_WRITE_RECONS_FLAG((cedarv_get_version() < 0x1680));
    cedarv_control |= CEDARV_MPEG_CTRL_OUTLOOP_DBLK_EN(1);
            
    if(info->quarter_sample)
    {
      switch(decoder_p->vop_header.vop_coding_type)
      {
        case VOP_B:
                  //divx case missing
          cedarv_control |= CEDARV_MPEG_CTRL_MVCS_MV1_QM(1);
          cedarv_control |= CEDARV_MPEG_CTRL_MVCS_MV4_QM(2);
          cedarv_control |= CEDARV_MPEG_CTRL_MVCS_FLD_QM(1);
          break;
        case VOP_P:
        case VOP_S:
                  //divx case missing
          cedarv_control |= CEDARV_MPEG_CTRL_MVCS_MV1_QM(2);
          cedarv_control |= CEDARV_MPEG_CTRL_MVCS_MV4_QM(2);
          cedarv_control |= CEDARV_MPEG_CTRL_MVCS_FLD_QM(1);
          break;
      }
    }
    cedarv_control |= CEDARV_MPEG_CTRL_MVCS_MV1_QM(2);
    writel(cedarv_control, cedarv_regs + CEDARV_MPEG_CTRL);
    mp4mbaAddr_reg = 0;
    writel(mp4mbaAddr_reg, cedarv_regs + CEDARV_MPEG_MBA);

    decoder_p->pkt_hdr.mb_xpos = 0;
    decoder_p->pkt_hdr.mb_ypos = 0;
    uint32_t mba_reg = 0x0;

    uint32_t vop_hdr = 0;
    vop_hdr |= (info->short_video_header & 0x1) << 31;
    vop_hdr |= (info->interlaced & 0x1) << 30;
    vop_hdr |= ((decoder_p->vop_header.vop_coding_type == VOP_B ? h->cbp : 0) & 0x3) << 28;
    //needs correct variable !!!!!
    vop_hdr |= (1 << 27); //divx bit?
    //vop_hdr |= (h->vop_quant & 0x3) << 25;
    vop_hdr |= (info->quant_type & 0x1 ) << 24;
    vop_hdr |= (info->quarter_sample & 0x1) << 23;
    vop_hdr |= (info->resync_marker_disable & 0x1) << 22; //error_res_disable
    vop_hdr |= (decoder_p->vop_header.vop_coding_type & 0x3) << 18;
    vop_hdr |= (decoder_p->vop_header.no_rounding &0x1) << 17;
    vop_hdr |= (h->inter_intra_pred & 0x1) << 15;
    vop_hdr |= (decoder_p->vop_header.intra_dc_vlc_thr & 0x7) << 8;
    vop_hdr |= (info->top_field_first& 0x1) << 7;
    vop_hdr	|= (info->alternate_vertical_scan_flag & 0x1) << 6;
    vop_hdr	|= (decoder_p->vop_header.vop_coding_type != VOP_I ? 
                    info->vop_fcode_forward & 0x7 : 0) << 3;
    vop_hdr	|= (decoder_p->vop_header.vop_coding_type == VOP_B ? 
                    info->vop_fcode_backward & 0x7 : 0) << 0;

    writel(vop_hdr, cedarv_regs + CEDARV_MPEG_VOP_HDR);
    
    writel(decoder_p->vop_header.vop_quant, cedarv_regs + CEDARV_MPEG_QP_INPUT);
    
    //mba_reg = readl(cedarv_regs + CEDARV_MPEG_MBA);
    //if(!info->resync_marker_disable)
    //    mba_reg &= 0xff;    
    //writel( readl(cedarv_regs + CEDARV_MPEG_MBA) /* & 0xff */, cedarv_regs + CEDARV_MPEG_MBA);
    writel(mba_reg, cedarv_regs + CEDARV_MPEG_MBA);
    
    //clean up everything
    writel(0xffffffff, cedarv_regs + CEDARV_MPEG_STATUS);
    // set input offset in bits
    writel(bs.bitpos, cedarv_regs + CEDARV_MPEG_VLD_OFFSET);

    // set input length in bits
    uint32_t vld_len=0;
    vld_len |= ((len*8)+0x1F) & ~0x1F;
    writel(vld_len, cedarv_regs + CEDARV_MPEG_VLD_LEN);

    // input end
    uint32_t input_addr = cedarv_virt2phys(decoder->data);
    writel(input_addr + VBV_SIZE - 1, cedarv_regs + CEDARV_MPEG_VLD_END);

    // set input buffer
    writel((input_addr & 0x0ffffff0) | (input_addr >> 28) | (0x7 << 28), cedarv_regs + CEDARV_MPEG_VLD_ADDR);

    uint32_t msmpeg_pic_hdr = 0;
    int slice_height = decoder_p->vop_header.slice_height;
    if(slice_height == 0)
        slice_height = height;
    msmpeg_pic_hdr |= (slice_height << 24);
    msmpeg_pic_hdr |= (decoder_p->vop_header.per_mb_rl_table & 0x1) << 6;
    msmpeg_pic_hdr |= (decoder_p->vop_header.rl_table_index & 0x3) << 8;
    msmpeg_pic_hdr |= (decoder_p->vop_header.rl_chroma_table_index & 0x3) << 10;
    msmpeg_pic_hdr |= (decoder_p->vop_header.mv_table_index & 0x3) << 14;
    //msmpeg_pic_hdr |= (decoder_p->vop_header.mv_table_index & 0x3) << 12;
    msmpeg_pic_hdr |= (decoder_p->vop_header.dc_table_index & 0x1) << 7;
    //msmpeg_pic_hdr |= (decoder_p->vop_header.dc_table_index & 0x3) << 18;
    //msmpeg_pic_hdr |= (decoder_p->vop_header.use_intra_dc_vlc & 0x1) << 20;
    msmpeg_pic_hdr |= (decoder_p->vop_header.use_skip_mb_code & 0x1) << 20;
    //msmpeg_pic_hdr |= (1 << 2); //is VC1 bit
    
    writel(msmpeg_pic_hdr, cedarv_regs + CEDARV_MPEG_MSMPEG4_HDR);
    
    writel(0x0, cedarv_regs + CEDARV_MPEG_CTR_MB);
    //writel(100000 << 8, cedarv_regs + CEDARV_TIMEOUT);

    // trigger
    int vbv_size = width * height;
    uint32_t mpeg_trigger = 0;
    mpeg_trigger |= CEDARV_MPEG_TRIG_NUM_MB_IN_GOB(vbv_size);
    mpeg_trigger |= CEDARV_MPEG_TRIG_VE_START_TYPE(0xd);
    mpeg_trigger |= CEDARV_MPEG_TRIG_CHROM_FORMAT(CEDARV_MPEG_TRIG_COLOR_FORMAT_YUV_4_2_0);
    mpeg_trigger |= CEDARV_MPEG_TRIG_DEC_FORMAT(4);
    mpeg_trigger |= CEDARV_MPEG_TRIG_MB_BOUNDARY(1);
    writel(mpeg_trigger, cedarv_regs + CEDARV_MPEG_TRIGGER);

        // wait for interrupt
#ifdef TIMEMEAS
    ++num_pics;
    uint64_t tv, tv2;
    tv = get_time();
#endif
    cedarv_wait(1);
#ifdef TIMEMEAS                
    tv2 = get_time();
    if (tv2-tv > 10000000) {
        printf("cedarv_wait, longer than 10ms:%lld, pics=%ld, longs=%ld\n", tv2-tv, num_pics, ++num_longs);
    }
#endif
    // clean interrupt flag
    uint32_t status = readl(cedarv_regs + CEDARV_MPEG_STATUS);
    if(status)
    	printf("status=%X\n", status);
    writel(0x0000000f, cedarv_regs + CEDARV_MPEG_STATUS);
    error = readl(cedarv_regs + CEDARV_MPEG_ERROR);
    if(error)
        printf("got error=%d while decoding frame\n", error);
    writel(0x0, cedarv_regs + CEDARV_MPEG_ERROR);

    int veCurPos = readl(cedarv_regs + CEDARV_MPEG_VLD_OFFSET);
    int veCurPosAligned = (veCurPos+7) & ~ 0x7;
    int byteCurPos = veCurPosAligned / 8;

    if (decoder_p->vop_header.vop_coding_type == VOP_I && veCurPos+17 <= (len*8) )
    {
        bs.bitpos = veCurPos;
        //fps
        (void)get_bits(&bs, 5);
        decoder_p->vop_header.bit_rate = get_bits(&bs, 11);
        decoder_p->vop_header.flipflop_rounding = get_bits(&bs, 1);
    }
    writel(readl(cedarv_regs + CEDARV_MPEG_CTRL) | 0x7C, cedarv_regs + CEDARV_MPEG_CTRL);            
    // stop MPEG engine
    //writel((readl(cedarv_regs + CEDARV_CTRL) & ~0xf) | 0x7, cedarv_regs + CEDARV_CTRL);
    cedarv_put();

//    if (error)
//	return VDP_STATUS_ERROR;
//    else
    {
        output->frame_decoded = 1;
        return VDP_STATUS_OK;
    }
}

VdpStatus new_decoder_msmpeg4(decoder_ctx_t *decoder)
{
    mp4_private_t *decoder_p = calloc(1, sizeof(mp4_private_t));
    memset(decoder_p, 0, sizeof(*decoder_p));
    if (!decoder_p)
       goto err_priv;

    int width = ((decoder->width + 15) / 16);
    int height = ((decoder->height + 15) / 16);

    decoder_p->mbh_buffer = cedarv_malloc(height * 2048);
    if (! cedarv_isValid(decoder_p->mbh_buffer))
       goto err_mbh;

    decoder_p->dcac_buffer = cedarv_malloc(width * height * 2);
    if (! cedarv_isValid(decoder_p->dcac_buffer))
       goto err_dcac;

    decoder_p->ncf_buffer = cedarv_malloc(4 * 1024);
    if (! cedarv_isValid(decoder_p->ncf_buffer))
       goto err_ncf;

    decoder_p->vop_header.flipflop_rounding = 1;
    
    decoder->decode = msmpeg4_decode;
    decoder->private = decoder_p;
    decoder->private_free = msmpeg4_private_free;
    //decoder->setVideoControlData = msmpeg4_setVideoControlData;

    save_tables(&decoder_p->tables);
    
    return VDP_STATUS_OK;

err_ncf:
    cedarv_free(decoder_p->dcac_buffer);
err_dcac:
    cedarv_free(decoder_p->mbh_buffer);
err_mbh:
    free(decoder_p);
err_priv:
    return VDP_STATUS_RESOURCES;
}

