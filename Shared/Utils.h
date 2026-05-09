#ifndef UTILS_H
#define UTILS_H

#define UNICODE
#define _UNICODE

#define LOG(str) _tprintf(_T(str)); _tprintf("\n")
#define LOCK(mutex) WaitForSingleObject(mutex, INFINITE)
#define UNLOCK(mutex) ReleaseMutex(mutex)

#include <fcntl.h>
#include <io.h>
#include <stdio.h>

static void initConsola() {
#ifdef UNICODE
  _setmode(_fileno(stdin), _O_WTEXT);
  _setmode(_fileno(stdout), _O_WTEXT);
#endif
}

#endif
