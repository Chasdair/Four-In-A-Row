#include "Client_send.h"

//Sending data to the server
DWORD SendDataThread(void)
{
	char SendStr[101];
	TransferResult_t SendRes;
	HANDLE clearToSndMsgEventHandle;
	HANDLE clearToAcceptMsgEventHandle;
	HANDLE endOfTheWorldEventHandle;
	HANDLE handles[2];
	BOOL is_success;
	DWORD lastError;
	DWORD waitRes;
	char* parsedData;
	char toPrint[101];
	int i, stringlen, offset;

	// Append username to the first sent message:
	SendStr[0] = '\0';
	strcat(SendStr, "NEW_USER_REQUEST:");
	strcat(SendStr, userName);
	SendRes = SendString(SendStr, m_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		sprintf(toPrint, "Costume Message: Socket error while trying to write data to socket\n");
		printToLogMutexed(toPrint);
		ExitThread(1);
	}
	while (1) {
		clearToSndMsgEventHandle = GetClearToSendMsgEvent();
		endOfTheWorldEventHandle = GetEndOfTheWorldEvent();
		handles[0] = clearToSndMsgEventHandle;
		handles[1] = endOfTheWorldEventHandle;
		// Wait to be authorized to send messages and requests:
		//waitRes = WaitForSingleObject(clearToSndMsgEventHandle, INFINITE);
		waitRes = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if ((waitRes == WAIT_TIMEOUT) || (waitRes == WAIT_FAILED)) { // Check if succeeded and handle errors
			printf("Waitformultipileobjects error!\n");
			sprintf(toPrint, "Costume Message: Waitformultipileobjects error!\n");
			printToLogMutexed(toPrint);
			ExitThread(1);
		}
		is_success = ResetEvent(clearToSndMsgEventHandle);
		if (is_success == 0) {
			printf("error resetting event!\n");
			sprintf(toPrint, "Costume Message: error resetting event!\n");
			printToLogMutexed(toPrint);
			ExitThread(1);
		}
		/* polling the end of the world event */
		waitRes = WaitForSingleObject(endOfTheWorldEventHandle, 0);
		if (waitRes == WAIT_OBJECT_0) { // the end of the world event was set
			clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
			CloseHandle(clearToAcceptMsgEventHandle);
			CloseHandle(clearToSndMsgEventHandle);
			CloseHandle(endOfTheWorldEventHandle);
			ExitThread(0);
		}
		if (!STRINGS_ARE_EQUAL(toSend, "")) { // Print only if data is available
			sprintf(toPrint, "Sent to server: %s\n", toSend);
			printToLogMutexed(toPrint);
		}
		if (strstr(toSend, "play") != NULL) {
			parsedData = strstr(toSend, " ") + 1; // Parse the played column from the input string
			SendStr[0] = '\0';
			strcat(SendStr, "PLAY_REQUEST:");
			strcat(SendStr, parsedData);
			SendRes = SendString(SendStr, m_socket);
			if (SendRes == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				sprintf(toPrint, "Costume Message: Socket error while trying to write data to socket\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}
		}
		else if (strstr(toSend, "message") != NULL) {
			parsedData = strstr(toSend, " ") + 1; // Parse the played column from the input string
			SendStr[0] = '\0';
			strcat(SendStr, "SEND_MESSAGE:");
			stringlen = strlen(SendStr);
			offset = 0;
			for (i = 0; i < strlen(parsedData); i++) {
				if (parsedData[i] != ' ') {
					SendStr[stringlen + offset + i] = parsedData[i];
					SendStr[stringlen + offset + i + 1] = '\0';
				}
				else {
					strcat(SendStr, "; ;");
					offset = offset + 2;
				}
			}

			SendRes = SendString(SendStr, m_socket);
			if (SendRes == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				sprintf(toPrint, "Costume Message: Socket error while trying to write data to socket\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}
			// Allow the IO thread to recieve inputs from the user/file:
			clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
			// Check that the handle was recieved:
			lastError = GetLastError();

			is_success = SetEvent(clearToAcceptMsgEventHandle); // set the event - tell the IO thread that it can recieve messages from the user again
			if (is_success == 0) {
				printf("error setting event!\n");
				sprintf(toPrint, "Costume Message: error setting event\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}

		}
		strcpy(toSend, "");
	}

}
