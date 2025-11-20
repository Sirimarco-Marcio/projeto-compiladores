#include "analisador_sintatico.h"
#include "../Analisador Lexico/analisador_lexico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token token_atual;
static const char* g_codigo_fonte;
static bool houve_erro_sintatico = false;

// --- PROTÓTIPO ATUALIZADO ---
// Agora 'consumir' também recebe o contexto de 'follow'
static void consumir(int token_esperado, int follow_set[], int size);
// ------------------------------

static void erro_sintatico(const char* mensagem, int follow_set[], int size);

static void programa();
static void decls();
static void decl();
static void tipo();
static void comandos();
static void comando();
static void atribuicao();
static void chamada();
static void args();   
static void expr_list();
static void entrada();   
static void saida();     
static void if_stmt();   
static void else_opt();  
static void while_stmt();
static void bloco();     
static void expr();
static void expr_linha();
static void term();
static void term_linha();
static void factor();
static int in(int token, int set[], int size);

static int in(int token, int set[], int size) {
    for (int i = 0; i < size; i++) {
        if (token == set[i]) return 1;
    }
    return 0;
}


// continua sendo estática lá em cima
static void consumir(int token_esperado, int follow_set[], int size) {
    // caso normal: bateu o token esperado, só consome
    if (token_atual.nome_token == token_esperado) {
        token_atual = get_token();
        return;
    }

    // 1) Caso de INSERÇÃO de token:
    //    o token atual já é um dos possíveis "seguidores"
    //    => assumimos que o token esperado está faltando.
    if (in(token_atual.nome_token, follow_set, size)) {
        imprimir_linha_erro(g_lexer.fonte, token_atual.linha, token_atual.coluna);
        printf("ERRO SINTÁTICO: esperado token %d antes do token %d (recuperação por inserção).\n",
               token_esperado, token_atual.nome_token);
        houve_erro_sintatico = true;

        // MUITO IMPORTANTE: NÃO consome token_atual.
        // Apenas "finge" que o token_esperado existia no input.
        return;
    }

    // 2) Caso de modo pânico de verdade:
    //    nem é o esperado, nem é algo que poderia vir depois.
    //    Chamamos o erro_sintatico para descartar tokens.
    char msg[100];
    sprintf(msg, "Esperava token %d", token_esperado);
    erro_sintatico(msg, follow_set, size);
}

// ---------------------------

static void erro_sintatico(const char* mensagem, int follow_set[], int size) {
    imprimir_linha_erro(g_lexer.fonte, token_atual.linha, token_atual.coluna);
    printf("ERRO SINTÁTICO: %s.\n", mensagem);
    printf("  Token recebido: %d (inesperado)\n", token_atual.nome_token);

    houve_erro_sintatico = true;

    // se já estamos no EOF, não há o que sincronizar
    if (token_atual.nome_token == TOKEN_EOF) {
        printf("  Fim de arquivo atingido durante recuperação.\n");
        return;
    }

    printf("  Iniciando modo pânico... Sincronizando com { ");
    for (int i = 0; i < size; i++) printf("%d ", follow_set[i]);
    printf("}\n");

    // DESCARTA pelo menos um token para não travar em loop
    token_atual = get_token();

    // agora joga fora até achar um de sincronização ou EOF
    while (token_atual.nome_token != TOKEN_EOF &&
           !in(token_atual.nome_token, follow_set, size)) {
        token_atual = get_token();
    }

    if (token_atual.nome_token != TOKEN_EOF) {
        printf("  Sincronização encontrada. Continuando análise no token %d.\n",
               token_atual.nome_token);
    } else {
        printf("  Fim de arquivo alcançado durante recuperação.\n");
    }
}


