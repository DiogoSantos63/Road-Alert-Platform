#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include "Central.h"
#include "Constantes.h"
#include "Strings.h"
#include "Types.h"
#include "Utils.h"

//TODO: 
// - validar input do comando alerta (nao permitir duracoes negativas)
// - permitir alerta com aspas
// - fix do print dos ids

int _tmain(int argc, TCHAR *argv[]) {
  initConsola();

  CENTRAL central = {0};

  if (!validarArgumentos(&central, argc, argv)) {
    ExitProcess(1);
  }

  if (!initCentral(&central)) {
    ExitProcess(1);
  }

  if (!criarPipeInstances(&central)) {
    ExitProcess(1);
  }

  if (!initThreads(&central)) {
    terminarCentral(&central);
    ExitProcess(1);
  }

  WaitForMultipleObjects(2, central.hThreads, TRUE, INFINITE);

  terminarCentral(&central);
  ExitProcess(0);
}

BOOL initCentral(CENTRAL *central) {
  central->proximoId = 1;
  central->hMutex = CreateMutex(NULL, FALSE, NULL);

  if (central->hMutex == NULL) {
    _tprintf(STR_MUTEX_CREATION_ERROR, GetLastError());
    return FALSE;
  }
  return TRUE;
}

BOOL validarArgumentos(CENTRAL *central, int argc, TCHAR *argv[]) {
  if (argc > 1) {
    size_t tamPrefixo = _tcslen(CENTRAL_PIPE);
    size_t tamNome = _tcslen(argv[1]);

    if (tamPrefixo + tamNome + 1 <= TAM_NOME_PIPE) {
      _tcscpy_s(central->nomePipe, TAM_NOME_PIPE, CENTRAL_PIPE);
      _tcscat_s(central->nomePipe, TAM_NOME_PIPE, argv[1]);
    } else {
      _tprintf(STR_PIPE_NAME_EXCEEDED);
      return FALSE;
    }
  } else {
    _tprintf(STR_PIPE_NAME_NOT_FOUND);
    return FALSE;
  }

  _tprintf(STR_NAMED_PIPE, central->nomePipe);

  return TRUE;
}

BOOL criarPipeInstances(CENTRAL *central) {
  for (int i = 0; i < MAX_PLACARES; i++) {
    central->hEvents[i] = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (central->hEvents[i] == NULL) {
      _tprintf(STR_ERRO_CRIAR_EVENTOS);
      return FALSE;
    }

    memset(&central->placares[i], 0, sizeof(PLACAR_INFO));

    central->placares[i].olpInstance.hEvent = central->hEvents[i];
    central->placares[i].olpInstance.Offset = 0;
    central->placares[i].olpInstance.OffsetHigh = 0;

    central->placares[i].hPipeInstance = CreateNamedPipe(      //
        central->nomePipe,                                     // nome
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,             // flags
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // tipo
        MAX_PLACARES,                                          // instancias
        MAX_BUF*sizeof(TCHAR),                                               // tam buffer output
        MAX_BUF*sizeof(TCHAR),                                               // tam buffer input
        0,                                                     // time-out
        NULL                                                   // atributos seguranca default
        );

    if (central->placares[i].hPipeInstance == INVALID_HANDLE_VALUE) {
      _tprintf(STR_NAMED_PIPE_CREATION);
      return FALSE;
    }

    if (!criaConexao(&central->placares[i])) {
      _tprintf(STR_NAMED_PIPE_CREATION);
      return FALSE;
    }
  }

  return TRUE;
}

