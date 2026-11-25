/'
    Project: FreeBASIC browser corpus
    File: fb-corpus-brpg-map.bas

    Purpose:
        Adapt BRPGEngine's Map2Bto4B converter for compact .map files.

    Responsibilities:
        Validate one map and emit the two signed layer IDs for each cell
        in the order used by the RPG loader. Rendering lives in the player.

    Based on BRpg/Samples/Map2Bto4B.bas by fangcun, distributed under MIT.
'/

const HeaderBytes = 4
const CellBytes = 4
const MaxAxis = 81  ' RPG.bas declares layer arrays with bounds 0 to 80.
const TileSignBit = &h8000u
const TileMagnitudeMask = &h7fffu

private function ReadWord(byval source as integer, byref value as ushort) as boolean
    dim as ubyte bytes(0 to 1)
    if get(#source, , bytes()) <> 0 then return false
    value = cushort(bytes(0)) or (cushort(bytes(1)) shl 8)
    return true
end function

private function PutLong(byval target as integer, byval value as long) as boolean
    dim as ubyte bytes(0 to 3)
    dim as ulong bits = culng(value)
    for index as integer = 0 to 3
        bytes(index) = cubyte(bits and &hffu)
        bits shr= 8
    next
    return put(#target, , bytes()) = 0
end function

private function ConvertMap(byval source as integer, byval target as integer, _
    byval mapWidth as ushort, byval mapHeight as ushort) as boolean
    if not PutLong(target, mapWidth) orelse not PutLong(target, mapHeight) then return false
    ' Row-major cells contain the ground ID followed by the overlay ID.
    ' Compact IDs are sign-magnitude. The wide format writes signed longs.
    for row as integer = 1 to mapHeight
        for column as integer = 1 to mapWidth
            for layer as integer = 0 to 1
                dim as ushort compact
                if not ReadWord(source, compact) then return false
                dim as long value = compact and TileMagnitudeMask
                if (compact and TileSignBit) <> 0 then value = -value
                if not PutLong(target, value) then return false
            next
        next
    next
    return true
end function

private function Finish(byval code as integer, byref message as const string) as integer
    print message
    return code
end function

private function Main() as integer
    dim as string inputPath = command(1)
    dim as string outputPath = command(2)
    if inputPath = "" then inputPath = "/input.map"
    if outputPath = "" then outputPath = "/converted.map"
    if inputPath = outputPath then return Finish(1, "Choose a different output file.")

    dim as integer source = freefile
    if open(inputPath for binary access read as #source) <> 0 then return Finish(1, "Could not open the input map.")
    dim as ushort mapWidth, mapHeight
    if not ReadWord(source, mapWidth) orelse not ReadWord(source, mapHeight) orelse _
        mapWidth = 0 orelse mapHeight = 0 orelse mapWidth > MaxAxis orelse mapHeight > MaxAxis orelse _
        lof(source) <> HeaderBytes + CellBytes * cint(mapWidth) * cint(mapHeight) then
        close #source
        return Finish(1, "Invalid compact map dimensions or length.")
    end if

    dim as integer target = freefile
    if open(outputPath for binary access write as #target) <> 0 then
        close #source
        return Finish(1, "Could not create the output map.")
    end if
    if lof(target) <> 0 then
        close #target
        close #source
        return Finish(1, "The output map already exists.")
    end if
    dim as boolean converted = ConvertMap(source, target, mapWidth, mapHeight)
    close #source
    dim as integer closeResult = close(#target)
    if not converted orelse closeResult <> 0 then
        kill outputPath
        return Finish(1, "Could not finish writing the map.")
    end if
    return Finish(0, "Converted " & str(mapWidth) & " x " & str(mapHeight) & " map.")
end function

end Main()

/' end of fb-corpus-brpg-map.bas '/
