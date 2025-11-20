#include "analisador_sintatico.h"
#include "../Analisador Lexico/analisador_lexico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token token_atual;
static const char* g_codigo_fonte;

static bool houve_erro_sintatico  = false;
static bool houve_erro_semantico  = false;
static bool semantica_ativa       = true;

/* ========= PROTÓTIPOS ========= */

static void consumir(int token_esperado, int follow_set[], int size);
static void erro_sintatico(const char* mensagem, int follow_set[], int size);

/* sintaxe */
static void programa();
static void decls();
static void decl();
static TipoSimbolo tipo();
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
static TipoSimbolo expr();
static TipoSimbolo expr_linha(TipoSimbolo h);
static TipoSimbolo term();
static TipoSimbolo term_linha(TipoSimbolo h);
static TipoSimbolo factor();
static int in(int token, int set[], int size);

/* semântica */
static void  erro_semantico(const char* mensagem, Token tk);
static void  registrar_declaracao(Token id_token, TipoSimbolo tipo);
static TipoSimbolo tipo_de_variavel(Token id_token);
static TipoSimbolo tipo_do_numero(Token num_token);
static int   eh_numerico(TipoSimbolo t);
static TipoSimbolo tipo_resultado_binario(TipoSimbolo left,
                                          TipoSimbolo right,
                                          Token op_token);
static int tipos_atribuicao_compativeis(TipoSimbolo dest, TipoSimbolo src);
static const char* nome_tipo(TipoSimbolo t);
static void checar_condicao(TipoSimbolo t, Token kw_token);

/* ========= FUNÇÕES AUXILIARES ========= */

static int in(int token, int set[], int size) {
    for (int i = 0; i < size; i++) {
        if (token == set[i]) return 1;
    }
    return 0;
}

/* consumir com inserção + modo pânico */
static void consumir(int token_esperado, int follow_set[], int size) {
    if (token_atual.nome_token == token_esperado) {
        token_atual = get_token();
        return;
    }

    /* caso de INSERÇÃO: token atual já é um símbolo de sincronização */
    if (in(token_atual.nome_token, follow_set, size)) {
        imprimir_linha_erro(g_lexer.fonte, token_atual.linha, token_atual.coluna);
        printf("ERRO SINTÁTICO: esperado token %d antes do token %d (recuperação por inserção).\n",
               token_esperado, token_atual.nome_token);

        houve_erro_sintatico = true;
        semantica_ativa      = false; /* desativa análise semântica a partir daqui */
        return;
    }

    /* caso geral: modo pânico */
    char msg[100];
    sprintf(msg, "Esperava token %d", token_esperado);
    erro_sintatico(msg, follow_set, size);
}

