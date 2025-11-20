#include "analisador_sintatico.h"
#include "../Analisador Lexico/analisador_lexico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token token_atual;
static const char* g_codigo_fonte;
static bool houve_erro_sintatico = false;
static bool houve_erro_semantico = false; // (Bônus)

// --- PROTÓTIPOS DAS FUNÇÕES SEMÂNTICAS (BÔNUS) ---
static void erro_semantico(const char* mensagem, int linha, int coluna);
static void inicializar_tabela_nativa();
static Simbolo* buscar_simbolo(const char* lexema);
static int checar_op(int tipo1, int tipo2, char op, int linha);

// --- PROTÓTIPOS ATUALIZADOS DO PARSER (BÔNUS) ---
static void consumir(int token_esperado, int follow_set[], int size);
static void erro_sintatico(const char* mensagem, int follow_set[], int size);

static void programa();
static void decls();
static void decl();
static int  tipo(); // <-- MUDOU: Retorna int (tipo)
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
static int  expr(); // <-- MUDOU: Retorna int (tipo)
static int  expr_linha(int h_tipo); // <-- MUDOU: Recebe h_tipo
static int  term(); // <-- MUDOU: Retorna int (tipo)
static int  term_linha(int h_tipo); // <-- MUDOU: Recebe h_tipo
static int  factor(); // <-- MUDOU: Retorna int (tipo)
static int  in(int token, int set[], int size);

// --- FUNÇÕES SEMÂNTICAS (BÔNUS) ---

// (Requisito B3) Reporta erro semântico sem parar
static void erro_semantico(const char* mensagem, int linha, int coluna) {
    // Evita imprimir linha de erro se a coluna for 0 (erro de tipo geral)
    if (coluna > 0) {
        imprimir_linha_erro(g_lexer.fonte, linha, coluna);
    } else {
        printf("\nErro linha %d:\n", linha);
    }
    printf("ERRO SEMÂNTICO: %s\n", mensagem);
    houve_erro_semantico = true; // Não para, apenas registra
}

// Busca um símbolo na tabela
static Simbolo* buscar_simbolo(const char* lexema) {
    for (int i = 0; i < tamanho_tabela; i++) {
        if (strcmp(tabela_de_simbolos[i].lexema, lexema) == 0) {
            return &tabela_de_simbolos[i];
        }
    }
    return NULL;
}

// Pré-carrega 'read' e 'print'
static void inicializar_tabela_nativa() {
    // Instala 'read'
    int pos = instalar_id("read");
    tabela_de_simbolos[pos].categoria = CAT_FUNCAO;
    tabela_de_simbolos[pos].tipo_dado = TIPO_VAZIO;
    tabela_de_simbolos[pos].tipo_parametro = CAT_VARIAVEL; 
    tabela_de_simbolos[pos].linha_declaracao = -1; // -1 = Nativo

    // Instala 'print'
    pos = instalar_id("print");
    tabela_de_simbolos[pos].categoria = CAT_FUNCAO;
    tabela_de_simbolos[pos].tipo_dado = TIPO_VAZIO;
    tabela_de_simbolos[pos].tipo_parametro = TIPO_QUALQUER; 
    tabela_de_simbolos[pos].linha_declaracao = -1;
}

// Checa tipos em operações binárias (+, -, *, /)
static int checar_op(int tipo1, int tipo2, char op, int linha) {
    if (tipo1 == TIPO_ERRO || tipo2 == TIPO_ERRO) return TIPO_ERRO;

    // Operações +/- não podem ter string
    if (op == '+' || op == '-') {
        if (tipo1 == TIPO_STRING || tipo2 == TIPO_STRING) {
            char msg[256];
            sprintf(msg, "Operador '%c' não pode ser aplicado ao tipo 'string'.", op);
            erro_semantico(msg, linha, 0); // Coluna 0 para não imprimir a linha
            return TIPO_ERRO;
        }
    }
    // Operações */ não podem ter string
    if (op == '*' || op == '/') {
         if (tipo1 == TIPO_STRING || tipo2 == TIPO_STRING) {
            char msg[256];
            sprintf(msg, "Operador '%c' não pode ser aplicado ao tipo 'string'.", op);
            erro_semantico(msg, linha, 0);
            return TIPO_ERRO;
        }
    }

    // Coerção: se um for float, o resultado é float
    if (tipo1 == TIPO_FLOAT || tipo2 == TIPO_FLOAT) {
        return TIPO_FLOAT;
    }
    return TIPO_INT;
}


