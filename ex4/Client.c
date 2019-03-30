#include "Client_h.h"

// The log file handle is used by all the threads.
// This function puts it inside a critical section.
// The toPrint argument is the string to print ot the log file.
// The function prints to the log file, has no outputs.
void printToLogMutexed(char* toPrint) {
	DWORD wait_code;
	BOOL ret_val;
	wait_code = WaitForSingleObject(logFileMutexHandle, INFINITE);
	if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting for mutex\n");
		exit(3);
	}
	// Critical section
	fprintf(logFileHandle, "%s", toPrint);
	// End of critical section
	ret_val = ReleaseMutex(logFileMutexHandle);
	if (ret_val == FALSE)
	{
		printf("Error when releasing\n");
		exit(3);
	}
}

int MainClient(int serverPort)
{
	SOCKADDR_IN clientService;
	HANDLE hThread[3];
	char toPrint[101];
	DWORD wait_code;
	BOOL ret_val;
	DWORD waitRes;

	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 1;
	}

	// Call the socket function and return its value to the m_socket variable. 
	// For this application, use the Internet address family, streaming sockets, and the TCP/IP protocol.
	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Connect to a server.
	// Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;	// AF_INET is the Internet address family.
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); // Setting the IP address to connect to
	clientService.sin_port = htons(serverPort); // Setting the port to connect to.

	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		printf("Failed connecting to server on 127.0.0.1:%d.Exiting\n", serverPort);
		sprintf(toPrint, "Failed connecting to server on 127.0.0.1:%d.Exiting\n", serverPort);
		printToLogMutexed(toPrint);
		WSACleanup();
		return 1;
	}
	// Succesfull connection:
	printf("Connected to server on 127.0.0.1:%d\n", serverPort);
	sprintf(toPrint, "Connected to server on 127.0.0.1:%d\n", serverPort);
	printToLogMutexed(toPrint);


	// Get the username - input mode and input file (for file mode) are global
	getUsername();

	// Start the end and receive data threads
	hThread[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendDataThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvDataThread, NULL, 0, NULL);
	hThread[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IOThread, NULL, 0, NULL);

	waitRes = WaitForMultipleObjects(3, hThread, FALSE, INFINITE);
	if ((waitRes == WAIT_TIMEOUT) || (waitRes == WAIT_FAILED)) { // Check if succeeded and handle errors
		return 1;
	}
	LPDWORD lpExitCode0 = 0;
	LPDWORD lpExitCode1 = 0;
	LPDWORD lpExitCode2 = 0;
	ret_val = GetExitCodeThread(hThread[0], &lpExitCode0) || GetExitCodeThread(hThread[1], &lpExitCode1) || GetExitCodeThread(hThread[2], &lpExitCode2);
	if ((ret_val == 0) || ((lpExitCode0 != 0) && (lpExitCode1 != 0) && (lpExitCode2 != 0))) {
		return 1; // exit code function failed or error in one of the threads
	}
	shutdown(m_socket, 2); // Shut down the socket for both read and write
	closesocket(m_socket);
	TerminateThread(hThread[0], 0x555);
	TerminateThread(hThread[1], 0x555);
	TerminateThread(hThread[2], 0x555);

	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	CloseHandle(hThread[2]);

	// Mutex for log file closing:
	wait_code = WaitForSingleObject(logFileMutexHandle, INFINITE);
	if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting for mutex\n");
		return 1;
	}
	fclose(logFileHandle);
	ret_val = ReleaseMutex(logFileMutexHandle);
	if (ret_val == FALSE)
	{
		printf("Error when releasing\n");
		return 1;
	}
	if (strcmp(inputMode, "file") == 0) // File input mode
		fclose(inFileHandle);

	WSACleanup();
	strcpy(userNames[0], "@");
	strcpy(userNames[1], "@");
	return 0;
}

/* This function defines an event that clears the user to send messages and commands
   to the server. this will happen after the user is accepted as a player.
   it returns the handle to this event. This is a manual event! */
