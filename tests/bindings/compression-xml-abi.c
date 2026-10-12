/*
    Project: FreeBASIC native compression and XML qualification
    File: compression-xml-abi.c
    Purpose: Describe the real SDK layouts used by the examples.
    Responsibilities: ABI facts, runtime versions and Expat build features.
    This file contains no replacement codec or file extraction.
*/
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <zip.h>
#include <expat.h>
/* Both SDKs use XMLCALL. Expat's declarations have already captured it. */
#undef XMLCALL
#include <libxml/xmlreader.h>
#include <zlib.h>
#include <bzlib.h>
#include <lzo/lzo1x.h>

int main(void)
{
    const XML_Feature *feature;
    printf("ZIP %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(zip_uint64_t), sizeof(time_t), sizeof(zip_stat_t),
           offsetof(zip_stat_t, name), offsetof(zip_stat_t, index),
           offsetof(zip_stat_t, mtime), offsetof(zip_stat_t, crc),
           offsetof(zip_stat_t, comp_method), offsetof(zip_stat_t, flags));
    printf("EXPAT %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(XML_Char), sizeof(XML_LChar), sizeof(XML_Index), sizeof(XML_Size),
           sizeof(XML_Content), offsetof(XML_Content, children),
           sizeof(XML_ParsingStatus), sizeof(XML_Memory_Handling_Suite));
    printf("XML %zu %zu %zu %zu\n", sizeof(xmlChar), sizeof(xmlTextReaderPtr),
           sizeof(xmlReaderTypes), sizeof(xmlParserOption));
    printf("ZLIB %zu %zu %zu %zu %zu %zu %zu %zu\n", sizeof(uLong), sizeof(z_off_t),
           sizeof(z_stream), offsetof(z_stream, total_in), offsetof(z_stream, state),
           offsetof(z_stream, zalloc), offsetof(z_stream, adler), offsetof(z_stream, reserved));
    printf("BZ2 %zu %zu %zu %zu %zu\n", sizeof(bz_stream), offsetof(bz_stream, next_out),
           offsetof(bz_stream, state), offsetof(bz_stream, bzalloc), offsetof(bz_stream, opaque));
    printf("LZO %zu %zu %zu %zu\n", sizeof(lzo_uint), sizeof(lzo_int),
           sizeof(lzo_align_t), (size_t)LZO1X_1_15_MEM_COMPRESS);
    printf("VERSIONS zip=%s expat=%s xml=%s zlib=%s bz2=%s lzo=%s\n",
           zip_libzip_version(), XML_ExpatVersion(), xmlParserVersion,
           zlibVersion(), BZ2_bzlibVersion(), lzo_version_string());
    printf("EXPAT_FEATURES");
    for (feature = XML_GetFeatureList(); feature->feature != XML_FEATURE_END; ++feature)
        printf(" %d:%ld", (int)feature->feature, feature->value);
    printf("\n");
    return 0;
}
/* end of compression-xml-abi.c */
