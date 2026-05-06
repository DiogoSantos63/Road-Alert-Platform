#ifndef CENTRAL_H
#define CENTRAL_H

#include "Constantes.h"
#include "Types.h"

typedef struct {
  DWORD id;
  HANDLE hPipe;
  HANDLE hThread;
  BOOL ligado;
  BOOL temAlerta;
  TCHAR msgAlerta[TAM_MSG];
  DWORD duracaoAlerta;
} PLACAR_INFO;

typedef struct {
  TCHAR nomePipe[TAM_NOME_PIPE];
  HANDLE hPipe;
  HANDLE hThreads[THREADS_CENTRAL];
  PLACAR_INFO placares[MAX_PLACARES];
  DWORD numPlacares;
  DWORD proximoId;
  HANDLE hMutex;
  BOOL terminar;
} CENTRAL;

typedef struct {
  CENTRAL *central;
  int index;
} THREAD_PLACAR_ARGS;

BOOL validarArgumentos(CENTRAL *central, int argc, TCHAR *argv[]);

BOOL initCentral(CENTRAL *central);

BOOL criarPipe(CENTRAL *central);

BOOL initThreads(CENTRAL *central);

COMANDO parsearComando(TCHAR *cmd);

DWORD WINAPI threadComandos(LPVOID param);

DWORD WINAPI threadPlacares(LPVOID param);

DWORD WINAPI threadPlacar(LPVOID param);

BOOL cmdEnviarAlerta(CENTRAL *central, TCHAR *args);

BOOL enviarPlacar(CENTRAL *central, int id, void *msg, int size);

void enviarTodos(CENTRAL *central, void *msg, int size);

BOOL cmdCancelarAlerta(CENTRAL *central, TCHAR *args);

BOOL cmdListarPlacares(CENTRAL *central);

BOOL cmdListarComandos();

BOOL cmdEncerrarCentral(CENTRAL *central);

void processarLigar(CENTRAL *central, int idx);

void processarDesligar(CENTRAL *central, int idx, BOOL *ativo);

void processarFimAlerta(CENTRAL *central, int idx);

void processarConfirmacaoAlerta(CENTRAL *central, int idx, HANDLE hPipe);

void processarConfirmacaoCancelar(CENTRAL *central, int idx, HANDLE hPipe);

void terminarCentral(CENTRAL *central);

#endif
