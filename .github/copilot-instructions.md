# Copilot Instructions — Road Alert Platform (Placar Informativo)

## Project Overview

This is a **Windows native C application** developed for the "Sistemas Operativos 2" (Operating Systems 2) course at ISEC. The project implements a **Road Alert Platform** — a system for distributing alert messages to electronic road signs (placares), similar to those found on highways.

The platform consists of three programs:

- **Placar** (sign) — Console application representing a road sign. Receives alerts from Central and displays them with a timed duration. Communicates via named pipes. Uses the Windows Registry to store/retrieve the named pipe name.
- **Central** — Console application acting as the command center. Manages placar registration (up to 20), sends alerts, cancels alerts, lists active alerts, and shuts down the platform. Communicates with placares via named pipes and with monitors via shared memory.
- **Monitor** — Win32 GUI application that displays all currently active alerts. Communicates with Central exclusively via shared memory (memory-mapped files).

## Language and Environment

- **Language:** C (not C++)
- **Target OS:** Windows (Win32/WinNT API)
- **IDE:** Visual Studio (Windows SDK project)
- **Character encoding:** Unicode via `TCHAR`. The project is configured for Unicode (`_UNICODE` / `UNICODE` defined).
- **Entry point:** `_tmain` for console apps, `WinMain` for GUI apps.

## Critical Rules

### TCHAR and Unicode Compliance

- **Always** use `TCHAR` instead of `char` or `wchar_t` for all string data.
- **Always** use `_tcs*` functions (e.g., `_tcscpy_s`, `_tcslen`, `_tcscmp`) instead of `str*` or `wcs*` functions.
- **Always** use `_T("...")` or `TEXT("...")` macros for string literals.
- **Always** use `_tprintf` / `_tscanf` / `_fgetts` instead of `printf` / `scanf` / `fgets`.
- **Always** use Win32 API functions without the `A` or `W` suffix (e.g., `CreateFile`, not `CreateFileA` or `CreateFileW`).
- Console apps must set Unicode mode at startup:
  ```c
  #ifdef UNICODE
      _setmode(_fileno(stdin), _O_WTEXT);
      _setmode(_fileno(stdout), _O_WTEXT);
  #endif
  ```

### Win32 Data Types

- Use Windows typedefs: `DWORD`, `BOOL`, `BYTE`, `HANDLE`, `LPVOID`, `LPTSTR`, `LPCTSTR`, etc.
- Follow Hungarian notation for variable names as per Win32 convention:
  - `dw` prefix for `DWORD` (e.g., `dwIdentificador`)
  - `sz` or `lpsz` prefix for strings (e.g., `szMensagem`)
  - `h` prefix for handles (e.g., `hPipe`, `hThread`, `hEvento`)
  - `b` prefix for `BOOL` (e.g., `bAtivo`)
  - `lp` prefix for pointers (e.g., `lpDados`)
  - `n` or `i` prefix for integers used as counts/indices

### Error Handling

- **Always** check return values of Win32 API calls.
- Use `GetLastError()` to retrieve error details after a failed API call.
- Log meaningful error messages with `_tprintf` before exiting or recovering.
- Clean up all resources (handles, memory, synchronization objects) even on error paths.

## Data Structures (from the specification)

These structures are **mandated by the assignment** — do not change their layout:

```c
// Placar → Central messages
typedef struct {
    BYTE tipo;  // 1 – ligar, 2 – desligar, 3 – fim do alerta (duração)
} MSG_CMD;

// Central → Placar: identifier assignment
typedef struct {
    BYTE tipo;          // 3 – atribuir identificador
    DWORD identificador;
} MSG_ID;

// Central → Placar: new alert
typedef struct {
    BYTE tipo;          // 4 – novo alerta
    TCHAR msg[140];
    DWORD duracao;
} MSG_ALERTA;

// Central → Placar: cancel alert / shutdown
typedef struct {
    BYTE tipo;          // 5 – cancelar alerta, 6 – encerrar plataforma
} MSG_CMD;

// Shared memory structure (Central ↔ Monitor)
typedef struct {
    DWORD identificador, duracao;
    TCHAR msg[140];
    BOOL desligar;
} SHM_ALERTA;
```

