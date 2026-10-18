#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "fila.h"

// Estados do processo:
#define NAO_CRIADO -1
#define PRONTO 0
#define EXECUTANDO 1
#define BLOQUEADO 2
#define TERMINADO 3

// Struct que guarda as informacoes de cada processo criado (A1, A2 e A3)
typedef struct processo {
    pid_t pid;              // PID do processo
    char nome[10];          // Nome da aplicacao (apenas para identificacao)
    char executavel[20];    // Nome do executavel da aplicacao
    int estado;
} Processo;

int main(void) {

    // Inicializando structs para os processos (pid = -1 porque o processo ainda nao foi criado)
    // e guardado em um vetor:
    Processo processos[QT_PROCESSOS] = {
        {-1, "A1", "./A1", -1},
        {-1, "A2", "./A2", -1},
        {-1, "A3", "./A3", -1}
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
        int status;
        waitpid(pid, &status, WUNTRACED);      // pai espera o filho parar para continuar 
                                               // (guarda o status para conferir se o filho parou certinho)

        if (WIFSTOPPED(status)) {               // so considera o processo pronto se tiver parado 
            processos[i].estado = PRONTO;
            if (enfileirar(i) == -1) {
                fprintf(stderr, "Erro: fila de prontos cheia.\n");
                return EXIT_FAILURE;
            }
        }
        else {
            fprintf(stderr, "O processo %s nao parou como esperado.\n", processos[i].nome);
            return EXIT_FAILURE;
        }
                                                            
        processos[i].estado = PRONTO;
    }


    int ativos = QT_PROCESSOS;  // quantidade de processos que ainda nao terminaram

    while (ativos > 0) {
        int i = desenfileirar();    // pega o proximo processo na fila de prontos

        if (i == -1) {
            fprintf(stderr, "Erro: fila de prontos vazia.\n");
            return EXIT_FAILURE;
        }

        printf("KernelSim: executando %s\n", processos[i].nome);
        processos[i].estado = EXECUTANDO;

        if (kill(processos[i].pid, SIGCONT) == -1) {        // comeca a executar o processo que foi desenfileirado
            perror("Erro ao continuar processo");
            return EXIT_FAILURE;
        }

        sleep(1);    // time slice (temporario, vai depender dos sinais recebidos de InterControllerSim)

        // Tenta interromper o processo ao fim da fatia de tempo.
        if (kill(processos[i].pid, SIGSTOP) == -1 && errno != ESRCH) {     

            // ESRCH indica que o processo nao existe mais
            // Pode acontecer se ele terminou antes de receber o SIGSTOP

            perror("Erro ao interromper processo");
            return EXIT_FAILURE;
        }

        int status;
        pid_t resultado = waitpid(processos[i].pid, &status, WUNTRACED);    
        // WUNTRACED permite distinguir um processo que foi interrompido de um que terminou

        if (resultado == -1) {
            perror("Erro ao esperar pelo processo");
            return EXIT_FAILURE;
        }

        if (WIFSTOPPED(status)) {
            // confere se o processo esta pausado para deixar seu estado como pronto
            processos[i].estado = PRONTO;

            if (enfileirar(i) == -1) {      // tenta enfileirar o processo na fila de prontos
                fprintf(stderr, "Erro: fila de prontos cheia.\n");
                return EXIT_FAILURE;
            }

            printf("KernelSim: %s parou de executar e voltou para a fila\n", processos[i].nome);
        }

        else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // verifica se o processo terminou (ou normalmente ou por algum sinal)
            processos[i].estado = TERMINADO;
            ativos--;   // atualiza a quantidade de processos ativos (nao terminados)

            printf("KernelSim: %s terminou. Restam %d processo(s).\n",
                processos[i].nome, ativos);
        }
    }

    for (int i = 0; i < QT_PROCESSOS; i++) {
        kill(processos[i].pid, SIGCONT);
        waitpid(processos[i].pid, NULL, 0);
    }

    printf("KernelSim: todos os processos terminaram.\n");

    return 0;
}