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

#include "threadsTabla.h" //NUEVO
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

// NUEVO  tabla de threads activos actualmente

threadsTabla* threadsActivos = new threadsTabla();

///////////////////////////////////////////////////////////////////////////////

void returnFromSystemCall() {

  int pc, npc;                                    // pc = contador actual, npc = contador siguiente
  pc = machine->ReadRegister( PCReg );            // ingresa el valor leyendo el registro con el valor del PC actual
  npc = machine->ReadRegister( NextPCReg );       // ingresa al npc el valor del pc siguiente
  machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
  machine->WriteRegister( PCReg, npc );           // PC <- NextPC
  machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4
}       // returnFromSystemCall



//---------------------------NUEVO----------------------------------------

/** ---  ReadFromNachosMemory ---
 * Lee de la memoria virtual de NachOS.
 * @param virtualmemory Posición en la memoria virtual de donde se quiere
 *                      leer.
 * @return Devuelve el string leído de la memoria de NachOS.
 */

char * ReadFromNachosMemory(int virtualmemory){
	//buffer
	char* string = new char[100];
	//contadores y identificador de fin de string
	int fin = '\0';
	int value = 0;
	int posicion = 0;
	//Lee nombre de archivo
	do{
		machine->ReadMem(virtualmemory+posicion,1,&value);
		*(string + posicion) = value;
		posicion++;
	}while(value != fin);
	//finaliza string
	*(string + posicion) ='\0';
	//retorna string
	return string;
}

/** ---  startProcess ---
 * Efectúa lo mismo que machine->startProcess.
 */

void startProcess(const char *filename){
	OpenFile *executable = fileSystem->Open(filename);
	AddrSpace *space;

	if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
	}

	space = new AddrSpace(executable);
	currentThread->space = space;

	delete executable;			// close file

	space->InitRegisters();		// set the initial register values
	space->RestoreState();		// load page table register

	machine->Run();			// jump to the user progam
	ASSERT(false);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

/** ---  NachosForkThread ---
 * Inicializa los registros para el hilo nuevo que creó un fork
 * recibe un puntero a la rutina que se va a ejecutar.
 *
 * @param funcPtr Puntero a la rutina que se va a ejecutar.
 * @return No devuelve nada.
 */


void NachosForkThread(void* funcPtr){ //parametro es la direccion de la funcion que se va a correr en el nuevo Thread
	//DEBUG
  DEBUG('t', "Estableciendo registros para hilo %s: 0x%x...\n", currentThread->getName(), funcPtr);
  //AddrSpace actual
  AddrSpace* space = currentThread->space;
  //Inicializa registros y reestablece el estado delthread
  space->InitRegisters();
  space->RestoreState();


  //valor de retorno (syscall 4 - Exit por si falla el llamado)
  machine->WriteRegister(RetAddrReg, 4);
  //cast a int para meterlo a un registro
  int x = *((int*)(&funcPtr));
  //proxima instruccion a ejecutar
  machine->WriteRegister(PCReg, (long)funcPtr);
  //instruccion que sigue
  machine->WriteRegister(NextPCReg, x+4);
  printf("\n");
	//corre el programa
  machine->Run();
  //no ocupa volver del system call (ReturnFrom...)
  ASSERT(false);
}

//-------------------- VERSIÓN CORREGIDA --------------------//

/////////////////////////// System call 0 ///////////////////////////

/** ---  Nachos_Halt ---
 * Detiene la ejecución de NachOS.
 * @param Ninguno.
 * @return Nada.
 */

void Nachos_Halt() {

        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();

}       // Nachos_Halt

/////////////////////////// System call 1 ///////////////////////////


void Nachos_Exit(){

  currentThread->Finish();    // Finaliza el thread actual
  returnFromSystemCall();

}


/////////////////////////// System call 2 ///////////////////////////

