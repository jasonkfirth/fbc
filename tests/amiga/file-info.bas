'' FreeBASIC classic AmigaOS metadata qualification
'' ------------------------------------------------
'' File: file-info.bas
'' Purpose: Check the DOS FileInfoBlock alignment boundary in the real runtime.
'' Responsibilities: Return the native bridge's filesystem qualification result.
'' This file intentionally does NOT contain DOS hooks or file implementations.

declare function amiga_test_fileinfo_alignment cdecl alias "amiga_test_fileinfo_alignment" () as long

end amiga_test_fileinfo_alignment()

'' end of file-info.bas
