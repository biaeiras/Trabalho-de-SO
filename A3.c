#include <stdio.h>
#include <unistd.h>

#define MAX 5

int main(void) {
    int PC = 0;

    while (PC < MAX) {
        PC++;

        printf("A3: PC = %d\n", PC);
        sleep(1);
    }

    return 0;
}