bool analisar(const char* codigo_fonte) 
{
    g_codigo_fonte = codigo_fonte; // Salva o ponteiro para os erros
    houve_erro_sintatico = false; // Reseta a flag de erro

    // Inicializa o léxico
    g_lexer.fonte = g_codigo_fonte;
    g_lexer.posicao = 0;
    g_lexer.atual = g_codigo_fonte[0];
    g_lexer.linha = 1;
    g_lexer.coluna = 1;

    // Carrega o primeiro token
    token_atual = get_token();

    // Inicia a análise pela regra inicial
    programa();

    // Verifica se parou no EOF e se não houve lixo depois
    if (!houve_erro_sintatico && token_atual.nome_token != TOKEN_EOF) {
        // Erro: Lixo depois do 'fim'
        imprimir_linha_erro(g_codigo_fonte, token_atual.linha, token_atual.coluna);
        fprintf(stderr, "ERRO: Código encontrado após a declaração 'fim'.\n");
        houve_erro_sintatico = true;
    }

    // Retorna true se NÃO houve erros
    return !houve_erro_sintatico;
}

static void programa() {
    int first_programa[] = FIRST_programa;
    int follow_programa[] = FOLLOW_programa;
    int size = sizeof(follow_programa)/sizeof(follow_programa[0]); // Pega o tamanho do follow
    
    if (!in(token_atual.nome_token, first_programa, sizeof(first_programa)/sizeof(first_programa[0]))) {
        erro_sintatico("Esperado 'inicio'", follow_programa, size);
    }
    
    // ATUALIZADO: Passa o 'follow_set' e 'size'
    consumir(TOKEN_INICIO, follow_programa, size);
    decls();
    comandos();
    consumir(TOKEN_FIM, follow_programa, size);
}

static void decls() {
    int first_decls[] = FIRST_decls;
    int follow_decls[] = FOLLOW_decls;
    
    // Regra: <decls> ::= <decl> <decls>
    if (in(token_atual.nome_token, first_decls, sizeof(first_decls)/sizeof(first_decls[0]))) {
        decl();
        decls();
    }
    // Regra: <decls> ::= ε (vazio)
    else if (!in(token_atual.nome_token, follow_decls, sizeof(follow_decls)/sizeof(follow_decls[0]))) {
        erro_sintatico("Esperado declaracao de variavel ou inicio dos comandos", 
                       follow_decls, sizeof(follow_decls)/sizeof(follow_decls[0]));
    }
}

static void decl() {
    // 'decl' é chamado por 'decls', então seu 'follow' relevante é FOLLOW_decl
    int follow_decl[] = FOLLOW_decl;
    int size = sizeof(follow_decl)/sizeof(follow_decl[0]);
    
    tipo();
    // ATUALIZADO: Passa o 'follow_set' e 'size'
    consumir(TOKEN_IDENTIFICADOR, follow_decl, size);
    consumir(TOKEN_PONTO_VIRGULA, follow_decl, size);
}

static void tipo() {
    int first_tipo[] = FIRST_tipo;
    int follow_tipo[] = FOLLOW_tipo;
    int size = sizeof(follow_tipo)/sizeof(follow_tipo[0]);

    if (token_atual.nome_token == TOKEN_TIPO_INT) {
        consumir(TOKEN_TIPO_INT, follow_tipo, size);
    } else if (token_atual.nome_token == TOKEN_TIPO_FLOAT) {
        consumir(TOKEN_TIPO_FLOAT, follow_tipo, size);
    } else if (token_atual.nome_token == TOKEN_TIPO_STRING) {
        consumir(TOKEN_TIPO_STRING, follow_tipo, size);
    } else {
        erro_sintatico("Esperado um tipo (int, float, string)", follow_tipo, size);
    }
}

static void comandos() {
    int first_comandos[] = FIRST_comandos;
    int follow_comandos[] = FOLLOW_comandos;

    // Regra: <comandos> ::= <comando> <comandos>
    if (in(token_atual.nome_token, first_comandos, sizeof(first_comandos)/sizeof(first_comandos[0]))) {
        comando();
        comandos();
    }
    // Regra: <comandos> ::= ε (vazio)
    else if (!in(token_atual.nome_token, follow_comandos, sizeof(follow_comandos)/sizeof(follow_comandos[0]))) {
        erro_sintatico("Esperado um comando ou 'fim'", 
                       follow_comandos, sizeof(follow_comandos)/sizeof(follow_comandos[0]));
    }
}

