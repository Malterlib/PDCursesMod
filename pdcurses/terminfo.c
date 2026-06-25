/* PDCursesMod */

#include <curspriv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*man-start**************************************************************

terminfo
--------

### Synopsis

    int vidattr(chtype attr);
    int vid_attr(attr_t attr, short color_pair, void *opt);
    int vidputs(chtype attr, int (*putfunc)(int));
    int vid_puts(attr_t attr, short color_pair, void *opt,
    int (*putfunc)(int));

    int del_curterm(TERMINAL *);
    int putp(const char *);
    int restartterm(const char *, int, int *);
    TERMINAL *set_curterm(TERMINAL *);
    int setterm(const char *term);
    int setupterm(const char *, int, int *);
    int tgetent(char *, const char *);
    int tgetflag(const char *);
    int tgetnum(const char *);
    char *tgetstr(const char *, char **);
    char *tgoto(const char *, int, int);
    int tigetflag(const char *);
    int tigetnum(const char *);
    char *tigetstr(const char *);
    char *tparm(const char *,long, long, long, long, long, long,
                long, long, long);
    int tputs(const char *, int, int (*)(int));

### Description

   These functions provide a small built-in VT-compatible termcap surface.
   Capabilities outside that subset return the appropriate errors.

### Portability
   Function              | X/Open | ncurses | NetBSD
   :---------------------|:------:|:-------:|:------:
   mvcur                 |    Y   |    Y    |   Y

**man-end****************************************************************/

#include <term.h>

TERMINAL *cur_term = NULL;

typedef struct
{
    const char *id;
    const char *value;
} PDC_TERM_STRING_CAP;

typedef struct
{
    const char *id;
    int value;
} PDC_TERM_NUM_CAP;

static const char * const _pdc_vt_term_names[] =
{
    "alacritty",
    "ansi",
    "foot",
    "gnome",
    "kitty",
    "konsole",
    "linux",
    "rxvt",
    "screen",
    "tmux",
    "vt100",
    "vt220",
    "vte",
    "wezterm",
    "xterm",
    NULL
};

static const char * const _pdc_dumb_term_names[] =
{
    "dumb",
    "emacs",
    NULL
};

static const PDC_TERM_STRING_CAP _pdc_vt_term_strings[] =
{
    { "@7", "\033[F" },
    { "DC", "\033[%dP" },
    { "DO", "\033[%dB" },
    { "IC", "\033[%d@" },
    { "LE", "\033[%dD" },
    { "RI", "\033[%dC" },
    { "UP", "\033[%dA" },
    { "al", "\033[L" },
    { "bl", "\007" },
    { "cd", "\033[J" },
    { "ce", "\033[K" },
    { "ch", "\033[%i%dG" },
    { "cl", "\033[H\033[2J" },
    { "dc", "\033[P" },
    { "dl", "\033[M" },
    { "ei", "\033[4l" },
    { "ho", "\033[H" },
    { "ic", "\033[@" },
    { "im", "\033[4h" },
    { "kD", "\033[3~" },
    { "kd", "\033[B" },
    { "kh", "\033[H" },
    { "kl", "\033[D" },
    { "kr", "\033[C" },
    { "ku", "\033[A" },
    { "md", "\033[1m" },
    { "me", "\033[0m" },
    { "nd", "\033[C" },
    { "se", "\033[0m" },
    { "so", "\033[7m" },
    { "ue", "\033[0m" },
    { "up", "\033[A" },
    { "us", "\033[4m" },
    { "vb", "\033[?5h\033[?5l" },
    { NULL, NULL }
};

static const PDC_TERM_NUM_CAP _pdc_vt_term_nums[] =
{
    { "co", 80 },
    { "li", 24 },
    { NULL, 0 }
};

static int _pdc_term_is_vt = FALSE;
static char _pdc_tgoto_buffer[64];

static bool _pdc_term_name_matches(const char *name, const char *candidate)
{
    const size_t candidate_length = strlen(candidate);

    return !strncmp(name, candidate, candidate_length) &&
           (name[candidate_length] == '\0' ||
            name[candidate_length] == '-' ||
            name[candidate_length] == '.' ||
            name[candidate_length] == '+');
}

