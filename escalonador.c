#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "fila.h"

// Struct que guarda as informacoes de cada processo criado (A1, A2 e A3)
typedef struct processo {
    pid_t pid;              // PID do processo
    char nome[10];          // Nome da aplicacao (apenas para identificacao)
    char executavel[20];    // Nome do executavel da aplicacao
} Processo;

int main(void) {

    // Inicializando structs para os processos (pid = -1 porque o processo ainda nao foi criado)
    // e guardado em um vetor:
    Processo processos[QT_PROCESSOS] = {
        {-1, "A1", "./A1"},
        {-1, "A2", "./A2"},
        {-1, "A3", "./A3"}
    };

    // Criar os processos
    for (int i = 0; i < QT_PROCESSOS; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Erro ao criar o processo");
            return EXIT_FAILURE;        // encerra a funcao main
        } 

        if (pid == 0) {
            raise(SIGSTOP); // pausa o filho para que nao comece ate que o escalonador mande
            char *argv[] = {processos[i].nome, NULL};

            if (execvp(processos[i].executavel, argv) == -1) {
                perror("Erro ao executar programa");
                exit(EXIT_FAILURE);
            }
        }

        processos[i].pid = pid;     // guarda o pid do processo agora que ja foi criado

        printf("KernelSim: Processo %s criado com PID %d\n", processos[i].nome, (int)processos[i].pid);
        waitpid(pid, NULL, WUNTRACED);      // pai espera o filho parar para continuar
    }

    for (int i = 0; i < QT_PROCESSOS; i++) {
        kill(processos[i].pid, SIGCONT);
        waitpid(processos[i].pid, NULL, 0);
    }

    printf("KernelSim: todos os processos terminaram.\n");

    return 0;
}