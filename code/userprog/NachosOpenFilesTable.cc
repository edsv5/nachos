
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

    openFiles = new int[limiteDeArchivosAbiertos]; // 15 archivos abiertos como máximo, Inicializamos

    // Empieza en 2 por que el 0 y el 1 son de la consola
    for(int i=0; i < limiteDeArchivosAbiertos; i++)
    {
	     openFiles[i] = -1; // Los espacios en -1 es que están disponibles
    }

    // vecMapsOpenFiles = new vector<BitMap*>; // Inicializamos el vector de bitmaps
    openFilesMap = new BitMap(limiteDeArchivosAbiertos); // bitmap de 15 bits, uno por cada archivo que se puede abrir

    // Marca como ocupados los bits de la consola (Console Input, Console Output)

    openFilesMap->Mark(0);
    openFilesMap->Mark(1);


    usage = 0; // Inicia en 0

    // El archivo 0 y 1 se dejan abiertos siempre para que no interfiera con el Console Input
    // o Console Output, los cuales valen 0 y 1, respectivamente

    openFiles[0] = 0;
    openFiles[1] = 1;

}

/*
 * Este método revisa el bitmap de archivos abiertos, si el archivo está abierto
 * lo reabre y asigna el handle a el mismo archivo, si no está abierto, inserta en el
 * bitmap un 1 correspondiente al archivo abierto y un handle en el vector de archivos
 * abiertos. Al final devuelve el NachOS handle del archivo recién abierto.
 *
 */

int NachosOpenFilesTable::Open(int UnixHandle, int idThread)
{

  // Se crea el handle
  int handle = 0;

  bool archivoEstaAbierto = 0;
  bool reabrir = false;

  //printf("Bitmap antes de creación de archivo\n");
  //openFilesMap->Print();
  //printf("\n");

  // Recorre el bitmap de archivos abiertos buscando si está abierto (si su bit está en 1)

  for(int indiceArchivo = 0; indiceArchivo < openFilesMap->getNumBits() ; indiceArchivo ++){

    archivoEstaAbierto = (openFilesMap->Test(indiceArchivo)); //isOpened(indiceArchivo, indiceThead); //  (vecMapsOpenFiles->at(i)->Test(j));
    bool handlesSonIguales = (openFiles[indiceArchivo] == UnixHandle);

    // Si el archivo está abierto y es el archivo al que nos referimos
    if(archivoEstaAbierto && handlesSonIguales){
      reabrir = true;
      handle = indiceArchivo; // El handle será el índice del archivo elegido
    }
  }

  if(reabrir == true){ // Si se está reabriendo el archivo, se devuelve el handle, porque ya está abierto
    return handle;
  }

  // Si no estaba abierto, se trata de abrir el archivo, es decir, se modifica el vector de archivos abiertos
  // y se marca el bit para el mismo

  handle = openFilesMap->Find();

  // Si se logró asignar el handle, es decir, el handle no es -1

  if(handle != -1)
	{
		if(openFiles[handle] == -1){ // Si la posición está desocupada
			openFiles[handle] = UnixHandle; // ASIGNA
      openFilesMap->Mark(handle); // Marca en el bitmap como ocupado
    }
		else //Si el espacio está ocupado, se le hace clear y se devuelve -1
		{
      printf("handle: %d", handle);
			// openFilesMap->Clear(handle);
			// handle = -1;
		}
	}

  //printf("Bitmap después de creación de archivo\n");
  //openFilesMap->Print();

  return handle;
}

// Este método retorna el UNIX handle del archivo recién cerrado

int NachosOpenFilesTable::Close(int NachosHandle)
{
  // Si el archivo está abierto, busca en el bitmap, en el índice del archivo, le da clear

  if( isOpened(NachosHandle) ){
    int handleUnix = getUnixHandle(NachosHandle); // Traduce el handle de NachOS a handle de UNIX para retornarlo
    openFilesMap->Clear(NachosHandle); // Pone el bit en 0, ya que se está cerrando
    openFiles[NachosHandle] = -1; // Libera el espacio, pone -1 en el vector de archivos abiertos
    return handleUnix; // Retorna el handle de UNIX para cerrarlo en UNIX
  }else{ // Si el archivo no está abierto
    return -1; // Devuelve -1
  }

}

/*
 * Devuelve el UNIX handle correspondiente al archivo ingresado por parámetro
 */

int NachosOpenFilesTable::getUnixHandle(int nachosHandle)
{
  if(isOpened(nachosHandle)){
    return openFiles[nachosHandle]; // Devuelve el handle
  }else{
    return -1;
  }
}

// Devuelve si un archivo específico está abierto
// para esto, revisa el mapa de archivos abiertos
// correspondiente

bool NachosOpenFilesTable::isOpened(int nachosHandle){

  //return openFilesMaps->at(idThread)->Test(nachosHandle);
  //return openFilesMap->Test(nachosHandle);

  return openFilesMap->Test(nachosHandle);
}

NachosOpenFilesTable::~NachosOpenFilesTable(){

  delete[] openFiles; // Borra los open files
	delete[] openFilesMap; // Borra el map de los files abiertos

}
