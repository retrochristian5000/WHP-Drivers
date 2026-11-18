/* Compile-only 16-bit Windows 95 audio driver header boundary.
 * No MM driver entry points, DSP I/O, VxD calls or installable binary.
 */
#include <ddk/win95rtm/audio/ddk_audio.h>

/* Water's shared Win16 ABI name must not be display-only. Keep the
 * historical display alias for compatibility, but reference the neutral
 * version from the traditional Win16 audio compile boundary.
 */
#ifndef WHP_WIN95_RTM_VERSION
#error "Water Win16 ABI lacks shared Windows 95 RTM version name"
#endif
#if WHP_WIN95_RTM_VERSION != 0x0400u || \
    WHP_WIN95_RTM_DISPLAY_VERSION != WHP_WIN95_RTM_VERSION
#error "Windows 95 RTM Win16 version aliases disagree"
#endif

#ifdef WHP_WIN95_AUDIO_USE_HISTORICAL_DDK
#ifndef WODM_OPEN
#error "Historical DDK lacks wave output driver message definitions"
#endif
#ifndef WIDM_OPEN
#error "Historical DDK lacks wave input driver message definitions"
#endif
#ifndef MODM_OPEN
#error "Historical DDK lacks MIDI output driver message definitions"
#endif
#ifndef MIDM_OPEN
#error "Historical DDK lacks MIDI input driver message definitions"
#endif
#ifndef AUXDM_GETNUMDEVS
#error "Historical DDK lacks auxiliary audio driver message definitions"
#endif
#ifndef MXDM_GETNUMDEVS
#error "Historical DDK lacks mixer driver message definitions"
#endif
/* Request type existence without guessing Win95 RTM field offsets. */
typedef char whp_w95_audio_wavehdr_exists[(sizeof(WAVEHDR) != 0) ? 1 : -1];
#endif

unsigned short whp_win95_audio_header_probe(void)
{
    return (unsigned short)(sizeof(WORD) + sizeof(DWORD) + sizeof(LPSTR));
}
