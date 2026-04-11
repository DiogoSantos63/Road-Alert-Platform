#ifndef strings_h
#define strings_h

#include <tchar.h> 

// Info
#define STR_CMD_INPUT _T("CMD> ")
#define STR_NAMED_PIPE _T("Named Pipe = '%s'\n")
#define STR_PLACAR_ID _T("Identificador = %lu\n")
#define STR_DATA_HORA _T("\n%02d/%02d/%04d (%02d:%02d:%02d): ")
#define STR_ALERTA _T("'%s' (%d segundos)\n")
#define STR_ALERTA_FIM _T("'---'\n")

// Avisos 
#define STR_PLACAR_LIGADO _T("Placar já ligado.\n")
#define STR_PLACAR_ENCERRAR _T("A terminar...\n\n")

// Erros
#define STR_REGISTRY_SAVE_ERROR _T("[ERRO] Não foi possível escrever no Registry.\n")
#define STR_REGISTRY_PIPE_NAME_NOT_FOUND _T("[ERRO] O nome do 'NPIPE' não foi especificado (args ou registry)!\n")
#define STR_COMANDO_DESCONHECIDO _T("[ERRO] Comando desconhecido!\n")
#define STR_ERRO_CRIAR_TIMER _T("[ERRO] Criar o waitable timer.\n")
#define STR_ERRO_ALERTA_REGISTRY _T("[ERRO] Ler alerta do registry!\n")
#define STR_ERRO_CRIAR_THREADS _T("[ERRO] Criar threads.\n")
#define STR_ERRO_CRIAR_EVENTOS _T("[ERRO] Criar eventos.\n")

// Comandos
#define STR_CMD_LIGAR _T("liga")
#define STR_CMD_DESLIGAR _T("desliga")


#endif 
