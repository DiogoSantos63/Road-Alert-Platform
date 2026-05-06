#ifndef types_h
#define types_h

#include <tchar.h>
#include <windows.h>

#include "Constantes.h"

typedef enum {
  LIGAR = 1,
  DESLIGAR = 2,
  FIM_ALERTA = 3,
  NOVO_ALERTA = 4,
  CANCELAR_ALERTA = 5,
  ENCERRAR_PLATAFORMA = 6,
  ATRIBUIR_IDENTIFICADOR = 7,
} TIPO_MSG;

typedef struct {
  BYTE tipo;
} MSG_CMD;

typedef struct {
  BYTE tipo;
  DWORD identificador;
} MSG_ID;

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
