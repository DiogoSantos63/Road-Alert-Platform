#include <windows.h> 
#include <tchar.h> 
#include <io.h> 
#include <stdio.h> 
#include <fcntl.h> 

#include "Central.h"
#include "Strings.h"
#include "Utils.h"
#include "Types.h"
#include "Constantes.h"

int _tmain(int argc, TCHAR* argv[]) {
	initConsola();

	CENTRAL central = { 0 };

	if (!validarArgumentos(&central, argc, argv)) {
		ExitProcess(1);
	}

	if (!initCentral(&central)) {
		ExitProcess(1);
	}

	if (!criarPipe(&central)) {
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

BOOL initCentral(CENTRAL* central) {
	central->hMutex = CreateMutex(NULL, FALSE, NULL);

	if (central->hMutex == NULL) {
		_tprintf(STR_MUTEX_CREATION_ERROR, GetLastError());
		return FALSE;
	}
	return TRUE;
}

BOOL validarArgumentos(CENTRAL* central, int argc, TCHAR* argv[]) {
	if (argc > 1) {
		size_t tamPrefixo = _tcslen(CENTRAL_PIPE);
		size_t tamNome = _tcslen(argv[1]);

		if (tamPrefixo + tamNome + 1 <= TAM_NOME_PIPE) {
			_tcscpy_s(central->nomePipe, TAM_NOME_PIPE, CENTRAL_PIPE);
			_tcscat_s(central->nomePipe, TAM_NOME_PIPE, argv[1]);
		}
		else {
			_tprintf(STR_PIPE_NAME_EXCEEDED);
			return FALSE;
		}
	}
	else {
		_tprintf(STR_PIPE_NAME_NOT_FOUND);
		return FALSE;
	}

	_tprintf(STR_NAMED_PIPE, central->nomePipe);

	return TRUE;
}

BOOL criarPipe(CENTRAL* central) {
	central->hPipe = CreateNamedPipe(
		central->nomePipe,
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE |
		PIPE_READMODE_MESSAGE |
		PIPE_WAIT,
		1,
		1024,
		1024,
		0,
		NULL
	);

	if (central->hPipe == INVALID_HANDLE_VALUE) {
		_tprintf(STR_NAMED_PIPE_CREATION, GetLastError());
		return FALSE;
	}

	_tprintf(STR_CENTRAL_ACTIVE);

	return TRUE;
}

BOOL initThreads(CENTRAL* central) {
	central->hThreads[0] = CreateThread(NULL, 0, threadComandos, central, 0, NULL);
	central->hThreads[1] = CreateThread(NULL, 0, threadPlacares, central, 0, NULL);

	if (central->hThreads[0] == NULL || central->hThreads[1] == NULL) {
		CloseHandle(central->hThreads[0]);
		CloseHandle(central->hThreads[1]);
		_tprintf(STR_ERRO_INIT_THREADS);
		return FALSE;
	}

	return TRUE;
}

COMANDO parsearComando(TCHAR* cmd) {
	if (_tcscmp(cmd, STR_CMD_ALERTA) == 0) return CMD_ALERTA;
	if (_tcscmp(cmd, STR_CMD_CANCELAR) == 0) return CMD_CANCELAR;
	if (_tcscmp(cmd, STR_CMD_LISTAR) == 0) return CMD_LISTAR;
	if (_tcscmp(cmd, STR_CMD_ENCERRAR) == 0) return CMD_ENCERRAR;
	if (_tcscmp(cmd, STR_CMD_AJUDA) == 0) return CMD_AJUDA;


	return CMD_DESCONHECIDO;
}

DWORD WINAPI threadComandos(LPVOID param) {
	CENTRAL* central = (CENTRAL*)param;
	TCHAR input[TAM_CMD];

	while (!central->terminar) {

		_tprintf(_T("\n"));
		_tprintf(STR_CMD_INPUT);

		_fgetts(input, TAM_CMD, stdin);

		size_t len = _tcslen(input);
		if (len > 0 && input[len - 1] == _T('\n'))
			input[len - 1] = _T('\0');

		TCHAR* args = NULL;
		TCHAR* comando = _tcstok_s(input, _T(" "), &args);

		if (!comando)
			continue;

		switch(parsearComando(comando)) {
		case CMD_ALERTA:
			if (args == NULL)
				_tprintf(STR_CMD_ALERTA_ERRO);
			enviarAlerta(central, args);
			break;
		case CMD_CANCELAR:
			if (args == NULL)
				_tprintf(STR_CMD_ALERTA_ERRO);
			cancelarAlerta(central, args);
			break;
		case CMD_LISTAR:
			listarPlacares(central);
			break;
		case CMD_ENCERRAR:

			break;
		case CMD_AJUDA:
			listarComandos();
			break;
		case CMD_DESCONHECIDO:
			_tprintf(STR_ERRO_COMANDO);
			_tprintf(STR_HELP);
			break;
		}
	}
	return 0;
}

DWORD WINAPI threadPlacares(LPVOID param) {
	CENTRAL* central = (CENTRAL*)param;

	while (!central->terminar) {
		_tprintf(STR_WAITING_CONNECTION);

	
	}

	return 0;
}

BOOL enviarAlerta(CENTRAL* central) {
	//TODO
	return TRUE;
}

BOOL enviarPlacar(CENTRAL* central, int id, void* msg, int size) {

	WaitForSingleObject(central->hMutex, INFINITE);

	for (int i = 0; i < central->numPlacares; i++) {

		if (central->placares[i].id == id &&
			central->placares[i].ligado) {

			DWORD escrito;
			WriteFile(
				central->placares[i].hPipe,
				msg,
				size,
				&escrito,
				NULL
			);

			ReleaseMutex(central->hMutex);
			return TRUE;
		}
	}

	ReleaseMutex(central->hMutex);
	return FALSE;
}

void enviarTodos(CENTRAL* central, void* msg, int size) {

	WaitForSingleObject(central->hMutex, INFINITE);

	for (int i = 0; i < central->numPlacares; i++) {

		if (central->placares[i].ligado) {

			DWORD escrito;
			WriteFile(
				central->placares[i].hPipe,
				msg,
				size,
				&escrito,
				NULL
			);
		}
	}

	ReleaseMutex(central->hMutex);
}

BOOL cancelarAlerta(CENTRAL* central) {
	//TODO
	return TRUE;
}

BOOL listarPlacares(CENTRAL* central) {
	WaitForSingleObject(central->hMutex, INFINITE);

	_tprintf(STR_PLACARES_ATIVOS, central->numPlacares);

	for (int i = 0; i < central->numPlacares; i++) {
		if (central->placares[i].ligado) {
			_tprintf(
				STR_INFO_PLACAR,
				central->placares[i].id,
				central->placares[i].temAlerta ? central->placares[i].msgAlerta : STR_EMPTY
			);
		}
	}
	ReleaseMutex(central->hMutex);

	return TRUE;
}

BOOL listarComandos() {
	_tprintf(STR_CENTRAL);
	_tprintf(STR_CMD_COMANDOS);
	_tprintf(STR_CMD_ALERTA_HELP);
	_tprintf(STR_CMD_CANCELAR_HELP);
	_tprintf(STR_CMD_LISTAR_HELP);
	_tprintf(STR_CMD_ENCERRAR_HELP);
	_tprintf(STR_CMD_AJUDA_HELP);

	return TRUE;
}

BOOL encerrarCentral() {
	//TODO
	return TRUE;
}

void terminarCentral(CENTRAL* central) {
	if (central->hThreads[0]) CloseHandle(central->hThreads[0]);
	if (central->hThreads[1]) CloseHandle(central->hThreads[1]);
	if (central->hMutex) CloseHandle(central->hMutex);
}

