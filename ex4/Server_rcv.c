#include "Server_rcv.h"

/* This is the server receiver thread function. It gets a struct that contains
   a socket and an index (0/1) as an input value. It receives data from the user
   and assigns it to the FIFOs used by the server receiver threads. */
DWORD ServerRcvThread(struct thread_data *inData) {
	struct timeval tv = { 0,1 };
	fd_set fds;
	DWORD wait_code;
	BOOL ret_val;
	TransferResult_t RecvRes;
	TransferResult_t SendRes;
	BOOLEAN Done = 0;
	int selectRes = 0;
	int tempStartFlag = 0;
	int fifoIndex = 0;

	while (!Done) {
		char *AcceptedStr = NULL;
		FD_ZERO(&fds);
		FD_SET(inData->t_socket, &fds);
		RecvRes = ReceiveString(&AcceptedStr, inData->t_socket);
		if (RecvRes == TRNS_FAILED)
		{
			if (terminateFlag)
				ExitThread(0);
			else {
				printf("Service socket error while reading, closing thread.\n");
				fprintf(logFileHandle, "Service socket error while reading, closing thread.\n");
				closesocket(inData->t_socket);
				strcpy(userNames[inData->idx], "");
				strcpy(userNames[!inData->idx], "");
				ExitThread(1);
			}
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			printf("Player disconnected. Ending communication.\n");
			fprintf(logFileHandle, "Player disconnected. Ending communication.\n");
			if (endFlag || terminateFlag) {//happy path
				// We enter here if this thread's user has disconnected, following the end of the game.
				// We close all the resources in the server thread of the user.
				closesocket(inData->t_socket);

				ExitThread(0);
			}
			else {
				// We enter here if this thread's user has disconnected, if the game hasn't finished
				fifoHead[inData->idx] = TerminateFifo(fifoHead[inData->idx], inData->idx);
				fifoHead[!inData->idx] = TerminateFifo(fifoHead[!inData->idx], inData->idx);
				// We close the sockets in the senders end
				ExitThread(1);
			}
		}

		/* Decide whether the data will be written to the send thread with the same index
		   as the rcv thread or to the FIFO other idx sender thread */
		if (strstr(AcceptedStr, "SEND_MESSAGE") != NULL) { // User wishes to send a message
			// Write command to the other thread's FIFO
			fifoIndex = !inData->idx;
		}
		else if ((strstr(AcceptedStr, "PLAY_REQUEST") != NULL) || (strstr(AcceptedStr, "NEW_USER_REQUEST") != NULL)) {
			// User wishes to participate, or new user request
			// Write command to own FIFO
			fifoIndex = inData->idx;
		}

		// Write the request to the FIFO according to fifoIndex:
		if (fifoHead[fifoIndex] == NULL) { // Nothing in FIFO
			AddToFifo(&fifoHead[fifoIndex], AcceptedStr);
		}
		else { // FIFO in use
			wait_code = WaitForSingleObject(fifoHead[fifoIndex]->mutex, INFINITE);
			if (WAIT_OBJECT_0 != wait_code)
			{
				printf("Error when waiting for mutex\n");
				fprintf(logFileHandle, "Error when waiting for mutex\n");
				ExitThread(1);
			}
			// Critical section start
			AddToFifo(&fifoHead[fifoIndex], AcceptedStr);
			// Critical section end
			ret_val = ReleaseMutex(fifoHead[fifoIndex]->mutex);
			if (ret_val == FALSE)
			{
				printf("Error when releasing mutex\n");
				fprintf(logFileHandle, "Error when waiting for mutex\n");
				ExitThread(1);
			}
		}

		/* Stuck in a loop until the game starts, polling the socket, and answering
		   any request with "game not started" */
		if (strstr(AcceptedStr, "NEW_USER_REQUEST") != NULL) // User wishes to participate
		{
			while (tempStartFlag < 2) { // Stay in this loop until both sender threads increment the flag
				// Select to check when the game started, until then answer to anything with "game not started"
				selectRes = select((inData->t_socket) + 1, &fds, NULL, NULL, &tv);
				if (selectRes == 1) { // Some request was sent from user, send him that the game has not started
					FD_ZERO(&fds);
					FD_SET(inData->t_socket, &fds);
					SendCommand("PLAY_DECLINED:Game; ;has; ;not; ;started", inData->t_socket);
				}
				wait_code = WaitForSingleObject(startFlagMutex, INFINITE);
				if (WAIT_OBJECT_0 != wait_code)
				{
					printf("Error when waiting for mutex\n");
					fprintf(logFileHandle, "Error when waiting for mutex\n");
					ExitThread(1);
				}
				// Critical section start (startFlagMutex)
				tempStartFlag = startFlag;
				// Critical section end (startFlagMutex)
				ret_val = ReleaseMutex(startFlagMutex);
				if (ret_val == FALSE)
				{
					printf("Error when releasing mutex\n");
					fprintf(logFileHandle, "Error when releasing for mutex\n");
					ExitThread(1);
				}
			}
		}
	}
}