// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create several threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
//

//Includes que ya venían

#include <unistd.h>
#include "copyright.h"
#include "system.h"
#include "dinningph.h"
#include "synch.h"

//Para usar memoria compartida

#include <sys/ipc.h>
#include <sys/shm.h>

//Para poder hacer wait

#include <sys/types.h>
#include <sys/wait.h>

//Para poder usar time()

#include <time.h>

//Para usar en los ciclos

#define N 5


DinningPh * dp;

void Philo( void * p ) {

    int eats, thinks;
    long who = (long) p;

    currentThread->Yield();

    for ( int i = 0; i < 10; i++ ) {

      printf(" Philosopher %ld will try to pickup sticks\n", who + 1);

      dp->pickup( who );
      dp->print();
      eats = Random() % 6;

      currentThread->Yield();
      sleep( eats );

      dp->putdown( who );

      thinks = Random() % 6;
      currentThread->Yield();
      sleep( thinks );
    }

}


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//      debugging purposes.
//----------------------------------------------------------------------

Semaphore* s = new Semaphore("Nombre", 0); // Se crea el semáforo con estado inicial de 0 para que espere

void
SimpleThread(void* name)
{
    // Reinterpreta name como un string
    char* threadName = (char*)name;

    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.

    // Imprime 10 veces lo siguiente:

    //      *** CICLO FOR: [nombre del hilo actual] loopeando por vez [num (índice)]

    for (int num = 0; num < 10; num++) {
      //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	    printf("*** CICLO FOR: %s loopeando por vez %d  \n", threadName, num);
	    //interrupt->SetLevel(oldLevel);
      //currentThread->Yield();
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> HILO %s terminé\n", threadName); //Una vez que el hilo termina, imprime que el hilo [threadName] terminó
    //interrupt->SetLevel(oldLevel);

    s->V(); //Hace signal para que sigan los siguientes hilos
}



//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several threads, by launching
//	ten threads which call SimpleThread, and finally calling
//	SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    Thread * Ph; //Se crea el hilo

    DEBUG('t', "Probando ThreadTest");
/*
    dp = new DinningPh();

    for ( long k = 0; k < 5; k++ ) {
        Ph = new Thread( "dp" );
        Ph->Fork( Philo, (void *) k );
    }

    return;
*/
    // Crea 5 hilos, después de esto, se ejecuta SimpleThread, debería ser ejecutado en los 5 hilos

    for ( int k=1; k<=5; k++) {

      char* threadname = new char[100];   // Crea el nombre del hilo
      sprintf(threadname, "Hilo %d", k);      // Imprime: Hilo (número de iteración)
      Thread* newThread = new Thread (threadname);                      // Crea el hilo con su nombre respectivo
      newThread->Fork (SimpleThread, (void*)threadname);                // Pone el hilo a forkear

      s->P(); //Hace wait hasta que el proceso hijo le da signal

    }
    //SimpleThread( (void*)"Llamando a SimpleThread en Hilo 0");
}

//Se crean los semáforos necesarios

Semaphore* sO = new Semaphore("Oxigeno", 1);	// Inicializado en 1
Semaphore* sH = new Semaphore("Hidrogeno", 1);	// Inicializado en 1

//Se declaran las variables compartidas

struct Compartidos {
  int nO;
  int nH;
};


typedef struct Compartidos Compartir;
Compartir * pComp;
int id = 0;

// Métodos para crear oxígeno y hidrógeno

void O( int i ){

	if ( pComp->nH > 1 ) {
    printf( "                                      %d: | H's: %d | O's: %d |\n",getpid(), pComp->nH, pComp->nO + 1 ); //Lo imprime con 1 para que sea más visual
    printf( "                                      ********* %d: Molécula de H2O creada por un O [%d] ********* \n",getpid(), i );
	  sH->V(); //Signal
	  sH->V();
	  pComp->nH -= 2; //Se creó agua entonces hay dos moléculas menos de H
    printf( "                                      %d: Después de crear agua, | H's: %d | O's: %d | Signal a dos semáforos de H\n",getpid(), pComp->nH, pComp->nO );
	} else {
	  pComp->nO++; // Si no se crea agua, agrega una molécula de O
    printf( "                                      %d: O no creó agua, | H's: %d | O's: %d | Esperando...\n",getpid(), pComp->nH, pComp->nO );
	  sO->P();      //Wait hasta que la creación de hidrógeno le dé el signal
	}

}

void H( int i ){

	if ( ( pComp->nH > 0) && (pComp->nO > 0) ) {
    printf( "                                      %d: | H's: %d | O's: %d |\n",getpid(), pComp->nH + 1, pComp->nO );
    printf( "                                      ********* %d: Molécula de H2O creada por un H [%d] ********* \n", getpid(),i );
	  sH->V(); //Siga creando moléculas de hidrógeno
	  sO->V(); //Siga creando moléculas de oxígeno
	  pComp->nH --; //Como hizo agua, decremente dos en las moléculas
	  pComp->nO --;
    printf( "                                      %d: Después de crear agua, | H's: %d | O's: %d | Signal a semáforo de H y O\n",getpid(), pComp->nH, pComp->nO );
	} else {
	  pComp->nH++; //En caso contrario aumente 1 hidrógeno, pare y cree oxígeno
    printf( "                                      %d: H no creó agua, | H's: %d | O's: %d | Esperando...\n",getpid(), pComp->nH, pComp->nO );
	  sH->P();
	}
}

//Aquí corre el "main"

void Agua(){

  //printf("\nREAL NIGGA\n\n");

  int i = 0;

  // Creacion de las variables compartidas
  id = shmget( 0xABCDEF, sizeof( Compartir ), 0600 | IPC_CREAT );



  if ( -1 == id ) {
    perror( "main" );
    exit( 0 );
  }

  pComp = ( Compartir * ) shmat( id, NULL, 0 );

  ///////////// EJECUCIÓN DE LA SOLUCIÓN AL PROBLEMA DEL AGUA /////////////

  srand( time( NULL ) );  // Genera una nueva semilla para los aleatorios

  while( i++ < N ) {
    int r = rand(); // Se crea un número aleatorio
    //printf( "rand = %d\n", r );
    if ( ! fork() ) { // Es el hijo
      if ( r % 2 ) { // 50% de probabilidad de que cree oxígeno
        printf( "                                      %d: Se creo un atomo de oxigeno [%d]\n",getpid(), i );
        O( i );
      } else {  //50% de que cree hidrógeno
        printf( "                                      %d: Se creo un atomo de hidrogeno [%d]\n",getpid(), i );
        H( i );
      }
    }else{
      //printf("---- Proceso padre ----\n");
    }
  }


  ///////////// EJECUCIÓN DE LA SOLUCIÓN AL PROBLEMA DEL AGUA /////////////

  i = 0;
  int k, n;
  while ( i < N ) {
    printf("Esperando por proceso %d \n", i );
    n = wait( &k ); //Cierra los procesos
    i++;
  };

  printf( "Destruyendo los recursos de memoria compartida\n");
  shmdt( pComp );
  shmctl( id, IPC_RMID, NULL );

}
