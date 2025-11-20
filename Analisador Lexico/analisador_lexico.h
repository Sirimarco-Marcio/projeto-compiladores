#ifndef ANALISADOR_LEXICO_H
#define ANALISADOR_LEXICO_H

#include <stdio.h>

//TOKENS
#define TOKEN_BASE 256

#define TOKEN_IDENTIFICADOR    (TOKEN_BASE + 1)
#define TOKEN_NUMERO           (TOKEN_BASE + 2)
#define TOKEN_STRING           (TOKEN_BASE + 3)

//Token para palavras chave
#define TOKEN_INICIO           (TOKEN_BASE + 4)
#define TOKEN_FIM              (TOKEN_BASE + 5)
#define TOKEN_IF               (TOKEN_BASE + 6)
#define TOKEN_ELSE             (TOKEN_BASE + 7)
#define TOKEN_WHILE            (TOKEN_BASE + 8)
#define TOKEN_READ             (TOKEN_BASE + 9)
#define TOKEN_PRINT            (TOKEN_BASE + 10)

//Variaveis
#define TOKEN_TIPO_STRING      (TOKEN_BASE + 11)
#define TOKEN_TIPO_INT         (TOKEN_BASE + 12)
#define TOKEN_TIPO_FLOAT       (TOKEN_BASE + 13)

//Operadores, pontuacao valor do ASCII como valor de token
#define TOKEN_ATRIBUICAO       '='
#define TOKEN_OPERADOR_SOMA    '+'
#define TOKEN_OPERADOR_SUB     '-'
#define TOKEN_OPERADOR_MUL     '*'
#define TOKEN_OPERADOR_DIV     '/'
#define TOKEN_PONTO_VIRGULA    ';'
#define TOKEN_VIRGULA          ','
#define TOKEN_ABRE_PARENTESES  '('
#define TOKEN_FECHA_PARENTESES ')'
#define TOKEN_ABRE_CHAVES      '{'
#define TOKEN_FECHA_CHAVES     '}'

//Tokens para fim de loops
#define TOKEN_EOF              (TOKEN_BASE + 14)
#define TOKEN_ERRO             (TOKEN_BASE + 15)

// --- INICIO DAS MUDANÇAS SEMÂNTICAS (BÔNUS) ---

// Constantes de Tipo (Reutilizando tokens)
#define TIPO_INT           (TOKEN_TIPO_INT)
#define TIPO_FLOAT         (TOKEN_TIPO_FLOAT)
#define TIPO_STRING        (TOKEN_TIPO_STRING)
#define TIPO_VAZIO         (TOKEN_BASE + 16)
#define TIPO_ERRO          (TOKEN_BASE + 17) // Para propagar erros de tipo
#define TIPO_QUALQUER      (TOKEN_BASE + 18) // Para 'print'

// Categorias de Símbolos
#define CAT_VARIAVEL       (TOKEN_BASE + 19)
#define CAT_FUNCAO         (TOKEN_BASE + 20)

// --- FIM DAS MUDANÇAS SEMÂNTICAS ---

#define MAX_ID_LEN 32
#define INCREMENTO_TABELA 32

// --- 2. ESTRUTURAS DE DADOS PÚBLICAS ---

typedef struct
{
    int nome_token;
    union
    {
        int pos_simbolo;
        char valor_literal[1024];
    } atributo;
    int linha;
    int coluna;
} Token;

typedef struct
{
    const char* fonte;
    int posicao;
    char atual;
    int linha;
    int coluna;
    Token token_devolvido;
    int tem_token_devolvido; 
} AnalisadorLexico;

// --- MUDANÇA SEMÂNTICA (Requisito B1) ---
typedef struct 
{
    char lexema[MAX_ID_LEN + 1];
    int categoria; // CAT_VARIAVEL, CAT_FUNCAO
    int tipo_dado; // TIPO_INT, TIPO_FLOAT, TIPO_STRING, TIPO_VAZIO
    int tipo_parametro; // O tipo que a função espera (para read/print)
    int linha_declaracao; // Para erro de redeclaração
} Simbolo;
// --- FIM DA MUDANÇA ---

typedef struct
{
    const char* nome;
    int token;
} PalavraChave;

// --- 3. VARIÁVEIS GLOBAIS (EXTERN) ---
extern AnalisadorLexico g_lexer;
extern Simbolo* tabela_de_simbolos; // <-- MUDANÇA: Exposto para o Sintático
extern int tamanho_tabela;         // <-- MUDANÇA: Exposto para o Sintático


// --- 4. PROTÓTIPOS DE FUNÇÕES PÚBLICAS ---
Token get_token(void);
char* ler_arquivo_fonte(const char* caminho);
void imprimir_linha_erro(const char* fonte_completa, int linha_num, int coluna_num);
void imprimir_token(Token token);
void devolver_token(Token token);

// Protótipo da Tabela de Símbolos (agora usada pelo Sintático)
int instalar_id(const char* lexema);

#endif