/** ---  Nachos_Exec ---
 * Crea un nuevo thread y lo agrega a los threads activos, lee el
 * nombre del archivo ejecutable en el registro 4 y lo corre.
 *
 * @param name Nombre del archivo que se va a ejecutar.
 * @return No devuelve nada.
 */

void Nachos_Exec(){				//NUEVO

  // Crea el nuevo thread
	Thread* newT = new Thread("Thread for Exec");
	// Lo agrega a threads activos
	newT->SpaceId = threadsActivos->AddThread(newT);
	// Lee nombre del archivo ejecutable guardado en el registro 4
 	char* name = ReadFromNachosMemory(machine->ReadRegister(4));
 	// Corre el programa
	startProcess((const char*)name);

}
/////////////////////////// System call 3////////////////////////

/** ---  Nachos_Join ---
 *
 * Lee un identificador de un hilo y lo une con otro hilo.
 *
 * @param   id Identificador del hilo con el que se va a hacer
 *          Join.
 * @return  Devuelve el identificador del thread al que se le
 *          hizo el Join.
 */

void Nachos_Join(){
	// Crea semáforo
	Semaphore* s = new Semaphore("Join",0);
	// Lee con cual thread hacer el join
	int id = machine->ReadRegister(4);
	// Espera al join
	int identificador = threadsActivos->addJoin(currentThread,s,id);
	s->P();
	// Hace el join
	threadsActivos->delJoin(currentThread,s,identificador,id);
	// Devuelve id del thread
	machine->WriteRegister(2,identificador);
	returnFromSystemCall();
}

/////////////////////////// System call 4 ///////////////////////////

/** ---  Nachos_Create ---
 *
 * Crea un archivo con el nombre ingresado por parámetro.
 *
 * @param   nombreIngresado Nombre que se le quiere dar al archivo que
 *          se va a crear
 * @return  Si tiene éxito, devuelve el handle de UNIX del archivo
 *          recién creado, si falla, devuelve -1.
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

  // Se utiliza el creat de UNIX para la creación del archivo nuevo
  int handleNuevo = creat(nombreFile,0777);
  if(handleNuevo != -1){ // Si no falla en el creat
    printf("    Archivo '%s' creado.\n", nombreFile );
    printf("    UNIX handle: %d\n", handleNuevo);
    machine->WriteRegister(2,handleNuevo); // Devuelve el handle de UNIX del archivo recién creado
  }else{
    printf("Error al crear el archivo %s\n", nombreFile);
    machine->WriteRegister(2,-1); // Si falla, devuelve -1
  }

	returnFromSystemCall();		// Update the PC registers
}


/////////////////////////// System call 5 ///////////////////////////


/** ---  Nachos_Open ---
 *
 * Recibe en el registro 4 el nombre del archivo que se quiere abrir.
 * Se hace una traducción del nombre de este archivo con la función
 * ReadMem y se asigna dicha traducción, caracter por caracter, al
 * arreglo nombreFile, después de que este arreglo está completo, se
 * utiliza la función de UNIX open, con el nombre del archivo que
 * traducimos y las banderas necesarias para poder crear el archivo.
.*
 * @param   nombreIngresado El nombre del archivo que se quiere abrir.
 * @return  Si tiene éxito, devuelve el handle de UNIX del archivo
 *          recién abierto, si falla, devuelve -1.
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

  // Read the name from the user memory, see 4 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors

  if(fileId == -1){ // Si el archivo no existe, no hace nada
    printf("    El archivo '%s' no existe.\n", nombreFile);
  }else{ // Pero si existe, lo abre
    printf("    Archivo '%s' abierto.\n", nombreFile);
    printf("    UNIX handle: %d\n", fileId );
    //fileId = currentThread->space->openFilesTable->Open(fileId,currentThread->getArchivosAbiertosPorThread());
    fileId = currentThread->space->openFilesTable->Open(fileId,currentThread->getArchivosAbiertosPorThread());
  }

  // Devuelve el fid correspondiente

  machine->WriteRegister(2,fileId);
  returnFromSystemCall();		// Update the PC registers

}       // Nachos_Open

/////////////////////////// System call 6 ///////////////////////////

/** ---  Nachos_Read ---
 *
 * Lee, en el buffer recibido en el registro 4, la cantidad de bytes
 * recibida en el registro 5 del archivo cuyo NachOS handle se recibe
 * en el registro 6. Se escriben los datos leídos en memoria
 * principal. Si se recibe como handle un 0, lee del console input
 * un texto ingresado por el usuario. Si se recibe un 1, no hace
 * nada, pues no se puede leer del ConsoleOutput. En cualquier otro
 * caso (se quiere leer de un archivo) se revisa si el archivo está
 * abierto, si lo está, lee del mismo. Si no lo está, devuelve un
 * -1. Si está abierto, lo lee.
.*
 * @param   bufferAddr La dirección del buffer de memoria en donde se
 *          quiere leer el contenido del archivo.
 * @param   size Tamaño en bytes de lo que se quiere leer.
 * @param   descriptorFile NachOS handle del archivo del cual se quiere
 *          leer
 * @return  Si tiene éxito, devuelve la cantidad de bytes leídos. Si
 *          falla, devuelve -1.
 */

