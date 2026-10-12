'' Project: FreeBASIC native library regression checks
'' File: cryptlib-abi.bas
'' Purpose: Compare the binding's storage and constants with native cryptlib.
'' Responsibilities: Handle width, object layout and hash selectors.
'' This file contains no key generation or application security configuration.

#lang "fb"
#include once "cryptlib.bi"

Print "ABI "; SizeOf(CRYPT_CONTEXT); " "; SizeOf(CRYPT_QUERY_INFO); " "; _
	OffsetOf(CRYPT_QUERY_INFO, blockSize); " "; SizeOf(CRYPT_OBJECT_INFO); " "; _
	OffsetOf(CRYPT_OBJECT_INFO, cryptAlgo); " "; OffsetOf(CRYPT_OBJECT_INFO, salt(0)); " "; _
	OffsetOf(CRYPT_OBJECT_INFO, saltSize); " "; SizeOf(CRYPT_PKCINFO_RSA); " "; _
	OffsetOf(CRYPT_PKCINFO_RSA, nLen); " "; SizeOf(CRYPT_PKCINFO_DLP); " "; _
	SizeOf(CRYPT_PKCINFO_ECC); " "; SizeOf(CRYPT_ALGO_TYPE)
Print "CONSTANTS "; CRYPT_ALGO_MD5; " "; CRYPT_ALGO_SHA1; " "; _
	CRYPT_ALGO_SHA2; " "; CRYPT_CTXINFO_HASHVALUE; " "; CRYPT_MAX_HASHSIZE

'' end of cryptlib-abi.bas
