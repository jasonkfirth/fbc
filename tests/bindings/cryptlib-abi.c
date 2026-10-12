/*
    FreeBASIC native library regression checks
    File: cryptlib-abi.c
    Purpose: Obtain layout and API constants from the official cryptlib header.
    Responsibilities: Context width, query/object layouts and hash selectors.
    This file contains no replacement crypto implementation or key material.
*/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "cryptlib.h"

int main(void)
{
    struct {
        unsigned int before;
        CRYPT_QUERY_INFO value;
        unsigned int after;
    } query;
    const CRYPT_ALGO_TYPE algorithms[] = {
        CRYPT_ALGO_MD5, CRYPT_ALGO_SHA1, CRYPT_ALGO_SHA2
    };
    int status;
    size_t index;
    CRYPT_ALGO_TYPE algorithm;
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(CRYPT_CONTEXT), sizeof(CRYPT_QUERY_INFO),
           offsetof(CRYPT_QUERY_INFO, blockSize), sizeof(CRYPT_OBJECT_INFO),
           offsetof(CRYPT_OBJECT_INFO, cryptAlgo), offsetof(CRYPT_OBJECT_INFO, salt),
           offsetof(CRYPT_OBJECT_INFO, saltSize), sizeof(CRYPT_PKCINFO_RSA),
           offsetof(CRYPT_PKCINFO_RSA, nLen), sizeof(CRYPT_PKCINFO_DLP),
           sizeof(CRYPT_PKCINFO_ECC), sizeof(CRYPT_ALGO_TYPE));
    printf("CONSTANTS %d %d %d %d %d\n", CRYPT_ALGO_MD5, CRYPT_ALGO_SHA1,
           CRYPT_ALGO_SHA2, CRYPT_CTXINFO_HASHVALUE, CRYPT_MAX_HASHSIZE);
    if(cryptInit() != CRYPT_OK)
        return 1;
    /* Algorithm selectors are not consecutive: obsolete reserved values
       remain in the public enumeration for binary compatibility. */
    for(index = 0; index < sizeof(algorithms) / sizeof(algorithms[0]); index++) {
        algorithm = algorithms[index];
        memset(&query, 0, sizeof(query));
        query.before = query.after = 0x13579BDFu;
        status = cryptQueryCapability((CRYPT_ALGO_TYPE)algorithm, &query.value);
        printf("CAPABILITY %d status=%d bytes=%d name=%.64s\n", algorithm,
               status, query.value.blockSize, query.value.algoName);
        if(algorithm == CRYPT_ALGO_MD5) {
            if(status != CRYPT_ERROR_NOTAVAIL)
                return 2;
        } else
        if(status != CRYPT_OK || query.value.blockSize !=
           (algorithm == CRYPT_ALGO_SHA1 ? 20 : 32) || query.value.algoName[0] == '\0')
            return 3;
        if(query.before != 0x13579BDFu || query.after != 0x13579BDFu)
            return 4;
    }
    if(cryptEnd() != CRYPT_OK)
        return 5;
    puts("CRYPTLIB_C_CAPABILITIES=PASS");
    return 0;
}

/* end of cryptlib-abi.c */