void Nachos_Read() {


  int bufferAddr = machine->ReadRegister( 4 ); // Lee la dirección del buffer que se quiere leer
  int size = machine->ReadRegister( 5 ); // Tamaño del archivo por leer
  OpenFileId descriptorFile = machine->ReadRegister( 6 ); // Id del file que se quiere leer

  // Se crea un buffer del tamaño de lo que se va a leer

  char buffer[size]; // Guarda lo que se va a escribir en memoria
  int valPorEscribir = 0; // ya que WriteMem acepta ints, guardamos como int
  int bytesEscritosEnMemoria = 0;

  switch (descriptorFile) {
 	case  ConsoleInput:	// Lee del input de la consola
      printf("    Leyendo de la consola...\n");
      printf("    > ");
      scanf("%s", buffer); // Lee y deja lo leído en buffer

      // Luego escribe ese buffer en memoria

      for(int i=0; i < size; i++)
      {
        valPorEscribir = (int) buffer[i]; // Convierte el char a int
        // Escribe en memoria en la posición apropiado (bufferAddr + i)
        machine->WriteMem(bufferAddr+i,1,valPorEscribir);
        bytesEscritosEnMemoria++;
        if(valPorEscribir == 0){ // Si el char escrito es null (llega al final), sale
         i=size;
        }
      }

      // Retorna los bytes escritos en memoria

      machine->WriteRegister(2,bytesEscritosEnMemoria);
      printf("    %d bytes leídos en memoria.\n", bytesEscritosEnMemoria );

		break;
    case  ConsoleOutput: // No se puede hacer read del output
      printf("    No se puede leer de console output. \n");
      machine->WriteRegister(2,-1);
      break;

	  default: // Cualquier otro caso es que se lee de un archivo
      // Si el file handle (NachOS) recibido es un -1 es porque se quiere leer de un archivo
      // que no existe.

      // Empieza en 0, si el archivo no está abierto o no existe, se deja en 0
      bool archivoEstaAbierto = 0;
      int bytesLeidos = 0;
      if(descriptorFile != -1){

        printf("    File handle (NachOS) del archivo por leer: %d...\n", descriptorFile );
        archivoEstaAbierto = currentThread->space->openFilesTable->isOpened(descriptorFile);
        //int bytesLeidos = read(descriptorFile, buffer, size);
        printf("    Está abierto?: %d\n", archivoEstaAbierto );

      }


      if(archivoEstaAbierto){

        //Obtenemos el file handle de UNIX para usar los llamados de UNIX

        int fileHandle = currentThread->space->openFilesTable->getUnixHandle(descriptorFile);
        printf("    NachOS handle: %d\n", descriptorFile);
        printf("    UNIX handle: %d\n", fileHandle);

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

        printf("    %d bytes leídos con éxito del archivo %d (UNIX) | %d (NachOS).\n", bytesLeidos, fileHandle, descriptorFile );

      }else{
        printf("    Error en el Read, el archivo no existe.\n");
        machine->WriteRegister(2,-1); // Si el archivo no estaba abierto
      }

      break;
    }

    returnFromSystemCall();

  // Fin
}


