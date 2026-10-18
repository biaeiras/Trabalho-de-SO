// Evita que o cabecalho seja incluido mais de uma vez
#ifndef COMMON_H
#define COMMON_H 

#include <signal.h> 

#define SIG_SYSCALL SIGALRM //Avisa que o A(1,2 ou 3) fez syscall

typedef struct {
    int idx; //indica qual é o processo (A1 = 0, A2 = 1, A3= 2)
    int pc; //contador de iterações 
    char op; //'R' ou 'W' passado pelo syscall ou 0 se não tiver syscall
} Msg; 


#endif