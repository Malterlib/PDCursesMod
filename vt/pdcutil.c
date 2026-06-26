#ifdef _WIN32
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
   #undef MOUSE_MOVED
#elif !defined( DOS)
   #include <unistd.h>
   #include <stdlib.h>
#endif
#include "curspriv.h"

#ifdef DOS
         /* cheat and use DOS utils from the DOS port for beep and nap */
   #include "../dos/pdcdos.h"
   #include "../common/dosutil.c"
#else

#include "../common/beep.c"

#ifdef _WIN32
void PDC_check_for_resize( void);            /* pdcscrn.c */
#endif

#ifdef LINUX_FRAMEBUFFER_PORT
   void PDC_check_for_blinking( void);       /* ../common/blink.c */
#endif

static void _service_beeps( const long t)
{
    if( SP->n_beeps_queued && (int)( t - SP->t_next_beep) > 0)
    {
        SP->n_beeps_queued--;
        SP->t_next_beep = t + beep_interval;
        if( SP->n_beeps_queued > 0)
           _raw_beep( );
    }
}

void PDC_napms(int ms)
{
    long t = PDC_millisecs( );
    const long end_t = t + ms;
    int remains;

    do
    {
        remains = (int)( end_t - t);

        _service_beeps( t);
        if( remains > 50)        /* break into 50 ms bits */
            remains = 50;
#ifdef LINUX_FRAMEBUFFER_PORT
             PDC_check_for_blinking( );
#endif
#ifdef _WIN32
        PDC_check_for_resize( );
        if( remains > 0)
        {
            Sleep( remains);
            t = PDC_millisecs( );
        }
#else
        if( remains > 0)
        {
            usleep( 1000 * remains);
            t = PDC_millisecs( );
        }
#endif
    }
    while( remains > 0);
}

#ifdef _WIN32
bool PDC_wait_for_key_or_timeout(int ms)
{
    /* GetTickCount, as changes of the system time move PDC_millisecs */
    const DWORD start = GetTickCount( );
    HANDLE input_handle;

    if( ms <= 0)
        return FALSE;

    _service_beeps( PDC_millisecs( ));
    PDC_check_for_resize( );
    input_handle = GetStdHandle( STD_INPUT_HANDLE);
    if( input_handle == INVALID_HANDLE_VALUE || !input_handle)
        return FALSE;

    /* Records that the keyboard checks don't read, as key releases, also
       signal the handle and stay until a key is read. Then the wait polls
       for a key for the rest of its time, in short steps, rather than return
       at once again. A handle that can't be waited on leaves the sleep to
       the caller. */
    switch( WaitForSingleObject( input_handle, (DWORD)ms))
    {
        case WAIT_FAILED:
            return FALSE;
        case WAIT_OBJECT_0:
            while( !PDC_check_key( ))
            {
                const DWORD waited = GetTickCount( ) - start;

                if( waited >= (DWORD)ms)
                    break;
                Sleep( (DWORD)ms - waited < 5 ? (DWORD)ms - waited : 5);
            }
            break;
        default:
            break;
    }
    PDC_check_for_resize( );
    return TRUE;
}
#endif
#endif               /* non-DOS case */

const char *PDC_sysname(void)
{
#ifdef USE_DRM
   return( "Direct Rendering Manager");
#else
   #ifdef LINUX_FRAMEBUFFER_PORT
      return( "LinuxFB");
   #else
      return( "VTx00");
   #endif
#endif
}

#ifdef LINUX_FRAMEBUFFER_PORT
enum PDC_port PDC_port_val = PDC_PORT_LINUX_FB;
#else
enum PDC_port PDC_port_val = PDC_PORT_VT;
#endif