// --- FUNÇÕES DO PARSER (SINTÁTICO) ---

static int in(int token, int set[], int size) {
    for (int i = 0; i < size; i++) {
        if (token == set[i]) return 1;
    }
    return 0;
}

static void consumir(int token_esperado, int follow_set[], int size) {
    if (token_atual.nome_token == token_esperado) {
        token_atual = get_token();
    } else {
        char msg[100];
        sprintf(msg, "Esperava token %d ('%c'), mas recebeu %d.", 
                token_esperado, (char)token_esperado, token_atual.nome_token);
        erro_sintatico(msg, follow_set, size);
    }
}

void erro_sintatico(const char* mensagem, int follow_set[], int size) {
    // Relatar o erro (usando a nova função)
    imprimir_linha_erro(g_lexer.fonte, token_atual.linha, token_atual.coluna);
    printf("ERRO SINTÁTICO: %s.\n", mensagem);
    printf("  Token recebido: %d (Inesperado)\n", token_atual.nome_token);

    // Marca que houve um erro para o 'analisar' saber
    houve_erro_sintatico = true;

    // Modo Pânico (Sincronização)
    printf("  Iniciando modo pânico... Sincronizando com { ");
    for(int i=0; i<size; i++) printf("%d ", follow_set[i]);
    printf("}\n");

    // REMOVA O 'get_token()' DAQUI

    while (token_atual.nome_token != TOKEN_EOF) {
        
        // 1. O token atual JÁ É um token de sincronização?
        bool encontrado = false;
        for (int i = 0; i < size; i++) {
            if (token_atual.nome_token == follow_set[i]) {
                encontrado = true;
                break;
            }
        }

        if (encontrado) {
            printf("  Sincronização encontrada. Continuando análise no token %d.\n", token_atual.nome_token);
            return; // Encontrou! Retorna SEM consumir.
        }

        // 2. Não é um token de sync. Consome ele e tenta o próximo.
        token_atual = get_token(); 
    }
}

bool analisar(const char* codigo_fonte) 
{
    g_codigo_fonte = codigo_fonte; 
    houve_erro_sintatico = false;
    houve_erro_semantico = false; // (Bônus)

    g_lexer.fonte = g_codigo_fonte;
    g_lexer.posicao = 0;
    g_lexer.atual = g_codigo_fonte[0];
    g_lexer.linha = 1;
    g_lexer.coluna = 1;
    
    inicializar_tabela_nativa(); // (Bônus)

    token_atual = get_token();
    programa();

    if (!houve_erro_sintatico && token_atual.nome_token != TOKEN_EOF) {
        imprimir_linha_erro(g_codigo_fonte, token_atual.linha, token_atual.coluna);
        fprintf(stderr, "ERRO: Código encontrado após a declaração 'fim'.\n");
        houve_erro_sintatico = true;
    }

    // (Bônus) Retorna true se NÃO houve erros de NENHUM tipo
    return !houve_erro_sintatico && !houve_erro_semantico;
}

// --- FUNÇÕES DO PARSER (SEMÂNTICO) ---

static void programa() {
    int first_programa[] = FIRST_programa;
    int follow_programa[] = FOLLOW_programa;
    int size = sizeof(follow_programa)/sizeof(follow_programa[0]); 
    
    if (!in(token_atual.nome_token, first_programa, sizeof(first_programa)/sizeof(first_programa[0]))) {
        erro_sintatico("Esperado 'inicio'", follow_programa, size);
    }
    
    consumir(TOKEN_INICIO, follow_programa, size);
    decls();
    comandos();
    consumir(TOKEN_FIM, follow_programa, size);
}

static void decls() {
    int first_decls[] = FIRST_decls;
    int follow_decls[] = FOLLOW_decls;
    
    if (in(token_atual.nome_token, first_decls, sizeof(first_decls)/sizeof(first_decls[0]))) {
        decl();
        decls();
    }
    else if (!in(token_atual.nome_token, follow_decls, sizeof(follow_decls)/sizeof(follow_decls[0]))) {
        erro_sintatico("Esperado declaracao de variavel ou inicio dos comandos", 
                       follow_decls, sizeof(follow_decls)/sizeof(follow_decls[0]));
        return;
    }
}

