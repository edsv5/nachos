// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////

void returnFromSystemCall() {

  int pc, npc;                                    // pc = contador actual, npc = contador siguiente
  pc = machine->ReadRegister( PCReg );            // ingresa el valor leyendo el registro con el valor del PC actual
  npc = machine->ReadRegister( NextPCReg );       // ingresa al npc el valor del pc siguiente
  machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
  machine->WriteRegister( PCReg, npc );           // PC <- NextPC
  machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4
}       // returnFromSystemCall


///////////////////////////////////////////////////////////////////////////////

/*
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2); //Lee el contenido de la interrupcion

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(false);
    }
} */

//-------------------- VERSIÓN CORREGIDA --------------------//

/////////////////////////// System call 0 ///////////////////////////

void Nachos_Halt() {

        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();

}       // Nachos_Halt



/////////////////////////// System call 1 ///////////////////////////

// Implementado en el case de abajo


/////////////////////////// System call 2 ///////////////////////////


void Nachos_Exec(){

  returnFromSystemCall();		// Update the PC registers

}


/////////////////////////// System call 4 ///////////////////////////
/* ---  Nachos_Read ---
 *
 * Esto recibe en el registro 4 el nombre del archivo que se quiere leer.
 * Se hace una traducción del nombre de este archivo con la función
 * ReadMem y se asigna dicha traducción, caracter por caracter, al arreglo
 * nombreFile, después de que este arreglo está completo, se utiliza la
 * función de UNIX creat, con el nombre del archivo que traducimos
 * y los permisos necesarios para poder hacerle read y write al archivo
 *
 */
void Nachos_Create(){


  // Dirección del archivo que se va a crear
  int nombreIngresado = machine->ReadRegister( 4 );
  // Aquí se va a guardar la traduccion del nombre
  char nombreFile [50];
  // Para detener la traduccion
  bool llegoAlfinal = false;
  // Cada char se guarda acá antes de asignárselo a nombreFile[i]
  int traduccion = 0;
  // Para que vaya iterando sobre el nombre
  int indice = 0;
  // Llamo a ReadMem para que traduzca
  while(!llegoAlfinal)
	{
		machine->ReadMem(nombreIngresado+indice,1,&traduccion);
    nombreFile[indice] = traduccion;

    // El final del nombre es un 0 (UNIX)
		if(traduccion == 0){
      llegoAlfinal = true;
    }

		indice++; // Sigue con la siguiente posicion del nombre
	}

  // Se utiliza el creat de UNIX

	creat(nombreFile,0777);

	returnFromSystemCall();		// Update the PC registers
}


/////////////////////////// System call 5 ///////////////////////////
/* ---  Nachos_Open ---
 *
 * Esto recibe en el registro 4 el nombre del archivo que se quiere abrir.
 * Se hace una traducción del nombre de este archivo con la función
 * ReadMem y se asigna dicha traducción, caracter por caracter, al arreglo
 * nombreFile, después de que este arreglo está completo, se utiliza la
 * función de UNIX open, con el nombre del archivo que traducimos
 * y las banderas necesarias para poder crear el archivo.
 * Devuelve el Unix handle del archivo nuevo.
 */
void Nachos_Open() {
  /* System call definition described to user
  	int Open(
  		char *name	// Register 4
  	);
  */

  // Dirección del archivo que se va a crear
  int nombreIngresado = machine->ReadRegister( 4 );
  // Aquí se va a guardar la traduccion del nombre
  char nombreFile [50];
  // Para detener la traduccion
  bool llegoAlfinal = false;
  // Cada char se guarda acá antes de asignárselo a nombreFile[i]
  int traduccion = 0;
  // Para que vaya iterando sobre el nombre
  int indice = 0;
  // Llamo a ReadMem para que traduzca
  while(!llegoAlfinal)
	{
		machine->ReadMem(nombreIngresado+indice,1,&traduccion);
    nombreFile[indice] = traduccion;

    // El final del nombre es un 0 (UNIX)
		if(traduccion == 0){
      llegoAlfinal = true;
    }

		indice++; // Sigue con la siguiente posicion del nombre
	}

  // Inicializa el descriptor del nuevo file
  OpenFileId fileId = 0;

  // Open de UNIX, O_APPEND porque queremos attachearle cosas ya que estamos abriéndolo
  fileId = open(nombreFile,O_RDWR | O_APPEND);

  // Devuelve el fid correspondiente

  machine->WriteRegister(2,fileId);

  // Read the name from the user memory, see 4 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors

  returnFromSystemCall();		// Update the PC registers

}       // Nachos_Open

