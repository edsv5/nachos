#ifndef THREADSTABLA_H
#define THREADSTABLA_H

#define MAX_THREADS 128
#include "bitmap.h"
#include "../threads/thread.h"
#include "../threads/synch.h"
class threadsTabla{
	typedef int SpaceId;
	
	struct hilosEnEspera{
		Thread* hilo=NULL;
		Semaphore* sem=NULL;
		SpaceId hiloEsperado=-1;
	};
	
	public: 
	threadsTabla();
	~threadsTabla();
	
	SpaceId AddThread(Thread* thread);
	int DelThread(int NachosHandle);
 	inline Thread* getThread(int NachosHandle){return threadsOn[NachosHandle];};
	bool UniqueSpaceUsing(AddrSpace *space,SpaceId ID);
	//int addJoin(Thread* thread, Semaphore* s,int identificador, SpaceId id);
	//int delJoin(Thread* thread, Semaphore* s,int identificador, SpaceId id);
	
	int addJoin(Thread* thread, Semaphore* s, SpaceId id);
	int delJoin(Thread* thread, Semaphore* s,int identificador, SpaceId id);
	int avisarHilo(int id);
	
 	
 	private:
 	Thread** threadsOn;		// A vector with user operating threads
    	BitMap * threadsOnMap;	// A bitmap to control our vector
	hilosEnEspera* hilosJoin;
	BitMap * hilosMapa;
	

};



#endif //THREADSTABLA