static void decl() {
    int follow_decl[] = FOLLOW_decl;
    int size = sizeof(follow_decl)/sizeof(follow_decl[0]);
    
    int s_tipo = tipo(); // 1. Pega o tipo de <tipo>
    
    // (Bônus) Padrão "Checar-antes-de-Consumir"
    if (token_atual.nome_token != TOKEN_IDENTIFICADOR) {
        erro_sintatico("Esperado um IDENTIFICADOR na declaração.", follow_decl, size);
        return; // Para a função
    }
    
    Token id_token = token_atual; // Guarda o ID para checagem
    
    // (Bônus) AÇÃO SEMÂNTICA 2: Checar redeclaração (Erro B2)
    const char* lexema = tabela_de_simbolos[id_token.atributo.pos_simbolo].lexema;
    Simbolo* s = buscar_simbolo(lexema); 

    if (s && s->linha_declaracao != 0) { // Já foi declarado
        char msg[256];
        sprintf(msg, "Variável '%s' já foi declarada na linha %d.", s->lexema, s->linha_declaracao);
        erro_semantico(msg, id_token.linha, id_token.coluna);
    } else if (s) {
        // (Bônus) AÇÃO SEMÂNTICA 3: Adicionar na tabela
        s->categoria = CAT_VARIAVEL;
        s->tipo_dado = s_tipo;
        s->linha_declaracao = id_token.linha;
    }
    
    consumir(TOKEN_IDENTIFICADOR, follow_decl, size);
    consumir(TOKEN_PONTO_VIRGULA, follow_decl, size);
}

static int tipo() { // <-- MUDOU: Retorna int (tipo)
    int first_tipo[] = FIRST_tipo;
    int follow_tipo[] = FOLLOW_tipo;
    int size = sizeof(follow_tipo)/sizeof(follow_tipo[0]);

    int s_tipo = TIPO_ERRO; // Default em caso de erro

    if (token_atual.nome_token == TOKEN_TIPO_INT) {
        s_tipo = TIPO_INT; // (Bônus) AÇÃO SEMÂNTICA
        consumir(TOKEN_TIPO_INT, follow_tipo, size);
    } else if (token_atual.nome_token == TOKEN_TIPO_FLOAT) {
        s_tipo = TIPO_FLOAT; // (Bônus) AÇÃO SEMÂNTICA
        consumir(TOKEN_TIPO_FLOAT, follow_tipo, size);
    } else if (token_atual.nome_token == TOKEN_TIPO_STRING) {
        s_tipo = TIPO_STRING; // (Bônus) AÇÃO SEMÂNTICA
        consumir(TOKEN_TIPO_STRING, follow_tipo, size);
    } else {
        erro_sintatico("Esperado um tipo (int, float, string)", follow_tipo, size);
        return TIPO_ERRO;
    }
    return s_tipo; // (Bônus) Retorna o tipo sintetizado
}

