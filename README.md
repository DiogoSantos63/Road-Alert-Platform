# Placar Informativo — Meta 1, Meta 2 e Meta 3

## Grupo

- Diogo Bento Santos - 2022108969
- Gustavo Alexandre da Silva Paraíso - 2021132846

## Requisitos Implementados

### Meta 1 — Programa Placar

#### 1. Argumentos e Registry
- Recebe o nome do named pipe via linha de comandos (`placar.exe <nome>`)
- Guarda o nome do pipe no Registry em `HKEY_CURRENT_USER\Software\TrabSO2` (chave `NPIPE`)
- Em caso de omissão do argumento, lê o valor do Registry
- Caso não exista nem argumento nem valor no Registry, o programa termina com erro

#### 2. Interface de linha de comandos
- Comando `ligar` — estabelece ligação ao central e mostra `Identificador = <id>`
- Comando `desligar` — termina o programa de forma controlada, libertando todos os recursos

#### 3. Threads
- Duas threads em execução simultânea:
  - `threadComandos` — gere a interação com o utilizador
  - `threadAlertas` — gere a receção e temporização de alertas
- Sincronização do output da consola via `CRITICAL_SECTION`

#### 4. Receção de alertas (Meta 1)
- A thread de alertas aguarda no evento de reset manual com o nome `"notificar"`
- Após sinalização, lê a estrutura `MSG_ALERTA` do par nome-valor Registry (tipo `BINARY`)
- Apresenta na consola a hora do sistema e o texto do alerta

#### 5. Temporização do alerta
- Utiliza um `Waitable Timer` para gerir a duração do alerta
- Após expirar a duração, apresenta `'---'` na consola
- Se um novo alerta chegar antes do timer expirar, o alerta anterior é cancelado automaticamente

---

### Meta 2 — Programa Central + Alterações ao Placar

#### 1. Programa Central
- Recebe o nome do named pipe via linha de comandos (`central.exe <nome>`)
- Cria o named pipe `\\.\pipe\<nome>` em modo message, bidirecional, com Overlapped I/O
- Suporta até `MAX_PLACARES` (20) instâncias simultâneas do pipe
- Instâncias do pipe são reutilizadas após desconexão de um placar

#### 2. Threads do Central
- `threadComandos` — gere a interação com o utilizador administrador
- `threadPlacaresOverlappedIO` — gere todas as ligações e comunicações com os placares numa única thread usando Overlapped I/O e máquina de estados por placar (`ESPERA_CONEXAO` → `LE_PEDIDO` → `ENVIA_RESPOSTA`)

#### 3. Comandos do administrador
- `alerta <msg> <duracao> <id>` — envia alerta a um placar específico ou a todos (id=0); suporta mensagens com espaços entre aspas
- `cancelar <id>` — cancela o alerta ativo no placar especificado
- `listar` — lista todos os placares ligados e respetivos alertas ativos
- `encerrar` — envia MSG_CMD tipo 6 a todos os placares e termina a plataforma
- `ajuda` — lista os comandos disponíveis

#### 4. Comunicação Central ↔ Placar (Named Pipes)
- Tipo 1 (ligar) → central atribui ID e responde com MSG_ID (tipo 7)
- Tipo 2 (desligar) → central confirma e remove o placar da lista
- Tipo 3 (fim alerta) → central atualiza o estado interno do alerta
- Tipo 4 (novo alerta) → central envia MSG_ALERTA; placar confirma com a mesma struct
- Tipo 5 (cancelar alerta) → central envia MSG_CMD; placar confirma
- Tipo 6 (encerrar) → central envia MSG_CMD; placar termina

#### 5. Alterações ao Placar (Meta 2)
- `ligarPlacar` — abre o named pipe do central via `CreateFile`, envia MSG_CMD tipo 1 e recebe MSG_ID tipo 7 com o identificador atribuído
- `desligarPlacar` — envia MSG_CMD tipo 2 ao central e aguarda confirmação antes de terminar
- `threadEscutaCentral` — thread dedicada à escuta do central; trata tipos 2, 4, 5 e 6; sinaliza eventos internos para a `threadAlertas`
- Após timer expirar, envia MSG_CMD tipo 3 ao central

#### 6. Sincronização
- Mutex para proteger o acesso concorrente à lista de placares no central
- `CRITICAL_SECTION` para proteger o output da consola no placar
- `CancelIoEx` usado para interromper operações I/O pendentes quando o administrador envia comandos
- Dois loops separados no `cmdEnviarAlerta` e `cmdEncerrarCentral`: primeiro preenche todos os dados, depois acorda as threads — evita race conditions

---

### Meta 3 — Programa Monitor + Alterações ao Central

#### 1. Programa Monitor
- Interface gráfica Win32
- Menu `Ficheiro` com três entradas:
  - `Configuracao` — DialogBox para definir número máximo de alertas a apresentar e nomes dos recursos de comunicação (`SHM` e evento)
  - `Acerca` — MessageBox com nome e número dos elementos do grupo
  - `Sair` — termina a aplicação libertando todos os recursos
- Área de cliente mostra identificador do placar, mensagem de alerta e duração em segundos
- Suporte a paginação via teclas `Page Down` e `Page Up` quando o número de alertas supera o máximo configurado
- Liga automaticamente à memória partilhada no arranque; se o central não estiver a correr tenta religar periodicamente via timer sem bloquear a interface

#### 2. Alterações ao Central (Meta 3)
- Criação de bloco de memória partilhada (`SHM_ALERTA`) com nome `SHM_TrabSO2_Alertas`
- Criação de evento com nome `EVT_TrabSO2_Monitor` para notificar os monitores
- A memória partilhada é atualizada sempre que há alterações ao estado dos alertas: novo alerta, cancelamento, fim de duração, ligação ou desligação de placar, encerramento da plataforma
- Evento de reset manual — permite notificar múltiplos monitores em simultâneo; o central faz `ResetEvent` após um breve delay para garantir que todos os monitores acedem à informação atualizada antes do reset

#### 3. Sincronização Monitor ↔ Central
- O monitor usa `WaitForSingleObject` no evento nomeado — sem polling
- O evento é manual-reset, criado pelo central, aberto pelos monitores via `OpenEvent`
- Quando o central encerra, define `desligar = TRUE` na SHM e notifica os monitores, que apresentam uma mensagem e terminam
- Podem existir múltiplas instâncias do monitor em simultâneo

#### 4. Decisoes de implementacao
- Os nomes da SHM e do evento são configuráveis via DialogBox de Configuracao — permite ao utilizador adaptar o monitor a diferentes instâncias do central sem recompilar
- O monitor usa `FILE_MAP_READ` para aceder à SHM — apenas leitura, o central tem acesso exclusivo de escrita
- O tamanho mínimo da janela é fixo (700x500) para garantir que toda a informação é sempre visível