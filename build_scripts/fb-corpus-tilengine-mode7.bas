/'
    Project: FreeBASIC browser corpus
    File: fb-corpus-tilengine-mode7.bas

    Purpose:
        Run Tilengine's Mode 7 sample with a generated island and sky.

    Responsibilities:
        Create indexed graphics, handle camera input and supply the
        scanline transform. Tilengine owns tile sampling and presentation.

    Adapted from samples/Mode7.c by Marc Palacios, 2015. This Source Code
    Form is subject to the Mozilla Public License, version 2.0.
'/

#include once "Tilengine.bi"

const ViewWidth = 400
const ViewHeight = 240
const HorizonLine = 72
const TilePixels = 16
const MapTiles = 64
const MapPixels = TilePixels * MapTiles
const SkyWidth = 1024
const RadiansPerDegree = 0.01745329252
const Acceleration = 0.2
const MaxSpeed = 2.0

enum SceneLayer
    LayerGround
    LayerSky
    LayerCount
end enum

enum GroundTile
    TileWater = 1
    TileShallows
    TileBeach
    TileMeadow
    TileRidge
    TileRoad
    TileLane
    TileBeacon
    TerrainTiles
end enum

enum SceneColor
    InkWater = 1
    InkWave
    InkShallows
    InkFoam
    InkBeach
    InkMeadow
    InkRidge
    InkFoliage
    InkRoad
    InkRoadEdge
    InkGold
    InkSkyTop
    InkSkyBand2
    InkSkyBand3
    InkSkyBand4
    InkSkyBand5
    InkSkyBand6
    InkSkyBand7
    InkSkyBottom
    InkCloud
    InkCloudShadow
    InkMountain
    InkSun
    PaletteEntries
end enum

type Mode7Scene
    groundTileset as TLN_Tileset
    groundMap as TLN_Tilemap
    skyBitmap as TLN_Bitmap
    x as single
    y as single
    heading as integer
    speed as single
end type

' Tilengine runs the raster callback synchronously during TLN_DrawFrame.
' The browser event loop can only resume after the frame has returned.
dim shared as Mode7Scene scene

private function CreateScenePalette() as TLN_Palette
    dim as ulong colors(1 to PaletteEntries - 1) = { _
        &h195777, &h287c95, &h3a9e9c, &h68b6a8, &hd9bd7b, &h527650, _
        &h355b4b, &h6b8b56, &h354552, &h82918d, &hffe098, _
        &h254c78, &h2d5f89, &h3b769b, &h5090a8, &h68a5b0, &h82b5ae, _
        &ha9c0ad, &hcfccb0, &hebe9c9, &h94b3b3, &h445c70, &hffe8ad }
    dim as TLN_Palette scenePalette = TLN_CreatePalette(PaletteEntries)
    if scenePalette = 0 then return 0
    for index as integer = lbound(colors) to ubound(colors)
        dim as ulong shade = colors(index)
        TLN_SetPaletteColor(scenePalette, index, cubyte(shade shr 16), cubyte(shade shr 8), cubyte(shade))
    next
    return scenePalette
end function

private function TerrainPixel(byval tileCode as GroundTile, byval x as integer, byval y as integer) as ubyte
    select case tileCode
    case TileWater
        if y = 5 andalso x >= 9 andalso x <= 12 then return InkWave
        return InkWater
    case TileShallows
        if (y = 4 orelse y = 12) andalso x >= 7 then return InkFoam
        return InkShallows
    case TileBeach
        return InkBeach
    case TileMeadow
        if (x + y * 3) mod 17 = 0 then return InkFoliage
        return InkMeadow
    case TileRidge
        if y = 0 orelse y = 1 then return InkFoliage
        return InkRidge
    case TileBeacon
        if x >= 4 andalso x <= 11 andalso y >= 4 andalso y <= 11 then return InkGold
        return InkBeach
    end select
    if tileCode = TileLane andalso (x = 7 orelse x = 8) andalso y < 9 then return InkGold
    if x = 0 orelse x = TilePixels - 1 then return InkRoadEdge
    return InkRoad
end function

private function BuildTerrain() as boolean
    dim as TLN_Palette groundPalette = CreateScenePalette()
    if groundPalette = 0 then return false
    scene.groundTileset = TLN_CreateTileset(TerrainTiles, TilePixels, TilePixels, groundPalette, 0, 0)
    if scene.groundTileset = 0 then
        TLN_DeletePalette(groundPalette)
        return false
    end if

    dim as ubyte pixels(0 to TilePixels * TilePixels - 1)
    for tileCode as integer = TileWater to TerrainTiles - 1
        for row as integer = 0 to TilePixels - 1
            for column as integer = 0 to TilePixels - 1
                pixels(row * TilePixels + column) = TerrainPixel(tileCode, column, row)
            next
        next
        if not TLN_SetTilesetPixels(scene.groundTileset, tileCode, @pixels(0), TilePixels) then return false
    next

    ' Each map entry matches Tilengine's 16-bit index and 16-bit flags.
    ' Tile 0 is transparent. The constructor copies the row-major table.
    dim as Tile mapData(0 to MapTiles * MapTiles - 1)
    for row as integer = 0 to MapTiles - 1
        dim as integer coast = 16 + cint(sin(row * 0.20) * 5)
        for column as integer = 0 to MapTiles - 1
            dim as integer fromRoad = abs(column - MapTiles \ 2)
            dim as ushort tileCode
            select case fromRoad
            case 0
                tileCode = TileLane
            case 1, 2
                tileCode = TileRoad
            case 3
                tileCode = TileBeach
                if row mod 6 = 0 then tileCode = TileBeacon
            case is < coast - 1
                tileCode = TileMeadow
                if (row + fromRoad * 2) mod 9 < 3 then tileCode = TileRidge
            case coast - 1, coast
                tileCode = TileBeach
            case coast + 1, coast + 2
                tileCode = TileShallows
            case else
                tileCode = TileWater
            end select
            mapData(row * MapTiles + column).index = tileCode
        next
    next
    scene.groundMap = TLN_CreateTilemap(MapTiles, MapTiles, @mapData(0), 0, scene.groundTileset)
    return scene.groundMap <> 0