/////////////////////////// System call 6 ///////////////////////////

/* ---  Nachos_Read ---
 *
 * Documentación del método:
 *
 * Read "size" bytes from the open file into "buffer".
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough
 * characters to read, return whatever is available (for I/O devices,
 * you should always wait until you can return at least one character).
 *
 * Se supone que debería leer una hilera y devolverla a memoria
 * principal
 *
 * Read(char *buffer, int size, OpenFileId id);
 */

void Nachos_Read() {

  int bufferAddr = machine->ReadRegister( 4 ); // Lee la dirección del buffer que se quiere leer
  int size = machine->ReadRegister( 5 ); // Tamaño del archivo por leer
  OpenFileId descriptorFile = machine->ReadRegister( 6 ); // Id del file que se quiere leer

  // Se crea un buffer del tamaño de lo que se va a leer

  char buffer[size]; // Guarda lo que se va a escribir en memoria
  int valPorEscribir = 0; // ya que WriteMem acepta ints, guardamos como int

  switch (descriptorFile) {
		case  ConsoleInput:	// Lee del input de la consola

      scanf("%s", buffer); // Lee y deja lo leído en buffer

      // Luego escribe ese buffer en memoria

      for(int i=0; i < size; i++)
      {
        valPorEscribir = (int) buffer[i]; // Convierte el char a int
        // Escribe en memoria en la posición apropiado (bufferAddr + i)
        machine->WriteMem(bufferAddr+i,1,valPorEscribir);
        if(valPorEscribir == 0){ // Si el char escrito es null (llega al final), sale
          i=size;
        }
      }

      // Retorna los bytes escritos en memoria

      machine->WriteRegister(2,size);

      //TODO: Preguntar si esto está bien
			break;
    case  ConsoleOutput: // No se puede hacer read del output
      printf("No se puede leer de console output. \n");
      machine->WriteRegister(2,-1);
      break;

		default: // Cualquier otro caso es que se lee de un archivo

      /* ************************** MÉTODO A ************************** TODO: Preguntar por qué se sale del vector?


      // Si el thread actual, en su addrspace tiene un archivo abierto en su tabla de archivos abiertos
      int idDelThreadActual = currentThread->getIdThread(); // Para claridad del código
      int bytesLeidos = 0; // Va a almacenar los bytes que leemos, si es que leemos
      bool archivoEstaAbierto = currentThread->space->openFilesTable->isOpened(descriptorFile, idDelThreadActual);

      if(archivoEstaAbierto)
      {
        //Obtenemos el file handle de UNIX para usar los llamados de UNIX

        int fileHandle = currentThread->space->openFilesTable->getUnixHandle(descriptorFile, idDelThreadActual );

        // Se lee utilizando el read de UNIX, se guarda en bytes leídos

        bytesLeidos = read(fileHandle, buffer, size);
        int escrito = 0;

        // En este ciclo escribe los datos en memoria principal

        for(int i = 0; i < size; i++){

          escrito = buffer[i];
          machine->WriteMem(bufferAddr + i, 1, escrito); // Va leyendo byte por byte
        }

        // Al final de la cadena, escribe un 0 para señalar que es el final de la cadena
        machine->WriteMem(bufferAddr+size,1,0);
        // Devuelve al final, la cantidad de bytes que se leyeron
        machine->WriteRegister(2,bytesLeidos);


      }

      */

      // ************************** MÉTODO B **************************


      int bytesLeidos = read(descriptorFile, buffer, size);
      int escrito = 0;

      // En este ciclo escribe los datos en memoria principal

      for(int i = 0; i < size; i++){

        escrito = buffer[i];
        machine->WriteMem(bufferAddr + i, 1, escrito); // Va escribiendo byte por byte
      }

      machine->WriteRegister(2,bytesLeidos);

      break;
    }

    returnFromSystemCall();

  // Fin
}


