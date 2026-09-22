/'
    NeHe BMP texture loader

    Resource ownership:

    LoadBMP owns the file handle and temporary image storage until it returns.
    A nonzero result transfers the image record and its RGB buffer to the
    caller, which releases both after the OpenGL upload.

    Only packed BITMAPFILEHEADER/BITMAPINFOHEADER records and uncompressed
    8-bit or 24-bit BI_RGB pixels are accepted here.
'/

#ifndef __nehe_bmpload_bi__
#define __nehe_bmpload_bi__

const BITMAP_ID = &H4D42

'' Layout: bytes 0-39 are the fixed-size BMP DIB information header.
type BITMAPINFOHEADER Field = 1
  biSize          as ulong
  biWidth         as long
  biHeight        as long
  biPlanes        as ushort
  biBitCount      as ushort
  biCompression   as ulong
  biSizeImage     as ulong
  biXPelsPerMeter as long
  biYPelsPerMeter as long
  biClrUsed       as ulong
  biClrImportant  as ulong
end type

type PALETTEENTRY Field = 1
  peRed   as ubyte
  peGreen as ubyte
  peBlue  as ubyte
  peFlags as ubyte
end type

'' Layout: bytes 0-13 are the fixed-size BMP file header.
type BITMAPFILEHEADER FIELD = 1
  bfType as ushort
  bfSize as ulong
  bfReserved1 as ushort
  bfReserved2 as ushort
  bfOffBits as ulong
end type

'This is a replacement for AUX_RGBImageRec because GLAUX lib is not part of
'the freebasic install (perhaps not worth having for only one useful function).
type BITMAP_RGBImageRec FIELD = 1
  sizeX as integer
  sizeY as integer
  buffer as ubyte ptr
end type


declare function LoadBMP(byref Filename as string) as BITMAP_RGBImageRec ptr

private sub StoreRGB(byref destination as ubyte ptr, byval red as ubyte, _
  byval green as ubyte, byval blue as ubyte)

  if destination = 0 then exit sub
  destination[0] = red
  destination[1] = green
  destination[2] = blue
  destination += 3
end sub


