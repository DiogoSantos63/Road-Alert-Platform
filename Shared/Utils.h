#ifndef UTILS_H
#define UTILS_H

#define UNICODE
#define _UNICODE

#include <io.h>
#include <fcntl.h>
#include <stdio.h>

static void initConsola() {
#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
#endif
}

#endif