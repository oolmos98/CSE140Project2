#include "tips.h"
#include <stdbool.h>

/* Global variable for tracking the least recently used block
// Assigning a varible for least recently used set */

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
char *lfu_to_string(int assoc_index, int block_index)
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
char *lru_to_string(int assoc_index, int block_index)
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
This function will find which block in the given set hasnt been used the longest
 */
int findLRU(int index)
{
    //Index is which SET in the cache
    int max = 0, block = 0;
    //max is used to determine which value hasnt been used the longest.
    //block is used to hold the index of which block in cache to return.

    for (int i = 0; i < assoc; i++)
    {
        if (cache[index].block[i].lru.value > max)
        {
            max = cache[index].block[i].lru.value;
            block = i;
        }
    }
    return block;
}
/* 
This function will incrament the LRU values in the given set/block.
 */
void incLRU(int sindex, int bindex)
{
    //sindex is the SETS index.
    //bindex is the BLOCKS index
    for (int i = 0; i < assoc; i++)
    {
        //If valid, inc LRU. else data is not valid to use.
        if (cache[sindex].block[i].valid == VALID)
            cache[sindex].block[i].lru.value++;
    }
    //Actual set and block used is set to 1.
    cache[sindex].block[bindex].lru.value = 1;
}

/*
The main function 
*/
void accessMemory(address addr, word *data, WriteEnable we)
{
    unsigned int tagBits, //Matching Tags
        indexBits,        //Which set
        offsetBits,       //Where in block.
        tag,             //Actual tag value.
        index,           //Actual index/set value.
        offset;          //Actual offset value.
    bool hit = false; //Verify HIT or MISS.
    unsigned int addr2;                                 //Used for Writeback policy
    TransferUnit transfer_unit = uint_log2(block_size); //Used to determine size for AccessDRAM.

    /* handle the case of no cache at all - leave this in */
    if (assoc == 0)
    {
        accessDRAM(addr, (byte *)data, WORD_SIZE, we);
        return;
    }

    //Calculates #bits for each section
    indexBits = uint_log2(set_count);
    offsetBits = uint_log2(block_size);
    tagBits = 32 - indexBits - offsetBits;

    //Actual values for each section utilizing the above calculations
    index = (addr << tagBits) >> (tagBits + offsetBits);
    offset = (addr << (tagBits + indexBits)) >> (tagBits + indexBits);
    tag = (addr >> (offsetBits + indexBits));

    //Need to determine which block from the index is available/valid
    //if not, miss
    int accessedBlock;
    for (int i = 0; i < assoc; i++)
    {
        if (cache[index].block[i].tag == tag && cache[index].block[i].valid)
        {
            //Hit always false until here.
            hit = true;
            //The verified block in the index to work!
            accessedBlock = i;
            break;
        }
    }
    if(hit){
        //If hit, highlight green
        highlight_offset(index, accessedBlock, offset, HIT);
    }
    else // Miss
    {
        //LFU not implemented, Only LRU/Random

        //LRU Case
        //Since a miss, need to find the block in the index that matches the LRU case.
        switch (policy) {
            case LFU:
            {
                int minIndex = 0;
                for(int i = 0 ; i < assoc ; i++) {
                    if(cache[index].block[minIndex].accessCount < cache[index].block[i].accessCount) {
                        minIndex = i;
                    }
                }
                accessedBlock = minIndex;
                cache[index].block[accessedBlock].accessCount++;
                break;
            }
            case LRU:
            {
                accessedBlock = findLRU(index);
                //Must Increment LRU in the index.
                incLRU(index, accessedBlock);
                break;
            }
            case RANDOM: {
                accessedBlock = randomint(assoc);
                break;
            }
            default:
                accessedBlock = randomint(assoc);
        }

        //WriteBack Policy.
        if (memory_sync_policy == WRITE_BACK)
        {
            //Confirm the dirty bit.
            if (cache[index].block[accessedBlock].dirty)
            {
                //Finding correct address to copy cache to memory.
                addr2 = (cache[index].block[accessedBlock].tag << (indexBits + offsetBits)) + (index << offsetBits);
                //Accessing WRITE to memory.
                accessDRAM(addr2, cache[index].block[accessedBlock].data, transfer_unit, WRITE);
                //After, block in index is not dirty.
                cache[index].block[accessedBlock].dirty = VIRGIN;
            }
            cache[index].block[accessedBlock].dirty = VIRGIN;
            if(policy == LFU) {
                cache[index].block[accessedBlock].accessCount = 0;
            }
        }
        ///Now the cache and memory are in sync, we can read the memory
        accessDRAM(addr, cache[index].block[accessedBlock].data, transfer_unit, READ);



        //update the cache values.
        cache[index].block[accessedBlock].tag = tag;
        cache[index].block[accessedBlock].valid = VALID;
        //For tips to highlight for miss.
        highlight_block(index, accessedBlock);
        highlight_offset(index, accessedBlock, offset, MISS);
    }
    //After the above switch case, now we have the correct values to access the memory and cache
    //and do the appropriate functions.
    switch (we)
    {
        //Reading Case
    case READ:
    {
        memcpy(data, cache[index].block[accessedBlock].data + offset, 4);
        break;
    }
    //Writing Case
    case WRITE:
    {
        memcpy(cache[index].block[accessedBlock].data + offset, data, 4);

        //WriteThrough keeps the main memory Up-to-Date
        //WriteBack is either cache or memory that has up-to-date data, hence the DIRTY bit.
        if (memory_sync_policy == WRITE_THROUGH)
            accessDRAM(addr2, cache[index].block[accessedBlock].data, transfer_unit, WRITE);
        else
            cache[index].block[accessedBlock].dirty = DIRTY;
    }
    }
}