// Cria uma conexão, que pode ser utilizada por um placar para se ligar ao sistema
// Cada conexão fica ativa durante todo o programa
// Pode ser reutilizada no caso de um placar se desconectar
// TRUE on success
// FALSE on error
BOOL criaConexao(PLACAR_INFO *placar) {
  if (ConnectNamedPipe(placar->hPipeInstance, &placar->olpInstance) != 0) {
    _tprintf(STR_ERRO_CONNECT_PIPE);
    return FALSE;
  }

  switch (GetLastError()) {
    case ERROR_IO_PENDING:
      // fica à espera que um placar se conecte
      placar->estadoIO = ESPERA_CONEXAO;
      placar->operacaoPendente = TRUE;
      placar->conexaoAtiva = FALSE;
      break;

    case ERROR_PIPE_CONNECTED:
      // placar já conectado, enviar evento
      placar->estadoIO = LE_PEDIDO;
      placar->operacaoPendente = FALSE;
      placar->conexaoAtiva = TRUE;
      SetEvent(placar->olpInstance.hEvent);
      break;

      // erro
    default:
      _tprintf(STR_ERRO_CONNECT_PIPE);
      return FALSE;
  }

  return TRUE;
}

BOOL initThreads(CENTRAL *central) {
  central->hThreads[0] = CreateThread(NULL, 0, threadComandos, central, 0, NULL);
  central->hThreads[1] = CreateThread(NULL, 0, threadPlacaresOverlappedIO, central, 0, NULL);

  if (central->hThreads[0] == NULL || central->hThreads[1] == NULL) {
    CloseHandle(central->hThreads[0]);
    CloseHandle(central->hThreads[1]);
    _tprintf(STR_ERRO_INIT_THREADS);
    return FALSE;
  }

  return TRUE;
}

COMANDO parsearComando(TCHAR *cmd) {
  if (_tcscmp(cmd, STR_CMD_ALERTA) == 0) return CMD_ALERTA;
  if (_tcscmp(cmd, STR_CMD_CANCELAR) == 0) return CMD_CANCELAR;
  if (_tcscmp(cmd, STR_CMD_LISTAR) == 0) return CMD_LISTAR;
  if (_tcscmp(cmd, STR_CMD_ENCERRAR) == 0) return CMD_ENCERRAR;
  if (_tcscmp(cmd, STR_CMD_AJUDA) == 0) return CMD_AJUDA;

  return CMD_DESCONHECIDO;
}

DWORD WINAPI threadComandos(LPVOID param) {
  CENTRAL *central = (CENTRAL *)param;
  TCHAR input[TAM_CMD];

  while (!central->terminar) {

    _tprintf(_T("\n"));
    _tprintf(STR_CMD_INPUT);

    _fgetts(input, TAM_CMD, stdin);

    size_t len = _tcslen(input);
    if (len > 0 && input[len - 1] == _T('\n')) input[len - 1] = _T('\0');

    TCHAR *args = NULL;
    TCHAR *comando = _tcstok_s(input, _T(" "), &args);

    if (!comando) continue;

    switch (parsearComando(comando)) {
      case CMD_ALERTA:
        if (args == NULL) _tprintf(STR_CMD_ALERTA_ERRO);
        cmdEnviarAlerta(central, args);
        break;
      case CMD_CANCELAR:
        if (args == NULL) _tprintf(STR_CMD_CANCELAR_ERRO);
        cmdCancelarAlerta(central, args);
        break;
      case CMD_LISTAR:
        cmdListarPlacares(central);
        break;
      case CMD_ENCERRAR:
        cmdEncerrarCentral(central);
        break;
      case CMD_AJUDA:
        cmdListarComandos();
        break;
      case CMD_DESCONHECIDO:
        _tprintf(STR_ERRO_COMANDO);
        _tprintf(STR_HELP);
        break;
    }
  }
  return 0;
}

void processarLeituraConcluida(CENTRAL *central, PLACAR_INFO *placar) {
  LOCK(central->hMutex);
  BYTE tipo = placar->bufferPedido[0];
  switch(tipo) {
    case LIGAR:
      placar->msg_por_enviar = TRUE;
      placar->tipo_msg_enviar = LIGAR;
      placar->estadoIO = ENVIA_RESPOSTA;
      break;

    case DESLIGAR:
      placar->msg_por_enviar = TRUE;
      placar->tipo_msg_enviar = DESLIGAR;
      placar->estadoIO = ENVIA_RESPOSTA;
      break;

    case FIM_ALERTA:
      LOG("Fim do alerta do placar %d", placar->id);
      ZeroMemory(placar->msgAlerta, sizeof(placar->msgAlerta));
      placar->temAlerta = FALSE;
      placar->msg_por_enviar = FALSE;
      placar->estadoIO = LE_PEDIDO;
      break;

    case NOVO_ALERTA:
      placar->msg_por_enviar = FALSE;
      placar->estadoIO = LE_PEDIDO;
      placar->temAlerta = TRUE;
      break;

    case CANCELAR_ALERTA:
      placar->msg_por_enviar = FALSE;
      placar->estadoIO = LE_PEDIDO;
      placar->temAlerta = FALSE;
      break;
    default:
      placar->msg_por_enviar = FALSE;
      placar->estadoIO = LE_PEDIDO;
  }
  UNLOCK(central->hMutex);
}

