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
#include "vdpau_private.h"
#include <stdio.h>
#include <stdlib.h>

VdpStatus vdp_video_surface_create(VdpDevice device, VdpChromaType chroma_type, uint32_t width, uint32_t height, VdpVideoSurface *surface)
{
   if (!surface)
      return VDP_STATUS_INVALID_POINTER;
   
   if (!width || !height)
      return VDP_STATUS_INVALID_SIZE;
   
   device_ctx_t *dev = handle_get(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
   
   video_surface_ctx_t *vs = handle_create(sizeof(*vs), surface, htype_video);
   if (!vs)
      return VDP_STATUS_RESOURCES;
   
   VDPAU_DBG("vdpau video surface=%d created", *surface);

   vs->device = dev;
   vs->width = width;
   vs->height = height;
   vs->chroma_type = chroma_type;
   
   vs->stride_width 	= (width + 63) & ~63;
   vs->stride_height 	= (height + 63) & ~63;
   if(cedarv_get_version() < 0x1680)
   {
     vs->source_format = INTERNAL_YCBCR_FORMAT;
   }
   else
   {
     vs->source_format = VDP_YCBCR_FORMAT_NV12;
   }
   vs->alignment[0] = 16;
   vs->alignment[1] = 16;
   vs->alignment[2] = 16;

   vs->plane_size 	= vs->stride_width * vs->stride_height;
   cedarv_setBufferInvalid(vs->dataY);
   cedarv_setBufferInvalid(vs->dataU);
   cedarv_setBufferInvalid(vs->dataV);
   
   switch (chroma_type)
   {
   case VDP_CHROMA_TYPE_444:
      //vs->data = cedarv_malloc(vs->plane_size * 3);
      vs->dataY = cedarv_malloc(vs->plane_size);
      vs->dataU = cedarv_malloc(vs->plane_size);
      vs->dataV = cedarv_malloc(vs->plane_size);
      if (! cedarv_isValid(vs->dataY) || ! cedarv_isValid(vs->dataU) || ! cedarv_isValid(vs->dataV))
      {
	  printf("vdpau video surface=%d create, failure\n", *surface);

	  handle_destroy(*surface);
          handle_release(device);
	  return VDP_STATUS_RESOURCES;
      }
      break;
   case VDP_CHROMA_TYPE_422:
      //vs->data = cedarv_malloc(vs->plane_size * 2);
      vs->dataY = cedarv_malloc(vs->plane_size);
      vs->dataU = cedarv_malloc(vs->plane_size/2);
      vs->dataV = cedarv_malloc(vs->plane_size/2);
      if (! cedarv_isValid(vs->dataY) || ! cedarv_isValid(vs->dataU) || ! cedarv_isValid(vs->dataV))
      {
	  printf("vdpau video surface=%d create, failure\n", *surface);

	  handle_destroy(*surface);
          handle_release(device);
	  return VDP_STATUS_RESOURCES;
      }
      break;
   case VDP_CHROMA_TYPE_420:
      //vs->data = cedarv_malloc(vs->plane_size + (vs->plane_size / 2));
      vs->dataY = cedarv_malloc(vs->plane_size);
      vs->dataU = cedarv_malloc(vs->plane_size/2);
      if (! cedarv_isValid(vs->dataY) || ! cedarv_isValid(vs->dataU))
      {
	  printf("vdpau video surface=%d create, failure\n", *surface);

	  handle_destroy(*surface);
          handle_release(device);
	  return VDP_STATUS_RESOURCES;
      }
      
      break;
   default:
      free(vs);
      handle_release(device);
      return VDP_STATUS_INVALID_CHROMA_TYPE;
   }
   handle_release(device);
   
   return VDP_STATUS_OK;
}

VdpStatus vdp_video_surface_destroy(VdpVideoSurface surface)
{
	video_surface_ctx_t *vs = handle_get(surface);
	if (!vs)
		return VDP_STATUS_INVALID_HANDLE;

	if (vs->decoder_private_free)
		vs->decoder_private_free(vs);
	if( cedarv_isValid(vs->dataY) )
	  cedarv_free(vs->dataY);
	if( cedarv_isValid(vs->dataU) )
	  cedarv_free(vs->dataU);
	if (cedarv_isValid(vs->dataV) )
	  cedarv_free(vs->dataV);

        cedarv_setBufferInvalid(vs->dataY);
        cedarv_setBufferInvalid(vs->dataU);
        cedarv_setBufferInvalid(vs->dataV);
        
        VDPAU_DBG("vdpau video surface=%d destroyed", surface);
        
        handle_release(surface);
        handle_destroy(surface);

	return VDP_STATUS_OK;
}

VdpStatus vdp_video_surface_get_parameters(VdpVideoSurface surface, VdpChromaType *chroma_type, uint32_t *width, uint32_t *height)
{
	video_surface_ctx_t *vid = handle_get(surface);
	if (!vid)
		return VDP_STATUS_INVALID_HANDLE;

	if (chroma_type)
		*chroma_type = vid->chroma_type;

	if (width)
		*width = vid->width;

	if (height)
		*height = vid->height;
        
        handle_release(surface);

	return VDP_STATUS_OK;
}

VdpStatus vdp_video_surface_get_bits_y_cb_cr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format, void *const *destination_data, uint32_t const *destination_pitches)
{
	video_surface_ctx_t *vs = handle_get(surface);
	if (!vs)
		return VDP_STATUS_INVALID_HANDLE;

        handle_release(surface);
	return VDP_STATUS_ERROR;
}

