// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}


//-------------------------------REFERENCIA DE ReadAt---------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//
//int OpenFile::ReadAt(char *into, int numBytes, int position)
//
//	Este método lee del disco algún archivo que está abierto y lo
//	coloca en alguna posición de memoria
//-------------------------------REFERENCIA DE ReadAt---------------------------------------

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------


AddrSpace::AddrSpace(OpenFile *executable)
{
  NoffHeader noffH;
  unsigned int i, size;

	// NUEVO : Cada address space inicializa su propia tabla de archivos abiertos

	openFilesTable = new NachosOpenFilesTable();

	// El programa es leído del disco aquí, se comienza por leer el header del archivo, de ahí se saca toda la información importante sobre este

  executable->ReadAt((char *)&noffH, sizeof(noffH), 0);

	//printf("El tamaño del encabezado es %d \n", sizeof(noffH));

  if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    SwapHeader(&noffH);
  ASSERT(noffH.noffMagic == NOFFMAGIC);

	// how big is address space? - Aquí asigna el tamano del addrspace
  size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;	// we need to increase the size
	// to leave room for the stack
	// Aquí se contempla que los segmentos no sean múltiplos exactos de una página (creo), redondea para arriba
  numPages = divRoundUp(size, PageSize);
  size = numPages * PageSize;

	//printf("Tamaño del page: %d\n",PageSize );

	//tamaño del proceso = tamaño del address space
	//printf("El tamaño del address space es %d \n", size);
	//El número mágico solamente sirve para identificar el formato NOFF, es simplemente un identificador arbitrario, aquí se imprime
	//printf("El número mágico es %d \n", NOFFMAGIC);
	//La pila es una constante que está definida en addrspace.h, en este caso, el largo es 1024, es decir, 8 páginas
	//printf("El tamaño de la pila es %d \n", UserStackSize);


	//Pregunta si esa condición se da, en el caso de que se dé, no sigue

  ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

  DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, size);
// first, set up the translation

  pageTable = new TranslationEntry[numPages];
  for (i = 0; i < numPages; i++) {
		//printf("Pagina actual: %d\n", i);
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = mapMemoria->Find(); // Iguala la página al primer bit que esté libre del bitmap de memoria
		//pageTable[i].physicalPage = i;
		pageTable[i].valid = true;
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
		// if the code segment was entirely
		// a separate page, we could set
		// pages to be read-only
  }

// zero out the entire address space, to zero the unitialized data segment
// and the stack segment
  bzero(machine->mainMemory, size);

// Aquí es que empieza a leer del disco, en el primer ReadAt leímos el encabezado
// en estos se lee lo demás y se coloca en memoria para ser ejecutado

// then, copy in the code and data segments into memory
  if (noffH.code.size > 0) {
    DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
		noffH.code.virtualAddr, noffH.code.size);

    executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
		noffH.code.size, noffH.code.inFileAddr);

		//printf("Cantidad de code leída: %d\n", noffH.code.size);
		//printf("Posición de lectura del code: %d\n", noffH.code.inFileAddr);
		//printf("Posición en la memoria en la que se coloca lo leído: %s \n", &(machine->mainMemory[noffH.code.virtualAddr]) );

  }
  if (noffH.initData.size > 0) {
    DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
		noffH.initData.virtualAddr, noffH.initData.size);

		//Se agrega esto para que imprima//
		//printf('a', "Initializing data segment, at 0x%x, size %d\n", noffH.initData.virtualAddr, noffH.initData.size);
		//Se agrega esto para que imprima//

		executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
		noffH.initData.size, noffH.initData.inFileAddr);

		//printf("Cantidad de initData leída: %d\n", noffH.initData.size);
		//printf("Posición de lectura de initData: %d\n", noffH.initData.inFileAddr);
		//printf("Posición en la memoria en la que se coloca lo leído: %s \n", &(machine->mainMemory[noffH.initData.virtualAddr]) );

		//La dirección del buffer en el que se colocan las cosas está en &(machine->mainMemory[noffH.initData.virtualAddr])

  }
}






//--------------------------NUEVOS_GETTERS------------------------------
unsigned int AddrSpace::getNumPages(){
	return numPages;
}

TranslationEntry* AddrSpace::getPageTable(){
	return pageTable;
}

//-------------------NUEVO_CONSTRUCTOR----------------------------------

AddrSpace::AddrSpace(AddrSpace* space){
	//calcula las paginas que ocupa para el stack
	int newNumPages = divRoundUp(UserStackSize, PageSize);
	//PageTable "padre" de donde se copian los datos
	TranslationEntry* padre = space->getPageTable();

	// AÑADIDO
	//El OpenFTable es el mismo del currentThread
	openFilesTable = space->openFilesTable;

	//verifica que las paginas usadas mas las del stack no sean mas que las paginas disponibles
	if(numPages + newNumPages <= NumPhysPages){
		//espacio suficiente
		//crea PageTable con espacio para el stack
		TranslationEntry *newPageTable = new TranslationEntry[numPages + newNumPages];
		int i;
		//copia los datos del "padre"
		for(i = 0; i < numPages; i++){
			newPageTable[i] = padre[i];
		}
		//espacio del stack
		for(i = numPages; i < newNumPages; i++){
			newPageTable[i].virtualPage = i;
			newPageTable[i].physicalPage = i;
			newPageTable[i].valid = true;
			newPageTable[i].use = false;
			newPageTable[i].dirty = false;
			newPageTable[i].readOnly = false;
		}
		//nuevos valores para los atributos
		pageTable = newPageTable;		//PageTable con stack
		numPages = (numPages + newNumPages);		//paginas usadas anteriormente mas las paginas de stack

	}else{
		//espacio insuficiente
		DEBUG('a', "Espacio insuficiente para pila\n");
	}
}








//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

  for (i = 0; i < NumTotalRegs; i++)
		machine->WriteRegister(i, 0);

  // Initial program counter -- must be location of "Start"
  machine->WriteRegister(PCReg, 0);

  // Need to also tell MIPS where next instruction is, because
  // of branch delay possibility
  machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
  machine->WriteRegister(StackReg, numPages * PageSize - 16);
  DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
