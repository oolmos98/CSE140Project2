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

//  This function find the last recently used block

int findLRU(int index)
{
    int max = 0, block = 0;
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

void incLRU(int sindex, int bindex)
{
    for (int i = 0; i < assoc; i++)
    {
        if (cache[sindex].block[i].valid == VALID)
            cache[sindex].block[i].lru.value++;
    }

    cache[sindex].block[bindex].lru.value = 1;
}
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
    unsigned int addr2; //Used for Writeback
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
        //LFU not implemented, Only LRU/Random

        //LRU Case
        if (policy == LRU)
        {
            accessedBlock = findLRU(index);
            incLRU(index, accessedBlock);
        }
        else //Random Case
            accessedBlock = randomint(assoc);

        //WriteBack checks if dirty before copying.
        if (memory_sync_policy == WRITE_BACK)
        {

            if (cache[index].block[accessedBlock].dirty)
            {
                //Finding correct address to copy
                addr2 = (cache[index].block[accessedBlock].tag << (indexBits + offsetBits)) + (index << offsetBits);
                accessDRAM(addr2, cache[index].block[accessedBlock].data, transfer_unit, WRITE);
                cache[index].block[accessedBlock].dirty = VIRGIN;
            }
            cache[index].block[accessedBlock].dirty = VIRGIN;
        }

        accessDRAM(addr, cache[index].block[accessedBlock].data, transfer_unit, READ);
        cache[index].block[accessedBlock].tag = tag;
        cache[index].block[accessedBlock].valid = VALID;
        highlight_block(index, accessedBlock);
        highlight_offset(index, accessedBlock, offset, MISS);
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