void processarEscritaConcluida(CENTRAL *central, PLACAR_INFO *placar) {
  LOG("Mensagem enviada com sucesso");

  LOCK(central->hMutex);
  placar->msg_por_enviar = FALSE;
  placar->estadoIO = LE_PEDIDO;

  switch(placar->tipo_msg_enviar) {
    case LIGAR:
      MSG_ID *resposta = (MSG_ID*)placar->bufferResposta;
      placar->id = resposta->identificador;

      LOG("Placar ligado - id: %d", placar->id);

      placar->ligado = TRUE;
      central->numPlacares++;
      break;

    case ENCERRAR_PLATAFORMA:
    case DESLIGAR:
      LOG("Placar desligado - id: %d", placar->id);
      //FlushFileBuffers(placar->hPipeInstance);

      placar->id = -1;
      placar->ligado = FALSE;
      placar->temAlerta = FALSE;
      central->numPlacares--;
      desconectarPlacar(central, placar);
      break;
  }
  UNLOCK(central->hMutex);
}

DWORD WINAPI threadPlacaresOverlappedIO(LPVOID param) {
  CENTRAL *central = param;

  while (TRUE) {
    // condicao de encerramento
    if (central->terminar) {
      BOOL prontoASair = TRUE;
      for (int k = 0; k < MAX_PLACARES; k++) {
        if (central->placares[k].ligado || central->placares[k].operacaoPendente) {
          prontoASair = FALSE;
          break;
        }
      }
      if (prontoASair) {
        LOG("Todos os placares terminados. A sair da thread de I/O.");
        break;
      }
    } else {
      _tprintf(STR_CMD_INPUT);
    }


    DWORD indexEvento = WaitForMultipleObjects(
        MAX_PLACARES,
        central->hEvents,
        FALSE, // n espera por todos
        INFINITE);

    int i = indexEvento - WAIT_OBJECT_0;
    if (i < 0 || i >= MAX_PLACARES) {
        _tprintf(STR_OUT_OF_RANGE);
      continue;
    }

    ResetEvent(central->hEvents[i]);

    DWORD bytesLidos = 0;
    BOOL res;

    // resultado de operações que ficaram pendentes
    if (central->placares[i].operacaoPendente) {
      res = GetOverlappedResult(
          central->placares[i].hPipeInstance,
          &central->placares[i].olpInstance,
          &bytesLidos,
          FALSE);

      switch (central->placares[i].estadoIO) {
        case ESPERA_CONEXAO:
          //LOG("OPERACAO CONEXAO CONCLUIDA");
          if (!res) {
            if(GetLastError() == ERROR_OPERATION_ABORTED) {
              // encerramento sistema
              central->placares[i].operacaoPendente = FALSE;
              break;
            }

            LOG("Erro desconhecido");
            continue;
          }
          central->placares[i].estadoIO = LE_PEDIDO;
          break;

        case LE_PEDIDO:
          //LOG("OPERACAO LEITURA CONCLUIDA");
          if (!res || bytesLidos == 0) {

            // comando do utilizador para enviar
            if (GetLastError() == ERROR_OPERATION_ABORTED && central->placares[i].msg_por_enviar) {
              central->placares[i].operacaoPendente = FALSE;
              central->placares[i].estadoIO = ENVIA_RESPOSTA;
              break;
            }

            desconectarPlacar(central, &central->placares[i]);
            continue;
          }
          central->placares[i].operacaoPendente = FALSE;
          processarLeituraConcluida(central, &central->placares[i]);
          break;

        case ENVIA_RESPOSTA:
          //LOG("OPERACAO ESCRITA CONCLUIDA");
          if (!res) {
            desconectarPlacar(central, &central->placares[i]);
            continue;
          }
          central->placares[i].operacaoPendente = FALSE;
          processarEscritaConcluida(central, &central->placares[i]);
          break;
      }
    }

    // realiza acao conforme o estado atual
    switch (central->placares[i].estadoIO) {
      case LE_PEDIDO:
        if(!handleMsgPlacarReceber(central, &central->placares[i])) {
          desconectarPlacar(central, &central->placares[i]);
        }
        continue;

      case ENVIA_RESPOSTA:
        if(!handleMsgPlacarEnviar(central, &central->placares[i])) {
          desconectarPlacar(central, &central->placares[i]);
        }
        continue;

      default:
        continue;
    }
  }

  return 0;
}