static void comando() {
    int first_comando[] = FIRST_comando;
    int follow_comando[] = FOLLOW_comando;
    int size = sizeof(follow_comando)/sizeof(follow_comando[0]);
    
    if (!in(token_atual.nome_token, first_comando, sizeof(first_comando)/sizeof(first_comando[0]))) {
        erro_sintatico("Esperado comando (ID, read, print, if, while ou {)", follow_comando, size);
        return;
    }
    
    if (token_atual.nome_token == TOKEN_IDENTIFICADOR) {
        Token lookahead = get_token();
        
        if (lookahead.nome_token == TOKEN_ATRIBUICAO) {
            devolver_token(lookahead);
            atribuicao();
            consumir(TOKEN_PONTO_VIRGULA, follow_comando, size); // ATUALIZADO
        } 
        else if (lookahead.nome_token == TOKEN_ABRE_PARENTESES) {
            devolver_token(lookahead);
            chamada();
            consumir(TOKEN_PONTO_VIRGULA, follow_comando, size); // ATUALIZADO
        }
        else {
            devolver_token(lookahead);
            erro_sintatico("Depois do ID esperava '=' ou '('", follow_comando, size);
        }
    }
    else if (token_atual.nome_token == TOKEN_READ) {
        entrada();
        consumir(TOKEN_PONTO_VIRGULA, follow_comando, size); // ATUALIZADO
    }
    else if (token_atual.nome_token == TOKEN_PRINT) {
        saida();
        consumir(TOKEN_PONTO_VIRGULA, follow_comando, size); // ATUALIZADO
    }
    else if (token_atual.nome_token == TOKEN_IF) {
        if_stmt();
    }
    else if (token_atual.nome_token == TOKEN_WHILE) {
        while_stmt();
    }
    else if (token_atual.nome_token == TOKEN_ABRE_CHAVES) {
        bloco();
    }
}

static void atribuicao() {
    int first_atribuicao[] = FIRST_atribuicao;
    int follow_atribuicao[] = FOLLOW_atribuicao;
    int size = sizeof(follow_atribuicao)/sizeof(follow_atribuicao[0]);
    
    if (!in(token_atual.nome_token, first_atribuicao, sizeof(first_atribuicao)/sizeof(first_atribuicao[0]))) {
        erro_sintatico("Esperado ID para atribuicao", follow_atribuicao, size);
        return;
    }
    
    consumir(TOKEN_IDENTIFICADOR, follow_atribuicao, size); // ATUALIZADO
    consumir(TOKEN_ATRIBUICAO, follow_atribuicao, size); // ATUALIZADO
    expr();
}

static void chamada() {
    int first_chamada[] = FIRST_chamada;
    int follow_chamada[] = FOLLOW_chamada;
    int size = sizeof(follow_chamada)/sizeof(follow_chamada[0]);
    
    if (!in(token_atual.nome_token, first_chamada, sizeof(first_chamada)/sizeof(first_chamada[0]))) {
        erro_sintatico("Esperado ID para chamada", follow_chamada, size);
        return;
    }
    
    consumir(TOKEN_IDENTIFICADOR, follow_chamada, size); // ATUALIZADO
    consumir(TOKEN_ABRE_PARENTESES, follow_chamada, size); // ATUALIZADO
    args();
    consumir(TOKEN_FECHA_PARENTESES, follow_chamada, size); // ATUALIZADO
}

static void args() {
    int first_args[] = FIRST_args;
    int follow_args[] = FOLLOW_args;
    
    if (in(token_atual.nome_token, first_args, sizeof(first_args)/sizeof(first_args[0]))) {
        expr_list();
    }
    // ε - não faz nada
}

static void expr_list() {
    int first_expr_list[] = FIRST_expr_list;
    int follow_expr_list[] = FOLLOW_expr_list;
    int size = sizeof(follow_expr_list)/sizeof(follow_expr_list[0]);
    
    if (!in(token_atual.nome_token, first_expr_list, sizeof(first_expr_list)/sizeof(first_expr_list[0]))) {
        erro_sintatico("Esperado expressao para lista de argumentos", follow_expr_list, size);
        return;
    }
    
    expr();
    
    if (token_atual.nome_token == TOKEN_VIRGULA) {
        consumir(TOKEN_VIRGULA, follow_expr_list, size); // ATUALIZADO
        expr_list();
    }
}

