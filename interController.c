#include <stdio.h> 
#include <signal.h> 
#include <stdlib.h> 

#define IO_SEC 3 
#define SIGNAL_PEDIDO_IO SIGUSR1
#define MAX_PEDIDOS 100

int filaIO[MAX_PEDIDOS]; 
int qtdIO = 0; 
int pedidos_recebidos = 0; 

//Recebe o aviso de pedido de Entrada e Saída vindo do KernelSim(escalonador.c) 
static void pedidoIO(int sig){
    pedidos_recebidos++; 

}

void gerar_IRQ0(pid_t pid){
    kill(pid, SIGUSR1); 
}

void gerar_IRQ1(pid_t pid){
    kill(pid, SIGUSR2);
}

//Adiciona o pedido na fila de bloqueados
void adicionarPedidoIO(void){
    if(quantidadeIO < MAX_PEDIDOS){
        filaIO[quantidadeIO] = IO_SEC; //guarda os segundos de deve ficar na fila de bloqueados
        quantidadeIO++; 
    }else{
        printf("Fila de Pedidos cheia")
    }
}
// decrementa o tempo de cada E/S
void atualizaIO(pid_t pid){
    int i = 0; 
    int disparou = 0; 

    while(i< quantidadeIO){

        if(filaIO[i] > 0){
            filaIO[i]--; 
        }

        if(filaIO[i] <= 0 && !disparou){
            printf("INTERCONTROLLERSIM: IRQ1 - E/S concluida\n");
            gerar_IRQ1(pid); 
            disparou = 1; 

            //Remove a operação já concluida
            for(int j = 1; j < quantidadeIO -1; j++){
                filaIO[j] = filaIO[j+1]; 
            }
            quantidadeIO--; 
        }

        else{
            i++; 
        }
    }

}

int main(int argc, char *argv[]){

    pid_t pid = (pid_t) atoi(argv[1]);

    signal(SIGUSR1, pedido);

    while(1){

        unsigned int falta = 1; 
        if (falta > 0){
            falta = sleep(falta); 
        }

        atualizaIO(pid); 

        while(pedidos_recebidos > 0){
            pedidos_recebidos--; 
            adicionarPedidoIO();
        }


        gerar_IRQ0(pid); 
    }

    return 0; 
    
}