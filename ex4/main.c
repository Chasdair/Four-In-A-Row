#include "main.h"
/*
This function is a wrapper function for fprintf to the game logfile.
This purpose of this wrapper is to use logFileMutex in order to prevent
More the one thread to access the log file. The input is the meesage
we want send in char* type. The fuction does not has ret val.
*/
void mutexedPrintToLogFile(char* errorMessage, DWORD lastError) {
	DWORD wait_code;
	BOOL ret_val;
	printf("%s: %d\n", errorMessage, lastError);
	wait_code = WaitForSingleObject(logFileMutexHandle, INFINITE);
	if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting for mutex\n");
		ExitThread(1);
	}
	/*Critical section of user threads*/
	fprintf(logFileHandle, "Custom message: %s: %d\n", errorMessage, lastError);
	/*End of critical section*/
	ret_val = ReleaseMutex(logFileMutexHandle);
	if (FALSE == ret_val)
	{
		printf("Error when releasing\n");
		ExitThread(1);
	}
}


int main(int argc, char** argv) {
	HANDLE hThread[3];
	if (argc > 4) {
		inputMode = argv[4];
	}
	logFile = argv[2];
	int serverPort = atoi(argv[3]);
	char* mode = argv[1];
	DWORD wait_code;
	BOOL ret_val;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	struct thread_data inData[2];
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	int Ind;
	int Loop;
	int threadStartFlag = -1;
	
	
	/* Create the mutex that will be used to synchronize access to the log file */
	logFileMutexHandle = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (logFileMutexHandle == NULL)
	{
		printf("Error when creating mutex: %d\n", GetLastError());
		return 1;
	}

	if (STRINGS_ARE_EQUAL(mode, "server")) {
		// Open the log file and check the pointer:
		logFileHandle = fopen(logFile, "w+");
		if (logFileHandle == NULL)
		{
			printf("Error!1");
			return 1;
		}
		startFlagMutex = CreateMutex(
			NULL,	/* default security attributes */
			FALSE,	/* initially not owned */
			NULL);	/* unnamed mutex */
		fifoHead[0] = NULL;
		fifoHead[1] = NULL;

		// Initialize Winsock.
		WSADATA wsaData;
		int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

		if (StartupRes != NO_ERROR)
		{
			printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
			return 1;
		}

		/* The WinSock DLL is acceptable. Proceed. */

		// Create a socket.    
		MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (MainSocket == INVALID_SOCKET)
		{
			printf("Error at socket( ): %ld\n", WSAGetLastError());
			return 1;
		}

		// Create a sockaddr_in object and set its values.
		// Declare variables
		Address = inet_addr(SERVER_ADDRESS_STR);
		if (Address == INADDR_NONE)
		{
			printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
				SERVER_ADDRESS_STR);
			return 1;
		}

		service.sin_family = AF_INET;
		service.sin_addr.s_addr = Address;
		service.sin_port = htons(serverPort);

		// Bind the socket.

		bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
		if (bindRes == SOCKET_ERROR)
		{
			printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
			return 1;
		}
		while (1) {
			// Listen on the Socket.
			ListenRes = listen(MainSocket, SOMAXCONN);
			if (ListenRes == SOCKET_ERROR)
			{
				printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
				return 1;
			}

			SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
			if (AcceptSocket == INVALID_SOCKET)
			{
				printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
				return 1;
			}
			threadStartFlag = (threadStartFlag + 1) % 2;
			inData[threadStartFlag].idx = threadStartFlag;
			printf("Client Connected.\n");

			Ind = FindFirstUnusedThreadSlot();

			if (Ind == 2) //no slot is available
			{
				printf("No slots available for client, dropping the connection.\n");
				closesocket(AcceptSocket); //Closing the socket, dropping the connection.
			}
			else
			{
				// Reset the board:
				for (int i = 0; i < BOARD_HEIGHT; i++) {
					for (int j = 0; j < BOARD_WIDTH; j++) {
						board[i][j] = 0;
					}
				}
				if (!STRINGS_ARE_EQUAL(userNames[inData[threadStartFlag].idx], "")) {
					strcpy(userNames[inData[threadStartFlag].idx], "");
					strcpy(inData[threadStartFlag].userName, "");
					strcpy(userNames[inData[!threadStartFlag].idx], "");
					strcpy(inData[!threadStartFlag].userName, "");
				}
				terminateFlag = 0;
				endFlag = FALSE;
				inData[threadStartFlag].t_socket = AcceptSocket;
				ThreadHandles[inData[threadStartFlag].idx] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerRcvThread, &(inData[threadStartFlag]), 0, NULL);
				ThreadHandles[inData[threadStartFlag].idx + 2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerSendThread, &(inData[threadStartFlag]), 0, NULL);
			}
		}
	}
	else {
		inputFile = argv[5];
		// Open the log file and check the pointer:
		logFileHandle = fopen(logFile, "w+");
		if (logFileHandle == NULL)
		{
			printf("Error!2");
			return 1;
		}
		return MainClient(2345);
	}
}