static bool _pdc_term_name_is_in_list(const char *name, const char * const *list)
{
    while (*list)
    {
        if (_pdc_term_name_matches(name, *list))
            return true;

        list++;
    }

    return false;
}

static const char *_pdc_find_term_string(const char *id)
{
    const PDC_TERM_STRING_CAP *cap = _pdc_vt_term_strings;

    if (!_pdc_term_is_vt || !id)
        return NULL;

    while (cap->id)
    {
        if (!strcmp(cap->id, id))
            return cap->value;

        cap++;
    }

    return NULL;
}

static int _pdc_find_term_num(const char *id)
{
    const PDC_TERM_NUM_CAP *cap = _pdc_vt_term_nums;

    if (!_pdc_term_is_vt || !id)
        return ERR;

    while (cap->id)
    {
        if (!strcmp(cap->id, id))
            return cap->value;

        cap++;
    }

    return ERR;
}

static int _pdc_find_term_flag(const char *id)
{
    if (!_pdc_term_is_vt || !id)
        return FALSE;

    return !strcmp(id, "am") ||
           !strcmp(id, "km") ||
           !strcmp(id, "pt") ||
           !strcmp(id, "xn");
}

static void _pdc_add_tgoto_number(char **output, char *end, int value)
{
    const int written = snprintf(*output, (size_t)(end - *output), "%d", value);

    if (written > 0)
    {
        *output += written;
        if (*output > end)
            *output = end;
    }
}

int vidattr(chtype attr)
{
    PDC_LOG(("vidattr() - called: attr %d\n", attr));

    INTENTIONALLY_UNUSED_PARAMETER( attr);
    return ERR;
}

int vid_attr(attr_t attr, short color_pair, void *opt)
{
    PDC_LOG(("vid_attr() - called\n"));

    INTENTIONALLY_UNUSED_PARAMETER( attr);
    INTENTIONALLY_UNUSED_PARAMETER( color_pair);
    INTENTIONALLY_UNUSED_PARAMETER( opt);
    return ERR;
}

int vidputs(chtype attr, int (*putfunc)(int))
{
    PDC_LOG(("vidputs() - called: attr %d\n", attr));

    INTENTIONALLY_UNUSED_PARAMETER( attr);
    INTENTIONALLY_UNUSED_PARAMETER( putfunc);
    return ERR;
}

int vid_puts(attr_t attr, short color_pair, void *opt, int (*putfunc)(int))
{
    PDC_LOG(("vid_puts() - called\n"));

    INTENTIONALLY_UNUSED_PARAMETER( attr);
    INTENTIONALLY_UNUSED_PARAMETER( color_pair);
    INTENTIONALLY_UNUSED_PARAMETER( opt);
    INTENTIONALLY_UNUSED_PARAMETER( putfunc);
    return ERR;
}

int del_curterm(TERMINAL *oterm)
{
    PDC_LOG(("del_curterm() - called\n"));

    INTENTIONALLY_UNUSED_PARAMETER( oterm);
    return ERR;
}

int putp(const char *str)
{
    PDC_LOG(("putp() - called: str %s\n", str));

    return tputs(str, 1, putchar);
}

int restartterm(const char *term, int filedes, int *errret)
{
    PDC_LOG(("restartterm() - called\n"));

    if (errret)
        *errret = -1;

    INTENTIONALLY_UNUSED_PARAMETER( term);
    INTENTIONALLY_UNUSED_PARAMETER( filedes);
    return ERR;
}

TERMINAL *set_curterm(TERMINAL *nterm)
{
    PDC_LOG(("set_curterm() - called\n"));

    INTENTIONALLY_UNUSED_PARAMETER( nterm);
    return (TERMINAL *)NULL;
}

int setterm(const char *term)
{
    PDC_LOG(("setterm() - called\n"));

    return tgetent(NULL, term) == 1 ? OK : ERR;
}

int setupterm(const char *term, int filedes, int *errret)
{
    PDC_LOG(("setupterm() - called\n"));

    if (tgetent(NULL, term) == 1)
    {
        if (errret)
            *errret = 1;

        INTENTIONALLY_UNUSED_PARAMETER( filedes);
        return OK;
    }

    if (errret)
        *errret = 0;

    INTENTIONALLY_UNUSED_PARAMETER( filedes);
    return ERR;
}