static void comandos() {
    int first_comandos[] = FIRST_comandos;
    int follow_comandos[] = FOLLOW_comandos;

    if (in(token_atual.nome_token, first_comandos, sizeof(first_comandos)/sizeof(first_comandos[0]))) {
        comando();
        comandos();
    }
    else if (!in(token_atual.nome_token, follow_comandos, sizeof(follow_comandos)/sizeof(follow_comandos[0]))) {
        erro_sintatico("Esperado um comando ou 'fim'", 
                       follow_comandos, sizeof(follow_comandos)/sizeof(follow_comandos[0]));
        return;
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
            return;
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
    
    // (Bônus) Padrão "Checar-antes-de-Consumir"
    if (token_atual.nome_token != TOKEN_IDENTIFICADOR) {
        erro_sintatico("Esperado ID para atribuicao", follow_atribuicao, size);
        return;
    }
    
    Token id_token = token_atual; // Guarda o ID
    
    // (Bônus) AÇÃO SEMÂNTICA 1: Buscar tipo do ID (Erro B1)
    const char* lexema = tabela_de_simbolos[id_token.atributo.pos_simbolo].lexema;
    Simbolo* s = buscar_simbolo(lexema);
    int tipo_id = TIPO_ERRO; // Assumir erro
    
    if (!s || s->linha_declaracao == 0) {
        char msg[256];
        sprintf(msg, "Variável '%s' não declarada.", lexema);
        erro_semantico(msg, id_token.linha, id_token.coluna);
    } else if (s->categoria != CAT_VARIAVEL) {
        char msg[256];
        sprintf(msg, "'%s' não é uma variável.", s->lexema);
        erro_semantico(msg, id_token.linha, id_token.coluna);
    } else {
        tipo_id = s->tipo_dado; // Sucesso
    }

    consumir(TOKEN_IDENTIFICADOR, follow_atribuicao, size); 
    consumir(TOKEN_ATRIBUICAO, follow_atribuicao, size); 
    int tipo_expr = expr(); // (Bônus) Pega o tipo da expressão
    
    // (Bônus) AÇÃO SEMÂNTICA 2: Checar atribuição (Erro B4)
    if (tipo_id == TIPO_INT && tipo_expr == TIPO_FLOAT) {
        erro_semantico("Atribuição de um valor 'float' a uma variável 'int'.", id_token.linha, id_token.coluna);
    } 
    else if (tipo_id != TIPO_ERRO && tipo_expr != TIPO_ERRO && tipo_id != tipo_expr) {
         erro_semantico("Tipos incompatíveis na atribuição.", id_token.linha, id_token.coluna);
    }
}

static void chamada() {
    int first_chamada[] = FIRST_chamada;
    int follow_chamada[] = FOLLOW_chamada;
    int size = sizeof(follow_chamada)/sizeof(follow_chamada[0]);
    
    // (Bônus) Padrão "Checar-antes-de-Consumir"
    if (token_atual.nome_token != TOKEN_IDENTIFICADOR) {
        erro_sintatico("Esperado ID para chamada", follow_chamada, size);
        return;
    }
    
    Token id_token = token_atual; // Guarda o ID
    
    // (Bônus) AÇÃO SEMÂNTICA: (Erro B1)
    char msg[256];
    sprintf(msg, "Procedimento '%s' não declarado.", 
            tabela_de_simbolos[id_token.atributo.pos_simbolo].lexema);
    erro_semantico(msg, id_token.linha, id_token.coluna);
    
    consumir(TOKEN_IDENTIFICADOR, follow_chamada, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_chamada, size);
    args();
    consumir(TOKEN_FECHA_PARENTESES, follow_chamada, size);
}

static void args() {
    int first_args[] = FIRST_args;
    
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
    
    expr(); // Chama expr, que já faz checagem de tipo
    
    if (token_atual.nome_token == TOKEN_VIRGULA) {
        consumir(TOKEN_VIRGULA, follow_expr_list, size);
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
    
    consumir(TOKEN_READ, follow_entrada, size); 
    consumir(TOKEN_ABRE_PARENTESES, follow_entrada, size); 
    
    // (Bônus) Padrão "Checar-antes-de-Consumir" (CORREÇÃO DO BUG)
    if (token_atual.nome_token != TOKEN_IDENTIFICADOR) {
        erro_sintatico("Esperava um IDENTIFICADOR como parâmetro para 'read'.", follow_entrada, size);
        return; // PARA A FUNÇÃO
    }

    Token id_token = token_atual; // Guarda o ID

    // (Bônus) AÇÃO SEMÂNTICA: Checar parâmetro (Erro B1, B3)
    const char* lexema = tabela_de_simbolos[id_token.atributo.pos_simbolo].lexema;
    Simbolo* s = buscar_simbolo(lexema);

    if (!s || s->linha_declaracao == 0) {
        char msg[256];
        sprintf(msg, "Variável '%s' não declarada.", lexema);
        erro_semantico(msg, id_token.linha, id_token.coluna);
    } else if (s->categoria != CAT_VARIAVEL) {
        char msg[256];
        sprintf(msg, "'%s' não é uma variável (parâmetro de 'read').", s->lexema);
        erro_semantico(msg, id_token.linha, id_token.coluna);
    }
    
    consumir(TOKEN_IDENTIFICADOR, follow_entrada, size);
    consumir(TOKEN_FECHA_PARENTESES, follow_entrada, size);
}

static void saida() {
    int first_saida[] = FIRST_saida;
    int follow_saida[] = FOLLOW_saida;
    int size = sizeof(follow_saida)/sizeof(follow_saida[0]);
    
    if (!in(token_atual.nome_token, first_saida, sizeof(first_saida)/sizeof(first_saida[0]))) {
        erro_sintatico("Esperado 'print'", follow_saida, size);
        return;
    }
    
    consumir(TOKEN_PRINT, follow_saida, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_saida, size);
    
    int tipo_expr = expr(); // (Bônus) Pega o tipo da expressão
    
    // (Bônus) AÇÃO SEMÂNTICA: Checar parâmetro (Erro B3)
    if (tipo_expr == TIPO_VAZIO || tipo_expr == TIPO_ERRO) {
        erro_semantico("Expressão inválida ou vazia para 'print'.", token_atual.linha, token_atual.coluna);
    }
    
    consumir(TOKEN_FECHA_PARENTESES, follow_saida, size);
}

static void if_stmt() {
    int first_if_stmt[] = FIRST_if_stmt;
    int follow_if_stmt[] = FOLLOW_if_stmt;
    int size = sizeof(follow_if_stmt)/sizeof(follow_if_stmt[0]);
    
    if (!in(token_atual.nome_token, first_if_stmt, sizeof(first_if_stmt)/sizeof(first_if_stmt[0]))) {
        erro_sintatico("Esperado 'if'", follow_if_stmt, size);
        return;
    }
    
    consumir(TOKEN_IF, follow_if_stmt, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_if_stmt, size);
    
    int tipo_expr = expr(); // (Bônus)
    // (Bônus) AÇÃO SEMÂNTICA: Checar condicional
    if (tipo_expr != TIPO_INT && tipo_expr != TIPO_FLOAT && tipo_expr != TIPO_ERRO) {
        erro_semantico("Expressão condicional do 'if' deve ser 'int' ou 'float'.", token_atual.linha, 0);
    }
    
    consumir(TOKEN_FECHA_PARENTESES, follow_if_stmt, size);
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
    
    consumir(TOKEN_WHILE, follow_while_stmt, size);
    consumir(TOKEN_ABRE_PARENTESES, follow_while_stmt, size);
    
    int tipo_expr = expr(); // (Bônus)
    // (Bônus) AÇÃO SEMÂNTICA: Checar condicional
    if (tipo_expr != TIPO_INT && tipo_expr != TIPO_FLOAT && tipo_expr != TIPO_ERRO) {
        erro_semantico("Expressão condicional do 'while' deve ser 'int' ou 'float'.", token_atual.linha, 0);
    }
    
    consumir(TOKEN_FECHA_PARENTESES, follow_while_stmt, size);
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

static int expr() { // <-- MUDOU
    int first_expr[] = FIRST_expr;
    int follow_expr[] = FOLLOW_expr;
    
    if (!in(token_atual.nome_token, first_expr, sizeof(first_expr)/sizeof(first_expr[0]))) {
        erro_sintatico("Esperado expressao", follow_expr, sizeof(follow_expr)/sizeof(follow_expr[0]));
        return TIPO_ERRO; // Retorna erro
    }
    
    int h_tipo = term(); // Pega tipo do 'term'
    return expr_linha(h_tipo); // Passa como herdado
}

static int expr_linha(int h_tipo) { // <-- MUDOU
    int first_expr_linha[] = FIRST_expr_linha;
    int follow_expr_linha[] = FOLLOW_expr_linha;
    int size = sizeof(follow_expr_linha)/sizeof(follow_expr_linha[0]);
    
    if (token_atual.nome_token == TOKEN_OPERADOR_SOMA) {
        Token op_token = token_atual; // Salva o token da operação
        consumir(TOKEN_OPERADOR_SOMA, follow_expr_linha, size);
        int s_tipo_term = term();
        
        int tipo_resultante = checar_op(h_tipo, s_tipo_term, '+', op_token.linha);
        return expr_linha(tipo_resultante); // Passa resultado como herdado
    }
    else if (token_atual.nome_token == TOKEN_OPERADOR_SUB) {
        Token op_token = token_atual;
        consumir(TOKEN_OPERADOR_SUB, follow_expr_linha, size);
        int s_tipo_term = term();
        
        int tipo_resultante = checar_op(h_tipo, s_tipo_term, '-', op_token.linha);
        return expr_linha(tipo_resultante);
    }
    
    return h_tipo; // Regra ε: retorna o tipo herdado
}

static int term() { // <-- MUDOU
    int first_term[] = FIRST_term;
    int follow_term[] = FOLLOW_term;
    
    if (!in(token_atual.nome_token, first_term, sizeof(first_term)/sizeof(first_term[0]))) {
        erro_sintatico("Esperado termo", follow_term, sizeof(follow_term)/sizeof(follow_term[0]));
        return TIPO_ERRO;
    }
    
    int h_tipo = factor();
    return term_linha(h_tipo);
}

static int term_linha(int h_tipo) { // <-- MUDOU
    int first_term_linha[] = FIRST_term_linha;
    int follow_term_linha[] = FOLLOW_term_linha;
    int size = sizeof(follow_term_linha)/sizeof(follow_term_linha[0]);
    
    if (token_atual.nome_token == TOKEN_OPERADOR_MUL) {
        Token op_token = token_atual;
        consumir(TOKEN_OPERADOR_MUL, follow_term_linha, size);
        int s_tipo_factor = factor();
        int tipo_resultante = checar_op(h_tipo, s_tipo_factor, '*', op_token.linha);
        return term_linha(tipo_resultante);
    }
    else if (token_atual.nome_token == TOKEN_OPERADOR_DIV) {
         Token op_token = token_atual;
        consumir(TOKEN_OPERADOR_DIV, follow_term_linha, size);
        int s_tipo_factor = factor();
        int tipo_resultante = checar_op(h_tipo, s_tipo_factor, '/', op_token.linha);
        return term_linha(tipo_resultante);
    }
    
    return h_tipo; // Regra ε: retorna o tipo herdado
}

static int factor() { // <-- MUDOU
    int first_factor[] = FIRST_factor;
    int follow_factor[] = FOLLOW_factor;
    int size = sizeof(follow_factor)/sizeof(follow_factor[0]);
    
    if (!in(token_atual.nome_token, first_factor, sizeof(first_factor)/sizeof(first_factor[0]))) {
        erro_sintatico("Esperado fator (ID, NUMERO, STRING, '(', '-')", follow_factor, size);
        return TIPO_ERRO;
    }
    
    if (token_atual.nome_token == TOKEN_IDENTIFICADOR) {
        Token id_token = token_atual;
        
        // (Bônus) AÇÃO SEMÂNTICA: (Erro B1)
        const char* lexema = tabela_de_simbolos[id_token.atributo.pos_simbolo].lexema;
        Simbolo* s = buscar_simbolo(lexema);
        
        consumir(TOKEN_IDENTIFICADOR, follow_factor, size); // Consome APÓS buscar

        if (!s || s->linha_declaracao == 0) {
            char msg[256];
            sprintf(msg, "Variável '%s' não declarada.", lexema);
            erro_semantico(msg, id_token.linha, id_token.coluna);
            return TIPO_ERRO;
        } else if (s->categoria != CAT_VARIAVEL) {
             char msg[256];
            sprintf(msg, "'%s' não é uma variável.", s->lexema);
            erro_semantico(msg, id_token.linha, id_token.coluna);
            return TIPO_ERRO;
        }
        return s->tipo_dado; // Retorna o tipo da variável
    }
    else if (token_atual.nome_token == TOKEN_NUMERO) {
        int tipo = (strchr(token_atual.atributo.valor_literal, '.') ? TIPO_FLOAT : TIPO_INT);
        consumir(TOKEN_NUMERO, follow_factor, size);
        return tipo;
    }
    else if (token_atual.nome_token == TOKEN_STRING) {
        consumir(TOKEN_STRING, follow_factor, size);
        return TIPO_STRING;
    }
    else if (token_atual.nome_token == TOKEN_ABRE_PARENTESES) {
        consumir(TOKEN_ABRE_PARENTESES, follow_factor, size);
        int s_tipo = expr();
        consumir(TOKEN_FECHA_PARENTESES, follow_factor, size);
        return s_tipo;
    }
    else if (token_atual.nome_token == TOKEN_OPERADOR_SUB) {
        Token op_token = token_atual;
        consumir(TOKEN_OPERADOR_SUB, follow_factor, size);
        int s_tipo = factor();
        
        if (s_tipo != TIPO_INT && s_tipo != TIPO_FLOAT) {
            erro_semantico("Operador unário '-' aplicado a tipo não-numérico.", op_token.linha, op_token.coluna);
            return TIPO_ERRO;
        }
        return s_tipo;
    }
    return TIPO_ERRO; // Caso não caia em nada
}