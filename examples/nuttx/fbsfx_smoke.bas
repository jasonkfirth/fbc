''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbsfx_smoke.bas
''
'' Purpose:
''
''     Exercise the first sfxlib path in a NuttX image.
''
'' Responsibilities:
''
''     - compile and link the NuttX sfxlib target support
''     - issue a pair of foreground SOUND commands
''     - return without waiting for user input
''
'' This file intentionally does NOT contain:
''
''     - board-specific I2S or HDMI audio setup
''     - WAV loading
''     - capture or MIDI coverage
''

print "fbsfx: starting"

sound 440, 2
sound 660, 2

print "FB_NUTTX_SFX_SMOKE_OK"

#ifdef FB_NUTTX_QEMU_VIRTIO_SOUND
    '' QEMU virtio-sound can complete audio buffers after task teardown.
    '' Keep the smoke task alive so the emulator can prove playback without
    '' exercising that lower-half close race.
    do
        sleep 1000, 1
    loop
#endif

end 0

'' end of fbsfx_smoke.bas
