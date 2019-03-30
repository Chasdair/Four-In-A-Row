#ifndef SERVER_RCV_H
#define SERVER_RCV_H

#include "Client_h.h"

char userNames[2][31];

struct thread_data {
	SOCKET t_socket;
	int idx;
	char userName[31];
} thread_data_s;

//void mainServer();
DWORD ServerRcvThread(struct thread_data *inData);

struct FifoCell_s *fifoHead[2];
int startFlag;
HANDLE startFlagMutex;
BOOL myTurn;

#endif // SERVER_RCV_H