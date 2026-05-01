#ifndef types_h
#define types_h

#include <windows.h>
#include <tchar.h>

#include "Constantes.h"

/*
	Placar -> Central:
	1 - ligar,
	2 - desligar
	3 - fim do alerta (duração)
	5 - cancelar alerta
	6 - encerrar plataforma
*/
typedef struct {
	BYTE tipo;
} MSG_CMD;

/*
	Central -> Placar:
	3 - atribuir identificador
*/
typedef struct {
	BYTE tipo;
	DWORD identificador;
} MSG_ID;

/*
	Central -> Placar:
	4 - novo alerta
*/
typedef struct {
	BYTE tipo;
	TCHAR msg[TAM_MSG];
	DWORD duracao;
} MSG_ALERTA;

// Central -> Monitor
typedef struct {
	DWORD identificador, duracao;
	TCHAR msg[TAM_MSG];
	BOOL desligar;
} SHM_ALERTA;

// Comandos
typedef enum {
	CMD_LIGAR,
	CMD_DESLIGAR,
	CMD_DESCONHECIDO,
	CMD_ALERTA,
	CMD_CANCELAR,
	CMD_LISTAR,
	CMD_ENCERRAR,
	CMD_AJUDA
} COMANDO;

#endif