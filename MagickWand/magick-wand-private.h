/*
  Copyright 1999-2012 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  ImageMagick pixel wand API.
*/
#ifndef _MAGICKWAND_MAGICK_WAND_PRIVATE_H
#define _MAGICKWAND_MAGICK_WAND_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define DegreesToRadians(x)  (MagickPI*(x)/180.0)
#define MagickWandId  "MagickWand"
#define QuantumTick(i,span) ((MagickBooleanType) ((((i) & ((i)-1)) == 0) || \
   (((i) & 0xfff) == 0) || \
   ((MagickOffsetType) (i) == ((MagickOffsetType) (span)-1))))
#define RadiansToDegrees(x) (180.0*(x)/MagickPI)

struct _MagickWand
{
  size_t
    id;

  char
    name[MaxTextExtent];  /* Wand name to use for MagickWand Logs */

  Image
    *images;          /* The images in this wand - also the current image */

  ImageInfo
    *image_info;      /* Global settings used for images in Wand */

  ExceptionInfo
    *exception;

  MagickBooleanType
    insert_before,    /* wand set to first image, prepend new images */
    image_pending,    /* this image is pending Next/Previous Iteration */
    debug;            /* Log calls to MagickWand library */

  size_t
    signature;
};

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
