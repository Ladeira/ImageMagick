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

  MagickWand command-line option process.
*/
#ifndef _MAGICKWAND_OPERATION_H
#define _MAGICKWAND_OPERATION_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WandExport void
  CLISettingOptionInfo(MagickCLI *,const char *,const char *, const char *),
  CLISimpleOperatorImages(MagickCLI *,const char *,const char *,const char *),
  CLIListOperatorImages(MagickCLI *, const char *,const char *,const char *),
  CLISpecialOperator(MagickCLI *,const char *,const char *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
