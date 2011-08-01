/*
  Copyright 1999-2011 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore cache view methods.
*/
#ifndef _MAGICKCORE_CACHE_VIEW_H
#define _MAGICKCORE_CACHE_VIEW_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "MagickCore/pixel.h"

typedef enum
{
  UndefinedVirtualPixelMethod,
  BackgroundVirtualPixelMethod,
  DitherVirtualPixelMethod,
  EdgeVirtualPixelMethod,
  MirrorVirtualPixelMethod,
  RandomVirtualPixelMethod,
  TileVirtualPixelMethod,
  TransparentVirtualPixelMethod,
  MaskVirtualPixelMethod,
  BlackVirtualPixelMethod,
  GrayVirtualPixelMethod,
  WhiteVirtualPixelMethod,
  HorizontalTileVirtualPixelMethod,
  VerticalTileVirtualPixelMethod,
  HorizontalTileEdgeVirtualPixelMethod,
  VerticalTileEdgeVirtualPixelMethod,
  CheckerTileVirtualPixelMethod
} VirtualPixelMethod;

typedef struct _CacheView
  CacheView;

extern MagickExport CacheView
  *AcquireCacheView(const Image *),
  *CloneCacheView(const CacheView *),
  *DestroyCacheView(CacheView *);

extern MagickExport ClassType
  GetCacheViewStorageClass(const CacheView *);

extern MagickExport ColorspaceType
  GetCacheViewColorspace(const CacheView *);

extern MagickExport const Quantum
  *GetCacheViewVirtualPixels(const CacheView *,const ssize_t,const ssize_t,
    const size_t,const size_t,ExceptionInfo *),
  *GetCacheViewVirtualPixelQueue(const CacheView *);

extern MagickExport const void
  *GetCacheViewVirtualMetacontent(const CacheView *);

extern MagickExport ExceptionInfo
  *GetCacheViewException(const CacheView *);

extern MagickExport MagickBooleanType
  GetOneCacheViewVirtualPixel(const CacheView *,const ssize_t,const ssize_t,
    PixelPacket *,ExceptionInfo *),
  GetOneCacheViewVirtualMethodPixel(const CacheView *,
    const VirtualPixelMethod,const ssize_t,const ssize_t,PixelPacket *,
    ExceptionInfo *),
  GetOneCacheViewAuthenticPixel(const CacheView *,const ssize_t,const ssize_t,
    PixelPacket *,ExceptionInfo *),
  SetCacheViewStorageClass(CacheView *,const ClassType),
  SetCacheViewVirtualPixelMethod(CacheView *,const VirtualPixelMethod),
  SyncCacheViewAuthenticPixels(CacheView *,ExceptionInfo *);

extern MagickExport MagickSizeType
  GetCacheViewExtent(const CacheView *);

extern MagickExport Quantum
  *GetCacheViewAuthenticPixelQueue(CacheView *),
  *GetCacheViewAuthenticPixels(CacheView *,const ssize_t,const ssize_t,
    const size_t,const size_t,ExceptionInfo *),
  *QueueCacheViewAuthenticPixels(CacheView *,const ssize_t,const ssize_t,
    const size_t,const size_t,ExceptionInfo *);

extern MagickExport void
  *GetCacheViewAuthenticMetacontent(CacheView *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif