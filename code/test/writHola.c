#include "syscall.h"

int main() {
    char *buff = "Estamos probando el Write.\n";
    Write(buff, 27, 1);
    Exit(5);
}
