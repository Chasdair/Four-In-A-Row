#include "Client_rcv.h"

/***********************************************************
* This function prints the board, and uses O as the holes.
* The disks are presented by red or yellow backgrounds.
* Input: A 2D array representing the board and the console handle
* Output: Prints the board, no return value
************************************************************/
void PrintBoard(int board[][BOARD_WIDTH], HANDLE consoleHandle)
{

	int row, column;
	//Draw the board
	for (row = 0; row < BOARD_HEIGHT; row++)
	{
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			printf("| ");
			if (board[row][column] == RED_PLAYER)
				SetConsoleTextAttribute(consoleHandle, RED);

			else if (board[row][column] == YELLOW_PLAYER)
				SetConsoleTextAttribute(consoleHandle, YELLOW);

			printf("O");

			SetConsoleTextAttribute(consoleHandle, BLACK);
			printf(" ");
		}
		printf("\n");

		//Draw dividing line between the rows
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			printf("----");
		}
		printf("\n");
	}


}

/* This function recieves a string, a character to find, and a character that will replace it.
   It finds and replaces all the characters and returns the switched string. */
char* replace_char(char* strToParse, char toFind, char toReplace) {
	char *current_pos = strchr(strToParse, toFind);
	while (current_pos) {
		*current_pos = toReplace;
		current_pos = strchr(current_pos, toFind);
	}
	return strToParse;
}

char* omitString(char** strToPase, char toFind) {
	int parsedStrLen = strlen(*strToPase);
	int newIdx = 0;
	char newString[101];
	for (int idx = 0; idx < parsedStrLen; idx++) {
		if ((*strToPase)[idx] == toFind) {
			continue;
		}
		newString[newIdx++] = (*strToPase)[idx];
	}
	newString[newIdx] = '\0';
	*strToPase = newString;
}

