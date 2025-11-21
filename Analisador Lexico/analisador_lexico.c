#include "analisador_lexico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/* estados do AFD */
#define STATE_INICIAL          0
#define STATE_EM_IDENTIFICADOR 1
#define STATE_EM_NUMERO        2
#define STATE_EM_COMENTARIO    3
#define STATE_EM_STRING        4

/* tabela de símbolos (global) */
Simbolo* tabela_de_simbolos = NULL;
int tamanho_tabela = 0;
int capacidade_tabela = 0;

AnalisadorLexico g_lexer;

/* palavras-chave */
PalavraChave palavras_chave[] = 
{
    {"inicio", TOKEN_INICIO},
    {"fim", TOKEN_FIM},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE}, 
    {"while", TOKEN_WHILE}, 
    {"string", TOKEN_TIPO_STRING},
    {"int", TOKEN_TIPO_INT}, 
    {"float", TOKEN_TIPO_FLOAT}, 
    {"read", TOKEN_READ},
    {"print", TOKEN_PRINT},
    {NULL, 0}
};

void avancar();
Token get_token(void);
int instalar_id(const char* lexema);
char peek(void);

/* ================== TABELA DE SÍMBOLOS ================== */

int instalar_id(const char* lexema)
{
    /* verifica se já existe */
    for(int i = 0; i < tamanho_tabela; i++) 
    {
        if(strcmp(tabela_de_simbolos[i].lexema, lexema) == 0) return i;
    }

    if(tamanho_tabela >= capacidade_tabela) 
    {
        int nova_capacidade = capacidade_tabela + INCREMENTO_TABELA;
        Simbolo* nova_tabela = (Simbolo*) realloc(tabela_de_simbolos,
                                                  nova_capacidade * sizeof(Simbolo));
        if(nova_tabela == NULL)
        {
            printf("ERRO FATAL: Falha ao realocar a tabela de símbolos!\n");
            exit(1);
        }

        tabela_de_simbolos = nova_tabela;
        capacidade_tabela  = nova_capacidade;
    }
    
    /* insere o novo símbolo */
    strncpy(tabela_de_simbolos[tamanho_tabela].lexema, lexema, MAX_ID_LEN);
    tabela_de_simbolos[tamanho_tabela].lexema[MAX_ID_LEN] = '\0';
    tabela_de_simbolos[tamanho_tabela].categoria = CAT_VARIAVEL;
    tabela_de_simbolos[tamanho_tabela].tipo      = TIPO_INDEFINIDO;

    return tamanho_tabela++;
}

void devolver_token(Token token) {
    if (g_lexer.tem_token_devolvido) {
        printf("ERRO: Já existe um token devolvido!\n");
        return;
    }
    g_lexer.token_devolvido = token;
    g_lexer.tem_token_devolvido = 1;
}

/* ================== LÉXICO ================== */
const char* nome_token_legivel(int token) {
    switch (token) {
        /* Tokens Genéricos */
        case TOKEN_IDENTIFICADOR:    return "IDENTIFICADOR";
        case TOKEN_NUMERO:           return "NUMERO";
        case TOKEN_STRING:           return "LITERAL_STRING";

        /* Palavras-chave */
        case TOKEN_INICIO:           return "'inicio'";
        case TOKEN_FIM:              return "'fim'";
        case TOKEN_IF:               return "'if'";
        case TOKEN_ELSE:             return "'else'";
        case TOKEN_WHILE:            return "'while'";
        case TOKEN_READ:             return "'read'";
        case TOKEN_PRINT:            return "'print'";

        /* Tipos de Dados */
        case TOKEN_TIPO_STRING:      return "'string'";
        case TOKEN_TIPO_INT:         return "'int'";
        case TOKEN_TIPO_FLOAT:       return "'float'";

        /* Símbolos e Operadores (Mapeados para ASCII no header) */
        case TOKEN_ATRIBUICAO:       return "'='";
        case TOKEN_OPERADOR_SOMA:    return "'+'";
        case TOKEN_OPERADOR_SUB:     return "'-'";
        case TOKEN_OPERADOR_MUL:     return "'*'";
        case TOKEN_OPERADOR_DIV:     return "'/'";
        case TOKEN_PONTO_VIRGULA:    return "';'";
        case TOKEN_VIRGULA:          return "','";
        case TOKEN_ABRE_PARENTESES:  return "'('";
        case TOKEN_FECHA_PARENTESES: return "')'";
        case TOKEN_ABRE_CHAVES:      return "'{'";
        case TOKEN_FECHA_CHAVES:     return "'}'";

        /* Especiais */
        case TOKEN_EOF:              return "FIM_DE_ARQUIVO";
        case TOKEN_ERRO:             return "ERRO_LEXICO";

        default:                     return "TOKEN_DESCONHECIDO";
    }
}