/////////////////////////// System call 7 ///////////////////////////

// Recibe una dirección a un buffer
// el tamano de la hilera
// id del file abierto

void Nachos_Write() {

// System call definition described to user
//        void Write(
//		char *buffer,	// Register 4
//		int size,	// Register 5
//		 OpenFileId id	// Register 6
//	);


  // Lee parámetros de los registros

  int bufIngresado = machine->ReadRegister(4);
  int tamBuf = machine->ReadRegister(5);
  OpenFileId descriptorFile = machine->ReadRegister(6);

  //Crea el buffer en el que va a guardar la traducción que lee de memoria de usuario

  char buffer[tamBuf];
  int traduccion; // Cada char se guarda acá en el for antes de asignárselo a buffer[i]

  // Llamo a ReadMem para que llene el buffer con los datos ingresados
  // bufIngresado: de donde leemos el texto, se le va sumando i cada ciclo para que avance 1 caracter
  // 1: Se usa 1 porque se lee de 1 en 1
  // &traduccion: donde se va a guardar dicho caracter antes de asignarlo a buffer[i]
  // Va traduciendo, espacio por espacio, los ints a chars

  for(int i = 0; i < tamBuf; i++){

    // Se le va sumando 1 a bufIngresado para que siga leyendo
    machine->ReadMem(bufIngresado + i, 1, &traduccion); // Va leyendo byte por byte
    buffer[i] = traduccion; // Traduce el número a char
  }


	// Need a semaphore to synchronize access to console
	//consoleS->P();

	switch (descriptorFile) {
		case  ConsoleInput:	// El usuario no puede escribir al input de la consola
			machine->WriteRegister( 2, -1 );
      printf("No puede escribir en el input de la consola. \n ");
			break;
		case  ConsoleError:
    case  ConsoleOutput: // Ambos casos se contemplan en 1
      printf("\nEscribiendo en consola... \n");
      printf("--------------------------------------\n");
      printf( "%s \n", buffer );
      printf("--------------------------------------\n");
      machine->WriteRegister( 2, tamBuf ); // Devuelve la cantidad de bytes escritos
		  break;
		/*case ConsoleError:	// This trick permits to write integers to console
			printf( "ERROR %d\n", machine->ReadRegister( 4 ) );
			break; */
		default:

      // All other opened files
      // Verify if the file is opened, if not return -1 in r2
      // Get the unix handle from our table for open files
      // Do the write to the already opened Unix file
      // Return the number of chars written to user, via r2

      // ************************** MÉTODO A ************************** TODO: Preguntar por qué se sale del vector?

      // Se recupera el id del thread para mayor claridad

      /*

      int idDelThreadActual = currentThread->getIdThread();
      bool archivoEstaAbierto = currentThread->space->openFilesTable->isOpened(descriptorFile, idDelThreadActual);

      //printf("idDelThreadActual: %d \n", idDelThreadActual);
      printf("Archivo abierto?: %d \n", archivoEstaAbierto);


      // "Verify if the file is opened, if not return -1 in r2"
      if(archivoEstaAbierto)
      {
        printf("Escribiendo en el archivo \n");
        //Obtenemos el file handle de UNIX para usar los llamados de UNIX
        // "Get the unix handle from our table for open files"
        // TODO: Por alguna razón, no está escribiendo el file
        // int fileHandle = currentThread->space->openFilesTable->getUnixHandle(descriptorFile, idDelThreadActual );

        // Se utiliza el write de UNIX para escribir el buffer al file descrito por el handle que
        // acabamos de obtener, tamBuf es la cantidad de bytes que escribe en el file

        // "Do the write to the already opened Unix file"
        write(descriptorFile ,buffer, tamBuf);

        // "Return the number of chars written to user, via r2"
        machine->WriteRegister( 2, tamBuf );

      }else{
        machine->WriteRegister( 2, -1 ); // Error
        printf("El archivo no está abierto \n");
      }

      printf("Se escribió en el archivo \n");

      */

      // ************************** MÉTODO B **************************

      write(descriptorFile, buffer, tamBuf);
      machine->WriteRegister( 2, tamBuf ); // Devuelve la cantidad de bytes escritos

			break;

	}


	// Update simulation stats, see details in Statistics class in machine/stats.cc
  //consoleS->V();




  returnFromSystemCall();		// Update the PC registers

}  // Nachos_Write