## IPC Mechanisms

### Named Pipes (Placar ↔ Central)

- Used for all communication between Placar and Central.
- The pipe name is received via command-line arguments or read from the Windows Registry at `HKEY_CURRENT_USER\Software\TrabSO2` (value name: `NPIPE`, type: `REG_SZ`).
- If the pipe name is provided via command line, store it in the Registry. If not provided, read it from the Registry. If neither exists, the program must not run.
- Use `CreateNamedPipe`, `ConnectNamedPipe`, `CreateFile` (client side), `ReadFile`, `WriteFile`.
- Always consult [Microsoft Named Pipes documentation](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipes) for correct usage.

### Shared Memory (Central ↔ Monitor)

- Used exclusively for Central-to-Monitor communication.
- Implemented via memory-mapped files (`CreateFileMapping` with `INVALID_HANDLE_VALUE`, `MapViewOfFile`, `OpenFileMapping`).
- **Never** place pointers in shared memory — they are meaningless across process boundaries.
- **Never** place C++ STL objects (string, vector, etc.) in shared memory.
- Use only flat structures with fixed-size arrays.
- Synchronization of shared memory access must be done explicitly with mutexes or semaphores — the shared memory mechanism provides no built-in synchronization.
- Changes to shared memory should be signaled to monitors using appropriate synchronization objects (events, etc.) — not via any other IPC mechanism.

### Windows Registry

- Key path: `HKEY_CURRENT_USER\Software\TrabSO2`
- Value name: `NPIPE` (type `REG_SZ`)
- Use `RegCreateKeyEx`, `RegSetValueEx`, `RegQueryValueEx`, `RegGetValue`, `RegCloseKey`.
- Always consult [Microsoft Registry documentation](https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry) for correct API usage.

## Threading

- The Placar program uses **at least two threads**: one for user input and one for receiving/timing alerts.
- Use `CreateThread` to create threads. Each thread function follows the `DWORD WINAPI ThreadFunc(LPVOID lpParam)` signature.
- **Never** use `TerminateThread` — always use cooperative termination via shared flags or events.
- Thread termination should be graceful: signal the thread, then `WaitForSingleObject` on its handle.
- Always `CloseHandle` on thread handles when done.

## Synchronization

- Use the correct synchronization primitive for each scenario:
  - **Mutexes** — for mutual exclusion between threads/processes (named mutexes for cross-process).
  - **Critical Sections** — for mutual exclusion within a single process (lighter than mutexes).
  - **Semaphores** — for resource counting and producer/consumer patterns.
  - **Events** — for signaling/coordination between threads (manual-reset or auto-reset as appropriate).
  - **Waitable Timers** — for alert duration management (use `CreateWaitableTimer` + `SetWaitableTimer`).
