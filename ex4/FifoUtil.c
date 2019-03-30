#include "FifoUtil.h"

/* Free the memory allocated for the node of type
   FifoCell_t and close the mutex handle. */
void FreeFifoCellCloseMutex(FifoCell_t * node) {
	if (node != NULL) {
		CloseHandle(node->mutex); // TODO: MAKE SURE IT WORKS WHEN MUTEX WAS NOT DEFINED YET
		free(node);
	}
}

/* Free all nodes inside the primitive_trio linked list pointed by given
   head pointer. Also close all the mutex handles of each node. */
void FreeFifo(FifoCell_t * head) {
	FifoCell_t * next;
	while (head != NULL) {
		next = head->next;
		FreeFifoCellCloseMutex(head);
		head = next;
	}
}

/* Provide all the required variables to create a new node of type
   FifoCell_t, then create it and return the pointer to this node */
FifoCell_t* NewFifoCell(char message[101], HANDLE mutex) {
	FifoCell_t* node = malloc(sizeof(FifoCell_t));
	node->mutex = mutex;
	strcpy(node->message, message);
	node->next = NULL;
	return node;
}


/* Adding new item to the Fifo in the end of the linked list*/
int AddToFifo(FifoCell_t ** head, char message[101]) {
	DWORD wait_code;
	BOOL ret_val;
	FifoCell_t* toPush;
	/* Create the mutex that will be used to synchronize access to user name array */
	HANDLE mutexHandle = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (NULL == mutexHandle)
	{
		printf("Error when creating mutex: %d\n", GetLastError());
		ExitThread(1);
	}
	wait_code = WaitForSingleObject(mutexHandle, INFINITE);
	if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting for mutex 3\n");
		ExitThread(1);

	}
	/*Critical section*/

	toPush = NewFifoCell(message, mutexHandle);

	FifoCell_t * current = *head;
	if (current == NULL)
		*head = toPush;
	else {
		while (current->next != NULL) {
			current = current->next;
		}
		/* now we can add a new variable */
		current->next = toPush;
	}
	/*End of critical section*/

	ret_val = ReleaseMutex(mutexHandle);
	if (FALSE == ret_val)
	{
		printf("Error when releasing\n");
		ExitThread(1);
	}
	return 0;
}
/* Removing an item from Fifo (pop) */
FifoCell_t* RemoveFromFifo(FifoCell_t * head) {
	FifoCell_t * temp = head->next;
	FreeFifoCellCloseMutex(head);
	head = temp;
	return head;

}
/*
This function releasing all the FIFO's data, and inserting "TERMINATE"
instruction which is used when one of the players has disconnected.
It's input is *head to fifo and it return *newHead which only include
The terminate instructions.
*/
FifoCell_t* TerminateFifo(FifoCell_t * head, int idx) {
	FifoCell_t *newHead = NULL;
	char message[30];

	sprintf(message, "TERMINATE:%d", idx);
	FreeFifo(head);
	AddToFifo(&newHead, message);
	return newHead;
}

