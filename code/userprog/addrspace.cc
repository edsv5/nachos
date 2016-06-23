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
//#include "noff.h"

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
    //Cada address space inicializa su propia tabla de archivos abiertos
    openFilesTable = new NachosOpenFilesTable();

    // El programa es leído del disco aquí, se comienza por leer el header del archivo, de ahí se saca toda la información importante sobre este
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
    
    threadHeader.code = noffH.code;
    threadHeader.initData = noffH.initData; 
    

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
						
	// Aquí se contempla que los segmentos no sean múltiplos exactos de una página (creo), redondea para arriba					
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
   

    DEBUG('j', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);

// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
	int freePage;

    for (i = 0; i < numPages; i++) {
		
	pageTable[i].virtualPage = i;
	 DEBUG('j', "Initializing page table, i %d, pageTable[]i.virtualPage %d\n", 
					i,pageTable[i].virtualPage);
	#ifdef VM
	
	#else
		freePage = mapMemoria->Find();
		pageTable[i].physicalPage = freePage;		
	#endif 
	#ifdef VM
		pageTable[i].valid = false;
  	#else
     	pageTable[i].valid = true;
  	#endif
	pageTable[i].use = false;
	pageTable[i].dirty = false;
	pageTable[i].readOnly = false;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
					
    }
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
   // bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory

	 #ifdef VM
		
	#else
		for(unsigned int j = 0;j<numPages;j++){
			int page = pageTable[j].physicalPage;
			executable->ReadAt(&(machine->mainMemory[128 * page]),
					   128, noffH.code.inFileAddr + j * 128);
			DEBUG('M',"En memoria esta %d",	&(machine->mainMemory[128 * page]));	
		}
	#endif
}






//Getters
unsigned int AddrSpace::getNumPages(){
	return numPages;
}

TranslationEntry* AddrSpace::getPageTable(){
	return pageTable;
}

////constructor con parametro AddrSpace

AddrSpace::AddrSpace(AddrSpace* space){
	
	numPages = space->getNumPages();

	//Crea TranslationTable para pageTable
    pageTable = new TranslationEntry[numPages];
	int freePage;
	unsigned int i;
    TranslationEntry* pageTableFather = space->getPageTable();
	
	   DEBUG('j', "Initializing address space, num pages %d, size %d\n", 
					numPages, UserStackSize);
	//Copia pageTable de space
    for (i = 0; i < numPages - 8; i++) {
	pageTable[i].virtualPage = i;
	pageTable[i].physicalPage = pageTableFather[i].physicalPage;
	pageTable[i].valid = true;
	pageTable[i].use = false;
	pageTable[i].dirty = false;
	pageTable[i].readOnly = false;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
	//Crea espacio nuevo para el Thread
  for( i; i< numPages;i++){
	pageTable[i].virtualPage = i;
	freePage = mapMemoria->Find();
	pageTable[i].valid = true;
	pageTable[i].use = false;
	pageTable[i].dirty = false;
	pageTable[i].readOnly = false; 
	if(-1 != freePage){
		pageTable[i].physicalPage = freePage;
		//printf("Free Page %d \n",freePage);
		 // if the code segment was entirely on 				
		bzero((void*)&machine->mainMemory[128 * pageTable[i].physicalPage], 128);
	}
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
	#ifdef VM
	
	#else
		machine->pageTable = pageTable;
		machine->pageTableSize = numPages;
    #endif
}
