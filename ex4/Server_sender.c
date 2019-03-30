#include "Server_sender.h"

/*
This Function is a wraper functiom for sending data. It uses SendString from
SocketSendReecvTools and catches errors. if an error occured we close the thread
*/
void SendCommand(char* SendStr, struct thread_data *inData) {
	int SendRes;
	SendRes = SendString(SendStr, inData->t_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		fprintf(logFileHandle, "Service socket error while writing, closing thread.\n");
		closesocket(inData->t_socket);
		ExitThread(1);
	}
}

/*
This function responsible to propogate the value of startFlag from 0 to 2.
Each sender thread enters the critical section in order to declare that it
finsihed the NEW_USER_REQ sequence, by propgating the start flag by 1.
When both threads finished the sequence, start flag's value is 2.
*/
void propogateStartFlag() {
	DWORD wait_code;
	BOOL ret_val;
	wait_code = WaitForSingleObject(startFlagMutex, INFINITE);
	if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting for mutex\n");
		fprintf(logFileHandle, "Error when waiting for mutex\n");
		ExitThread(1);
	}
	/*Critical section of user threads*/
	startFlag++;
	/*End of critical section*/
	ret_val = ReleaseMutex(startFlagMutex);
	if (FALSE == ret_val)
	{
		printf("Error when releasing\n");
		fprintf(logFileHandle, "Error when releasing\n");
		ExitThread(1);
	}
}

/*
This function is reponsible to check if the player's move is valid.
The input is the player move in type string. The function returns
the value zero if move is invalid, -1 if not this player's turn, else returns 1
*/
int CheckMove(char* move, int playerIdx, int turnOfPlayer) {
	if (playerIdx != turnOfPlayer) // Not this player's move
		return -1;
	int intmove = atoi(move);
	if (intmove < 0 || intmove >(BOARD_WIDTH - 1)) // Column is out of range
		return 0;
	if (board[5][intmove] != 0) // Column is full
		return 0;
	return 1;
}

/*
This function updates the board game accoring to the last move.
The input args are: integer player number, and the move string.
It return the newly assigned row in the played column.
*/
int PlayMove(char* move, int playerIdx) {
	int row = 0;
	int intmove = atoi(move);
	for (row = 0; row < BOARD_HEIGHT; row++) { //iterating to place the point in the correct height
		if (board[row][intmove] == 0)
			break;
	}
	board[row][intmove] = playerIdx;
	return (5 - row);
}

/*
This function take as input the player number, move coords and directions flag.
The directions flags define in which direction we check for four signs in a row.
If we do we return 1, else we return 0;
*/
int CheckDirection(int playerNum, int widthCoord, int hightCoord, int upDown, int leftRight) {
	for (int step = 1; step < 4; step++) {
		if (board[hightCoord + step * upDown][widthCoord + step * leftRight] != playerNum)
			return 0;
	}
	return 1;
}

/*
This function checks if the last move was a winning move.
The input is the move coords and player number. According to the coords, the function
checks in the relevant directions if the move is a winning move. if its a winning move
the function return 1, else return 0.
*/
int CheckIfWinner(int playerNum, int playedCol, int playedRow) {
	int res = 0;
	if (playedRow < 3) {
		res = CheckDirection(playerNum, playedCol, playedRow, 1, 0); //searches up
		if (playedCol < 3) {
			res = CheckDirection(playerNum, playedCol, playedRow, 0, 1) || res; //searches right
			res = CheckDirection(playerNum, playedCol, playedRow, 1, 1) || res; //searches upwards right diagonal
		}
		if (playedCol == 3) {
			res = CheckDirection(playerNum, playedCol, playedRow, 0, 1) || res; //searches right
			res = CheckDirection(playerNum, playedCol, playedRow, 0, -1) || res; //searches left 
			res = CheckDirection(playerNum, playedCol, playedRow, 1, 1) || res; //searches upwards right diagonal
			res = CheckDirection(playerNum, playedCol, playedRow, 1, -1) || res; //searches upwards left diagonal
		}
		if (playedCol > 3) {
			res = CheckDirection(playerNum, playedCol, playedRow, 0, -1) || res; //searches left 
			res = CheckDirection(playerNum, playedCol, playedRow, 1, -1) || res; //searches upwards left diagonal
		}
	}
	else {
		res = CheckDirection(playerNum, playedCol, playedRow, -1, 0); //searches down
		if (playedCol < 3) {
			res = CheckDirection(playerNum, playedCol, playedRow, 0, 1) || res; //searches right
			res = CheckDirection(playerNum, playedCol, playedRow, -1, 1) || res; //searches downards right diagonal
		}
		if (playedCol == 3) {
			res = CheckDirection(playerNum, playedCol, playedRow, 0, 1) || res; //searches right
			res = CheckDirection(playerNum, playedCol, playedRow, 0, -1) || res; //searches left
			res = CheckDirection(playerNum, playedCol, playedRow, -1, 1) || res; //searches downwards right diagonal
			res = CheckDirection(playerNum, playedCol, playedRow, -1, -1) || res; //searches downwards left diagonal
		}
		if (playedCol > 3) {
			res = CheckDirection(playerNum, playedCol, playedRow, 0, -1) || res; //searches left
			res = CheckDirection(playerNum, playedCol, playedRow, -1, -1) || res; //searches downwards left diagonal
		}
	}
	return res;
}

