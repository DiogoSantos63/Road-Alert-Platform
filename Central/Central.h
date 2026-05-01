#ifndef CENTRAL_H
#define CENTRAL_H

#include "Types.h"
#include "Constantes.h"

typedef struct {
    DWORD id;
    HANDLE hPipe;          
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

BOOL validarArgumentos(CENTRAL* central, int argc, TCHAR* argv[]);

BOOL initCentral(CENTRAL* central);

BOOL criarPipe(CENTRAL* central);

BOOL initThreads(CENTRAL* central);

COMANDO parsearComando(TCHAR* cmd);

DWORD WINAPI threadComandos(LPVOID param);

DWORD WINAPI threadPlacares(LPVOID param);

BOOL enviarAlerta(CENTRAL* central, TCHAR* args);

BOOL enviarPlacar(CENTRAL* central, int id, void* msg, int size);

void enviarTodos(CENTRAL* central, void* msg, int size);

BOOL cancelarAlerta(CENTRAL* central);

BOOL listarPlacares(CENTRAL* central);

BOOL listarComandos();

BOOL encerrarCentral();

void terminarCentral(CENTRAL* central);

#endif
