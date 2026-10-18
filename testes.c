#include <stdio.h>
#include "fila.h"

int main(void) {
    int resultado;

    // Teste das funções da fila:

    resultado = enfileirar(0);
    printf("Enfileirar A1: %d\n", resultado);

    resultado = enfileirar(1);
    printf("Enfileirar A2: %d\n", resultado);

    resultado = enfileirar(2);
    printf("Enfileirar A3: %d\n", resultado);

    printf("Desenfileirar: A%d\n", desenfileirar() + 1);
    printf("Desenfileirar: A%d\n", desenfileirar() + 1);

    resultado = enfileirar(0);
    printf("Enfileirar A1 novamente: %d\n", resultado);

    printf("Desenfileirar: A%d\n", desenfileirar() + 1);
    printf("Desenfileirar: A%d\n", desenfileirar() + 1);

    return 0;
}
