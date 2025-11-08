#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>


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

//Estados
#define STATE_INICIAL          0
#define STATE_EM_IDENTIFICADOR 1
#define STATE_EM_NUMERO        2
#define STATE_EM_COMENTARIO    3
#define STATE_EM_STRING        4


typedef struct
{
    int nome_token;
    union
    {
        int pos_simbolo;
        char valor_literal[1024];
    } atributo;
} Token;


#define MAX_ID_LEN 32
#define INCREMENTO_TABELA 32 //Quantos novos espaços alocar quando a tabela encher

typedef struct 
{
    char lexema[MAX_ID_LEN + 1];
} Simbolo;

Simbolo* tabela_de_simbolos = NULL; 
int tamanho_tabela = 0;             
int capacidade_tabela = 0;          

typedef struct
{
    const char* fonte;
    int posicao;
    char atual;
} AnalisadorLexico;

AnalisadorLexico g_lexer;



void avancar();
char* ler_arquivo_fonte(const char* caminho);
Token get_token(void);
int instalar_id(const char* lexema);
void imprimir_token(Token token);
char peek(void);


//Mapa de palavras chave 
//"Dicionario"
typedef struct
{
    const char* nome;
    int token;
} PalavraChave;


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


//check ✓
int instalar_id(const char* lexema)
{
    // Busca se o ID já existe
    for(int i = 0; i < tamanho_tabela; i++) 
    {
        if(strcmp(tabela_de_simbolos[i].lexema, lexema) == 0) return i;
    }

    if(tamanho_tabela >= capacidade_tabela) 
    {
        int nova_capacidade = capacidade_tabela + INCREMENTO_TABELA;
        
        Simbolo* nova_tabela = (Simbolo*) realloc(tabela_de_simbolos, nova_capacidade * sizeof(Simbolo));

        if(nova_tabela == NULL)
        {
            printf("ERRO FATAL: Falha ao realocar a tabela de símbolos!\n");
            exit(1);
        }

        tabela_de_simbolos = nova_tabela;
        capacidade_tabela = nova_capacidade;
        //printf("\n[INFO: Tabela de símbolos realocada para %d posições]\n", capacidade_tabela);
    }
    
    //Insere o novo símbolo na primeira posição livre
    strncpy(tabela_de_simbolos[tamanho_tabela].lexema, lexema, MAX_ID_LEN);
    tabela_de_simbolos[tamanho_tabela].lexema[MAX_ID_LEN] = '\0';
    return tamanho_tabela++;
}


//check ✓
Token get_token(void) 
{
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
                    //Continua lendo o ID para o buffer temporário,
                    //limite do buffer
                    if(lexeme_pos < 1023) 
                    {
                        lexeme_buffer[lexeme_pos++] = g_lexer.atual;
                    }
                    avancar();

                } else
                {
                    lexeme_buffer[lexeme_pos] = '\0';

                    if(strlen(lexeme_buffer) > MAX_ID_LEN)
                    {
                        token.nome_token = TOKEN_ERRO;
                        char msg_erro[256];
                        sprintf(msg_erro, "ID excede o limite de %d caracteres: %.35s...", MAX_ID_LEN, lexeme_buffer);
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
                } else
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
                    
                    sprintf(msg_erro, "String nao finalizada antes do fim do arquivo: \"%.35s", lexeme_buffer);
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

                // Comentário longo
                if(g_lexer.atual == '[' && peek() == '[') 
                {
                    avancar();
                    avancar();

                    while(g_lexer.atual != '\0' && !(g_lexer.atual == ']' && peek() == ']')) 
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
                // Comentário curto
                else
                {
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

//check ✓
int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Uso: %s <arquivo_fonte.txt>\n", argv[0]); 
        return 1; 
    }

    char* codigo_fonte = ler_arquivo_fonte(argv[1]);

    if(codigo_fonte == NULL) 
    {
        return 1;
    }

    g_lexer.fonte = codigo_fonte;
    g_lexer.posicao = 0;
    g_lexer.atual = codigo_fonte[0];

    while(1)
    {
        Token token = get_token();
        imprimir_token(token);

        if(token.nome_token == TOKEN_EOF || token.nome_token == TOKEN_ERRO)
        {
            break;
        }
    }

    printf("\n\n--- Tabela de Símbolos ---\n");
    printf("Índice  | Lexema\n");
    printf("---------------------------\n");
    for(int i = 0; i < tamanho_tabela; i++)
    {
        printf("%-7d | %s\n", i, tabela_de_simbolos[i].lexema);
    }
    printf("---------------------------\n");

    free(codigo_fonte);
    free(tabela_de_simbolos);
    
    return 0;
}

//check ✓
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
        case TOKEN_ERRO:          printf("<ERRO, Caractere '%s' inválido>\n", token.atributo.valor_literal); break;

        default:
            printf("<DESCONHECIDO, %d>\n", token.nome_token); 
        break;
    }
}


//check ✓
void avancar() 
{
    g_lexer.posicao++;
    g_lexer.atual = (g_lexer.posicao >= strlen(g_lexer.fonte)) ? '\0' : g_lexer.fonte[g_lexer.posicao];
}


//check ✓
char* ler_arquivo_fonte(const char* caminho)
{

    FILE* arquivo = fopen(caminho, "r");
    if(!arquivo)
    {
        fprintf(stderr, "ERRO: nao foi possivel abrir o arquivo '%s'.\n", caminho); return NULL; 
    }

    fseek(arquivo, 0, SEEK_END); long tamanho = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);

    char* buffer = (char*)malloc(tamanho + 1);
    if(!buffer)
    {
        fprintf(stderr, "ERRO: nao foi possivel alocar memoria.\n"); fclose(arquivo); return NULL; 
    }

    size_t bytes_lidos = fread(buffer, 1, tamanho, arquivo);
    buffer[bytes_lidos] = '\0';
    fclose(arquivo);
    return buffer;
}

char peek() 
{
    if(g_lexer.posicao + 1 >= strlen(g_lexer.fonte))
    {
        return '\0';
    }

    return g_lexer.fonte[g_lexer.posicao + 1];
}