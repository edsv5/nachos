// system.h
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "synch.h"

// Para la tabla de archivos abiertos

//#include "NachosOpenFilesTable.h"

// Para poder hacer read y write

#include "unistd.h"

// Para poder hacer los opens

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Para poder utilizar mapas y tener el mapa de semáforos del sistema

#include <map>

// Para poder utilizar los semáforos de UNIX

//#include <sys/ipc.h>
//#include <sys/sem.h>


// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock
//extern Semaphore* consoleS; // Semaforo para la consola
//extern NachosOpenFilesTable* openFilesTable; // Tabla global de archivos abiertos

#ifdef USER_PROGRAM
#include "machine.h"
// Agregado, para usar bitmap
#include "bitmap.h"

extern Machine* machine;	// user program memory and registers
//Agregado: BitMap como variable global
extern BitMap* mapMemoria; // Para representar las páginas de memoria

// Contador con la cantidad de semáforos que hay actualmente
extern int cantidadSemaforosNachos;

// Mapa que indica, el índice del semáforo, junto con su respectivo estado
extern std::map<int,int> *mapSemaforosNachos;

// Agregado: Que mapee índices con punteros a semáforos

//extern std::map<int, Semaphore* > *mapSemaforosNachos;


//Tabla global de files abiertos (prueba mientras se implementa la multiprogramación)
//extern NachosOpenFilesTable* openFilesTable;

#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
