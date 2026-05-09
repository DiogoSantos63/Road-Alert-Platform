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

// TODO:
// - Adaptar reads e writes para receber/enviar respostas corretas consoante o tipo de pedido
// - Mutex para as instancias dos placares
// - Trocar tipos hardcoded por valores enum
// - Limpar comentarios

int _tmain(int argc, TCHAR *argv[]) {
  initConsola();

  CENTRAL central = {0};

  //TODO: remove this later
  char* fake_argv[] = { "Placar.exe", _T("tubo") };
  argc = 2;
  argv = fake_argv;

  if (!validarArgumentos(&central, argc, argv)) {
    ExitProcess(1);
  }

  if (!initCentral(&central)) {
    ExitProcess(1);
  }

  // if (!criarPipe(&central)) {
  //   ExitProcess(1);
  // }

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

BOOL criarPipe(CENTRAL *central) {
  return FALSE;

  // central->hPipe =
  //     CreateNamedPipe(central->nomePipe, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
  //                     MAX_PLACARES, 1024, 1024, 0, NULL);
  //
  // if (central->hPipe == INVALID_HANDLE_VALUE) {
  //   _tprintf(STR_NAMED_PIPE_CREATION, GetLastError());
  //   return FALSE;
  // }
  //
  // _tprintf(STR_CENTRAL_ACTIVE);
  //
  // return TRUE;
}

BOOL initThreads(CENTRAL *central) {
  LOG("INIT THREADS");
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

DWORD WINAPI threadPlacaresOverlappedIO(LPVOID param) {
  CENTRAL *central = param;

  while (!central->terminar) {
    _tprintf(STR_WAITING_CONNECTIONS_AND_REQUESTS);
    _tprintf(STR_CMD_INPUT);

    DWORD indexEvento = WaitForMultipleObjects( //
        MAX_PLACARES,                           // numero de eventos
        central->hEvents,                       // array de event handles
        FALSE,                                  // nao espera por todos
        INFINITE);                              // infinito

    LOG("novo evento");

    int i = indexEvento - WAIT_OBJECT_0;
    if (i < 0 || i >= MAX_PLACARES) {
      _tprintf(STR_OUT_OF_RANGE);
      continue; // TODO: handle erro?
    }

    DWORD bytesLidos = 0;
    BOOL res;

    // resultado de operações que ficaram pendentes
    if (central->placares[i].operacaoPendente) {
      res = GetOverlappedResult(              //
          central->placares[i].hPipeInstance, // hFile
          &central->placares[i].olpInstance,  // *overlapped
          &bytesLidos,                        // bytes transferidos
          FALSE);                             // nao esperar

      switch (central->placares[i].estadoIO) {
        case ESPERA_CONEXAO:
          LOG("OPERACAO CONEXAO");
          if (!res) {
            _tprintf(STR_OUT_OF_RANGE);
            continue; // TODO: handle erro?
          }

          central->placares[i].estadoIO = LE_PEDIDO;
          break;

        case LE_PEDIDO:
          LOG("OPERACAO LEITURA");
          // desconexao ou erro
          if (!res) {
            desconectarPlacar(&central->placares[i]);
            continue;
          }

          central->placares[i].estadoIO = ENVIA_RESPOSTA;
          break;

        case ENVIA_RESPOSTA:
          LOG("OPERACAO ESCRITA");
          // desconexao ou erro
          if (!res) {
            desconectarPlacar(&central->placares[i]);
            continue;
          }

          central->placares[i].estadoIO = LE_PEDIDO;
          break;
      }
    }

    // realiza acao conforme o estado atual
    switch (central->placares[i].estadoIO) {
      // ler request
      case LE_PEDIDO:
        // TODO: lock mutex aqui?
        if(!handleMsgPlacarReceber(central, &central->placares[i])) {
          desconectarPlacar(&central->placares[i]);
          break;
        }
        continue;

      // enviar msg ao placar (pode ser resposta a pedido ou cmd do utilizador)
      case ENVIA_RESPOSTA:
        // error
        if(!handleMsgPlacarEnviar(central, &central->placares[i])) {
          desconectarPlacar(&central->placares[i]);
          break;
        }
        continue;
    }
  }

  return 0;
}

// retorna falso se houver erro de leitura
BOOL handleMsgPlacarReceber(CENTRAL *central, PLACAR_INFO *placar) {
  DWORD bytes_lidos;

  BOOL res = ReadFile(                    //
      placar->hPipeInstance, // instancia pipe
      placar->bufferPedido,  // buffer onde recebemos o pedido
      MAX_BUF * sizeof(TCHAR),            // max size do pedido
      &bytes_lidos,                       // numero de bytes lidos
      &placar->olpInstance); // estrutura overlapped

  if (res && bytes_lidos != 0) {
    // read completou
    placar->operacaoPendente = FALSE;

    BYTE tipo = placar->bufferPedido[0];
    switch(tipo) {
      case LIGAR:
        LOG("placar conectado");
        placar->msg_por_enviar = TRUE;
        placar->tipo_msg_enviar = LIGAR;

        placar->estadoIO = ENVIA_RESPOSTA;
        break;

      case DESLIGAR:
        LOG("placar %d desligado", placar->id);
        placar->msg_por_enviar = TRUE;
        placar->tipo_msg_enviar = DESLIGAR;

        placar->estadoIO = ENVIA_RESPOSTA;
        break;

      case FIM_ALERTA:
        LOG("Fim do alerta do placar %d", placar->id);
        placar->temAlerta = FALSE;
        placar->msg_por_enviar = FALSE;
        placar->estadoIO = LE_PEDIDO;
        break;

      case NOVO_ALERTA:
        LOG("Confirmacao alerta recebido, placar %d", placar->id);
        placar->msg_por_enviar = FALSE;
        placar->estadoIO = LE_PEDIDO;
        break;

      case CANCELAR_ALERTA:
        LOG("Confirmacao cancelamento alerta recebido, placar %d", placar->id);
        placar->msg_por_enviar = FALSE;
        placar->temAlerta = FALSE;
        placar->estadoIO = LE_PEDIDO;
        break;

      default:
        LOG("Tipo de mensagem desconhecido...");
        placar->msg_por_enviar = FALSE;
        placar->estadoIO = LE_PEDIDO;
    }

    return TRUE;
  }

  // operacao read em espera
  if (!res && (GetLastError() == ERROR_IO_PENDING)) {
    placar->operacaoPendente = TRUE;
    return TRUE;
  }

  // se chegámos aqui, houve um erro
  return FALSE;
}

// retorna falso se houve erro de escrita
BOOL handleMsgPlacarEnviar(CENTRAL *central, PLACAR_INFO *placar) {
  if(!placar->msg_por_enviar) {
    placar->operacaoPendente = FALSE;
    placar->estadoIO = LE_PEDIDO;
    return TRUE;
  }

  // atualiza dados para enviar ao placar
  // nota: talvez faça mais sentido fazer isto logo que recebemos a mensagem?
  switch(placar->tipo_msg_enviar) {
    case LIGAR:
      MSG_ID resposta = {
        .tipo = ATRIBUIR_IDENTIFICADOR,
        .identificador = central->proximoId++
      };
      memcpy(placar->bufferResposta, &resposta, sizeof(resposta));
      placar->bytesResposta = sizeof(resposta);
      break;

    case DESLIGAR:
      // envia msg de confirmaçao de volta
      memcpy(placar->bufferResposta, &placar->bufferPedido, sizeof(MSG_CMD));
      placar->bytesResposta = sizeof(MSG_CMD);
      break;

    default:
      return TRUE;
  }


  LOG("A Enviar resposta...");

  DWORD bytes_escritos = 0;
  BOOL res = WriteFile(                    //
      placar->hPipeInstance,  //
      placar->bufferResposta, //
      placar->bytesResposta,  //
      &bytes_escritos,                     //
      &placar->olpInstance);   //


  // escrita completou
  if (res && bytes_escritos == placar->bytesResposta) {
    LOG("Reposta enviada com sucesso");
    placar->msg_por_enviar = FALSE;
    placar->operacaoPendente = FALSE;
    placar->estadoIO = LE_PEDIDO;

    // atualizar dados internoss
    switch(placar->tipo_msg_enviar) {
      case LIGAR:
        MSG_ID *resposta = (MSG_ID*)placar->bufferResposta;
        placar->id = resposta->identificador;
        placar->ligado = TRUE;
        break;
      case DESLIGAR:
        // libertar instancia do pipe para uma nova conexão
        placar->id = -1;
        placar->ligado = FALSE;
        desconectarPlacar(placar);
        break;
    }

    return TRUE;
  }

  // msg por enviar
  if (!res && (GetLastError() == ERROR_IO_PENDING)) {
    placar->operacaoPendente = TRUE;
    placar->msg_por_enviar = TRUE;
    return TRUE;
  }

  // erro se chegarmos aqui
  return FALSE;
}

void desconectarPlacar(PLACAR_INFO *placar) {
  DisconnectNamedPipe(placar->hPipeInstance);

  // reutiliza instancia do pipe para nova conexao
  criaConexao(placar);
}


// void updateResposta(CENTRAL *central, PLACAR_INFO *placar) {
  // BYTE tipo = placar->bufferPedido[0];
  // switch(tipo) {
  //   case LIGAR: 
  //
  //   case DESLIGAR: 
  //
  //   case FIM_ALERTA:
  //     LOG("FIM ALERTA placar %d", placar->id);
  //     // nao precisamos de enviar nada
  //     break;
  //
  //   default:
  //     LOG("Tipo de pedido desconhecido");
  //
  // }

  // _tcscpy_s(placar->bufferResposta, MAX_BUF, _T("RESPOSTA"));
  // placar->bytesResposta = _tcslen(placar->bufferResposta); // tamanho da resposta a enviar
// }

DWORD WINAPI threadPlacares(LPVOID param) {
  // CENTRAL *central = (CENTRAL *)param;
  //
  // while (!central->terminar) {
  //   _tprintf(STR_WAITING_CONNECTION);
  //   _tprintf(STR_CMD_INPUT);
  //
  //   BOOL ligado = ConnectNamedPipe(central->hPipe, NULL);
  //
  //   if (!ligado && GetLastError() != ERROR_PIPE_CONNECTED) {
  //     _tprintf(STR_ERRO_CONNECT_PIPE, GetLastError());
  //     continue;
  //   }
  //
  //   WaitForSingleObject(central->hMutex, INFINITE);
  //
  //   if (central->numPlacares >= MAX_PLACARES) {
  //     ReleaseMutex(central->hMutex);
  //     _tprintf(STR_ERRO_MAX_PLACARES);
  //     DisconnectNamedPipe(central->hPipe);
  //     continue;
  //   }
  //
  //   int index = central->numPlacares;
  //   central->placares[index].hPipe = central->hPipe;
  //   central->placares[index].ligado = TRUE;
  //   central->placares[index].id = 0;
  //   central->numPlacares++;
  //
  //   ReleaseMutex(central->hMutex);
  //
  //   THREAD_PLACAR_ARGS *args = malloc(sizeof(THREAD_PLACAR_ARGS));
  //   args->central = central;
  //   args->index = index;
  //
  //   central->placares[index].hThread = CreateThread(NULL, 0, threadPlacar, args, 0, NULL);
  //
  //   central->hPipe =
  //       CreateNamedPipe(central->nomePipe, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
  //                       MAX_PLACARES, 1024, 1024, 0, NULL);
  //
  //   if (central->hPipe == INVALID_HANDLE_VALUE) {
  //     _tprintf(STR_NAMED_PIPE_CREATION, GetLastError());
  //     break;
  //   }
  // }
  //
  // return 0;
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
  alerta.tipo = 4;
  alerta.duracao = duracao;
  _tcscpy_s(alerta.msg, TAM_MSG, msg);

  if (id == 0)
    enviarTodos(central, &alerta, sizeof(MSG_ALERTA));
  else if (!enviarPlacar(central, id, &alerta, sizeof(MSG_ALERTA))) {
    _tprintf(STR_PLACAR_NAO_ENCONTRADO, id);
    return FALSE;
  }


  WaitForSingleObject(central->hMutex, INFINITE);

  for (int i = 0; i < central->numPlacares; i++) {
    if ((id == 0 || central->placares[i].id == id) && central->placares[i].ligado) {
      central->placares[i].temAlerta = TRUE;
      central->placares[i].duracaoAlerta = duracao;
      _tcscpy_s(central->placares[i].msgAlerta, TAM_MSG, msg);

      // TODO: 
      // - lancar evento para o placar
      // - meter operacao pendente a falso
      // - placar->msg_a_enviar = TRUE;
      // - placar->tipo_msg_enviar = ALERTA;
      // - placar->alerta_a_enviar = .{...};
    }
  }
  ReleaseMutex(central->hMutex);

  return TRUE;
}

BOOL enviarPlacar(CENTRAL *central, int id, void *msg, int size) {
  return FALSE;
  //
  // WaitForSingleObject(central->hMutex, INFINITE);
  //
  // for (int i = 0; i < central->numPlacares; i++) {
  //
  //   if (central->placares[i].id == id && central->placares[i].ligado) {
  //
  //     DWORD escrito;
  //     WriteFile(central->placares[i].hPipe, msg, size, &escrito, NULL);
  //
  //     ReleaseMutex(central->hMutex);
  //     return TRUE;
  //   }
  // }
  //
  // ReleaseMutex(central->hMutex);
  // return FALSE;
}

void enviarTodos(CENTRAL *central, void *msg, int size) {

  // WaitForSingleObject(central->hMutex, INFINITE);
  //
  // for (int i = 0; i < central->numPlacares; i++) {
  //
  //   if (central->placares[i].ligado) {
  //
  //     DWORD escrito;
  //     WriteFile(central->placares[i].hPipe, msg, size, &escrito, NULL);
  //   }
  // }
  //
  // ReleaseMutex(central->hMutex);
}

BOOL cmdCancelarAlerta(CENTRAL *central, TCHAR *args) {
  if (args == NULL) {
    _tprintf(STR_CMD_CANCELAR_ERRO);
    return FALSE;
  }

  DWORD id = (DWORD)_ttoi(args);

  MSG_CMD msg = {0};
  msg.tipo = 5;

  if (id == 0) {
    _tprintf(STR_CMD_CANCELAR_ERRO);
    return FALSE;
  }

  if (!enviarPlacar(central, id, &msg, sizeof(MSG_CMD))) {
    _tprintf(STR_PLACAR_NAO_ENCONTRADO, id);
    return FALSE;
  }

  WaitForSingleObject(central->hMutex, INFINITE);
  for (int i = 0; i < central->numPlacares; i++) {
    if (central->placares[i].id == id && central->placares[i].ligado) {
      central->placares[i].temAlerta = FALSE;
      ZeroMemory(central->placares[i].msgAlerta, sizeof(central->placares[i].msgAlerta));
    }
  }
  ReleaseMutex(central->hMutex);

  _tprintf(STR_ALERTA_CANCELADO, id);
  return TRUE;
}

BOOL cmdListarPlacares(CENTRAL *central) {
  WaitForSingleObject(central->hMutex, INFINITE);

  _tprintf(STR_PLACARES_ATIVOS, central->numPlacares);

  for (int i = 0; i < central->numPlacares; i++) {
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
  MSG_CMD msg = {0};
  msg.tipo = 6;
  enviarTodos(central, &msg, sizeof(MSG_CMD));

  _tprintf(STR_CENTRAL_ENCERRAR);

  // TODO
  //
  // WaitForSingleObject(central->hMutex, INFINITE);
  // for (int i = 0; i < central->numPlacares; i++) {
  //   if (central->placares[i].hThread) {
  //     WaitForSingleObject(central->placares[i].hThread, 5000);
  //     CloseHandle(central->placares[i].hThread);
  //     central->placares[i].hThread = NULL;
  //   }
  // }
  // ReleaseMutex(central->hMutex);

  terminarCentral(central);
  ExitProcess(0);

  return TRUE;
}

DWORD WINAPI threadPlacar(LPVOID param) {
  // THREAD_PLACAR_ARGS *args = (THREAD_PLACAR_ARGS *)param;
  // CENTRAL *central = args->central;
  // int idx = args->index;
  // free(args);
  //
  // HANDLE hPipe = central->placares[idx].hPipe;
  // BYTE tipo;
  // DWORD lido;
  // BOOL ativo = TRUE;
  //
  // while (!central->terminar && ativo) {
  //
  //   BOOL ok = ReadFile(hPipe, &tipo, sizeof(BYTE), &lido, NULL);
  //
  //   if (!ok || lido == 0) {
  //     _tprintf(STR_PLACAR_DESCONECTADO, central->placares[idx].id);
  //     break;
  //   }
  //
  //   switch (tipo) {
  //   case 1:
  //     processarLigar(central, idx);
  //     break;
  //   case 2:
  //     processarDesligar(central, idx, &ativo);
  //     break;
  //   case 3:
  //     processarFimAlerta(central, idx);
  //     break;
  //   case 4:
  //     processarConfirmacaoAlerta(central, idx, hPipe);
  //     break;
  //   case 5:
  //     processarConfirmacaoCancelar(central, idx, hPipe);
  //     break;
  //   default:
  //     _tprintf(STR_ERRO_MSG, tipo);
  //     break;
  //   }
  // }
  //
  // DisconnectNamedPipe(hPipe);
  // CloseHandle(hPipe);
  // return 0;
}
void processarLigar(CENTRAL *central, int idx) {

  // WaitForSingleObject(central->hMutex, INFINITE);
  // central->placares[idx].id = central->proximoId++;
  // central->placares[idx].ligado = TRUE;
  // DWORD id = central->placares[idx].id;
  // ReleaseMutex(central->hMutex);
  //
  // MSG_ID resposta = {0};
  // resposta.tipo = 7;
  // resposta.identificador = id;
  //
  // DWORD escrito;
  // WriteFile(central->placares[idx].hPipe, &resposta, sizeof(MSG_ID), &escrito, NULL);
  //
  // _tprintf(STR_PLACAR_LIGADO, id);
}

void processarDesligar(CENTRAL *central, int idx, BOOL *ativo) {
  // MSG_CMD confirmacao = {0};
  // confirmacao.tipo = 2;
  //
  // DWORD escrito;
  // WriteFile(central->placares[idx].hPipe, &confirmacao, sizeof(MSG_CMD), &escrito, NULL);
  //
  // WaitForSingleObject(central->hMutex, INFINITE);
  // central->placares[idx].ligado = FALSE;
  // central->placares[idx].temAlerta = FALSE;
  // ReleaseMutex(central->hMutex);
  //
  // _tprintf(STR_PLACAR_DESLIGADO, central->placares[idx].id);
  // *ativo = FALSE;
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
  // if (central->hPipe) CloseHandle(central->hPipe);

  // TODO
  // for (int i = 0; i < central->numPlacares; i++) {
  //   if (central->placares[i].hPipe) CloseHandle(central->placares[i].hPipe);
  // }
}