BOOL handleMsgPlacarReceber(CENTRAL *central, PLACAR_INFO *placar) {
  DWORD bytes_lidos;

  // limpar o overlap antes de voltar a usar
  HANDLE tempEvent = placar->olpInstance.hEvent;
  memset(&placar->olpInstance, 0, sizeof(OVERLAPPED));
  placar->olpInstance.hEvent = tempEvent;

  BOOL res = ReadFile(
      placar->hPipeInstance,
      placar->bufferPedido,
      MAX_BUF * sizeof(TCHAR),
      &bytes_lidos,
      &placar->olpInstance);

  if (res && bytes_lidos != 0) {
    placar->operacaoPendente = FALSE;
    processarLeituraConcluida(central, placar);
    return TRUE;
  }

  if (!res && (GetLastError() == ERROR_IO_PENDING)) {
    placar->operacaoPendente = TRUE;
    return TRUE;
  }

  return FALSE;
}

BOOL handleMsgPlacarEnviar(CENTRAL *central, PLACAR_INFO *placar) {
  LOCK(central->hMutex);
  if(!placar->msg_por_enviar) {
    placar->operacaoPendente = FALSE;
    placar->estadoIO = LE_PEDIDO;
    UNLOCK(central->hMutex);
    return TRUE;
  }

  switch(placar->tipo_msg_enviar) {
    case LIGAR: {
      LOG("A enviar identificador");
      MSG_ID resposta = {
        .tipo = ATRIBUIR_IDENTIFICADOR,
        .identificador = central->proximoId++
      };
      memcpy(placar->bufferResposta, &resposta, sizeof(resposta));
      placar->bytesResposta = sizeof(resposta);
      break;
    }
    case DESLIGAR: {
      LOG("A enviar msg desligar");
      memcpy(placar->bufferResposta, placar->bufferPedido, sizeof(MSG_CMD));
      placar->bytesResposta = sizeof(MSG_CMD);
      break;
    }
    case NOVO_ALERTA: {
      LOG("A enviar alerta");
      MSG_ALERTA alerta = { .tipo = NOVO_ALERTA, .duracao = placar->duracaoAlerta };
      _tcscpy_s(alerta.msg, TAM_MSG, placar->msgAlerta);
      memcpy(placar->bufferResposta, &alerta, sizeof(MSG_ALERTA));
      placar->bytesResposta = sizeof(MSG_ALERTA);
      break;
    }
    case CANCELAR_ALERTA: {
      LOG("A cancelar alerta");
      MSG_CMD cmd = { .tipo = CANCELAR_ALERTA };
      memcpy(placar->bufferResposta, &cmd, sizeof(MSG_CMD));
      placar->bytesResposta = sizeof(MSG_CMD);
      break;
    }
    case ENCERRAR_PLATAFORMA: {
      LOG("A enviar comando encerrar");
      MSG_CMD cmd = { .tipo = ENCERRAR_PLATAFORMA };
      memcpy(placar->bufferResposta, &cmd, sizeof(MSG_CMD));
      placar->bytesResposta = sizeof(MSG_CMD);
      break;
    }
    default:
      UNLOCK(central->hMutex);
      return TRUE;
  }

  UNLOCK(central->hMutex);

  DWORD bytes_escritos = 0;

  // limpar o overlap antes de voltar a usar
  HANDLE tempEvent = placar->olpInstance.hEvent;
  memset(&placar->olpInstance, 0, sizeof(OVERLAPPED));
  placar->olpInstance.hEvent = tempEvent;

  BOOL res = WriteFile(
      placar->hPipeInstance,
      placar->bufferResposta,
      placar->bytesResposta,
      &bytes_escritos,
      &placar->olpInstance);

  if (res && bytes_escritos == placar->bytesResposta) {
    placar->operacaoPendente = FALSE;
    processarEscritaConcluida(central, placar);
    return TRUE;
  }

  if (!res && (GetLastError() == ERROR_IO_PENDING)) {
    placar->operacaoPendente = TRUE;
    placar->msg_por_enviar = TRUE;
    return TRUE;
  }

  return FALSE;
}

