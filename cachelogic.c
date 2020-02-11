#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
//unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

int counter = 0;
/*-------HELPER FUNCTIONS-------*/

unsigned int createMask(address addrs, unsigned int lower, unsigned int upper) {
	unsigned int opMask = addrs;
	lower = (31 - upper) + lower;

	opMask = opMask << (31 - upper);
	opMask = opMask >> lower;

	return opMask;
}

unsigned int my_log2(unsigned int num) {
	unsigned int count = 0;
	while (num != 1){
		num = (num / 2);
		count++;
	}
	return count;
}

unsigned int appendAddr(address addrs, unsigned int Tag, unsigned int Index, unsigned int Offset, unsigned int indexbits, unsigned int offsetbits) {
	addrs = Tag;
	addrs = addrs << indexbits;
	addrs += Index;
	addrs = addrs << offsetbits;
	addrs += Offset;
	return addrs;
}

unsigned int checkAge(unsigned int set_length) {	//only called when replaced, FULL cache
	unsigned int newIndexORoffset;
	unsigned int minVal = counter;
	for (int i = 0; i < set_length; i++) {
		if (cache[i].block[0].lru.value < minVal) {
			newIndexORoffset = i;					
			minVal = cache[i].block[0].lru.value;
		}
	}
	return newIndexORoffset;						//forces least recent to be replaced
}

