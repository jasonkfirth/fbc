/'
    Project: FreeBASIC browser corpus
    File: fb-corpus-darkcastle.bas
    Purpose: Start the original Dark Castle room editor in a browser.
    Responsibilities: Set up the display, provide original workshop textures,
    and seed the editor's initial room. The engine owns rendering and editing.
'/

#lang "fb"

#include once "fbgfx.bi"
#include once "vbcompat.bi"
#include once "pcxlib.bi"
#include once "isolib.bi"
#include once "conlib.bi"
#include once "maplib.bi"
#include once "texlib.bi"
#include once "gamlib.bi"

Private Function makeStoneTexture(ByRef filename As Const String, ByVal tileSize As Integer, _
                                  ByVal accent As Integer) As Integer
  ' The renderer reads 8-bit PCX. The dimensions use little-endian words,
  ' and the 256-entry RGB palette follows the image bytes.
  Dim As String header = String(128, Chr(0))
  header[0] = 10  ' PCX manufacturer byte.
  header[1] = 5   ' PCX version with a 256-entry palette.
  header[2] = 1   ' PCX run-length encoding.
  header[3] = 8   ' Bits per indexed pixel.
  header[8] = tileSize - 1
  header[10] = tileSize - 1
  header[65] = 1  ' One indexed plane.
  header[66] = tileSize
  header[68] = 1  ' Color palette interpretation.

  ' Every index is below the PCX run marker (&hC0), so the decoder can read
  ' each pixel directly. Stagger the joints by half a block on every row.
  Dim As String pixels = String(tileSize * tileSize, Chr(0))
  For y As Integer = 0 To tileSize - 1
    For x As Integer = 0 To tileSize - 1
      Dim As Integer brickX = (x + (y \ 8 Mod 2) * 8) Mod 16
      Dim As Integer shade = 2 + ((x * 3 + y * 7) Mod 3)
      If y Mod 8 = 0 OrElse brickX = 0 Then shade = 1
      pixels[y * tileSize + x] = shade
    Next x
  Next y

  Dim As String colorTable = String(768, Chr(0))
  For i As Integer = 1 To 4
    colorTable[i * 3] = 24 + 11 * i + accent
    colorTable[i * 3 + 1] = 34 + 12 * i
    colorTable[i * 3 + 2] = 36 + 10 * i
  Next i
  Dim As String bytes = header + pixels + Chr(12) + colorTable
  Dim As Integer handle = FreeFile()
  If Open(filename For Binary Access Write As #handle) <> 0 Then Return 0
  Dim As Integer result = Put(#handle, 1, bytes)
  Close #handle
  Return result = 0
End Function

Sub DarkCastleSeedMap(ByRef attributes As roomattr, cells() As mapcell)
  attributes.mname = "Stone workshop"
  attributes.ax = 9
  attributes.ay = 9
  attributes.depx = 4
  attributes.depy = 4
  For x As Integer = 0 To attributes.ax - 1
    For y As Integer = 0 To attributes.ay - 1
      ' Leave the viewer-facing sides open so the raised floor is visible.
      If x = 0 OrElse y = 0 Then
        cells(x, y).floor = 2
      ElseIf x >= 3 AndAlso x <= 5 AndAlso y >= 3 AndAlso y <= 5 Then
        cells(x, y).floor = 1
      End If
    Next y
  Next x
End Sub

' IDBFS supplies the maps directory. Other resources stay at the virtual root.
If ChDir("/") <> 0 Then End 3
If Dir("maps", fbDirectory) = "" AndAlso MkDir("maps") <> 0 Then End 3
If MkDir("textures") <> 0 Then End 3
If MkDir("textures/wall") <> 0 Then End 3
If MkDir("textures/ground") <> 0 Then End 3
If MkDir("textures/object") <> 0 Then End 3
If MkDir("textures/entity") <> 0 Then End 3
If makeStoneTexture("textures/wall/wall0001.pcx", 64, 6) = 0 Then End 3
If makeStoneTexture("textures/ground/wall0002.pcx", 64, 20) = 0 Then End 3
If ScreenRes(AX0, AY0, 32, 3) <> 0 Then End 3

InitFonts()
InitPictograms()
KillTexMemory()
LoadObjectList()
LoadEntityTypes()
ActionTable()
MapEditor()

/' end of fb-corpus-darkcastle.bas '/
