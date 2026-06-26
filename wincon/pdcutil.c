/* PDCursesMod */

#include "pdcwin.h"
#include "../common/beep.c"

/* Plays queued beeps and blinks text, which waits must keep doing */
static void _service_beeps_and_blinking( const long t)
{
    if( SP->n_beeps_queued && (int)( t - SP->t_next_beep) > 0)
    {
        SP->n_beeps_queued--;
        SP->t_next_beep = t + beep_interval;
        if( SP->n_beeps_queued > 0)
           _raw_beep( );
    }
    if ((SP->termattrs & A_BLINK) && (GetTickCount() >= pdc_last_blink + 500))
        PDC_blink_text();
}

void PDC_napms(int ms)     /* 'ms' = milli,  _not_ microseconds! */
{
    long t = PDC_millisecs( );
    const long end_t = t + ms;
    int remains;

    do
    {
        remains = (int)( end_t - t);

        _service_beeps_and_blinking( t);
        if( remains > 0)
        {
           Sleep( remains > 50 ? 50 : remains);
           t = PDC_millisecs( );
        }
    } while( remains > 0);
}

bool PDC_wait_for_key_or_timeout(int ms)
{
    /* GetTickCount, as changes of the system time move PDC_millisecs */
    const DWORD start = GetTickCount( );
    DWORD waited = 0;

    if( ms <= 0 || pdc_con_in == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        const DWORD remains = (DWORD)ms - waited;

        _service_beeps_and_blinking( PDC_millisecs( ));
        switch( WaitForSingleObject( pdc_con_in, remains > 50 ? 50 : remains))
        {
            /* A handle that can't be waited on leaves the sleep to the
               caller */
            case WAIT_FAILED:
                return FALSE;
            case WAIT_OBJECT_0:
                if( PDC_check_key( ))
                    return TRUE;
                /* The handle is also signaled with no record to read, as
                   it stays once the input is closed. Then the wait polls
                   in short steps, rather than return at once again. */
                Sleep( remains > 5 ? 5 : remains);
                break;
            default:
                break;
        }
        waited = GetTickCount( ) - start;
    } while( waited < (DWORD)ms);
    return TRUE;
}

const char *PDC_sysname(void)
{
    return "Windows";
}

enum PDC_port PDC_port_val = PDC_PORT_WINCON;