/*-----END HELPER FUNCTIONS-----*/

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/
void accessMemory(address addr, word* data, WriteEnable we)
{
  /* Declare variables here */
	printf("Address (PC) : 0x%X\n", addr);	//testing
	//printf("Cache Data: 0x%X\n", *data);	//testing
	
	unsigned int tag, index, offset, offsetbits, indexbits, assocnum = 0;
	int used = 0;
	word* temp;
	TransferUnit blockSize;
	counter++;
	//printf("Set Count: %d\n", set_count);	//testing

	if (block_size == 2) {
		blockSize = HALF_WORD_SIZE;
	}
	else if (block_size == 4) {
		blockSize = WORD_SIZE;
	}
	else if (block_size == 8) {
		blockSize = DOUBLEWORD_SIZE;
	}
	else if (block_size == 16) {
		blockSize = QUADWORD_SIZE;
	}
	else if (block_size == 32) {
		blockSize = OCTWORD_SIZE;
	}

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

	  offsetbits = my_log2(block_size);
	  indexbits = my_log2(set_count);

	  offset = createMask(addr, 0, offsetbits - 1);
	  index = createMask(addr, offsetbits, offsetbits + indexbits - 1);
	  tag = createMask(addr, offsetbits + indexbits, 31);
  
  address dumAddr = (addr - offset);		//used for accessDRAM alteration (copying over whole block, no just the offset word/bytes)
 
  printf("index: %d\n", index);					//testing
  printf("offset: %d\n", offset);				//testing
  printf("dummy address: 0x%X\n", dumAddr);		//testing

  printf("index bits: %d\n", indexbits);		//testing
  printf("offset bits: %d\n", offsetbits);		//testing

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */
 
  /*------------------------------Direct-Mapped---------------------------------------------*/
  /*----------------------------------------------------------------------------------------*/

  if (assoc < 6) {			/*direct-mapped*/
	  if (we == READ) {
		  //direct-mapped read

		  if (cache[index].block[assocnum].valid == VALID) {
			  //valid read
			  
			  for (int i = 0; i < assoc; i++) {												//checking each block in set for tag
				  if (cache[index].block[i].tag == tag) {
					  assocnum = i;
					  used = 1;
				  }
			  }
			  if (used == 1) {	  //valid read tag match (HIT)
				  memcpy(data, cache[index].block[assocnum].data + offset, block_size);		//data read back size of word
				  if (policy == LRU) {
					  cache[index].block[assocnum].lru.value = counter;
				  }
				  highlight_block(index, assocnum);
				  highlight_offset(index, assocnum, offset, HIT);
				  printf("We're here0\n"); //testing
				  return;
			  }
			  if (cache[index].block[assocnum].tag != tag) {	//placeholder; I don't need this here because I checked for the opposite above
				  //valid read, tag mismatch (miss)
				  if (memory_sync_policy == WRITE_BACK) {
					  //data in block, tag mismatch, writeback

					  if (cache[index].block[assocnum].dirty == DIRTY) {
						  if (policy == LRU) {
							  //cache miss, replacement is LRU
							  if (assoc > 1) {
								  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
									  if (cache[index].block[i].valid == INVALID) {
										  assocnum = i;
										  used = 1;
										  break;
									  }
								  }
							  }
							  if (assoc == 1) {
								  index = checkAge(set_count);
							  }
							  else if (used == 0) {
								  offset = checkAge(set_count);
							  }
							  if (used == 0) {
								  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
								  dumAddr = (addr - offset);
								  memcpy(data, cache[index].block[offset].data, block_size);
								  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
								  memcpy(cache[index].block[offset].data, data, block_size);
								  cache[index].block[offset].tag = tag;
								  cache[index].block[offset].valid = VALID;
								  highlight_block(index, offset);
								  highlight_offset(index, offset, 0, MISS);
								  printf("We're here24\n");
								  return;
							  }
							  accessDRAM(dumAddr, (byte*)data, blockSize, READ);
							  memcpy(cache[index].block[assocnum].data, data, block_size);
							  cache[index].block[assocnum].tag = tag;
							  cache[index].block[assocnum].valid = VALID;
							  if (memory_sync_policy == WRITE_BACK) {
								  cache[index].block[assocnum].dirty = DIRTY;
							  }
							  else {	//write-through
								  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
							  }
							  if (policy == LRU) {
								  cache[index].block[assocnum].lru.value = counter;
							  }
							  highlight_block(index, assocnum);
							  highlight_offset(index, assocnum, offset, MISS);
							  printf("We're here1\n");
							  return;
						  }
						  else {
							  //cache miss, replacement is random
							  if (assoc > 1) {
								  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
									  if (cache[index].block[i].valid == INVALID) {
										  assocnum = i;
										  used = 1;
										  break;
									  }
								  }
							  }
							  if (assoc == 1) {
								  index = randomint(set_count);
							  }
							  else if (used == 0) {
								  offset = randomint(set_count);
							  }
							  if (used == 0) {
								  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
								  dumAddr = (addr - offset);
								  memcpy(data, cache[index].block[offset].data, block_size);
								  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
								  memcpy(cache[index].block[offset].data, data, block_size);
								  cache[index].block[offset].tag = tag;
								  cache[index].block[offset].valid = VALID;
								  highlight_block(index, offset);
								  highlight_offset(index, offset, 0, MISS);
								  printf("We're here20\n");
								  return;
							  }
							  accessDRAM(dumAddr, (byte*)data, blockSize, READ);
							  memcpy(cache[index].block[assocnum].data, data, block_size);
							  cache[index].block[assocnum].tag = tag;
							  cache[index].block[assocnum].valid = VALID;
							  if (memory_sync_policy == WRITE_BACK) {
								  cache[index].block[assocnum].dirty = DIRTY;
							  }
							  else {	//write-through
								  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
							  }
							  if (policy == LRU) {
								  cache[index].block[assocnum].lru.value = counter;
							  }
							  highlight_block(index, assocnum);
							  highlight_offset(index, assocnum, offset, MISS);
							  printf("We're here2\n"); //testing
							  return;
						  }
					  }
					  else {
						  //valid, tag mismatch, virgin block - does this ever happen?
						  memcpy(cache[index].block[assocnum].data, data, block_size);
						  cache[index].block[assocnum].dirty = DIRTY;
						  cache[index].block[assocnum].lru.value = counter;
						  cache[index].block[assocnum].tag = tag;
						  highlight_block(index, assocnum);
						  highlight_offset(index, assocnum, offset, MISS);
						  printf("We're here3\n"); //testing
						  return;
					  }
				  }
				  else {
					  //data in block, tag mismatch, write-through, LRU
					  if (policy == LRU) {
						  //data in block, tag mismatch, write-through, LRU
						  if (assoc > 1) {
							  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
								  if (cache[index].block[i].valid == INVALID) {
									  assocnum = i;
									  used = 1;
									  break;
								  }
							  }
						  }
						  if (assoc == 1) {
							  index = checkAge(set_count);
						  }
						  else if (used == 0) {
							  offset = checkAge(set_count);
						  }
						  if (used == 0) {
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
							  dumAddr = (addr - offset);
							  memcpy(data, cache[index].block[offset].data, block_size);
							  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
							  memcpy(cache[index].block[offset].data, data, block_size);
							  cache[index].block[offset].tag = tag;
							  cache[index].block[offset].valid = VALID;
							  highlight_block(index, offset);
							  highlight_offset(index, offset, 0, MISS);
							  printf("We're here21\n");
							  return;
						  }
						  accessDRAM(dumAddr, (byte*)data, blockSize, READ);
						  memcpy(cache[index].block[assocnum].data, data, block_size);
						  cache[index].block[assocnum].tag = tag;
						  cache[index].block[assocnum].valid = VALID;
						  if (memory_sync_policy == WRITE_BACK) {
							  cache[index].block[assocnum].dirty = DIRTY;
						  }
						  else {	//write-through
							  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
						  }
						  if (policy == LRU) {
							  cache[index].block[assocnum].lru.value = counter;
						  }
						  highlight_block(index, assocnum);
						  highlight_offset(index, assocnum, offset, MISS);
						  printf("We're here4\n"); //testing
						  return;
					  }
					  else {
					  //data in block, tag mismatch, write-through, random

						  if (assoc > 1) {
							  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
								  if (cache[index].block[i].valid == INVALID) {
									  assocnum = i;
									  used = 1;
									  break;
								  }
							  }
						  }
						  if (assoc == 1) {
							  index = randomint(set_count);
						  }
						  else if (used == 0) {
							  offset = randomint(set_count);
						  }
						  if (used == 0) {
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
							  dumAddr = (addr - offset);
							  memcpy(data, cache[index].block[offset].data, block_size);
							  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
							  memcpy(cache[index].block[offset].data, data, block_size);
							  cache[index].block[offset].tag = tag;
							  cache[index].block[offset].valid = VALID;
							  highlight_block(index, offset);
							  highlight_offset(index, offset, 0, MISS);
							  printf("We're here22\n");
							  return;
						  }
						  accessDRAM(dumAddr, (byte*)data, blockSize, READ);
						  memcpy(cache[index].block[assocnum].data, data, block_size);
						  cache[index].block[assocnum].tag = tag;
						  cache[index].block[assocnum].valid = VALID;
						  if (memory_sync_policy == WRITE_BACK) {
							  cache[index].block[assocnum].dirty = DIRTY;
						  }
						  else {	//write-through
							  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
						  }
						  if (policy == LRU) {
							  cache[index].block[assocnum].lru.value = counter;
						  }
						  highlight_block(index, assocnum);
						  highlight_offset(index, assocnum, offset, MISS);
						  printf("We're here5\n"); //testing
						  return;
					  }
				  }
			  }
		  }
		  else {
			  //invalid read (miss) (empty block) (compulsory)

			  accessDRAM(dumAddr, (byte*)data, blockSize, READ);	//reading from DRAM
			  memcpy(cache[index].block[assocnum].data, data, block_size);
			  cache[index].block[assocnum].valid = VALID;
			  if (memory_sync_policy == WRITE_BACK) {
				  cache[index].block[assocnum].dirty = DIRTY;
			  }
			  else {	//write-through
				  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
			  }
			  if (policy == LRU) {
				  cache[index].block[assocnum].lru.value = counter;
			  }
			  cache[index].block[assocnum].tag = tag;
			  highlight_block(index, assocnum);
			  highlight_offset(index, assocnum, offset, MISS);
			  printf("We're here6\n"); //testing
			  return;
		  }
	  }
	  else {
		  //direct-mapped write

		  if (cache[index].block[assocnum].valid == VALID) {
			  //valid write, data in block

			  for (int i = 0; i < assoc; i++) {												//checking each block in set for tag
				  if (cache[index].block[i].tag == tag) {
					  //tag match, cache hit
					  memcpy(cache[index].block[assocnum].data + offset, data, block_size);
					  cache[index].block[assocnum].tag = tag;	//do I need to reiterate this? Should I?
					  if (policy == LRU) {
						  cache[index].block[assocnum].lru.value = counter;
					  }
					  highlight_block(index, assocnum);
					  highlight_offset(index, assocnum, offset, HIT);
					  printf("We're here12\n"); //testing
					  return;
				  }
			  }

			  if (cache[index].block[assocnum].tag != tag) {	//placeholder; I don't need to check this because I checked the oppposite above
				  //write cache miss

				  if (memory_sync_policy == WRITE_BACK) {
					  //writing w/ WB

					  if (cache[index].block[assocnum].dirty == DIRTY) {
						  //block is dirty

						  if (policy == LRU) {
							  //LRU replacement

							  *temp = *data;
							  if (assoc > 1) {
								  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
									  if (cache[index].block[i].valid == INVALID) {
										  assocnum = i;
										  used = 1;
										  break;
									  }
								  }
							  }
							  if (assoc == 1) {
								  index = checkAge(set_count);
							  }
							  else if (used == 0) {
								  offset = checkAge(set_count);
							  }
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
							  dumAddr = (addr - offset);
							  memcpy(data, cache[index].block[assocnum].data + offset, block_size);
							  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
							  *data = *temp;
							  memcpy(cache[index].block[assocnum].data + offset, data, block_size);		//How to write in whole block? Data offset in bytes???? I don't understand
							  cache[index].block[assocnum].tag = tag;
							  cache[index].block[assocnum].valid = VALID;
							  cache[index].block[assocnum].lru.value = counter;
							  cache[index].block[assocnum].dirty = DIRTY;
							  highlight_block(index, assocnum);
							  highlight_offset(index, assocnum, offset, MISS);
							  printf("We're here7\n"); //testing
							  return;
						  }
						  else {
							  //random replacement, valid, WB, dirty
							  *temp = *data;
							  if (assoc > 1) {
								  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
									  if (cache[index].block[i].valid == INVALID) {
										  assocnum = i;
										  used = 1;
										  break;
									  }
								  }
							  }
							  if (assoc == 1) {
								  index = randomint(set_count);
							  }
							  else if (used == 0) {
								  offset = randomint(set_count);
							  }
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);	//new address for random
							  dumAddr = (addr - offset);
							  memcpy(data, cache[index].block[assocnum].data + offset, block_size);
							  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
							  *data = *temp;
							  memcpy(cache[index].block[assocnum].data + offset, data, block_size);
							  cache[index].block[assocnum].tag = tag;
							  cache[index].block[assocnum].valid = VALID;
							  cache[index].block[assocnum].dirty = DIRTY;
							  highlight_block(index, assocnum);
							  highlight_offset(index, assocnum, offset, MISS);
							  printf("We're here8\n"); //testing
							  return;
						  }
					  }
					  else {
						  //valid, miss, WB, block is virgin - does this happen?
						  memcpy(cache[index].block[0].data + offset, data, block_size);
						  cache[index].block[assocnum].tag = tag;
						  cache[index].block[assocnum].lru.value = counter;
						  cache[index].block[assocnum].dirty = DIRTY;
						  cache[index].block[assocnum].valid = VALID;
						  highlight_block(index, assocnum);
						  highlight_offset(index, assocnum, offset, MISS);
						  printf("We're here9\n"); //testing
						  return;
					  }
				  }
				  else {
					  //writing w/ WT
					  if (policy == LRU) {
						  //Write through with LRU
						  
						  if (assoc > 1) {
							  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
								  if (cache[index].block[i].valid == INVALID) {
									  assocnum = i;
									  used = 1;
									  break;
								  }
							  }
						  }
						  if (assoc == 1) {
							  index = checkAge(set_count);
						  }
						  else if (used == 0) {
							  offset = checkAge(set_count);
						  }
						  //need to change address
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
						  memcpy(cache[index].block[assocnum].data + offset, data, block_size);
						  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
						  cache[index].block[assocnum].tag = tag;
						  cache[index].block[assocnum].valid = VALID;
						  cache[index].block[assocnum].lru.value = counter;
						  highlight_block(index, assocnum);
						  highlight_offset(index, assocnum, offset, MISS);
						  printf("We're here10\n"); //testing
						  return;
					  }
					  else {
						  //Write through with random
						  if (assoc > 1) {
							  for (int i = 0; i < assoc; i++) {									//checking for empty blocks within set...
								  if (cache[index].block[i].valid == INVALID) {
									  assocnum = i;
									  used = 1;
									  break;
								  }
							  }
						  }
						  if (assoc == 1) {
							  index = randomint(set_count);
						  }
						  else if (used == 0) {
							  offset = randomint(set_count);
						  }
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);	//new address for random
						  dumAddr = (addr - offset);
						  memcpy(cache[index].block[assocnum].data + offset, data, block_size);
						  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
						  cache[index].block[assocnum].tag = tag;
						  cache[index].block[assocnum].valid = VALID;
						  highlight_block(index, assocnum);
						  highlight_offset(index, assocnum, offset, MISS);
						  printf("We're here11\n"); //testing
						  return;
					  }
				  }
			  }
				else {
					//placeholder; I should NEVER visit this branch
				}
		  }
			  else {
				  //invalid write, empty block (compulsory miss)
				  memcpy(cache[index].block[assocnum].data + offset, data, block_size);
				  cache[index].block[assocnum].tag = tag;
				  cache[index].block[assocnum].valid = VALID;
				  if (memory_sync_policy == WRITE_BACK) {
					  cache[index].block[assocnum].dirty = DIRTY;
				  }
				  else {
					  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
				  }
				  if (policy == LRU) {
					  cache[index].block[assocnum].lru.value = counter;
				  }
				  highlight_block(index, assocnum);
				  highlight_offset(index, assocnum, offset, MISS);
				  printf("We're here13\n"); //testing
				  return;
			  }
		  }
	  }

  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
}
