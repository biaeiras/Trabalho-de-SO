#include "fila_bloqueados.h"

//  Modulo com as funcoes e variaveis especificas para a fila de bloqueados

// Criar a fila de Bloqueados internamente
static int filaBloq[QT_PROCESSOS];
static int inicio = 0;
static int fim = 0;
static int quantidadeBloq = 0;

int enfileirar(int processo) {
    if (quantidadeBloq == QT_PROCESSOS)
        return -1;   // Retorna -1 se a fila ja estiver cheia, e nao adiciona o processo

    filaBloq[fim] = processo;
    fim = (fim + 1) % QT_PROCESSOS;
    quantidadeBloq++;

    return 0;       // Retorna 0 se enfileirar com sucesso
}

int desenfileirar(void) {
    if (quantidadeBloq == 0)
        return -1;      // Retorna -1 caso a fila esteja vazia 
    int processo = filaBloq[inicio];
    inicio = (inicio + 1) % QT_PROCESSOS;
    quantidadeBloq--;
    return processo;
}