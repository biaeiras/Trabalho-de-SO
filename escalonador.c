#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "fila.h"
#include "fila_bloqueados.h"
#include "common.h"

// Estados do processo:
#define NAO_CRIADO -1
#define PRONTO 0
#define EXECUTANDO 1
#define BLOQUEADO 2
#define TERMINADO 3

// Sinal que o KernelSim manda ao InterControllerSim avisando que houve um pedido de I/O
#define SIGNAL_PEDIDO_IO SIGTERM

// Variavel usada para indicar que uma IRQ0 foi recebida:
volatile sig_atomic_t irq0_recebida = 0;       // fim da fatia de tempo

// Variavel usada para indicar que uma IRQ0 foi recebida:
volatile sig_atomic_t irq1_recebida = 0;       // fim de uma operacao de I/O

// Variavel usada para indicar que uma syscall foi recebida:
volatile sig_atomic_t syscall_recebida = 0;    // algum processo pediu I/O 

// volatile indica que seu valor pode mudar fora do fluxo normal do programa
// sig_atomic_t permite ler e alterar esse valor de forma atômica
// no contexto do tratador de sinal

void tratarIRQ0(int sinal) {
    irq0_recebida = 1;
}

void tratarIRQ1(int sinal) {
    irq1_recebida = 1;
}

void tratarSyscall(int sinal) {
    syscall_recebida = 1;
}

// Dorme ate chegar qualquer um dos tres sinais (IRQ0, IRQ1 ou syscall)
void esperarEvento(void) {
    while (!irq0_recebida && !irq1_recebida && !syscall_recebida)
        pause();
}

// Struct que guarda as informacoes de cada processo criado (A1, A2 e A3)
// Aqui tambem fica o "contexto" do processo: o PC e o pedido de syscall
typedef struct processo {
    pid_t pid;              // PID do processo
    char nome[10];          // Nome da aplicacao (apenas para identificacao)
    char executavel[20];    // Nome do executavel da aplicacao
    int estado;
    int pc;                 // Contexto: ultimo PC informado pelo processo
    char op;                // Contexto: operacao da syscall ('R' ou 'W'), 0 se nao tiver
    int pedindoIO;          // 1 se o processo acabou de pedir I/O e ainda nao foi bloqueado
} Processo;

Processo processos[QT_PROCESSOS] = {
    {-1, "A1", "./A1", NAO_CRIADO, 0, 0, 0},
    {-1, "A2", "./A2", NAO_CRIADO, 0, 0, 0},
    {-1, "A3", "./A3", NAO_CRIADO, 0, 0, 0}
};

// Transforma o estado (representado como int) em string  
const char *nomeEstado(int estado) {
    switch (estado) {
        case PRONTO:      return "pronto";
        case EXECUTANDO:  return "executando";
        case BLOQUEADO:   return "bloqueado";
        case TERMINADO:   return "terminado";
        default:          return "nao criado";
    }
}

// Mostra a situacao do sistema (filas e PC de cada processo)
void mostrarEstado(void) {
    printf("    >> Fila de prontos: [ ");
    imprimirFila();
    printf("] | Fila de bloqueados (E/S): [ ");
    imprimirFilaBloq();
    printf("]\n    >> ");
    for (int i = 0; i < QT_PROCESSOS; i++)
        printf("%s: PC=%d (%s)   ", processos[i].nome, processos[i].pc, nomeEstado(processos[i].estado));
    printf("\n");
}


