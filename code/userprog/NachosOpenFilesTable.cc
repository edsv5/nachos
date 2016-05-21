
#include "NachosOpenFilesTable.h"

// Constructor de la tabla de archivos abiertos
/*
 * Esta clase controla los archivos abiertos de cada hilo a través de un bitmap
 * Cada addrspace (proceso) posee su propia instancia de esta clase para controlar sus archivos
 * Si el proceso crea hilos, estos deberían compartir la tabla de archivos abiertos
 * para que si piden abrir uno, se revise si este ya fue abierto anteriormente y sea
 * usado.
 *
*/

NachosOpenFilesTable::NachosOpenFilesTable(){

    openFiles = new int[5]; // 5 archivos abiertos como máximo, Inicializamos
    for(int i=0; i < 5; i++)
    {
	     openFiles[i] = 0;
    }

    vecMapsOpenFiles = new vector<BitMap*>; // Inicializamos el vector de bitmaps
    usage = 0; // Inicia en 0

    //openFiles[0] = 0;
    //openFiles[1] =1;

}

int NachosOpenFilesTable::getUnixHandle( int nachosHandle, int idThread)
{

  //Si el archivo existe && está abierto según el mapa del thread actual
	if(nachosHandle >= 0 && vecMapsOpenFiles->at(idThread)->Test(nachosHandle))
	{
		return openFiles[nachosHandle]; // Devuelve el handle del archivo correspondiente
	}
	else
		return -1; // Si no, devuelve -1
}

// Este método devuelve si un archivo está abierto
// por alguno de los threads activo
// Recibe el id del thread respectivo y el nachosHandle de un archivo
// Devuelve true si algún archivo está abierto en algún hilo
// Si no, devuelve 0

bool NachosOpenFilesTable::isOpened(int nachosHandle, int idThread){

  //return openFilesMaps->at(threadID)->Test(nachosHandle);
  //return openFilesMap->Test(nachosHandle);
  // Los métodos anteriores funcionan sin multiprogramación

  // Devuelve si el archivo está abierto, en el thread especificado

  return vecMapsOpenFiles->at(idThread)->Test(nachosHandle);

}

NachosOpenFilesTable::~NachosOpenFilesTable(){

  delete[] openFiles; // Borra los open files
	delete[] vecMapsOpenFiles; // Borra el map de los files abiertos

}
