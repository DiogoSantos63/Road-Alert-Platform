#ifndef MONITOR_H
#define MONITOR_H

#include "resource.h"

extern WCHAR nomeSHM[256];

extern WCHAR nomeEvento[256];

ATOM registarClasse(HINSTANCE hInstance);

BOOL initJanela(HINSTANCE hInstance, int nCmdShow);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK DlgConfiguracaoProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK DlgAcercaProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI threadEscutaCentral(LPVOID param);

BOOL ligarSharedMemory();

void libertarRecursos();

#endif