static void erro_sintatico(const char* mensagem, int follow_set[], int size) {
    imprimir_linha_erro(g_lexer.fonte, token_atual.linha, token_atual.coluna);
    printf("ERRO SINTÁTICO: %s.\n", mensagem);
    printf("  Token recebido: %d (inesperado)\n", token_atual.nome_token);

    houve_erro_sintatico = true;
    semantica_ativa      = false; /* semântica não deve mais rodar */

    if (token_atual.nome_token == TOKEN_EOF) {
        printf("  Fim de arquivo atingido durante recuperação.\n");
        return;
    }

    printf("  Iniciando modo pânico... Sincronizando com { ");
    for (int i = 0; i < size; i++) printf("%d ", follow_set[i]);
    printf("}\n");

    /* descarta pelo menos um token */
    token_atual = get_token();

    /* descarta até encontrar um token do conjunto de sincronização */
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

/* ========= FUNÇÕES DE ERRO SEMÂNTICO ========= */

static void erro_semantico(const char* mensagem, Token tk) {
    if (!semantica_ativa) return;

    imprimir_linha_erro(g_lexer.fonte, tk.linha, tk.coluna);
    printf("ERRO SEMÂNTICO: %s.\n", mensagem);
    houve_erro_semantico = true;
}

/* registro de variáveis na tabela de símbolos */
static void registrar_declaracao(Token id_token, TipoSimbolo tipo_var) {
    if (!semantica_ativa) return;

    int idx = id_token.atributo.pos_simbolo;
    if (idx < 0 || idx >= tamanho_tabela) return;

    Simbolo* s = &tabela_de_simbolos[idx];

    if (s->tipo == TIPO_INDEFINIDO || s->tipo == TIPO_ERRO) {
        s->tipo      = tipo_var;
        s->categoria = CAT_VARIAVEL;
    } else {
        char msg[256];
        sprintf(msg, "Variável '%s' declarada mais de uma vez", s->lexema);
        erro_semantico(msg, id_token);
    }
}

/* tipo de uma variável (verifica se foi declarada) */
static TipoSimbolo tipo_de_variavel(Token id_token) {
    if (!semantica_ativa) return TIPO_ERRO;

    int idx = id_token.atributo.pos_simbolo;
    if (idx < 0 || idx >= tamanho_tabela) return TIPO_ERRO;

    Simbolo* s = &tabela_de_simbolos[idx];

    if (s->tipo == TIPO_INDEFINIDO) {
        char msg[256];
        sprintf(msg, "Variável '%s' não declarada", s->lexema);
        erro_semantico(msg, id_token);
        return TIPO_ERRO;
    }

    return s->tipo;
}

/* tipo de um literal NUMERO */
static TipoSimbolo tipo_do_numero(Token num_token) {
    if (!semantica_ativa) return TIPO_INT_S;

    if (strchr(num_token.atributo.valor_literal, '.') != NULL)
        return TIPO_FLOAT_S;
    else
        return TIPO_INT_S;
}

static int eh_numerico(TipoSimbolo t) {
    return (t == TIPO_INT_S || t == TIPO_FLOAT_S);
}

/* tipo resultante de uma operação binária */
static TipoSimbolo tipo_resultado_binario(TipoSimbolo left,
                                          TipoSimbolo right,
                                          Token op_token) {
    if (!semantica_ativa) return TIPO_ERRO;

    if (left == TIPO_ERRO || right == TIPO_ERRO)
        return TIPO_ERRO;

    if (!eh_numerico(left) || !eh_numerico(right)) {
        char msg[256];
        sprintf(msg, "Operador '%c' só é permitido entre números",
                (char)op_token.nome_token);
        erro_semantico(msg, op_token);
        return TIPO_ERRO;
    }

    if (left == TIPO_FLOAT_S || right == TIPO_FLOAT_S)
        return TIPO_FLOAT_S;
    return TIPO_INT_S;
}

static int tipos_atribuicao_compativeis(TipoSimbolo dest, TipoSimbolo src) {
    if (dest == TIPO_ERRO || src == TIPO_ERRO) return 1;

    if (dest == src) return 1;

    /* coerção implícita permitida: float = int */
    if (dest == TIPO_FLOAT_S && src == TIPO_INT_S) return 1;

    /* qualquer outra combinação é inválida (inclusive int = float) */
    return 0;
}

static const char* nome_tipo(TipoSimbolo t) {
    switch (t) {
        case TIPO_INT_S:      return "int";
        case TIPO_FLOAT_S:    return "float";
        case TIPO_STRING_S:   return "string";
        case TIPO_INDEFINIDO: return "indefinido";
        case TIPO_ERRO:       return "erro";
        default:              return "?";
    }
}

static void checar_condicao(TipoSimbolo t, Token kw_token) {
    if (!semantica_ativa) return;
    if (t == TIPO_ERRO)    return;

    if (!eh_numerico(t)) {
        char msg[256];
        const char* kw = (kw_token.nome_token == TOKEN_IF) ? "if" : "while";
        sprintf(msg, "Expressão de condição do '%s' deve ser numérica", kw);
        erro_semantico(msg, kw_token);
    }
}

/* ========= FUNÇÃO PRINCIPAL ========= */

bool analisar(const char* codigo_fonte) 
{
    g_codigo_fonte      = codigo_fonte;
    houve_erro_sintatico = false;
    houve_erro_semantico = false;
    semantica_ativa      = true;

    g_lexer.fonte   = g_codigo_fonte;
    g_lexer.posicao = 0;
    g_lexer.atual   = g_codigo_fonte[0];
    g_lexer.linha   = 1;
    g_lexer.coluna  = 1;
    g_lexer.tem_token_devolvido = 0;

    token_atual = get_token();

    programa();

    if (!houve_erro_sintatico && token_atual.nome_token != TOKEN_EOF) {
        imprimir_linha_erro(g_codigo_fonte, token_atual.linha, token_atual.coluna);
        fprintf(stderr, "ERRO: Código encontrado após a declaração 'fim'.\n");
        houve_erro_sintatico = true;
    }

    return !houve_erro_sintatico && !houve_erro_semantico;
}

/* ========= REGRAS DA GRAMÁTICA ========= */

static void programa() {
    int first_programa[]  = FIRST_programa;
    int follow_programa[] = FOLLOW_programa;
    int size = sizeof(follow_programa)/sizeof(follow_programa[0]);

    if (!in(token_atual.nome_token, first_programa,
            sizeof(first_programa)/sizeof(first_programa[0]))) {
        erro_sintatico("Esperado 'inicio'", follow_programa, size);
    }

    consumir(TOKEN_INICIO, follow_programa, size);
    decls();
    comandos();
    consumir(TOKEN_FIM, follow_programa, size);
}

static void decls() {
    int first_decls[]  = FIRST_decls;
    int follow_decls[] = FOLLOW_decls;

    if (in(token_atual.nome_token, first_decls,
           sizeof(first_decls)/sizeof(first_decls[0]))) {
        decl();
        decls();
    }
    else if (!in(token_atual.nome_token, follow_decls,
                 sizeof(follow_decls)/sizeof(follow_decls[0]))) {
        erro_sintatico("Esperado declaracao de variavel ou inicio dos comandos",
                       follow_decls,
                       sizeof(follow_decls)/sizeof(follow_decls[0]));
    }
}

static void decl() {
    int follow_decl[] = FOLLOW_decl;
    int size = sizeof(follow_decl)/sizeof(follow_decl[0]);

    TipoSimbolo t = tipo();

    Token id_token = token_atual;
    consumir(TOKEN_IDENTIFICADOR, follow_decl, size);

    /* semântica: registrar tipo da variável */
    registrar_declaracao(id_token, t);

    consumir(TOKEN_PONTO_VIRGULA, follow_decl, size);
}

static TipoSimbolo tipo() {
    int follow_tipo[] = FOLLOW_tipo;
    int size = sizeof(follow_tipo)/sizeof(follow_tipo[0]);

    if (token_atual.nome_token == TOKEN_TIPO_INT) {
        consumir(TOKEN_TIPO_INT, follow_tipo, size);
        return TIPO_INT_S;
    } else if (token_atual.nome_token == TOKEN_TIPO_FLOAT) {
        consumir(TOKEN_TIPO_FLOAT, follow_tipo, size);
        return TIPO_FLOAT_S;
    } else if (token_atual.nome_token == TOKEN_TIPO_STRING) {
        consumir(TOKEN_TIPO_STRING, follow_tipo, size);
        return TIPO_STRING_S;
    } else {
        erro_sintatico("Esperado um tipo (int, float, string)", follow_tipo, size);
        return TIPO_ERRO;
    }
}

static void comandos() {
    int first_comandos[]  = FIRST_comandos;
    int follow_comandos[] = FOLLOW_comandos;

    if (in(token_atual.nome_token, first_comandos,
           sizeof(first_comandos)/sizeof(first_comandos[0]))) {
        comando();
        comandos();
    }
    else if (!in(token_atual.nome_token, follow_comandos,
                 sizeof(follow_comandos)/sizeof(follow_comandos[0]))) {
        erro_sintatico("Esperado um comando ou 'fim'",
                       follow_comandos,
                       sizeof(follow_comandos)/sizeof(follow_comandos[0]));
    }
}

static void comando() {
    int first_comando[]  = FIRST_comando;
    int follow_comando[] = FOLLOW_comando;
    int size = sizeof(follow_comando)/sizeof(follow_comando[0]);
    
    if (!in(token_atual.nome_token, first_comando,
            sizeof(first_comando)/sizeof(first_comando[0]))) {
        erro_sintatico("Esperado comando (ID, read, print, if, while ou {)",
                       follow_comando, size);
        return;
    }
    
    if (token_atual.nome_token == TOKEN_IDENTIFICADOR) {
        Token lookahead = get_token();
        
        if (lookahead.nome_token == TOKEN_ATRIBUICAO) {
            devolver_token(lookahead);
            atribuicao();
            consumir(TOKEN_PONTO_VIRGULA, follow_comando, size);
        } 
        else if (lookahead.nome_token == TOKEN_ABRE_PARENTESES) {
            devolver_token(lookahead);
            chamada();
            consumir(TOKEN_PONTO_VIRGULA, follow_comando, size);
        }
        else {
            devolver_token(lookahead);
            erro_sintatico("Depois do ID esperava '=' ou '('",
                           follow_comando, size);
        }
    }
    else if (token_atual.nome_token == TOKEN_READ) {
        entrada();
        consumir(TOKEN_PONTO_VIRGULA, follow_comando, size);
    }
    else if (token_atual.nome_token == TOKEN_PRINT) {
        saida();
        consumir(TOKEN_PONTO_VIRGULA, follow_comando, size);
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
    int first_atribuicao[]  = FIRST_atribuicao;
    int follow_atribuicao[] = FOLLOW_atribuicao;
    int size = sizeof(follow_atribuicao)/sizeof(follow_atribuicao[0]);
    
    if (!in(token_atual.nome_token, first_atribuicao,
            sizeof(first_atribuicao)/sizeof(first_atribuicao[0]))) {
        erro_sintatico("Esperado ID para atribuicao",
                       follow_atribuicao, size);
        return;
    }
    
    Token id_token = token_atual;
    consumir(TOKEN_IDENTIFICADOR, follow_atribuicao, size);
    consumir(TOKEN_ATRIBUICAO, follow_atribuicao, size);
    TipoSimbolo t_expr = expr();

    if (semantica_ativa) {
        TipoSimbolo t_id = tipo_de_variavel(id_token);
        if (!tipos_atribuicao_compativeis(t_id, t_expr)) {
            const char* lexema =
                tabela_de_simbolos[id_token.atributo.pos_simbolo].lexema;
            char msg[256];
            sprintf(msg,
                    "Tipos incompatíveis na atribuição para '%s' "
                    "(variável: %s, expressão: %s)",
                    lexema, nome_tipo(t_id), nome_tipo(t_expr));
            erro_semantico(msg, id_token);
        }
    }
}

static void chamada() {
    int first_chamada[]  = FIRST_chamada;
    int follow_chamada[] = FOLLOW_chamada;
    int size = sizeof(follow_chamada)/sizeof(follow_chamada[0]);
    
    if (!in(token_atual.nome_token, first_chamada,
            sizeof(first_chamada)/sizeof(first_chamada[0]))) {
        erro_sintatico("Esperado ID para chamada", follow_chamada, size);
        return;
    }
    
    consumir(TOKEN_IDENTIFICADOR, follow_chamada, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_chamada, size);
    args();
    consumir(TOKEN_FECHA_PARENTESES, follow_chamada, size);
}

static void args() {
    int first_args[]  = FIRST_args;
    int follow_args[] = FOLLOW_args;
    (void)follow_args;

    if (in(token_atual.nome_token, first_args,
           sizeof(first_args)/sizeof(first_args[0]))) {
        expr_list();
    }
    /* ε */
}

static void expr_list() {
    int first_expr_list[]  = FIRST_expr_list;
    int follow_expr_list[] = FOLLOW_expr_list;
    int size = sizeof(follow_expr_list)/sizeof(follow_expr_list[0]);
    
    if (!in(token_atual.nome_token, first_expr_list,
            sizeof(first_expr_list)/sizeof(first_expr_list[0]))) {
        erro_sintatico("Esperado expressao para lista de argumentos",
                       follow_expr_list, size);
        return;
    }
    
    (void) expr();
    
    if (token_atual.nome_token == TOKEN_VIRGULA) {
        consumir(TOKEN_VIRGULA, follow_expr_list, size);
        expr_list();
    }
}

static void entrada() {
    int first_entrada[]  = FIRST_entrada;
    int follow_entrada[] = FOLLOW_entrada;
    int size = sizeof(follow_entrada)/sizeof(follow_entrada[0]);
    
    if (!in(token_atual.nome_token, first_entrada,
            sizeof(first_entrada)/sizeof(first_entrada[0]))) {
        erro_sintatico("Esperado 'read'", follow_entrada, size);
        return;
    }
    
    consumir(TOKEN_READ, follow_entrada, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_entrada, size);
    Token id_token = token_atual;
    consumir(TOKEN_IDENTIFICADOR, follow_entrada, size);

    if (semantica_ativa) {
        /* precisa ser variável declarada */
        (void) tipo_de_variavel(id_token);
    }

    consumir(TOKEN_FECHA_PARENTESES, follow_entrada, size);
}

static void saida() {
    int first_saida[]  = FIRST_saida;
    int follow_saida[] = FOLLOW_saida;
    int size = sizeof(follow_saida)/sizeof(follow_saida[0]);
    
    if (!in(token_atual.nome_token, first_saida,
            sizeof(first_saida)/sizeof(first_saida[0]))) {
        erro_sintatico("Esperado 'print'", follow_saida, size);
        return;
    }
    
    consumir(TOKEN_PRINT, follow_saida, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_saida, size);
    (void) expr();
    consumir(TOKEN_FECHA_PARENTESES, follow_saida, size);
}

static void if_stmt() {
    int first_if_stmt[]  = FIRST_if_stmt;
    int follow_if_stmt[] = FOLLOW_if_stmt;
    int size = sizeof(follow_if_stmt)/sizeof(follow_if_stmt[0]);
    
    if (!in(token_atual.nome_token, first_if_stmt,
            sizeof(first_if_stmt)/sizeof(first_if_stmt[0]))) {
        erro_sintatico("Esperado 'if'", follow_if_stmt, size);
        return;
    }
    
    Token if_token = token_atual;
    consumir(TOKEN_IF, follow_if_stmt, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_if_stmt, size);
    TipoSimbolo cond_tipo = expr();
    if (semantica_ativa) {
        checar_condicao(cond_tipo, if_token);
    }
    consumir(TOKEN_FECHA_PARENTESES, follow_if_stmt, size);
    comando();
    else_opt();
}

static void else_opt() {
    int first_else_opt[]  = FIRST_else_opt;
    int follow_else_opt[] = FOLLOW_else_opt;
    int size = sizeof(follow_else_opt)/sizeof(follow_else_opt[0]);
    (void)first_else_opt;

    if (token_atual.nome_token == TOKEN_ELSE) {
        consumir(TOKEN_ELSE, follow_else_opt, size);
        comando();
    }
}

static void while_stmt() {
    int first_while_stmt[]  = FIRST_while_stmt;
    int follow_while_stmt[] = FOLLOW_while_stmt;
    int size = sizeof(follow_while_stmt)/sizeof(follow_while_stmt[0]);
    
    if (!in(token_atual.nome_token, first_while_stmt,
            sizeof(first_while_stmt)/sizeof(first_while_stmt[0]))) {
        erro_sintatico("Esperado 'while'", follow_while_stmt, size);
        return;
    }
    
    Token while_token = token_atual;
    consumir(TOKEN_WHILE, follow_while_stmt, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_while_stmt, size);
    TipoSimbolo cond_tipo = expr();
    if (semantica_ativa) {
        checar_condicao(cond_tipo, while_token);
    }
    consumir(TOKEN_FECHA_PARENTESES, follow_while_stmt, size);
    comando();
}

static void bloco() {
    int first_bloco[]  = FIRST_bloco;
    int follow_bloco[] = FOLLOW_bloco;
    int size = sizeof(follow_bloco)/sizeof(follow_bloco[0]);
    
    if (!in(token_atual.nome_token, first_bloco,
            sizeof(first_bloco)/sizeof(first_bloco[0]))) {
        erro_sintatico("Esperado '{' para bloco", follow_bloco, size);
        return;
    }
    
    consumir(TOKEN_ABRE_CHAVES, follow_bloco, size);
    comandos();
    consumir(TOKEN_FECHA_CHAVES, follow_bloco, size);
}

/* ======== EXPRESSÕES ======== */

static TipoSimbolo expr() {
    int first_expr[]  = FIRST_expr;
    int follow_expr[] = FOLLOW_expr;

    if (!in(token_atual.nome_token, first_expr,
            sizeof(first_expr)/sizeof(first_expr[0]))) {
        erro_sintatico("Esperado expressao",
                       follow_expr,
                       sizeof(follow_expr)/sizeof(follow_expr[0]));
        return TIPO_ERRO;
    }

    TipoSimbolo t_term = term();
    return expr_linha(t_term);
}

static TipoSimbolo expr_linha(TipoSimbolo h) {
    int follow_expr_linha[] = FOLLOW_expr_linha;
    int size = sizeof(follow_expr_linha)/sizeof(follow_expr_linha[0]);
    
    if (token_atual.nome_token == TOKEN_OPERADOR_SOMA ||
        token_atual.nome_token == TOKEN_OPERADOR_SUB) {

        Token op_token = token_atual;
        int op = token_atual.nome_token;
        consumir(op, follow_expr_linha, size);
        TipoSimbolo t_term = term();
        TipoSimbolo t_res  = tipo_resultado_binario(h, t_term, op_token);
        return expr_linha(t_res);
    }

    /* ε */
    return h;
}

static TipoSimbolo term() {
    int first_term[]  = FIRST_term;
    int follow_term[] = FOLLOW_term;

    if (!in(token_atual.nome_token, first_term,
            sizeof(first_term)/sizeof(first_term[0]))) {
        erro_sintatico("Esperado termo",
                       follow_term,
                       sizeof(follow_term)/sizeof(follow_term[0]));
        return TIPO_ERRO;
    }

    TipoSimbolo t_factor = factor();
    return term_linha(t_factor);
}

static TipoSimbolo term_linha(TipoSimbolo h) {
    int follow_term_linha[] = FOLLOW_term_linha;
    int size = sizeof(follow_term_linha)/sizeof(follow_term_linha[0]);
    
    if (token_atual.nome_token == TOKEN_OPERADOR_MUL ||
        token_atual.nome_token == TOKEN_OPERADOR_DIV) {

        Token op_token = token_atual;
        int op = token_atual.nome_token;
        consumir(op, follow_term_linha, size);
        TipoSimbolo t_factor = factor();
        TipoSimbolo t_res    = tipo_resultado_binario(h, t_factor, op_token);
        return term_linha(t_res);
    }

    /* ε */
    return h;
}

static TipoSimbolo factor() {
    int first_factor[]  = FIRST_factor;
    int follow_factor[] = FOLLOW_factor;
    int size = sizeof(follow_factor)/sizeof(follow_factor[0]);
    
    if (!in(token_atual.nome_token, first_factor,
            sizeof(first_factor)/sizeof(first_factor[0]))) {
        erro_sintatico("Esperado fator (ID, NUMERO, STRING, '(', '-')",
                       follow_factor, size);
        return TIPO_ERRO;
    }
    
    if (token_atual.nome_token == TOKEN_IDENTIFICADOR) {
        Token id_token = token_atual;
        consumir(TOKEN_IDENTIFICADOR, follow_factor, size);
        return tipo_de_variavel(id_token);
    }
    else if (token_atual.nome_token == TOKEN_NUMERO) {
        Token num_token = token_atual;
        consumir(TOKEN_NUMERO, follow_factor, size);
        return tipo_do_numero(num_token);
    }
    else if (token_atual.nome_token == TOKEN_STRING) {
        Token str_token = token_atual;
        consumir(TOKEN_STRING, follow_factor, size);
        (void)str_token;
        return TIPO_STRING_S;
    }
    else if (token_atual.nome_token == TOKEN_ABRE_PARENTESES) {
        consumir(TOKEN_ABRE_PARENTESES, follow_factor, size);
        TipoSimbolo t = expr();
        consumir(TOKEN_FECHA_PARENTESES, follow_factor, size);
        return t;
    }
    else if (token_atual.nome_token == TOKEN_OPERADOR_SUB) {
        Token op_token = token_atual;
        consumir(TOKEN_OPERADOR_SUB, follow_factor, size);
        TipoSimbolo t = factor();
        if (semantica_ativa && t != TIPO_ERRO && !eh_numerico(t)) {
            erro_semantico("Operador unário '-' só pode ser aplicado a números",
                           op_token);
            return TIPO_ERRO;
        }
        return t;
    }

    return TIPO_ERRO;
}
