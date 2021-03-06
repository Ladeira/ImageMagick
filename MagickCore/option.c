/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   OOO   PPPP   TTTTT  IIIII   OOO   N   N                   %
%                  O   O  P   P    T      I    O   O  NN  N                   %
%                  O   O  PPPP     T      I    O   O  N N N                   %
%                  O   O  P        T      I    O   O  N  NN                   %
%                   OOO   P        T    IIIII   OOO   N   N                   %
%                                                                             %
%                                                                             %
%                         MagickCore Option Methods                           %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2000                                  %
%                                                                             %
%                                                                             %
%  Copyright 1999-2012 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/color.h"
#include "MagickCore/compare.h"
#include "MagickCore/constitute.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/layer.h"
#include "MagickCore/mime-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resample.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"

/*
  ImageMagick options.
*/
static const OptionInfo
  AlignOptions[] =
  {
    { "Undefined", UndefinedAlign, UndefinedOptionFlag, MagickTrue },
    { "Center", CenterAlign, UndefinedOptionFlag, MagickFalse },
    { "End", RightAlign, UndefinedOptionFlag, MagickFalse },
    { "Left", LeftAlign, UndefinedOptionFlag, MagickFalse },
    { "Middle", CenterAlign, UndefinedOptionFlag, MagickFalse },
    { "Right", RightAlign, UndefinedOptionFlag, MagickFalse },
    { "Start", LeftAlign, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedAlign, UndefinedOptionFlag, MagickFalse }
  },
  AlphaOptions[] =
  {
    { "Undefined", UndefinedAlphaChannel, UndefinedOptionFlag, MagickTrue },
    { "Activate", ActivateAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Background", BackgroundAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Copy", CopyAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Deactivate", DeactivateAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Extract", ExtractAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Off", DeactivateAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "On", ActivateAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Opaque", OpaqueAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Remove", RemoveAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Set", SetAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Shape", ShapeAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Reset", SetAlphaChannel, DeprecateOptionFlag, MagickTrue },
    { "Transparent", TransparentAlphaChannel, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedAlphaChannel, UndefinedOptionFlag, MagickFalse }
  },
  BooleanOptions[] =
  {
    { "False", MagickFalse, UndefinedOptionFlag, MagickFalse },
    { "True", MagickTrue, UndefinedOptionFlag, MagickFalse },
    { "0", MagickFalse, UndefinedOptionFlag, MagickFalse },
    { "1", MagickTrue, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, MagickFalse, UndefinedOptionFlag, MagickFalse }
  },
  ChannelOptions[] =
  {
    { "Undefined", UndefinedChannel, UndefinedOptionFlag, MagickTrue },
    /* special */
    { "All", CompositeChannels, UndefinedOptionFlag, MagickFalse },
    { "Sync", SyncChannels, UndefinedOptionFlag, MagickFalse },
    { "Default", DefaultChannels, UndefinedOptionFlag, MagickFalse },
    /* individual channel */
    { "A", AlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Alpha", AlphaChannel, UndefinedOptionFlag, MagickFalse },
    { "Black", BlackChannel, UndefinedOptionFlag, MagickFalse },
    { "B", BlueChannel, UndefinedOptionFlag, MagickFalse },
    { "Blue", BlueChannel, UndefinedOptionFlag, MagickFalse },
    { "C", CyanChannel, UndefinedOptionFlag, MagickFalse },
    { "Cyan", CyanChannel, UndefinedOptionFlag, MagickFalse },
    { "Gray", GrayChannel, UndefinedOptionFlag, MagickFalse },
    { "G", GreenChannel, UndefinedOptionFlag, MagickFalse },
    { "Green", GreenChannel, UndefinedOptionFlag, MagickFalse },
    { "H", RedChannel, UndefinedOptionFlag, MagickFalse },
    { "Hue", RedChannel, UndefinedOptionFlag, MagickFalse },
    { "K", BlackChannel, UndefinedOptionFlag, MagickFalse },
    { "L", BlueChannel, UndefinedOptionFlag, MagickFalse },
    { "Lightness", BlueChannel, UndefinedOptionFlag, MagickFalse },
    { "Luminance", BlueChannel, UndefinedOptionFlag, MagickFalse },
    { "Luminosity", BlueChannel, DeprecateOptionFlag, MagickTrue },
    { "M", MagentaChannel, UndefinedOptionFlag, MagickFalse },
    { "Magenta", MagentaChannel, UndefinedOptionFlag, MagickFalse },
    { "Matte", AlphaChannel, DeprecateOptionFlag, MagickTrue },/*depreciate*/
    { "Opacity", AlphaChannel, DeprecateOptionFlag, MagickTrue },/*depreciate*/
    { "R", RedChannel, UndefinedOptionFlag, MagickFalse },
    { "Red", RedChannel, UndefinedOptionFlag, MagickFalse },
    { "S", GreenChannel, UndefinedOptionFlag, MagickFalse },
    { "Saturation", GreenChannel, UndefinedOptionFlag, MagickFalse },
    { "Y", YellowChannel, UndefinedOptionFlag, MagickFalse },
    { "Yellow", YellowChannel, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedChannel, UndefinedOptionFlag, MagickFalse }
  },
  ClassOptions[] =
  {
    { "Undefined", UndefinedClass, UndefinedOptionFlag, MagickTrue },
    { "DirectClass", DirectClass, UndefinedOptionFlag, MagickFalse },
    { "PseudoClass", PseudoClass, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedClass, UndefinedOptionFlag, MagickFalse }
  },
  ClipPathOptions[] =
  {
    { "Undefined", UndefinedPathUnits, UndefinedOptionFlag, MagickTrue },
    { "ObjectBoundingBox", ObjectBoundingBox, UndefinedOptionFlag, MagickFalse },
    { "UserSpace", UserSpace, UndefinedOptionFlag, MagickFalse },
    { "UserSpaceOnUse", UserSpaceOnUse, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedPathUnits, UndefinedOptionFlag, MagickFalse }
  },
  CommandOptions[] =
  {
    /* WARNING: this must be sorted by name, then by switch character
       So that it can be referenced using a binary search for speed.
       See GetCommandOptionInfo() below for details.

       Check on sort...
           magick -list command > t1
           sort -k 1.2  t1 | diff t1 -
       Should not show any differences...
    */
    { "(", 0L, SpecialOptionFlag, MagickTrue },
    { ")", 0L, SpecialOptionFlag, MagickTrue },
    { "{", 0L, SpecialOptionFlag, MagickTrue },
    { "}", 0L, SpecialOptionFlag, MagickTrue },
    { "--", 1L, SpecialOptionFlag, MagickTrue },
    { "+adaptive-blur", 1L, DeprecateOptionFlag, MagickTrue },
    { "-adaptive-blur", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+adaptive-resize", 1L, DeprecateOptionFlag, MagickTrue },
    { "-adaptive-resize", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+adaptive-sharpen", 1L, DeprecateOptionFlag, MagickTrue },
    { "-adaptive-sharpen", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+adjoin", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-adjoin", 0L, ImageInfoOptionFlag, MagickFalse },
    { "+affine", 0L, DeprecateOptionFlag | DrawInfoOptionFlag, MagickTrue },
    { "-affine", 1L, DeprecateOptionFlag | DrawInfoOptionFlag, MagickTrue },
    { "+affinity", 0L, DeprecateOptionFlag, MagickTrue },
    { "-affinity", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "+alpha", 1L, DeprecateOptionFlag, MagickTrue },
    { "-alpha", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+annotate", 0L, DeprecateOptionFlag, MagickTrue },
    { "-annotate", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+antialias", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-antialias", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+append", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-append", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+attenuate", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-attenuate", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+authenticate", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-authenticate", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+auto-gamma", 0L, DeprecateOptionFlag, MagickTrue },
    { "-auto-gamma", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+auto-level", 0L, DeprecateOptionFlag, MagickTrue },
    { "-auto-level", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+auto-orient", 0L, DeprecateOptionFlag, MagickTrue },
    { "-auto-orient", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+average", 0L, DeprecateOptionFlag | ListOperatorOptionFlag | FireOptionFlag, MagickTrue },
    { "-average", 0L, DeprecateOptionFlag | ListOperatorOptionFlag | FireOptionFlag, MagickTrue },
    { "+backdrop", 0L, NonMagickOptionFlag, MagickFalse },
    { "-backdrop", 1L, NonMagickOptionFlag, MagickFalse },
    { "+background", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-background", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+bench", 1L, DeprecateOptionFlag, MagickTrue },
    { "-bench", 1L, GenesisOptionFlag, MagickFalse },
    { "+bias", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-bias", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+black-point-compensation", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-black-point-compensation", 0L, ImageInfoOptionFlag, MagickFalse },
    { "+black-threshold", 0L, DeprecateOptionFlag, MagickTrue },
    { "-black-threshold", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+blend", 0L, NonMagickOptionFlag, MagickFalse },
    { "-blend", 1L, NonMagickOptionFlag, MagickFalse },
    { "+blue-primary", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-blue-primary", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+blue-shift", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-blue-shift", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+blur", 0L, DeprecateOptionFlag, MagickTrue },
    { "-blur", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+border", 1L, DeprecateOptionFlag, MagickTrue },
    { "-border", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+bordercolor", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-bordercolor", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+borderwidth", 0L, NonMagickOptionFlag, MagickFalse },
    { "-borderwidth", 1L, NonMagickOptionFlag, MagickFalse },
    { "+box", 0L, DeprecateOptionFlag | ImageInfoOptionFlag | DrawInfoOptionFlag, MagickTrue },
    { "-box", 1L, DeprecateOptionFlag | ImageInfoOptionFlag | DrawInfoOptionFlag, MagickTrue },
    { "+brightness-contrast", 0L, DeprecateOptionFlag, MagickTrue },
    { "-brightness-contrast", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+cache", 0L, GlobalOptionFlag, MagickFalse },
    { "-cache", 1L, GlobalOptionFlag, MagickFalse },
    { "+caption", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-caption", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+cdl", 1L, DeprecateOptionFlag, MagickTrue },
    { "-cdl", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+channel", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-channel", 1L, ImageInfoOptionFlag, MagickFalse },
    { "-channel-fx", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+charcoal", 0L, DeprecateOptionFlag, MagickTrue },
    { "-charcoal", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+chop", 1L, DeprecateOptionFlag, MagickTrue },
    { "-chop", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+clamp", 0L, DeprecateOptionFlag, MagickTrue },
    { "-clamp", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+clip", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-clip", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+clip-mask", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-clip-mask", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+clip-path", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-clip-path", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+clone", 0L, SpecialOptionFlag, MagickFalse },
    { "-clone", 1L, SpecialOptionFlag, MagickFalse },
    { "+clut", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-clut", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+coalesce", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-coalesce", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+color-matrix", 1L, DeprecateOptionFlag, MagickTrue },
    { "-color-matrix", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+colorize", 1L, DeprecateOptionFlag, MagickTrue },
    { "-colorize", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+colormap", 0L, NonMagickOptionFlag, MagickFalse },
    { "-colormap", 1L, NonMagickOptionFlag, MagickFalse },
    { "+colors", 1L, DeprecateOptionFlag, MagickTrue },
    { "-colors", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+colorspace", 0L, ImageInfoOptionFlag | SimpleOperatorOptionFlag, MagickFalse },
    { "-colorspace", 1L, ImageInfoOptionFlag | SimpleOperatorOptionFlag, MagickFalse },
    { "+combine", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-combine", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+comment", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-comment", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+compose", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-compose", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+composite", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-composite", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+compress", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-compress", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+concurrent", 0L, DeprecateOptionFlag, MagickTrue },
    { "-concurrent", 0L, GenesisOptionFlag, MagickFalse },
    { "+contrast", 0L, DeprecateOptionFlag, MagickTrue },
    { "-contrast", 0L, DeprecateOptionFlag, MagickTrue },
    { "+contrast-stretch", 1L, DeprecateOptionFlag, MagickTrue },
    { "-contrast-stretch", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+convolve", 1L, DeprecateOptionFlag, MagickTrue },
    { "-convolve", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+crop", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-crop", 1L, SimpleOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+cycle", 1L, DeprecateOptionFlag, MagickTrue },
    { "-cycle", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+debug", 0L, GlobalOptionFlag | FireOptionFlag, MagickFalse },
    { "-debug", 1L, GlobalOptionFlag | FireOptionFlag, MagickFalse },
    { "+decipher", 1L, DeprecateOptionFlag, MagickTrue },
    { "-decipher", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+deconstruct", 0L, DeprecateOptionFlag, MagickTrue },
    { "-deconstruct", 0L, DeprecateOptionFlag | ListOperatorOptionFlag | FireOptionFlag, MagickTrue },
    { "+define", 1L, ImageInfoOptionFlag, MagickFalse },
    { "-define", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+delay", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-delay", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+delete", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-delete", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+density", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-density", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+depth", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-depth", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+descend", 0L, NonMagickOptionFlag, MagickFalse },
    { "-descend", 1L, NonMagickOptionFlag, MagickFalse },
    { "+deskew", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-deskew", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+despeckle", 0L, DeprecateOptionFlag, MagickTrue },
    { "-despeckle", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+direction", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-direction", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+displace", 0L, NonMagickOptionFlag, MagickFalse },
    { "-displace", 1L, NonMagickOptionFlag, MagickFalse },
    { "+display", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-display", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+dispose", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-dispose", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+dissolve", 0L, NonMagickOptionFlag, MagickFalse },
    { "-dissolve", 1L, NonMagickOptionFlag, MagickFalse },
    { "+distort", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "-distort", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+dither", 0L, ImageInfoOptionFlag | QuantizeInfoOptionFlag, MagickFalse },
    { "-dither", 1L, ImageInfoOptionFlag | QuantizeInfoOptionFlag, MagickFalse },
    { "+draw", 0L, DeprecateOptionFlag, MagickTrue },
    { "-draw", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+duplicate", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-duplicate", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+duration", 1L, GenesisOptionFlag, MagickFalse },
    { "-duration", 1L, GenesisOptionFlag, MagickFalse },
    { "+edge", 1L, DeprecateOptionFlag, MagickTrue },
    { "-edge", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+emboss", 1L, DeprecateOptionFlag, MagickTrue },
    { "-emboss", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+encipher", 1L, DeprecateOptionFlag, MagickTrue },
    { "-encipher", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+encoding", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-encoding", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+endian", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-endian", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+enhance", 0L, DeprecateOptionFlag, MagickTrue },
    { "-enhance", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+equalize", 0L, DeprecateOptionFlag, MagickTrue },
    { "-equalize", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+evaluate", 2L, DeprecateOptionFlag, MagickTrue },
    { "-evaluate", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+evaluate-sequence", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-evaluate-sequence", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-exit", 0L, SpecialOptionFlag, MagickFalse },
    { "+extent", 1L, DeprecateOptionFlag, MagickTrue },
    { "-extent", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+extract", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-extract", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+family", 0L, DeprecateOptionFlag, MagickTrue },
    { "-family", 1L, DrawInfoOptionFlag, MagickFalse },
    { "+features", 0L, SimpleOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-features", 1L, SimpleOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+fft", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-fft", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+fill", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-fill", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+filter", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-filter", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+flatten", 0L, DeprecateOptionFlag, MagickTrue },
    { "-flatten", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+flip", 0L, DeprecateOptionFlag, MagickTrue },
    { "-flip", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+floodfill", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "-floodfill", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+flop", 0L, DeprecateOptionFlag, MagickTrue },
    { "-flop", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+font", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-font", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+foreground", 0L, NonMagickOptionFlag, MagickFalse },
    { "-foreground", 1L, NonMagickOptionFlag, MagickFalse },
    { "+format", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-format", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+frame", 1L, DeprecateOptionFlag, MagickTrue },
    { "-frame", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+function", 2L, DeprecateOptionFlag, MagickTrue },
    { "-function", 2L,SimpleOperatorOptionFlag, MagickFalse },
    { "+fuzz", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-fuzz", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+fx", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-fx", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+gamma", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-gamma", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+gaussian", 1L, DeprecateOptionFlag, MagickTrue },
    { "-gaussian", 1L, DeprecateOptionFlag | SimpleOperatorOptionFlag, MagickTrue },
    { "+gaussian-blur", 1L, DeprecateOptionFlag, MagickTrue },
    { "-gaussian-blur", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+geometry", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-geometry", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+gravity", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-gravity", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+green-primary", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-green-primary", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+hald-clut", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-hald-clut", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+highlight-color", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-highlight-color", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+iconGeometry", 0L, NonMagickOptionFlag, MagickFalse },
    { "-iconGeometry", 1L, NonMagickOptionFlag, MagickFalse },
    { "+iconic", 0L, NonMagickOptionFlag, MagickFalse },
    { "-iconic", 1L, NonMagickOptionFlag, MagickFalse },
    { "+identify", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-identify", 0L, SimpleOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+ift", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-ift", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+immutable", 0L, NonMagickOptionFlag, MagickFalse },
    { "-immutable", 0L, NonMagickOptionFlag, MagickFalse },
    { "+implode", 0L, DeprecateOptionFlag, MagickTrue },
    { "-implode", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+insert", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-insert", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+intent", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-intent", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+interlace", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-interlace", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+interline-spacing", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-interline-spacing", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+interpolate", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-interpolate", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+interpolative-resize", 1L, DeprecateOptionFlag, MagickTrue },
    { "-interpolative-resize", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+interword-spacing", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-interword-spacing", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+kerning", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-kerning", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+label", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-label", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+lat", 1L, DeprecateOptionFlag, MagickTrue },
    { "-lat", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+layers", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-layers", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+level", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-level", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+level-colors", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-level-colors", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+limit", 0L, DeprecateOptionFlag, MagickTrue },
    { "-limit", 2L, GlobalOptionFlag | FireOptionFlag, MagickFalse },
    { "+linear-stretch", 1L, DeprecateOptionFlag, MagickTrue },
    { "-linear-stretch", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+liquid-rescale", 1L, DeprecateOptionFlag, MagickTrue },
    { "-liquid-rescale", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+list", 0L, DeprecateOptionFlag, MagickTrue },
    { "-list", 1L, SpecialOptionFlag, MagickFalse },
    { "+log", 0L, DeprecateOptionFlag, MagickFalse },
    { "-log", 1L, GlobalOptionFlag, MagickFalse },
    { "+loop", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-loop", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+lowlight-color", 1L, DeprecateOptionFlag, MagickTrue },
    { "-lowlight-color", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+magnify", 0L, NonMagickOptionFlag, MagickFalse },
    { "-magnify", 1L, NonMagickOptionFlag, MagickFalse },
    { "+map", 0L, DeprecateOptionFlag | ListOperatorOptionFlag | FireOptionFlag, MagickTrue },
    { "-map", 1L, DeprecateOptionFlag | SimpleOperatorOptionFlag, MagickTrue },
    { "+mask", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-mask", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+matte", 0L, DeprecateOptionFlag | SimpleOperatorOptionFlag, MagickTrue },
    { "-matte", 0L, DeprecateOptionFlag | SimpleOperatorOptionFlag, MagickTrue },
    { "+mattecolor", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-mattecolor", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+maximum", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-maximum", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "+median", 1L, DeprecateOptionFlag, MagickTrue },
    { "-median", 1L, DeprecateOptionFlag | SimpleOperatorOptionFlag | FireOptionFlag, MagickTrue },
    { "+metric", 0L, NonMagickOptionFlag, MagickFalse },
    { "-metric", 1L, NonMagickOptionFlag, MagickFalse },
    { "+minimum", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-minimum", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "+mode", 1L, NonMagickOptionFlag, MagickFalse },
    { "-mode", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+modulate", 1L, DeprecateOptionFlag, MagickTrue },
    { "-modulate", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+monitor", 0L, ImageInfoOptionFlag | SimpleOperatorOptionFlag, MagickFalse },
    { "-monitor", 0L, ImageInfoOptionFlag | SimpleOperatorOptionFlag, MagickFalse },
    { "+monochrome", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-monochrome", 0L, ImageInfoOptionFlag | SimpleOperatorOptionFlag, MagickFalse },
    { "+morph", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-morph", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+morphology", 2L, DeprecateOptionFlag, MagickTrue },
    { "-morphology", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+mosaic", 0L, DeprecateOptionFlag, MagickTrue },
    { "-mosaic", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+motion-blur", 1L, DeprecateOptionFlag, MagickTrue },
    { "-motion-blur", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+name", 0L, NonMagickOptionFlag, MagickFalse },
    { "-name", 1L, NonMagickOptionFlag, MagickFalse },
    { "+negate", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-negate", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+noise", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-noise", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-noop", 0L, SpecialOptionFlag, MagickFalse },
    { "+normalize", 0L, DeprecateOptionFlag, MagickTrue },
    { "-normalize", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+opaque", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-opaque", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+ordered-dither", 0L, DeprecateOptionFlag, MagickTrue },
    { "-ordered-dither", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+orient", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-orient", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+origin", 0L, DeprecateOptionFlag, MagickTrue },
    { "-origin", 1L, DeprecateOptionFlag, MagickTrue },
    { "+page", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-page", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+paint", 0L, DeprecateOptionFlag, MagickTrue },
    { "-paint", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+path", 0L, NonMagickOptionFlag, MagickFalse },
    { "-path", 1L, NonMagickOptionFlag, MagickFalse },
    { "+pause", 0L, NonMagickOptionFlag, MagickFalse },
    { "-pause", 1L, NonMagickOptionFlag, MagickFalse },
    { "+ping", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-ping", 0L, ImageInfoOptionFlag, MagickFalse },
    { "+pointsize", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-pointsize", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+polaroid", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-polaroid", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+posterize", 1L, DeprecateOptionFlag, MagickTrue },
    { "-posterize", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+precision", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-precision", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+preview", 0L, DeprecateOptionFlag, MagickTrue },
    { "-preview", 1L, GlobalOptionFlag, MagickFalse },
    { "+print", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-print", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+process", 1L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-process", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+profile", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-profile", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+quality", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-quality", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+quantize", 0L, QuantizeInfoOptionFlag, MagickFalse },
    { "-quantize", 1L, QuantizeInfoOptionFlag, MagickFalse },
    { "+quiet", 0L, GlobalOptionFlag | FireOptionFlag, MagickFalse },
    { "-quiet", 0L, GlobalOptionFlag | FireOptionFlag, MagickFalse },
    { "+radial-blur", 1L, DeprecateOptionFlag, MagickTrue },
    { "-radial-blur", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+raise", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-raise", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+random-threshold", 1L, DeprecateOptionFlag, MagickTrue },
    { "-random-threshold", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-read", 1L, SpecialOptionFlag, MagickFalse },
    { "+recolor", 1L, DeprecateOptionFlag, MagickTrue },
    { "-recolor", 1L, DeprecateOptionFlag, MagickTrue },
    { "+red-primary", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-red-primary", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+regard-warnings", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-regard-warnings", 0L, ImageInfoOptionFlag, MagickFalse },
    { "+region", 0L, SpecialOptionFlag, MagickFalse },
    { "-region", 1L, SpecialOptionFlag, MagickFalse },
    { "+remap", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-remap", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+remote", 0L, NonMagickOptionFlag, MagickFalse },
    { "-remote", 1L, NonMagickOptionFlag, MagickFalse },
    { "+render", 0L, DrawInfoOptionFlag, MagickFalse },
    { "-render", 0L, DrawInfoOptionFlag, MagickFalse },
    { "+repage", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-repage", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+resample", 1L, DeprecateOptionFlag, MagickTrue },
    { "-resample", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+resize", 1L, DeprecateOptionFlag, MagickTrue },
    { "-resize", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+respect-parenthesis", 0L, SpecialOptionFlag, MagickFalse },
    { "-respect-parenthesis", 0L, SpecialOptionFlag, MagickFalse },
    { "+reverse", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-reverse", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+roll", 1L, DeprecateOptionFlag, MagickTrue },
    { "-roll", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+rotate", 1L, DeprecateOptionFlag, MagickTrue },
    { "-rotate", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+sample", 1L, DeprecateOptionFlag, MagickTrue },
    { "-sample", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+sampling-factor", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-sampling-factor", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+sans", 1L, SpecialOptionFlag, MagickTrue },
    { "-sans", 1L, SpecialOptionFlag, MagickTrue },
    { "+sans0", 0L, SpecialOptionFlag, MagickTrue }, /* equivelent to 'noop' */
    { "-sans0", 0L, SpecialOptionFlag, MagickTrue },
    { "+sans2", 2L, SpecialOptionFlag, MagickTrue },
    { "-sans2", 2L, SpecialOptionFlag, MagickTrue },
    { "+scale", 1L, DeprecateOptionFlag, MagickTrue },
    { "-scale", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+scene", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-scene", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+scenes", 0L, NonMagickOptionFlag, MagickFalse },
    { "-scenes", 1L, NonMagickOptionFlag, MagickFalse },
    { "+screen", 0L, NonMagickOptionFlag, MagickFalse },
    { "-screen", 1L, NonMagickOptionFlag, MagickFalse },
    { "-script", 1L, UndefinedOptionFlag, MagickFalse }, /* special handling */
    { "+seed", 0L, GlobalOptionFlag, MagickFalse },
    { "-seed", 1L, GlobalOptionFlag, MagickFalse },
    { "+segment", 1L, DeprecateOptionFlag, MagickTrue },
    { "-segment", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+selective-blur", 1L, DeprecateOptionFlag, MagickTrue },
    { "-selective-blur", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+separate", 0L, DeprecateOptionFlag | FireOptionFlag, MagickTrue },
    { "-separate", 0L, SimpleOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+sepia-tone", 1L, DeprecateOptionFlag, MagickTrue },
    { "-sepia-tone", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+set", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-set", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+shade", 0L, DeprecateOptionFlag, MagickTrue },
    { "-shade", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+shadow", 1L, DeprecateOptionFlag, MagickTrue },
    { "-shadow", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+shared-memory", 0L, NonMagickOptionFlag, MagickFalse },
    { "-shared-memory", 1L, NonMagickOptionFlag, MagickFalse },
    { "+sharpen", 1L, DeprecateOptionFlag, MagickTrue },
    { "-sharpen", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+shave", 1L, DeprecateOptionFlag, MagickTrue },
    { "-shave", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+shear", 1L, DeprecateOptionFlag, MagickTrue },
    { "-shear", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+sigmoidal-contrast", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-sigmoidal-contrast", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+silent", 0L, NonMagickOptionFlag, MagickFalse },
    { "-silent", 1L, NonMagickOptionFlag, MagickFalse },
    { "+size", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-size", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+sketch", 1L, DeprecateOptionFlag, MagickTrue },
    { "-sketch", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+smush", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-smush", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+snaps", 0L, NonMagickOptionFlag, MagickFalse },
    { "-snaps", 1L, NonMagickOptionFlag, MagickFalse },
    { "+solarize", 1L, DeprecateOptionFlag, MagickTrue },
    { "-solarize", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+sparse-color", 2L, DeprecateOptionFlag, MagickTrue },
    { "-sparse-color", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+splice", 1L, DeprecateOptionFlag, MagickTrue },
    { "-splice", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+spread", 1L, DeprecateOptionFlag, MagickTrue },
    { "-spread", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+statistic", 2L, DeprecateOptionFlag, MagickTrue },
    { "-statistic", 2L, SimpleOperatorOptionFlag, MagickFalse },
    { "+stegano", 0L, NonMagickOptionFlag, MagickFalse },
    { "-stegano", 1L, NonMagickOptionFlag, MagickFalse },
    { "+stereo", 0L, DeprecateOptionFlag, MagickTrue },
    { "-stereo", 1L, NonMagickOptionFlag, MagickFalse },
    { "+stretch", 1L, DeprecateOptionFlag, MagickTrue },
    { "-stretch", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+strip", 0L, DeprecateOptionFlag, MagickTrue },
    { "-strip", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+stroke", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-stroke", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+strokewidth", 1L, ImageInfoOptionFlag, MagickFalse },
    { "-strokewidth", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+style", 0L, DrawInfoOptionFlag, MagickFalse },
    { "-style", 1L, DrawInfoOptionFlag, MagickFalse },
    { "+subimage-search", 0L, NonMagickOptionFlag, MagickFalse },
    { "-subimage-search", 0L, NonMagickOptionFlag, MagickFalse },
    { "+swap", 0L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "-swap", 1L, ListOperatorOptionFlag | FireOptionFlag, MagickFalse },
    { "+swirl", 1L, DeprecateOptionFlag, MagickTrue },
    { "-swirl", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+synchronize", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-synchronize", 0L, ImageInfoOptionFlag, MagickFalse },
    { "+taint", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-taint", 0L, ImageInfoOptionFlag, MagickFalse },
    { "+text-font", 0L, NonMagickOptionFlag, MagickFalse },
    { "-text-font", 1L, NonMagickOptionFlag, MagickFalse },
    { "+texture", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-texture", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+threshold", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-threshold", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+thumbnail", 1L, DeprecateOptionFlag, MagickTrue },
    { "-thumbnail", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+tile", 0L, DrawInfoOptionFlag, MagickFalse },
    { "-tile", 1L, DrawInfoOptionFlag, MagickFalse },
    { "+tile-offset", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-tile-offset", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+tint", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-tint", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+title", 0L, NonMagickOptionFlag, MagickFalse },
    { "-title", 1L, NonMagickOptionFlag, MagickFalse },
    { "+transform", 0L, DeprecateOptionFlag, MagickTrue },
    { "-transform", 0L, DeprecateOptionFlag | SimpleOperatorOptionFlag, MagickTrue },
    { "+transparent", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "-transparent", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+transparent-color", 1L, ImageInfoOptionFlag, MagickFalse },
    { "-transparent-color", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+transpose", 0L, DeprecateOptionFlag, MagickTrue },
    { "-transpose", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+transverse", 0L, DeprecateOptionFlag, MagickTrue },
    { "-transverse", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+treedepth", 1L, DeprecateOptionFlag, MagickTrue },
    { "-treedepth", 1L, QuantizeInfoOptionFlag, MagickFalse },
    { "+trim", 0L, DeprecateOptionFlag, MagickTrue },
    { "-trim", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+type", 0L, ImageInfoOptionFlag | SimpleOperatorOptionFlag, MagickFalse },
    { "-type", 1L, ImageInfoOptionFlag | SimpleOperatorOptionFlag, MagickFalse },
    { "+undercolor", 0L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "-undercolor", 1L, ImageInfoOptionFlag | DrawInfoOptionFlag, MagickFalse },
    { "+unique", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "-unique", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+unique-colors", 0L, DeprecateOptionFlag, MagickTrue },
    { "-unique-colors", 0L, SimpleOperatorOptionFlag, MagickFalse },
    { "+units", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-units", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+unsharp", 1L, DeprecateOptionFlag, MagickTrue },
    { "-unsharp", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+update", 0L, NonMagickOptionFlag, MagickFalse },
    { "-update", 1L, NonMagickOptionFlag, MagickFalse },
    { "+use-pixmap", 0L, NonMagickOptionFlag, MagickFalse },
    { "-use-pixmap", 1L, NonMagickOptionFlag, MagickFalse },
    { "+verbose", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-verbose", 0L, ImageInfoOptionFlag, MagickFalse },
    { "+version", 0L, DeprecateOptionFlag, MagickTrue },
    { "-version", 0L, SpecialOptionFlag, MagickFalse },
    { "+view", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-view", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+vignette", 1L, DeprecateOptionFlag, MagickTrue },
    { "-vignette", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+virtual-pixel", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-virtual-pixel", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+visual", 0L, NonMagickOptionFlag, MagickFalse },
    { "-visual", 1L, NonMagickOptionFlag, MagickFalse },
    { "+watermark", 0L, NonMagickOptionFlag, MagickFalse },
    { "-watermark", 1L, NonMagickOptionFlag, MagickFalse },
    { "+wave", 1L, DeprecateOptionFlag, MagickTrue },
    { "-wave", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+weight", 1L, DeprecateOptionFlag, MagickTrue },
    { "-weight", 1L, DrawInfoOptionFlag, MagickFalse },
    { "+white-point", 0L, ImageInfoOptionFlag, MagickFalse },
    { "-white-point", 1L, ImageInfoOptionFlag, MagickFalse },
    { "+white-threshold", 1L, DeprecateOptionFlag, MagickTrue },
    { "-white-threshold", 1L, SimpleOperatorOptionFlag, MagickFalse },
    { "+window", 0L, NonMagickOptionFlag, MagickFalse },
    { "-window", 1L, NonMagickOptionFlag, MagickFalse },
    { "+window-group", 0L, NonMagickOptionFlag, MagickFalse },
    { "-window-group", 1L, NonMagickOptionFlag, MagickFalse },
    { "+write", 1L, SpecialOptionFlag | FireOptionFlag, MagickFalse },
    { "-write", 1L, SpecialOptionFlag | FireOptionFlag, MagickFalse },
    { (char *) NULL, 0L, UndefinedOptionFlag, MagickFalse }
  },
  ComposeOptions[] =
  {
    { "Undefined", UndefinedCompositeOp, UndefinedOptionFlag, MagickTrue },
    { "Atop", AtopCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Blend", BlendCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Blur", BlurCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Bumpmap", BumpmapCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "ChangeMask", ChangeMaskCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Clear", ClearCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "ColorBurn", ColorBurnCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "ColorDodge", ColorDodgeCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Colorize", ColorizeCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyAlpha", CopyAlphaCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyBlack", CopyBlackCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyBlue", CopyBlueCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyCyan", CopyCyanCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyGreen", CopyGreenCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Copy", CopyCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyMagenta", CopyMagentaCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyRed", CopyRedCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "CopyYellow", CopyYellowCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Darken", DarkenCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "DarkenIntensity", DarkenIntensityCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "DivideDst", DivideDstCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "DivideSrc", DivideSrcCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Dst", DstCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Difference", DifferenceCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Displace", DisplaceCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Dissolve", DissolveCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Distort", DistortCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "DstAtop", DstAtopCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "DstIn", DstInCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "DstOut", DstOutCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "DstOver", DstOverCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Exclusion", ExclusionCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "HardLight", HardLightCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Hue", HueCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "In", InCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Intensity", IntensityCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Lighten", LightenCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "LightenIntensity", LightenIntensityCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "LinearBurn", LinearBurnCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "LinearDodge", LinearDodgeCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "LinearLight", LinearLightCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Luminize", LuminizeCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Mathematics", MathematicsCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "MinusDst", MinusDstCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "MinusSrc", MinusSrcCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Modulate", ModulateCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "ModulusAdd", ModulusAddCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "ModulusSubtract", ModulusSubtractCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Multiply", MultiplyCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "None", NoCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Out", OutCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Overlay", OverlayCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Over", OverCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "PegtopLight", PegtopLightCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "PinLight", PinLightCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Plus", PlusCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Replace", ReplaceCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Saturate", SaturateCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Screen", ScreenCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "SoftLight", SoftLightCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Src", SrcCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "SrcAtop", SrcAtopCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "SrcIn", SrcInCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "SrcOut", SrcOutCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "SrcOver", SrcOverCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "VividLight", VividLightCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Xor", XorCompositeOp, UndefinedOptionFlag, MagickFalse },
    { "Divide", DivideDstCompositeOp, DeprecateOptionFlag, MagickTrue },
    { "Minus", MinusDstCompositeOp, DeprecateOptionFlag, MagickTrue },
    { "Threshold", ThresholdCompositeOp, DeprecateOptionFlag, MagickTrue },
    { "CopyOpacity", CopyAlphaCompositeOp, UndefinedOptionFlag, MagickTrue },
    { (char *) NULL, UndefinedCompositeOp, UndefinedOptionFlag, MagickFalse }
  },
  CompressOptions[] =
  {
    { "Undefined", UndefinedCompression, UndefinedOptionFlag, MagickTrue },
    { "B44", B44Compression, UndefinedOptionFlag, MagickFalse },
    { "B44A", B44ACompression, UndefinedOptionFlag, MagickFalse },
    { "BZip", BZipCompression, UndefinedOptionFlag, MagickFalse },
    { "DXT1", DXT1Compression, UndefinedOptionFlag, MagickFalse },
    { "DXT3", DXT3Compression, UndefinedOptionFlag, MagickFalse },
    { "DXT5", DXT5Compression, UndefinedOptionFlag, MagickFalse },
    { "Fax", FaxCompression, UndefinedOptionFlag, MagickFalse },
    { "Group4", Group4Compression, UndefinedOptionFlag, MagickFalse },
    { "JBIG1", JBIG1Compression, UndefinedOptionFlag, MagickFalse },
    { "JBIG2", JBIG2Compression, UndefinedOptionFlag, MagickFalse },
    { "JPEG", JPEGCompression, UndefinedOptionFlag, MagickFalse },
    { "JPEG2000", JPEG2000Compression, UndefinedOptionFlag, MagickFalse },
    { "Lossless", LosslessJPEGCompression, UndefinedOptionFlag, MagickFalse },
    { "LosslessJPEG", LosslessJPEGCompression, UndefinedOptionFlag, MagickFalse },
    { "LZMA", LZMACompression, UndefinedOptionFlag, MagickFalse },
    { "LZW", LZWCompression, UndefinedOptionFlag, MagickFalse },
    { "None", NoCompression, UndefinedOptionFlag, MagickFalse },
    { "Piz", PizCompression, UndefinedOptionFlag, MagickFalse },
    { "Pxr24", Pxr24Compression, UndefinedOptionFlag, MagickFalse },
    { "RLE", RLECompression, UndefinedOptionFlag, MagickFalse },
    { "Zip", ZipCompression, UndefinedOptionFlag, MagickFalse },
    { "RunlengthEncoded", RLECompression, UndefinedOptionFlag, MagickFalse },
    { "ZipS", ZipSCompression, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedCompression, UndefinedOptionFlag, MagickFalse }
  },
  ColorspaceOptions[] =
  {
    { "Undefined", UndefinedColorspace, UndefinedOptionFlag, MagickTrue },
    { "CMY", CMYColorspace, UndefinedOptionFlag, MagickFalse },
    { "CMYK", CMYKColorspace, UndefinedOptionFlag, MagickFalse },
    { "Gray", GRAYColorspace, UndefinedOptionFlag, MagickFalse },
    { "HSB", HSBColorspace, UndefinedOptionFlag, MagickFalse },
    { "HSL", HSLColorspace, UndefinedOptionFlag, MagickFalse },
    { "HWB", HWBColorspace, UndefinedOptionFlag, MagickFalse },
    { "Lab", LabColorspace, UndefinedOptionFlag, MagickFalse },
    { "Log", LogColorspace, UndefinedOptionFlag, MagickFalse },
    { "OHTA", OHTAColorspace, UndefinedOptionFlag, MagickFalse },
    { "Rec601Luma", Rec601LumaColorspace, UndefinedOptionFlag, MagickFalse },
    { "Rec601YCbCr", Rec601YCbCrColorspace, UndefinedOptionFlag, MagickFalse },
    { "Rec709Luma", Rec709LumaColorspace, UndefinedOptionFlag, MagickFalse },
    { "Rec709YCbCr", Rec709YCbCrColorspace, UndefinedOptionFlag, MagickFalse },
    { "RGB", RGBColorspace, UndefinedOptionFlag, MagickFalse },
    { "sRGB", sRGBColorspace, UndefinedOptionFlag, MagickFalse },
    { "Transparent", TransparentColorspace, UndefinedOptionFlag, MagickFalse },
    { "XYZ", XYZColorspace, UndefinedOptionFlag, MagickFalse },
    { "YCbCr", YCbCrColorspace, UndefinedOptionFlag, MagickFalse },
    { "YCC", YCCColorspace, UndefinedOptionFlag, MagickFalse },
    { "YIQ", YIQColorspace, UndefinedOptionFlag, MagickFalse },
    { "YPbPr", YPbPrColorspace, UndefinedOptionFlag, MagickFalse },
    { "YUV", YUVColorspace, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedColorspace, UndefinedOptionFlag, MagickFalse }
  },
  DataTypeOptions[] =
  {
    { "Undefined", UndefinedData, UndefinedOptionFlag, MagickTrue },
    { "Byte", ByteData, UndefinedOptionFlag, MagickFalse },
    { "Long", LongData, UndefinedOptionFlag, MagickFalse },
    { "Short", ShortData, UndefinedOptionFlag, MagickFalse },
    { "String", StringData, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedData, UndefinedOptionFlag, MagickFalse }
  },
  DecorateOptions[] =
  {
    { "Undefined", UndefinedDecoration, UndefinedOptionFlag, MagickTrue },
    { "LineThrough", LineThroughDecoration, UndefinedOptionFlag, MagickFalse },
    { "None", NoDecoration, UndefinedOptionFlag, MagickFalse },
    { "Overline", OverlineDecoration, UndefinedOptionFlag, MagickFalse },
    { "Underline", UnderlineDecoration, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedDecoration, UndefinedOptionFlag, MagickFalse }
  },
  DirectionOptions[] =
  {
    { "Undefined", UndefinedDirection, UndefinedOptionFlag, MagickTrue },
    { "right-to-left", RightToLeftDirection, UndefinedOptionFlag, MagickFalse },
    { "left-to-right", LeftToRightDirection, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedDirection, UndefinedOptionFlag, MagickFalse }
  },
  DisposeOptions[] =
  {
    { "Undefined", UndefinedDispose, UndefinedOptionFlag, MagickTrue },
    { "Background", BackgroundDispose, UndefinedOptionFlag, MagickFalse },
    { "None", NoneDispose, UndefinedOptionFlag, MagickFalse },
    { "Previous", PreviousDispose, UndefinedOptionFlag, MagickFalse },
    { "Undefined", UndefinedDispose, UndefinedOptionFlag, MagickFalse },
    { "0", UndefinedDispose, UndefinedOptionFlag, MagickFalse },
    { "1", NoneDispose, UndefinedOptionFlag, MagickFalse },
    { "2", BackgroundDispose, UndefinedOptionFlag, MagickFalse },
    { "3", PreviousDispose, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedDispose, UndefinedOptionFlag, MagickFalse }
  },
  DistortOptions[] =
  {
    { "Affine", AffineDistortion, UndefinedOptionFlag, MagickFalse },
    { "AffineProjection", AffineProjectionDistortion, UndefinedOptionFlag, MagickFalse },
    { "ScaleRotateTranslate", ScaleRotateTranslateDistortion, UndefinedOptionFlag, MagickFalse },
    { "SRT", ScaleRotateTranslateDistortion, UndefinedOptionFlag, MagickFalse },
    { "Perspective", PerspectiveDistortion, UndefinedOptionFlag, MagickFalse },
    { "PerspectiveProjection", PerspectiveProjectionDistortion, UndefinedOptionFlag, MagickFalse },
    { "Bilinear", BilinearForwardDistortion, UndefinedOptionFlag, MagickTrue },
    { "BilinearForward", BilinearForwardDistortion, UndefinedOptionFlag, MagickFalse },
    { "BilinearReverse", BilinearReverseDistortion, UndefinedOptionFlag, MagickFalse },
    { "Polynomial", PolynomialDistortion, UndefinedOptionFlag, MagickFalse },
    { "Arc", ArcDistortion, UndefinedOptionFlag, MagickFalse },
    { "Polar", PolarDistortion, UndefinedOptionFlag, MagickFalse },
    { "DePolar", DePolarDistortion, UndefinedOptionFlag, MagickFalse },
    { "Barrel", BarrelDistortion, UndefinedOptionFlag, MagickFalse },
    { "Cylinder2Plane", Cylinder2PlaneDistortion, UndefinedOptionFlag, MagickTrue },
    { "Plane2Cylinder", Plane2CylinderDistortion, UndefinedOptionFlag, MagickTrue },
    { "BarrelInverse", BarrelInverseDistortion, UndefinedOptionFlag, MagickFalse },
    { "Shepards", ShepardsDistortion, UndefinedOptionFlag, MagickFalse },
    { "Resize", ResizeDistortion, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedDistortion, UndefinedOptionFlag, MagickFalse }
  },
  DitherOptions[] =
  {
    { "Undefined", UndefinedDitherMethod, UndefinedOptionFlag, MagickTrue },
    { "None", NoDitherMethod, UndefinedOptionFlag, MagickFalse },
    { "FloydSteinberg", FloydSteinbergDitherMethod, UndefinedOptionFlag, MagickFalse },
    { "Riemersma", RiemersmaDitherMethod, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedEndian, UndefinedOptionFlag, MagickFalse }
  },
  EndianOptions[] =
  {
    { "Undefined", UndefinedEndian, UndefinedOptionFlag, MagickTrue },
    { "LSB", LSBEndian, UndefinedOptionFlag, MagickFalse },
    { "MSB", MSBEndian, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedEndian, UndefinedOptionFlag, MagickFalse }
  },
  EvaluateOptions[] =
  {
    { "Undefined", UndefinedEvaluateOperator, UndefinedOptionFlag, MagickTrue },
    { "Abs", AbsEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Add", AddEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "AddModulus", AddModulusEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "And", AndEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Cos", CosineEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Cosine", CosineEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Divide", DivideEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Exp", ExponentialEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Exponential", ExponentialEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "GaussianNoise", GaussianNoiseEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "ImpulseNoise", ImpulseNoiseEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "LaplacianNoise", LaplacianNoiseEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "LeftShift", LeftShiftEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Log", LogEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Max", MaxEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Mean", MeanEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Median", MedianEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Min", MinEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "MultiplicativeNoise", MultiplicativeNoiseEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Multiply", MultiplyEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Or", OrEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "PoissonNoise", PoissonNoiseEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Pow", PowEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "RightShift", RightShiftEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Set", SetEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Sin", SineEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Sine", SineEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Subtract", SubtractEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Sum", SumEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Threshold", ThresholdEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "ThresholdBlack", ThresholdBlackEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "ThresholdWhite", ThresholdWhiteEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "UniformNoise", UniformNoiseEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { "Xor", XorEvaluateOperator, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedEvaluateOperator, UndefinedOptionFlag, MagickFalse }
  },
  FillRuleOptions[] =
  {
    { "Undefined", UndefinedRule, UndefinedOptionFlag, MagickTrue },
    { "Evenodd", EvenOddRule, UndefinedOptionFlag, MagickFalse },
    { "NonZero", NonZeroRule, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedRule, UndefinedOptionFlag, MagickFalse }
  },
  FilterOptions[] =
  {
    { "Undefined", UndefinedFilter, UndefinedOptionFlag, MagickTrue },
    { "Bartlett", BartlettFilter, UndefinedOptionFlag, MagickFalse },
    { "Blackman", BlackmanFilter, UndefinedOptionFlag, MagickFalse },
    { "Bohman", BohmanFilter, UndefinedOptionFlag, MagickFalse },
    { "Box", BoxFilter, UndefinedOptionFlag, MagickFalse },
    { "Catrom", CatromFilter, UndefinedOptionFlag, MagickFalse },
    { "Cubic", CubicFilter, UndefinedOptionFlag, MagickFalse },
    { "Gaussian", GaussianFilter, UndefinedOptionFlag, MagickFalse },
    { "Hamming", HammingFilter, UndefinedOptionFlag, MagickFalse },
    { "Hanning", HanningFilter, UndefinedOptionFlag, MagickFalse },
    { "Hermite", HermiteFilter, UndefinedOptionFlag, MagickFalse },
    { "Jinc", JincFilter, UndefinedOptionFlag, MagickFalse },
    { "Kaiser", KaiserFilter, UndefinedOptionFlag, MagickFalse },
    { "Lagrange", LagrangeFilter, UndefinedOptionFlag, MagickFalse },
    { "Lanczos", LanczosFilter, UndefinedOptionFlag, MagickFalse },
    { "LanczosSharp", LanczosSharpFilter, UndefinedOptionFlag, MagickFalse },
    { "Lanczos2", Lanczos2Filter, UndefinedOptionFlag, MagickFalse },
    { "Lanczos2Sharp", Lanczos2SharpFilter, UndefinedOptionFlag, MagickFalse },
    { "Mitchell", MitchellFilter, UndefinedOptionFlag, MagickFalse },
    { "Parzen", ParzenFilter, UndefinedOptionFlag, MagickFalse },
    { "Point", PointFilter, UndefinedOptionFlag, MagickFalse },
    { "Quadratic", QuadraticFilter, UndefinedOptionFlag, MagickFalse },
    { "Robidoux", RobidouxFilter, UndefinedOptionFlag, MagickFalse },
    { "Sinc", SincFilter, UndefinedOptionFlag, MagickFalse },
    { "SincFast", SincFastFilter, UndefinedOptionFlag, MagickFalse },
    { "Triangle", TriangleFilter, UndefinedOptionFlag, MagickFalse },
    { "Welsh", WelshFilter, UndefinedOptionFlag, MagickFalse },
    /* For backward compatibility - must be after "Jinc" */
    { "Bessel", JincFilter, UndefinedOptionFlag, MagickTrue },
    { (char *) NULL, UndefinedFilter, UndefinedOptionFlag, MagickFalse }
  },
  FunctionOptions[] =
  {
    { "Undefined", UndefinedFunction, UndefinedOptionFlag, MagickTrue },
    { "Polynomial", PolynomialFunction, UndefinedOptionFlag, MagickFalse },
    { "Sinusoid", SinusoidFunction, UndefinedOptionFlag, MagickFalse },
    { "ArcSin", ArcsinFunction, UndefinedOptionFlag, MagickFalse },
    { "ArcTan", ArctanFunction, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedFunction, UndefinedOptionFlag, MagickFalse }
  },
  GravityOptions[] =
  {
    { "Undefined", UndefinedGravity, UndefinedOptionFlag, MagickTrue },
    { "None", UndefinedGravity, UndefinedOptionFlag, MagickFalse },
    { "Center", CenterGravity, UndefinedOptionFlag, MagickFalse },
    { "East", EastGravity, UndefinedOptionFlag, MagickFalse },
    { "Forget", ForgetGravity, UndefinedOptionFlag, MagickFalse },
    { "NorthEast", NorthEastGravity, UndefinedOptionFlag, MagickFalse },
    { "North", NorthGravity, UndefinedOptionFlag, MagickFalse },
    { "NorthWest", NorthWestGravity, UndefinedOptionFlag, MagickFalse },
    { "SouthEast", SouthEastGravity, UndefinedOptionFlag, MagickFalse },
    { "South", SouthGravity, UndefinedOptionFlag, MagickFalse },
    { "SouthWest", SouthWestGravity, UndefinedOptionFlag, MagickFalse },
    { "West", WestGravity, UndefinedOptionFlag, MagickFalse },
    { "Static", StaticGravity, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedGravity, UndefinedOptionFlag, MagickFalse }
  },
  IntentOptions[] =
  {
    { "Undefined", UndefinedIntent, UndefinedOptionFlag, MagickTrue },
    { "Absolute", AbsoluteIntent, UndefinedOptionFlag, MagickFalse },
    { "Perceptual", PerceptualIntent, UndefinedOptionFlag, MagickFalse },
    { "Relative", RelativeIntent, UndefinedOptionFlag, MagickFalse },
    { "Saturation", SaturationIntent, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedIntent, UndefinedOptionFlag, MagickFalse }
  },
  InterlaceOptions[] =
  {
    { "Undefined", UndefinedInterlace, UndefinedOptionFlag, MagickTrue },
    { "Line", LineInterlace, UndefinedOptionFlag, MagickFalse },
    { "None", NoInterlace, UndefinedOptionFlag, MagickFalse },
    { "Plane", PlaneInterlace, UndefinedOptionFlag, MagickFalse },
    { "Partition", PartitionInterlace, UndefinedOptionFlag, MagickFalse },
    { "GIF", GIFInterlace, UndefinedOptionFlag, MagickFalse },
    { "JPEG", JPEGInterlace, UndefinedOptionFlag, MagickFalse },
    { "PNG", PNGInterlace, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedInterlace, UndefinedOptionFlag, MagickFalse }
  },
  InterpolateOptions[] =
  {
    { "Undefined", UndefinedInterpolatePixel, UndefinedOptionFlag, MagickTrue },
    { "Average", AverageInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "Bicubic", BicubicInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "Bilinear", BilinearInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "filter", FilterInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "Integer", IntegerInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "Mesh", MeshInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "Nearest", NearestNeighborInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "NearestNeighbor", NearestNeighborInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { "Spline", SplineInterpolatePixel, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedInterpolatePixel, UndefinedOptionFlag, MagickFalse }
  },
  KernelOptions[] =
  {
    { "Undefined", UndefinedKernel, UndefinedOptionFlag, MagickTrue },
    { "Unity", UnityKernel, UndefinedOptionFlag, MagickFalse },
    { "Gaussian", GaussianKernel, UndefinedOptionFlag, MagickFalse },
    { "DoG", DoGKernel, UndefinedOptionFlag, MagickFalse },
    { "LoG", LoGKernel, UndefinedOptionFlag, MagickFalse },
    { "Blur", BlurKernel, UndefinedOptionFlag, MagickFalse },
    { "Comet", CometKernel, UndefinedOptionFlag, MagickFalse },
    { "Laplacian", LaplacianKernel, UndefinedOptionFlag, MagickFalse },
    { "Sobel", SobelKernel, UndefinedOptionFlag, MagickFalse },
    { "FreiChen", FreiChenKernel, UndefinedOptionFlag, MagickFalse },
    { "Roberts", RobertsKernel, UndefinedOptionFlag, MagickFalse },
    { "Prewitt", PrewittKernel, UndefinedOptionFlag, MagickFalse },
    { "Compass", CompassKernel, UndefinedOptionFlag, MagickFalse },
    { "Kirsch", KirschKernel, UndefinedOptionFlag, MagickFalse },
    { "Diamond", DiamondKernel, UndefinedOptionFlag, MagickFalse },
    { "Square", SquareKernel, UndefinedOptionFlag, MagickFalse },
    { "Rectangle", RectangleKernel, UndefinedOptionFlag, MagickFalse },
    { "Disk", DiskKernel, UndefinedOptionFlag, MagickFalse },
    { "Octagon", OctagonKernel, UndefinedOptionFlag, MagickFalse },
    { "Plus", PlusKernel, UndefinedOptionFlag, MagickFalse },
    { "Cross", CrossKernel, UndefinedOptionFlag, MagickFalse },
    { "Ring", RingKernel, UndefinedOptionFlag, MagickFalse },
    { "Peaks", PeaksKernel, UndefinedOptionFlag, MagickFalse },
    { "Edges", EdgesKernel, UndefinedOptionFlag, MagickFalse },
    { "Corners", CornersKernel, UndefinedOptionFlag, MagickFalse },
    { "Diagonals", DiagonalsKernel, UndefinedOptionFlag, MagickFalse },
    { "ThinDiagonals", DiagonalsKernel, DeprecateOptionFlag, MagickTrue },
    { "LineEnds", LineEndsKernel, UndefinedOptionFlag, MagickFalse },
    { "LineJunctions", LineJunctionsKernel, UndefinedOptionFlag, MagickFalse },
    { "Ridges", RidgesKernel, UndefinedOptionFlag, MagickFalse },
    { "ConvexHull", ConvexHullKernel, UndefinedOptionFlag, MagickFalse },
    { "ThinSe", ThinSEKernel, UndefinedOptionFlag, MagickFalse },
    { "Skeleton", SkeletonKernel, UndefinedOptionFlag, MagickFalse },
    { "Chebyshev", ChebyshevKernel, UndefinedOptionFlag, MagickFalse },
    { "Manhattan", ManhattanKernel, UndefinedOptionFlag, MagickFalse },
    { "Octagonal", OctagonalKernel, UndefinedOptionFlag, MagickFalse },
    { "Euclidean", EuclideanKernel, UndefinedOptionFlag, MagickFalse },
    { "User Defined", UserDefinedKernel, UndefinedOptionFlag, MagickTrue },
    { (char *) NULL, UndefinedKernel, UndefinedOptionFlag, MagickFalse }
  },
  LayerOptions[] =
  {
    { "Undefined", UndefinedLayer, UndefinedOptionFlag, MagickTrue },
    { "Coalesce", CoalesceLayer, UndefinedOptionFlag, MagickFalse },
    { "CompareAny", CompareAnyLayer, UndefinedOptionFlag, MagickFalse },
    { "CompareClear", CompareClearLayer, UndefinedOptionFlag, MagickFalse },
    { "CompareOverlay", CompareOverlayLayer, UndefinedOptionFlag, MagickFalse },
    { "Dispose", DisposeLayer, UndefinedOptionFlag, MagickFalse },
    { "Optimize", OptimizeLayer, UndefinedOptionFlag, MagickFalse },
    { "OptimizeFrame", OptimizeImageLayer, UndefinedOptionFlag, MagickFalse },
    { "OptimizePlus", OptimizePlusLayer, UndefinedOptionFlag, MagickFalse },
    { "OptimizeTransparency", OptimizeTransLayer, UndefinedOptionFlag, MagickFalse },
    { "RemoveDups", RemoveDupsLayer, UndefinedOptionFlag, MagickFalse },
    { "RemoveZero", RemoveZeroLayer, UndefinedOptionFlag, MagickFalse },
    { "Composite", CompositeLayer, UndefinedOptionFlag, MagickFalse },
    { "Merge", MergeLayer, UndefinedOptionFlag, MagickFalse },
    { "Flatten", FlattenLayer, UndefinedOptionFlag, MagickFalse },
    { "Mosaic", MosaicLayer, UndefinedOptionFlag, MagickFalse },
    { "TrimBounds", TrimBoundsLayer, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedLayer, UndefinedOptionFlag, MagickFalse }
  },
  LineCapOptions[] =
  {
    { "Undefined", UndefinedCap, UndefinedOptionFlag, MagickTrue },
    { "Butt", ButtCap, UndefinedOptionFlag, MagickFalse },
    { "Round", RoundCap, UndefinedOptionFlag, MagickFalse },
    { "Square", SquareCap, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedCap, UndefinedOptionFlag, MagickFalse }
  },
  LineJoinOptions[] =
  {
    { "Undefined", UndefinedJoin, UndefinedOptionFlag, MagickTrue },
    { "Bevel", BevelJoin, UndefinedOptionFlag, MagickFalse },
    { "Miter", MiterJoin, UndefinedOptionFlag, MagickFalse },
    { "Round", RoundJoin, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedJoin, UndefinedOptionFlag, MagickFalse }
  },
  ListOptions[] =
  {
    { "Align", MagickAlignOptions, UndefinedOptionFlag, MagickFalse },
    { "Alpha", MagickAlphaOptions, UndefinedOptionFlag, MagickFalse },
    { "Boolean", MagickBooleanOptions, UndefinedOptionFlag, MagickFalse },
    { "Channel", MagickChannelOptions, UndefinedOptionFlag, MagickFalse },
    { "Class", MagickClassOptions, UndefinedOptionFlag, MagickFalse },
    { "ClipPath", MagickClipPathOptions, UndefinedOptionFlag, MagickFalse },
    { "Coder", MagickCoderOptions, UndefinedOptionFlag, MagickFalse },
    { "Color", MagickColorOptions, UndefinedOptionFlag, MagickFalse },
    { "Colorspace", MagickColorspaceOptions, UndefinedOptionFlag, MagickFalse },
    { "Command", MagickCommandOptions, UndefinedOptionFlag, MagickFalse },
    { "Compose", MagickComposeOptions, UndefinedOptionFlag, MagickFalse },
    { "Compress", MagickCompressOptions, UndefinedOptionFlag, MagickFalse },
    { "Configure", MagickConfigureOptions, UndefinedOptionFlag, MagickFalse },
    { "DataType", MagickDataTypeOptions, UndefinedOptionFlag, MagickFalse },
    { "Debug", MagickDebugOptions, UndefinedOptionFlag, MagickFalse },
    { "Decoration", MagickDecorateOptions, UndefinedOptionFlag, MagickFalse },
    { "Delegate", MagickDelegateOptions, UndefinedOptionFlag, MagickFalse },
    { "Direction", MagickDirectionOptions, UndefinedOptionFlag, MagickFalse },
    { "Dispose", MagickDisposeOptions, UndefinedOptionFlag, MagickFalse },
    { "Distort", MagickDistortOptions, UndefinedOptionFlag, MagickFalse },
    { "Dither", MagickDitherOptions, UndefinedOptionFlag, MagickFalse },
    { "Endian", MagickEndianOptions, UndefinedOptionFlag, MagickFalse },
    { "Evaluate", MagickEvaluateOptions, UndefinedOptionFlag, MagickFalse },
    { "FillRule", MagickFillRuleOptions, UndefinedOptionFlag, MagickFalse },
    { "Filter", MagickFilterOptions, UndefinedOptionFlag, MagickFalse },
    { "Font", MagickFontOptions, UndefinedOptionFlag, MagickFalse },
    { "Format", MagickFormatOptions, UndefinedOptionFlag, MagickFalse },
    { "Function", MagickFunctionOptions, UndefinedOptionFlag, MagickFalse },
    { "Gravity", MagickGravityOptions, UndefinedOptionFlag, MagickFalse },
    { "Intent", MagickIntentOptions, UndefinedOptionFlag, MagickFalse },
    { "Interlace", MagickInterlaceOptions, UndefinedOptionFlag, MagickFalse },
    { "Interpolate", MagickInterpolateOptions, UndefinedOptionFlag, MagickFalse },
    { "Kernel", MagickKernelOptions, UndefinedOptionFlag, MagickFalse },
    { "Layers", MagickLayerOptions, UndefinedOptionFlag, MagickFalse },
    { "LineCap", MagickLineCapOptions, UndefinedOptionFlag, MagickFalse },
    { "LineJoin", MagickLineJoinOptions, UndefinedOptionFlag, MagickFalse },
    { "List", MagickListOptions, UndefinedOptionFlag, MagickFalse },
    { "Locale", MagickLocaleOptions, UndefinedOptionFlag, MagickFalse },
    { "LogEvent", MagickLogEventOptions, UndefinedOptionFlag, MagickFalse },
    { "Log", MagickLogOptions, UndefinedOptionFlag, MagickFalse },
    { "Magic", MagickMagicOptions, UndefinedOptionFlag, MagickFalse },
    { "Method", MagickMethodOptions, UndefinedOptionFlag, MagickFalse },
    { "Metric", MagickMetricOptions, UndefinedOptionFlag, MagickFalse },
    { "Mime", MagickMimeOptions, UndefinedOptionFlag, MagickFalse },
    { "Mode", MagickModeOptions, UndefinedOptionFlag, MagickFalse },
    { "Morphology", MagickMorphologyOptions, UndefinedOptionFlag, MagickFalse },
    { "Module", MagickModuleOptions, UndefinedOptionFlag, MagickFalse },
    { "Noise", MagickNoiseOptions, UndefinedOptionFlag, MagickFalse },
    { "Orientation", MagickOrientationOptions, UndefinedOptionFlag, MagickFalse },
    { "PixelChannel", MagickPixelChannelOptions, UndefinedOptionFlag, MagickFalse },
    { "PixelTrait", MagickPixelTraitOptions, UndefinedOptionFlag, MagickFalse },
    { "Policy", MagickPolicyOptions, UndefinedOptionFlag, MagickFalse },
    { "PolicyDomain", MagickPolicyDomainOptions, UndefinedOptionFlag, MagickFalse },
    { "PolicyRights", MagickPolicyRightsOptions, UndefinedOptionFlag, MagickFalse },
    { "Preview", MagickPreviewOptions, UndefinedOptionFlag, MagickFalse },
    { "Primitive", MagickPrimitiveOptions, UndefinedOptionFlag, MagickFalse },
    { "QuantumFormat", MagickQuantumFormatOptions, UndefinedOptionFlag, MagickFalse },
    { "Resource", MagickResourceOptions, UndefinedOptionFlag, MagickFalse },
    { "SparseColor", MagickSparseColorOptions, UndefinedOptionFlag, MagickFalse },
    { "Statistic", MagickStatisticOptions, UndefinedOptionFlag, MagickFalse },
    { "Storage", MagickStorageOptions, UndefinedOptionFlag, MagickFalse },
    { "Stretch", MagickStretchOptions, UndefinedOptionFlag, MagickFalse },
    { "Style", MagickStyleOptions, UndefinedOptionFlag, MagickFalse },
    { "Threshold", MagickThresholdOptions, UndefinedOptionFlag, MagickFalse },
    { "Type", MagickTypeOptions, UndefinedOptionFlag, MagickFalse },
    { "Units", MagickResolutionOptions, UndefinedOptionFlag, MagickFalse },
    { "Undefined", MagickUndefinedOptions, UndefinedOptionFlag, MagickTrue },
    { "Validate", MagickValidateOptions, UndefinedOptionFlag, MagickFalse },
    { "VirtualPixel", MagickVirtualPixelOptions, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, MagickUndefinedOptions, UndefinedOptionFlag, MagickFalse }
  },
  LogEventOptions[] =
  {
    { "Undefined", UndefinedEvents, UndefinedOptionFlag, MagickTrue },
    { "All", (AllEvents &~ TraceEvent), UndefinedOptionFlag, MagickFalse },
    { "Accelerate", AccelerateEvent, UndefinedOptionFlag, MagickFalse },
    { "Annotate", AnnotateEvent, UndefinedOptionFlag, MagickFalse },
    { "Blob", BlobEvent, UndefinedOptionFlag, MagickFalse },
    { "Cache", CacheEvent, UndefinedOptionFlag, MagickFalse },
    { "Coder", CoderEvent, UndefinedOptionFlag, MagickFalse },
    { "Configure", ConfigureEvent, UndefinedOptionFlag, MagickFalse },
    { "Deprecate", DeprecateEvent, UndefinedOptionFlag, MagickFalse },
    { "Draw", DrawEvent, UndefinedOptionFlag, MagickFalse },
    { "Exception", ExceptionEvent, UndefinedOptionFlag, MagickFalse },
    { "Locale", LocaleEvent, UndefinedOptionFlag, MagickFalse },
    { "Module", ModuleEvent, UndefinedOptionFlag, MagickFalse },
    { "None", NoEvents, UndefinedOptionFlag, MagickFalse },
    { "Pixel", PixelEvent, UndefinedOptionFlag, MagickFalse },
    { "Policy", PolicyEvent, UndefinedOptionFlag, MagickFalse },
    { "Resource", ResourceEvent, UndefinedOptionFlag, MagickFalse },
    { "Trace", TraceEvent, UndefinedOptionFlag, MagickFalse },
    { "Transform", TransformEvent, UndefinedOptionFlag, MagickFalse },
    { "User", UserEvent, UndefinedOptionFlag, MagickFalse },
    { "Wand", WandEvent, UndefinedOptionFlag, MagickFalse },
    { "X11", X11Event, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedEvents, UndefinedOptionFlag, MagickFalse }
  },
  MetricOptions[] =
  {
    { "Undefined", UndefinedMetric, UndefinedOptionFlag, MagickTrue },
    { "AE", AbsoluteErrorMetric, UndefinedOptionFlag, MagickFalse },
    { "Fuzz", FuzzErrorMetric, UndefinedOptionFlag, MagickFalse },
    { "MAE", MeanAbsoluteErrorMetric, UndefinedOptionFlag, MagickFalse },
    { "MEPP", MeanErrorPerPixelMetric, UndefinedOptionFlag, MagickFalse },
    { "MSE", MeanSquaredErrorMetric, UndefinedOptionFlag, MagickFalse },
    { "NCC", NormalizedCrossCorrelationErrorMetric, UndefinedOptionFlag, MagickFalse },
    { "PAE", PeakAbsoluteErrorMetric, UndefinedOptionFlag, MagickFalse },
    { "PSNR", PeakSignalToNoiseRatioMetric, UndefinedOptionFlag, MagickFalse },
    { "RMSE", RootMeanSquaredErrorMetric, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedMetric, UndefinedOptionFlag, MagickFalse }
  },
  MethodOptions[] =
  {
    { "Undefined", UndefinedMethod, UndefinedOptionFlag, MagickTrue },
    { "FillToBorder", FillToBorderMethod, UndefinedOptionFlag, MagickFalse },
    { "Floodfill", FloodfillMethod, UndefinedOptionFlag, MagickFalse },
    { "Point", PointMethod, UndefinedOptionFlag, MagickFalse },
    { "Replace", ReplaceMethod, UndefinedOptionFlag, MagickFalse },
    { "Reset", ResetMethod, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedMethod, UndefinedOptionFlag, MagickFalse }
  },
  ModeOptions[] =
  {
    { "Undefined", UndefinedMode, UndefinedOptionFlag, MagickTrue },
    { "Concatenate", ConcatenateMode, UndefinedOptionFlag, MagickFalse },
    { "Frame", FrameMode, UndefinedOptionFlag, MagickFalse },
    { "Unframe", UnframeMode, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedMode, UndefinedOptionFlag, MagickFalse }
  },
  MorphologyOptions[] =
  {
    { "Undefined", UndefinedMorphology, UndefinedOptionFlag, MagickTrue },
    { "Correlate", CorrelateMorphology, UndefinedOptionFlag, MagickFalse },
    { "Convolve", ConvolveMorphology, UndefinedOptionFlag, MagickFalse },
    { "Dilate", DilateMorphology, UndefinedOptionFlag, MagickFalse },
    { "Erode", ErodeMorphology, UndefinedOptionFlag, MagickFalse },
    { "Close", CloseMorphology, UndefinedOptionFlag, MagickFalse },
    { "Open", OpenMorphology, UndefinedOptionFlag, MagickFalse },
    { "DilateIntensity", DilateIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "ErodeIntensity", ErodeIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "CloseIntensity", CloseIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "OpenIntensity", OpenIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "DilateI", DilateIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "ErodeI", ErodeIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "CloseI", CloseIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "OpenI", OpenIntensityMorphology, UndefinedOptionFlag, MagickFalse },
    { "Smooth", SmoothMorphology, UndefinedOptionFlag, MagickFalse },
    { "EdgeOut", EdgeOutMorphology, UndefinedOptionFlag, MagickFalse },
    { "EdgeIn", EdgeInMorphology, UndefinedOptionFlag, MagickFalse },
    { "Edge", EdgeMorphology, UndefinedOptionFlag, MagickFalse },
    { "TopHat", TopHatMorphology, UndefinedOptionFlag, MagickFalse },
    { "BottomHat", BottomHatMorphology, UndefinedOptionFlag, MagickFalse },
    { "Hmt", HitAndMissMorphology, UndefinedOptionFlag, MagickFalse },
    { "HitNMiss", HitAndMissMorphology, UndefinedOptionFlag, MagickFalse },
    { "HitAndMiss", HitAndMissMorphology, UndefinedOptionFlag, MagickFalse },
    { "Thinning", ThinningMorphology, UndefinedOptionFlag, MagickFalse },
    { "Thicken", ThickenMorphology, UndefinedOptionFlag, MagickFalse },
    { "Distance", DistanceMorphology, UndefinedOptionFlag, MagickFalse },
    { "IterativeDistance", IterativeDistanceMorphology, UndefinedOptionFlag, MagickFalse },
    { "Voronoi", VoronoiMorphology, UndefinedOptionFlag, MagickTrue },
    { (char *) NULL, UndefinedMorphology, UndefinedOptionFlag, MagickFalse }
  },
  NoiseOptions[] =
  {
    { "Undefined", UndefinedNoise, UndefinedOptionFlag, MagickTrue },
    { "Gaussian", GaussianNoise, UndefinedOptionFlag, MagickFalse },
    { "Impulse", ImpulseNoise, UndefinedOptionFlag, MagickFalse },
    { "Laplacian", LaplacianNoise, UndefinedOptionFlag, MagickFalse },
    { "Multiplicative", MultiplicativeGaussianNoise, UndefinedOptionFlag, MagickFalse },
    { "Poisson", PoissonNoise, UndefinedOptionFlag, MagickFalse },
    { "Random", RandomNoise, UndefinedOptionFlag, MagickFalse },
    { "Uniform", UniformNoise, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedNoise, UndefinedOptionFlag, MagickFalse }
  },
  OrientationOptions[] =
  {
    { "Undefined", UndefinedOrientation, UndefinedOptionFlag, MagickTrue },
    { "TopLeft", TopLeftOrientation, UndefinedOptionFlag, MagickFalse },
    { "TopRight", TopRightOrientation, UndefinedOptionFlag, MagickFalse },
    { "BottomRight", BottomRightOrientation, UndefinedOptionFlag, MagickFalse },
    { "BottomLeft", BottomLeftOrientation, UndefinedOptionFlag, MagickFalse },
    { "LeftTop", LeftTopOrientation, UndefinedOptionFlag, MagickFalse },
    { "RightTop", RightTopOrientation, UndefinedOptionFlag, MagickFalse },
    { "RightBottom", RightBottomOrientation, UndefinedOptionFlag, MagickFalse },
    { "LeftBottom", LeftBottomOrientation, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedOrientation, UndefinedOptionFlag, MagickFalse }
  },
  PixelChannelOptions[] =
  {
    { "Undefined", UndefinedPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "A", AlphaPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Alpha", AlphaPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "B", BluePixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Bk", BlackPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Black", BlackPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Blue", BluePixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Cb", CbPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Composite", CompositePixelChannel, UndefinedOptionFlag, MagickFalse },
    { "C", CyanPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Cr", CrPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Cyan", CyanPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Gray", GrayPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "G", GreenPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Green", GreenPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Index", IndexPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Intensity", IntensityPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "K", BlackPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "M", MagentaPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Magenta", MagentaPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Mask", MaskPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "R", RedPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Red", RedPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Sync", SyncPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Y", YellowPixelChannel, UndefinedOptionFlag, MagickFalse },
    { "Yellow", YellowPixelChannel, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedPixelChannel, UndefinedOptionFlag, MagickFalse }
  },
  PixelTraitOptions[] =
  {
    { "Undefined", UndefinedPixelTrait, UndefinedOptionFlag, MagickTrue },
    { "Blend", BlendPixelTrait, UndefinedOptionFlag, MagickFalse },
    { "Copy", CopyPixelTrait, UndefinedOptionFlag, MagickFalse },
    { "Update", UpdatePixelTrait, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedPixelTrait, UndefinedOptionFlag, MagickFalse }
  },
  PolicyDomainOptions[] =
  {
    { "Undefined", UndefinedPolicyDomain, UndefinedOptionFlag, MagickTrue },
    { "Coder", CoderPolicyDomain, UndefinedOptionFlag, MagickFalse },
    { "Delegate", DelegatePolicyDomain, UndefinedOptionFlag, MagickFalse },
    { "Filter", FilterPolicyDomain, UndefinedOptionFlag, MagickFalse },
    { "Path", PathPolicyDomain, UndefinedOptionFlag, MagickFalse },
    { "Resource", ResourcePolicyDomain, UndefinedOptionFlag, MagickFalse },
    { "System", SystemPolicyDomain, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedPolicyDomain, UndefinedOptionFlag, MagickFalse }
  },
  PolicyRightsOptions[] =
  {
    { "Undefined", UndefinedPolicyRights, UndefinedOptionFlag, MagickTrue },
    { "None", NoPolicyRights, UndefinedOptionFlag, MagickFalse },
    { "Read", ReadPolicyRights, UndefinedOptionFlag, MagickFalse },
    { "Write", WritePolicyRights, UndefinedOptionFlag, MagickFalse },
    { "Execute", ExecutePolicyRights, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedPolicyRights, UndefinedOptionFlag, MagickFalse }
  },
  PreviewOptions[] =
  {
    { "Undefined", UndefinedPreview, UndefinedOptionFlag, MagickTrue },
    { "AddNoise", AddNoisePreview, UndefinedOptionFlag, MagickFalse },
    { "Blur", BlurPreview, UndefinedOptionFlag, MagickFalse },
    { "Brightness", BrightnessPreview, UndefinedOptionFlag, MagickFalse },
    { "Charcoal", CharcoalDrawingPreview, UndefinedOptionFlag, MagickFalse },
    { "Despeckle", DespecklePreview, UndefinedOptionFlag, MagickFalse },
    { "Dull", DullPreview, UndefinedOptionFlag, MagickFalse },
    { "EdgeDetect", EdgeDetectPreview, UndefinedOptionFlag, MagickFalse },
    { "Gamma", GammaPreview, UndefinedOptionFlag, MagickFalse },
    { "Grayscale", GrayscalePreview, UndefinedOptionFlag, MagickFalse },
    { "Hue", HuePreview, UndefinedOptionFlag, MagickFalse },
    { "Implode", ImplodePreview, UndefinedOptionFlag, MagickFalse },
    { "JPEG", JPEGPreview, UndefinedOptionFlag, MagickFalse },
    { "OilPaint", OilPaintPreview, UndefinedOptionFlag, MagickFalse },
    { "Quantize", QuantizePreview, UndefinedOptionFlag, MagickFalse },
    { "Raise", RaisePreview, UndefinedOptionFlag, MagickFalse },
    { "ReduceNoise", ReduceNoisePreview, UndefinedOptionFlag, MagickFalse },
    { "Roll", RollPreview, UndefinedOptionFlag, MagickFalse },
    { "Rotate", RotatePreview, UndefinedOptionFlag, MagickFalse },
    { "Saturation", SaturationPreview, UndefinedOptionFlag, MagickFalse },
    { "Segment", SegmentPreview, UndefinedOptionFlag, MagickFalse },
    { "Shade", ShadePreview, UndefinedOptionFlag, MagickFalse },
    { "Sharpen", SharpenPreview, UndefinedOptionFlag, MagickFalse },
    { "Shear", ShearPreview, UndefinedOptionFlag, MagickFalse },
    { "Solarize", SolarizePreview, UndefinedOptionFlag, MagickFalse },
    { "Spiff", SpiffPreview, UndefinedOptionFlag, MagickFalse },
    { "Spread", SpreadPreview, UndefinedOptionFlag, MagickFalse },
    { "Swirl", SwirlPreview, UndefinedOptionFlag, MagickFalse },
    { "Threshold", ThresholdPreview, UndefinedOptionFlag, MagickFalse },
    { "Wave", WavePreview, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedPreview, UndefinedOptionFlag, MagickFalse }
  },
  PrimitiveOptions[] =
  {
    { "Undefined", UndefinedPrimitive, UndefinedOptionFlag, MagickTrue },
    { "Arc", ArcPrimitive, UndefinedOptionFlag, MagickFalse },
    { "Bezier", BezierPrimitive, UndefinedOptionFlag, MagickFalse },
    { "Circle", CirclePrimitive, UndefinedOptionFlag, MagickFalse },
    { "Color", ColorPrimitive, UndefinedOptionFlag, MagickFalse },
    { "Ellipse", EllipsePrimitive, UndefinedOptionFlag, MagickFalse },
    { "Image", ImagePrimitive, UndefinedOptionFlag, MagickFalse },
    { "Line", LinePrimitive, UndefinedOptionFlag, MagickFalse },
    { "Matte", MattePrimitive, UndefinedOptionFlag, MagickFalse },
    { "Path", PathPrimitive, UndefinedOptionFlag, MagickFalse },
    { "Point", PointPrimitive, UndefinedOptionFlag, MagickFalse },
    { "Polygon", PolygonPrimitive, UndefinedOptionFlag, MagickFalse },
    { "Polyline", PolylinePrimitive, UndefinedOptionFlag, MagickFalse },
    { "Rectangle", RectanglePrimitive, UndefinedOptionFlag, MagickFalse },
    { "RoundRectangle", RoundRectanglePrimitive, UndefinedOptionFlag, MagickFalse },
    { "Text", TextPrimitive, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedPrimitive, UndefinedOptionFlag, MagickFalse }
  },
  QuantumFormatOptions[] =
  {
    { "Undefined", UndefinedQuantumFormat, UndefinedOptionFlag, MagickTrue },
    { "FloatingPoint", FloatingPointQuantumFormat, UndefinedOptionFlag, MagickFalse },
    { "Signed", SignedQuantumFormat, UndefinedOptionFlag, MagickFalse },
    { "Unsigned", UnsignedQuantumFormat, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, FloatingPointQuantumFormat, UndefinedOptionFlag, MagickFalse }
  },
  ResolutionOptions[] =
  {
    { "Undefined", UndefinedResolution, UndefinedOptionFlag, MagickTrue },
    { "PixelsPerInch", PixelsPerInchResolution, UndefinedOptionFlag, MagickFalse },
    { "PixelsPerCentimeter", PixelsPerCentimeterResolution, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedResolution, UndefinedOptionFlag, MagickFalse }
  },
  ResourceOptions[] =
  {
    { "Undefined", UndefinedResource, UndefinedOptionFlag, MagickTrue },
    { "Area", AreaResource, UndefinedOptionFlag, MagickFalse },
    { "Disk", DiskResource, UndefinedOptionFlag, MagickFalse },
    { "File", FileResource, UndefinedOptionFlag, MagickFalse },
    { "Map", MapResource, UndefinedOptionFlag, MagickFalse },
    { "Memory", MemoryResource, UndefinedOptionFlag, MagickFalse },
    { "Thread", ThreadResource, UndefinedOptionFlag, MagickFalse },
    { "Time", TimeResource, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedResource, UndefinedOptionFlag, MagickFalse }
  },
  SparseColorOptions[] =
  {
    { "Undefined", UndefinedDistortion, UndefinedOptionFlag, MagickTrue },
    { "Barycentric", BarycentricColorInterpolate, UndefinedOptionFlag, MagickFalse },
    { "Bilinear", BilinearColorInterpolate, UndefinedOptionFlag, MagickFalse },
    { "Inverse", InverseColorInterpolate, UndefinedOptionFlag, MagickFalse },
    { "Shepards", ShepardsColorInterpolate, UndefinedOptionFlag, MagickFalse },
    { "Voronoi", VoronoiColorInterpolate, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedResource, UndefinedOptionFlag, MagickFalse }
  },
  StatisticOptions[] =
  {
    { "Undefined", UndefinedStatistic, UndefinedOptionFlag, MagickTrue },
    { "Gradient", GradientStatistic, UndefinedOptionFlag, MagickFalse },
    { "Maximum", MaximumStatistic, UndefinedOptionFlag, MagickFalse },
    { "Mean", MeanStatistic, UndefinedOptionFlag, MagickFalse },
    { "Median", MedianStatistic, UndefinedOptionFlag, MagickFalse },
    { "Minimum", MinimumStatistic, UndefinedOptionFlag, MagickFalse },
    { "Mode", ModeStatistic, UndefinedOptionFlag, MagickFalse },
    { "Nonpeak", NonpeakStatistic, UndefinedOptionFlag, MagickFalse },
    { "StandardDeviation", StandardDeviationStatistic, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedMethod, UndefinedOptionFlag, MagickFalse }
  },
  StorageOptions[] =
  {
    { "Undefined", UndefinedPixel, UndefinedOptionFlag, MagickTrue },
    { "Char", CharPixel, UndefinedOptionFlag, MagickFalse },
    { "Double", DoublePixel, UndefinedOptionFlag, MagickFalse },
    { "Float", FloatPixel, UndefinedOptionFlag, MagickFalse },
    { "Long", LongPixel, UndefinedOptionFlag, MagickFalse },
    { "LongLong", LongLongPixel, UndefinedOptionFlag, MagickFalse },
    { "Quantum", QuantumPixel, UndefinedOptionFlag, MagickFalse },
    { "Short", ShortPixel, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedResource, UndefinedOptionFlag, MagickFalse }
  },
  StretchOptions[] =
  {
    { "Undefined", UndefinedStretch, UndefinedOptionFlag, MagickTrue },
    { "Any", AnyStretch, UndefinedOptionFlag, MagickFalse },
    { "Condensed", CondensedStretch, UndefinedOptionFlag, MagickFalse },
    { "Expanded", ExpandedStretch, UndefinedOptionFlag, MagickFalse },
    { "ExtraCondensed", ExtraCondensedStretch, UndefinedOptionFlag, MagickFalse },
    { "ExtraExpanded", ExtraExpandedStretch, UndefinedOptionFlag, MagickFalse },
    { "Normal", NormalStretch, UndefinedOptionFlag, MagickFalse },
    { "SemiCondensed", SemiCondensedStretch, UndefinedOptionFlag, MagickFalse },
    { "SemiExpanded", SemiExpandedStretch, UndefinedOptionFlag, MagickFalse },
    { "UltraCondensed", UltraCondensedStretch, UndefinedOptionFlag, MagickFalse },
    { "UltraExpanded", UltraExpandedStretch, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedStretch, UndefinedOptionFlag, MagickFalse }
  },
  StyleOptions[] =
  {
    { "Undefined", UndefinedStyle, UndefinedOptionFlag, MagickTrue },
    { "Any", AnyStyle, UndefinedOptionFlag, MagickFalse },
    { "Italic", ItalicStyle, UndefinedOptionFlag, MagickFalse },
    { "Normal", NormalStyle, UndefinedOptionFlag, MagickFalse },
    { "Oblique", ObliqueStyle, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedStyle, UndefinedOptionFlag, MagickFalse }
  },
  TypeOptions[] =
  {
    { "Undefined", UndefinedType, UndefinedOptionFlag, MagickTrue },
    { "Bilevel", BilevelType, UndefinedOptionFlag, MagickFalse },
    { "ColorSeparation", ColorSeparationType, UndefinedOptionFlag, MagickFalse },
    { "ColorSeparationMatte", ColorSeparationMatteType, UndefinedOptionFlag, MagickFalse },
    { "Grayscale", GrayscaleType, UndefinedOptionFlag, MagickFalse },
    { "GrayscaleMatte", GrayscaleMatteType, UndefinedOptionFlag, MagickFalse },
    { "Optimize", OptimizeType, UndefinedOptionFlag, MagickFalse },
    { "Palette", PaletteType, UndefinedOptionFlag, MagickFalse },
    { "PaletteBilevelMatte", PaletteBilevelMatteType, UndefinedOptionFlag, MagickFalse },
    { "PaletteMatte", PaletteMatteType, UndefinedOptionFlag, MagickFalse },
    { "TrueColorMatte", TrueColorMatteType, UndefinedOptionFlag, MagickFalse },
    { "TrueColor", TrueColorType, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedType, UndefinedOptionFlag, MagickFalse }
  },
  ValidateOptions[] =
  {
    { "Undefined", UndefinedValidate, UndefinedOptionFlag, MagickTrue },
    { "All", AllValidate, UndefinedOptionFlag, MagickFalse },
    { "Compare", CompareValidate, UndefinedOptionFlag, MagickFalse },
    { "Composite", CompositeValidate, UndefinedOptionFlag, MagickFalse },
    { "Convert", ConvertValidate, UndefinedOptionFlag, MagickFalse },
    { "FormatsInMemory", FormatsInMemoryValidate, UndefinedOptionFlag, MagickFalse },
    { "FormatsOnDisk", FormatsOnDiskValidate, UndefinedOptionFlag, MagickFalse },
    { "Identify", IdentifyValidate, UndefinedOptionFlag, MagickFalse },
    { "ImportExport", ImportExportValidate, UndefinedOptionFlag, MagickFalse },
    { "Montage", MontageValidate, UndefinedOptionFlag, MagickFalse },
    { "Stream", StreamValidate, UndefinedOptionFlag, MagickFalse },
    { "None", NoValidate, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedValidate, UndefinedOptionFlag, MagickFalse }
  },
  VirtualPixelOptions[] =
  {
    { "Undefined", UndefinedVirtualPixelMethod, UndefinedOptionFlag, MagickTrue },
    { "Background", BackgroundVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Black", BlackVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Constant", BackgroundVirtualPixelMethod, DeprecateOptionFlag, MagickTrue },
    { "CheckerTile", CheckerTileVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Dither", DitherVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Edge", EdgeVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Gray", GrayVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "HorizontalTile", HorizontalTileVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "HorizontalTileEdge", HorizontalTileEdgeVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Mirror", MirrorVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Random", RandomVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Tile", TileVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "Transparent", TransparentVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "VerticalTile", VerticalTileVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "VerticalTileEdge", VerticalTileEdgeVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { "White", WhiteVirtualPixelMethod, UndefinedOptionFlag, MagickFalse },
    { (char *) NULL, UndefinedVirtualPixelMethod, UndefinedOptionFlag, MagickFalse }
  };

static const OptionInfo *GetOptionInfo(const CommandOption option)
{
  switch (option)
  {
    case MagickAlignOptions: return(AlignOptions);
    case MagickAlphaOptions: return(AlphaOptions);
    case MagickBooleanOptions: return(BooleanOptions);
    case MagickChannelOptions: return(ChannelOptions);
    case MagickClassOptions: return(ClassOptions);
    case MagickClipPathOptions: return(ClipPathOptions);
    case MagickColorspaceOptions: return(ColorspaceOptions);
    case MagickCommandOptions: return(CommandOptions);
    case MagickComposeOptions: return(ComposeOptions);
    case MagickCompressOptions: return(CompressOptions);
    case MagickDataTypeOptions: return(DataTypeOptions);
    case MagickDebugOptions: return(LogEventOptions);
    case MagickDecorateOptions: return(DecorateOptions);
    case MagickDirectionOptions: return(DirectionOptions);
    case MagickDisposeOptions: return(DisposeOptions);
    case MagickDistortOptions: return(DistortOptions);
    case MagickDitherOptions: return(DitherOptions);
    case MagickEndianOptions: return(EndianOptions);
    case MagickEvaluateOptions: return(EvaluateOptions);
    case MagickFillRuleOptions: return(FillRuleOptions);
    case MagickFilterOptions: return(FilterOptions);
    case MagickFunctionOptions: return(FunctionOptions);
    case MagickGravityOptions: return(GravityOptions);
/*  case MagickImageListOptions: return(ImageListOptions); */
    case MagickIntentOptions: return(IntentOptions);
    case MagickInterlaceOptions: return(InterlaceOptions);
    case MagickInterpolateOptions: return(InterpolateOptions);
    case MagickKernelOptions: return(KernelOptions);
    case MagickLayerOptions: return(LayerOptions);
    case MagickLineCapOptions: return(LineCapOptions);
    case MagickLineJoinOptions: return(LineJoinOptions);
    case MagickListOptions: return(ListOptions);
    case MagickLogEventOptions: return(LogEventOptions);
    case MagickMetricOptions: return(MetricOptions);
    case MagickMethodOptions: return(MethodOptions);
    case MagickModeOptions: return(ModeOptions);
    case MagickMorphologyOptions: return(MorphologyOptions);
    case MagickNoiseOptions: return(NoiseOptions);
    case MagickOrientationOptions: return(OrientationOptions);
    case MagickPixelChannelOptions: return(PixelChannelOptions);
    case MagickPixelTraitOptions: return(PixelTraitOptions);
    case MagickPolicyDomainOptions: return(PolicyDomainOptions);
    case MagickPolicyRightsOptions: return(PolicyRightsOptions);
    case MagickPreviewOptions: return(PreviewOptions);
    case MagickPrimitiveOptions: return(PrimitiveOptions);
    case MagickQuantumFormatOptions: return(QuantumFormatOptions);
    case MagickResolutionOptions: return(ResolutionOptions);
    case MagickResourceOptions: return(ResourceOptions);
    case MagickSparseColorOptions: return(SparseColorOptions);
    case MagickStatisticOptions: return(StatisticOptions);
    case MagickStorageOptions: return(StorageOptions);
    case MagickStretchOptions: return(StretchOptions);
    case MagickStyleOptions: return(StyleOptions);
    case MagickTypeOptions: return(TypeOptions);
    case MagickValidateOptions: return(ValidateOptions);
    case MagickVirtualPixelOptions: return(VirtualPixelOptions);
    default: break;
  }
  return((const OptionInfo *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e O p t i o n s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageOptions() clones one or more image options.
%
%  The format of the CloneImageOptions method is:
%
%      MagickBooleanType CloneImageOptions(ImageInfo *image_info,
%        const ImageInfo *clone_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o clone_info: the clone image info.
%
*/
MagickExport MagickBooleanType CloneImageOptions(ImageInfo *image_info,
  const ImageInfo *clone_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(clone_info != (const ImageInfo *) NULL);
  assert(clone_info->signature == MagickSignature);
  if (clone_info->options != (void *) NULL)
    image_info->options=CloneSplayTree((SplayTreeInfo *) clone_info->options,
      (void *(*)(void *)) ConstantString,(void *(*)(void *)) ConstantString);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e f i n e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineImageOption() associates an assignment string of the form
%  "key=value" with an image option. It is equivelent to SetImageOption().
%
%  The format of the DefineImageOption method is:
%
%      MagickBooleanType DefineImageOption(ImageInfo *image_info,
%        const char *option)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o option: the image option assignment string.
%
*/
MagickExport MagickBooleanType DefineImageOption(ImageInfo *image_info,
  const char *option)
{
  char
    key[MaxTextExtent],
    value[MaxTextExtent];

  register char
    *p;

  assert(image_info != (ImageInfo *) NULL);
  assert(option != (const char *) NULL);
  (void) CopyMagickString(key,option,MaxTextExtent);
  for (p=key; *p != '\0'; p++)
    if (*p == '=')
      break;
  *value='\0';
  if (*p == '=')
    (void) CopyMagickString(value,p+1,MaxTextExtent);
  *p='\0';
  return(SetImageOption(image_info,key,value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteImageOption() deletes an key from the image map.
%
%  Returns MagickTrue is the option is found and deleted from the Options.
%
%  The format of the DeleteImageOption method is:
%
%      MagickBooleanType DeleteImageOption(ImageInfo *image_info,
%        const char *key)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o option: the image option.
%
*/
MagickExport MagickBooleanType DeleteImageOption(ImageInfo *image_info,
  const char *option)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return(MagickFalse);
  return(DeleteNodeFromSplayTree((SplayTreeInfo *) image_info->options,option));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e O p t i o n s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageOptions() releases memory associated with image option values.
%
%  The format of the DestroyDefines method is:
%
%      void DestroyImageOptions(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
MagickExport void DestroyImageOptions(ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options != (void *) NULL)
    image_info->options=DestroySplayTree((SplayTreeInfo *) image_info->options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e O p t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageOption() gets a value associated with an image option.
%
%  The format of the GetImageOption method is:
%
%      const char *GetImageOption(const ImageInfo *image_info,
%        const char *option)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o option: the option.
%
*/
MagickExport const char *GetImageOption(const ImageInfo *image_info,
  const char *option)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((const char *) NULL);
  return((const char *) GetValueFromSplayTree((SplayTreeInfo *)
    image_info->options,option));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o m m a n d O p t i o n F l a g s                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCommandOptionFlags() parses a string and returns an enumerated option
%  flags(s).  Return a value of -1 if no such option is found.
%
%  The format of the GetCommandOptionFlags method is:
%
%      ssize_t GetCommandOptionFlags(const CommandOption option,
%        const MagickBooleanType list,const char *options)
%
%  A description of each parameter follows:
%
%    o option: Index to the option table to lookup
%
%    o list: A option other than zero permits more than one option separated by
%      a comma or pipe.
%
%    o options: One or more options separated by commas.
%
*/

MagickExport ssize_t GetCommandOptionFlags(const CommandOption option,
  const MagickBooleanType list,const char *options)
{
  char
    token[MaxTextExtent];

  const OptionInfo
    *option_info;

  int
    sentinel;

  MagickBooleanType
    negate;

  register char
    *q;

  register const char
    *p;

  register ssize_t
    i;

  ssize_t
    option_types;

  option_info=GetOptionInfo(option);
  if (option_info == (const OptionInfo *) NULL)
    return(UndefinedOptionFlag);
  option_types=0;
  sentinel=',';
  if (strchr(options,'|') != (char *) NULL)
    sentinel='|';
  for (p=options; p != (char *) NULL; p=strchr(p,sentinel))
  {
    while (((isspace((int) ((unsigned char) *p)) != 0) || (*p == sentinel)) &&
           (*p != '\0'))
      p++;
    negate=(*p == '!') ? MagickTrue : MagickFalse;
    if (negate != MagickFalse)
      p++;
    q=token;
    while (((isspace((int) ((unsigned char) *p)) == 0) && (*p != sentinel)) &&
           (*p != '\0'))
    {
      if ((q-token) >= (MaxTextExtent-1))
        break;
      *q++=(*p++);
    }
    *q='\0';
    for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
      if (LocaleCompare(token,option_info[i].mnemonic) == 0)
        {
          if (*token == '!')
            option_types=option_types &~ option_info[i].flags;
          else
            option_types=option_types | option_info[i].flags;
          break;
        }
    if ((option_info[i].mnemonic == (char *) NULL) &&
        ((strchr(token+1,'-') != (char *) NULL) ||
         (strchr(token+1,'_') != (char *) NULL)))
      {
        while ((q=strchr(token+1,'-')) != (char *) NULL)
          (void) CopyMagickString(q,q+1,MaxTextExtent-strlen(q));
        while ((q=strchr(token+1,'_')) != (char *) NULL)
          (void) CopyMagickString(q,q+1,MaxTextExtent-strlen(q));
        for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
          if (LocaleCompare(token,option_info[i].mnemonic) == 0)
            {
              if (*token == '!')
                option_types=option_types &~ option_info[i].flags;
              else
                option_types=option_types | option_info[i].flags;
              break;
            }
      }
    if (option_info[i].mnemonic == (char *) NULL)
      return(-1);
    if (list == MagickFalse)
      break;
  }
  return(option_types);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o m m a n d O p t i o n I n f o                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCommandOptionInfo() returns pointer to the matching OptionInfo entry
%  for the "CommandOptions" table only. A specialised binary search is used,
%  to speed up the lookup for that very large table, and returns both the
%  type (arg count) and flags (arg type).
%
%  This search reduces linear search of over 500 options (250 tests of
%  average) to about 10 lookups!
%
%  The format of the GetCommandOptionInfo method is:
%
%      const char **GetCommandOptions(const CommandOption value)
%
%  A description of each parameter follows:
%
%    o value: the value.
%
*/
MagickExport const OptionInfo *GetCommandOptionInfo(const char *value)
{
  const OptionInfo
    *option_info=CommandOptions;

  static ssize_t
    table_size = 0;

  register int
    i,l,h;

  assert(value != (char *) NULL);
  assert(*value != '\0');

  /* count up table items - first time only */
  if ( table_size == 0 )
    {
      l=-1;
      for (i=0; option_info[i].mnemonic != (const char *) NULL; i++)
        if ( LocaleCompare(value,option_info[i].mnemonic) == 0 )
          l=i;
      table_size = i;
      return( &option_info[(l>=0)?l:i] );
    }

  /* faster binary search of command table, now that its length is known */
  l=0;
  h=table_size;
  while ( l < h )
  {
    int cmp;
    i = (l+h)/2; /* half the bounds */
    /* compare string part, then switch character! */
    cmp=LocaleCompare(value+1,option_info[i].mnemonic+1);
    if ( cmp == 0 )
      cmp = *value - *(option_info[i].mnemonic);
#if 0
    (void) FormatLocaleFile(stderr,
      "%d --- %u < %u < %u --- \"%s\" < \"%s\" < \"%s\"\n",
      cmp,l,i,h,option_info[l].mnemonic,option_info[i].mnemonic,
      option_info[h].mnemonic);
#endif
    if (cmp == 0)
      return(&option_info[i]);
    if (cmp > 0) l=i+1; else h=i;  /* reassign search bounds */
  }
  /* option was not found in table - return last 'null' entry. */
  return(&option_info[table_size]);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o m m a n d O p t i o n s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCommandOptions() returns a list of values.
%
%  The format of the GetCommandOptions method is:
%
%      const char **GetCommandOptions(const CommandOption value)
%
%  A description of each parameter follows:
%
%    o value: the value.
%
*/
MagickExport char **GetCommandOptions(const CommandOption value)
{
  char
    **values;

  const OptionInfo
    *option_info;

  register ssize_t
    i;

  option_info=GetOptionInfo(value);
  if (option_info == (const OptionInfo *) NULL)
    return((char **) NULL);
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++) ;
  values=(char **) AcquireQuantumMemory((size_t) i+1UL,sizeof(*values));
  if (values == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++)
    values[i]=AcquireString(option_info[i].mnemonic);
  values[i]=(char *) NULL;
  return(values);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t I m a g e O p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextImageOption() gets the next image option value.
%
%  The format of the GetNextImageOption method is:
%
%      char *GetNextImageOption(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
MagickExport char *GetNextImageOption(const ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((char *) NULL);
  return((char *) GetNextKeyInSplayTree((SplayTreeInfo *) image_info->options));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s C o m m a n d O p t i o n                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsCommandOption() returns MagickTrue if the option begins with a - or + and
%  the first character that follows is alphanumeric.
%
%  The format of the IsCommandOption method is:
%
%      MagickBooleanType IsCommandOption(const char *option)
%
%  A description of each parameter follows:
%
%    o option: the option.
%
*/
MagickExport MagickBooleanType IsCommandOption(const char *option)
{
  assert(option != (const char *) NULL);
  if ((*option != '-') && (*option != '+'))
    return(MagickFalse);
  if (strlen(option) == 1)
    return(MagickFalse);
  option++;
  if (isalpha((int) ((unsigned char) *option)) == 0)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m m a n d O p t i o n T o M n e m o n i c                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CommandOptionToMnemonic() returns an enumerated value as a mnemonic.
%
%  The format of the CommandOptionToMnemonic method is:
%
%      const char *CommandOptionToMnemonic(const CommandOption option,
%        const ssize_t type)
%
%  A description of each parameter follows:
%
%    o option: the option.
%
%    o type: one or more values separated by commas.
%
*/
MagickExport const char *CommandOptionToMnemonic(const CommandOption option,
  const ssize_t type)
{
  const OptionInfo
    *option_info;

  register ssize_t
    i;

  option_info=GetOptionInfo(option);
  if (option_info == (const OptionInfo *) NULL)
    return((const char *) NULL);
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++)
    if (type == option_info[i].type)
      break;
  if (option_info[i].mnemonic == (const char *) NULL)
    return("undefined");
  return(option_info[i].mnemonic);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L i s t C o m m a n d O p t i o n s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListCommandOptions() lists the contents of enumerated option type(s).
%
%  The format of the ListCommandOptions method is:
%
%      MagickBooleanType ListCommandOptions(FILE *file,
%        const CommandOption option,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o file:  list options to this file handle.
%
%    o option:  list these options.
%
%    o exception:  return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListCommandOptions(FILE *file,
  const CommandOption option,ExceptionInfo *magick_unused(exception))
{
  const OptionInfo
    *option_info;

  register ssize_t
    i;

  if (file == (FILE *) NULL)
    file=stdout;
  option_info=GetOptionInfo(option);
  if (option_info == (const OptionInfo *) NULL)
    return(MagickFalse);
  for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
  {
    if (option_info[i].stealth != MagickFalse)
      continue;
    (void) FormatLocaleFile(file,"%s\n",option_info[i].mnemonic);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e C h a n n e l O p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseChannelOption() parses a string and returns an enumerated channel
%  type(s).
%
%  The format of the ParseChannelOption method is:
%
%      ssize_t ParseChannelOption(const char *channels)
%
%  A description of each parameter follows:
%
%    o options: One or more values separated by commas.
%
*/
MagickExport ssize_t ParseChannelOption(const char *channels)
{
  register ssize_t
    i;

  ssize_t
    channel;

  channel=ParseCommandOption(MagickChannelOptions,MagickTrue,channels);
  if (channel >= 0)
    return(channel);
  channel=0;
  for (i=0; i < (ssize_t) strlen(channels); i++)
  {
    switch (channels[i])
    {
      case 'A':
      case 'a':
      {
        channel|=AlphaChannel;
        break;
      }
      case 'B':
      case 'b':
      {
        channel|=BlueChannel;
        break;
      }
      case 'C':
      case 'c':
      {
        channel|=CyanChannel;
        break;
      }
      case 'g':
      case 'G':
      {
        channel|=GreenChannel;
        break;
      }
      case 'K':
      case 'k':
      {
        channel|=BlackChannel;
        break;
      }
      case 'M':
      case 'm':
      {
        channel|=MagentaChannel;
        break;
      }
      case 'o':
      case 'O':
      {
        channel|=AlphaChannel; /* depreciate */
        break;
      }
      case 'R':
      case 'r':
      {
        channel|=RedChannel;
        break;
      }
      case 'Y':
      case 'y':
      {
        channel|=YellowChannel;
        break;
      }
      case ',':
      {
        ssize_t
          type;

        /*
          Gather the additional channel flags and merge with shorthand.
        */
        type=ParseCommandOption(MagickChannelOptions,MagickTrue,channels+i+1);
        if (type < 0)
          return(type);
        channel|=type;
        return(channel);
      }
      default:
        return(-1);
    }
  }
  return(channel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e C o m m a n d O p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseCommandOption() parses a string and returns an enumerated option
%  type(s).  Return a value of -1 if no such option is found.
%
%  The format of the ParseCommandOption method is:
%
%      ssize_t ParseCommandOption(const CommandOption option_table,
%        const MagickBooleanType list,const char *options)
%
%  A description of each parameter follows:
%
%    o option_table: Index to the option table to lookup
%
%    o list: A option other than zero permits more than one option separated by
%      a comma or pipe.
%
%    o options: One or more options separated by commas.
%
*/
MagickExport ssize_t ParseCommandOption(const CommandOption option_table,
  const MagickBooleanType list,const char *options)
{
  char
    token[MaxTextExtent];

  const OptionInfo
    *option_info;

  int
    sentinel;

  MagickBooleanType
    negate;

  register char
    *q;

  register const char
    *p;

  register ssize_t
    i;

  ssize_t
    option_types;

  option_info=GetOptionInfo(option_table);
  if (option_info == (const OptionInfo *) NULL)
    return(-1);
  option_types=0;
  sentinel=',';
  if (strchr(options,'|') != (char *) NULL)
    sentinel='|';
  for (p=options; p != (char *) NULL; p=strchr(p,sentinel))
  {
    while (((isspace((int) ((unsigned char) *p)) != 0) || (*p == sentinel)) &&
           (*p != '\0'))
      p++;
    negate=(*p == '!') ? MagickTrue : MagickFalse;
    if (negate != MagickFalse)
      p++;
    q=token;
    while (((isspace((int) ((unsigned char) *p)) == 0) && (*p != sentinel)) &&
           (*p != '\0'))
    {
      if ((q-token) >= (MaxTextExtent-1))
        break;
      *q++=(*p++);
    }
    *q='\0';
    for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
      if (LocaleCompare(token,option_info[i].mnemonic) == 0)
        {
          if (*token == '!')
            option_types=option_types &~ option_info[i].type;
          else
            option_types=option_types | option_info[i].type;
          break;
        }
    if ((option_info[i].mnemonic == (char *) NULL) &&
        ((strchr(token+1,'-') != (char *) NULL) ||
         (strchr(token+1,'_') != (char *) NULL)))
      {
        while ((q=strchr(token+1,'-')) != (char *) NULL)
          (void) CopyMagickString(q,q+1,MaxTextExtent-strlen(q));
        while ((q=strchr(token+1,'_')) != (char *) NULL)
          (void) CopyMagickString(q,q+1,MaxTextExtent-strlen(q));
        for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
          if (LocaleCompare(token,option_info[i].mnemonic) == 0)
            {
              if (*token == '!')
                option_types=option_types &~ option_info[i].type;
              else
                option_types=option_types | option_info[i].type;
              break;
            }
      }
    if (option_info[i].mnemonic == (char *) NULL)
      return(-1);
    if (list == MagickFalse)
      break;
  }
  return(option_types);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e P i x e l C h a n n e l O p t i o n                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParsePixelChannelOption() parses a string and returns an enumerated pixel
%  channel type(s).
%
%  The format of the ParsePixelChannelOption method is:
%
%      ssize_t ParsePixelChannelOption(const char *channels)
%
%  A description of each parameter follows:
%
%    o channels: One or more channels separated by commas.
%
*/
MagickExport ssize_t ParsePixelChannelOption(const char *channels)
{
  char
    *q,
    token[MaxTextExtent];

  ssize_t
    channel;

  GetMagickToken(channels,NULL,token);
  if ((*token == ';') || (*token == '|'))
    return(RedPixelChannel);
  channel=ParseCommandOption(MagickPixelChannelOptions,MagickTrue,token);
  if (channel >= 0)
    return(channel);
  q=(char *) token;
  channel=InterpretLocaleValue(token,&q);
  if ((q == token) || (channel < 0) || (channel >= MaxPixelChannels))
    return(-1);
  return(channel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveImageOption() removes an option from the image and returns its value.
%
%  The format of the RemoveImageOption method is:
%
%      char *RemoveImageOption(ImageInfo *image_info,const char *option)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o option: the image option.
%
*/
MagickExport char *RemoveImageOption(ImageInfo *image_info,const char *option)
{
  char
    *value;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((char *) NULL);
  value=(char *) RemoveNodeFromSplayTree((SplayTreeInfo *)
    image_info->options,option);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e O p t i o n                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImageOptions() resets the image_info option.  That is, it deletes
%  all options associated with the image_info structure.
%
%  The format of the ResetImageOptions method is:
%
%      ResetImageOptions(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
MagickExport void ResetImageOptions(const ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return;
  ResetSplayTree((SplayTreeInfo *) image_info->options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e O p t i o n I t e r a t o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImageOptionIterator() resets the image_info values iterator.  Use it
%  in conjunction with GetNextImageOption() to iterate over all the values
%  associated with an image option.
%
%  The format of the ResetImageOptionIterator method is:
%
%      ResetImageOptionIterator(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
MagickExport void ResetImageOptionIterator(const ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return;
  ResetSplayTreeIterator((SplayTreeInfo *) image_info->options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e O p t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageOption() associates an value with an image option.
%
%  The format of the SetImageOption method is:
%
%      MagickBooleanType SetImageOption(ImageInfo *image_info,
%        const char *option,const char *value)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o option: the image option.
%
%    o values: the image option values.
%
*/
MagickExport MagickBooleanType SetImageOption(ImageInfo *image_info,
  const char *option,const char *value)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);

  /* This should not be here! - but others might */
  if (LocaleCompare(option,"size") == 0)
    (void) CloneString(&image_info->size,value);

  /* create tree if needed - specify how key,values are to be freed */
  if (image_info->options == (void *) NULL)
    image_info->options=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,RelinquishMagickMemory);

  /* Delete Option if NULL */
  if (value == (const char *) NULL)
    return(DeleteImageOption(image_info,option));

  /* add option and return */
  return(AddValueToSplayTree((SplayTreeInfo *) image_info->options,
    ConstantString(option),ConstantString(value)));
}
