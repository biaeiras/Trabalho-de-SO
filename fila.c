#include <stdio.h>
#include "fila.h"

//  Modulo com as funcoes e variaveis especificas para a fila de prontos

// Criar a fila de prontos internamente
static int filaProntos[QT_PROCESSOS];
static int inicio = 0;
static int fim = 0;
static int quantidadeProntos = 0;

int enfileirar(int processo) {
    if (quantidadeProntos == QT_PROCESSOS)
        return -1;   // Retorna -1 se a fila ja estiver cheia, e nao adiciona o processo

    filaProntos[fim] = processo;
    fim = (fim + 1) % QT_PROCESSOS;
    quantidadeProntos++;

    return 0;       // Retorna 0 se enfileirar com sucesso
}

int desenfileirar(void) {
    if (quantidadeProntos == 0)
        return -1;      // Retorna -1 caso a fila esteja vazia 
    int processo = filaProntos[inicio];
    inicio = (inicio + 1) % QT_PROCESSOS;
    quantidadeProntos--;
    return processo;
}

// Mostra o conteudo da fila (apenas para acompanhar a execucao)
void imprimirFila(void) {
    for (int k = 0; k < quantidadeProntos; k++)
        printf("A%d ", filaProntos[(inicio + k) % QT_PROCESSOS] + 1);
}
