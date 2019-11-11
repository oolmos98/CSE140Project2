#include "tips.h"
#include <stdbool.h>

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
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/

//Helper Functions

int lruBlock(int index)
{
  int minimum = 0;
  for (int i = 0; i < assoc; i++)
  {
    (cache[index].block[i].valid == INVALID) ? i : ((cache[index].block[i].lru.value < cache[index].block[minimum].lru.value) ? minimum = i : 0);
  }
  return minimum;
}
int lfuBlock(int index)
{
  int minimum = 0;
  for (int i = 0; i < assoc; i++)
  {
    (cache[index].block[i].valid == INVALID) ? i : ((cache[index].block[i].accessCount < cache[index].block[minimum].accessCount) ? minimum = i : 0);
  }
  return minimum;
}

void lfuplusplus(int sindex, int bindex)
{
  cache[index].block[bindex].accessCount++;
  // Handle overflow
  /*if (cache[set_index].block[block_index].accessCount - 1 > cache[set_index].block[block_index].accessCount)
  {
    cache[set_index].block[block_index].accessCount--;
    for (int i = 0; i < assoc; ++i)
    {
      cache[set_index].block[block_index].accessCount /= 2;
    }
  }*/
}
void increment_lru(int set_index, int block_index)
{
  int min_lru_val;
  cache[set_index].block[block_index].lru.value = ++lru_count[set_index];
  if (lru_count[set_index] - 1 > lru_count[set_index])
  {
    min_lru_val = cache[set_index].block[find_lru_block(set_index)].lru.value;
    for (int i = 0; i < assoc; ++i)
    {
      cache[set_index].block[i].lru.value += (0xFFFFFFFF - min_lru_val);
    }
  }
}
void accessMemory(address addr, word *data, WriteEnable we)
{
  /* Declare variables here */
  unsigned int tagBits, //Matching Tags
      indexBits,        //Which set
      offsetBits,       //Where in block.
      tag, index, offset;
  bool hit = false;

  TransferUnit transfer_unit = 0;

  /* handle the case of no cache at all - leave this in */

  if (assoc == 0)
  {
    accessDRAM(addr, (byte *)data, WORD_SIZE, we);
    return;
  }
  ////////////////////////////////
  transfer_unit = uint_log2(block_size);
  indexBits = uint_log2(set_count);
  offsetBits = uint_log2(block_size);
  tagBits = 32 - index - offset;

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

  if (hit) //IF Hit = True
  {
    if (we == READ)
    {
      memcpy(data, cache[index].block[accessedBlock].data + offset, 4);
    }
    if (we == WRITE)
    {
      memcpy(cache[index].block[accessedBlock].data + offset, data, 4);

      if (memory_sync_policy == WRITE_THROUGH)
      {
        accessDRAM(addr, cache[index].block[addr].data, WORD_SIZE, WRITE);
      }
    }
    else //A Miss
    {
    }
    if (policy == LRU)
    {
      accessedBlock = lruBlock(index);
      highlight_block(index, accessedBlock);
      highlight_offset(index, accessedBlock, offset, MISS);
      if (memory_sync_policy == WRITE_BACK && cache[index].block[accessedBlock].dirty)
      {
        int addr2 = (cache[index].block[accessedBlock].tag << (indexBits + offsetBits)) + (index << offsetBits);
        accessDRAM(addr2, cache[index].block[accessedBlock].data, transfer_unit, WRITE);
        accessDRAM(addr, cache[index].block[accessedBlock].data, transfer_unit, READ);
      }
    }
    else if (policy == LFU)
    {
      accessedBlock = lfuBlock(index);
      highlight_block(index, accessedBlock);
      highlight_offset(index, accessedBlock, offset, MISS);
      cache[index].block[accessedBlock].accessCount = 0;
      if (memory_sync_policy == WRITE_BACK)
        cache[index].block[accessedBlock].dirty = VIRGIN;
    }
  }
  else
  { // RANDOM
  }
}

if (we == READ)
{
  //Return Info to cpu.
}
else if (we == WRITE)
{
  //if we == WRITE, then data used to
  //update Cache / DRAM
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

/* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
accessDRAM(addr, (byte *)data, WORD_SIZE, we);
}