'' Loads an uncompressed 8-bit or 24-bit BMP into an RGB buffer.
'' The validation branches map directly to distinct BMP header constraints.
'' FB-LINTER: DISABLE-NEXT-LINE FBL111
private function LoadBMP(byref Filename as string) as BITMAP_RGBImageRec ptr
  const MAX_BITMAP_BYTES as longint = 2147483647

  dim bitmapfileheader as BITMAPFILEHEADER
  dim bitmapinfoheader as BITMAPINFOHEADER
  dim bmpalette(0 to 255) as PALETTEENTRY

  dim index as integer
  dim rowIndex as integer
  dim paddingIndex as integer
  dim imageWidth as integer
  dim imageHeight as integer
  dim noofpixels as integer
  dim rgbBytes as integer
  dim sourceRowBytes as integer
  dim sourceRowStride as integer
  dim paletteEntries as integer
  dim fileOpen as integer
  dim fileSize as longint
  dim requiredSourceBytes as longint
  dim p as ubyte ptr
  dim r as ubyte, g as ubyte, b as ubyte
  dim paddingByte as ubyte
  dim pbmpdata as BITMAP_RGBImageRec ptr
  dim f as integer

  f = freefile
  open Filename for binary as #f
  if err <> 0 then return 0
  fileOpen = true

  fileSize = lof(f)
  if fileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) then goto load_failed

  '' The fixed headers define the byte layout and pixel-stream offset below.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
  get #f, , bitmapfileheader
  if err <> 0 then goto load_failed
  if bitmapfileheader.bfType <> BITMAP_ID then goto load_failed

  get #f, , bitmapinfoheader
  if err <> 0 then goto load_failed
  if bitmapinfoheader.biSize <> sizeof(BITMAPINFOHEADER) then goto load_failed
  if bitmapinfoheader.biPlanes <> 1 then goto load_failed
  if bitmapinfoheader.biCompression <> 0 then goto load_failed
  if bitmapinfoheader.biBitCount <> 8 and bitmapinfoheader.biBitCount <> 24 then goto load_failed
  if bitmapinfoheader.biWidth <= 0 or bitmapinfoheader.biHeight = 0 then goto load_failed
  if bitmapinfoheader.biHeight = (-2147483647 - 1) then goto load_failed

  imageWidth = bitmapinfoheader.biWidth
  imageHeight = bitmapinfoheader.biHeight
  if imageHeight < 0 then imageHeight = -imageHeight

  if imageWidth > MAX_BITMAP_BYTES \ 3 then goto load_failed
  if imageHeight > (MAX_BITMAP_BYTES \ 3) \ imageWidth then goto load_failed
  noofpixels = imageWidth * imageHeight
  rgbBytes = noofpixels * 3

  if bitmapinfoheader.biBitCount = 24 then
    sourceRowBytes = imageWidth * 3
  else
    sourceRowBytes = imageWidth
  end if
  if sourceRowBytes > MAX_BITMAP_BYTES - 3 then goto load_failed
  sourceRowStride = (sourceRowBytes + 3) and not 3
  requiredSourceBytes = clngint(sourceRowStride) * imageHeight

  if culngint(bitmapfileheader.bfOffBits) > culngint(fileSize) then goto load_failed
  if requiredSourceBytes > fileSize - clngint(bitmapfileheader.bfOffBits) then goto load_failed

  if bitmapinfoheader.biBitCount = 8 then
    if bitmapinfoheader.biClrUsed = 0 then
      paletteEntries = 256
    elseif bitmapinfoheader.biClrUsed > 256 then
      '' One cleanup label keeps every failed parse path releasing the same owners.
      '' FB-LINTER: DISABLE-NEXT-LINE FBL101 FBL-CF-003
      goto load_failed
    else
      paletteEntries = cint(bitmapinfoheader.biClrUsed)
    end if

    for index = 0 to paletteEntries - 1
      get #f, , bmpalette(index)
      if err <> 0 then goto load_failed
    next
  end if

  '' bfOffBits is zero-based in the BMP header; FreeBASIC binary Seek is one-based.
  seek #f, cint(bitmapfileheader.bfOffBits) + 1
  '' Seek reports a malformed position through Err even without ON ERROR state.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL613
  if err <> 0 then goto load_failed

  pbmpdata = callocate(sizeof(BITMAP_RGBImageRec))
  if pbmpdata = 0 then goto load_failed
  pbmpdata->sizeX = imageWidth
  pbmpdata->sizeY = imageHeight
  pbmpdata->buffer = callocate(rgbBytes)
  if pbmpdata->buffer = 0 then goto load_failed

  p = pbmpdata->buffer
  if p = 0 then goto load_failed
  for rowIndex = 0 to imageHeight - 1
    for index = 0 to imageWidth - 1
      if bitmapinfoheader.biBitCount = 24 then
        get #f, , b
        if err <> 0 then goto load_failed
        get #f, , g
        if err <> 0 then goto load_failed
        get #f, , r
        if err <> 0 then goto load_failed
        StoreRGB p, r, g, b
      else
        get #f, , b
        if err <> 0 then goto load_failed
        if b >= paletteEntries then goto load_failed
        StoreRGB p, bmpalette(b).peBlue, bmpalette(b).peGreen, bmpalette(b).peRed
      end if
    next

    for paddingIndex = sourceRowBytes to sourceRowStride - 1
      get #f, , paddingByte
      if err <> 0 then goto load_failed
    next
  next

  close #f
  fileOpen = false
  return pbmpdata

load_failed:
  if fileOpen then close #f
  if pbmpdata <> 0 then
    if pbmpdata->buffer <> 0 then
      deallocate(pbmpdata->buffer)
      pbmpdata->buffer = 0
    end if
    deallocate(pbmpdata)
    pbmpdata = 0
  end if
  return 0
end function

#endif