/////////////////////////// System call 8 ///////////////////////////

void Nachos_Close(){

  returnFromSystemCall();		// Update the PC registers

}

/////////////////////////// System call 9 ///////////////////////////


void Nachos_Fork(){


}

/////////////////////////// System call 10 ///////////////////////////


void Nachos_Yield(){

}



/////////////////////////// System call 11 ///////////////////////////

/* ---  Nachos_SemCreate ---

 * Este system call recibe en el registro 4 el valor de inicialización
 * del semáforo que se quiere crear. Este inserta en el mapa de semáforos
 * de NachOS un par (índice, valor de inicialización) que representa el semáforo
 * que se está creando

 */

// TODO: Por ahora, sólo controla la estructura que dice cuántos semáforos hay
// Averiguar cómo involucrarlos verdaderamente
void Nachos_SemCreate(){



  printf("Entrando a SemCreate\n");

  // Importante:
  // cantidadSemaforosNachos : variable global con la cantidad de semáforos activos
  // mapSemaforosNachos : mapa con los semáforos, asocia cada índice de semáforo
  // con un estado
  int initval = machine->ReadRegister(4); // Lee el parámetro 1

  // Inserta en el mapa de semáforos global, un semáforo inicializado en el valor del
  // initval. Insertaen cantidadSemaforosNachos porque es el índice del semáforo,
  // con initval porque es el valor ingresado en el parámetro
  //
  // NOTE: mapSemaforosNachos está en system.h

  //mapSemaforosNachos->insert(std::pair<int,int>(cantidadSemaforosNachos,initval));

  //TODO: Preguntar esto

  // CAMBIO, ahora se inserta en el mapa un semáforo asociado a un índice

  Semaphore* nuevoSem = new Semaphore("SemNuevo", initval);	// Crea un semáfoto con el initval especificado

  // Inserta en el mapa de semáforos

  mapSemaforosNachos->insert(std::pair<int,Semaphore*>(cantidadSemaforosNachos,nuevoSem));

  // Retorna en registro 2 el índice del semáforo recién creado

	machine->WriteRegister(2,cantidadSemaforosNachos);

  // Hay un semáforo más, aumenta el contador

  cantidadSemaforosNachos++;

  //TODO: Preguntar si esto está bien

  returnFromSystemCall();



}

/////////////////////////// System call 12 ///////////////////////////

/* ---  Nachos_SemDestroy ---
 *
 * Destruye el semáforo identificado por SemId, el cual lee del registro 4
 * Utiliza el erase del map para borrar el semáforo especificado por el id del
 * parámetro
 *
 */

void Nachos_SemDestroy(){

  int semId = machine->ReadRegister(4); // Saca el parámetro del registro

  // Destruye el semáforo

  mapSemaforosNachos->at(semId)->Destroy();

  //Elimina el semáforo correspondiente del mapa

	if((int) mapSemaforosNachos->erase(semId) > 0) // Si el borrado del mapa fue exitoso
 	{
		machine->WriteRegister(2,0); // Devuelve 0
	}
	else // Si no tiene éxito devuelve -1
  {
  	machine->WriteRegister(2,-1);
  }

  returnFromSystemCall();


}

