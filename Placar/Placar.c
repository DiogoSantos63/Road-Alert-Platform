#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include "Constantes.h"
#include "Placar.h"
#include "Strings.h"
#include "Types.h"
#include "Utils.h"

int _tmain(int argc, TCHAR *argv[]) {
  initConsola();

  PLACAR placar = {0};
  if (!validarArgumentos(&placar, argc, argv)) {
    return 1;
  }

  InitializeCriticalSection(&placar.cs);
  if (!(initEventos(&placar) && initThreads(&placar))) {
    encerrarPlacar(&placar);
    return 1;
  }

  WaitForMultipleObjects(2, placar.hThreads, TRUE, INFINITE);
  encerrarPlacar(&placar);

  return 0;
}

BOOL validarArgumentos(PLACAR *placar, int argc, TCHAR *argv[]) {
  if (argc > 1) {
    lstrcpy(placar->nomePipe, argv[1]);

    if (!escreverRegistry(placar->nomePipe)) {
      _tprintf(STR_REGISTRY_SAVE_ERROR);
    }
  } else if (!lerRegistry(placar->nomePipe)) {
    _tprintf(STR_REGISTRY_PIPE_NAME_NOT_FOUND);
    return FALSE;
  };

  _tprintf(STR_NAMED_PIPE, placar->nomePipe);

  return TRUE;
}

BOOL lerRegistry(TCHAR *nomePipe) {
  HKEY hKey;
  DWORD size = TAM_NOME_PIPE * sizeof(TCHAR);

  if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    return FALSE;
  }

  LONG result = RegQueryValueEx(hKey, REGISTRY_VALUE_NPIPE, NULL, NULL, (LPBYTE)nomePipe, &size);
  RegCloseKey(hKey);

  return result == ERROR_SUCCESS;
}

BOOL escreverRegistry(TCHAR *nomePipe) {
  HKEY hKey;
  DWORD size = (lstrlen(nomePipe) + 1) * sizeof(TCHAR);

  LONG result =
      RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

  if (result != ERROR_SUCCESS) {
    return FALSE;
  }

  result = RegSetValueEx(hKey, REGISTRY_VALUE_NPIPE, 0, REG_SZ, (LPBYTE)nomePipe, size);
  RegCloseKey(hKey);

  return result == ERROR_SUCCESS;
}

BOOL initThreads(PLACAR *placar) {
  placar->hThreads[0] = CreateThread(NULL, 0, threadComandos, placar, 0, NULL);
  placar->hThreads[1] = CreateThread(NULL, 0, threadAlertas, placar, 0, NULL);

  if (placar->hThreads[0] == NULL || placar->hThreads[1] == NULL) {
    _tprintf(STR_ERRO_CRIAR_THREADS);
    return FALSE;
  }

  return TRUE;
}

BOOL initEventos(PLACAR *placar) {
  placar->hEventoNotificar = CreateEvent(NULL, TRUE, FALSE, EVENTO_NOTIFICAR);
  placar->hEventoTerminar = CreateEvent(NULL, TRUE, FALSE, NULL);

  if (placar->hEventoNotificar == NULL || placar->hEventoTerminar == NULL) {
    _tprintf(STR_ERRO_CRIAR_EVENTOS);
    return FALSE;
  }

  return TRUE;
}

DWORD WINAPI threadComandos(LPVOID param) {
  PLACAR *placar = (PLACAR *)param;
  TCHAR cmd[TAM_CMD];

  while (!placar->terminar) {
    EnterCriticalSection(&placar->cs);
    _tprintf(STR_CMD_INPUT);
    LeaveCriticalSection(&placar->cs);
    _fgetts(cmd, TAM_CMD, stdin);

    size_t len = _tcslen(cmd);
    if (len > 0 && cmd[len - 1] == _T('\n')) cmd[len - 1] = _T('\0');

    switch (parsearComando(cmd)) {
    case CMD_LIGAR:
      ligarPlacar(placar);
      break;
    case CMD_DESLIGAR:
      desligarPlacar(placar);
      break;
    case CMD_DESCONHECIDO:
      _tprintf(STR_COMANDO_DESCONHECIDO);
      break;
    }
  }
  return 0;
}

COMANDO parsearComando(TCHAR *cmd) {
  if (_tcscmp(cmd, STR_CMD_LIGAR) == 0) return CMD_LIGAR;
  if (_tcscmp(cmd, STR_CMD_DESLIGAR) == 0) return CMD_DESLIGAR;
  return CMD_DESCONHECIDO;
}

