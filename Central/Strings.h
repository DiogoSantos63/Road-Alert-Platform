#ifndef STRINGS_H
#define STRINGS_H

#include <tchar.h>

// Info
#define STR_EMPTY _T("---")
#define STR_CENTRAL _T("\n[CENTRAL] ")
#define STR_HELP _T("[CENTRAL] Escreva <ajuda> para listar os comandos disponiveis\n")
#define STR_CMD_INPUT _T("\n> ")
#define STR_NAMED_PIPE _T("Named Pipe = '%s'\n")
#define STR_COPIA_NAMED_PIPE _T("[DEBUG] Criar uma copia do pipe '%s'...")
#define STR_INFO_PLACAR _T("Placar %d - '%s' (%d segundos)\n")
#define STR_WAITING_CONNECTIONS_AND_REQUESTS _T("\nA aguardar conexao ou pedido...\n")
#define STR_CENTRAL_PLACAR_LIGADO _T("\n[DEBUG] Ligou-se um placar...\n")
#define STR_CENTRAL_ENVIOU_ALERTA _T("\n[DEBUG] Enviei %d bytes ao placar %d... MSG_ALERTA (tipo = %d)")
#define STR_CENTRAL_RECEBEU_ALERTA _T("\n[DEBUG] Recebi %d bytes do placar %d... MSG_ALERTA (tipo = %d)")
#define STR_CENTRAL_ENVIOU_CMD _T("\n[DEBUG] Enviei %d bytes ao placar %d... MSG_CMD (tipo = %d)")
#define STR_CENTRAL_RECEBEU_CMD _T("\n[DEBUG] Recebi %d bytes do placar %d... MSG_CMD (tipo = %d)")
#define STR_CENTRAL_PLACAR_IDENTIFICADOR _T("\n[DEBUG] Atribui o identificador %d...")
#define STR_CENTRAL_PLACAR_ALERTA_ATIVO _T("\n[DEBUG] Placar %d com alerta ativo...")
#define STR_CENTRAL_PLACAR_TERMINOU _T("\n[DEBUG] A duracao do alerta do placar %d terminou...")
#define STR_CENTRAL_PLACAR_ALERTA_CANCELADO _T("\n[DEBUG] Cancelado alerta do placar %d...")
#define STR_CENTRAL_PLACAR_REMOVIDO _T("\n[DEBUG] Desliguei/removi o placar %d...")
#define STR_CENTRAL_ENCERRAR _T("\n[DEBUG] A encerrar plataforma...")

// Erros
#define STR_ERRO_CRIAR_EVENTOS _T("[ERRO] Criar eventos.\n")
#define STR_OUT_OF_RANGE _T("[ERRO] Index do placar acima do limite!\n")
#define STR_PIPE_NAME_NOT_FOUND _T("[ERRO] O nome do PIPE nao foi especificado!\n")
#define STR_PIPE_NAME_EXCEEDED _T("[ERRO] O nome do PIPE excedeu o limite!\n")
#define STR_ERRO_INIT_THREADS _T("[ERRO] Erro ao criar threads\n")
#define STR_ERRO_COMANDO _T("[ERRO] Comando desconhecido!\n")
#define STR_MUTEX_CREATION_ERROR _T("[ERRO] Erro ao criar o Mutex: %lu\n")
#define STR_NAMED_PIPE_CREATION _T("[ERRO] Erro ao criar o NamedPipe\n")
#define STR_CMD_ALERTA_ERRO _T("[ERRO] Uso: alerta <msg> <duracao> [id]\n")
#define STR_CMD_CANCEL_ALERTA_ERRO _T("[ERRO] Uso: cancelar [id_placar]\n")
#define STR_CMD_CANCELAR_ERRO _T("[ERRO] Uso: cancelar <msg> <duracao> [id]\n")
#define STR_ERRO_CONNECT_PIPE _T("[ERRO] Nao foi possivel ligar ao PIPE\n")
#define STR_ERRO_DURACAO _T("[ERRO] A duracao nao pode ser negativa\n")
#define STR_ERROR_MEMORIA_PARTILHADA _T("\n[ERRO] Erro ao criar memoria partilhada\n")
#define STR_ERROR_MAPEAR_MEMORIA _T("\n[ERRO] Erro ao mapear memoria partilhada\n")
#define STR_ERROR_EVENTO_MONITOR _T("\n[ERRO] Erro ao criar evento monitor\n")

// Comandos
#define STR_CMD_COMANDOS _T("Comandos disponiveis: \n\n")
#define STR_CMD_ALERTA _T("alerta")
#define STR_CMD_CANCELAR _T("cancelar")
#define STR_CMD_LISTAR _T("listar")
#define STR_CMD_ENCERRAR _T("encerrar")
#define STR_CMD_AJUDA _T("ajuda")
#define STR_CMD_ALERTA_HELP _T("<alerta> - Envia um alerta\n")
#define STR_CMD_CANCELAR_HELP _T("<cancelar> - Cancela um alerta\n")
#define STR_CMD_LISTAR_HELP _T("<listar> - Lista todos os placares ativos\n")
#define STR_CMD_ENCERRAR_HELP _T("<encerrar> - Encerra a central\n")
#define STR_CMD_AJUDA_HELP _T("<ajuda> - Mostra comandos disponiveis\n")

#endif