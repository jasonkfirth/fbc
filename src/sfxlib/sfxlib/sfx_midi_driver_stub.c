#if !defined(_WIN32) && !defined(__DJGPP__)

int fb_sfxMidiDriverOpen(int device)
{
    (void)device;
    return 0;
}

void fb_sfxMidiDriverClose(void)
{
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    (void)status;
    (void)data1;
    (void)data2;
    return 0;
}

#endif
