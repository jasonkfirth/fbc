/*
    Project: FreeBASIC native curl qualification
    File: curl-native.c
    Purpose: Describe the real SDK's stable public C layouts.
    Responsibilities: Report ABI facts and the loaded libcurl version.
    This file contains no substitute transfers or network server.
*/
#include <stddef.h>
#include <stdio.h>
#include <curl/curl.h>

int main(void)
{
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
        return 1;
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(size_t), sizeof(long), sizeof(curl_off_t),
           sizeof(curl_socket_t), sizeof(curl_socklen_t), sizeof(CURLcode),
           sizeof(struct curl_slist), offsetof(struct curl_slist, data),
           offsetof(struct curl_slist, next), sizeof(struct curl_httppost),
           offsetof(struct curl_httppost, contents),
           offsetof(struct curl_httppost, bufferlength));
    printf("VERSION %s\n", curl_version());
    curl_global_cleanup();
    return 0;
}
/* end of curl-native.c */