void desconectarPlacar(CENTRAL *central, PLACAR_INFO *placar) {
  DisconnectNamedPipe(placar->hPipeInstance);

  if (placar->ligado) {
    placar->ligado = FALSE;
    central->numPlacares--;
    placar->id = -1;
    placar->temAlerta = FALSE;
    placar->operacaoPendente = FALSE;
  }

  // reutiliza instancia do pipe para nova conexao
  // a nao ser que esteja a terminar
  if (!central->terminar) {
    criaConexao(placar);
  }
}

BOOL cmdEnviarAlerta(CENTRAL *central, TCHAR *args) {
  TCHAR *ctx = NULL;
  TCHAR *msg = _tcstok_s(args, _T(" "), &ctx);
  TCHAR *sDuracao = _tcstok_s(NULL, _T(" "), &ctx);
  TCHAR *sId = _tcstok_s(NULL, _T(" "), &ctx);

  if (!msg || !sDuracao || !sId) {
    _tprintf(STR_CMD_ALERTA_ERRO);
    return FALSE;
  }

  DWORD duracao = (DWORD)_ttoi(sDuracao);
  DWORD id = (DWORD)_ttoi(sId);

  MSG_ALERTA alerta = {0};
  alerta.tipo = NOVO_ALERTA;
  alerta.duracao = duracao;
  _tcscpy_s(alerta.msg, TAM_MSG, msg);

  WaitForSingleObject(central->hMutex, INFINITE);

  for (int i = 0; i < MAX_PLACARES; i++) {
    if ((id == 0 || central->placares[i].id == id) && central->placares[i].ligado) {
      central->placares[i].temAlerta = TRUE;
      central->placares[i].duracaoAlerta = duracao;
      _tcscpy_s(central->placares[i].msgAlerta, TAM_MSG, msg);


      // set dados da mensagem e acordar o WaitForMultipleObjects
      central->placares[i].msg_por_enviar = TRUE;
      central->placares[i].tipo_msg_enviar = NOVO_ALERTA;
      CancelIoEx(central->placares[i].hPipeInstance, &central->placares[i].olpInstance);
    }
  }
  ReleaseMutex(central->hMutex);

  return TRUE;
}

BOOL cmdCancelarAlerta(CENTRAL *central, TCHAR *args) {
  if (args == NULL) {
    _tprintf(STR_CMD_CANCEL_ALERTA_ERRO);
    return FALSE;
  }

  DWORD id = (DWORD)_ttoi(args);

  if (id == 0) {
    _tprintf(STR_CMD_CANCEL_ALERTA_ERRO);
    return FALSE;
  }

  WaitForSingleObject(central->hMutex, INFINITE);
  for (int i = 0; i < MAX_PLACARES; i++) {
    if ((id == 0 || central->placares[i].id == id) && central->placares[i].ligado) {
      central->placares[i].temAlerta = FALSE;
      ZeroMemory(central->placares[i].msgAlerta, sizeof(central->placares[i].msgAlerta));

      // set dados da mensagem e acordar o WaitForMultipleObjects
      central->placares[i].msg_por_enviar = TRUE;
      central->placares[i].tipo_msg_enviar = CANCELAR_ALERTA;
      CancelIoEx(central->placares[i].hPipeInstance, &central->placares[i].olpInstance);
    }
  }
  ReleaseMutex(central->hMutex);

  _tprintf(STR_ALERTA_CANCELADO, id);
  return TRUE;
}

