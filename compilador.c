#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Analisador Lexico/analisador_lexico.h"
#include "Analisador Sintático/analisador_sintatico.h"

// (Bônus) Deve ser declarado aqui para que o main possa liberar
extern Simbolo* tabela_de_simbolos; 

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <arquivo_fonte.txt>\n", argv[0]); 
        return 1; 
    }

    // 1. Ler o arquivo
    char* codigo_fonte = ler_arquivo_fonte(argv[1]);
    if (codigo_fonte == NULL) {
        return 1; // Erro já foi impresso pelo léxico
    }

    // 2. Chamar a análise (toda a mágica está aqui)
    bool sucesso = analisar(codigo_fonte);

    // 3. Reportar o resultado (Bônus)
    if (sucesso) {
        printf("\n[SUCESSO] Análise Sintática e Semântica concluídas.\n");
    } else {
        fprintf(stderr, "\n[FALHA] Análise encontrou erros.\n");
    }

    // 4. Limpar
    free(codigo_fonte);
    
    // (Bônus) Liberar a tabela de símbolos
    free(tabela_de_simbolos); 

    return (sucesso ? 0 : 1); // Retorna 0 em sucesso, 1 em erro
}