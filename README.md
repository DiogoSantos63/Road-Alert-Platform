# \# Placar Informativo — Meta 1

# 

# 

# \## Grupo

# 

# \- \[Diogo Bento Santos - 2022108969]

# 

# \- \[Gustavo Alexandre da Silva Paraíso - 2021132846]

# 

# 

# \## Requisitos Implementados 

# 

# \### 1. Argumentos e Registry

# 

# \- Recebe o nome do named pipe via linha de comandos (`placar.exe <nome>`)

# 

# \- Guarda o nome do pipe no Registry em `HKEY\\\_CURRENT\\\_USER\\\\Software\\\\TrabSO2` (chave `NPIPE`)

# 

# \- Em caso de omissão do argumento, lê o valor do Registry

# 

# \- Caso não exista nem argumento nem valor no Registry, o programa termina com erro

# &#x20;

# 

# \### 2. Interface de linha de comandos

# 

# \- Comando `liga` — gera um identificador aleatório e mostra `Identificador = <id>`

# 

# \- Comando `desliga` — termina o programa de forma controlada, libertando todos os recursos

# &#x20;

# 

# \### 3. Threads

# 

# \- Duas threads em execução simultânea:

# 

# &#x20; - `threadComandos` — gere a interação com o utilizador

# 

# &#x20; - `threadAlertas` — gere a receção e temporização de alertas

# 

# \- Sincronização do output da consola via `CRITICAL\\\_SECTION`

# 

# 

# \### 4. Receção de alertas

# 

# \- A thread de alertas aguarda no evento de reset manual com o nome `"notificar"`

# 

# \- Após sinalização, lê a estrutura `MSG\\\_ALERTA` do par nome-valor Registry (tipo `BINARY`)

# 

# \- Apresenta na consola a hora do sistema e o texto do alerta

# 



# \### 5. Temporização do alerta

# 

# \- Utiliza um `Waitable Timer` para gerir a duração do alerta

# 

# 

# 

# 

# 

# 

# 

# 

# 

# 

# 



