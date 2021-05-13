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

#pragma once

#define CS_MODE_BT709   1
#define CS_MODE_BT601   2
#define CS_MODE_YCC     3
#define CS_MODE_XVYCC   4

#ifdef __cplusplus
extern "C"
{
#endif

  struct videoFrameConfig
  {
    uint16_t  stride_width;
    uint16_t  stride_height;
    uint16_t  video_width;
    uint16_t  video_height;
    void      *addr[3];
    uint8_t   align[3];
    uint8_t   srcFormat;
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
  VdpStatus glVDPAUGetVideoFrameConfig(vdpauSurfaceCedar surface, struct videoFrameConfig *config);
  VdpStatus glVDPAUCreateSurfaceCedar(VdpChromaType chroma_type, VdpYCbCrFormat format, uint32_t width, uint32_t height, uint8_t align[3], vdpauSurfaceCedar *surface);
  VdpStatus glVDPAUDestroySurfaceCedar(vdpauSurfaceCedar surface);
#ifdef __cplusplus
}
#endif