/* This function goes over all the columns of the board game and check if they are full.
   Returns 1 if everything is full and there is a tie, 0 otherswise. */
int CheckIfTie() {
	for (int colIdx = 0; colIdx < BOARD_WIDTH; colIdx++) {
		if (board[5][colIdx] == 0) // Column is not full
			return 0;
	}
	return 1; // All columns are full - board is full
}

/*
  This function responsible of managing the user names in order to avoid
  the same user name for both players. We go to this function for 2
  threads, every thread take ownership over access mutex and than it
  can procceed to this function. The input of the function is a user name
  the function returns 1 if succeeded assiging 1st user, 2 if succeeded
  assiging 2nd user and -1 if failed
*/
int AssignUserNames(char* userName, struct thread_data *inData) {
	strcpy(userNames[inData->idx], userName); // enter username to array
	strcpy(inData->userName, userName);

	if (STRINGS_ARE_EQUAL(userNames[!inData->idx], userName)) //second player chose
		return -1;											  //same name as the first
	if (!STRINGS_ARE_EQUAL(userNames[inData->idx], "") && !STRINGS_ARE_EQUAL(userNames[!inData->idx], ""))
		return 2;
	else
		return 1;
}

/*
This function defines an event which is triggered following NEW_USER_REQUEST
sequence. Both of the server_sender threads wait for this event, while only
the second thread to be created is setting the event. When this event triggered
The game officaly starts.
*/
HANDLE GetGameStartEventHandle()
{
	HANDLE gameStartEventHandle;
	DWORD last_error;
	/* Parameters for CreateEvent */
	const LPSECURITY_ATTRIBUTES P_SECURITY_ATTRIBUTES = NULL;
	const BOOL IS_MANUAL_RESET = TRUE; /* Manual-reset event */
	const BOOL IS_INITIALLY_SET = FALSE;
	const char P_EVENT_NAME[] = "gameStart_event_Event";

	/* Get handle to event by name. If the event doesn't exist, create it */
	gameStartEventHandle = CreateEvent(
		P_SECURITY_ATTRIBUTES, /* default security attributes */
		IS_MANUAL_RESET,       /* manual-reset event */
		IS_INITIALLY_SET,      /* initial state is non-signaled */
		P_EVENT_NAME);         /* name */

	/* Check if succeeded and handle errors */
	last_error = GetLastError();
	if ((last_error != ERROR_SUCCESS) && (last_error != ERROR_ALREADY_EXISTS)) {
		mutexedPrintToLogFile("Error in creating event! %s", last_error);
		exit(2);
	}

	return gameStartEventHandle;
}

