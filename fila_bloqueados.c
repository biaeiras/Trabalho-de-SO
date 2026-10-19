#include <stdio.h>
#include "fila_bloqueados.h"

//  Modulo com as funcoes e variaveis especificas para a fila de bloqueados

// Criar a fila de Bloqueados internamente
static int filaBloq[QT_BLOQ];
static int inicio = 0;
static int fim = 0;
static int quantidadeBloq = 0;

int enfileirarBloq(int processo) {
    if (quantidadeBloq == QT_BLOQ)
        return -1;   // Retorna -1 se a fila ja estiver cheia, e nao adiciona o processo

    filaBloq[fim] = processo;
    fim = (fim + 1) % QT_BLOQ;
    quantidadeBloq++;

    return 0;       // Retorna 0 se enfileirar com sucesso
}

int desenfileirarBloq(void) {
    if (quantidadeBloq == 0)
        return -1;      // Retorna -1 caso a fila esteja vazia 
    int processo = filaBloq[inicio];
    inicio = (inicio + 1) % QT_BLOQ;
    quantidadeBloq--;
    return processo;
}

// Mostra o conteudo da fila (apenas para acompanhar a execucao)
void imprimirFilaBloq(void) {
    for (int k = 0; k < quantidadeBloq; k++)
        printf("A%d ", filaBloq[(inicio + k) % QT_BLOQ] + 1);
}