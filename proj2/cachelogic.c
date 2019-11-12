#include "tips.h"
#include <stdbool.h>

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

    lru_count[assoc_index] = 0; // We are basically setting LRU to 0
}

//  This function find the least recently used block

int findLRU(int index)
{
    int minimum = 0;
    for (int i = 0; i < assoc; i++)
    {
        if (cache[index].block[i].valid == INVALID)
            return i;
        if (cache[index].block[i].lru.value < cache[index].block[minimum].lru.value)
            minimum = i;
    }

    return minimum;
}

void incrmntLRU(int sindex, int bindex)
{
    int minValLRU;
    cache[sindex].block[bindex].lru.value = ++lru_count[sindex];
    if (lru_count[sindex] - 1 > lru_count[sindex])
    {
        minValLRU = cache[sindex].block[findLRU(sindex)].lru.value;

        int i = 0;
        while (i < assoc)
        {
            cache[sindex].block[i].lru.value += (0xFFFFFFFF - minValLRU);
            ++i;
        }
    }
}

// Basically what we are doing here is going through each set of the cache box and comparing it to the past set in order to determine which one is least frequently used.
//Function use LFU counter to increment LFU value of accessed block. If overflow, LFU value shift to maintain LRU counter.

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

 
*/
void accessMemory(address addr, word *data, WriteEnable we)
{

    unsigned int tagBits, //Matching Tags
        indexBits,        //Which set
        offsetBits,       //Where in block.
        tag, index, offset;
    bool hit = false;
    unsigned int addr2;
    TransferUnit transfer_unit = uint_log2(block_size);

    /* handle the case of no cache at all - leave this in */

    if (assoc == 0)
    {
        accessDRAM(addr, (byte *)data, WORD_SIZE, we);
        return;
    }
    ////////////////////////////////

    indexBits = uint_log2(set_count);
    offsetBits = uint_log2(block_size);
    tagBits = 32 - indexBits - offsetBits;

    index = (addr << tagBits) >> (tagBits + offsetBits);
    offset = (addr << (tagBits + indexBits)) >> (tagBits + indexBits);
    tag = (addr >> (offsetBits + indexBits));

    int accessedBlock;
    for (int i = 0; i < assoc; i++)
    {
        if (cache[index].block[i].tag == tag && cache[index].block[i].valid)
        {
            highlight_offset(index, i, offset, HIT);
            hit = true;
            accessedBlock = i;
            break;
        }
    }

    if (hit == false) //Miss
    {

        if (policy == LRU)
        {
            accessedBlock = findLRU(index);
            incrmntLRU(index, accessedBlock);
        }
        else
            accessedBlock = randomint(assoc);

        highlight_block(index, accessedBlock);
        highlight_offset(index, accessedBlock, offset, MISS);

        //WriteBack checks if dirty before copying.
        if (memory_sync_policy == WRITE_BACK && cache[index].block[accessedBlock].dirty)
        {
            //Finding correct address to
            addr2 = (cache[index].block[accessedBlock].tag << (indexBits + offsetBits)) + (index << offsetBits);
            accessDRAM(addr2, cache[index].block[accessedBlock].data, transfer_unit, WRITE);
        }

        accessDRAM(addr, cache[index].block[accessedBlock].data, transfer_unit, READ);
        cache[index].block[accessedBlock].tag = tag;
        cache[index].block[accessedBlock].valid = VALID;

        if (memory_sync_policy == WRITE_BACK)
        {
            cache[index].block[accessedBlock].dirty = VIRGIN;
        }
    }

    switch (we)
    {
    case READ:
    {
        memcpy(data, cache[index].block[accessedBlock].data + offset, 4);
        break;
    }
    case WRITE:
    {
        memcpy(cache[index].block[accessedBlock].data + offset, data, 4);
        if (memory_sync_policy == WRITE_THROUGH)
            accessDRAM(addr2, cache[index].block[accessedBlock].data, transfer_unit, WRITE);
        else
            cache[index].block[accessedBlock].dirty = DIRTY;
    }
    }
}
