/*
    FreeBASIC NuttX/RP2350-PiZero programming helper
    ------------------------------------------------

    File: fbbootsel.c

    Purpose:

        Provide a small NuttX shell command that asks the board to reboot
        into RP2350 BOOTSEL USB programming mode.

    Responsibilities:

        - call the public NuttX board reset interface
        - use the RP23xx board reset status value that selects BOOTSEL
        - report an error if the board control interface is not available

    This file intentionally does NOT contain:

        - direct RP2350 boot ROM table calls
        - UF2 flashing or mass-storage file copying
        - normal application restart logic

    Platform notes:

        The RP23xx NuttX board reset implementation treats reset status 3 as
        a request for the ROM BOOTSEL path.  Keeping that detail behind
        BOARDIOC_RESET lets this command use the board layer's existing reset
        sequencing instead of duplicating ROM lookup code in an application.
*/

#include <nuttx/config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/boardctl.h>

#ifndef FAR
#  define FAR
#endif

#define FBBOOTSEL_RESET_STATUS 3

int main(int argc, FAR char *argv[])
{
#if defined(CONFIG_BOARDCTL) && defined(CONFIG_BOARDCTL_RESET)
  int ret;
  int errcode;
#endif

  (void)argc;
  (void)argv;

#if !defined(CONFIG_BOARDCTL)
  fprintf(stderr, "bootsel: CONFIG_BOARDCTL is not enabled\n");
  return EXIT_FAILURE;
#elif !defined(CONFIG_BOARDCTL_RESET)
  fprintf(stderr, "bootsel: CONFIG_BOARDCTL_RESET is not enabled\n");
  return EXIT_FAILURE;
#else
  printf("bootsel: rebooting into RP2350 BOOTSEL programming mode\n");
  fflush(stdout);

  ret = boardctl(BOARDIOC_RESET, FBBOOTSEL_RESET_STATUS);

  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "bootsel: BOARDIOC_RESET failed: %d\n", errcode);
      return EXIT_FAILURE;
    }

  for (;;)
    {
    }

  return EXIT_SUCCESS;
#endif
}

/* end of fbbootsel.c */