DWORD ServerSendThread(struct thread_data *inData) {
	BOOL Done = FALSE;
	BOOL ret_val;
	BOOL fifoHasData;
	BOOL userReqFlag = 0;
	BOOL is_success;
	char SendStr[101];
	char AcceptedStr[101];
	char parsedMsg[101];
	char res_str[1];
	char threadUserName[30];
	char playedRowString[1];
	int playedRow;
	int playedCol;
	int moveRes;
	int turnOfPlayer = -1; // turn of player 1 or 2 
	int otherPlayerIdx;
	DWORD wait_code;
	HANDLE gameStartEventHandle;
	TransferResult_t SendRes;
	HANDLE assignUserMutexhandle;

	/* Create the mutex that will be used to synchronize access to user name array */
	assignUserMutexhandle = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (NULL == assignUserMutexhandle)
	{
		mutexedPrintToLogFile("Error when creating mutex: %d\n", GetLastError());
		ExitThread(1);
	}

	while (!Done) {
		if (fifoHead[inData->idx] == NULL) { // FIFO is empty
			fifoHasData = 0;
		}
		else { // FIFO isn't empty
			wait_code = WaitForSingleObject(fifoHead[inData->idx]->mutex, INFINITE);
			if (WAIT_OBJECT_0 != wait_code)
			{
				printf("Error when waiting for mutex\n");
				fprintf(logFileHandle, "Error when waiting for mutex\n");
				ExitThread(1);
			}
			/*Critical section of user threads*/
			if (fifoHead[inData->idx] != NULL)
				fifoHasData = 1;
			/*End of critical section*/
			ret_val = ReleaseMutex(fifoHead[inData->idx]->mutex);
			if (FALSE == ret_val)
			{
				printf("Error when releasing\n");
				fprintf(logFileHandle, "Error when releasing\n");
				ExitThread(1);
			}
		}

		if (fifoHasData) { // FIFO is not empty
			strcpy(AcceptedStr, fifoHead[inData->idx]->message); // Get the first command from the FIFO
			fifoHead[inData->idx] = RemoveFromFifo(fifoHead[inData->idx]); // Pop this command from the FIFO

			if (strstr(AcceptedStr, "NEW_USER_REQUEST") != NULL) { //User wish to participate

				strcpy(parsedMsg, strstr(AcceptedStr, ":") + 1);

				wait_code = WaitForSingleObject(assignUserMutexhandle, INFINITE);
				if (WAIT_OBJECT_0 != wait_code)
				{
					printf("Error when waiting for mutex\n");
					fprintf(logFileHandle, "Error when waiting for mutex\n");
					ExitThread(1);

				}
				/*Critical section of user threads*/
				strcpy(inData->userName, "");
				userReqFlag = AssignUserNames(parsedMsg, inData); //This function that manages the user name array

				/*End of critical section*/

				ret_val = ReleaseMutex(assignUserMutexhandle);
				if (FALSE == ret_val)
				{
					printf("Error when releasing\n");
					fprintf(logFileHandle, "Error when releasing\n");
					ExitThread(1);
				}

				if (userReqFlag == -1) { //2nd user chose the 1st user's name
					strcpy(SendStr, "NEW_USER_DECLINED");
					SendCommand(SendStr, inData);
					CloseHandle(assignUserMutexhandle);
					ExitThread(1);
				}
				else {
					strcpy(SendStr, "NEW_USER_ACCEPTED:");
					sprintf(res_str, "%d\0", userReqFlag);
					strcat(SendStr, res_str); //concat # of players
					SendCommand(SendStr, inData);
				}
				gameStartEventHandle = GetGameStartEventHandle();
				if (userReqFlag == 2) {
					if (inData->idx == 1)
						turnOfPlayer = 1;
					else 
						turnOfPlayer = 2;
					is_success = SetEvent(gameStartEventHandle);
					if (is_success == 0) {
						printf("Error setting event!");
						fprintf(logFileHandle, "Error setting event!");
						ExitThread(1);
					}

				}
				wait_code = WaitForSingleObject(gameStartEventHandle, INFINITE);
				if (WAIT_OBJECT_0 != wait_code)
				{
					printf("Error when waiting for mutex\n");
					fprintf(logFileHandle, "Error when waiting for mutex\n");
					ExitThread(1);
				}
				if (turnOfPlayer == -1) {
					if (inData->idx == 0)
						turnOfPlayer = 1;
					else
						turnOfPlayer = 2;
				}
				CloseHandle(gameStartEventHandle);
				strcpy(SendStr, "GAME_STARTED");
				SendCommand(SendStr, inData);
				strcpy(SendStr, "BOARD_VIEW:0");
				SendCommand(SendStr, inData);
				strcpy(SendStr, "TURN_SWITCH:");
				strcat(SendStr, userNames[0]);
				SendCommand(SendStr, inData);

				propogateStartFlag();

			} // end of NEW_USER_REQUEST
			else if (strstr(AcceptedStr, "SEND_MESSAGE") != NULL) { // This message was received from the other player. Parse it and send to own player.
				strcpy(parsedMsg, strstr(AcceptedStr, ":") + 1);
				strcpy(SendStr, "RECEIVE_MESSAGE:"); // The command name
				strcat(SendStr, userNames[!inData->idx]); // The username
				strcat(SendStr, ":;"); // Delimiter
				strcat(SendStr, parsedMsg); // The message
				/* Send this message to user */
				SendCommand(SendStr, inData);
			} // SEND_MESSAGE end
			else if (strstr(AcceptedStr, "PLAY_REQUEST") != NULL) { // The user wants to make a move 
				strcpy(parsedMsg, strstr(AcceptedStr, ":") + 1); // Parse the message
				moveRes = CheckMove(parsedMsg, (inData->idx + 1), turnOfPlayer);
				if (moveRes == 1) { // Check the move, if it is legal - it returns 1
					/* Send play accepeted to own player */
					strcpy(SendStr, "PLAY_ACCEPTED");
					SendCommand(SendStr, inData);
				}
				else { // Move it illegal
					if (moveRes == 0) {
						strcpy(SendStr, "PLAY_DECLINED:Illegal; ;move");
					}
					else if (moveRes == -1) { // Not this player's turn
						strcpy(SendStr, "PLAY_DECLINED:Not; ;your; ;turn");
					}
					SendCommand(SendStr, inData);
					continue; // Go on to next command in FIFO
				}
				// Update the move on the game board:
				playedRow = PlayMove(parsedMsg, (inData->idx + 1));
				/* Send move verification to the other player's FIFO */
				strcpy(SendStr, "VERIFMOVE:"); // The command name
				strcat(SendStr, parsedMsg); // The move column
				strcat(SendStr, ";");
				sprintf(playedRowString, "%d", playedRow);
				strcat(SendStr, playedRowString); // The move row
				AddToFifo(&fifoHead[!(inData->idx)], SendStr); // Add move to the other player's FIFO
				/* Toggle turn */
				if (turnOfPlayer == 1)
					turnOfPlayer = 2;
				else
					turnOfPlayer = 1;
				/* Send messages to player */
				// BOARD_VIEW:
				strcpy(SendStr, "BOARD_VIEW:"); // command
				strcat(SendStr, parsedMsg); // column
				strcat(SendStr, ";");
				sprintf(playedRowString, "%d\0", playedRow); // row
				strcat(SendStr, playedRowString);
				strcat(SendStr, ";");
				sprintf(res_str, "%d\0", (inData->idx + 1)); // player number
				strcat(SendStr, res_str);
				SendCommand(SendStr, inData); // send
				// TURN_SWITCH:
				strcpy(SendStr, "TURN_SWITCH:"); // command
				WaitForSingleObject(assignUserMutexhandle, INFINITE);
				if (WAIT_OBJECT_0 != wait_code)
				{
					printf("Error when waiting for mutex\n");
					fprintf(logFileHandle, "Error when waiting for mutex\n");
					ExitThread(1);
				}
				/* start of critical section */
				strcat(SendStr, userNames[turnOfPlayer - 1]); // username
				/* end of critical section */
				ret_val = ReleaseMutex(assignUserMutexhandle);
				if (FALSE == ret_val)
				{
					printf("Error when releasing mutex\n");
					fprintf(logFileHandle, "Error when releasing mutex\n");
					ExitThread(1);
				}
				SendCommand(SendStr, inData); // send
				/* Check if this play resulted in a win. If it did - end the game for your own player. */
				if (CheckIfWinner((inData->idx + 1), atoi(parsedMsg), (5 - playedRow))) {
					/* Send win message to own player */
					strcpy(SendStr, "GAME_ENDED:"); // The command name
					strcat(SendStr, inData->userName); // The username
					SendCommand(SendStr, inData);
					Done = 1;
				}
				else if (CheckIfTie()) { // Check if this play resulted in a tie
					strcpy(SendStr, "GAME_ENDED:TIE"); // The command name
					/* Send tie message to own player */
					SendCommand(SendStr, inData);
					Done = 1;
				}
			} // PLAY_REQUEST end
			else if (strstr(AcceptedStr, "VERIFMOVE") != NULL) { // Other player made a move
				/* Send messages to player */
				// BOARD_VIEW:
				strcpy(parsedMsg, strstr(AcceptedStr, ":") + 1); // get played column
				strcpy(SendStr, "BOARD_VIEW:"); // command
				strcat(SendStr, parsedMsg); // column and row
				strcat(SendStr, ";");
				if (inData->idx == 0) // Make sure to send other player's number
					otherPlayerIdx = 2;
				else
					otherPlayerIdx = 1;
				sprintf(res_str, "%d\0", otherPlayerIdx); // player number
				strcat(SendStr, res_str);
				SendCommand(SendStr, inData); // send

				/* Toggle turn */
				if (turnOfPlayer == 1)
					turnOfPlayer = 2;
				else
					turnOfPlayer = 1;
				// TURN_SWITCH:
				strcpy(SendStr, "TURN_SWITCH:"); // command
				WaitForSingleObject(assignUserMutexhandle, INFINITE);
				if (WAIT_OBJECT_0 != wait_code)
				{
					printf("Error when waiting for mutex\n");
					fprintf(logFileHandle, "Error when waiting for mutex\n");
					ExitThread(1);
				}
				/* start of critical section */
				strcat(SendStr, userNames[turnOfPlayer - 1]); // username
				/* end of critical section */
				ret_val = ReleaseMutex(assignUserMutexhandle);
				if (FALSE == ret_val)
				{
					printf("Error when releasing mutex\n");
					fprintf(logFileHandle, "Error when releasing mutex\n");
					ExitThread(1);
				}
				SendCommand(SendStr, inData); // send
				/* Check if other user's play resulted in a win. If it did - end the game for your own player. */
				playedCol = atoi(strtok(parsedMsg, ";"));
				playedRow = atoi(strtok(NULL, ";"));
				if (CheckIfWinner(otherPlayerIdx, playedCol, (5 - playedRow))) {
					/* Send win message to own player */
					strcpy(SendStr, "GAME_ENDED:"); // The command name
					WaitForSingleObject(assignUserMutexhandle, INFINITE);
					if (WAIT_OBJECT_0 != wait_code)
					{
						printf("Error when waiting for mutex\n");
						fprintf(logFileHandle, "Error when waiting for mutex\n");
						ExitThread(1);
					}
					/* start of critical section */
					strcat(SendStr, userNames[otherPlayerIdx - 1]); // The other player's username
					/* end of critical section */
					ret_val = ReleaseMutex(assignUserMutexhandle);
					if (FALSE == ret_val)
					{
						printf("Error when releasing mutex\n");
						fprintf(logFileHandle, "Error when releasing mutex\n");
						ExitThread(1);
					}
					SendCommand(SendStr, inData);
					Done = 1;
				}
				else if (CheckIfTie()) { // Check if this play resulted in a tie
					strcpy(SendStr, "GAME_ENDED:TIE"); // The command name
					/* Send tie message to own player */
					SendCommand(SendStr, inData);
					Done = 1;
				}
			} // VERIFMOVE
			else if (strstr(AcceptedStr, "TERMINATE") != NULL) {
				strcpy(parsedMsg, strstr(AcceptedStr, ":") + 1); // get idx of disconnected user
				if (inData->idx != atoi(parsedMsg)) {
					terminateFlag = 1;
					shutdown(inData->t_socket, 2);
					closesocket(inData->t_socket);
				}
				ExitThread(0);
			} //TERMINATE
		}
		strcpy(AcceptedStr, "");
	}
	//Game has ended
	endFlag = 1;
	if (fifoHead[inData->idx] == NULL) { // FIFO is empty
		fifoHasData = 0;
	}
	if (fifoHasData == 1) {
		WaitForSingleObject(fifoHead[inData->idx]->mutex, INFINITE);
		if (WAIT_OBJECT_0 != wait_code)
		{
			printf("Error when waiting for mutex\n");
			fprintf(logFileHandle, "Error when waiting for mutex\n");
			ExitThread(1);
		}

		/* critical section */

		FreeFifo(fifoHead[inData->idx]); //Freeing this user's fifo if is has other instructions (e.g. messages)

		/* end of critical section */

		ret_val = ReleaseMutex(fifoHead[inData->idx]->mutex);
		if (FALSE == ret_val)
		{
			printf("Error when releasing mutex\n");
			fprintf(logFileHandle, "Error when releasing mutex\n");
			ExitThread(1);
		}
	}
	ExitThread(1);
}