''
''
'' GdiplusGpStubs -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __win_GdiplusGpStubs_bi__
#define __win_GdiplusGpStubs_bi__

#include once "windows.bi"
#include once "GdiplusTypes.bi"

#ifndef __FB_GDIPLUS_NAMESPACE_ACTIVE__
#define __FB_GDIPLUS_NAMESPACE_ACTIVE__
#define __FB_GDIPLUS_LOCAL_NAMESPACE__
namespace Gdiplus
#endif

'' The C++ binding uses forward declarations for these handles.  FreeBASIC
'' needs a concrete placeholder before it can form pointers to the handles.
type GpPath_ as any
type GpFontFamily_ as any
type GpStringFormat_ as any
type GpPen_ as any
type GpGraphics_ as any
type GpPathIterator_ as any
type GpRegion_ as any
type GpBrush_ as any
type GpHatch_ as any
type GpSolidFill_ as any
type GpImage_ as any
type GpTexture_ as any
type GpImageAttributes_ as any
type GpLineGradient_ as any
type GpPathGradient_ as any
type GpCustomLineCap_ as any
type GpAdjustableArrowCap_ as any
type GpBitmap_ as any
type GpMetafile_ as any
type GpFontCollection_ as any
type GpFont_ as any
type GpCachedBitmap_ as any

type GpPath as GpPath_
type GpFontFamily as GpFontFamily_
type GpStringFormat as GpStringFormat_
type GpPen as GpPen_
type GpGraphics as GpGraphics_
type GpPathIterator as GpPathIterator_
type GpRegion as GpRegion_
type GpBrush as GpBrush_
type GpHatch as GpHatch_
type GpSolidFill as GpSolidFill_
type GpImage as GpImage_
type GpTexture as GpTexture_
type GpImageAttributes as GpImageAttributes_
type GpLineGradient as GpLineGradient_
type GpPathGradient as GpPathGradient_
type GpCustomLineCap as GpCustomLineCap_
type GpAdjustableArrowCap as GpAdjustableArrowCap_
type GpBitmap as GpBitmap_
type GpMetafile as GpMetafile_
type GpFontCollection as GpFontCollection_
type GpFont as GpFont_
type GpCachedBitmap as GpCachedBitmap_

type GpStatus as Status
type GpFillMode as FillMode
type GpWrapMode as WrapMode
type GpUnit as Unit
type GpCoordinateSpace as CoordinateSpace
type GpPointF as PointF
type GpPoint as Point
type GpRectF as RectF
type GpRect as Rect
type GpSizeF as SizeF
type GpHatchStyle as HatchStyle
type GpDashStyle as DashStyle
type GpLineCap as LineCap
type GpDashCap as DashCap
type GpPenAlignment as PenAlignment
type GpLineJoin as LineJoin
type GpPenType as PenType
type GpMatrix as Matrix
type GpBrushType as BrushType
type GpMatrixOrder as MatrixOrder
type GpFlushIntention as FlushIntention
type GpPathData as PathData

#ifdef __FB_GDIPLUS_LOCAL_NAMESPACE__
end namespace
#undef __FB_GDIPLUS_LOCAL_NAMESPACE__
#undef __FB_GDIPLUS_NAMESPACE_ACTIVE__
#endif

#endif
