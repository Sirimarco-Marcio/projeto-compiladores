# Projeto de Compilador (2025.2)

Implementação de um compilador completo (Front-end) para a disciplina de **Compiladores**, ministrada pela **Prof.ª Lis Custódio**.

O projeto realiza a leitura de arquivos fonte de uma linguagem personalizada e executa as etapas de análise léxica, sintática e semântica, com mecanismos avançados de recuperação de erros.

**Autores:**
* Marcio Sirimarco de Souza Junior (Sirimarco-Marcio)
* Mateus Henrique Freitas Maciel (MtHenriqueF)

---

## 🚀 Funcionalidades

### 1. Análise Léxica
* Identificação de tokens (palavras-chave, identificadores, números, strings).
* Remoção de espaços em branco e comentários (linha `--` e bloco `--[[ ... ]]`).
* Detecção de erros léxicos (ex: caracteres inválidos, strings não fechadas).

### 2. Análise Sintática
* Parser Descendente Recursivo.
* **Recuperação de Erros:**
  * **Modo Pânico:** Descarta tokens até encontrar um ponto de sincronização seguro (ex: `;`, `}`).
  * **Inserção:** Insere tokens faltantes (ex: `;`) para tentar continuar a análise.

### 3. Análise Semântica
* **Tabela de Símbolos:** Verificação de existência e escopo de variáveis.
* **Verificação de Tipos:** Garante que operações matemáticas e atribuições sejam feitas entre tipos compatíveis.
* **Validação de Comandos:** Checa se expressões em `if`/`while` são numéricas, etc.

---

## 📦 Como Compilar e Rodar

O projeto utiliza um **Makefile** para facilitar a compilação. Certifique-se de ter o `gcc` e o `make` instalados.

### 1. Compilar o projeto
No terminal, na raiz do projeto, execute:

```bash
make
````

Isso irá gerar o executável `compilador`.

### 2\. Executar um teste

Passe o caminho do arquivo de código-fonte como argumento:

```bash
./compilador "Analisador Sintático/Testes/teste_erro1.txt"
```

### 3\. Limpar arquivos temporários

Para remover o executável gerado:

```bash
make clean
```

-----

## 🧪 Exemplos de Execução e Tratamento de Erros

Abaixo, exemplos de como o compilador reage a diferentes tipos de erros no código-fonte.

### 1\. Erro Léxico

*Cenário: O programador esqueceu de fechar as aspas de uma string.*

**Entrada:**

```text
inicio
  string texto = "Ola mundo; 
fim
```

**Saída do Compilador:**

```text
<INICIO, >
<TIPO_STRING, >
<ID, 0>
<ATRIBUICAO, >
<ERRO, String nao finalizada antes do fim do arquivo: "Ola mundo;>
```

### 2\. Erro Sintático (Com Recuperação)

*Cenário: Falta de expressão após atribuição e falta de fechamento de bloco.*

**Entrada:**

```text
inicio
  int a;
  a = ;     -- Erro: Falta expressão
  {
    print("bloco incompleto");
  -- Erro: Falta '}'
fim
```

**Saída do Compilador:**

```text
Erro linha 4, coluna 7:
  >    a = ;  -- Erro 1: Expressao faltando apos a atribuicao
          ^
ERRO SINTÁTICO: Esperado expressao.
  Token recebido: ';' (inesperado)
  Iniciando modo pânico... Sincronizando com { ')' ',' ';' }
  Sincronização encontrada. Continuando análise no token ';'.

Erro linha 10, coluna 1:
  > fim
    ^
ERRO SINTÁTICO: esperado '}' antes do token 'fim' (recuperação por inserção).

[FALHA] Foram encontrados erros sintáticos e/ou semânticos.
```

### 3\. Erro Semântico

*Cenário: Uso de variável não declarada e tipos incompatíveis.*

**Entrada:**

```text
inicio
  int a;
  a = b + 10;    -- Erro: 'b' não existe
  read(print);   -- Erro: 'print' é palavra reservada, não variável
fim
```

**Saída do Compilador:**

```text
Erro linha 3, coluna 7:
  >    a = b + 10;
          ^
ERRO SEMÂNTICO: Variável 'b' não declarada.

Erro linha 4, coluna 8:
  >    read(print);
            ^
ERRO SINTÁTICO: Esperava IDENTIFICADOR.
  Token recebido: 'print' (inesperado)
```

-----

## 📂 Estrutura de Diretórios

```bash
.
├── Analisador Lexico/      # Implementação e headers do léxico
├── Analisador Sintatico/   # Implementação do sintático e semântico
├── compilador.c            # Ponto de entrada (Main)
├── Makefile                # Script de automação de build
└── README.md               # Documentação
```