int tgetent(char *bp, const char *name)
{
    PDC_LOG(("tgetent() - called: name %s\n", name));

    if (bp)
        *bp = '\0';

    _pdc_term_is_vt = FALSE;

    if (!name || !*name)
        name = getenv("TERM");

    if (!name || !*name || _pdc_term_name_is_in_list(name, _pdc_dumb_term_names))
        return 0;

    if (_pdc_term_name_is_in_list(name, _pdc_vt_term_names))
    {
        _pdc_term_is_vt = TRUE;
        return 1;
    }

    return 0;
}

int tgetflag(const char *id)
{
    PDC_LOG(("tgetflag() - called: id %s\n", id));

    return _pdc_find_term_flag(id);
}

int tgetnum(const char *id)
{
    PDC_LOG(("tgetnum() - called: id %s\n", id));

    return _pdc_find_term_num(id);
}

char *tgetstr(const char *id, char **area)
{
    const char *cap;

    PDC_LOG(("tgetstr() - called: id %s\n", id));

    cap = _pdc_find_term_string(id);

    if (!cap)
        return (char *)NULL;

    if (area && *area)
    {
        char *result = *area;

        strcpy(*area, cap);
        *area += strlen(cap) + 1;
        return result;
    }

    return (char *)cap;
}

char *tgoto(const char *cap, int col, int row)
{
    const char *input;
    char *output;
    char *end;
    int values[2];
    int current_value;

    PDC_LOG(("tgoto() - called\n"));

    if (!cap)
        return (char *)NULL;

    input = cap;
    output = _pdc_tgoto_buffer;
    end = _pdc_tgoto_buffer + sizeof(_pdc_tgoto_buffer) - 1;
    values[0] = row;
    values[1] = col;
    current_value = 0;

    while (*input && output < end)
    {
        if (*input != '%')
        {
            *output++ = *input++;
            continue;
        }

        input++;

        switch (*input)
        {
        case '\0':
            input--;
            break;

        case '%':
            *output++ = '%';
            break;

        case 'i':
            values[0]++;
            values[1]++;
            break;

        case 'r':
        {
            const int temp = values[0];

            values[0] = values[1];
            values[1] = temp;
            break;
        }

        case 'd':
            _pdc_add_tgoto_number(&output, end, values[current_value]);
            if (current_value < 1)
                current_value++;
            break;

        default:
            if (output + 1 < end)
            {
                *output++ = '%';
                *output++ = *input;
            }
            break;
        }

        if (*input)
            input++;
    }

    *output = '\0';
    return _pdc_tgoto_buffer;
}

int tigetflag(const char *capname)
{
    PDC_LOG(("tigetflag() - called: capname %s\n", capname));

    return _pdc_find_term_flag(capname);
}

int tigetnum(const char *capname)
{
    PDC_LOG(("tigetnum() - called: capname %s\n", capname));

    return _pdc_find_term_num(capname);
}

char *tigetstr(const char *capname)
{
    PDC_LOG(("tigetstr() - called: capname %s\n", capname));

    return tgetstr(capname, NULL);
}

char *tparm(const char *cap, long p1, long p2, long p3, long p4,
            long p5, long p6, long p7, long p8, long p9)
{
    PDC_LOG(("tparm() - called: cap %s\n", cap));
    INTENTIONALLY_UNUSED_PARAMETER( cap);
    INTENTIONALLY_UNUSED_PARAMETER( p1);
    INTENTIONALLY_UNUSED_PARAMETER( p2);
    INTENTIONALLY_UNUSED_PARAMETER( p3);
    INTENTIONALLY_UNUSED_PARAMETER( p4);
    INTENTIONALLY_UNUSED_PARAMETER( p5);
    INTENTIONALLY_UNUSED_PARAMETER( p6);
    INTENTIONALLY_UNUSED_PARAMETER( p7);
    INTENTIONALLY_UNUSED_PARAMETER( p8);
    INTENTIONALLY_UNUSED_PARAMETER( p9);

    return (char *)NULL;
}

int tputs(const char *str, int affcnt, int (*putfunc)(int))
{
    PDC_LOG(("tputs() - called\n"));

    INTENTIONALLY_UNUSED_PARAMETER( affcnt);

    if (!str || !putfunc)
        return ERR;

    while (*str)
        putfunc((unsigned char)*str++);

    return OK;
}
