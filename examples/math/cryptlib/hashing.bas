#include once "cryptlib.bi"

Function calc_hash( ByVal filename As String, ByVal algo As CRYPT_ALGO_TYPE ) As String
    Const BUFFER_SIZE = 8192
    Dim As Byte buffer( 0 To BUFFER_SIZE-1 )
    Dim As String result

    '' A context owns the algorithm state until cryptDestroyContext().  Every
    '' path after creation releases it before returning to the caller.
    Dim As CRYPT_CONTEXT ctx
    Dim As Long status = cryptCreateContext( @ctx, CRYPT_UNUSED, algo )
    If cryptStatusError( status ) Then
        Return ""
    End If

    '' open input file in binary mode
    Dim As Integer f = FreeFile()
    If( Open( filename For Binary Access Read As #f ) <> 0 ) Then
        cryptDestroyContext( ctx )
        Return ""
    End If

    '' Seek() uses a pointer-width file position.  The byte count is bounded by
    '' BUFFER_SIZE before it is passed to cryptlib's 32-bit length parameter.
    Do Until( EOF( f ) )
        Dim As LongInt oldpos = Seek( f )
        Get #f, , buffer()
        Dim As LongInt readlength = Seek( f ) - oldpos
        '' GET reports a stream read failure through Err even without a legacy
        '' ON ERROR handler.  The check prevents hashing an incomplete block.
        '' FB-LINTER: DISABLE-NEXT-LINE FBL613
        If Err <> 0 OrElse readlength < 0 OrElse readlength > BUFFER_SIZE Then
            status = CRYPT_ERROR_READ
            Exit Do
        End If

        If readlength > 0 Then
            status = cryptEncrypt( ctx, @buffer(0), CLng( readlength ) )
            If cryptStatusError( status ) Then
                Exit Do
            End If
        End If
    Loop

    '' close input file
    Close #f

    If cryptStatusOK( status ) Then
        '' A zero-length final update completes the hash calculation.
        status = cryptEncrypt( ctx, 0, 0 )
    End If

    If cryptStatusOK( status ) Then
        Dim As Long buffersize = BUFFER_SIZE
        status = cryptGetAttributeString( ctx, CRYPT_CTXINFO_HASHVALUE, @buffer(0), @buffersize )

        If cryptStatusOK( status ) AndAlso buffersize >= 0 AndAlso buffersize <= BUFFER_SIZE Then
            '' Each digest byte needs exactly two digits.  Preallocating also avoids
            '' repeated string reallocation while a file is being displayed.
            result = String( buffersize * 2, " " )
            For i As Long = 0 To buffersize - 1
                '' result has two writable characters for every iteration.
                '' FB-LINTER: DISABLE-NEXT-LINE FBL514
                Mid( result, i * 2 + 1, 2 ) = Right( "0" & Hex( CUByte( buffer(i) ) ), 2 )
            Next
        End If
    End If

    '' free the context
    cryptDestroyContext( ctx )

    Return result
End Function

    Dim As String filename = Trim( Command(1) )
    If( Len( filename ) = 0 ) Then
        Print "Usage: hash.exe filename"
        End -1
    End If

    '' init cryptlib
    cryptInit( )

    '' calculate hashes
    Print "md5: "; calc_hash( filename, CRYPT_ALGO_MD5 )
    Print "sha: "; calc_hash( filename, CRYPT_ALGO_SHA1 )

    '' shutdown cryptlib
    cryptEnd( )

    Sleep

' end of hashing.bas
