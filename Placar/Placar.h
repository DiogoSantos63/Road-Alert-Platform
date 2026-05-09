#ifndef PLACAR_H
#define PLACAR_H

#include <tchar.h>
#include <windows.h>

#include "Constantes.h"
#include "Types.h"

typedef struct {
  TCHAR nomePipe[TAM_NOME_PIPE];
  DWORD identificador;
  BOOL ligado;
  BOOL terminar;
  HANDLE hPipe;
  HANDLE hEventoNotificar;
  HANDLE hEventoTerminar;
  HANDLE hEventoCancelar;
  HANDLE hTimer;
  MSG_ALERTA alertaAtual;
  BOOL alertaAtivo;
  CRITICAL_SECTION cs;

  HANDLE hThreadComandos;
  HANDLE hThreadAlertas;
  HANDLE hThreadEscutaCentral;
} PLACAR;


BOOL validarArgumentos(PLACAR *placar, int argc, TCHAR *argv[]);

BOOL lerRegistry(TCHAR *nomePipe);

BOOL escreverRegistry(TCHAR *nomePipe);

BOOL lerAlertaRegistry(TCHAR *nomePipe, MSG_ALERTA *alerta);

BOOL initThreads(PLACAR *placar);

BOOL initEventos(PLACAR *placar);

DWORD WINAPI threadComandos(LPVOID param);

DWORD WINAPI threadAlertas(LPVOID param);

DWORD WINAPI threadEscutaCentral(LPVOID param);

COMANDO parsearComando(TCHAR *cmd);

void ligarPlacar(PLACAR *placar);

void desligarPlacar(PLACAR *placar, BOOL comConfirmacao);

void mostrarHora();

void initTimer(HANDLE hTimer, DWORD duracao);

void encerrarPlacar(PLACAR *placar);

#endif
