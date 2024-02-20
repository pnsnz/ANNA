#include <stdio.h>
#include <string.h>

int main(void){

    int x = 2;
    int *xPtr;
    xPtr = &x;
    printf("%d\n", *xPtr);



    return 0;
}