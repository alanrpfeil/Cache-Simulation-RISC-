#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
//unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

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
	
	unsigned int minVal = 0xFFFFFFFF;
	unsigned int tag, index, offset, offsetbits, indexbits, rows = (set_count * assoc);
	word* temp;
	TransferUnit blockSize;

	//printf("Set Count: %d\n", set_count);	//testing

	if (block_size == 1) {
		blockSize = HALF_WORD_SIZE;
	}
	else if (block_size == 2) {
		blockSize = WORD_SIZE;
	}
	else if (block_size == 4) {
		blockSize = DOUBLEWORD_SIZE;
	}
	else if (block_size == 8) {
		blockSize = QUADWORD_SIZE;
	}
	else if (block_size == 16) {
		blockSize = OCTWORD_SIZE;
	}

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  if (assoc == 1) {	//direct mapped setup
	  offsetbits = my_log2(block_size);
	  indexbits = my_log2(rows);

	  offset = createMask(addr, 0, offsetbits - 1);
	  index = createMask(addr, offsetbits, offsetbits + indexbits - 1);
	  tag = createMask(addr, offsetbits + indexbits, 31);
  }
  else {			//n-way set assoc setup
	  offsetbits = my_log2(block_size);
	  indexbits = my_log2(set_count);

	  offset = createMask(addr, 0, offsetbits - 1);
	  index = createMask(addr, offsetbits, offsetbits + indexbits - 1);
	  tag = createMask(addr, offsetbits + indexbits, 31);
  }
  
  address dumAddr = (addr - offset);		//used for accessDRAM alteration (copying over whole block, no just the offset part)
  //dumAddr = addr;
 
  //printf("index: %d\n", index);
  //printf("offset: %d\n", offset);
  printf("dummy address: 0x%X\n", dumAddr);

  //printf("index bits: %d\n", indexbits);		//testing
  //printf("offset bits: %d\n", offsetbits);		//testing

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
  /*----------------------------------------------------------------------------------------*/	//fix LRU to not act like LFU
 
  if (assoc == 1) {			/*direct-mapped*/
	  if (we == READ) {
		  //direct-mapped read

		  if (cache[index].block[0].valid == VALID) {
			  //valid read
			  if (cache[index].block[0].tag == tag) {
				  //valid read tag match (hit)
				  *data = cache[index].block[offset].data[0];				//data read back
				  cache[index].block[offset].lru.value++;					
				  cache[index].block[offset].tag = tag;
				  printf("We're here0\n");
				  return;
			  }
			  else {
				  //valid read, tag mismatch (miss)
				  if (memory_sync_policy == WRITE_BACK) {
					  //data in block, tag mismatch, writeback

					  if (cache[index].block[0].dirty == DIRTY) {
						  if (policy == LRU) {
							  //cache miss, replacement is LRU
							  for (int i = 0; i < block_size; i++) {								//finds least recently-used block
								  if (cache[i].block[0].lru.value < minVal) {
									  offset = i;													//sets new offset to LRU block; finds block with smallest LRU value
									  minVal = cache[i].block[0].lru.value;
								  }
							  }
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);	//new address for LRU
							  dumAddr = (addr - offset);
							  accessDRAM(addr, (byte*)data, blockSize, READ);						//reading from DRAM
							  cache[index].block[offset].data[blockSize] = *data;					//read data now in cache
							  cache[index].block[0].lru.value++;
							  cache[index].block[0].tag = tag;
							  printf("We're here1\n");
							  return;
						  }
						  else {
							  //cache miss, replacement is random
							  offset = randomint(rows);
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);	//new address for random
							  dumAddr = (addr - offset);
							  accessDRAM(addr, (byte*)data, blockSize, READ);
							  cache[index].block[offset].data[blockSize] = *data;
							  cache[index].block[0].tag = tag;
							  printf("We're here2\n");
							  return;
						  }
					  }
					  else {
						  //valid, tag mismatch, virgin block
						  cache[index].block[offset].data[blockSize] = *data;
						  cache[index].block[0].dirty = DIRTY;
						  cache[index].block[0].lru.value++;
						  cache[index].block[0].tag = tag;
						  printf("We're here3\n");
						  return;
					  }
				  }
				  else {
					  //data in block, tag mismatch, write-through
					  if (policy == LRU) {
						  //data in block, tag mismatch, write-through, LRU
						  for (int i = 0; i < block_size; i++) {						//finds least recently-used block
							  if (cache[i].block[0].lru.value < minVal) {
								  offset = i;													//sets new offset to LRU block; finds block with smallest LRU value
								  minVal = cache[i].block[0].lru.value;
							  }
						  }
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
						  accessDRAM(addr, (byte*)data, blockSize, READ);
						  cache[index].block[offset].data[blockSize] = *data;
						  cache[index].block[0].lru.value++;
						  cache[index].block[0].tag = tag;
						  printf("We're here4\n");
						  return;
					  }
					  else {
						  //data in block, tag mismatch, write-through, random
						  offset = randomint(rows);
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);	//new address for random
						  dumAddr = (addr - offset);
						  accessDRAM(addr, (byte*)data, blockSize, READ);
						  cache[index].block[offset].data[blockSize] = *data;
						  cache[index].block[0].tag = tag;
						  printf("We're here5\n");
						  return;
					  }
				  }
			  }
		  }
		  else {
			  //invalid read (miss) (empty block)
			  accessDRAM(dumAddr, (byte*)data, blockSize, READ);	//reading from DRAM
			  cache[index].block[offset].data[0] = *data;	//read data now in cache
			  cache[index].block[0].valid = VALID;
			  if (memory_sync_policy == WRITE_BACK) {
				  cache[index].block[0].dirty = DIRTY;
			  }
			  else {	//write-through
				  accessDRAM(addr, (byte*)data, blockSize, WRITE);
			  }
			  if (policy == LRU) {
				  cache[index].block[0].lru.value++;
			  }
			  cache[index].block[0].tag = tag;
			  printf("We're here6\n");
			  return;
		  }
	  }
	  else {
		  //direct-mapped write

		  if (cache[index].block[0].valid == VALID) {
			  //valid write, data in block

			  if (cache[index].block[0].tag != tag) {
				  //write cache miss

				  if (memory_sync_policy == WRITE_BACK) {
					  //writing w/ WB

					  if (cache[index].block[0].dirty == DIRTY) {
						  //block is dirty

						  if (policy == LRU) {
							  //LRU replacement

							  for (int i = 0; i < block_size; i++) {						//finds least recently-used block
								  if (cache[i].block[0].lru.value < minVal) {
									  offset = i;													//sets new offset to LRU block; finds block with smallest LRU value
									  minVal = cache[i].block[0].lru.value;
								  }
							  }
							  //need to change address
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
							  dumAddr = (addr - offset);
							  accessDRAM(dumAddr, (byte*)data, blockSize, READ);
							  cache[index].block[offset].data[blockSize] = *data;	//How to write in whole block? Data offset in bytes???? I don't understand
							  cache[index].block[0].tag = tag;
							  cache[index].block[0].lru.value++;
							  cache[index].block[0].dirty = DIRTY;
							  printf("We're here7\n");
							  return;
						  }
						  else {
							  //random replacement, valid, WB, dirty
							  offset = randomint(rows);
							  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);	//new address for random
							  dumAddr = (addr - offset);
							  accessDRAM(dumAddr, (byte*)data, blockSize, READ);
							  cache[index].block[offset].data[blockSize] = *data;
							  cache[index].block[0].tag = tag;
							  cache[index].block[0].dirty = DIRTY;
							  printf("We're here8\n");
							  return;
						  }
					  }
					  else {
						  //block is virgin
						  cache[index].block[offset].data[blockSize] = *data;
						  cache[index].block[0].tag = tag;
						  cache[index].block[0].lru.value++;							//do I need an if-statement for if LRU, LRU.value++, or can i increment it anyway if random policy?
						  cache[index].block[0].dirty = DIRTY;
						  cache[index].block[0].valid = VALID;
						  printf("We're here9\n");
						  return;
					  }
				  }
				  else {
					  //writing w/ WT
					  if (policy == LRU) {
						  //Write through with LRU
						  for (int i = 0; i < block_size; i++) {						//finds least recently-used block
							  if (cache[i].block[0].lru.value < minVal) {
								  offset = i;													//sets new offset to LRU block; finds block with smallest LRU value
								  minVal = cache[i].block[0].lru.value;
							  }
						  }
						  //need to change address
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
						  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
						  cache[index].block[offset].data[blockSize] = *data;
						  cache[index].block[0].tag = tag;
						  cache[index].block[0].lru.value++;
						  printf("We're here10\n");
						  return;
					  }
					  else {
						  //Write through with random
						  offset = randomint(rows);
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);	//new address for random
						  dumAddr = (addr - offset);
						  cache[index].block[offset].data[blockSize] = *data;
						  cache[index].block[0].tag = tag;
						  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
						  printf("We're here11\n");
						  return;
					  }
				  }
			  }
				else {
					//tag match, cache hit
					cache[index].block[offset].data[blockSize] = *data;
					cache[index].block[0].tag = tag;
					cache[index].block[0].lru.value++;
					printf("We're here12\n");
					return;
				}
		  }
			  else {
				  //invalid write, empty block (compulsory miss)
				  cache[index].block[offset].data[blockSize] = *data;
				  cache[index].block[0].tag = tag;
				  if (memory_sync_policy == WRITE_BACK) {
					  cache[index].block[0].dirty = DIRTY;
				  }
				  else {
					  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
				  }
				  if (policy == LRU) {
					  cache[index].block[0].lru.value++;
				  }
				  printf("We're here13\n");
				  return;
			  }
		  }
	  }

  /*------------------------------N-Way-Set-Associative-------------------------------------*/
  /*----------------------------------------------------------------------------------------*/

  else {	/*2-way set associative*/

	  if (we == READ) {
		  //2-way set read

		  if (cache[index].block[offset].valid == VALID) {
			  //2-way valid
			  
			  if (cache[index].block[offset].tag == tag) {
				  //valid block, matching tag

				  *data = cache[index].block[offset].data[0];
				  cache[index].block[offset].tag = tag;
				  if (policy == LRU) {
					  cache[index].block[offset].lru.value++;
				  }
				  printf("We're here2\n");
				  return;
			  }
			  else {
				  //valid block, mismatching tag

				  if (memory_sync_policy == WRITE_BACK) {
					  //tag mismatch w/ WB

					  if (policy == LRU) {
						  for (int i = 0; i < assoc; i++) {				//finds least recently-used block
							  unsigned int minVal = 0xFFFFFFFF;
							  if (cache[index].block[i].lru.value < minVal) {
								  offset = i;											//sets new offset to LRU block; finds block with smallest LRU value
							  }
						  }
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }
					  else {
						  offset = randomint(block_size);								//random replacement new block
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }

					  if (cache[index].block[offset].dirty == DIRTY) {
						  //tag mismatch w/ WB Dirty bit
							
						    *temp = *data;													//making sure *temp = *data
						    accessDRAM(dumAddr, (byte*) temp, blockSize, READ);
							*data = cache[index].block[offset].data[0];						//temp point to data in cache
							accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);				//write original block to cache
							*data = *temp;													//go back to new block you're putting in	
							cache[index].block[offset].data[0] = *data;
							cache[index].block[offset].tag = tag;
							if (policy == LRU) {
								cache[index].block[offset].lru.value++;
							 }
							cache[index].block[offset].dirty = DIRTY;
							printf("We're here3\n");
							return;
					  }
					  else {
						  //tag mismatch w/ WB Virgin Bit

							accessDRAM(dumAddr, (byte*)data, blockSize, READ);
							cache[index].block[offset].data[0] = *data;	
							cache[index].block[offset].tag = tag;
							if (policy == LRU) {
								cache[index].block[offset].lru.value++; 
							}
							cache[index].block[offset].dirty = DIRTY;
							printf("We're here4\n");
							return;
					  }
				  }
				  else {
					  //tag mismatch w/ WT
					
					  accessDRAM(dumAddr, (byte*)data, blockSize, READ);					//first before getting replacement block, get data from DRAM

					  if (policy == LRU) {
						  for (int i = 0; i < assoc; i++) {								//finds least recently-used block
							  unsigned int minVal = 0xFFFFFFFF;
							  if (cache[index].block[i].lru.value < minVal) {
							  	  offset = i;											//sets new offset to LRU block; finds block with smallest LRU value
							  }
						  }
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }
					  else {
						  offset = randomint(block_size);								//random replacement new block
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }

					  cache[index].block[offset].data[0] = *data;				//How to write in whole block? Data offset in bytes???? I don't understand
					  cache[index].block[offset].tag = tag;
					  accessDRAM(dumAddr, (byte*) data, blockSize, WRITE);
					  if (policy == LRU) {
						  cache[index].block[offset].lru.value++;
					  }
					  printf("We're here5\n");
					  return;
				  }
			  }
		  }
		  else {
			  //2-way invalid read, compulsory miss
			  printf("We're here\n");
			  accessDRAM(dumAddr, (byte*)data, blockSize, READ);
			  cache[index].block[offset].data[0] = *data;
			  cache[index].block[offset].tag = tag;
			  cache[index].block[offset].valid = VALID;
			  if (memory_sync_policy == WRITE_BACK) {
				  cache[index].block[offset].dirty = DIRTY;
			  }
			  else {
				  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
			  }
			  if (policy == LRU) {
				  cache[index].block[offset].lru.value++;
			  }
			  return;
		  }
	  }
	  else {
		  //2-way set write
		  if (cache[index].block[offset].valid == VALID) {
			  //valid write

			  if (cache[index].block[offset].tag != tag) {
				 //tag mismatch
				  
				  if (memory_sync_policy == WRITE_BACK) {
					  //writing w/ WB
					  //need to change to replacement block before evaluating any further...

					  if (policy == LRU) {
						  for (int i = 0; i < assoc; i++) {				//finds least recently-used block
							  unsigned int minVal = 0xFFFFFFFF;
							  if (cache[index].block[i].lru.value < minVal) {
								  offset = i;											//sets new offset to LRU block; finds block with smallest LRU value
							  }
						  }
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }
					  else {
						  offset = randomint(block_size);								//random replacement new block
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }

					  if (cache[index].block[offset].dirty == DIRTY) {
						  //block is dirty
						  *temp = *data;												//making sure *temp = *data (hold onto new data to write)
						  *data = cache[index].block[offset].data[0];					//temp point to data in cache
						  accessDRAM(dumAddr, (byte*) data, blockSize, WRITE);			//write original block to cache
						  *data = *temp;												//go back to new block you're putting in
						  cache[index].block[offset].data[0] = *data;
						  if (policy == LRU) {
							  cache[index].block[offset].lru.value++;
						  }
					 	  cache[index].block[offset].dirty = DIRTY;
						  printf("We're here6\n");
					      return;
					  }
					  else {
						  //block is virgin

						  cache[index].block[offset].data[0] = *data;
						  cache[index].block[offset].dirty = DIRTY;
						  cache[index].block[offset].tag = tag;
						  if (policy == LRU) {
							  cache[index].block[offset].lru.value++;
						  }
						  printf("We're here7\n");
						  return;
					  }
				  }
				  else {
					  //tag mismatch, write-through

					  if (policy == LRU) {
						  for (int i = 0; i < assoc; i++) {				//finds least recently-used block
							  unsigned int minVal = 0xFFFFFFFF;
							  if (cache[index].block[i].lru.value < minVal) {
								  offset = i;									//sets new offset to LRU block; finds block with smallest LRU value
							  }
						  }
						  //need to change address
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }
					  else {
						  offset = randomint(block_size);
						  addr = appendAddr(addr, tag, index, offset, indexbits, offsetbits);
						  dumAddr = (addr - offset);
					  }
					  cache[index].block[offset].data[0] = *data;
					  if (policy == LRU) {
						  cache[index].block[offset].lru.value++;
					  }
					  cache[index].block[offset].tag = tag;
					  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
					  printf("We're here8\n");
					  return;
				  }
			  }
			  else {
				  //tag match
				  
				  cache[index].block[offset].data[0] = *data;	
				  cache[index].block[offset].tag = tag;
				  if (memory_sync_policy == WRITE_BACK) {
					  cache[index].block[offset].dirty = DIRTY;			//correct logic for tag match write?
				  }
				  else {	
					  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);	//correct logic for tag match write?
				  }
				  if (policy == LRU) {
					  cache[index].block[offset].lru.value++;
				  }
				  printf("We're here9\n");
				  return;
			  }
		  }
		  else {
			  //invalid write, empty block

			  cache[index].block[offset].data[0] = *data;
			  cache[index].block[offset].tag = tag;
			  if (memory_sync_policy == WRITE_BACK) {
				  cache[index].block[offset].dirty = DIRTY;
			  }
			  else { //WT
				  accessDRAM(dumAddr, (byte*)data, blockSize, WRITE);
			  }
			  if (policy == LRU) {
				  cache[index].block[offset].lru.value++;
			  }
			  cache[index].block[offset].valid = VALID;
			  //printf("We're here10\n");
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
