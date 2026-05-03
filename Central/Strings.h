#ifndef STRINGS_H
#define STRINGS_H

#include <tchar.h> 

// Info
#define STR_EMPTY _T("")
#define STR_CENTRAL _T("\n[CENTRAL] ")
#define STR_HELP _T("[CENTRAL] Escreva <ajuda> para listar os comandos disponíveis\n")
#define STR_CMD_INPUT _T("CMD> ")
#define STR_NAMED_PIPE _T("[CENTRAL] Named Pipe = '%s'\n")
#define STR_CENTRAL_ACTIVE _T("[CENTRAL] Central ativa!\n")
#define STR_PLACARES_ATIVOS _T("----- [Placares %d] -----\n")
#define STR_INFO_PLACAR _T("[PLACAR %d] - %s\n")
#define STR_PLACAR_ATIVO _T("Ativo")
#define STR_PLACAR_DESATIVO _T("Desativo")
#define STR_INSERT_PLACAR_ID_OR_ALL _T("Insira o ID do placar: ")
#define STR_WAITING_CONNECTION _T("\nA aguardar conexão...\n")
#define STR_PLACAR_LIGADO _T("\n[CENTRAL] Placar ligado %d\n")
#define STR_PLACAR_DESLIGADO _T("\n[CENTRAL] Placar desligado %d\n")
#define STR_PLACAR_FIM_ALERTA _T("\n[CENTRAL] Placar %d fim de alerta\n")
#define STR_ALERTA_CONFIRMADO _T("\n[CENTRAL] Placar %d - Alerta confirmado\n")
#define STR_ALERTA_CONFIRMADO _T("\n[CENTRAL] Placar %d - Alerta confirmado\n")
#define STR_CANCELAR_CONFIRMADO _T("\n[CENTRAL] Placar %d - Cancelar confirmado\n")
#define STR_ALERTA_CANCELADO _T("\n[CENTRAL] Alerta %d cancelado\n")
#define STR_CENTRAL_ENCERRAR _T("\n[CENTRAL] A encerrar a central...\n")

// Erros
#define STR_PIPE_NAME_NOT_FOUND _T("[ERRO] O nome do 'PIPE' não foi especificado!\n")
#define STR_PIPE_NAME_EXCEEDED _T("[ERRO] O nome do PIPE excedeu o limite!\n")
#define STR_ERRO_INIT_THREADS _T("[ERRO] Erro ao criar threads\n")
#define STR_ERRO_COMANDO _T("[ERRO] Comando desconhecido!\n")
#define STR_MUTEX_CREATION_ERROR _T("[ERRO] Erro ao criar o Mutex: %lu\n")
#define STR_NAMED_PIPE_CREATION _T("[ERRO] Erro ao criar o NamedPipe: %lu\n")
#define STR_CMD_ALERTA_ERRO _T("[ERRO] Uso: alerta <msg> <duracao> [id]\n")
#define STR_CMD_CANCELAR_ERRO _T("[ERRO] Uso: cancelar <msg> <duracao> [id]\n")
#define STR_MAX_PLACARES_REACHED _T("[ERRO] Limite de placares atingido!\n")
#define STR_ERRO_CONNECT_PIPE _T("[ERRO] Não foi possível ligar ao PIPE: %s\n")
#define STR_ERRO_MAX_PLACARES _T("[ERRO] Máximo de placares atingido!\n")
#define STR_PLACAR_DESCONECTADO _T("[ERRO] Placar %d desconectado inesperadamente...\n")
#define STR_ERRO_MSG _T("[ERRO] Erro ao ler mensagem: %d\n")
#define STR_PLACAR_NAO_ENCONTRADO _T("[ERRO] Placar não encontrado: %d\n")

// Comandos
#define STR_CMD_COMANDOS _T("Comandos disponíveis: \n\n")
#define STR_CMD_ALERTA _T("alerta")
#define STR_CMD_CANCELAR _T("cancelar")
#define STR_CMD_LISTAR _T("listar")
#define STR_CMD_ENCERRAR _T("encerrar")
#define STR_CMD_AJUDA _T("ajuda")

#define STR_CMD_ALERTA_HELP _T("<alerta> - Envia um alerta\n")
#define STR_CMD_CANCELAR_HELP _T("<cancelar> - Cancela um alerta\n")
#define STR_CMD_LISTAR_HELP _T("<listar> - Lista todos os placares ativos\n")
#define STR_CMD_ENCERRAR_HELP _T("<encerrar> - Encerra a central\n")
#define STR_CMD_AJUDA_HELP _T("<ajuda> - Mostra comandos disponíveis\n")

#endif