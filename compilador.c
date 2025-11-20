#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Analisador Lexico/analisador_lexico.h"
#include "Analisador Sintático/analisador_sintatico.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <arquivo_fonte.txt>\n", argv[0]); 
        return 1; 
    }

    char* codigo_fonte = ler_arquivo_fonte(argv[1]);
    if (codigo_fonte == NULL) {
        return 1;
    }

    bool sucesso = analisar(codigo_fonte);

    if (sucesso) {
        printf("\n[SUCESSO] Análise Sintática e Semântica concluída.\n");
    } else {
        fprintf(stderr,
                "\n[FALHA] Foram encontrados erros sintáticos e/ou semânticos.\n");
    }

    free(codigo_fonte);

    if (tabela_de_simbolos != NULL) {
        free(tabela_de_simbolos);
    }

    return (sucesso ? 0 : 1);
}