// Le as mensagens do pipe e guarda no contexto de cada processo
void lerMensagens(int fd) {
    Msg m;      // variavel para armazenar uma Msg
    ssize_t bytesLidos;     // armazena a quantidade de bytes

    while ((bytesLidos = read(fd, &m, sizeof(Msg))) > 0) {      // read tenta ler a mensagem do pipe enquanto tiver dados disponiveis
        if (bytesLidos == sizeof(Msg)) {
            printf("KernelSim: mensagem recebida - processo A%d, PC = %d, operacao = %c\n", 
            m.idx + 1, m.pc, m.op == 0 ? '-' : m.op);       // mostra "-" quando nao tiver operacao de I/O
                
             // Salva o contexto do processo
            processos[m.idx].pc = m.pc;
            if (m.op != 0) {
                processos[m.idx].op = m.op;
                processos[m.idx].pedindoIO = 1;
            }
        }
    }

    if (bytesLidos == -1 && errno != EAGAIN && errno != EWOULDBLOCK) 
        perror("Erro ao ler o pipe");
}


int main(void) {

    // Criar o pipe para comunicacao entre os processos e KernelSim, com ifnormacoes sobre PC e pedidos de I/O:
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("Erro ao criar pipe");
        return EXIT_FAILURE;
    }

    // Configura a leitura do pipe para não bloquear o KernelSim
    int flags = fcntl(pipefd[0], F_GETFL, 0);   // consulta as configuracoes da ponta de leitura do pipe

    if (flags == -1 || fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("Erro ao configurar o pipe");
        return EXIT_FAILURE;
    }



    // Iniciar interControllerSim (interController.c):
    pid_t pidKernel = getpid();   // guarda o pid do processo pai (KernelSim)

    signal(SIGUSR1, tratarIRQ0);       // trata o recebimento do sinal SIGUSR1(IRQ0)
    signal(SIGUSR2, tratarIRQ1);       // trata o recebimento do sinal SIGUSR2(IRQ1)
    signal(SIGALRM, tratarSyscall);    // trata o recebimento do sinal SIG_SYSCALL(SIGALRM)


    // Transforma o pid em string para ser passado como argumento:
    char pidKernelStr[20];
    snprintf(pidKernelStr, sizeof(pidKernelStr), "%d", (int) pidKernel);
    
    pid_t pidControlador = fork();

    if (pidControlador < 0) {
        perror("Erro ao criar o processo de InterControllerSim");
        return EXIT_FAILURE;        // encerra a funcao main
    }

    if (pidControlador == 0) {
        // O InterControllerSim se comunica com o KernelSim apenas por sinais.
        // Por isso fecha as duas pontas do pipe.
        close(pipefd[0]);
        close(pipefd[1]);
        char *argvControlador[] = {"./interController", pidKernelStr, NULL};

        if (execvp(argvControlador[0], argvControlador) == -1) {
            perror("Erro ao executar programa");
            exit(EXIT_FAILURE);
        }
    }


    // Criar os processos
    for (int i = 0; i < QT_PROCESSOS; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Erro ao criar o processo");
            return EXIT_FAILURE;        // encerra a funcao main
        } 

        if (pid == 0) {
            // Os processos enviam mensagens ao KernelSim,
            // portanto fecham a ponta de leitura e mantem a de escrita
            close(pipefd[0]);

            char idxStr[16];
            char fdStr[16];

            // Converte o índice do processo e o descritor do pipe para strings
            snprintf(idxStr, sizeof(idxStr), "%d", i);
            snprintf(fdStr, sizeof(fdStr), "%d", pipefd[1]);

            raise(SIGSTOP); // pausa o filho para que nao comece ate que o escalonador mande
            char *argv[] = {processos[i].nome, idxStr, fdStr, NULL};

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

    // O KernelSim recebe mensagens pela ponta de leitura
    // Como ele não envia mensagens pelo pipe, fecha a ponta de escrita depois de criar todos os processos filhos
    close(pipefd[1]);
    int ativos = QT_PROCESSOS;  // quantidade de processos que ainda nao terminaram
    int executando = -1;        // indice do processo que esta executando no momento (-1 = nenhum)

    printf("\n ----- KernelSim: inicio do escalonamento ----- \n");
    mostrarEstado();

    while (ativos > 0) {
        // Se a CPU esta livre, executa o proximo processo pronto
        if (executando == -1) {
            int i = desenfileirar();    // pega o proximo processo na fila de prontos
            if (i != -1) {
                printf("KernelSim: executando %s\n", processos[i].nome);
                processos[i].estado = EXECUTANDO;
                executando = i;

                if (kill(processos[i].pid, SIGCONT) == -1) {        // comeca a executar o processo que foi desenfileirado
                    perror("Erro ao continuar processo");
                    return EXIT_FAILURE;
                }
            }
            // Se i == -1 todos estao bloqueados esperando I/O: so espera o proximo IRQ1
        }

        // Espera acontecer algum evento (IRQ0, IRQ1 ou syscall)
        esperarEvento();

        // Anota quais eventos aconteceram e limpa os avisos
        int irq0 = irq0_recebida;
        int irq1 = irq1_recebida;
        irq0_recebida = 0;
        irq1_recebida = 0;
        syscall_recebida = 0;

        // Le as mensagens do pipe (atualiza PC e pedidos de syscall)
        lerMensagens(pipefd[0]);

        // Se IRQ1 foi recebido terminou um I/O, entao tira o primeiro da fila de bloqueados
        // Isso vem ANTES do tratamento do IRQ0, pois quem terminou I/O
        // entra na fila de prontos antes de quem terminou o time slice.
        if (irq1) {     // Verifica se houve um IRQ1
            int b = desenfileirarBloq();

            if (b != -1) {
                processos[b].estado = PRONTO;       // processo desbloqueado, volta a ser pronto
                processos[b].op = 0;    // syscall concluida
                if (enfileirar(b) == -1) {
                    fprintf(stderr, "Erro: fila de prontos cheia.\n");
                    return EXIT_FAILURE;
                }
                printf("KernelSim: IRQ1 - %s terminou o I/O e foi para a fila de prontos\n", processos[b].nome);
            }
        }

        // Trata o precesso que esta executando, se tiver

        if (executando != -1) {
            int i = executando;

            // Se acabou o time slice (IRQ0) ou se pediu I/O (syscall), o processo para de executar
            if (irq0 || processos[i].pedindoIO) {

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

                // Pode ter chegado mensagem entre a leitura anterior e o SIGSTOP
                lerMensagens(pipefd[0]);
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // o processo terminou (ou normalmente ou por algum sinal)
                    processos[i].estado = TERMINADO;
                    ativos--;   // atualiza a quantidade de processos ativos (nao terminados)

                    printf("KernelSim: %s terminou. Restam %d processo(s).\n",
                        processos[i].nome, ativos);
                }

                else if (processos[i].pedindoIO) {
                    // Se tem pedido de IO, o processo vai para a fila de bloqueados
                    processos[i].estado = BLOQUEADO;
                    processos[i].pedindoIO = 0;
                    if (enfileirarBloq(i) == -1) {
                        fprintf(stderr, "Erro: fila de bloqueados cheia.\n");
                        return EXIT_FAILURE;
                    }

                    // avisa o InterControllerSim que ha um novo pedido de I/O
                    kill(pidControlador, SIGNAL_PEDIDO_IO);

                    printf("KernelSim: %s fez syscall(D1, %c) e foi bloqueado\n", processos[i].nome, processos[i].op);
                }

                else {
                    // acabou o time slice: volta para a fila de prontos
                    processos[i].estado = PRONTO;

                    if (enfileirar(i) == -1) {
                        fprintf(stderr, "Erro: fila de prontos cheia.\n");
                        return EXIT_FAILURE;
                    }

                    printf("KernelSim: IRQ0 - %s parou de executar e voltou para a fila\n", processos[i].nome);
                }
                executando = -1;    // CPU livre
            }
        }
        if (irq0 || irq1)
            mostrarEstado();
    }

    // Todos terminaram: encerra o InterControllerSim (que e um processo infinito)
    kill(pidControlador, SIGKILL);
    waitpid(pidControlador, NULL, 0);

    printf("KernelSim: todos os processos terminaram.\n");

    return 0;
}