/////////////////////////// System call 7 ///////////////////////////

/** ---  Nachos_Write ---
 *
 * Escribe 'tamBuf' bytes del 'bufIngresado' en el archivo cuyo
 * descriptor corresponde a 'descriptorFile'. Esto incluye que se
 * quiera escribir en el archivo 0 (ConsoleInput, en cuyo caso da
 * error) o el archivo 1 (ConsoleOutput, escribe en la consola).
 * Cualquier otro handle recibido corresponde a un archivo en el
 * cual se desea escribir.
.*
 * @param   bufIngresado Corresponde a el buffer en el que se guarda
 *          el contenido de lo que se quiere escribir para
 *          escribirlo. Puede ser un string de texto o un arreglo
 *          de caracteres.
 * @param   tamBuf Cuántos bytes se quieren escribir del
 *          bufIngresado.
 * @param   descriptorFile NachOS handle del archivo del cual se quiere
 *          leer
 * @return  Si tiene éxito, devuelve la cantidad de bytes escritos.
 *          Si falla, devuelve un -1.
 */

void Nachos_Write() {

  // Lee parámetros de los registros

  int bufIngresado = machine->ReadRegister(4);
  int tamBuf = machine->ReadRegister(5);
  OpenFileId descriptorFile = machine->ReadRegister(6);

  printf("    NachOS handle: %d\n", descriptorFile );

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

	switch (descriptorFile) {
		case  ConsoleInput:	// El usuario no puede escribir al input de la consola
      // Console Input = 0
			machine->WriteRegister( 2, -1 );
      printf("    No puede escribir en el input de la consola. \n ");
			break;
		//case  ConsoleError:
    case  ConsoleOutput: // Ambos casos se contemplan en 1
      printf("    Escribiendo en consola... \n");
      printf("\n");
      printf("    < %s \n", buffer );
      printf("\n");
      machine->WriteRegister( 2, tamBuf ); // Devuelve la cantidad de bytes escritos
		  break;
		default:

      // All other opened files
      // Verify if the file is opened, if not return -1 in r2
      // Get the unix handle from our table for open files
      // Do the write to the already opened Unix file
      // Return the number of chars written to user, via r2
      //********* LISTO *********

      printf("    Escribiendo en archivo %d (NachOS handle) ...\n", descriptorFile);

      bool archivoEstaAbierto = currentThread->space->openFilesTable->isOpened(descriptorFile);

      if(archivoEstaAbierto){

        //Obtenemos el file handle de UNIX para usar los llamados de UNIX
        int unixFileHandle = currentThread->space->openFilesTable->getUnixHandle(descriptorFile);
        // Se escribe utilizando el read de UNIX
        write(unixFileHandle, buffer, tamBuf);
        // Devuelve al final, la cantidad de bytes que se leyeron
        machine->WriteRegister(2,tamBuf);

        printf("    Escritos %d bytes en archivo %d (UNIX) | %d (NachOS) con éxito.\n", tamBuf, unixFileHandle, descriptorFile);

      }else{

        machine->WriteRegister(2,-1); // Si el archivo no estaba abierto
      }
			break;

	}

  returnFromSystemCall();		// Update the PC registers

}


/////////////////////////// System call 8 ///////////////////////////

