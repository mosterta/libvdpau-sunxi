/*
 * Copyright (c) 2015 Martin Ostertag <martin.ostertag@gmx.de>
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

#include "vdpau_private.h"
#include "ve.h"
#include <vdpau/vdpau_x11.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <assert.h>
#include "cedar_display.h"
#include "ve.h"
#include "veisp.h"
#include "libcedarDisplay.h"

#include <stdlib.h>

#include <sys/ioctl.h>
#include "sunxi_disp_ioctl.h"
#include <errno.h>
#include "vdpau_private.h"

static void (*Log)(int loglevel, const char *format, ...);

#define DEBUG_IMAGE_DATA 0

enum col_plane
{
  y_plane,
  u_plane,
  v_plane,
  uv_plane
};

void glVDPAUUnmapSurfacesCedar(GLsizei numSurfaces, const vdpauSurfaceCedar *surfaces);
void glVDPAUInitCedar(const void *vdpDevice, const void *getProcAddress, void (*_Log)(int loglevel, const char *format, ...));
void glVDPAUFiniCedar(void);
void glVDPAUCloseVideoLayerCedar(int hLayer, int dispFd);
vdpauSurfaceCedar glVDPAURegisterVideoSurfaceCedar (const void *vdpSurface);
vdpauSurfaceCedar glVDPAURegisterOutputSurfaceCedar (const void *vdpSurface);
int glVDPAUIsSurfaceCedar (vdpauSurfaceCedar surface);
void glVDPAUUnregisterSurfaceCedar (vdpauSurfaceCedar surface);
void glVDPAUMapSurfacesCedar(GLsizei numSurfaces, const vdpauSurfaceCedar *surfaces);
void glVDPAUUnmapSurfacesCedar(GLsizei numSurfaces, const vdpauSurfaceCedar *surfaces);

#if DEBUG_IMAGE_DATA == 1
static void writeBuffers(void* dataY, size_t szDataY, void* dataU, size_t szDataU, int h, int w);
#endif

void glVDPAUInitCedar(const void *vdpDevice, const void *getProcAddress,
                   void (*_Log)(int loglevel, const char *format, ...))
{
   Log = _Log;

  cedarv_disp_init();
}

void glVDPAUFiniCedar()
{
  cedarv_disp_close();
}

vdpauSurfaceCedar glVDPAURegisterVideoSurfaceCedar (const void *vdpSurface)
{
   vdpauSurfaceCedar surfaceNV;

   enum HandleType type = handle_get_type((VdpHandle)vdpSurface);
   assert(type == htype_video);

   video_surface_ctx_t *vs = (video_surface_ctx_t *)handle_get((VdpHandle)vdpSurface);
   assert(vs);

   assert(vs->chroma_type == VDP_CHROMA_TYPE_420);

   surface_display_ctx_t *nv = handle_create(sizeof(*nv), &surfaceNV, htype_display_vdpau);
   assert(nv);

   assert(vs->vdpNvState == VdpauNVState_Unregistered);

   vs->vdpNvState = VdpauNVState_Registered;

   nv->surface 		= (uint32_t)vdpSurface;
   nv->vdpNvState 	= VdpauNVState_Registered;

   nv->surfaceType = type;
 
   return surfaceNV;
}

vdpauSurfaceCedar glVDPAURegisterOutputSurfaceCedar (const void *vdpSurface)
{

  vdpauSurfaceCedar surfaceNV;

  enum HandleType type = handle_get_type((VdpHandle)vdpSurface);
  assert(type == htype_output);

  output_surface_ctx_t *vs = (output_surface_ctx_t *)handle_get((VdpHandle)vdpSurface);
  assert(vs);

  surface_display_ctx_t *nv = handle_create(sizeof(*nv), &surfaceNV, htype_display_vdpau);
  assert(nv);

  assert(vs->vdpNvState == VdpauNVState_Unregistered);

  vs->vdpNvState = VdpauNVState_Registered;

  nv->surface 		= (uint32_t)vdpSurface;
  nv->vdpNvState 	= VdpauNVState_Registered;

  nv->surfaceType = type;
 
  return surfaceNV;
}

int glVDPAUIsSurfaceCedar (vdpauSurfaceCedar surface)
{
  int status = VDP_STATUS_INVALID_HANDLE;
  if (handle_get_type(surface) == htype_display_vdpau)
    status = VDP_STATUS_OK;
  return status;
}

void glVDPAUUnregisterSurfaceCedar (vdpauSurfaceCedar surface)
{
   surface_display_ctx_t *nv  = handle_get(surface);
   if(!nv)
   {
      return VDP_STATUS_INVALID_HANDLE;
   }

   video_surface_ctx_t *vs = handle_get(nv->surface);
   if(!vs)
   {
      return VDP_STATUS_INVALID_HANDLE;
   }

   if(vs->vdpNvState == VdpauNVState_Mapped)
   {
      vdpauSurfaceCedar surf[] = {surface};
      glVDPAUUnmapSurfacesCedar(1, surf);
   }

   vs->vdpNvState = VdpauNVState_Unregistered;
   if(nv->surface)
   {
      handle_release(nv->surface);
      handle_destroy(nv->surface);
      //nv->surface = 0;
   }

   handle_release(surface);
   handle_destroy(surface); 
}

void glVDPAUGetSurfaceivCedar(vdpauSurfaceCedar surface, uint32_t pname, GLsizei bufSize,
			 GLsizei *length, int *values)
{
}

void glVDPAUSurfaceAccessCedar(vdpauSurfaceCedar surface, uint32_t access)
{
}

static void mapVideoTextures(GLsizei numSurfaces, const vdpauSurfaceCedar *surfaces)
{
  int j;
  for(j = 0; j < numSurfaces; j++)
  {
    surface_display_ctx_t *nv = handle_get(surfaces[j]);
    if(nv)
    {

       video_surface_ctx_t *vs = handle_get(nv->surface);
       if(vs)
          vs->vdpNvState = VdpauNVState_Mapped;
       handle_release(nv->surface);
       nv->vdpNvState = VdpauNVState_Mapped;
    }
    handle_release(surfaces[j]);
  }
}

static void mapOutputTextures(GLsizei numSurfaces, const vdpauSurfaceCedar *surfaces)
{
  int j;
  for(j = 0; j < numSurfaces; j++)
  {
    surface_display_ctx_t *nv = handle_get(surfaces[j]);
    if(nv)
    {
       output_surface_ctx_t *vs = handle_get(nv->surface);
       if(vs)
          vs->vdpNvState = VdpauNVState_Mapped;
       handle_release(nv->surface);
       nv->vdpNvState = VdpauNVState_Mapped;
    }
    handle_release(surfaces[j]);
  }
}

void glVDPAUMapSurfacesCedar(GLsizei numSurfaces, const vdpauSurfaceCedar *surfaces)
{
  surface_display_ctx_t *nv  = handle_get(surfaces[0]);
  enum HandleType surfaceType = nv->surfaceType;
  handle_release(surfaces[0]);
  
  if(surfaceType == htype_video)
    mapVideoTextures(numSurfaces, surfaces);
  else if(surfaceType == htype_output)
    mapOutputTextures(numSurfaces, surfaces);
}

void glVDPAUUnmapSurfacesCedar(GLsizei numSurfaces, const vdpauSurfaceCedar *surfaces)
{
  int j;
  
  for(j = 0; j < numSurfaces; j++)
  {
    surface_display_ctx_t *nv  = handle_get(surfaces[j]);
    if(nv)
    {
       video_surface_ctx_t *vs = handle_get(nv->surface);
       if(vs)
          vs->vdpNvState = VdpauNVState_Registered;
       handle_release(nv->surface);
       nv->vdpNvState = VdpauNVState_Registered;
    }
    handle_release(surfaces[j]);
  }
}

VdpStatus glVDPAUGetVideoFrameConfig(vdpauSurfaceCedar surface, struct videoFrameConfig *config)
{
  if(! config)
  {
    return VDP_STATUS_INVALID_HANDLE;
  }
  
  surface_display_ctx_t *nv  = handle_get(surface);
  if(! nv)
  {
    return VDP_STATUS_INVALID_HANDLE;
  }

  if(handle_get_type(nv->surface) != htype_video)
  {
    return VDP_STATUS_INVALID_HANDLE;
  }
  video_surface_ctx_t *vs = handle_get(nv->surface);
  if(! vs)
  {
    handle_release(surface);
    return VDP_STATUS_INVALID_HANDLE;
  }

  config->srcFormat = vs->source_format;
  config->addr[0] = (void*)cedarv_virt2phys(vs->dataY);
  config->addr[1] = (void*)cedarv_virt2phys(vs->dataU);
  config->align[0] = vs->alignment[0];
  config->align[1] = vs->alignment[1];
  if( cedarv_isValid(vs->dataV))
  {
    config->addr[2] = (void*)cedarv_virt2phys(vs->dataV);
    config->align[2] = vs->alignment[2];
  }
  else
  {
    config->addr[2] = NULL;
    config->align[2] = 0;
  }

  config->stride_height = vs->stride_height;
  config->stride_width = vs->stride_width;
  config->video_height = vs->stride_height;
  config->video_width = vs->stride_width;

#if DEBUG_IMAGE_DATA == 1
  static int first=1;
  static int frmNum=0;
  if(first && frmNum==3)
  {
     writeBuffers(cedarv_getPointer(vs->dataY),
                  cedarv_getSize(vs->dataY),
                  cedarv_getPointer(vs->dataU),
                  cedarv_getSize(vs->dataU),
                  config->height,
                  config->width);
     first = 0;
  }
  frmNum++;
#endif

  handle_release(nv->surface);
  handle_release(surface);
  return VDP_STATUS_OK;
}

VdpStatus glVDPAUCreateSurfaceCedar(VdpChromaType chroma_type, VdpYCbCrFormat format, uint32_t width, uint32_t height, uint8_t align[3], vdpauSurfaceCedar *surface)
{
  int alignment[3];
  int externalDecode = 0;
  int mem_width;
  int mem_height;

  if (!surface)
    return VDP_STATUS_INVALID_POINTER;

  if(*surface == 0xdeadbeaf)
    externalDecode = 1;

  if (!width || !height)
    return VDP_STATUS_INVALID_SIZE;

  video_surface_ctx_t *vs = handle_create(sizeof(*vs), surface, htype_video);
  if (!vs)
    return VDP_STATUS_RESOURCES;

  VDPAU_DBG("vdpau video surface=%d created", *surface);

  vs->width = width;
  vs->height = height;
  vs->chroma_type = chroma_type;
  vs->source_format = format;

  printf("version:%d, externalDecode:%d\n", cedarv_get_version(), externalDecode);

  if(align[0] == 0)
  {
    mem_width 	= (width + 63) & ~63;
    mem_height 	= (height + 63) & ~63;
    if(cedarv_get_version() < 0x1680)
    {
        vs->source_format = INTERNAL_YCBCR_FORMAT;
    }
    else
    {
        vs->source_format = VDP_YCBCR_FORMAT_NV12;
    }
    alignment[0] = 16;
    alignment[1] = 16;
    alignment[2] = 16;
  }
  else
  {
    mem_width   = (width + (align[0]-1)) & ~(align[0]-1);
    mem_height  = (height);
    alignment[0] = align[0];
    alignment[1] = align[1];
    alignment[2] = align[2];
  }
  for(int i=0; i<3; i++)
    vs->alignment[i] = alignment[i];
  vs->stride_width      = (width + (alignment[0]-1)) & ~(alignment[0]-1);
  vs->stride_height     = (height);

  vs->plane_size 	= mem_width * mem_height;
  cedarv_setBufferInvalid(vs->dataY);
  cedarv_setBufferInvalid(vs->dataU);
  cedarv_setBufferInvalid(vs->dataV);

  switch (format)
  {
    case VDP_YCBCR_FORMAT_NV12:
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
            return VDP_STATUS_RESOURCES;
          }

          break;
        default:
          free(vs);
          return VDP_STATUS_INVALID_CHROMA_TYPE;
      }
      break;
    case VDP_YCBCR_FORMAT_YV12:
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
            return VDP_STATUS_RESOURCES;
          }
          break;
        case VDP_CHROMA_TYPE_420:
              //vs->data = cedarv_malloc(vs->plane_size + (vs->plane_size / 2));
          vs->dataY = cedarv_malloc(vs->plane_size);
          vs->dataU = cedarv_malloc(vs->plane_size/2);
          vs->dataV = cedarv_malloc(vs->plane_size/2);
          if (! cedarv_isValid(vs->dataY) || ! cedarv_isValid(vs->dataU) || ! cedarv_isValid(vs->dataV))
          {
            printf("vdpau video surface=%d create, failure\n", *surface);

            handle_destroy(*surface);
            return VDP_STATUS_RESOURCES;
          }

          break;
        default:
          free(vs);
          return VDP_STATUS_INVALID_CHROMA_TYPE;
      }
  }
  return VDP_STATUS_OK;
}

VdpStatus glVDPAUDestroySurfaceCedar(vdpauSurfaceCedar surface)
{
  video_surface_ctx_t *vs = handle_get(surface);
  if (!vs)
    return VDP_STATUS_INVALID_HANDLE;

  assert(vs->vdpNvState == VdpauNVState_Unregistered);

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


VdpStatus glVDPAUGetMappedMemoryCedar(vdpauSurfaceCedar surface, void** addrY, void** addrU, void** addrV)
{
  vdpauSurfaceCedar videoSurface = surface;
  surface_display_ctx_t *nv = NULL;
  
  if(handle_get_type(surface) == htype_display_vdpau)
  {
    nv  = handle_get(surface);
    if(! nv)
    {
      return VDP_STATUS_INVALID_HANDLE;
    }
    videoSurface = nv->surface;
  }

  assert(handle_get_type(surface) == htype_video);

  video_surface_ctx_t *vs = handle_get(videoSurface);
  if(! vs)
  {
    handle_release(surface);
    return VDP_STATUS_INVALID_HANDLE;
  }

  *addrY = (void*)cedarv_getPointer(vs->dataY);
  *addrU = (void*)cedarv_getPointer(vs->dataU);
  if( cedarv_isValid(vs->dataV))
    *addrV = (void*)cedarv_getPointer(vs->dataV);
  else
    *addrV = NULL;

  if(nv)
    handle_release(nv->surface);
  handle_release(surface);
  return VDP_STATUS_OK;
}

#if DEBUG_IMAGE_DATA == 1
static void writeBuffers(void* dataY, size_t szDataY, void* dataU, size_t szDataU, int h, int w)
{
  FILE *file = fopen("/tmp/dataY.bin", "wb");
  fwrite(dataY, szDataY, 1, file);
  fclose(file);
  file = fopen("/tmp/dataU.bin", "wb");
  fwrite(dataU, szDataU, 1, file);
  fclose(file);
  file = fopen("/tmp/dataSz.txt", "w");
  fprintf(file, "width=%d height=%d\n", w, h);
  fclose(file);
}
#endif