BOOL cmdListarPlacares(CENTRAL *central) {
  WaitForSingleObject(central->hMutex, INFINITE);

  _tprintf(STR_PLACARES_ATIVOS, central->numPlacares);

  for (int i = 0; i < MAX_PLACARES; i++) {
    if (central->placares[i].ligado) {
      _tprintf(STR_INFO_PLACAR, central->placares[i].id,
          central->placares[i].temAlerta ? central->placares[i].msgAlerta : STR_EMPTY);
    }
  }
  ReleaseMutex(central->hMutex);

  return TRUE;
}

BOOL cmdListarComandos() {
  _tprintf(STR_CENTRAL);
  _tprintf(STR_CMD_COMANDOS);
  _tprintf(STR_CMD_ALERTA_HELP);
  _tprintf(STR_CMD_CANCELAR_HELP);
  _tprintf(STR_CMD_LISTAR_HELP);
  _tprintf(STR_CMD_ENCERRAR_HELP);
  _tprintf(STR_CMD_AJUDA_HELP);

  return TRUE;
}

BOOL cmdEncerrarCentral(CENTRAL *central) {
  MSG_CMD msg = {.tipo = ENCERRAR_PLATAFORMA};

  _tprintf(STR_CENTRAL_ENCERRAR);

  WaitForSingleObject(central->hMutex, INFINITE);

  central->terminar = TRUE;

  for (int i = 0; i < MAX_PLACARES; i++) {
    if (central->placares[i].ligado) {
      central->placares[i].msg_por_enviar = TRUE;
      central->placares[i].tipo_msg_enviar = ENCERRAR_PLATAFORMA;
    }

    // Acordar instancias do pipe
    CancelIoEx(central->placares[i].hPipeInstance, &central->placares[i].olpInstance);
  }
  ReleaseMutex(central->hMutex);

  return TRUE;
}

void processarFimAlerta(CENTRAL *central, int idx) {
  WaitForSingleObject(central->hMutex, INFINITE);
  central->placares[idx].temAlerta = FALSE;
  ZeroMemory(central->placares[idx].msgAlerta, sizeof(central->placares[idx].msgAlerta));
  ReleaseMutex(central->hMutex);

  _tprintf(STR_PLACAR_FIM_ALERTA, central->placares[idx].id);
}

void processarConfirmacaoAlerta(CENTRAL *central, int idx, HANDLE hPipe) {
  MSG_ALERTA confirmacao = {0};
  DWORD lido;

  ReadFile(hPipe, (LPBYTE)&confirmacao + sizeof(BYTE), sizeof(MSG_ALERTA) - sizeof(BYTE), &lido, NULL);

  _tprintf(STR_ALERTA_CONFIRMADO, central->placares[idx].id);
}

void processarConfirmacaoCancelar(CENTRAL *central, int idx, HANDLE hPipe) {
  MSG_CMD confirmacao = {0};
  DWORD lido;

  ReadFile(hPipe, (LPBYTE)&confirmacao + sizeof(BYTE), sizeof(MSG_CMD) - sizeof(BYTE), &lido, NULL);

  _tprintf(STR_CANCELAR_CONFIRMADO, central->placares[idx].id);
}

void terminarCentral(CENTRAL *central) {
  if (central->hThreads[0]) CloseHandle(central->hThreads[0]);
  if (central->hThreads[1]) CloseHandle(central->hThreads[1]);
  if (central->hMutex) CloseHandle(central->hMutex);

  for (int i = 0; i < MAX_PLACARES; i++) {
    if (central->placares[i].hPipeInstance) {
      DisconnectNamedPipe(central->placares[i].hPipeInstance);
      CloseHandle(central->placares[i].hPipeInstance);
    }

    if (central->hEvents[i]) {
      CloseHandle(central->hEvents[i]);
    }
  }
}
