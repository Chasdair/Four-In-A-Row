#include "Client_IO.h"

/* This function defines an event that clears the user to send messages and commands
   to the client send thread. This will happen when the user enters a new command.
   it returns the handle to this event. This is a Manual event! */
HANDLE GetClearToSendMsgEvent()
{
	HANDLE clearToSndMsgEventHandle;
	DWORD last_error;
	/* Parameters for CreateEvent */
	const LPSECURITY_ATTRIBUTES P_SECURITY_ATTRIBUTES = NULL;
	const BOOL IS_MANUAL_RESET = TRUE; /* Manual-reset event */
	const BOOL IS_INITIALLY_SET = FALSE;
	const char P_EVENT_NAME[] = "ClearToSendMsg_event_Event";
	char toPrint[101];

	/* Get handle to event by name. If the event doesn't exist, create it */
	clearToSndMsgEventHandle = CreateEvent(
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
		ExitThread(2);
	}

	return clearToSndMsgEventHandle;
}

/* (4) This function gets the user name either from the file, or from the command line.
   It receives the char array input mode and input file. It gets the username and assigns
   it to the global username char array */
void getUsername() {
	int checker;
	if (strcmp(inputMode, "human") == 0) {	  // Human input mode
		printf("Please enter a username, then press \"Enter\":\n");
		checker = scanf("%s", userName); // Get username from cmd line
		if (checker < 0) { // scanf error
			printf("Error! Scanf failed to get username.\n");
			exit(1);
		}
	}
	else if (strcmp(inputMode, "file") == 0) { // File input mode
		// Open the input file and check the pointer:
		inFileHandle = fopen(inputFile, "r");
		if (inFileHandle == NULL)
		{
			printf("Error opening input file!\n");
			exit(1);
		}
		if (fgets(userName, 30, inFileHandle) == NULL)
		{
			printf("Error reading username from file!\n");
			exit(1);
		}
		// Replace new line character ('\n') with '\0':
		size_t userLen = strlen(userName) - 1;
		if (*userName && userName[userLen] == '\n')
			userName[userLen] = '\0';
	}
	else {
		printf("Error, the input method is not \"human\" or \"file\"!\n");
		exit(1);
	}
}

DWORD IOThread(void)
{
	int checker;
	char commandOrMsg[101];
	HANDLE clearToAcceptMsgEventHandle;
	HANDLE clearToSndMsgEventHandle;
	HANDLE clearToAcceptMsgEventHandleFile;
	BOOL is_success;
	char toPrint[101];
	// Get the clear to send msg event handle
	clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
	is_success = ResetEvent(clearToAcceptMsgEventHandle);
	if (is_success == 0) {
		printf("error resetting event!\n");
		sprintf(toPrint, "Costume Message: error resetting event!\n");
		printToLogMutexed(toPrint);
		ExitThread(1);
	}
	printf("Waiting for server to accept game connection...\n");
	sprintf(toPrint, "Custom message: Waiting for server to accept game connection...\n");
	printToLogMutexed(toPrint);
	// Wait to be authorized to send messages and requests:
	if (WaitForSingleObject(clearToAcceptMsgEventHandle, INFINITE) != WAIT_OBJECT_0) {// Check if succeeded and handle errors
		printf("error waiting for object!\n");
		sprintf(toPrint, "Costume Message: error waiting for object!\n");
		printToLogMutexed(toPrint);
		ExitThread(1);
	}
	printf("Server accepted game connection! Player number is: %s\n", playerNum);
	sprintf(toPrint, "Custom message: Server accepted game connection! Player number is: %s\n", playerNum);
	printToLogMutexed(toPrint);

	while (1) { // Get user inputs as long as the connection is on
		if (strcmp(inputMode, "human") == 0) {	  // Human input mode
			printf("\nYou can now enter commands and messages.\nFor commands \"play <column num>\", then press \"Enter\":\nFor messages \"message <message body>\", then press \"Enter\":\nTo exit enter \"exit\"\n");

			checker = scanf(" %[^\n]s", commandOrMsg); // Get command/play/exit from cmd
			if (checker < 0) {// scanf error
				printf("error scanf failed!\n");
				sprintf(toPrint, "Costume Message: error scanf failed!\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}
			
		}
		else if (strcmp(inputMode, "file") == 0) { // File input mode
			if (fgets(commandOrMsg, 100, inFileHandle) == NULL) // Read a line from file
			{
				printf("Error reading username from file!\n");
				printf("error readsing from file!\n");
				sprintf(toPrint, "Costume Message: error reading from file!\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}
			clearToAcceptMsgEventHandleFile = GetClearToAcceptMsgEventFile();
			if (WaitForSingleObject(clearToAcceptMsgEventHandleFile, INFINITE) != WAIT_OBJECT_0) {// Check if succeeded and handle errors
				printf("error waiting for object!\n");
				sprintf(toPrint, "Costume Message: error waiting for object!\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}
			while ((!myTurn && (strstr(commandOrMsg, "play ") != NULL))); // Not this player's turn and play move			if (strstr(commandOrMsg, "play ") != NULL)
			Sleep(100);
			if (strstr(commandOrMsg, "play ") != NULL)
				myTurn = FALSE;
			commandOrMsg[strlen(commandOrMsg) - 1] = '\0';
		}

		// Check if the given command is "play" or "message"
		if ((strstr(commandOrMsg, "play ") != NULL) || (strstr(commandOrMsg, "message ") != NULL)) {
			strcpy(toSend, commandOrMsg); // Assign the command to the toSend string that is used in the send thread
			clearToSndMsgEventHandle = GetClearToSendMsgEvent();

			// Reset the accept message event until the sender threads sets it again
			is_success = ResetEvent(clearToAcceptMsgEventHandle);
			if (is_success == 0) {
				printf("error resetting event!\n");
				sprintf(toPrint, "Costume Message: error resetting event!\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}
			is_success = SetEvent(clearToSndMsgEventHandle); // set the event - tell the sender thread that it can send the play move
			if (is_success == 0) {
				printf("error setting event!\n");
				sprintf(toPrint, "Costume Message: error setting event!\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}

			// Wait for the message to be sent in the send thread to be able to recieve another one
			if (WaitForSingleObject(clearToAcceptMsgEventHandle, INFINITE) != WAIT_OBJECT_0) {// Check if succeeded and handle errors
				printf("error waiting for event!\n");
				sprintf(toPrint, "Costume Message: error waiyting for event!\n");
				printToLogMutexed(toPrint);
				ExitThread(1);
			}

		}
		else if (STRINGS_ARE_EQUAL(commandOrMsg, "exit")) {    // Check if the given command is "exit"
			is_success = CloseHandle(clearToAcceptMsgEventHandle);
			clearToSndMsgEventHandle = GetClearToSendMsgEvent();
			is_success = is_success && CloseHandle(clearToSndMsgEventHandle);
			if (is_success == 0) { // Error closing handle
				printf("Error closing event handle!");
				ExitThread(1);
			}
			terminateFlag = 1;
			endFlag = TRUE; // Notify receiver thread about closing (it will know when the socket is shut down)
			ExitThread(0);
		}
		else { // Wrong input given by user
			printf("Error: Illegal command\n");
			sprintf(toPrint, "Error: Illegal commmand\n");
			printToLogMutexed(toPrint);
		}
		strcpy(commandOrMsg, "\0");
	}
}