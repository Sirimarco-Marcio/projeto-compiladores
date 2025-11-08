# Projeto de Compilador (2025.2)

Implementação de um compilador por etapas para a disciplina de Compiladores (2025.2), ministrada pela Prof.ª Lis Custódio.

**Autores:**
* Marcio Sirimarco de Souza Junior (Sirimarco-Marcio)
* Mateus Henrique Freitas Maciel (MtHenriqueF) 

---

## 📂 Estrutura do Repositório

* **/Etapa 1 - Analisador Lexico:** Contém o código-fonte, relatório e testes da primeira fase do projeto.
* **/Etapa 2 - Analisador Sintatico:** (Trabalho em andamento)

---

## Etapa 1: Análise Léxica

Esta etapa consiste em um **Analisador Léxico** completo, implementado em C, para a linguagem definida na especificação do projeto.

### 🎯 Funcionalidades Principais

* **Tokenização:** Converte um fluxo de caracteres do código-fonte em um fluxo de tokens (palavras-chave, IDs, números, operadores).
* **Tabela de Símbolos:** Implementa uma tabela de símbolos dinâmica para armazenar e gerenciar identificadores (IDs).
* **Tratamento de Literais:** Reconhece corretamente números (inteiros e `float`) e `string`.
* **Tratamento de Comentários:** Ignora comentários de linha curta (`--`) e comentários de bloco longos (`--[[ ... ]]`).
* **Detecção de Erros:** Identifica e reporta erros léxicos, como strings ou comentários longos não finalizados.

### 🚀 Como Compilar e Executar

O programa foi escrito em C e pode ser compilado com o `gcc`.

```bash
# 1. Compile o analisador
gcc "Analisador Lexico/analisador_lexico.c" -o analisador

# 2. Execute passando um arquivo de teste como argumento
./analisador "Analisador Lexico/Testes/teste2.txt"

# 3. Exemplo de Saída (usando teste2.txt ):
<INICIO, >
<TIPO_FLOAT, >
<ID, 0>
<PONTO_VIRGULA, >
<TIPO_FLOAT, >
<ID, 1>
<PONTO_VIRGULA, >
<ID, 0>
<ATRIBUICAO, >
<NUM, 2500.50>
<PONTO_VIRGULA, >
<ID, 1>
<ATRIBUICAO, >
<ID, 0>
<OP_MULT, >
<NUM, 0.1>
<PONTO_VIRGULA, >
<PRINT, >
<ABRE_PARENTESES, >
<ID, 0>
<OP_SOMA, >
<ID, 1>
<FECHA_PARENTESES, >
<PONTO_VIRGULA, >
<FIM, >
<EOF, >

--- Tabela de Símbolos ---
Índice  | Lexema
---------------------------
0       | salario
1       | bonus
---------------------------
