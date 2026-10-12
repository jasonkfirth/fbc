'' examples/math/cryptlib/hashing.bas
'' Purpose: Hash a binary file with the native cryptlib API.
'' Responsibilities: Exact reads, checked native status and owned cleanup.
'' This example contains no network protocol or library configuration.

#include once "cryptlib.bi"

Function calc_hash( ByVal filename As String, ByVal algo As CRYPT_ALGO_TYPE, ByRef status As Long ) As String
    Const BUFFER_SIZE = 8192
    Dim As Byte buffer( 0 To BUFFER_SIZE-1 )
    Dim As String result

    '' The context owns the algorithm state. Every path after creation releases it.
    Dim As CRYPT_CONTEXT ctx
    status = cryptCreateContext( @ctx, CRYPT_UNUSED, algo )
    If cryptStatusError( status ) Then Return ""

    Dim As Integer f = FreeFile()
    If Open( filename For Binary Access Read As #f ) <> 0 Then
        status = CRYPT_ERROR_READ
        cryptDestroyContext( ctx )
        Return ""
    End If

    Do Until EOF( f )
        '' GET succeeds on a partial final block. Hash only the reported bytes,
        '' never the unused tail of the fixed buffer or a saved error value.
        Dim As UInteger readlength
        Dim As Long read_error = Get( #f, , buffer(), , readlength )
        If read_error <> 0 OrElse readlength > BUFFER_SIZE Then
            status = CRYPT_ERROR_READ
            Exit Do
        End If
        If readlength = 0 Then
            If EOF( f ) = 0 Then status = CRYPT_ERROR_READ
            Exit Do
        End If
        status = cryptEncrypt( ctx, @buffer(0), CLng( readlength ) )
        If cryptStatusError( status ) Then Exit Do
    Loop

    If Close( f ) <> 0 AndAlso cryptStatusOK( status ) Then status = CRYPT_ERROR_READ
    If cryptStatusOK( status ) Then
        '' A zero-length update completes the hash. Keep a valid buffer address
        '' because the native API declares this pointer nonnull, even at length zero.
        status = cryptEncrypt( ctx, @buffer(0), 0 )
    End If
    If cryptStatusOK( status ) Then
        Dim As Long buffersize
        status = cryptGetAttributeString( ctx, CRYPT_CTXINFO_HASHVALUE, @buffer(0), @buffersize )
        If cryptStatusOK( status ) Then
            If buffersize <= 0 OrElse buffersize > CRYPT_MAX_HASHSIZE Then
                status = CRYPT_ERROR_OVERFLOW
            Else
                result = String( buffersize * 2, " " )
                For i As Long = 0 To buffersize - 1
                    '' result has exactly two writable characters for every digest byte.
                    '' FB-LINTER: DISABLE-NEXT-LINE FBL514
                    Mid( result, i * 2 + 1, 2 ) = Right( "0" & Hex( CUByte( buffer(i) ) ), 2 )
                Next
            End If
        End If
    End If

    Dim As Long release_error = cryptDestroyContext( ctx )
    If cryptStatusOK( status ) Then status = release_error
    Return result
End Function

Dim As String filename = Command(1)
Dim As String mode = Command(2)
If filename = "" OrElse (mode <> "" AndAlso mode <> "--no-pause") OrElse Command(3) <> "" Then
    Print "Usage: hash filename [--no-pause]"
    End 1
End If

Dim As Long status = cryptInit()
If cryptStatusError( status ) Then
    Print "cryptInit failed: "; status
    End 1
End If
'' SHA-256 is the primary file digest. SHA-1 remains for legacy comparison;
'' neither output is a keyed authentication code. Current cryptlib keeps MD5
'' internal and rejects attempts to create it through this public API.
Dim As String sha256_digest = calc_hash( filename, CRYPT_ALGO_SHA2, status )
If cryptStatusError( status ) Then
    cryptEnd()
    Print "SHA-256 failed: "; status
    End 1
End If
Dim As String sha1_digest = calc_hash( filename, CRYPT_ALGO_SHA1, status )
If cryptStatusError( status ) Then
    cryptEnd()
    Print "SHA-1 failed: "; status
    End 1
End If
status = cryptEnd()
If cryptStatusError( status ) Then
    Print "cryptEnd failed: "; status
    End 1
End If

'' Calculate both results before printing, so a failed API call cannot leave
'' an apparently successful empty digest.
Print "sha256: "; sha256_digest
Print "sha1:   "; sha1_digest
If mode <> "--no-pause" Then Sleep

'' end of hashing.bas