/** ---  Nachos_Close ---
 *
 * Escribe 'tamBuf' bytes del 'bufIngresado' en el archivo cuyo
 * descriptor corresponde a 'descriptorFile'. Esto incluye que se
 * quiera escribir en el archivo 0 (ConsoleInput, en cuyo caso da
 * error) o el archivo 1 (ConsoleOutput, escribe en la consola).
 * Cualquier otro handle recibido corresponde a un archivo en el
 * cual se desea escribir.
.*
 * @param   descriptorFile File handle de NachOS del archivo que se
 *          quiere cerrar.
 * @return  Devuelve 1 si tuvo éxito, devuelve -1 si no tuvo éxito.
 */

void Nachos_Close(){

  OpenFileId descriptorFile = machine->ReadRegister(4); // Lee el id del file que queremos cerrar
  bool archivoEstaAbierto = 0;
  // Si el descriptor es -1 es porque está tratando de cerrar un archivo que no existe
  if(descriptorFile != -1){
    archivoEstaAbierto = currentThread->space->openFilesTable->isOpened(descriptorFile);
    printf("    Cerrando archivo %d (NachOS)\n", descriptorFile);
    printf("    Archivo está abierto?: %d\n", archivoEstaAbierto );

  }else{
    printf("    El archivo que está tratando de cerrar no existe.\n");
  }

  // Primero pregunta si el archivo está abierto, si está abierto, utiliza el close de UNIX para cerrar el
  // archivo, luego hace las actualizaciones correspondientes en el bitmap de archivos abiertos
  // y el vector de fd's de los archivos abiertos, hace también el close de NachOS

  if (archivoEstaAbierto) {
    // Si está abierto, se hace el close de NachOS
    int handleUnix = currentThread->space->openFilesTable->Close(descriptorFile);
    printf("    UNIX handle: %d\n", handleUnix );
    // Se hace después el close de Unix con el handle de UNIX que nos devolvieron
    // Si da error, avisa
    if(close(handleUnix) == -1){
      printf("    No se pudo cerrar el archivo %d.\n", handleUnix);
      machine->WriteRegister(2,-1); // Si falla, devuelve -1
    }else{
      printf("    Archivo %d (UNIX) cerrado con éxito.\n", handleUnix);
      machine->WriteRegister(2,1); // Si tiene éxito, devuelve un 1
    }
  }else{
    printf("    No se pudo cerrar el archivo especificado\n");
  }

  returnFromSystemCall();		// Update the PC registers

}

/////////////////////////// System call 9 ///////////////////////////

/** ---  Nachos_Fork ---
 *
 * Crea un nuevo hilo que comparte codigo y datos con el hilo padre,
 * pero posee su propia pila. El parametro recuperado del registro 4,
 * contiene la dirección a una rutina.
.*
 * @param   (void*) machine->ReadRegister( 4 ) Corresponde a la
 *          dirección de una rutina que se quiere forkear.
 * @return  No devuelve nada
 */


void Nachos_Fork(){  		//NUEVO
DEBUG( 'u', "Entering Fork System call\n" );  //DEBUG
	//Crea un nuevo Thread
	Thread * newT = new Thread( "child to execute Fork code" );
	//Crea Addrspace para el thread, constructor asigna espacio al stack
	//copia variables compartidas (tabla de archivos abiertos, addrspace original, numPages+stack)
	newT->openFilesTable = currentThread->openFilesTable;

	newT->space = new AddrSpace( currentThread->space );
	//Llama al Fork
	newT->Fork( NachosForkThread, (void*) machine->ReadRegister( 4 ) );  //cast to pointer from integer of different size
	// Llama Yield
	currentThread->Yield();
	//incrementa PC
	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC

	DEBUG( 'u', "Exiting Fork System call\n" );		//DEBUG
}

/////////////////////////// System call 10 ///////////////////////////

/** ---  Nachos_Yield ---
 *
 * Cede el control del CPU, si hay otro hilo activo.
.*
 * @param   El método recibe en el registro 4 el id del hilo activo
 * @return  No devuelve nada
 */

