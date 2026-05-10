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

  WaitForSingleObject(placar.hEventoTerminar, INFINITE);

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

BOOL initEventos(PLACAR *placar) {
  placar->hEventoNotificar = CreateEvent(NULL, TRUE, FALSE, NULL);
  placar->hEventoCancelar = CreateEvent(NULL, TRUE, FALSE, NULL);
  placar->hEventoTerminar = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (placar->hEventoNotificar == NULL || placar->hEventoTerminar == NULL || placar->hEventoCancelar == NULL) {
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
      desligarPlacar(placar, TRUE);
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

      // apenas ativos quando está a decorrer um alerta
      placar->hTimer,
      placar->hEventoCancelar
  };

  placar->alertaAtivo = FALSE;
  while (!placar->terminar) {
    DWORD result = WaitForMultipleObjects(placar->alertaAtivo ? 4 : 2, eventos, FALSE, INFINITE);

    // terminar
    if (result == WAIT_OBJECT_0) {
      placar->alertaAtivo = FALSE;
      continue;
    }

    // notificacao
    if (result == WAIT_OBJECT_0 + 1) {
      ResetEvent(placar->hEventoNotificar);

      EnterCriticalSection(&placar->cs);
      MSG_ALERTA alerta = placar->alertaAtual;
      mostrarHora();
      _tprintf(STR_ALERTA, alerta.msg, alerta.duracao);
      _tprintf(STR_CMD_INPUT);
      LeaveCriticalSection(&placar->cs);

      placar->alertaAtivo = TRUE;
      initTimer(placar->hTimer, alerta.duracao);
    }

    // timer expirou
    if (placar->alertaAtivo && result == WAIT_OBJECT_0 + 2) {
      EnterCriticalSection(&placar->cs);
      mostrarHora();
      _tprintf(STR_ALERTA_FIM);
      _tprintf(STR_CMD_INPUT);
      LeaveCriticalSection(&placar->cs);

      // notificar thread de escuta para enviar fim de alerta ao central
      if(placar->ligado) {
        CancelSynchronousIo(placar->hThreadEscutaCentral);
      }

      placar->alertaAtivo = FALSE;
      CancelWaitableTimer(placar->hTimer);
    }

    // alerta cancelado pelo central
    if (placar->alertaAtivo && result == WAIT_OBJECT_0 + 3) {
      ResetEvent(placar->hEventoCancelar);

      EnterCriticalSection(&placar->cs);
      mostrarHora();
      LOG("Alerta cancelado");
      _tprintf(STR_CMD_INPUT);
      LeaveCriticalSection(&placar->cs);

      placar->alertaAtivo = FALSE;
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

  // iniciar escuta da central
  placar->hThreadEscutaCentral = CreateThread(NULL, 0, threadEscutaCentral, placar, 0, NULL);
  if (placar->hThreadEscutaCentral == NULL) {
    LOG("Erro ao iniciar a escuta da central.");
  }
}

void desligarPlacar(PLACAR *placar, BOOL comConfirmacao) {
  if(placar->terminar == TRUE){
    // ja foi enviado pedido de desligar
    return;
  }

  LOG("A desligar...");

  // desbloquear thread de escuta, para podermos fazer writes / reads
  // sem usar overlapped IO
  placar->terminar = TRUE;
  placar->ligado = FALSE;
  CancelSynchronousIo(placar->hThreadEscutaCentral);

  if (comConfirmacao) {
    MSG_CMD msg = { .tipo = DESLIGAR };
    DWORD escrito, lido;

    BOOL res = WriteFile(placar->hPipe, &msg, sizeof(MSG_CMD), &escrito, NULL);

    if(res == 0 || escrito != sizeof(msg)) {
      LOG("Nao foi possivel enviar pedido de confirmacao, encerrando sistema...");
    } else {
      res = ReadFile(placar->hPipe, &msg, sizeof(MSG_CMD), &lido, NULL);

      if(res == 0 || escrito != sizeof(msg)) {
        LOG("Nao foi possivel receber confirmacao, encerrando sistema...");
      } else {
        LOG("Recebi confirmacao");
      }
    }
  }

  SetEvent(placar->hEventoTerminar);
}


// Thread responsavel por reads/writes do/ao central
// Se for preciso fazer um write/read de outra thread,
// temos de primeiro desbloquear esta thread com
// CancelSynchronousIo(placar->hThreadEscutaCentral);
// (fazemos isto ao desligar e ao enviar o fim de alerta)
DWORD WINAPI threadEscutaCentral(LPVOID param) {
  PLACAR* placar = (PLACAR*)param;
  DWORD lido, escrito;
  BYTE buffer[sizeof(MSG_ALERTA)] = {0}; // Buffer com o tamanho da maior mensagem

  while(!placar->terminar && placar->ligado) {
      BOOL res = ReadFile(placar->hPipe, buffer, sizeof(buffer), &lido, NULL);

      if (!res || lido == 0) {
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
          // abortado pelo comando desligar
          if(placar->terminar) {
            break;
          } else {
            // abortado pelo fim alerta
            buffer[0] = FIM_ALERTA;
          }
        } else {
          LOG("Conexao perdida, encerrando placar...");
          placar->ligado = FALSE;
          placar->terminar = TRUE;
          SetEvent(placar->hEventoTerminar);
          break;
        }
      }

      BYTE tipo = buffer[0];
      switch(tipo) {
          case NOVO_ALERTA: {
              //LOG("Novo Alerta!");
              MSG_ALERTA *alerta = (MSG_ALERTA*)buffer;

              // enviar confirmacao
              WriteFile(placar->hPipe, alerta, sizeof(MSG_ALERTA), &escrito, NULL);

              EnterCriticalSection(&placar->cs);
              memcpy(&placar->alertaAtual, alerta, sizeof(MSG_ALERTA));
              LeaveCriticalSection(&placar->cs);
              SetEvent(placar->hEventoNotificar);
              break;
          }
          case CANCELAR_ALERTA: {
              if(!placar->alertaAtivo) {
                break;
              };

              LOG("Alerta cancelado");
              MSG_CMD cmd = { .tipo = CANCELAR_ALERTA };

              // enviar confirmacao
              WriteFile(placar->hPipe, &cmd, sizeof(MSG_CMD), &escrito, NULL);
              SetEvent(placar->hEventoCancelar);
              break;
          }
          case FIM_ALERTA: {
              MSG_CMD fim = { .tipo = FIM_ALERTA };
              DWORD escrito;

              // enviar confirmacao
              WriteFile(placar->hPipe, &fim, sizeof(MSG_CMD), &escrito, NULL);
              break;
          }
          case ENCERRAR_PLATAFORMA: {
              LOG("Encerramento Sistema...");
              desligarPlacar(placar, FALSE);
              break;
          }
      }
  }
  return 0;
}

BOOL initThreads(PLACAR *placar) {
  placar->hThreadComandos = CreateThread(NULL, 0, threadComandos, placar, 0, NULL);
  placar->hThreadAlertas = CreateThread(NULL, 0, threadAlertas, placar, 0, NULL);

  if (placar->hThreadComandos == NULL || placar->hThreadAlertas == NULL) {
    _tprintf(STR_ERRO_CRIAR_THREADS);
    return FALSE;
  }

  return TRUE;
}

void encerrarPlacar(PLACAR *placar) {
  if (placar->hThreadComandos) {
    WaitForSingleObject(placar->hThreadComandos, 1000);
    CloseHandle(placar->hThreadComandos);
    placar->hThreadComandos = NULL;
  }
  
  if (placar->hThreadAlertas) {
    WaitForSingleObject(placar->hThreadAlertas, 1000);
    CloseHandle(placar->hThreadAlertas);
    placar->hThreadAlertas = NULL;
  }

  if (placar->hThreadEscutaCentral) {
      if (placar->hPipe && placar->hPipe != INVALID_HANDLE_VALUE) {
          CloseHandle(placar->hPipe);
          placar->hPipe = NULL;
      }
      WaitForSingleObject(placar->hThreadEscutaCentral, 1000);
      CloseHandle(placar->hThreadEscutaCentral);
      placar->hThreadEscutaCentral = NULL;
  }
  
  if (placar->hEventoNotificar) CloseHandle(placar->hEventoNotificar);
  if (placar->hEventoTerminar) CloseHandle(placar->hEventoTerminar);
  if (placar->hEventoCancelar) CloseHandle(placar->hEventoCancelar);

  DeleteCriticalSection(&placar->cs);
}
