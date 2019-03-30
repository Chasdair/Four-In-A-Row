#ifndef FIFOUTIL
#define FIFOUTIL

#include "Client_h.h"

/*   Fifo Cell data structure    */

/*
This is the fifo-cell data structures. Its purpose is to hold messages
 inserted by server-reciever threads. The messages are withrawed by
 server-sender threads to send them forward to clients
*/
typedef struct FifoCell_s {
	char message[101];
	HANDLE mutex;
	struct node * next;

} FifoCell_t;


/*     Function Declerations     */

void FreeFifoCellCloseMutex(FifoCell_t * node);

void FreeFifo(FifoCell_t * head);

FifoCell_t* NewFifoCell(char message[101], HANDLE mutex);

int AddToFifo(FifoCell_t ** head, char message[101]);

FifoCell_t* RemoveFromFifo(FifoCell_t * head);

FifoCell_t* TerminateFifo(FifoCell_t * head, int idx);


#endif
