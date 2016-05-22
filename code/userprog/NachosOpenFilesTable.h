#include "bitmap.h" // Para usar el bitmap (?)

// Para poder usar los vectores que simbolizan los hilos

#include <vector>

using namespace std;

class NachosOpenFilesTable {
  public:
    NachosOpenFilesTable();       // Initialize
    ~NachosOpenFilesTable();      // De-allocate

    //int Open( int UnixHandle ); // Register the file handle
    //int Close( int NachosHandle );      // Unregister the file handle
    bool isOpened( int NachosHandle, int idThread );
    int getUnixHandle( int NachosHandle, int idThread ); // Devuelve el UNIX handle, según el thread correspondiente y el nachos handle
    //void addThread();		// If a user thread is using this table, add it
    //void delThread();		// If a user thread is using this table, delete it

    //void Print();               // Print contents

  private:
    int * openFiles;		// A vector with user opened files
    //BitMap * openFilesMap;	// A bitmap to control our vector, sólo funciona sin multiprogramación
    
    // Para controlar todos los threads a la vez, se utiliza un vector de bitmaps, cada espacio del vector
    // es un thread diferente, cada thread tiene entonces, su propio bitmap
    vector<BitMap*> *vecMapsOpenFiles;
    int usage;			// How many threads are using this table

};