// Reading data coming from the server
DWORD RecvDataThread(void)
{
	BOOL gameIsOn = FALSE;
	TransferResult_t RecvRes;
	HANDLE clearToAcceptMsgEventHandle;
	HANDLE endOfTheWorldEventHandle = GetEndOfTheWorldEvent();
	HANDLE clearToAcceptMsgEventHandleFile;
	BOOL is_success;
	BOOL ret_val;
	BOOL FirstSwitch = TRUE;
	int turncount = 0;
	DWORD lastError;
	DWORD wait_code;
	char* temp;
	char* parsedMsg;
	char toPrint[101];
	int col, row;

	HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	while (1)
	{
		char* AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, m_socket);
		if (RecvRes == TRNS_FAILED)
		{
			if (!endFlag) {
				printf("Socket error while trying to write data to socket\n");
				sprintf(toPrint, "Custom message: Socket error while trying to write data to socket\n");
				printToLogMutexed(toPrint);
				ExitThread(1); // socket error, exit
			}
			else { // "exit" was given to IOthread
				printf("Exiting receiver thread...\n");
				// Get the event handle and close it (in case it wasn't called yet)
				clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
				CloseHandle(clearToAcceptMsgEventHandle);
				CloseHandle(endOfTheWorldEventHandle);
				CloseHandle(hConsole);
				free(AcceptedStr);
				ExitThread(0);
			}
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{ // Game ended and server shut down the connection
			is_success = SetEvent(endOfTheWorldEventHandle); // Mark that all the threads should close now
			if (is_success == 0)
			{
				printf("Error while setting event!\n");
				sprintf(toPrint, "Custom message: Error while setting event!\n");
				printToLogMutexed(toPrint);
				exit(1);
			}
			printf("Server disconnected user. Exiting.\n");
			sprintf(toPrint, "Custom message: Server disconnected user.Exiting.\n");
			printToLogMutexed(toPrint);
			// Get the event handle and close it (in case it wasn't called yet)
			clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
			CloseHandle(clearToAcceptMsgEventHandle);
			is_success = SetEvent(endOfTheWorldEventHandle); // Mark that all the threads should close now
			if (is_success == 0)
			{
				printf("Error while setting event!\n");
				sprintf(toPrint, "Custom message: Error while setting event!\n");
				printToLogMutexed(toPrint);
				exit(1);
			}
			//closesocket(m_socket);
			CloseHandle(endOfTheWorldEventHandle);
			CloseHandle(hConsole);
			free(AcceptedStr);
			ExitThread(0);
		}

		sprintf(toPrint, "Received from server: %s\n", AcceptedStr);
		printToLogMutexed(toPrint);
		if (strstr(AcceptedStr, "NEW_USER_ACCEPTED") != NULL) { // Check if the sent username was accepted
			temp = strstr(AcceptedStr, ":") + 1; // Parse the player number from the message
			if (STRINGS_ARE_EQUAL(temp, "1")) {
				myTurn = TRUE;
				turncount = 1;
			}
			else
				myTurn = FALSE;
			strcpy(playerNum, temp);
			// Player can now send messages and requests to the server:
			clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
			is_success = SetEvent(clearToAcceptMsgEventHandle); // set the event
			if (is_success == 0) {
				printf("Error when setting event!\n");
				sprintf(toPrint, "Custom message: Error when setting event!\n");
				printToLogMutexed(toPrint);
				exit(1);
			}
		}
		else if (strstr(AcceptedStr, "NEW_USER_DECLINED") != NULL) { // Check if the sent username was declined
			printf("Request to join was refused, exiting!\n");
			sprintf(toPrint, "Custom message: Request to join was refused, exiting!\n");
			printToLogMutexed(toPrint);
			// Get the event handle and close it (in case it wasn't called yet)
			clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
			CloseHandle(clearToAcceptMsgEventHandle);
			is_success = SetEvent(endOfTheWorldEventHandle); // Mark that all the threads should close now
			if (is_success == 0)
			{
				printf("Error while setting event!\n");
				sprintf(toPrint, "Custom message: Error when setting event!\n");
				printToLogMutexed(toPrint);
				exit(1);
			}
			CloseHandle(endOfTheWorldEventHandle);
			CloseHandle(hConsole);
			free(AcceptedStr);
			ExitThread(0x99);
		}
		else if (STRINGS_ARE_EQUAL(AcceptedStr, "GAME_STARTED")) { // Game started
			gameIsOn = TRUE;
			printf("Game is on!\n");
		}
		else if (strstr(AcceptedStr, "TURN_SWITCH") != NULL) { // The turn was switched
			if ((turncount % 2) == 1) {
				if (!FirstSwitch)
					myTurn = !myTurn;
			}
			FirstSwitch = FALSE;
			turncount++;
			parsedMsg = strstr(AcceptedStr, ":") + 1;
			printf("%s's turn\n", parsedMsg);
			sprintf(toPrint, "%s's turn\n", parsedMsg);
			printToLogMutexed(toPrint);
		}
		else if (strstr(AcceptedStr, "BOARD_VIEW") != NULL) { // Board status update
			parsedMsg = strstr(AcceptedStr, ":") + 1;
			if (!STRINGS_ARE_EQUAL(parsedMsg, "0")) { // not initial board_view
				// BOARD_VIEW:<column_number>;<row_number>;<player_number>
				col = atoi(strtok(parsedMsg, ";"));
				row = atoi(strtok(NULL, ";"));
				// Update the board with row, col, and player number generated below:
				board[row][col] = atoi(strtok(NULL, ";"));
			}
			else {
				clearToAcceptMsgEventHandleFile = GetClearToAcceptMsgEventFile();
				is_success = SetEvent(clearToAcceptMsgEventHandleFile); // set the event - tell the IO thread that it can recieve messages from the user again
				if (is_success == 0) {
					printf("Error when setting event!\n");
					sprintf(toPrint, "Custom message: Error when setting event!\n");
					printToLogMutexed(toPrint);
					exit(1);
				}
			}
			// Print the new board status:
			printf("\n");
			PrintBoard(board, hConsole);
		}
		else if ((strstr(AcceptedStr, "PLAY_ACCEPTED") != NULL) || (strstr(AcceptedStr, "PLAY_DECLINED") != NULL)) {
			// Allow the IO thread to recieve inputs from the user/file:
			clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
			is_success = SetEvent(clearToAcceptMsgEventHandle); // set the event - tell the IO thread that it can recieve messages from the user again
			if (is_success == 0) {
				printf("Error when setting event!\n");
				sprintf(toPrint, "Custom message: Error when setting event!\n");
				printToLogMutexed(toPrint);
				exit(1);
			}
			if (strstr(AcceptedStr, "PLAY_ACCEPTED") != NULL) { // The play was accepted
				printf("Well played\n");
			}
			else if (strstr(AcceptedStr, "PLAY_DECLINED") != NULL) { // The play was declined
				parsedMsg = strstr(AcceptedStr, ":") + 1;
				omitString(&parsedMsg, ';');
				strcpy(toPrint, parsedMsg);
				printf("%s\n", toPrint);
			}
		}
		else if (strstr(AcceptedStr, "RECEIVE_MESSAGE") != NULL) { // Receive message from the other player
			parsedMsg = strstr(AcceptedStr, ":") + 1;
			omitString(&parsedMsg, ';');
			strcpy(toPrint, parsedMsg);
			printf("%s\n", toPrint);
		}
		else if (strstr(AcceptedStr, "GAME_ENDED") != NULL) {
			parsedMsg = strstr(AcceptedStr, ":") + 1;
			if (strstr(AcceptedStr, ":TIE")) { // Tie indicator
				printf("Game ended.Everybody wins!\n");
				sprintf(toPrint, "Game ended.Everybody wins!\n");
				printToLogMutexed(toPrint);
			}
			else { // There is a winner
				printf("Game ended.The winner is %s!\n", parsedMsg);
				sprintf(toPrint, "Game ended.The winner is %s!\n", parsedMsg);
				printToLogMutexed(toPrint);
			}
			//is_success = SetEvent(endOfTheWorldEventHandle); // Mark that all the threads should close now
			if (is_success == 0)
			{
				printf("Error while setting event!");
				sprintf(toPrint, "Custom message: Error when setting event!\n");
				printToLogMutexed(toPrint);
				exit(1);
			}
			// Close all open handles and quit
			clearToAcceptMsgEventHandle = GetClearToAcceptMsgEvent();
			CloseHandle(clearToAcceptMsgEventHandle);
			CloseHandle(endOfTheWorldEventHandle);
			CloseHandle(hConsole);
			free(AcceptedStr);
			//shutdown(m_socket, 2); // Shut down the socket for both read and write
			//closesocket(m_socket);
			ExitThread(0);
		}
		else { // Wrong command received, don't free anything, print and quit:
			printf("Error! Unknown message recieved from the server!\n");
			sprintf(toPrint, "Custom message: Error! Unknown message recieved from the server!\n");
			printToLogMutexed(toPrint);
			exit(1);
		}
		free(AcceptedStr);
	}
}