Token get_token(void) {
    if (g_lexer.tem_token_devolvido) {
        g_lexer.tem_token_devolvido = 0;
        return g_lexer.token_devolvido;
    }

    int estado = STATE_INICIAL;
    char lexeme_buffer[1024];
    int lexeme_pos = 0;
    Token token;
    memset(&token, 0, sizeof(Token));

    while(1) 
    {
        char c = g_lexer.atual;
        switch (estado)
        {
            case STATE_INICIAL:
                if(isspace(c))
                { 
                    avancar();
                    continue; 
                }

                token.linha = g_lexer.linha;
                token.coluna = g_lexer.coluna;

                if(c == '\0')
                { 
                    token.nome_token = TOKEN_EOF;
                    return token; 
                }

                if(c == '-' && peek() == '-')
                { 
                    estado = STATE_EM_COMENTARIO;
                    continue;
                }
                
                if(c == '"')
                { 
                    estado = STATE_EM_STRING;
                    avancar();
                    continue;
                }

                if(strchr("=+-*/;(),{}", c))
                {
                    token.nome_token = c;
                    avancar();
                    return token;
                }

                if(isalpha(c) || c == '_') 
                {
                    estado = STATE_EM_IDENTIFICADOR;
                    lexeme_buffer[lexeme_pos++] = c;
                    avancar();
                    continue;
                }

                if(isdigit(c)) 
                {
                    estado = STATE_EM_NUMERO;
                    lexeme_buffer[lexeme_pos++] = c;
                    avancar();
                    continue;
                }

                token.nome_token = TOKEN_ERRO;
                token.atributo.valor_literal[0] = c;
                token.atributo.valor_literal[1] = '\0';
                avancar();
                return token;

            case STATE_EM_IDENTIFICADOR:
                if(isalnum(g_lexer.atual) || g_lexer.atual == '_')
                {
                    if(lexeme_pos < 1023) 
                    {
                        lexeme_buffer[lexeme_pos++] = g_lexer.atual;
                    }
                    avancar();
                } 
                else
                {
                    lexeme_buffer[lexeme_pos] = '\0';

                    if(strlen(lexeme_buffer) > MAX_ID_LEN)
                    {
                        token.nome_token = TOKEN_ERRO;
                        char msg_erro[256];
                        sprintf(msg_erro, "ID excede o limite de %d caracteres: %.35s...",
                                MAX_ID_LEN, lexeme_buffer);
                        strcpy(token.atributo.valor_literal, msg_erro);
                        return token;
                    }

                    token.nome_token = TOKEN_IDENTIFICADOR;

                    for(int i = 0; palavras_chave[i].nome != NULL; i++) 
                    {
                        if(strcmp(lexeme_buffer, palavras_chave[i].nome) == 0) 
                        {
                            token.nome_token = palavras_chave[i].token;
                            break;
                        }
                    }

                    if(token.nome_token == TOKEN_IDENTIFICADOR)
                    {
                        token.atributo.pos_simbolo = instalar_id(lexeme_buffer);
                    }

                    return token;
                }
                break;

            case STATE_EM_NUMERO:
                if(isdigit(g_lexer.atual) || g_lexer.atual == '.') 
                {
                    lexeme_buffer[lexeme_pos++] = g_lexer.atual;
                    avancar();
                } 
                else
                {
                    lexeme_buffer[lexeme_pos] = '\0';
                    token.nome_token = TOKEN_NUMERO;
                    strcpy(token.atributo.valor_literal, lexeme_buffer);
                    return token;
                }
                break;

            case STATE_EM_STRING:
                if(g_lexer.atual == '\0')
                {
                    token.nome_token = TOKEN_ERRO;

                    lexeme_buffer[lexeme_pos] = '\0';
                    char msg_erro[256];
                    
                    sprintf(msg_erro, "String nao finalizada antes do fim do arquivo: \"%.35s",
                            lexeme_buffer);
                    strcpy(token.atributo.valor_literal, msg_erro);

                    return token;
                }

                if(g_lexer.atual == '"')
                {
                    lexeme_buffer[lexeme_pos] = '\0';
                    token.nome_token = TOKEN_STRING;
                    strcpy(token.atributo.valor_literal, lexeme_buffer);
                    avancar(); 
                    return token;
                }

                if(lexeme_pos < 1023)
                { 
                    lexeme_buffer[lexeme_pos++] = g_lexer.atual;
                }

                avancar();
                break;

            case STATE_EM_COMENTARIO:
                avancar();
                avancar();

                /* comentário longo */
                if(g_lexer.atual == '[' && peek() == '[') 
                {
                    avancar();
                    avancar();

                    while(g_lexer.atual != '\0' &&
                         !(g_lexer.atual == ']' && peek() == ']')) 
                    {
                        avancar();
                    }

                    if(g_lexer.atual == '\0') 
                    {
                        token.nome_token = TOKEN_ERRO;
                        char msg_erro[256];
                        sprintf(msg_erro, "Comentario longo nao finalizado antes do fim do arquivo");
                        strcpy(token.atributo.valor_literal, msg_erro);
                        return token;
                    } 
                    else
                    {
                        avancar();
                        avancar();
                    }
                } 
                else
                {
                    /* comentário curto até o fim da linha */
                    while(g_lexer.atual != '\n' && g_lexer.atual != '\0') 
                    {
                        avancar(); 
                    }
                }

                estado = STATE_INICIAL;
                continue;
        }
    }
}