HANDLE GetClearToAcceptMsgEvent()
{
	HANDLE clearToAcceptMsgEventHandle;
	DWORD last_error;
	char toPrint[101];
	/* Parameters for CreateEvent */
	static const LPSECURITY_ATTRIBUTES P_SECURITY_ATTRIBUTES = NULL;
	static const BOOL IS_MANUAL_RESET = TRUE; /* Manual-reset event */
	static const BOOL IS_INITIALLY_SET = FALSE;
	static const char P_EVENT_NAME[] = "ClearToAcceptMsg_Event";

	/* Get handle to event by name. If the event doesn't exist, create it */
	clearToAcceptMsgEventHandle = CreateEvent(
		P_SECURITY_ATTRIBUTES, /* default security attributes */
		IS_MANUAL_RESET,       /* manual-reset event */
		IS_INITIALLY_SET,      /* initial state is non-signaled */
		P_EVENT_NAME);         /* name */
	/* Check if succeeded and handle errors */

	last_error = GetLastError();
	if ((last_error != ERROR_SUCCESS) && (last_error != ERROR_ALREADY_EXISTS)) {
		printf("Error in creating event! %s", last_error);
		sprintf(toPrint, "Custom message: Error in creating event! %s", last_error);
		printToLogMutexed(toPrint);
		exit(2);
	}
	return clearToAcceptMsgEventHandle;
}

/* This function defines an event that clears the user to send messages and commands
   to the server. this will happen after the user is accepted as a player.
   it returns the handle to this event. This is a manual event! for file mode */
HANDLE GetClearToAcceptMsgEventFile()
{
	HANDLE clearToAcceptMsgEventHandle;
	DWORD last_error;
	char toPrint[101];
	/* Parameters for CreateEvent */
	static const LPSECURITY_ATTRIBUTES P_SECURITY_ATTRIBUTES = NULL;
	static const BOOL IS_MANUAL_RESET = TRUE; /* Manual-reset event */
	static const BOOL IS_INITIALLY_SET = FALSE;
	static const char P_EVENT_NAME[] = "ClearToAcceptMsg_EventFile";

	/* Get handle to event by name. If the event doesn't exist, create it */
	clearToAcceptMsgEventHandle = CreateEvent(
		P_SECURITY_ATTRIBUTES, /* default security attributes */
		IS_MANUAL_RESET,       /* manual-reset event */
		IS_INITIALLY_SET,      /* initial state is non-signaled */
		P_EVENT_NAME);         /* name */
	/* Check if succeeded and handle errors */

	last_error = GetLastError();
	if ((last_error != ERROR_SUCCESS) && (last_error != ERROR_ALREADY_EXISTS)) {
		printf("Error in creating event! %s", last_error);
		sprintf(toPrint, "Custom message: Error in creating event! %s", last_error);
		printToLogMutexed(toPrint);
		exit(2);
	}
	return clearToAcceptMsgEventHandle;
}

/* This function defines an event that is signaled when all the threads need to exit
   after freeing all their resources. it returns the handle to this event.
   This is a manual event! */
HANDLE GetEndOfTheWorldEvent()
{
	HANDLE endOfTheWorldEventHandle;
	DWORD last_error;
	char toPrint[101];
	/* Parameters for CreateEvent */
	static const LPSECURITY_ATTRIBUTES P_SECURITY_ATTRIBUTES = NULL;
	static const BOOL IS_MANUAL_RESET = TRUE; /* Manual-reset event */
	static const BOOL IS_INITIALLY_SET = FALSE;
	static const char P_EVENT_NAME[] = "EndOfTheWorld_Event";

	/* Get handle to event by name. If the event doesn't exist, create it */
	endOfTheWorldEventHandle = CreateEvent(
		P_SECURITY_ATTRIBUTES, /* default security attributes */
		IS_MANUAL_RESET,       /* manual-reset event */
		IS_INITIALLY_SET,      /* initial state is non-signaled */
		P_EVENT_NAME);         /* name */
	/* Check if succeeded and handle errors */

	last_error = GetLastError();
	if ((last_error != ERROR_SUCCESS) && (last_error != ERROR_ALREADY_EXISTS)) {
		printf("Error in creating event! %s", last_error);
		sprintf(toPrint, "Custom message: Error in creating event! %s", last_error);
		printToLogMutexed(toPrint);
		exit(2);
	}
	return endOfTheWorldEventHandle;
}

int FindFirstUnusedThreadSlot()
{
	int Ind;

	for (Ind = 0; Ind < 2; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}