void Nachos_Yield(){		//NUEVO
	DEBUG('p', "%s esta cediendo\n", currentThread->getName());
  currentThread->Yield();
  machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
}


/////////////////////////// System call 11 ///////////////////////////

/** ---  Nachos_SemCreate ---
 *
 * Recibe en el registro 4 el valor de inicialización del semáforo
 * que se quiere crear. Inserta en el mapa de semáforos de NachOS
 * un par (índice, valor de inicialización) que representa el
 * semáforo que se está creando.
.*
 * @param   initval Valor inicial del semáforo que se desea crear.
 * @return  Cantidad de semáforos en el vector de semáforos de
 *          NachOS después del SemCreate.
 */

void Nachos_SemCreate(){

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

  // CAMBIO, ahora se inserta en el mapa un semáforo asociado a un índice

  Semaphore* nuevoSem = new Semaphore("SemNuevo", initval);	// Crea un semáfoto con el initval especificado

  // Inserta en el mapa de semáforos

  printf("    Semáforo creado. Valor inicial: %d \n", initval );
  printf("    Cantidad de semáforos antes del SemCreate: %d \n", cantidadSemaforosNachos );

  mapSemaforosNachos->insert(std::pair<int,Semaphore*>(cantidadSemaforosNachos,nuevoSem));
  printf("    Semáforo insertado en la posición %d del vector de semáforos. \n", cantidadSemaforosNachos );

  // Retorna en registro 2 el índice del semáforo recién creado

	machine->WriteRegister(2,cantidadSemaforosNachos);

  // Hay un semáforo más, aumenta el contador

  cantidadSemaforosNachos++;

  printf("    Cantidad de semáforos después del SemCreate: %d \n", cantidadSemaforosNachos );

  returnFromSystemCall();

}

/////////////////////////// System call 12 ///////////////////////////

/** ---  Nachos_SemDestroy ---
 *
 * Destruye el semáforo identificado por SemId, el cual lee del registro 4
 * Utiliza el erase de map para borrar el semáforo especificado por el id del
 * parámetro
.*
 * @param   semId Índice del semáforo que se quiere destruir.
 * @return  Si tiene éxito, devuelve 0, si falla, devuelve -1.
 */

void Nachos_SemDestroy(){

  int semId = machine->ReadRegister(4); // Saca el parámetro del registro

  // Destruye el semáforo
  mapSemaforosNachos->at(semId)->Destroy();

  printf("    Semáforo %d destruido.\n", semId);

  //Elimina el semáforo correspondiente del mapa

	if((int) mapSemaforosNachos->erase(semId) > 0) // Si el borrado del mapa fue exitoso
 	{
		machine->WriteRegister(2,0); // Devuelve 0
    cantidadSemaforosNachos--; // Decrementa la cantidad de semáforos
	}
	else // Si no tiene éxito devuelve -1
  {
    printf("    Error en el SemDestroy.\n");
  	machine->WriteRegister(2,-1);
  }

  printf("    Cantidad de semáforos: %d\n", cantidadSemaforosNachos);

  returnFromSystemCall();


}

/////////////////////////// System call 13 ///////////////////////////

/* ---  Nachos_SemSignal ---
 *
 * Le da Signal al semáforo especificado por el Id que ingresa en
 * el registro 4.
 *
 * @param   semId Índice del semáforo al que se le quiere hacer
 *          signal.
 * @return  Si tiene éxito, devuelve 0, si falla, devuelve -1.
 */

void Nachos_SemSignal(){

  // TODO: Preguntar si esto está bien

  int semId = machine->ReadRegister(4); // Saca el parámetro del registro

	if(mapSemaforosNachos->at(semId) != NULL ) // Si el semáforo existe
	{
    printf("    Haciendo signal en semáforo %d.\n", semId );
    mapSemaforosNachos->at(semId)-> V(); // le da signal
		machine->WriteRegister(2,0); // Retorna exitoso
	}else{
    printf("Error en SemSignal\n" );
    machine->WriteRegister(2,-1); // Retorna sin éxito
  }

  returnFromSystemCall();

}

