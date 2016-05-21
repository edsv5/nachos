/*

Programa para probar system call de Write, utiliza el system call de NachOS para Write

*/

#include "syscall.h"

int
main()
{
    char *  buff = "Hola Mundo \n";
    Write(buff, 10, 1);
    Exit(0);
}