VdpStatus vdp_video_surface_put_bits_y_cb_cr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format, void const *const *source_data, uint32_t const *source_pitches)
{
	int i;
	const uint8_t *src;
        int status = VDP_STATUS_OK;
	size_t offset = 0;
	video_surface_ctx_t *vs = handle_get(surface);
	if (!vs)
		return VDP_STATUS_INVALID_HANDLE;

	vs->source_format = source_ycbcr_format;

	switch (source_ycbcr_format)
	{
	case VDP_YCBCR_FORMAT_YUYV:
	case VDP_YCBCR_FORMAT_UYVY:
		if (vs->chroma_type != VDP_CHROMA_TYPE_422) {
                   status = VDP_STATUS_INVALID_CHROMA_TYPE;
                   break;
                }
		src = source_data[0];
		for (i = 0; i < vs->height; i++) {
			cedarv_memcpy(vs->dataY, offset, src, 2*vs->width);
			src += source_pitches[0];
			offset += 2*vs->width;
		}
		break;
	case VDP_YCBCR_FORMAT_Y8U8V8A8:
	case VDP_YCBCR_FORMAT_V8U8Y8A8:
		break;

	case VDP_YCBCR_FORMAT_NV12:
		if (vs->chroma_type != VDP_CHROMA_TYPE_420) {
                   status = VDP_STATUS_INVALID_CHROMA_TYPE;
                   break;
                }
		src = source_data[0];
		for (i = 0; i < vs->height; i++) {
			cedarv_memcpy(vs->dataY, offset, src, vs->width);
			src += source_pitches[0];
			offset += vs->width;
		}
		src = source_data[1];
		offset = vs->plane_size;
		for (i = 0; i < vs->height / 2; i++) {
			cedarv_memcpy(vs->dataU, offset, src, vs->width);
			src += source_pitches[1];
			offset += vs->width;
		}
		break;

	case VDP_YCBCR_FORMAT_YV12:
		if (vs->chroma_type != VDP_CHROMA_TYPE_420) {
                   status = VDP_STATUS_INVALID_CHROMA_TYPE;
                   break;
                }
		src = source_data[0];
		for (i = 0; i < vs->height; i++) {
			cedarv_memcpy(vs->dataY, offset, src, vs->width);
			src += source_pitches[0];
			offset += vs->width;
		}
		src = source_data[2];
		offset = 0; //vs->plane_size;
		for (i = 0; i < vs->height / 2; i++) {
			cedarv_memcpy(vs->dataU, offset, src, vs->width / 2);
			src += source_pitches[1];
			offset += vs->width / 2;
		}
		src = source_data[1];
		offset = 0; //vs->plane_size + vs->plane_size / 4;
		for (i = 0; i < vs->height / 2; i++) {
			cedarv_memcpy(vs->dataV, offset, src, vs->width / 2);
			src += source_pitches[2];
			offset += vs->width / 2;
		}
		break;
	}

        handle_release(surface);
	return status;
}

VdpStatus vdp_video_surface_query_capabilities(VdpDevice device, VdpChromaType surface_chroma_type, VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
	if (!is_supported || !max_width || !max_height)
		return VDP_STATUS_INVALID_POINTER;

	device_ctx_t *dev = handle_get(device);
	if (!dev)
		return VDP_STATUS_INVALID_HANDLE;

	*is_supported = surface_chroma_type == VDP_CHROMA_TYPE_420;
	*max_width = 8192;
	*max_height = 8192;

        handle_release(device);
	return VDP_STATUS_OK;
}

VdpStatus vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities(VdpDevice device, VdpChromaType surface_chroma_type, VdpYCbCrFormat bits_ycbcr_format, VdpBool *is_supported)
{
	if (!is_supported)
		return VDP_STATUS_INVALID_POINTER;

	device_ctx_t *dev = handle_get(device);
	if (!dev)
		return VDP_STATUS_INVALID_HANDLE;

	*is_supported = VDP_FALSE;

        handle_release(device);
	return VDP_STATUS_OK;
}
