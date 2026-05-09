#ifndef CENTRAL_H
#define CENTRAL_H

#include "Constantes.h"
#include "Types.h"

typedef enum { ESPERA_CONEXAO, LE_PEDIDO, ENVIA_RESPOSTA } PLACAR_STATE;

typedef struct {
  DWORD id;
  BOOL ligado;
  BOOL temAlerta;
  TCHAR msgAlerta[TAM_MSG];
  DWORD duracaoAlerta;

  HANDLE hPipeInstance;
  BYTE bufferPedido[MAX_BUF];
  DWORD bytesPedido;
  BYTE bufferResposta[MAX_BUF];
  DWORD bytesResposta;
  OVERLAPPED olpInstance;
  BOOL operacaoPendente;
  BOOL conexaoAtiva;
  PLACAR_STATE estadoIO;

  BOOL msg_por_enviar;
  TIPO_MSG tipo_msg_enviar;

  // HANDLE hThread;
} PLACAR_INFO;

typedef struct {
  TCHAR nomePipe[TAM_NOME_PIPE];
  HANDLE hPipe;
  HANDLE hThreads[THREADS_CENTRAL];

  PLACAR_INFO placares[MAX_PLACARES];
  HANDLE hEvents[MAX_PLACARES];
  DWORD iPlacarOperation; // index da instancia do pipe onde ocorreu operacao

  DWORD numPlacares;
  DWORD proximoId;
  HANDLE hMutex;
  BOOL terminar;
  BOOL mostrarPrompt;
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

DWORD WINAPI threadPlacaresOverlappedIO(LPVOID param);

void desconectarPlacar(CENTRAL *central, PLACAR_INFO *placar);

BOOL handleMsgPlacarReceber(CENTRAL *central, PLACAR_INFO *placar);

BOOL handleMsgPlacarEnviar(CENTRAL *central, PLACAR_INFO *placar);

DWORD WINAPI threadPlacares(LPVOID param);

DWORD WINAPI threadPlacar(LPVOID param);

BOOL cmdEnviarAlerta(CENTRAL *central, TCHAR *args);

BOOL enviarPlacar(CENTRAL *central, int id, void *msg, int size);

void enviarTodos(CENTRAL *central, void *msg, int size);

BOOL cmdCancelarAlerta(CENTRAL *central, TCHAR *args);

BOOL cmdListarPlacares(CENTRAL *central);

BOOL cmdListarComandos();

BOOL cmdEncerrarCentral(CENTRAL *central);

void processarLeituraConcluida(CENTRAL *central, PLACAR_INFO *placar);

void processarLigar(CENTRAL *central, int idx);

void processarDesligar(CENTRAL *central, int idx, BOOL *ativo);

void processarFimAlerta(CENTRAL *central, int idx);

void processarConfirmacaoAlerta(CENTRAL *central, int idx, HANDLE hPipe);

void processarConfirmacaoCancelar(CENTRAL *central, int idx, HANDLE hPipe);

void terminarCentral(CENTRAL *central);

#endif
