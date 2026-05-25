#define UNICODE
#define _UNICODE

#include <windows.h>
#include "Monitor.h"
#include <tchar.h>
#include "Resource.h"
#include "../Shared/Types.h"
#include "../Shared/Constantes.h"


HINSTANCE hInst;

HANDLE hSharedMem = NULL;

SHM_ALERTA* pShm = NULL;

HANDLE hEventoMonitor = NULL;

HANDLE hThreadMonitor = NULL;

HWND hWndPrincipal = NULL;

int maxAlertas = 5;

int paginaAtual = 0;


// --- WinMain ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;

    if (!registarClasse(hInstance))   return 1;
    if (!initJanela(hInstance, nCmdShow)) return 1;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

// --- Registar classe da janela ---
ATOM registarClasse(HINSTANCE hInstance) {
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
    wcex.lpszClassName = TEXT("MonitorClass");
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    return RegisterClassEx(&wcex);
}

// --- Criar e mostrar janela ---
BOOL initJanela(HINSTANCE hInstance, int nCmdShow) {
    hWndPrincipal = CreateWindowEx(
        0,
        TEXT("MonitorClass"),
        TEXT("Monitor - Placar Informativo"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        700, 500,
        NULL, NULL,
        hInstance, NULL
    );

    if (!hWndPrincipal) return FALSE;

    ShowWindow(hWndPrincipal, nCmdShow);
    UpdateWindow(hWndPrincipal);
    return TRUE;
}

// --- Ligar à memória partilhada ---
BOOL ligarSharedMemory() {
    hSharedMem = OpenFileMapping(FILE_MAP_READ, FALSE, NOME_SHM_ALERTA);
    if (hSharedMem == NULL) {
        MessageBox(NULL, TEXT("Erro ao abrir memoria partilhada.\nO central esta a correr?"),
            L"Erro", MB_ICONERROR);
        return FALSE;
    }

    pShm = (SHM_ALERTA*)MapViewOfFile(hSharedMem, FILE_MAP_READ, 0, 0, sizeof(SHM_ALERTA));
    if (pShm == NULL) {
        CloseHandle(hSharedMem);
        MessageBox(NULL, TEXT("Erro ao mapear memoria partilhada."), TEXT("Erro"), MB_ICONERROR);
        return FALSE;
    }

    hEventoMonitor = OpenEvent(SYNCHRONIZE, FALSE, NOME_EVENTO_MONITOR);
    if (hEventoMonitor == NULL) {
        UnmapViewOfFile(pShm);
        CloseHandle(hSharedMem);
        MessageBox(NULL, TEXT"Erro ao abrir evento do monitor."), TEXT("Erro"), MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

// --- Thread que espera notificações do central ---
DWORD WINAPI threadEscutaCentral(LPVOID param) {
    while (TRUE) {
        WaitForSingleObject(hEventoMonitor, INFINITE);

        if (pShm && pShm->desligar) {
            MessageBox(hWndPrincipal, TEXT("Central encerrou a plataforma."), TEXT("Info"), MB_ICONINFORMATION);
            PostMessage(hWndPrincipal, WM_DESTROY, 0, 0);
            break;
        }

        // força redesenho da janela
        InvalidateRect(hWndPrincipal, NULL, TRUE);
    }
    return 0;
}

// --- Libertar recursos ---
void libertarRecursos() {
    if (hThreadMonitor) { CloseHandle(hThreadMonitor); hThreadMonitor = NULL; }
    if (pShm) { UnmapViewOfFile(pShm);       pShm = NULL; }
    if (hSharedMem) { CloseHandle(hSharedMem);     hSharedMem = NULL; }
    if (hEventoMonitor) { CloseHandle(hEventoMonitor); hEventoMonitor = NULL; }
}

// --- WndProc ---
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE:
        // liga à SHM e lança thread
        if (ligarSharedMemory()) {
            hThreadMonitor = CreateThread(NULL, 0, threadEscutaCentral, NULL, 0, NULL);
        }
        break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDM_CONFIGURACAO:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURACAO), hWnd, DlgConfiguracaoProc);
            break;
        case IDM_ACERCA:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ACERCA), hWnd, DlgAcercaProc);
            break;
        case IDM_SAIR:
            DestroyWindow(hWnd);
            break;
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        if (pShm) {
            // conta alertas ativos
            int total = 0;
            for (int i = 0; i < MAX_PLACARES; i++)
                if (pShm->placar[i].identificador != 0 && pShm->placar[i].msg[0] != L'\0')
                    total++;

            int y = 10;
            int count = 0;
            int inicio = paginaAtual * maxAlertas;

            for (int i = 0; i < MAX_PLACARES; i++) {
                if (pShm->placar[i].identificador == 0) continue;
                if (pShm->placar[i].msg[0] == L'\0') continue;

                if (count < inicio) { count++; continue; }
                if (count >= inicio + maxAlertas) break;

                WCHAR linha[300];
                wsprintf(linha, TEXT("Placar %d: %s (%d segundos)"),
                    pShm->placar[i].identificador,
                    pShm->placar[i].msg,
                    pShm->placar[i].duracao);

                TextOut(hdc, 10, y, linha, lstrlen(linha));
                y += 30;
                count++;
            }

            // indica página
            WCHAR pagina[50];
            wsprintf(pagina, TEXT("Pagina %d  [PageUp/PageDown para navegar]"), paginaAtual + 1);
            TextOut(hdc, 10, 450, pagina, lstrlen(pagina));
        }
        else {
            TextOut(hdc, 10, 10, TEXT("A aguardar ligacao ao central..."), 32);
        }

        EndPaint(hWnd, &ps);
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_NEXT) {   // Page Down
            paginaAtual++;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (wParam == VK_PRIOR) {  // Page Up
            if (paginaAtual > 0) paginaAtual--;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_DESTROY:
        libertarRecursos();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// --- Dialog Configuracao ---
INT_PTR CALLBACK DlgConfiguracaoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        // preenche o campo com o valor atual
        WCHAR buf[10];
        wsprintf(buf, TEXT("%d", maxAlertas));
        SetDlgItemText(hDlg, IDC_MAX_ALERTAS, buf);
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            WCHAR buf[10];
            GetDlgItemText(hDlg, IDC_MAX_ALERTAS, buf, 10);
            int val = _wtoi(buf);
            if (val > 0) maxAlertas = val;
            EndDialog(hDlg, IDOK);
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
        }
        break;
    }
    return FALSE;
}

// --- Dialog Acerca ---
INT_PTR CALLBACK DlgAcercaProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
        }
        break;
    }
    return FALSE;
}