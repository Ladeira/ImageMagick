/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        CCCC   OOO   M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE         %
%       C      O   O  MM MM  P   P  O   O  SS       I      T    E             %
%       C      O   O  M M M  PPPP   O   O   SSS     I      T    EEE           %
%       C      O   O  M   M  P      O   O     SS    I      T    E             %
%        CCCC   OOO   M   M  P       OOO   SSSSS  IIIII    T    EEEEE         %
%                                                                             %
%                                                                             %
%                     MagickCore Image Composite Methods                      %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
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
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resample.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImage() returns the second image composited onto the first
%  at the specified offset, using the specified composite method.
%
%  The format of the CompositeImage method is:
%
%      MagickBooleanType CompositeImage(Image *image,
%        const Image *composite_image,const CompositeOperator compose,
%        const MagickBooleanType clip_to_self,const ssize_t x_offset,
%        const ssize_t y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the destination image, modified by he composition
%
%    o composite_image: the composite (source) image.
%
%    o compose: This operator affects how the composite is applied to
%      the image.  The operators and how they are utilized are listed here
%      http://www.w3.org/TR/SVG12/#compositing.
%
%    o clip_to_self: set to MagickTrue to limit composition to area composed.
%
%    o x_offset: the column offset of the composited image.
%
%    o y_offset: the row offset of the composited image.
%
%  Extra Controls from Image meta-data in 'composite_image' (artifacts)
%
%    o "compose:args"
%        A string containing extra numerical arguments for specific compose
%        methods, generally expressed as a 'geometry' or a comma separated list
%        of numbers.
%
%        Compose methods needing such arguments include "BlendCompositeOp" and
%        "DisplaceCompositeOp".
%
%    o exception: return any errors or warnings in this structure.
%
*/

/*
   Composition based on the SVG specification:

   A Composition is defined by...
      Color Function :  f(Sc,Dc)  where Sc and Dc are the normizalized colors
      Blending areas :  X = 1     for area of overlap, ie: f(Sc,Dc)
                        Y = 1     for source preserved
                        Z = 1     for destination preserved

   Conversion to transparency (then optimized)
      Dca' = f(Sc, Dc)*Sa*Da + Y*Sca*(1-Da) + Z*Dca*(1-Sa)
      Da'  = X*Sa*Da + Y*Sa*(1-Da) + Z*Da*(1-Sa)

   Where...
      Sca = Sc*Sa     normalized Source color divided by Source alpha
      Dca = Dc*Da     normalized Dest color divided by Dest alpha
      Dc' = Dca'/Da'  the desired color value for this channel.

   Da' in in the follow formula as 'gamma'  The resulting alpla value.

   Most functions use a blending mode of over (X=1,Y=1,Z=1) this results in
   the following optimizations...
      gamma = Sa+Da-Sa*Da;
      gamma = 1 - QuantiumScale*alpha * QuantiumScale*beta;
      opacity = QuantiumScale*alpha*beta;  // over blend, optimized 1-Gamma

   The above SVG definitions also definate that Mathematical Composition
   methods should use a 'Over' blending mode for Alpha Channel.
   It however was not applied for composition modes of 'Plus', 'Minus',
   the modulus versions of 'Add' and 'Subtract'.

   Mathematical operator changes to be applied from IM v6.7...

    1) Modulus modes 'Add' and 'Subtract' are obsoleted and renamed
       'ModulusAdd' and 'ModulusSubtract' for clarity.

    2) All mathematical compositions work as per the SVG specification
       with regard to blending.  This now includes 'ModulusAdd' and
       'ModulusSubtract'.

    3) When the special channel flag 'sync' (syncronize channel updates)
       is turned off (enabled by default) then mathematical compositions are
       only performed on the channels specified, and are applied
       independantally of each other.  In other words the mathematics is
       performed as 'pure' mathematical operations, rather than as image
       operations.
*/
static void CompositeHSB(const Quantum red,const Quantum green,
  const Quantum blue,double *hue,double *saturation,double *brightness)
{
  double
    delta;

  Quantum
    max,
    min;

  /*
    Convert RGB to HSB colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(brightness != (double *) NULL);
  max=(red > green ? red : green);
  if (blue > max)
    max=blue;
  min=(red < green ? red : green);
  if (blue < min)
    min=blue;
  *hue=0.0;
  *saturation=0.0;
  *brightness=(double) (QuantumScale*max);
  if (fabs((double) max) < MagickEpsilon)
    return;
  *saturation=(double) (1.0-min/max);
  delta=(MagickRealType) max-min;
  if (fabs(delta) < MagickEpsilon)
    return;
  if (fabs((double) red-max) < MagickEpsilon)
    *hue=(double) ((green-blue)/delta);
  else
    if (fabs((double) green-max) < MagickEpsilon)
      *hue=(double) (2.0+(blue-red)/delta);
    else
      if (fabs((double) blue-max) < MagickEpsilon)
        *hue=(double) (4.0+(red-green)/delta);
  *hue/=6.0;
  if (*hue < 0.0)
    *hue+=1.0;
}

static void HSBComposite(const double hue,const double saturation,
  const double brightness,double *red,double *green,double *blue)
{
  double
    f,
    h,
    p,
    q,
    t;

  /*
    Convert HSB to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  if (saturation == 0.0)
    {
      *red=(double) QuantumRange*brightness;
      *green=(*red);
      *blue=(*red);
      return;
    }
  h=6.0*(hue-floor(hue));
  f=h-floor((double) h);
  p=brightness*(1.0-saturation);
  q=brightness*(1.0-saturation*f);
  t=brightness*(1.0-saturation*(1.0-f));
  switch ((int) h)
  {
    case 0:
    default:
    {
      *red=(double) QuantumRange*brightness;
      *green=(double) QuantumRange*t;
      *blue=(double) QuantumRange*p;
      break;
    }
    case 1:
    {
      *red=(double) QuantumRange*q;
      *green=(double) QuantumRange*brightness;
      *blue=(double) QuantumRange*p;
      break;
    }
    case 2:
    {
      *red=(double) QuantumRange*p;
      *green=(double) QuantumRange*brightness;
      *blue=(double) QuantumRange*t;
      break;
    }
    case 3:
    {
      *red=(double) QuantumRange*p;
      *green=(double) QuantumRange*q;
      *blue=(double) QuantumRange*brightness;
      break;
    }
    case 4:
    {
      *red=(double) QuantumRange*t;
      *green=(double) QuantumRange*p;
      *blue=(double) QuantumRange*brightness;
      break;
    }
    case 5:
    {
      *red=(double) QuantumRange*brightness;
      *green=(double) QuantumRange*p;
      *blue=(double) QuantumRange*q;
      break;
    }
  }
}

static inline double MagickMin(const double x,const double y)
{
  if (x < y)
    return(x);
  return(y);
}
static inline double MagickMax(const double x,const double y)
{
  if (x > y)
    return(x);
  return(y);
}

static MagickBooleanType CompositeOverImage(Image *image,
  const Image *composite_image,const MagickBooleanType clip_to_self,
  const ssize_t x_offset,const ssize_t y_offset,ExceptionInfo *exception)
{
#define CompositeImageTag  "Composite/Image"

  CacheView
    *composite_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Composite image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireCacheView(image);
  composite_view=AcquireCacheView(composite_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static,4) shared(progress,status)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *pixels;

    register const Quantum
      *restrict p;

    register Quantum
      *restrict q;

    register ssize_t
      x;

    size_t
      channels;

    if (status == MagickFalse)
      continue;
    if (clip_to_self != MagickFalse)
      {
        if (y < y_offset)
          continue;
        if ((y-y_offset) >= (ssize_t) composite_image->rows)
          continue;
      }
    /*
      If pixels is NULL, y is outside overlay region.
    */
    pixels=(Quantum *) NULL;
    p=(Quantum *) NULL;
    if ((y >= y_offset) && ((y-y_offset) < (ssize_t) composite_image->rows))
      {
        p=GetCacheViewVirtualPixels(composite_view,0,y-y_offset,
          composite_image->columns,1,exception);
        if (p == (const Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        pixels=p;
        if (x_offset < 0)
          p-=x_offset*GetPixelChannels(composite_image);
      }
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      MagickRealType
        alpha,
        Da,
        Dc,
        gamma,
        Sa,
        Sc;

      register ssize_t
        i;

      if (clip_to_self != MagickFalse)
        {
          if (x < x_offset)
            {
              q+=GetPixelChannels(image);
              continue;
            }
          if ((x-x_offset) >= (ssize_t) composite_image->columns)
            break;
        }
      if ((pixels == (Quantum *) NULL) || (x < x_offset) ||
          ((x-x_offset) >= (ssize_t) composite_image->columns))
        {
          Quantum
            source[MaxPixelChannels];

          /*
            Virtual composite:
              Sc: source color.
              Dc: destination color.
          */
          if (GetPixelMask(image,q) != 0)
            {
              q+=GetPixelChannels(image);
              continue;
            }
          (void) GetOneVirtualPixel(composite_image,x-x_offset,y-y_offset,
            source,exception);
          for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
          {
            PixelChannel
              channel;

            PixelTrait
              composite_traits,
              traits;

            channel=GetPixelChannelMapChannel(image,i);
            traits=GetPixelChannelMapTraits(image,channel);
            composite_traits=GetPixelChannelMapTraits(composite_image,channel);
            if ((traits == UndefinedPixelTrait) ||
                (composite_traits == UndefinedPixelTrait))
              continue;
            q[i]=source[channel];
          }
          q+=GetPixelChannels(image);
          continue;
        }
      /*
        Authentic composite:
          Sa:  normalized source alpha.
          Da:  normalized destination alpha.
      */
      if (GetPixelMask(composite_image,p) != 0)
        {
          p+=GetPixelChannels(composite_image);
          channels=GetPixelChannels(composite_image);
          if (p >= (pixels+channels*composite_image->columns))
            p=pixels;
          q+=GetPixelChannels(image);
          continue;
        }
      Sa=QuantumScale*GetPixelAlpha(composite_image,p);
      Da=QuantumScale*GetPixelAlpha(image,q);
      alpha=Sa*(-Da)+Sa+Da;
      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        PixelChannel
          channel;

        PixelTrait
          composite_traits,
          traits;

        channel=GetPixelChannelMapChannel(image,i);
        traits=GetPixelChannelMapTraits(image,channel);
        composite_traits=GetPixelChannelMapTraits(composite_image,channel);
        if ((traits == UndefinedPixelTrait) ||
            (composite_traits == UndefinedPixelTrait))
          continue;
        if ((traits & CopyPixelTrait) != 0)
          {
            if (channel != AlphaPixelChannel)
              {
                /*
                  Copy channel.
                */
                q[i]=GetPixelChannel(composite_image,channel,p);
                continue;
              }
            /*
              Set alpha channel.
            */
            q[i]=ClampToQuantum(QuantumRange*alpha);
            continue;
          }
        /*
          Sc: source color.
          Dc: destination color.
        */
        Sc=(MagickRealType) GetPixelChannel(composite_image,channel,p);
        Dc=(MagickRealType) q[i];
        gamma=1.0/(fabs(alpha) <= MagickEpsilon ? 1.0 : alpha);
        q[i]=ClampToQuantum(gamma*(Sa*Sc-Sa*Da*Dc+Da*Dc));
      }
      p+=GetPixelChannels(composite_image);
      channels=GetPixelChannels(composite_image);
      if (p >= (pixels+channels*composite_image->columns))
        p=pixels;
      q+=GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp critical (MagickCore_CompositeImage)
#endif
        proceed=SetImageProgress(image,CompositeImageTag,progress++,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  composite_view=DestroyCacheView(composite_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

MagickExport MagickBooleanType CompositeImage(Image *image,
  const Image *composite_image,const CompositeOperator compose,
  const MagickBooleanType clip_to_self,const ssize_t x_offset,
  const ssize_t y_offset,ExceptionInfo *exception)
{
#define CompositeImageTag  "Composite/Image"

  CacheView
    *composite_view,
    *image_view;

  GeometryInfo
    geometry_info;

  Image
    *destination_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MagickRealType
    amount,
    destination_dissolve,
    midpoint,
    percent_brightness,
    percent_saturation,
    source_dissolve,
    threshold;

  MagickStatusType
    flags;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(composite_image != (Image *) NULL);
  assert(composite_image->signature == MagickSignature);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((IsGrayColorspace(image->colorspace) != MagickFalse) &&
      (IsGrayColorspace(composite_image->colorspace) == MagickFalse))
    (void) TransformImageColorspace(image,sRGBColorspace,exception);
  if ((compose == OverCompositeOp) || (compose == SrcOverCompositeOp))
    {
      status=CompositeOverImage(image,composite_image,clip_to_self,x_offset,
        y_offset,exception);
      return(status);
    }
  destination_image=(Image *) NULL;
  amount=0.5;
  destination_dissolve=1.0;
  percent_brightness=100.0;
  percent_saturation=100.0;
  source_dissolve=1.0;
  threshold=0.05f;
  switch (compose)
  {
    case CopyCompositeOp:
    {
      if ((x_offset < 0) || (y_offset < 0))
        break;
      if ((x_offset+(ssize_t) composite_image->columns) >= (ssize_t) image->columns)
        break;
      if ((y_offset+(ssize_t) composite_image->rows) >= (ssize_t) image->rows)
        break;
      status=MagickTrue;
      image_view=AcquireCacheView(image);
      composite_view=AcquireCacheView(composite_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static,4) shared(status)
#endif
      for (y=0; y < (ssize_t) composite_image->rows; y++)
      {
        MagickBooleanType
          sync;

        register const Quantum
          *p;

        register Quantum
          *q;

        register ssize_t
          x;

        if (status == MagickFalse)
          continue;
        p=GetCacheViewVirtualPixels(composite_view,0,y,composite_image->columns,
          1,exception);
        q=GetCacheViewAuthenticPixels(image_view,x_offset,y+y_offset,
          composite_image->columns,1,exception);
        if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
          {
            status=MagickFalse;
            continue;
          }
        for (x=0; x < (ssize_t) composite_image->columns; x++)
        {
          register ssize_t
            i;

          if (GetPixelMask(image,p) != 0)
            {
              p+=GetPixelChannels(composite_image);
              q+=GetPixelChannels(image);
              continue;
            }
          for (i=0; i < (ssize_t) GetPixelChannels(composite_image); i++)
          {
            PixelChannel
              channel;

            PixelTrait
              composite_traits,
              traits;

            channel=GetPixelChannelMapChannel(composite_image,i);
            composite_traits=GetPixelChannelMapTraits(composite_image,channel);
            traits=GetPixelChannelMapTraits(image,channel);
            if ((traits == UndefinedPixelTrait) ||
                (composite_traits == UndefinedPixelTrait))
              continue;
            SetPixelChannel(image,channel,p[i],q);
          }
          p+=GetPixelChannels(composite_image);
          q+=GetPixelChannels(image);
        }
        sync=SyncCacheViewAuthenticPixels(image_view,exception);
        if (sync == MagickFalse)
          status=MagickFalse;
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            MagickBooleanType
              proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
            #pragma omp critical (MagickCore_CompositeImage)
#endif
            proceed=SetImageProgress(image,CompositeImageTag,
              (MagickOffsetType) y,image->rows);
            if (proceed == MagickFalse)
              status=MagickFalse;
          }
      }
      composite_view=DestroyCacheView(composite_view);
      image_view=DestroyCacheView(image_view);
      return(status);
    }
    case CopyAlphaCompositeOp:
    case ChangeMaskCompositeOp:
    case IntensityCompositeOp:
    {
      /*
        Modify destination outside the overlaid region and require an alpha
        channel to exist, to add transparency.
      */
      if (image->matte == MagickFalse)
        (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      break;
    }
    case BlurCompositeOp:
    {
      CacheView
        *composite_view,
        *destination_view;

      const char
        *value;

      PixelInfo
        pixel;

      MagickRealType
        angle_range,
        angle_start,
        height,
        width;

      ResampleFilter
        *resample_filter;

      SegmentInfo
        blur;

      /*
        Blur Image dictated by an overlay gradient map: X = red_channel;
          Y = green_channel; compose:args =  x_scale[,y_scale[,angle]].
      */
      destination_image=CloneImage(image,image->columns,image->rows,MagickTrue,
        exception);
      if (destination_image == (Image *) NULL)
        return(MagickFalse);
      /*
        Determine the horizontal and vertical maximim blur.
      */
      SetGeometryInfo(&geometry_info);
      flags=NoValue;
      value=GetImageArtifact(composite_image,"compose:args");
      if (value != (char *) NULL)
        flags=ParseGeometry(value,&geometry_info);
      if ((flags & WidthValue) == 0 )
        {
          destination_image=DestroyImage(destination_image);
          return(MagickFalse);
        }
      width=geometry_info.rho;
      height=geometry_info.sigma;
      blur.x1=geometry_info.rho;
      blur.x2=0.0;
      blur.y1=0.0;
      blur.y2=geometry_info.sigma;
      angle_start=0.0;
      angle_range=0.0;
      if ((flags & HeightValue) == 0)
        blur.y2=blur.x1;
      if ((flags & XValue) != 0 )
        {
          MagickRealType
            angle;

          angle=DegreesToRadians(geometry_info.xi);
          blur.x1=width*cos(angle);
          blur.x2=width*sin(angle);
          blur.y1=(-height*sin(angle));
          blur.y2=height*cos(angle);
        }
      if ((flags & YValue) != 0 )
        {
          angle_start=DegreesToRadians(geometry_info.xi);
          angle_range=DegreesToRadians(geometry_info.psi)-angle_start;
        }
      /*
        Blur Image by resampling.
        FUTURE: this is currently broken, especially for small sigma blurs
        This needs to be fixed to use a non-user filter setup that provides
        far more control than currently available.
      */
      resample_filter=AcquireResampleFilter(image,exception);
      SetResampleFilter(resample_filter,CubicFilter); /* was blur*2 */
      destination_view=AcquireCacheView(destination_image);
      composite_view=AcquireCacheView(composite_image);
      for (y=0; y < (ssize_t) composite_image->rows; y++)
      {
        MagickBooleanType
          sync;

        register const Quantum
          *restrict p;

        register Quantum
          *restrict q;

        register ssize_t
          x;

        if (((y+y_offset) < 0) || ((y+y_offset) >= (ssize_t) image->rows))
          continue;
        p=GetCacheViewVirtualPixels(composite_view,0,y,composite_image->columns,
          1,exception);
        q=QueueCacheViewAuthenticPixels(destination_view,0,y,
          destination_image->columns,1,exception);
        if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
          break;
        for (x=0; x < (ssize_t) composite_image->columns; x++)
        {
          if (((x_offset+x) < 0) || ((x_offset+x) >= (ssize_t) image->columns))
            {
              p+=GetPixelChannels(composite_image);
              continue;
            }
          if (fabs(angle_range) > MagickEpsilon)
            {
              MagickRealType
                angle;

              angle=angle_start+angle_range*QuantumScale*
                GetPixelBlue(composite_image,p);
              blur.x1=width*cos(angle);
              blur.x2=width*sin(angle);
              blur.y1=(-height*sin(angle));
              blur.y2=height*cos(angle);
            }
          ScaleResampleFilter(resample_filter,blur.x1*QuantumScale*
            GetPixelRed(composite_image,p),blur.y1*QuantumScale*
            GetPixelGreen(composite_image,p),blur.x2*QuantumScale*
            GetPixelRed(composite_image,p),blur.y2*QuantumScale*
            GetPixelGreen(composite_image,p));
          (void) ResamplePixelColor(resample_filter,(double) x_offset+x,
            (double) y_offset+y,&pixel);
          SetPixelInfoPixel(destination_image,&pixel,q);
          p+=GetPixelChannels(composite_image);
          q+=GetPixelChannels(destination_image);
        }
        sync=SyncCacheViewAuthenticPixels(destination_view,exception);
        if (sync == MagickFalse)
          break;
      }
      resample_filter=DestroyResampleFilter(resample_filter);
      composite_view=DestroyCacheView(composite_view);
      destination_view=DestroyCacheView(destination_view);
      composite_image=destination_image;
      break;
    }
    case DisplaceCompositeOp:
    case DistortCompositeOp:
    {
      CacheView
        *composite_view,
        *destination_view,
        *image_view;

      const char
        *value;

      PixelInfo
        pixel;

      MagickRealType
        horizontal_scale,
        vertical_scale;

      PointInfo
        center,
        offset;

      /*
        Displace/Distort based on overlay gradient map:
          X = red_channel;  Y = green_channel;
          compose:args = x_scale[,y_scale[,center.x,center.y]]
      */
      destination_image=CloneImage(image,image->columns,image->rows,MagickTrue,
        exception);
      if (destination_image == (Image *) NULL)
        return(MagickFalse);
      SetGeometryInfo(&geometry_info);
      flags=NoValue;
      value=GetImageArtifact(composite_image,"compose:args");
      if (value != (char *) NULL)
        flags=ParseGeometry(value,&geometry_info);
      if ((flags & (WidthValue|HeightValue)) == 0 )
        {
          if ((flags & AspectValue) == 0)
            {
              horizontal_scale=(MagickRealType) (composite_image->columns-1.0)/
                2.0;
              vertical_scale=(MagickRealType) (composite_image->rows-1.0)/2.0;
            }
          else
            {
              horizontal_scale=(MagickRealType) (image->columns-1.0)/2.0;
              vertical_scale=(MagickRealType) (image->rows-1.0)/2.0;
            }
        }
      else
        {
          horizontal_scale=geometry_info.rho;
          vertical_scale=geometry_info.sigma;
          if ((flags & PercentValue) != 0)
            {
              if ((flags & AspectValue) == 0)
                {
                  horizontal_scale*=(composite_image->columns-1.0)/200.0;
                  vertical_scale*=(composite_image->rows-1.0)/200.0;
                }
              else
                {
                  horizontal_scale*=(image->columns-1.0)/200.0;
                  vertical_scale*=(image->rows-1.0)/200.0;
                }
            }
          if ((flags & HeightValue) == 0)
            vertical_scale=horizontal_scale;
        }
      /*
        Determine fixed center point for absolute distortion map
         Absolute distort ==
           Displace offset relative to a fixed absolute point
           Select that point according to +X+Y user inputs.
           default = center of overlay image
           arg flag '!' = locations/percentage relative to background image
      */
      center.x=(MagickRealType) x_offset;
      center.y=(MagickRealType) y_offset;
      if (compose == DistortCompositeOp)
        {
          if ((flags & XValue) == 0)
            if ((flags & AspectValue) == 0)
              center.x=(MagickRealType) x_offset+(composite_image->columns-1)/
                2.0;
            else
              center.x=((MagickRealType) image->columns-1)/2.0;
          else
            if ((flags & AspectValue) == 0)
              center.x=(MagickRealType) x_offset+geometry_info.xi;
            else
              center.x=geometry_info.xi;
          if ((flags & YValue) == 0)
            if ((flags & AspectValue) == 0)
              center.y=(MagickRealType) y_offset+(composite_image->rows-1)/2.0;
            else
              center.y=((MagickRealType) image->rows-1)/2.0;
          else
            if ((flags & AspectValue) == 0)
              center.y=(MagickRealType) y_offset+geometry_info.psi;
            else
              center.y=geometry_info.psi;
        }
      /*
        Shift the pixel offset point as defined by the provided,
        displacement/distortion map.  -- Like a lens...
      */
      GetPixelInfo(image,&pixel);
      image_view=AcquireCacheView(image);
      destination_view=AcquireCacheView(destination_image);
      composite_view=AcquireCacheView(composite_image);
      for (y=0; y < (ssize_t) composite_image->rows; y++)
      {
        MagickBooleanType
          sync;

        register const Quantum
          *restrict p;

        register Quantum
          *restrict q;

        register ssize_t
          x;

        if (((y+y_offset) < 0) || ((y+y_offset) >= (ssize_t) image->rows))
          continue;
        p=GetCacheViewVirtualPixels(composite_view,0,y,composite_image->columns,
          1,exception);
        q=QueueCacheViewAuthenticPixels(destination_view,0,y,
          destination_image->columns,1,exception);
        if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
          break;
        for (x=0; x < (ssize_t) composite_image->columns; x++)
        {
          if (((x_offset+x) < 0) || ((x_offset+x) >= (ssize_t) image->columns))
            {
              p+=GetPixelChannels(composite_image);
              continue;
            }
          /*
            Displace the offset.
          */
          offset.x=(horizontal_scale*(GetPixelRed(composite_image,p)-
            (((MagickRealType) QuantumRange+1.0)/2.0)))/(((MagickRealType)
            QuantumRange+1.0)/2.0)+center.x+((compose == DisplaceCompositeOp) ?
            x : 0);
          offset.y=(vertical_scale*(GetPixelGreen(composite_image,p)-
            (((MagickRealType) QuantumRange+1.0)/2.0)))/(((MagickRealType)
            QuantumRange+1.0)/2.0)+center.y+((compose == DisplaceCompositeOp) ?
            y : 0);
          (void) InterpolatePixelInfo(image,image_view,
            UndefinedInterpolatePixel,(double) offset.x,(double) offset.y,
            &pixel,exception);
          /*
            Mask with the 'invalid pixel mask' in alpha channel.
          */
          pixel.alpha=(MagickRealType) QuantumRange*(1.0-(1.0-QuantumScale*
            pixel.alpha)*(1.0-QuantumScale*GetPixelAlpha(composite_image,p)));
          SetPixelInfoPixel(destination_image,&pixel,q);
          p+=GetPixelChannels(composite_image);
          q+=GetPixelChannels(destination_image);
        }
        sync=SyncCacheViewAuthenticPixels(destination_view,exception);
        if (sync == MagickFalse)
          break;
      }
      destination_view=DestroyCacheView(destination_view);
      composite_view=DestroyCacheView(composite_view);
      image_view=DestroyCacheView(image_view);
      composite_image=destination_image;
      break;
    }
    case DissolveCompositeOp:
    {
      const char
        *value;

      /*
        Geometry arguments to dissolve factors.
      */
      value=GetImageArtifact(composite_image,"compose:args");
      if (value != (char *) NULL)
        {
          flags=ParseGeometry(value,&geometry_info);
          source_dissolve=geometry_info.rho/100.0;
          destination_dissolve=1.0;
          if ((source_dissolve-MagickEpsilon) < 0.0)
            source_dissolve=0.0;
          if ((source_dissolve+MagickEpsilon) > 1.0)
            {
              destination_dissolve=2.0-source_dissolve;
              source_dissolve=1.0;
            }
          if ((flags & SigmaValue) != 0)
            destination_dissolve=geometry_info.sigma/100.0;
          if ((destination_dissolve-MagickEpsilon) < 0.0)
            destination_dissolve=0.0;
        }
      break;
    }
    case BlendCompositeOp:
    {
      const char
        *value;

      value=GetImageArtifact(composite_image,"compose:args");
      if (value != (char *) NULL)
        {
          flags=ParseGeometry(value,&geometry_info);
          source_dissolve=geometry_info.rho/100.0;
          destination_dissolve=1.0-source_dissolve;
          if ((flags & SigmaValue) != 0)
            destination_dissolve=geometry_info.sigma/100.0;
        }
      break;
    }
    case MathematicsCompositeOp:
    {
      const char
        *value;

      /*
        Just collect the values from "compose:args", setting.
        Unused values are set to zero automagically.

        Arguments are normally a comma separated list, so this probably should
        be changed to some 'general comma list' parser, (with a minimum
        number of values)
      */
      SetGeometryInfo(&geometry_info);
      value=GetImageArtifact(composite_image,"compose:args");
      if (value != (char *) NULL)
        (void) ParseGeometry(value,&geometry_info);
      break;
    }
    case ModulateCompositeOp:
    {
      const char
        *value;

      /*
        Determine the brightness and saturation scale.
      */
      value=GetImageArtifact(composite_image,"compose:args");
      if (value != (char *) NULL)
        {
          flags=ParseGeometry(value,&geometry_info);
          percent_brightness=geometry_info.rho;
          if ((flags & SigmaValue) != 0)
            percent_saturation=geometry_info.sigma;
        }
      break;
    }
    case ThresholdCompositeOp:
    {
      const char
        *value;

      /*
        Determine the amount and threshold.
      */
      value=GetImageArtifact(composite_image,"compose:args");
      if (value != (char *) NULL)
        {
          flags=ParseGeometry(value,&geometry_info);
          amount=geometry_info.rho;
          threshold=geometry_info.sigma;
          if ((flags & SigmaValue) == 0)
            threshold=0.05f;
        }
      threshold*=QuantumRange;
      break;
    }
    default:
      break;
  }
  /*
    Composite image.
  */
  status=MagickTrue;
  progress=0;
  midpoint=((MagickRealType) QuantumRange+1.0)/2;
  image_view=AcquireCacheView(image);
  composite_view=AcquireCacheView(composite_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static,4) shared(progress,status)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *pixels;

    double
      blue,
      brightness,
      green,
      hue,
      red,
      saturation;

    register const Quantum
      *restrict p;

    register Quantum
      *restrict q;

    register ssize_t
      x;

    if (status == MagickFalse)
      continue;
    if (clip_to_self != MagickFalse)
      {
        if (y < y_offset)
          continue;
        if ((y-y_offset) >= (ssize_t) composite_image->rows)
          continue;
      }
    /*
      If pixels is NULL, y is outside overlay region.
    */
    pixels=(Quantum *) NULL;
    p=(Quantum *) NULL;
    if ((y >= y_offset) && ((y-y_offset) < (ssize_t) composite_image->rows))
      {
        p=GetCacheViewVirtualPixels(composite_view,0,y-y_offset,
          composite_image->columns,1,exception);
        if (p == (const Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        pixels=p;
        if (x_offset < 0)
          p-=x_offset*GetPixelChannels(composite_image);
      }
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    hue=0.0;
    saturation=0.0;
    brightness=0.0;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      MagickRealType
        alpha,
        Da,
        Dc,
        Dca,
        gamma,
        Sa,
        Sc,
        Sca;

      register ssize_t
        i;

      size_t
        channels;

      if (clip_to_self != MagickFalse)
        {
          if (x < x_offset)
            {
              q+=GetPixelChannels(image);
              continue;
            }
          if ((x-x_offset) >= (ssize_t) composite_image->columns)
            break;
        }
      if ((pixels == (Quantum *) NULL) || (x < x_offset) ||
          ((x-x_offset) >= (ssize_t) composite_image->columns))
        {
          Quantum
            source[MaxPixelChannels];

          /*
            Virtual composite:
              Sc: source color.
              Dc: destination color.
          */
          (void) GetOneVirtualPixel(composite_image,x-x_offset,y-y_offset,
            source,exception);
          if (GetPixelMask(image,q) != 0)
            {
              q+=GetPixelChannels(image);
              continue;
            }
          for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
          {
            MagickRealType
              pixel;

            PixelChannel
              channel;

            PixelTrait
              composite_traits,
              traits;

            channel=GetPixelChannelMapChannel(image,i);
            traits=GetPixelChannelMapTraits(image,channel);
            composite_traits=GetPixelChannelMapTraits(composite_image,channel);
            if ((traits == UndefinedPixelTrait) ||
                (composite_traits == UndefinedPixelTrait))
              continue;
            switch (compose)
            {
              case AlphaCompositeOp:
              case ChangeMaskCompositeOp:
              case CopyAlphaCompositeOp:
              case DstAtopCompositeOp:
              case DstInCompositeOp:
              case InCompositeOp:
              case IntensityCompositeOp:
              case OutCompositeOp:
              case SrcInCompositeOp:
              case SrcOutCompositeOp:
              {
                pixel=(MagickRealType) q[i];
                if (channel == AlphaPixelChannel)
                  pixel=(MagickRealType) TransparentAlpha;
                break;
              }
              case ClearCompositeOp:
              case CopyCompositeOp:
              case ReplaceCompositeOp:
              case SrcCompositeOp:
              {
                if (channel == AlphaPixelChannel)
                  {
                    pixel=(MagickRealType) TransparentAlpha;
                    break;
                  }
                pixel=0.0;
                break;
              }
              case BlendCompositeOp:
              case DissolveCompositeOp:
              {
                if (channel == AlphaPixelChannel)
                  {
                    pixel=destination_dissolve*GetPixelAlpha(composite_image,
                      source);
                    break;
                  }
                pixel=(MagickRealType) source[channel];
                break;
              }
              default:
              {
                pixel=(MagickRealType) source[channel];
                break;
              }
            }
            q[i]=ClampToQuantum(pixel);
          }
          q+=GetPixelChannels(image);
          continue;
        }
      /*
        Authentic composite:
          Sa:  normalized source alpha.
          Da:  normalized destination alpha.
      */
      Sa=QuantumScale*GetPixelAlpha(composite_image,p);
      Da=QuantumScale*GetPixelAlpha(image,q);
      switch (compose)
      {
        case BumpmapCompositeOp:
        {
          alpha=GetPixelIntensity(composite_image,p)*Sa;
          break;
        }
        case ColorBurnCompositeOp:
        case ColorDodgeCompositeOp:
        case DifferenceCompositeOp:
        case DivideDstCompositeOp:
        case DivideSrcCompositeOp:
        case ExclusionCompositeOp:
        case HardLightCompositeOp:
        case LinearBurnCompositeOp:
        case LinearDodgeCompositeOp:
        case LinearLightCompositeOp:
        case MathematicsCompositeOp:
        case MinusDstCompositeOp:
        case MinusSrcCompositeOp:
        case ModulusAddCompositeOp:
        case ModulusSubtractCompositeOp:
        case MultiplyCompositeOp:
        case OverlayCompositeOp:
        case PegtopLightCompositeOp:
        case PinLightCompositeOp:
        case ScreenCompositeOp:
        case SoftLightCompositeOp:
        case VividLightCompositeOp:
        {
          alpha=RoundToUnity(Sa+Da-Sa*Da);
          break;
        }
        case DarkenCompositeOp:
        case DstAtopCompositeOp:
        case DstInCompositeOp:
        case InCompositeOp:
        case LightenCompositeOp:
        case SrcInCompositeOp:
        {
          alpha=Sa*Da;
          break;
        }
        case DissolveCompositeOp:
        {
          alpha=source_dissolve*Sa*(-destination_dissolve*Da)+source_dissolve*
            Sa+destination_dissolve*Da;
          break;
        }
        case DstOverCompositeOp:
        {
          alpha=Da*(-Sa)+Da+Sa;
          break;
        }
        case DstOutCompositeOp:
        {
          alpha=Da*(1.0-Sa);
          break;
        }
        case OutCompositeOp:
        case SrcOutCompositeOp:
        {
          alpha=Sa*(1.0-Da);
          break;
        }
        case OverCompositeOp:
        case SrcOverCompositeOp:
        {
          alpha=Sa*(-Da)+Sa+Da;
          break;
        }
        case BlendCompositeOp:
        case PlusCompositeOp:
        {
          alpha=RoundToUnity(Sa+Da);
          break;
        }
        case XorCompositeOp:
        {
          alpha=Sa+Da-2.0*Sa*Da;
          break;
        }
        default:
        {
          alpha=1.0;
          break;
        }
      }
      if (GetPixelMask(image,p) != 0)
        {
          p+=GetPixelChannels(composite_image);
          q+=GetPixelChannels(image);
          continue;
        }
      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        double
          sans;

        MagickRealType
          pixel;

        PixelChannel
          channel;

        PixelTrait
          composite_traits,
          traits;

        channel=GetPixelChannelMapChannel(image,i);
        traits=GetPixelChannelMapTraits(image,channel);
        composite_traits=GetPixelChannelMapTraits(composite_image,channel);
        if (traits == UndefinedPixelTrait)
          continue;
        if ((compose != IntensityCompositeOp) &&
            (composite_traits == UndefinedPixelTrait))
          continue;
        /*
          Sc: source color.
          Dc: destination color.
        */
        Sc=(MagickRealType) GetPixelChannel(composite_image,channel,p);
        Dc=(MagickRealType) q[i];
        if ((traits & CopyPixelTrait) != 0)
          {
            if (channel != AlphaPixelChannel)
              {
                /*
                  Copy channel.
                */
                q[i]=ClampToQuantum(Sc);
                continue;
              }
            /*
              Set alpha channel.
            */
            switch (compose)
            {
              case AlphaCompositeOp:
              {
                pixel=QuantumRange*Sa;
                break;
              }
              case AtopCompositeOp:
              case CopyBlackCompositeOp:
              case CopyBlueCompositeOp:
              case CopyCyanCompositeOp:
              case CopyGreenCompositeOp:
              case CopyMagentaCompositeOp:
              case CopyRedCompositeOp:
              case CopyYellowCompositeOp:
              case SrcAtopCompositeOp:
              case DstCompositeOp:
              case NoCompositeOp:
              {
                pixel=QuantumRange*Da;
                break;
              }
              case ChangeMaskCompositeOp:
              {
                MagickBooleanType
                  equivalent;

                if (Da > ((MagickRealType) QuantumRange/2.0))
                  {
                    pixel=(MagickRealType) TransparentAlpha;
                    break;
                  }
                equivalent=IsFuzzyEquivalencePixel(composite_image,p,image,q);
                if (equivalent != MagickFalse)
                  {
                    pixel=(MagickRealType) TransparentAlpha;
                    break;
                  }
                pixel=(MagickRealType) OpaqueAlpha;
                break;
              }
              case ClearCompositeOp:
              {
                pixel=(MagickRealType) TransparentAlpha;
                break;
              }
              case ColorizeCompositeOp:
              case HueCompositeOp:
              case LuminizeCompositeOp:
              case SaturateCompositeOp:
              {
                if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
                  {
                    pixel=QuantumRange*Da;
                    break;
                  }
                if (fabs(QuantumRange*Da-TransparentAlpha) < MagickEpsilon)
                  {
                    pixel=QuantumRange*Sa;
                    break;
                  }
                if (Sa < Da)
                  {
                    pixel=QuantumRange*Da;
                    break;
                  }
                pixel=QuantumRange*Sa;
                break;
              }
              case CopyCompositeOp:
              case CopyAlphaCompositeOp:
              case DisplaceCompositeOp:
              case DistortCompositeOp:
              case DstAtopCompositeOp:
              case ReplaceCompositeOp:
              case SrcCompositeOp:
              {
                pixel=QuantumRange*Sa;
                break;
              }
              case DarkenIntensityCompositeOp:
              {
                pixel=(1.0-Sa)*GetPixelIntensity(composite_image,p) <
                  (1.0-Da)*GetPixelIntensity(image,q) ? Sa : Da;
                break;
              }
              case IntensityCompositeOp:
              {
                pixel=(MagickRealType) GetPixelIntensity(composite_image,p);
                break;
              }
              case LightenIntensityCompositeOp:
              {
                pixel=Sa*GetPixelIntensity(composite_image,p) >
                  Da*GetPixelIntensity(image,q) ? Sa : Da;
                break;
              }
              case ModulateCompositeOp:
              {
                if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
                  {
                    pixel=QuantumRange*Da;
                    break;
                  }
                pixel=QuantumRange*Da;
                break;
              }
              default:
              {
                pixel=QuantumRange*alpha;
                break;
              }
            }
            q[i]=ClampToQuantum(pixel);
            continue;
          }
        /*
          Porter-Duff compositions:
            Sca: source normalized color multiplied by alpha.
            Dca: normalized destination color multiplied by alpha.
        */
        Sca=QuantumScale*Sa*Sc;
        Dca=QuantumScale*Da*Dc;
        switch (compose)
        {
          case DarkenCompositeOp:
          case LightenCompositeOp:
          case ModulusSubtractCompositeOp:
          {
            gamma=1.0-alpha;
            break;
          }
          default:
            break;
        }
        gamma=1.0/(fabs(alpha) <= MagickEpsilon ? 1.0 : alpha);
        pixel=Dc;
        switch (compose)
        {
          case AlphaCompositeOp:
          {
            pixel=QuantumRange*Sa;
            break;
          }
          case AtopCompositeOp:
          case SrcAtopCompositeOp:
          {
            pixel=Sc*Sa+Dc*(1.0-Sa);
            break;
          }
          case BlendCompositeOp:
          {
            pixel=gamma*(source_dissolve*Sa*Sc+destination_dissolve*Da*Dc);
            break;
          }
          case BlurCompositeOp:
          case DisplaceCompositeOp:
          case DistortCompositeOp:
          case CopyCompositeOp:
          case ReplaceCompositeOp:
          case SrcCompositeOp:
          {
            pixel=Sc;
            break;
          }
          case BumpmapCompositeOp:
          {
            if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Dc;
                break;
              }
            pixel=QuantumScale*GetPixelIntensity(composite_image,p)*Dc;
            break;
          }
          case ChangeMaskCompositeOp:
          {
            pixel=Dc;
            break;
          }
          case ClearCompositeOp:
          {
            pixel=0.0;
            break;
          }
          case ColorBurnCompositeOp:
          {
            /*
              Refer to the March 2009 SVG specification.
            */
            if ((fabs(Sca) < MagickEpsilon) && (fabs(Dca-Da) < MagickEpsilon))
              {
                pixel=QuantumRange*gamma*(Sa*Da+Dca*(1.0-Sa));
                break;
              }
            if (Sca < MagickEpsilon)
              {
                pixel=QuantumRange*gamma*(Dca*(1.0-Sa));
                break;
              }
            pixel=QuantumRange*gamma*(Sa*Da-Sa*MagickMin(Da,(Da-Dca)*Sa/Sca)+
              Sca*(1.0-Da)+Dca*(1.0-Sa));
            break;
          }
          case ColorDodgeCompositeOp:
          {
            if ((fabs(Sca-Sa) < MagickEpsilon) && (fabs(Dca) < MagickEpsilon))
              {
                pixel=QuantumRange*gamma*(Sca*(1.0-Da)+Dca*(1.0-Sa));
                break;
              }
            if (fabs(Sca-Sa) < MagickEpsilon)
              {
                pixel=QuantumRange*gamma*(Sa*Da+Sca*(1.0-Da)+Dca*(1.0-Sa));
                break;
              }
            pixel=QuantumRange*gamma*(Dca*Sa*Sa/(Sa-Sca)+Sca*(1.0-Da)+Dca*
              (1.0-Sa));
            break;
          }
          case ColorizeCompositeOp:
          {
            if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Dc;
                break;
              }
            if (fabs(QuantumRange*Da-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Sc;
                break;
              }
            CompositeHSB(GetPixelRed(image,q),GetPixelGreen(image,q),
              GetPixelBlue(image,q),&sans,&sans,&brightness);
            CompositeHSB(GetPixelRed(composite_image,p),
              GetPixelGreen(composite_image,p),GetPixelBlue(composite_image,p),
              &hue,&saturation,&sans);
            HSBComposite(hue,saturation,brightness,&red,&green,&blue);
            switch (channel)
            {
              case RedPixelChannel: pixel=red; break;
              case GreenPixelChannel: pixel=green; break;
              case BluePixelChannel: pixel=blue; break;
              default: pixel=Dc; break;
            }
            break;
          }
          case CopyAlphaCompositeOp:
          case IntensityCompositeOp:
          {
            if (channel == AlphaPixelChannel)
              pixel=(MagickRealType) GetPixelAlpha(composite_image,p);
            break;
          }
          case CopyBlackCompositeOp:
          {
            if (channel == BlackPixelChannel)
              pixel=(MagickRealType) GetPixelBlack(composite_image,p);
            break;
          }
          case CopyBlueCompositeOp:
          case CopyYellowCompositeOp:
          {
            if (channel == BluePixelChannel)
              pixel=(MagickRealType) GetPixelBlue(composite_image,p);
            break;
          }
          case CopyGreenCompositeOp:
          case CopyMagentaCompositeOp:
          {
            if (channel == GreenPixelChannel)
              pixel=(MagickRealType) GetPixelGreen(composite_image,p);
            break;
          }
          case CopyRedCompositeOp:
          case CopyCyanCompositeOp:
          {
            if (channel == RedPixelChannel)
              pixel=(MagickRealType) GetPixelRed(composite_image,p);
            break;
          }
          case DarkenCompositeOp:
          {
            /*
              Darken is equivalent to a 'Minimum' method
                OR a greyscale version of a binary 'Or'
                OR the 'Intersection' of pixel sets.
            */
            if (Sc < Dc)
              {
                pixel=gamma*(Sa*Sc-Sa*Da*Dc+Da*Dc);
                break;
              }
            pixel=gamma*(Da*Dc-Da*Sa*Sc+Sa*Sc);
            break;
          }
          case DarkenIntensityCompositeOp:
          {
            pixel=(1.0-Sa)*GetPixelIntensity(composite_image,p) <
              (1.0-Da)*GetPixelIntensity(image,q) ? Sc : Dc;
            break;
          }
          case DifferenceCompositeOp:
          {
            pixel=gamma*(Sa*Sc+Da*Dc-Sa*Da*2.0*MagickMin(Sc,Dc));
            break;
          }
          case DissolveCompositeOp:
          {
            pixel=gamma*(source_dissolve*Sa*Sc-source_dissolve*Sa*
              destination_dissolve*Da*Dc+destination_dissolve*Da*Dc);
            break;
          }
          case DivideDstCompositeOp:
          {
            if ((fabs(Sca) < MagickEpsilon) && (fabs(Dca) < MagickEpsilon))
              {
                pixel=QuantumRange*gamma*(Sca*(1.0-Da)+Dca*(1.0-Sa));
                break;
              }
            if (fabs(Dca) < MagickEpsilon)
              {
                pixel=QuantumRange*gamma*(Sa*Da+Sca*(1.0-Da)+Dca*(1.0-Sa));
                break;
              }
            pixel=QuantumRange*gamma*(Sca*Da*Da/Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
            break;
          }
          case DivideSrcCompositeOp:
          {
            if ((fabs(Dca) < MagickEpsilon) && (fabs(Sca) < MagickEpsilon))
              {
                pixel=QuantumRange*gamma*(Dca*(1.0-Sa)+Sca*(1.0-Da));
                break;
              }
            if (fabs(Sca) < MagickEpsilon)
              {
                pixel=QuantumRange*gamma*(Da*Sa+Dca*(1.0-Sa)+Sca*(1.0-Da));
                break;
              }
            pixel=QuantumRange*gamma*(Dca*Sa*Sa/Sca+Dca*(1.0-Sa)+Sca*(1.0-Da));
            break;
          }
          case DstAtopCompositeOp:
          {
            pixel=Dc*Da+Sc*(1.0-Da);
            break;
          }
          case DstCompositeOp:
          case NoCompositeOp:
          {
            pixel=Dc;
            break;
          }
          case DstInCompositeOp:
          {
            pixel=gamma*(Sa*Dc*Sa);
            break;
          }
          case DstOutCompositeOp:
          {
            pixel=gamma*(Da*Dc*(1.0-Sa));
            break;
          }
          case DstOverCompositeOp:
          {
            pixel=gamma*(Da*Dc-Da*Sa*Sc+Sa*Sc);
            break;
          }
          case ExclusionCompositeOp:
          {
            pixel=QuantumRange*gamma*(Sca*Da+Dca*Sa-2.0*Sca*Dca+Sca*(1.0-Da)+
              Dca*(1.0-Sa));
            break;
          }
          case HardLightCompositeOp:
          {
            if ((2.0*Sca) < Sa)
              {
                pixel=QuantumRange*gamma*(2.0*Sca*Dca+Sca*(1.0-Da)+Dca*
                  (1.0-Sa));
                break;
              }
            pixel=QuantumRange*gamma*(Sa*Da-2.0*(Da-Dca)*(Sa-Sca)+Sca*(1.0-Da)+
              Dca*(1.0-Sa));
            break;
          }
          case HueCompositeOp:
          {
            if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Dc;
                break;
              }
            if (fabs(QuantumRange*Da-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Sc;
                break;
              }
            CompositeHSB(GetPixelRed(image,q),GetPixelGreen(image,q),
              GetPixelBlue(image,q),&hue,&saturation,&brightness);
            CompositeHSB(GetPixelRed(composite_image,p),
              GetPixelGreen(composite_image,p),GetPixelBlue(composite_image,p),
              &hue,&sans,&sans);
            HSBComposite(hue,saturation,brightness,&red,&green,&blue);
            switch (channel)
            {
              case RedPixelChannel: pixel=red; break;
              case GreenPixelChannel: pixel=green; break;
              case BluePixelChannel: pixel=blue; break;
              default: pixel=Dc; break;
            }
            break;
          }
          case InCompositeOp:
          case SrcInCompositeOp:
          {
            pixel=gamma*(Da*Sc*Da);
            break;
          }
          case LinearBurnCompositeOp:
          {
            /*
              LinearBurn: as defined by Abode Photoshop, according to
              http://www.simplefilter.de/en/basics/mixmods.html is:

                f(Sc,Dc) = Sc + Dc - 1
            */
            pixel=QuantumRange*gamma*(Sca+Dca-Sa*Da);
            break;
          }
          case LinearDodgeCompositeOp:
          {
            pixel=QuantumRange*gamma*(Sa*Sc+Da*Dc);
            break;
          }
          case LinearLightCompositeOp:
          {
            /*
              LinearLight: as defined by Abode Photoshop, according to
              http://www.simplefilter.de/en/basics/mixmods.html is:

                f(Sc,Dc) = Dc + 2*Sc - 1
            */
            pixel=QuantumRange*gamma*((Sca-Sa)*Da+Sca+Dca);
            break;
          }
          case LightenCompositeOp:
          {
            if (Sc > Dc)
              {
                pixel=QuantumRange*gamma*(Sa*Sc-Sa*Da*Dc+Da*Dc);
                break;
              }
            pixel=QuantumRange*gamma*(Da*Dc-Da*Sa*Sc+Sa*Sc);
            break;
          }
          case LightenIntensityCompositeOp:
          {
            /*
              Lighten is equivalent to a 'Maximum' method
                OR a greyscale version of a binary 'And'
                OR the 'Union' of pixel sets.
            */
            pixel=Sa*GetPixelIntensity(composite_image,p) >
              Da*GetPixelIntensity(image,q) ? Sc : Dc;
            break;
          }
          case LuminizeCompositeOp:
          {
            if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Dc;
                break;
              }
            if (fabs(QuantumRange*Da-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Sc;
                break;
              }
            CompositeHSB(GetPixelRed(image,q),GetPixelGreen(image,q),
              GetPixelBlue(image,q),&hue,&saturation,&brightness);
            CompositeHSB(GetPixelRed(composite_image,p),
              GetPixelGreen(composite_image,p),GetPixelBlue(composite_image,p),
              &sans,&sans,&brightness);
            HSBComposite(hue,saturation,brightness,&red,&green,&blue);
            switch (channel)
            {
              case RedPixelChannel: pixel=red; break;
              case GreenPixelChannel: pixel=green; break;
              case BluePixelChannel: pixel=blue; break;
              default: pixel=Dc; break;
            }
            break;
          }
          case MathematicsCompositeOp:
          {
            /*
              'Mathematics' a free form user control mathematical composition
              is defined as...

                f(Sc,Dc) = A*Sc*Dc + B*Sc + C*Dc + D

              Where the arguments A,B,C,D are (currently) passed to composite
              as a command separated 'geometry' string in "compose:args" image
              artifact.

                 A = a->rho,   B = a->sigma,  C = a->xi,  D = a->psi

              Applying the SVG transparency formula (see above), we get...

               Dca' = Sa*Da*f(Sc,Dc) + Sca*(1.0-Da) + Dca*(1.0-Sa)

               Dca' = A*Sca*Dca + B*Sca*Da + C*Dca*Sa + D*Sa*Da + Sca*(1.0-Da) +
                 Dca*(1.0-Sa)
            */
            pixel=gamma*geometry_info.rho*Sa*Sc*Da*Dc+geometry_info.sigma*
              Sa*Sc*Da+geometry_info.xi*Da*Dc*Sa+geometry_info.psi*Sa*Da+
              Sa*Sc*(1.0-Da)+Da*Dc*(1.0-Sa);
            break;
          }
          case MinusDstCompositeOp:
          {
            pixel=gamma*(Sa*Sc+Da*Dc-2.0*Da*Dc*Sa);
            break;
          }
          case MinusSrcCompositeOp:
          {
            /*
              Minus source from destination.

                f(Sc,Dc) = Sc - Dc
            */
            pixel=QuantumRange*gamma*(Da*Dc+Sa*Sc-2.0*Sa*Sc*Da);
            break;
          }
          case ModulateCompositeOp:
          {
            ssize_t
              offset;

            if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Dc;
                break;
              }
            offset=(ssize_t) (GetPixelIntensity(composite_image,p)-midpoint);
            if (offset == 0)
              {
                pixel=Dc;
                break;
              }
            CompositeHSB(GetPixelRed(image,q),GetPixelGreen(image,q),
              GetPixelBlue(image,q),&hue,&saturation,&brightness);
            brightness+=(0.01*percent_brightness*offset)/midpoint;
            saturation*=0.01*percent_saturation;
            HSBComposite(hue,saturation,brightness,&red,&green,&blue);
            switch (channel)
            {
              case RedPixelChannel: pixel=red; break;
              case GreenPixelChannel: pixel=green; break;
              case BluePixelChannel: pixel=blue; break;
              default: pixel=Dc; break;
            }
            break;
          }
          case ModulusAddCompositeOp:
          {
            pixel=Sc+Dc;
            if (pixel > QuantumRange)
              pixel-=(QuantumRange+1.0);
            pixel=gamma*(pixel*Sa*Da+Sa*Sc*(1.0-Da)+Da*Dc*(1.0-Sa));
            break;
          }
          case ModulusSubtractCompositeOp:
          {
            pixel=Sc-Dc;
            if (pixel < 0.0)
              pixel+=(QuantumRange+1.0);
            pixel=gamma*(pixel*Sa*Da+Sa*Sc*(1.0-Da)+Da*Dc*(1.0-Sa));
            break;
          }
          case MultiplyCompositeOp:
          {
            pixel=QuantumRange*gamma*(Sca*Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
            break;
          }
          case OutCompositeOp:
          case SrcOutCompositeOp:
          {
            pixel=gamma*(Sa*Sc*(1.0-Da));
            break;
          }
          case OverCompositeOp:
          case SrcOverCompositeOp:
          {
            pixel=QuantumRange*gamma*(Sa*Sc-Sa*Da*Dc+Da*Dc);
            break;
          }
          case OverlayCompositeOp:
          {
            if ((2.0*Dca) < Da)
              {
                pixel=QuantumRange*gamma*(2.0*Dca*Sca+Dca*(1.0-Sa)+Sca*
                  (1.0-Da));
                break;
              }
            pixel=QuantumRange*gamma*(Da*Sa-2.0*(Sa-Sca)*(Da-Dca)+Dca*(1.0-Sa)+
              Sca*(1.0-Da));
            break;
          }
          case PegtopLightCompositeOp:
          {
            /*
              PegTop: A Soft-Light alternative: A continuous version of the
              Softlight function, producing very similar results.

                f(Sc,Dc) = Dc^2*(1-2*Sc) + 2*Sc*Dc

              http://www.pegtop.net/delphi/articles/blendmodes/softlight.htm.
            */
            if (fabs(Da) < MagickEpsilon)
              {
                pixel=QuantumRange*gamma*(Sca);
                break;
              }
            pixel=QuantumRange*gamma*(Dca*Dca*(Sa-2.0*Sca)/Da+Sca*(2.0*Dca+1.0-
              Da)+Dca*(1.0-Sa));
            break;
          }
          case PinLightCompositeOp:
          {
            /*
              PinLight: A Photoshop 7 composition method
              http://www.simplefilter.de/en/basics/mixmods.html

                f(Sc,Dc) = Dc<2*Sc-1 ? 2*Sc-1 : Dc>2*Sc   ? 2*Sc : Dc
            */
            if ((Dca*Sa) < (Da*(2.0*Sca-Sa)))
              {
                pixel=QuantumRange*gamma*(Sca*(Da+1.0)-Sa*Da+Dca*(1.0-Sa));
                break;
              }
            if ((Dca*Sa) > (2.0*Sca*Da))
              {
                pixel=QuantumRange*gamma*(Sca*Da+Sca+Dca*(1.0-Sa));
                break;
              }
            pixel=QuantumRange*gamma*(Sca*(1.0-Da)+Dca);
            break;
          }
          case PlusCompositeOp:
          {
            pixel=QuantumRange*gamma*(Sa*Sc+Da*Dc);
            break;
          }
          case SaturateCompositeOp:
          {
            if (fabs(QuantumRange*Sa-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Dc;
                break;
              }
            if (fabs(QuantumRange*Da-TransparentAlpha) < MagickEpsilon)
              {
                pixel=Sc;
                break;
              }
            CompositeHSB(GetPixelRed(image,q),GetPixelGreen(image,q),
              GetPixelBlue(image,q),&hue,&saturation,&brightness);
            CompositeHSB(GetPixelRed(composite_image,p),
              GetPixelGreen(composite_image,p),GetPixelBlue(composite_image,p),
              &sans,&saturation,&sans);
            HSBComposite(hue,saturation,brightness,&red,&green,&blue);
            switch (channel)
            {
              case RedPixelChannel: pixel=red; break;
              case GreenPixelChannel: pixel=green; break;
              case BluePixelChannel: pixel=blue; break;
              default: pixel=Dc; break;
            }
            break;
          }
          case ScreenCompositeOp:
          {
            /*
              Screen:  a negated multiply:

                f(Sc,Dc) = 1.0-(1.0-Sc)*(1.0-Dc)
            */
            pixel=QuantumRange*gamma*(Sca+Dca-Sca*Dca);
            break;
          }
          case SoftLightCompositeOp:
          {
            /*
              Refer to the March 2009 SVG specification.
            */
            if ((2.0*Sca) < Sa)
              {
                pixel=QuantumRange*gamma*(Dca*(Sa+(2.0*Sca-Sa)*(1.0-(Dca/Da)))+
                  Sca*(1.0-Da)+Dca*(1.0-Sa));
                break;
              }
            if (((2.0*Sca) > Sa) && ((4.0*Dca) <= Da))
              {
                pixel=QuantumRange*gamma*(Dca*Sa+Da*(2.0*Sca-Sa)*(4.0*(Dca/Da)*
                  (4.0*(Dca/Da)+1.0)*((Dca/Da)-1.0)+7.0*(Dca/Da))+Sca*(1.0-Da)+
                  Dca*(1.0-Sa));
                break;
              }
            pixel=QuantumRange*gamma*(Dca*Sa+Da*(2.0*Sca-Sa)*(pow((Dca/Da),0.5)-
              (Dca/Da))+Sca*(1.0-Da)+Dca*(1.0-Sa));
            break;
          }
          case ThresholdCompositeOp:
          {
            MagickRealType
              delta;

            delta=Sc-Dc;
            if ((MagickRealType) fabs((double) (2.0*delta)) < threshold)
              {
                pixel=gamma*Dc;
                break;
              }
            pixel=gamma*(Dc+delta*amount);
            break;
          }
          case VividLightCompositeOp:
          {
            /*
              VividLight: A Photoshop 7 composition method.  See
              http://www.simplefilter.de/en/basics/mixmods.html.

                f(Sc,Dc) = (2*Sc < 1) ? 1-(1-Dc)/(2*Sc) : Dc/(2*(1-Sc))
            */
            if ((fabs(Sa) < MagickEpsilon) || (fabs(Sca-Sa) < MagickEpsilon))
              {
                pixel=QuantumRange*gamma*(Sa*Da+Sca*(1.0-Da)+Dca*(1.0-Sa));
                break;
              }
            if ((2.0*Sca) <= Sa)
              {
                pixel=QuantumRange*gamma*(Sa*(Da+Sa*(Dca-Da)/(2.0*Sca))+Sca*
                  (1.0-Da)+Dca*(1.0-Sa));
                break;
              }
            pixel=QuantumRange*gamma*(Dca*Sa*Sa/(2.0*(Sa-Sca))+Sca*(1.0-Da)+
              Dca*(1.0-Sa));
            break;
          }
          case XorCompositeOp:
          {
            pixel=QuantumRange*gamma*(Sc*Sa*(1.0-Da)+Dc*Da*(1.0-Sa));
            break;
          }
          default:
          {
            pixel=Sc;
            break;
          }
        }
        q[i]=ClampToQuantum(pixel);
      }
      p+=GetPixelChannels(composite_image);
      channels=GetPixelChannels(composite_image);
      if (p >= (pixels+channels*composite_image->columns))
        p=pixels;
      q+=GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp critical (MagickCore_CompositeImage)
#endif
        proceed=SetImageProgress(image,CompositeImageTag,progress++,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  composite_view=DestroyCacheView(composite_view);
  image_view=DestroyCacheView(image_view);
  if (destination_image != (Image * ) NULL)
    destination_image=DestroyImage(destination_image);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T e x t u r e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TextureImage() repeatedly tiles the texture image across and down the image
%  canvas.
%
%  The format of the TextureImage method is:
%
%      MagickBooleanType TextureImage(Image *image,const Image *texture,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o texture_image: This image is the texture to layer on the background.
%
*/
MagickExport MagickBooleanType TextureImage(Image *image,const Image *texture,
  ExceptionInfo *exception)
{
#define TextureImageTag  "Texture/Image"

  CacheView
    *image_view,
    *texture_view;

  Image
    *texture_image;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (texture == (const Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  texture_image=CloneImage(texture,0,0,MagickTrue,exception);
  if (texture_image == (const Image *) NULL)
    return(MagickFalse);
  (void) SetImageVirtualPixelMethod(texture_image,TileVirtualPixelMethod,
    exception);
  status=MagickTrue;
  if ((image->compose != CopyCompositeOp) &&
      ((image->compose != OverCompositeOp) || (image->matte != MagickFalse) ||
       (texture_image->matte != MagickFalse)))
    {
      /*
        Tile texture onto the image background.
      */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status)
#endif
      for (y=0; y < (ssize_t) image->rows; y+=(ssize_t) texture_image->rows)
      {
        register ssize_t
          x;

        if (status == MagickFalse)
          continue;
        for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) texture_image->columns)
        {
          MagickBooleanType
            thread_status;

          thread_status=CompositeImage(image,texture_image,image->compose,
            MagickFalse,x+texture_image->tile_offset.x,y+
            texture_image->tile_offset.y,exception);
          if (thread_status == MagickFalse)
            {
              status=thread_status;
              break;
            }
        }
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            MagickBooleanType
              proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
           #pragma omp critical (MagickCore_TextureImage)
#endif
            proceed=SetImageProgress(image,TextureImageTag,(MagickOffsetType)
              y,image->rows);
            if (proceed == MagickFalse)
              status=MagickFalse;
          }
      }
      (void) SetImageProgress(image,TextureImageTag,(MagickOffsetType)
        image->rows,image->rows);
      texture_image=DestroyImage(texture_image);
      return(status);
    }
  /*
    Tile texture onto the image background (optimized).
  */
  status=MagickTrue;
  image_view=AcquireCacheView(image);
  texture_view=AcquireCacheView(texture_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    MagickBooleanType
      sync;

    register const Quantum
      *p,
      *pixels;

    register ssize_t
      x;

    register Quantum
      *q;

    size_t
      width;

    if (status == MagickFalse)
      continue;
    pixels=GetCacheViewVirtualPixels(texture_view,texture_image->tile_offset.x,
      (y+texture_image->tile_offset.y) % texture_image->rows,
      texture_image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if ((pixels == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) texture_image->columns)
    {
      register ssize_t
        j;

      p=pixels;
      width=texture_image->columns;
      if ((x+(ssize_t) width) > (ssize_t) image->columns)
        width=image->columns-x;
      for (j=0; j < (ssize_t) width; j++)
      {
        register ssize_t
          i;

        if (GetPixelMask(image,p) != 0)
          {
            p+=GetPixelChannels(texture_image);
            q+=GetPixelChannels(image);
            continue;
          }
        for (i=0; i < (ssize_t) GetPixelChannels(texture_image); i++)
        {
          PixelChannel
            channel;

          PixelTrait
            texture_traits,
            traits;

          channel=GetPixelChannelMapChannel(texture_image,i);
          texture_traits=GetPixelChannelMapTraits(texture_image,channel);
          traits=GetPixelChannelMapTraits(image,channel);
          if ((traits == UndefinedPixelTrait) ||
              (texture_traits == UndefinedPixelTrait))
            continue;
          SetPixelChannel(image,channel,p[i],q);
        }
        p+=GetPixelChannels(texture_image);
        q+=GetPixelChannels(image);
      }
    }
    sync=SyncCacheViewAuthenticPixels(image_view,exception);
    if (sync == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp critical (MagickCore_TextureImage)
#endif
        proceed=SetImageProgress(image,TextureImageTag,(MagickOffsetType) y,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  texture_view=DestroyCacheView(texture_view);
  image_view=DestroyCacheView(image_view);
  texture_image=DestroyImage(texture_image);
  return(status);
}