end function

private function BuildSky() as boolean
    dim as TLN_Palette skyPalette = CreateScenePalette()
    if skyPalette = 0 then return false
    scene.skyBitmap = TLN_CreateBitmap(SkyWidth, HorizonLine, 8)
    if scene.skyBitmap = 0 orelse not TLN_SetBitmapPalette(scene.skyBitmap, skyPalette) then
        TLN_DeletePalette(skyPalette)
        return false
    end if
    for y as integer = 0 to HorizonLine - 1
        dim as ubyte ptr scanline = TLN_GetBitmapPtr(scene.skyBitmap, 0, y)
        if scanline = 0 then return false
        for x as integer = 0 to SkyWidth - 1
            dim as integer shade = InkSkyTop + y * 8 \ HorizonLine
            dim as integer cloudX = x mod 192 - 80
            if abs(cloudX) < 47 andalso y >= 23 andalso y <= 29 then
                shade = InkCloud
                if y = 29 then shade = InkCloudShadow
            end if
            if (x - 292) * (x - 292) + (y - 28) * (y - 28) < 14 * 14 then shade = InkSun
            dim as integer mountain = 58 + cint(sin(x * RadiansPerDegree * 3.0) * 6)
            if y >= mountain then shade = InkMountain
            scanline[x] = shade
        next
    next
    return true
end function

private sub ResetCamera()
    scene.x = MapPixels \ 2 - ViewWidth \ 2
    scene.y = MapPixels \ 2
    scene.heading = 0
    scene.speed = 0
end sub

private sub MoveCamera()
    ' Tilengine's SDL defaults bind Z to INPUT_X, the third action button.
    if TLN_GetInput(INPUT_BUTTON3) then
        ResetCamera()
        exit sub
    end if
    if TLN_GetInput(INPUT_LEFT) then
        scene.heading -= 2
    elseif TLN_GetInput(INPUT_RIGHT) then
        scene.heading += 2
    end if
    scene.heading = (scene.heading + 360) mod 360

    if TLN_GetInput(INPUT_UP) then
        scene.speed += Acceleration
        if scene.speed > MaxSpeed then scene.speed = MaxSpeed
    elseif TLN_GetInput(INPUT_DOWN) then
        scene.speed -= Acceleration
        if scene.speed < -MaxSpeed then scene.speed = -MaxSpeed
    elseif scene.speed > 0 then
        scene.speed -= Acceleration
    elseif scene.speed < 0 then
        scene.speed += Acceleration
    end if
    if abs(scene.speed) < Acceleration / 2 then scene.speed = 0
    dim as single radians = scene.heading * RadiansPerDegree
    scene.x += sin(radians) * scene.speed
    scene.y -= cos(radians) * scene.speed
    ' The tilemap wraps. Bound the camera coordinates before converting
    ' them to pixel integers so long sessions retain subpixel precision.
    if scene.x >= MapPixels then scene.x -= MapPixels
    if scene.x < 0 then scene.x += MapPixels
    if scene.y >= MapPixels then scene.y -= MapPixels
    if scene.y < 0 then scene.y += MapPixels
end sub

private sub RasterLine cdecl(byval lineIndex as integer)
    if lineIndex < HorizonLine then exit sub
    if lineIndex = HorizonLine then
        TLN_SetLayer(LayerGround, scene.groundTileset, scene.groundMap)
        TLN_SetLayerPosition(LayerGround, fix(scene.x), fix(scene.y))
    end if
    ' Mode7.c interpolates from the distant plane to the foreground.
    ' The affine scale applies to this scanline before the next callback.
    dim as single scale = 0.2 + 4.8 * (lineIndex - HorizonLine) / (ViewHeight - HorizonLine)
    TLN_SetLayerTransform(LayerGround, scene.heading, ViewWidth / 2, ViewHeight, scale, scale)
end sub

private function Main() as integer
    if TLN_Init(ViewWidth, ViewHeight, LayerCount, 0, 0) = 0 then return 1
    dim as integer result = 1
    if BuildTerrain() andalso BuildSky() andalso TLN_SetLayerBitmap(LayerSky, scene.skyBitmap) then
        if TLN_CreateWindow(0, CWF_S2 or CWF_NEAREST) then
            ResetCamera()
            TLN_SetRasterCallback(@RasterLine)
            dim as integer frame = 0
            while TLN_ProcessWindow()
                MoveCamera()
                TLN_SetLayerPosition(LayerSky, scene.heading * SkyWidth \ 360, 0)
                TLN_DisableLayer(LayerGround)
                TLN_DrawFrame(frame)
                frame = (frame + 1) mod 3600
                ' SDL submits the frame here. Sleep lets Chromium present it
                ' and dispatch input before the next pass through SDL.
                sleep 16, 1
            wend
            TLN_SetRasterCallback(0)
            TLN_DeleteWindow()
            result = 0
        end if
    end if
    if result <> 0 then print *TLN_GetErrorString(TLN_GetLastError())
    if scene.skyBitmap <> 0 then TLN_DeleteBitmap(scene.skyBitmap)
    if scene.groundMap <> 0 then TLN_DeleteTilemap(scene.groundMap)
    if scene.groundTileset <> 0 then TLN_DeleteTileset(scene.groundTileset)
    TLN_Deinit()
    return result
end function

end Main()

/' end of fb-corpus-tilengine-mode7.bas '/
