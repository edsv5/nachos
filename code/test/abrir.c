
#include <syscall.h>
int main(){

        int fileID = 1;

	Write("Creando\n",7,1);
	Create("RH.1337");
	fileID = Open("RH.1337");
	Write( "I'm a n00b", 10, fileID);
        Halt();
}
