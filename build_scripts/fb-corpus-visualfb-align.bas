/'
    Project: FreeBASIC browser corpus
    File: fb-corpus-visualfb-align.bas

    Purpose:
        Run VisualFBEditor's Format with Basis Word command on source lines.

    Responsibilities:
        Choose the first line's basis, measure the matching columns, then
        align the selection. The browser supplies file and selection input.
        The desktop editor owns painting, undo, and project management.

    Based on TabWindow.FormatWithBasisWord by the VisualFBEditor authors.
    Distributed under GPLv3 or later with the accompanying license.
'/

const MaxSourceBytes = 262144
const MaxResultBytes = 1048576
const MaxLineCount = 16384

type SourceLine
    text as string
    newline as string
    code as string
    basisPosition as integer
end type

private function VisibleCode(byref lineText as const string, byref blockDepth as integer) as string
    dim as string code = lineText
    dim as boolean quoted, escapes
    dim as boolean blockLine = blockDepth > 0
    dim as boolean atStatementStart = true
    dim as integer index = 1
    while index <= len(lineText)
        dim as string pair = mid(lineText, index, 2)
        dim as string token = left(pair, 1)
        if blockDepth > 0 then
            select case pair
            case "/'"
                blockDepth += 1
                index += 1
            case "'/"
                blockDepth -= 1
                index += 1
            end select
        elseif quoted then
            mid(code, index, 1) = chr(1)
            if escapes andalso token = chr(92) andalso len(pair) = 2 then
                index += 1
                mid(code, index, 1) = chr(1)
            elseif token = chr(34) then
                quoted = false
            end if
        elseif pair = "/'" then
            blockDepth = 1
            blockLine = true
            index += 1
        elseif token = "'" then
            code = left(code, index - 1)
            exit while
        elseif token = chr(34) then
            escapes = index > 1 andalso mid(lineText, index - 1, 1) = "!"
            quoted = true
            atStatementStart = false
            mid(code, index, 1) = chr(1)
        elseif token = ":" then
            atStatementStart = true
        elseif token <> " " andalso token <> chr(9) then
            if atStatementStart andalso lcase(mid(lineText, index, 3)) = "rem" then
                dim as string afterRem = mid(lineText, index + 3, 1)
                if afterRem = "" orelse afterRem = " " orelse afterRem = chr(9) then
                    code = left(code, index - 1)
                    exit while
                end if
            end if
            atStatementStart = false
        end if
        index += 1
    wend
    if blockLine then return ""
    return lcase(rtrim(code, any !"\t "))
end function

private function FindBasis(byref code as const string, byref basis as const string) as integer
    dim as integer firstCode = len(code) - len(ltrim(code, any !"\t ")) + 1
    return instr(firstCode, code, basis)
end function

private function ChooseBasis(byref code as const string) as string
    ' The desktop command selects the earliest of these four markers.
    ' A run of spaces is used only when none of the markers is present.
    dim as string candidates(0 to 3) = { " = ", " as ", ": ", ", " }
    dim as integer earliest = len(code) + 1
    dim as string basis
    for index as integer = lbound(candidates) to ubound(candidates)
        dim as integer position = FindBasis(code, candidates(index))
        if position > 1 andalso position < earliest then
            earliest = position
            basis = candidates(index)
        end if
    next
    if basis = "" andalso FindBasis(code, "  ") > 1 then basis = "  "
    return basis
end function

private function TextColumns(byref text as const string) as integer
    ' Work on byte offsets so UTF-8 bytes and line endings can be copied
    ' back verbatim. Continuation bytes do not advance the source column.
    dim as integer columns = 0
    for index as integer = 0 to len(text) - 1
        if (text[index] and &hc0u) <> &h80u then columns += 1
    next
    return columns
end function

