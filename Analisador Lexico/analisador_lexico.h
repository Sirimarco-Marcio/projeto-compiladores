#ifndef ANALISADOR_LEXICO_H
#define ANALISADOR_LEXICO_H

#include <stdio.h>

/* ================== TOKENS ================== */

#define TOKEN_BASE 256

#define TOKEN_IDENTIFICADOR    (TOKEN_BASE + 1)
#define TOKEN_NUMERO           (TOKEN_BASE + 2)
#define TOKEN_STRING           (TOKEN_BASE + 3)

/* palavras-chave */
#define TOKEN_INICIO           (TOKEN_BASE + 4)
#define TOKEN_FIM              (TOKEN_BASE + 5)
#define TOKEN_IF               (TOKEN_BASE + 6)
#define TOKEN_ELSE             (TOKEN_BASE + 7)
#define TOKEN_WHILE            (TOKEN_BASE + 8)
#define TOKEN_READ             (TOKEN_BASE + 9)
#define TOKEN_PRINT            (TOKEN_BASE + 10)

/* tipos */
#define TOKEN_TIPO_STRING      (TOKEN_BASE + 11)
#define TOKEN_TIPO_INT         (TOKEN_BASE + 12)
#define TOKEN_TIPO_FLOAT       (TOKEN_BASE + 13)

/* símbolos de 1 caractere usam o próprio ASCII como token */
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

/* especiais */
#define TOKEN_EOF              (TOKEN_BASE + 14)
#define TOKEN_ERRO             (TOKEN_BASE + 15)

#define MAX_ID_LEN 32
#define INCREMENTO_TABELA 32

/* ================== TIPOS PARA A TABELA DE SÍMBOLOS ================== */

typedef enum {
    CAT_VARIAVEL,
    CAT_FUNCAO_BUILTIN
} CategoriaSimbolo;

typedef enum {
    TIPO_INT_S,
    TIPO_FLOAT_S,
    TIPO_STRING_S,
    TIPO_INDEFINIDO,
    TIPO_ERRO
} TipoSimbolo;

typedef struct
{
    char lexema[MAX_ID_LEN + 1];
    CategoriaSimbolo categoria;
    TipoSimbolo tipo;
} Simbolo;

/* ================== ESTRUTURAS DE TOKEN E LÉXICO ================== */

typedef struct
{
    int nome_token;
    union
    {
        int pos_simbolo;          /* para IDs */
        char valor_literal[1024]; /* para números e strings */
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

/* mapa de palavras chave */
typedef struct
{
    const char* nome;
    int token;
} PalavraChave;

/* ================== VARIÁVEIS GLOBAIS ================== */

extern AnalisadorLexico g_lexer;

/* tabela de símbolos exposta para o analisador sintático/semântico */
extern Simbolo* tabela_de_simbolos;
extern int tamanho_tabela;
extern int capacidade_tabela;

/* ================== PROTÓTIPOS ================== */

Token get_token(void);
char* ler_arquivo_fonte(const char* caminho);
void imprimir_linha_erro(const char* fonte_completa, int linha_num, int coluna_num);
void imprimir_token(Token token);
void devolver_token(Token token);
const char* nome_token_legivel(int token);

#endif