void imprimir_token(Token token) 
{
    switch(token.nome_token) 
    {
        case TOKEN_IDENTIFICADOR: printf("<ID, %d>\n", token.atributo.pos_simbolo); break;
        case TOKEN_NUMERO:        printf("<NUM, %s>\n", token.atributo.valor_literal); break;
        case TOKEN_STRING:        printf("<STRING, %s>\n", token.atributo.valor_literal); break;
        case TOKEN_INICIO:        printf("<INICIO, >\n"); break;
        case TOKEN_FIM:           printf("<FIM, >\n"); break;
        case TOKEN_IF:            printf("<IF, >\n"); break;
        case TOKEN_ELSE:          printf("<ELSE, >\n"); break;
        case TOKEN_WHILE:         printf("<WHILE, >\n"); break;
        case TOKEN_TIPO_STRING:   printf("<TIPO_STRING, >\n"); break;
        case TOKEN_TIPO_INT:      printf("<TIPO_INT, >\n"); break;
        case TOKEN_TIPO_FLOAT:    printf("<TIPO_FLOAT, >\n"); break;
        case TOKEN_READ:          printf("<READ, >\n"); break;
        case TOKEN_PRINT:         printf("<PRINT, >\n"); break;
        case TOKEN_ATRIBUICAO:       printf("<ATRIBUICAO, >\n"); break;
        case TOKEN_OPERADOR_SOMA:    printf("<OP_SOMA, >\n"); break;
        case TOKEN_OPERADOR_SUB:     printf("<OP_SUB, >\n"); break;
        case TOKEN_OPERADOR_MUL:     printf("<OP_MULT, >\n"); break;
        case TOKEN_OPERADOR_DIV:     printf("<OP_DIV, >\n"); break;
        case TOKEN_PONTO_VIRGULA:    printf("<PONTO_VIRGULA, >\n"); break;
        case TOKEN_VIRGULA:          printf("<VIRGULA, >\n"); break;
        case TOKEN_ABRE_PARENTESES:  printf("<ABRE_PARENTESES, >\n"); break;
        case TOKEN_FECHA_PARENTESES: printf("<FECHA_PARENTESES, >\n"); break;
        case TOKEN_ABRE_CHAVES:      printf("<ABRE_CHAVES, >\n"); break;
        case TOKEN_FECHA_CHAVES:     printf("<FECHA_CHAVES, >\n"); break;
        case TOKEN_EOF:           printf("<EOF, >\n"); break;
        case TOKEN_ERRO:          printf("<ERRO, Caractere '%s' inválido>\n",
                                         token.atributo.valor_literal); break;
        default:
            printf("<DESCONHECIDO, %d>\n", token.nome_token); 
        break;
    }
}

void avancar() 
{
    if (g_lexer.atual == '\n') 
    {
        g_lexer.linha++;
        g_lexer.coluna = 1;
    } else {
        g_lexer.coluna++;
    }
    g_lexer.posicao++;
    g_lexer.atual = (g_lexer.posicao >= (int)strlen(g_lexer.fonte))
                    ? '\0'
                    : g_lexer.fonte[g_lexer.posicao];
}

char* ler_arquivo_fonte(const char* caminho)
{
    FILE* arquivo = fopen(caminho, "r");
    if(!arquivo)
    {
        fprintf(stderr, "ERRO: nao foi possivel abrir o arquivo '%s'.\n", caminho);
        return NULL; 
    }

    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);

    char* buffer = (char*)malloc(tamanho + 1);
    if(!buffer)
    {
        fprintf(stderr, "ERRO: nao foi possivel alocar memoria.\n");
        fclose(arquivo);
        return NULL; 
    }

    size_t bytes_lidos = fread(buffer, 1, tamanho, arquivo);
    buffer[bytes_lidos] = '\0';
    fclose(arquivo);
    return buffer;
}

char peek() 
{
    if(g_lexer.posicao + 1 >= (int)strlen(g_lexer.fonte))
    {
        return '\0';
    }
    return g_lexer.fonte[g_lexer.posicao + 1];
}

/* impressão de linha de erro */

void imprimir_linha_erro(const char* fonte_completa, int linha_num, int coluna_num) {
    const char* p = fonte_completa;
    int linha_atual = 1;

    while (linha_atual < linha_num && *p != '\0') {
        if (*p == '\n') {
            linha_atual++;
        }
        p++;
    }

    printf("\nErro linha %d, coluna %d:\n", linha_num, coluna_num);
    printf("  > ");
    while (*p != '\n' && *p != '\0') {
        printf("%c", *p);
        p++;
    }
    printf("\n");

    printf("    ");
    for (int i = 1; i < coluna_num; i++) {
        printf(" ");
    }
    printf("^\n");
}