static void entrada() {
    int first_entrada[] = FIRST_entrada;
    int follow_entrada[] = FOLLOW_entrada;
    int size = sizeof(follow_entrada)/sizeof(follow_entrada[0]);
    
    if (!in(token_atual.nome_token, first_entrada, sizeof(first_entrada)/sizeof(first_entrada[0]))) {
        erro_sintatico("Esperado 'read'", follow_entrada, size);
        return;
    }
    
    consumir(TOKEN_READ, follow_entrada, size); // ATUALIZADO
    consumir(TOKEN_ABRE_PARENTESES, follow_entrada, size); // ATUALIZADO
    consumir(TOKEN_IDENTIFICADOR, follow_entrada, size); // ATUALIZADO
    consumir(TOKEN_FECHA_PARENTESES, follow_entrada, size); // ATUALIZADO
}

static void saida() {
    int first_saida[] = FIRST_saida;
    int follow_saida[] = FOLLOW_saida;
    int size = sizeof(follow_saida)/sizeof(follow_saida[0]);
    
    if (!in(token_atual.nome_token, first_saida, sizeof(first_saida)/sizeof(first_saida[0]))) {
        erro_sintatico("Esperado 'print'", follow_saida, size);
        return;
    }
    
    consumir(TOKEN_PRINT, follow_saida, size); // ATUALIZADO
    consumir(TOKEN_ABRE_PARENTESES, follow_saida, size); // ATUALIZADO
    expr();
    consumir(TOKEN_FECHA_PARENTESES, follow_saida, size); // ATUALIZADO
}

static void if_stmt() {
    int first_if_stmt[] = FIRST_if_stmt;
    int follow_if_stmt[] = FOLLOW_if_stmt;
    int size = sizeof(follow_if_stmt)/sizeof(follow_if_stmt[0]);
    
    if (!in(token_atual.nome_token, first_if_stmt, sizeof(first_if_stmt)/sizeof(first_if_stmt[0]))) {
        erro_sintatico("Esperado 'if'", follow_if_stmt, size);
        return;
    }
    
    consumir(TOKEN_IF, follow_if_stmt, size); // ATUALIZADO
    consumir(TOKEN_ABRE_PARENTESES, follow_if_stmt, size); // ATUALIZADO
    expr();
    consumir(TOKEN_FECHA_PARENTESES, follow_if_stmt, size); // ATUALIZADO
    comando();
    else_opt();
}

static void else_opt() {
    int first_else_opt[] = FIRST_else_opt;
    int follow_else_opt[] = FOLLOW_else_opt;
    int size = sizeof(follow_else_opt)/sizeof(follow_else_opt[0]);
    
    if (token_atual.nome_token == TOKEN_ELSE) {
        consumir(TOKEN_ELSE, follow_else_opt, size); // ATUALIZADO
        comando();
    }
    // ε - não faz nada
}

static void while_stmt() {
    int first_while_stmt[] = FIRST_while_stmt;
    int follow_while_stmt[] = FOLLOW_while_stmt;
    int size = sizeof(follow_while_stmt)/sizeof(follow_while_stmt[0]);
    
    if (!in(token_atual.nome_token, first_while_stmt, sizeof(first_while_stmt)/sizeof(first_while_stmt[0]))) {
        erro_sintatico("Esperado 'while'", follow_while_stmt, size);
        return;
    }
    
    consumir(TOKEN_WHILE, follow_while_stmt, size); // ATUALIZADO
    consumir(TOKEN_ABRE_PARENTESES, follow_while_stmt, size); // ATUALIZADO
    expr();
    consumir(TOKEN_FECHA_PARENTESES, follow_while_stmt, size); // ATUALIZADO
    comando();
}