private function AlignSelection(lines() as SourceLine, byval first as integer, byval last as integer, _
    byval inputBytes as longint) as integer
    dim as string basis = ChooseBasis(lines(first).code)
    if basis = "" then return 0
    dim as integer basisColumn = 0
    for index as integer = first to last
        dim as integer position = FindBasis(lines(index).code, basis)
        lines(index).basisPosition = position
        if position > 0 then
            dim as integer column = TextColumns(left(lines(index).text, position))
            if column > basisColumn then basisColumn = column
        end if
    next
    ' Retain the two-space separator so running the same command twice
    ' still chooses it when there is no explicit basis word.
    if basis = "  " then basisColumn += 1
    dim as integer changes = 0
    dim as longint remaining = MaxResultBytes - inputBytes
    for index as integer = first to last
        dim as integer position = lines(index).basisPosition
        if position = 0 then continue for
        dim as string original = lines(index).text
        dim as string prefix = rtrim(left(original, position), any !"\t ")
        dim as string tail = ltrim(mid(original, position + 1))
        dim as integer padding = basisColumn - TextColumns(prefix)
        remaining -= len(prefix) + padding + len(tail) - len(original)
        if remaining < 0 then return -1
        dim as string aligned = prefix & space(padding) & tail
        if aligned <> original then
            lines(index).text = aligned
            changes += 1
        end if
    next
    return changes
end function

private function Finish(byval code as integer, byref message as const string) as integer
    print message
    return code
end function

private function Main() as integer
    dim as string inputPath = command(3)
    if inputPath = "" then inputPath = "/sample.bas"
    dim as integer source = freefile
    if open(inputPath for binary access read as #source) <> 0 then return Finish(1, "Could not open the source.")
    dim as longint byteCount = lof(source)
    if byteCount < 1 orelse byteCount > MaxSourceBytes then
        close #source
        return Finish(1, "Choose source between 1 byte and 256 KiB.")
    end if
    dim as string originalSource = space(byteCount)
    dim as integer result = get(#source, , originalSource)
    close #source
    if result <> 0 orelse instr(originalSource, chr(0)) <> 0 then return Finish(1, "Choose a text source file.")

    dim as SourceLine lines()
    redim lines(1 to MaxLineCount)
    dim as integer lineCount = 0, blockDepth = 0, position = 1
    while position <= len(originalSource)
        if lineCount = MaxLineCount then return Finish(1, "Source exceeds 16,384 lines.")
        lineCount += 1
        dim as integer boundary = position
        while boundary <= len(originalSource)
            dim as ubyte character = originalSource[boundary - 1]
            if character = 10 orelse character = 13 then exit while
            boundary += 1
        wend
        if boundary > len(originalSource) then
            lines(lineCount).text = mid(originalSource, position)
            position = len(originalSource) + 1
        else
            lines(lineCount).newline = mid(originalSource, boundary, 1)
            if mid(originalSource, boundary, 2) = chr(13, 10) then lines(lineCount).newline = chr(13, 10)
            lines(lineCount).text = mid(originalSource, position, boundary - position)
            position = boundary + len(lines(lineCount).newline)
        end if
        lines(lineCount).code = VisibleCode(lines(lineCount).text, blockDepth)
    wend
    dim as integer first = 1, last = lineCount
    if command(1) <> "" then first = valint(command(1))
    if command(2) <> "" then last = valint(command(2))
    if first < 1 orelse last < first orelse last > lineCount then return Finish(1, "Choose a valid line range.")
    dim as integer changes = AlignSelection(lines(), first, last, byteCount)
    if changes < 0 then return Finish(1, "The aligned result exceeds 1 MiB.")

    dim as integer target = freefile
    if open("/formatted.bas" for binary access write as #target) <> 0 then return Finish(1, "Could not open the result.")
    for index as integer = 1 to lineCount
        dim as string record = lines(index).text & lines(index).newline
        if put(#target, , record) <> 0 then
            close #target
            return Finish(1, "Could not write the result.")
        end if
    next
    if close(#target) <> 0 then return Finish(1, "Could not close the result.")
    return Finish(0, "Aligned " & str(changes) & " lines.")
end function

end Main()

/' end of fb-corpus-visualfb-align.bas '/
