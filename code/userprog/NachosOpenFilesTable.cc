
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

    openFiles = new int[15]; // 15 archivos abiertos como máximo, Inicializamos
    for(int i=0; i < 15; i++)
    {
	     openFiles[i] = 0;
    }

    vecMapsOpenFiles = new vector<BitMap*>; // Inicializamos el vector de bitmaps
    usage = 0; // Inicia en 0

    //openFiles[0] = 0;
    //openFiles[1] =1;

}

// Este método revisa todos los threads a ver si un archivo está abierto
// Si está abierto, pero en otro thread, se abre en el thread actual (correspondiente a idThread)
// Si no está abierto, se le asigna un fileHandle y se actualiza openFiles y vecMapsOpenFiles

int NachosOpenFilesTable::Open(int UnixHandle, int idThread)
{


  // ---------------- Primero se revisa si el archivo no estaba abierto por otro thread ----------------
  // Se crea el handle
  int handle;
	bool reAbrirArchivo=false;

	// Se recorren todos los threads (primer for)
	for(int indiceThead = 0;indiceThead < (int)vecMapsOpenFiles->size(); indiceThead++)
	{
    // Se recorren todos los archivos de cada thread (con el limite declarado antes)
		for(int indiceArchivo=0; indiceArchivo < limiteDeArchivosAbiertos; indiceArchivo++)
		{

      bool archivoEstaAbierto = isOpened(indiceArchivo, indiceThead); //  (vecMapsOpenFiles->at(i)->Test(j));
      bool handlesSonIguales = (openFiles[indiceArchivo] == UnixHandle);

			if(archivoEstaAbierto && handlesSonIguales)
			{
        reAbrirArchivo=true;
				handle = indiceArchivo; // Se devuelve este handle
			}

		}
	}

  // Si el archivo estaba abierto en otro thread, se reabre y se devuelve el handle del file
	if(reAbrirArchivo)
	{
    // Se asigna en el handle del file del thread actual con el Mark

    vecMapsOpenFiles->at(idThread)->Mark(handle);
    // Se retorna el handle
		return handle;
	}

  // Si el archivo no estaba abierto por ningún thread se asigna un descriptor nuevo

	handle = vecMapsOpenFiles->at(idThread)->Find();

	// Si se logró asignar el nuevo descriptor, se cambia el openFiles y el vecMapsOpenFiles
	if(handle != -1)
	{
    // Si es -1, significa que está vacío ese espacio
		if(openFiles[handle] == -1)

			openFiles[handle] = UnixHandle; // ASIGNACIÓN DEL PARÁMETRO INGRESADO AL HANDLE

		else
		{
      // Si hay algo ahí, se limpia y se asigna
      vecMapsOpenFiles->at(idThread)->Clear(handle);
			handle = -1;
		}
	}

	return handle;
}


int NachosOpenFilesTable::getUnixHandle( int nachosHandle, int idThread)
{

  //Si el archivo existe && está abierto según el mapa del thread actual

  // NOTE: Recuerde que "vecMapsOpenFiles" está en NachosOpenFilesTable
  // Aquí se consulta por el vecMapsOpenFiles del thread actual, se prueba si en la posición
  // nachosHandle ( Test(nachosHandle) ) del thread actual ( at(idThread) ) está en 1 (con Test).
  // Si está en 1, devuelve lo que haya en "openFiles[nachosHandle]" (vector con índices de files, en NachosOpenFilesTable)

  bool archivoEstaAbierto = (nachosHandle >= 0 && vecMapsOpenFiles->at(idThread)->Test(nachosHandle));

	if(archivoEstaAbierto)
	{
		return openFiles[nachosHandle]; // Devuelve el handle del archivo correspondiente
	}
	else{
    return -1; // Si no, devuelve -1
  }
}

// Este método devuelve si un archivo está abierto
// por alguno de los threads activo
// Recibe el id del thread respectivo y el nachosHandle de un archivo
// Devuelve true si algún archivo está abierto en algún hilo
// Si no, devuelve 0

bool NachosOpenFilesTable::isOpened(int idThread, int nachosHandle){

  //return openFilesMaps->at(idThread)->Test(nachosHandle);
  //return openFilesMap->Test(nachosHandle);
  // Los métodos anteriores funcionan sin multiprogramación

  // Devuelve si el archivo está abierto, en el thread especificado

  return vecMapsOpenFiles->at(idThread)->Test(nachosHandle);

}

NachosOpenFilesTable::~NachosOpenFilesTable(){

  delete[] openFiles; // Borra los open files
	delete[] vecMapsOpenFiles; // Borra el map de los files abiertos

}
