// Fifo.c
//
//
//

#include "../inc/Fifo.h"
#include <stdint.h>
#define Size 64
uint8_t static PutI;
uint8_t static GetI;
uint8_t static Fifo[Size];

// *********** FiFo_Init**********
// Initializes a software FIFO of a
// fixed size and sets up indexes for
// put and get operations
void Fifo_Init(void) {
	PutI = 0;					// Start at same spot
	GetI = 0;
}

// *********** FiFo_Put**********
// Adds an element to the FIFO
// Input: Character to be inserted
// Output: 1 for success and 0 for failure
//         failure is when the buffer is full
uint32_t Fifo_Put(char data) {
	if (GetI == ((PutI + 1) % Size))				// if PutI == GetI, then Fifo is empty
		return 0;
	else {																	
		Fifo[PutI]=data;										// If not, then store data into Fifo
		PutI = ((PutI + 1)%Size);						// Increment Pointer (index)
		return 1;
	} 
}

// *********** FiFo_Get**********
// Gets an element from the FIFO
// Input: Pointer to a character that will get the character read from the buffer
// Output: 1 for success and 0 for failure
//         failure is when the buffer is empty
uint32_t Fifo_Get(char *datapt){
	if (PutI != GetI){			// If not empty,
		*datapt = Fifo[GetI];				// Get Oldest element into input pointer
		GetI = ((GetI + 1) % Size);		// Increment Pointer (Wrapping included)
		return 1;
	}
	else
		return 0;				// Fifo is empty if PutI == GetI
}