DWORD WINAPI threadAlertas(LPVOID param) {
  PLACAR *placar = (PLACAR *)param;

  placar->hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
  if (placar->hTimer == NULL) {
    _tprintf(STR_ERRO_CRIAR_TIMER);
    return 1;
  }

  HANDLE eventos[] = {
      placar->hEventoTerminar,
      placar->hEventoNotificar,
      placar->hTimer,
  };

  BOOL esperar_por_timer = FALSE;
  while (!placar->terminar) {
    DWORD result = WaitForMultipleObjects(esperar_por_timer ? 3 : 2, eventos, FALSE, INFINITE);

    // terminar
    if (result == WAIT_OBJECT_0) {
      esperar_por_timer = FALSE;
      continue;
    }

    // notificacao
    if (result == WAIT_OBJECT_0 + 1) {
      ResetEvent(placar->hEventoNotificar);

      MSG_ALERTA alerta = {0};
      if (!lerAlertaRegistry(placar->nomePipe, &alerta)) {
        EnterCriticalSection(&placar->cs);
        _tprintf(_T("\n"));
        _tprintf(STR_ERRO_ALERTA_REGISTRY);
        _tprintf(STR_CMD_INPUT);
        LeaveCriticalSection(&placar->cs);
        continue;
      }

      EnterCriticalSection(&placar->cs);
      mostrarHora();
      _tprintf(STR_ALERTA, alerta.msg, alerta.duracao);
      _tprintf(STR_CMD_INPUT);
      LeaveCriticalSection(&placar->cs);

      esperar_por_timer = TRUE;
      initTimer(placar->hTimer, alerta.duracao);
    }

    // timer expirou
    if (result == WAIT_OBJECT_0 + 2) {
      ResetEvent(placar->hEventoNotificar);

      EnterCriticalSection(&placar->cs);
      mostrarHora();
      _tprintf(STR_ALERTA_FIM);
      _tprintf(STR_CMD_INPUT);
      LeaveCriticalSection(&placar->cs);

      esperar_por_timer = FALSE;
      CancelWaitableTimer(placar->hTimer);
    }
  }

  CloseHandle(placar->hTimer);
  placar->hTimer = NULL;
  return 0;
}

BOOL lerAlertaRegistry(TCHAR *nomePipe, MSG_ALERTA *alerta) {
  HKEY hKey;
  DWORD size = sizeof(MSG_ALERTA);

  if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    return FALSE;
  }

  LONG result = RegQueryValueEx(hKey, nomePipe, NULL, NULL, (LPBYTE)alerta, &size);

  RegCloseKey(hKey);
  return result == ERROR_SUCCESS;
}

void mostrarHora() {
  SYSTEMTIME systemTime;

  GetLocalTime(&systemTime);

  _tprintf(STR_DATA_HORA, systemTime.wDay, systemTime.wMonth, systemTime.wYear, systemTime.wHour, systemTime.wMinute,
           systemTime.wSecond);
}

void initTimer(HANDLE hTimer, DWORD duracao) {
  LARGE_INTEGER li;
  li.QuadPart = -(LONGLONG)duracao * TICKS;
  SetWaitableTimer(hTimer, &li, 0, NULL, NULL, FALSE);
}

void ligarPlacar(PLACAR *placar) {
  if (placar->ligado) {
    _tprintf(STR_PLACAR_LIGADO);
    return;
  }

  TCHAR nomePipeCompleto[TAM_NOME_PIPE];
  _tcscpy_s(nomePipeCompleto, TAM_NOME_PIPE, CENTRAL_PIPE);
  _tcscat_s(nomePipeCompleto, TAM_NOME_PIPE, placar->nomePipe);

  placar->hPipe = CreateFile( //
      nomePipeCompleto, //
      GENERIC_READ | GENERIC_WRITE, //
      0, //
      NULL, //
      OPEN_EXISTING, //
      0, //
      NULL); //

  //TODO: WaitNamedPipe para esperar até haver uma instancia de pipe disponivel
  if (placar->hPipe == INVALID_HANDLE_VALUE) {
    _tprintf(STR_ERRO_LIGAR_CENTRAL, GetLastError());
    return;
  }

  DWORD modo = PIPE_READMODE_MESSAGE;
  SetNamedPipeHandleState(placar->hPipe, &modo, NULL, NULL);

  MSG_CMD msg = {.tipo = LIGAR};

  DWORD escrito;
  WriteFile(placar->hPipe, &msg, sizeof(MSG_CMD), &escrito, NULL);

  MSG_ID resposta = {0};
  DWORD lido;
  BOOL res;

  res = ReadFile(placar->hPipe, &resposta, sizeof(MSG_ID), &lido, NULL);

  if (resposta.tipo != ATRIBUIR_IDENTIFICADOR) {
    _tprintf(STR_ERRO_LIGAR_CENTRAL, 0);
    return;
  }

  placar->identificador = resposta.identificador;
  placar->ligado = TRUE;
  _tprintf(STR_PLACAR_ID, placar->identificador);
}

void desligarPlacar(PLACAR *placar) {
  LOG("A desligar placar");

  MSG_CMD msg = { .tipo = DESLIGAR };

  DWORD escrito;
  WriteFile(placar->hPipe, &msg, sizeof(MSG_CMD), &escrito, NULL);

  MSG_CMD resposta = {0};
  DWORD lido;
  BOOL res;


  res = ReadFile(placar->hPipe, &resposta, sizeof(MSG_CMD), &lido, NULL);

  if (res && resposta.tipo == DESLIGAR) {
    LOG("Recebi confirmaçao desligar");
  }

  placar->terminar = TRUE;
  SetEvent(placar->hEventoTerminar);

  _tprintf(STR_PLACAR_ENCERRAR);
}

void encerrarPlacar(PLACAR *placar) {
  if (placar->hThreads[0]) CloseHandle(placar->hThreads[0]);
  if (placar->hThreads[1]) CloseHandle(placar->hThreads[1]);
  if (placar->hEventoNotificar) CloseHandle(placar->hEventoNotificar);
  if (placar->hEventoTerminar) CloseHandle(placar->hEventoTerminar);

  DeleteCriticalSection(&placar->cs);
}
