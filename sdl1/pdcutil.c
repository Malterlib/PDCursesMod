/* PDCurses */

#include "pdcsdl.h"

void PDC_beep(void)
{
    PDC_LOG(("PDC_beep() - called\n"));
}

void PDC_check_for_blinking( void);

void PDC_napms(int ms)
{
    PDC_LOG(("PDC_napms() - called: ms=%d\n", ms));

    PDC_update_rects();
    while (ms > 50)
    {
        SDL_PumpEvents();
        SDL_Delay(50);
        ms -= 50;
        PDC_check_for_blinking( );
    }
    SDL_PumpEvents();
    SDL_Delay(ms);
    PDC_check_for_blinking( );
}

/* Has no input handle to wait on, so wgetch() polls with napms() */
bool PDC_wait_for_key_or_timeout(int ms)
{
    INTENTIONALLY_UNUSED_PARAMETER( ms);
    return FALSE;
}

const char *PDC_sysname(void)
{
    return "SDL";
}

enum PDC_port PDC_port_val = PDC_PORT_SDL1;
