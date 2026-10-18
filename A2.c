#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h> 
#include "common.h"

#define MAX 5

int main(int argc, char * argv[]) {
    int PC = 0;

    int idx = atoi(argv[1]); 
    int fd =  atoi(argv[2]); // numero da pipe 

    while (PC < MAX) {
        PC++;

        printf("A2: PC = %d\n", PC);
        fflush(stdout); 
        
        Msg m; 
        m.idx = idx; 
        m.pc = PC; 
        m.op = 0; 

        if(PC == 3){
            m.op = 'W'; 
        }

        //informa o PC para o kernel 
        write(fd, &m, sizeof(m)); 
        if(m.op != 0){
            kill(getppid(), SIG_SYSCALL); 
        }

        sleep(1); 
    }

    return 0;
}