static void bloco() {
    int first_bloco[] = FIRST_bloco;
    int follow_bloco[] = FOLLOW_bloco;
    int size = sizeof(follow_bloco)/sizeof(follow_bloco[0]);
    
    if (!in(token_atual.nome_token, first_bloco, sizeof(first_bloco)/sizeof(first_bloco[0]))) {
        erro_sintatico("Esperado '{' para bloco", follow_bloco, size);
        return;
    }
    
    consumir(TOKEN_ABRE_CHAVES, follow_bloco, size); // ATUALIZADO
    comandos();
    consumir(TOKEN_FECHA_CHAVES, follow_bloco, size); // ATUALIZADO
}

static void expr() {
    int first_expr[] = FIRST_expr;
    int follow_expr[] = FOLLOW_expr;
    
    if (!in(token_atual.nome_token, first_expr, sizeof(first_expr)/sizeof(first_expr[0]))) {
        erro_sintatico("Esperado expressao", follow_expr, sizeof(follow_expr)/sizeof(follow_expr[0]));
        return;
    }
    
    term();
    expr_linha();
}

static void expr_linha() {
    int first_expr_linha[] = FIRST_expr_linha;
    int follow_expr_linha[] = FOLLOW_expr_linha;
    int size = sizeof(follow_expr_linha)/sizeof(follow_expr_linha[0]);
    
    if (token_atual.nome_token == TOKEN_OPERADOR_SOMA || 
        token_atual.nome_token == TOKEN_OPERADOR_SUB) {
        consumir(token_atual.nome_token, follow_expr_linha, size); // ATUALIZADO
        term();
        expr_linha();
    }
    // ε - não faz nada
}

static void term() {
    int first_term[] = FIRST_term;
    int follow_term[] = FOLLOW_term;
    
    if (!in(token_atual.nome_token, first_term, sizeof(first_term)/sizeof(first_term[0]))) {
        erro_sintatico("Esperado termo", follow_term, sizeof(follow_term)/sizeof(follow_term[0]));
        return;
    }
    
    factor();
    term_linha();
}

static void term_linha() {
    int first_term_linha[] = FIRST_term_linha;
    int follow_term_linha[] = FOLLOW_term_linha;
    int size = sizeof(follow_term_linha)/sizeof(follow_term_linha[0]);
    
    if (token_atual.nome_token == TOKEN_OPERADOR_MUL || 
        token_atual.nome_token == TOKEN_OPERADOR_DIV) {
        consumir(token_atual.nome_token, follow_term_linha, size); // ATUALIZADO
        factor();
        term_linha();
    }
    // ε - não faz nada
}

static void factor() {
    int first_factor[] = FIRST_factor;
    int follow_factor[] = FOLLOW_factor;
    int size = sizeof(follow_factor)/sizeof(follow_factor[0]);
    
    if (!in(token_atual.nome_token, first_factor, sizeof(first_factor)/sizeof(first_factor[0]))) {
        erro_sintatico("Esperado fator (ID, NUMERO, STRING, '(', '-')", follow_factor, size);
        return;
    }
    
    if (token_atual.nome_token == TOKEN_IDENTIFICADOR) {
        consumir(TOKEN_IDENTIFICADOR, follow_factor, size); // ATUALIZADO
    }
    else if (token_atual.nome_token == TOKEN_NUMERO) {
        consumir(TOKEN_NUMERO, follow_factor, size); // ATUALIZADO
    }
    else if (token_atual.nome_token == TOKEN_STRING) {
        consumir(TOKEN_STRING, follow_factor, size); // ATUALIZADO
    }
    else if (token_atual.nome_token == TOKEN_ABRE_PARENTESES) {
        consumir(TOKEN_ABRE_PARENTESES, follow_factor, size); // ATUALIZADO
        expr();
        consumir(TOKEN_FECHA_PARENTESES, follow_factor, size); // ATUALIZADO
    }
    else if (token_atual.nome_token == TOKEN_OPERADOR_SUB) {
        consumir(TOKEN_OPERADOR_SUB, follow_factor, size); // ATUALIZADO
        factor();
    }
}