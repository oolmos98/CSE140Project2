#include "tips.h"


/* Global variable for tracking the least recently used block
// Assigning a varible for least recently used set */
unsigned int lru_count[MAX_SETS];

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w);

/* return random int from 0..x-1 */
int randomint(int x);

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char *lfu_to_string(int assoc_index, int block_index) {
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
char *lru_to_string(int assoc_index, int block_index) {
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
void init_lfu(int assoc_index, int block_index) {
    cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index) {
    cache[assoc_index].block[block_index].lru.value = 0;

    lru_count[assoc_index] = 0;     // We are basically setting LRU to 0
}


//  This function find the least recently used block 

 
int findLRU(int setInd) {
    int minimumLRU = 0;
	int i = 0;
	while(i < assoc){
		if(cache[setInd].block[i].valid == INVALID){
            return i;  //returns the index of least recently used block

		}
		
		if(cache[setInd].block[i].lru.value < cache[setInd].block[minimumLRU].lru.value){
			minimumLRU = i;
		}
		++i;
	}
	return minimumLRU;

    
    /* Basically what we are doing here are going through the cache box starting with the 1st set that has index 0, we assign it to i and than we compare to the next index e.g index 1 and than we
     check which one is least used and assign that index to the variable minimumLRU and it loops through the whole cache box */
}

//Function use LRU counter to increment LRU value of accessed block. If overflow, LRU value shift to maintain LRU counter.

void incrmntLRU(int setInd, int block_index) {
    int minValLRU;
    cache[setInd].block[block_index].lru.value = ++lru_count[setInd];
    if(lru_count[setInd]-1 > lru_count[setInd]) {
        minValLRU = cache[setInd].block[findLRU(setInd)].lru.value;
        
   
        int i = 0;
        while(i < assoc){
            cache[setInd].block[i].lru.value += (0xFFFFFFFF - minValLRU);
            ++i;
        
    
        }
    }
}


// Function finds the least frequently used block


int findLFU(int setInd) {
    int minimumLFU  = 0;
	
    int i = 0;
    while(i < assoc){
        if(cache[setInd].block[i].valid == INVALID){// If it has nothing inside the bit that consider it invalid
            return i;
        }
        
        if(cache[setInd].block[i].accessCount < cache[setInd].block[minimumLFU ].accessCount){
            minimumLFU  = i;
        }
        ++i;
    }

    
    
    return minimumLFU ;
}






// Basically what we are doing here is going through each set of the cache box and comparing it to the past set in order to determine which one is least frequently used.
//Function use LFU counter to increment LFU value of accessed block. If overflow, LFU value shift to maintain LRU counter.
void incrmntLFU(int setInd, int block_index) {
    cache[setInd].block[block_index].accessCount++;
    // In order for it to Handle overflow
    if(cache[setInd].block[block_index].accessCount-1 > cache[setInd].block[block_index].accessCount) {
        cache[setInd].block[block_index].accessCount--;
        
        
        
        int i = 0;
        while(i < assoc){
            cache[setInd].block[block_index].accessCount /= 2;
            ++i;
        
        }
    }
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

 
*/
void accessMemory(address addr, word *data, WriteEnable we) {

    /* Declare variables here */
    unsigned int index_bit_count;
    unsigned int temp;
    unsigned int tag_bit_count;
    unsigned int offset_bit_count;
    unsigned int tag;
    unsigned int offset;
    unsigned int set;
    unsigned int hit = 0;
    unsigned int accessed_block;
    unsigned int write_back_addr;
    TransferUnit transfer_unit = 0;

    /* handle the case of no cache at all - leave this in */
    if (assoc == 0) {
        accessDRAM(addr, (byte *) data, WORD_SIZE, we);
        return;
    }

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

    /* Start adding code here */
  
    transfer_unit = uint_log2(block_size);

    index_bit_count = uint_log2(set_count);
    offset_bit_count = uint_log2(block_size);
    tag_bit_count = 32 - (index_bit_count + offset_bit_count);
    
    temp = addr<< (32 - offset_bit_count);              // In order for you to get the offset you have to shift it to the left in order to eliminate the index and tag bits and than shift it back right
    offset = temp>> (32- offset_bit_count);
   
    
    temp = (addr >> offset_bit_count);              // In order for you to get the set value you first have to shift it right by the offset bits and than you shift it left to get rid of the tags and than shift it back
    temp= temp<< (32- index_bit_count);
    temp= temp>> (32- index_bit_count);
    set = temp;
    
    tag = addr >> (offset_bit_count + index_bit_count);     // In order to get the tag you may just shift right to get rid of the index and offset bits

    
    
    int i = 0;
    while(i < assoc){
        if(cache[set].block[i].tag == tag && cache[set].block[i].valid) {
            accessed_block = i;
            hit = 1;
            highlight_offset(set, accessed_block, offset, HIT);
            break;
        }
        ++i;
    }


switch (hit)
case 0: {                                                                                           // If in case its a miss
    
    
        if(policy == LRU){
            accessed_block = findLRU(set);
            incrmntLRU(set, accessed_block);
            
        }
    
        
        else if (policy == LFU){
            accessed_block = findLFU(set);
            incrmntLFU(set, accessed_block);
            
        }
        else
            accessed_block = randomint(assoc);
        
        highlight_block(set, accessed_block);
        highlight_offset(set, accessed_block, offset, MISS);

        if(memory_sync_policy == WRITE_BACK && cache[set].block[accessed_block].dirty) {
            write_back_addr = (cache[set].block[accessed_block].tag << (index_bit_count + offset_bit_count)) + (set << offset_bit_count);
            accessDRAM(write_back_addr, cache[set].block[accessed_block].data, transfer_unit, WRITE);
        }

        accessDRAM(addr, cache[set].block[accessed_block].data, transfer_unit, READ);

        if(policy == LFU)
            cache[set].block[accessed_block].accessCount = 0;

        cache[set].block[accessed_block].tag = tag;
        cache[set].block[accessed_block].valid = VALID;

        if(memory_sync_policy == WRITE_BACK)
            cache[set].block[accessed_block].dirty = VIRGIN;
    }

    switch (we){
        case READ:{
            /* in this case READ: the data is used to return
            information back to CPU */
            memcpy(data, cache[set].block[accessed_block].data + offset, 4);}
            break;
        case WRITE:                     //In this case WRITE: the data is used to update Cache/DRAM
        memcpy(cache[set].block[accessed_block].data + offset, data, 4);
        if(memory_sync_policy == WRITE_THROUGH)
            accessDRAM(write_back_addr, cache[set].block[accessed_block].data, transfer_unit, WRITE);
        else
            cache[set].block[accessed_block].dirty = DIRTY;
    }
    }