- Use `WaitForSingleObject` and `WaitForMultipleObjects` for blocking waits.
- Always release/close synchronization objects when they are no longer needed.
- Consult [Microsoft Synchronization documentation](https://learn.microsoft.com/en-us/windows/win32/sync/synchronization-objects) when in doubt.

## Code Style and Quality

### Self-Explanatory Code

- Write code that is **self-documenting**: meaningful variable names, clear function names, logical structure.
- **Do not** add comments that restate what the code obviously does.
- **Do** add comments only where the logic is non-obvious, where a design decision needs justification, or to reference the assignment specification.
- Function names should clearly describe their purpose (e.g., `RegistarPlacar`, `EnviarAlerta`, `ProcessarComando`).

### Code Organization

- Keep each program's source in its own project directory within the solution.
- Separate concerns: input handling, communication logic, data management, and display should be in distinct functions.
- Keep functions short and focused — each function should do one thing.
- Shared definitions (structs, constants, message types) should be in a common header file.

### Clean Code Practices

- No magic numbers — use `#define` or `enum` for constants.
- No global variables unless strictly necessary for thread communication (and then protect with synchronization).
- Always initialize variables before use.
- Use `_countof()` for array sizes and secure string functions (`_tcscpy_s`, `_tcscat_s`, `StringCchCopy`, etc.).
- Properly free/close all resources: handles, pipes, mapped views, registry keys, synchronization objects.

### Naming Conventions

- Constants and macros: `UPPER_SNAKE_CASE` (e.g., `MAX_PLACARES`, `TAM_MSG`)
- Structs: `UPPER_SNAKE_CASE` (e.g., `MSG_ALERTA`, `SHM_ALERTA`)
- Functions: `PascalCase` (e.g., `IniciarPlacar`, `EnviarAlerta`)
- Local variables: `camelCase` with Hungarian prefix where it aids clarity
- Global/shared variables: clearly documented, protected by synchronization

## Platform Constants

```c
#define MAX_PLACARES    20      // Maximum number of signs in the platform
#define TAM_MSG         140     // Alert message length (TCHAR)
```

## Assignment Milestones

### M1 — Placar (Deadline: April 12)

1. Receive "ligar" and "desligar" commands from the user.
2. Read named pipe name from command line or Registry (`HKEY_CURRENT_USER\Software\TrabSO2\NPIPE`). Create the Registry value if it doesn't exist.
3. Two threads: one for user input, one for simulated alert reception.
4. Alert reception: wait on a manual-reset event named `_T("notificar")`, then read alert data from the Registry value named after the pipe (type `REG_BINARY`, content matching `MSG_ALERTA` structure).
5. Use a waitable timer to manage alert duration, displaying `'---'` when the alert expires.

### M2 — Central (Deadline: May 10)

- Full Central implementation with named pipe communication to placares.
- Commands: `alerta`, `cancelar`, `listar`, `encerrar`.
- Adapt Placar to communicate with Central via named pipes.

### M3 — Monitor (Deadline: May 31)

- Win32 GUI application showing all active alerts.
- Communication with Central via shared memory only.
- Menu for: max alerts to display, about dialog, exit.

## Microsoft API Documentation

Whenever implementing or modifying Win32 API calls, **always consult the official Microsoft documentation**:

- [Win32 API Index](https://learn.microsoft.com/en-us/windows/win32/api/)
- [Processes and Threads](https://learn.microsoft.com/en-us/windows/win32/procthread/processes-and-threads)
- [Synchronization](https://learn.microsoft.com/en-us/windows/win32/sync/synchronization)
- [Named Pipes](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipes)
- [Memory-Mapped Files](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping)
- [Registry](https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry)
- [Waitable Timers](https://learn.microsoft.com/en-us/windows/win32/sync/waitable-timer-objects)

Pay special attention to:
- Correct parameter types and sizes
- Security attributes and access flags
- Proper cleanup sequences
- Thread safety requirements

## Common Pitfalls to Avoid

1. **Never** place pointers in shared memory.
2. **Never** use C++ STL containers (string, vector, etc.) in shared memory.
3. **Never** use `TerminateThread` — use cooperative shutdown.
4. **Never** hardcode character sizes — use `sizeof(TCHAR)` when needed.
5. **Never** mix `char`/`wchar_t` with `TCHAR` — be consistent.
6. **Never** forget to close handles, unmap views, or release synchronization objects.
7. **Never** assume shared memory is mapped at the same address in different processes.
8. **Always** check if the platform is already running (Central uniqueness) using a named mutex.
9. **Always** handle graceful shutdown — notify all placares and monitors before Central exits.
10. **Always** protect concurrent access to shared data with proper synchronization primitives.