/////////////////////////// System call 13 ///////////////////////////

/* ---  Nachos_SemSignal ---
 *
 *
 */

void Nachos_SemSignal(){

  // TODO: Preguntar si esto está bien


  int semId = machine->ReadRegister(4); // Saca el parámetro del registro

	if(mapSemaforosNachos->at(semId) != NULL ) // Si el semáforo existe
	{
    mapSemaforosNachos->at(semId)-> V(); // le da signal
		machine->WriteRegister(2,0); // Retorna exitoso
	}else{
    machine->WriteRegister(2,-1); // Retorna sin éxito
  }


  returnFromSystemCall();

}

/////////////////////////// System call 14 ///////////////////////////

/* ---  Nachos_SemWait ---
 *
 *
 */

void Nachos_SemWait(){

  // TODO: Preguntar si esto está bien


  int semId = machine->ReadRegister(4); // Saca el parámetro del registro

	if(mapSemaforosNachos->at(semId) != NULL ) // Si el semáforo existe
	{
    mapSemaforosNachos->at(semId)-> P(); // Pone a esperar al semáforo correspondiente
		machine->WriteRegister(2,0); // Retorna exitoso
	}else{
    machine->WriteRegister(2,-1); // Retorna sin éxito
  }


  returnFromSystemCall();

}


void
ExceptionHandler(ExceptionType which)
{
  int type = machine->ReadRegister(2); // Lee cual syscall es, en el R2 se encuentra esta informacion

  switch ( which ) {
    case SyscallException:
      switch ( type ) {
        case SC_Halt:                 // System call # 0
          Nachos_Halt();
          break;
        case SC_Exit:                 // System call # 1
          printf("--- SC_Exit ---\n");
          currentThread->Finish();    // Finaliza el thread actual
          break;
        case SC_Exec:                 // System call # 2
          printf("--- SC_Exec ---\n");                      // FALTA
          Nachos_Exec();
          break;
        case SC_Join:                 // System call # 3
          printf("--- SC_Join ---\n");                      // FALTA
          break;
        case SC_Create:               // System call # 4
          printf("--- SC_Create ---\n");
          Nachos_Create();
          break;
        case SC_Open:                 // System call # 5
          printf("--- SC_Open ---\n");                      // FALTA TERMINAR
          Nachos_Open();
          break;
        case SC_Read:                 // System call # 6
          printf("--- SC_Read ---\n");
          Nachos_Read();
          break;
        case SC_Write:                // System call # 7
          printf("--- SC_Write ---\n");
          Nachos_Write();
          break;
        case SC_Close:                // System call # 8
          printf("--- SC_Close ---\n");                      // FALTA
          Nachos_Close();
          break;
        case SC_Fork:                 // System call # 9
          printf("--- SC_Fork ---\n");                      // FALTA
          Nachos_Fork();
          break;
        case SC_Yield:                // System call # 10
          printf("--- SC_Yield ---\n");                      // FALTA
          Nachos_Yield();
          break;
        case SC_SemCreate:            // System call # 11
          printf("--- SC_SemCreate ---\n");
          Nachos_SemCreate();
          break;
        case SC_SemDestroy:           // System call # 12
          printf("--- SC_SemDestroy ---\n");
          Nachos_SemDestroy();
          break;
        case SC_SemSignal:            // System call # 13
          printf("--- SC_SemSignal ---\n");
          Nachos_SemSignal();
          break;
        case SC_SemWait:              // System call # 14
          printf("--- SC_SemWait ---\n");
          Nachos_SemWait();
          break;
        default:
          //printf("--- Case default ---\n");
          //printf("Unexpected syscall exception %d\n", type );
          ASSERT(false);
          break;
      }
      break;
    default:
      printf( "Unexpected exception %d\n", which );
      //ASSERT(FALSE);
      ASSERT(false);
      break;
  }
}
