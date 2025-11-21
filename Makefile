CC = gcc

# Flags: Mostra avisos (-Wall), Info de debug (-g) e onde achar os .h (-I)
CFLAGS = -Wall -Wextra -g -I"Analisador Lexico" -I"Analisador Sintático"

# Arquivos fonte (Aspas são necessárias por causa dos espaços e acentos)
SOURCES = compilador.c "Analisador Lexico/analisador_lexico.c" "Analisador Sintático/analisador_sintatico.c"

# Nome do executável final
OUTPUT = compilador

# Regra padrão (o que roda quando você digita 'make')
all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTPUT)

# Regra de limpeza (para remover o executável antigo)
clean:
	rm -f $(OUTPUT)