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

  MagickCore types.
*/
#ifndef _MAGICKCORE_MAGICK_TYPE_H
#define _MAGICKCORE_MAGICK_TYPE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "MagickCore/magick-config.h"

#if !defined(MAGICKCORE_QUANTUM_DEPTH)
#define MAGICKCORE_QUANTUM_DEPTH  16
#endif

#if defined(MAGICKCORE_WINDOWS_SUPPORT) && !defined(__MINGW32__)
#  define MagickLLConstant(c)  (MagickOffsetType) (c ## i64)
#  define MagickULLConstant(c)  (MagickSizeType) (c ## ui64)
#else
#  define MagickLLConstant(c)  (MagickOffsetType) (c ## LL)
#  define MagickULLConstant(c)  (MagickSizeType) (c ## ULL)
#endif

#if (MAGICKCORE_QUANTUM_DEPTH == 8)
#define MagickEpsilon  1.0e-6
#define MaxColormapSize  256UL
#define MaxMap  255UL

#if defined __arm__ || defined __thumb__
typedef float MagickRealType;
#else
typedef double MagickRealType;
#endif
#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef float Quantum;
#define QuantumRange  255.0
#define QuantumFormat  "%g"
#else
typedef unsigned char Quantum;
#define QuantumRange  255
#define QuantumFormat  "%u"
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
#define MagickEpsilon  1.0e-10
#define MaxColormapSize  65536UL
#define MaxMap  65535UL

#if defined __arm__ || defined __thumb__
typedef float MagickRealType;
#else
typedef double MagickRealType;
#endif
#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef float Quantum;
#define QuantumRange  65535.0
#define QuantumFormat  "%g"
#else
typedef unsigned short Quantum;
#define QuantumRange  65535
#define QuantumFormat  "%u"
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 32)
#define MagickEpsilon  1.0e-10
#define MaxColormapSize  65536UL
#define MaxMap  65535UL

typedef double MagickRealType;
#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef float Quantum;
#define QuantumRange  4294967295.0
#define QuantumFormat  "%g"
#else
typedef unsigned int Quantum;
#define QuantumRange  4294967295
#define QuantumFormat  "%u"
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 64) && defined(MAGICKCORE_HAVE_LONG_DOUBLE_WIDER)
#define MagickEpsilon  1.0e-10
#define MaxColormapSize  65536UL
#define MaxMap  65535UL

typedef long double MagickRealType;
typedef double Quantum;
#define QuantumRange  18446744073709551615.0
#define QuantumFormat  "%g"
#else
#if !defined(_CH_)
# error "MAGICKCORE_QUANTUM_DEPTH must be one of 8, 16, 32, or 64"
#endif
#endif
#define MagickHuge  3.402823466E+38F
#define MagickPI  3.14159265358979323846264338327950288419716939937510L
#define QuantumScale  ((double) 1.0/(double) QuantumRange)

/*
  Typedef declarations.
*/
typedef unsigned int MagickStatusType;
#if !defined(MAGICKCORE_WINDOWS_SUPPORT)
#if (MAGICKCORE_SIZEOF_UNSIGNED_LONG_LONG == 8)
typedef long long MagickOffsetType;
typedef unsigned long long MagickSizeType;
#define MagickOffsetFormat  "lld"
#define MagickSizeFormat  "llu"
#else
typedef ssize_t MagickOffsetType;
typedef size_t MagickSizeType;
#define MagickOffsetFormat  "ld"
#define MagickSizeFormat  "lu"
#endif
#else
typedef __int64 MagickOffsetType;
typedef unsigned __int64 MagickSizeType;
#define MagickOffsetFormat  "I64i"
#define MagickSizeFormat  "I64u"
#endif

#if QuantumDepth > 16
  typedef double SignedQuantum;
#else
  typedef ssize_t SignedQuantum;
#endif

#if defined(_MSC_VER) && (_MSC_VER == 1200)
typedef MagickOffsetType QuantumAny;
#else
typedef MagickSizeType QuantumAny;
#endif

#if defined(macintosh)
#define ExceptionInfo  MagickExceptionInfo
#endif

typedef enum
{
  UndefinedClass,
  DirectClass,
  PseudoClass
} ClassType;

typedef enum
{
  MagickFalse = 0,
  MagickTrue = 1
} MagickBooleanType;

/*
   Define some short-hand macros for handling MagickBooleanType
   Some of these assume MagickBooleanType uses values 0 and 1,
   and uses fast C typing with C boolean operations

     Is  -- returns MagickBooleanType
     If  -- returns C integer boolean (for if's and while's)

   IsTrue()  converts a C integer boolean to a MagickBooleanType
   IsFalse() is a  MagickBooleanType 'not' operation

   IfTrue()   converts MagickBooleanType to C integer Boolean
   IfFalse()  Not the MagickBooleanType to C integer Boolean

   IsNULL() and IsNotNULL() converts C pointers to MagickBooleanType
*/
#if 1
#  define IsMagickTrue(v)  ((MagickBooleanType)((int)(v)!= 0))
#  define IsMagickFalse(v) ((MagickBooleanType)(!(int)(v)))
#  define IfMagickTrue(v)  ((int)(v))
#  define IfMagickFalse(v) (!(int)(v))
#else
#  define IsMagickTrue(v)  (((MagickBooleanType)(v))!=MagickFalse?MagickTrue:MagickFalse)
#  define IsMagickFalse(v) (((MagickBooleanType)(v))==MagickFalse?MagickTrue:MagickFalse)
#  define IfMagickTrue(v)  ((v) != MagickFalse)
#  define IfMagickFalse(v) ((v) == MagickFalse)
#endif
#define IsMagickNULL(v) (((void *)(v) == NULL)?MagickTrue:MagickFalse)
#define IsMagickNotNULL(v) (((void *)(v) != NULL)?MagickTrue:MagickFalse)

typedef struct _BlobInfo BlobInfo;

typedef struct _ExceptionInfo ExceptionInfo;

typedef struct _Image Image;

typedef struct _ImageInfo ImageInfo;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