/////////////////////////// System call 14 ///////////////////////////

/* ---  Nachos_SemWait ---
 *
 * Le hace Wait al semáforo cuyo Id es el mismo del parámetro leído
 * en el registro 4.
 *
 * @param   semId Índice del semáforo al que se le quiere hacer
 *          wait.
 * @return  Si tiene éxito, devuelve 0, si falla, devuelve -1.
 */

void Nachos_SemWait(){

  int semId = machine->ReadRegister(4); // Saca el parámetro del registro

	if(mapSemaforosNachos->at(semId) != NULL ) // Si el semáforo existe
	{
    printf("    Haciendo wait en semáforo %d.\n", semId );
    mapSemaforosNachos->at(semId)-> P(); // Pone a esperar al semáforo correspondiente
		machine->WriteRegister(2,0); // Retorna exitoso
	}else{
    printf("Error en SemWait\n");
    machine->WriteRegister(2,-1); // Retorna sin éxito
  }

  returnFromSystemCall();

}

void Nachos_PageFault(){
	printf("PageFaultException: pagina %d\n", machine->ReadRegister(39));
	int direccion;  //direccion en memoria
	//interrupt->Halt();
}


void
ExceptionHandler(ExceptionType which)
{
  int type = machine->ReadRegister(2); // Lee cual syscall es, en el R2 se encuentra esta informacion

  switch ( which ) {
    case SyscallException:
      switch ( type ) {
        case SC_Halt:                 // System call # 0
          printf("*** SC_Halt ***\n");
          Nachos_Halt();
          break;
        case SC_Exit:                 // System call # 1
          printf("*** SC_Exit ***\n");
          Nachos_Exit();
          break;
        case SC_Exec:                 // System call # 2
          printf("*** SC_Exec ***\n");
          Nachos_Exec();
          break;
        case SC_Join:                 // System call # 3
          printf("*** SC_Join ***\n");
          Nachos_Join();
          break;
        case SC_Create:               // System call # 4
          printf("*** SC_Create ***\n");
          Nachos_Create();
          break;
        case SC_Open:                 // System call # 5
          printf("*** SC_Open ***\n");
          Nachos_Open();
          break;
        case SC_Read:                 // System call # 6
          printf("*** SC_Read ***\n");
          Nachos_Read();
          break;
        case SC_Write:                // System call # 7
          printf("*** SC_Write ***\n");
          Nachos_Write();
          break;
        case SC_Close:                // System call # 8
          printf("*** SC_Close ***\n");
          Nachos_Close();
          break;
        case SC_Fork:                 // System call # 9
          printf("*** SC_Fork ***\n");
          Nachos_Fork();
          break;
        case SC_Yield:                // System call # 10
          printf("--- SC_Yield ---\n");
          Nachos_Yield();
          break;
        case SC_SemCreate:            // System call # 11
          printf("*** SC_SemCreate ***\n");
          Nachos_SemCreate();
          break;
        case SC_SemDestroy:           // System call # 12
          printf("*** SC_SemDestroy ***\n");
          Nachos_SemDestroy();
          break;
        case SC_SemSignal:            // System call # 13
          printf("*** SC_SemSignal ***\n");
          Nachos_SemSignal();
          break;
        case SC_SemWait:              // System call # 14
          printf("*** SC_SemWait ***\n");
          Nachos_SemWait();
          break;
        default:
          printf("*** Case default ***\n");
          printf("Unexpected syscall exception %d\n", type );
          ASSERT(false);
          break;
      }
      break;
    case PageFaultException:
		Nachos_PageFault();
		ASSERT(false);
		break;
	case AddressErrorException:
		printf("AddressErrorException\n");
		ASSERT(false);
		break;
    break;
    default:
      printf( "Unexpected exception %d\n", which );
      ASSERT(false);
      break;
  }
}
