#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

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
  This function handles random replacement policy. Returns a random block index.
*/
int handleRandom() {
  return randomint(assoc);
}

/*
  This function handles LRU (least recently used) replacement policy. Returns a block index that has the highest LRU value.
*/
int handleLRU(unsigned index) {
  int highest = 0;
  for(int i = 1;i < assoc;i++)
  {
    if(cache[index].block[i].lru.value > cache[index].block[highest].lru.value)
      highest = i;
  }
  return highest;
}

/*
  This function handles LFU (least frequently used) replacement policy. Returns a block index that has the lowest access count.
*/
int handleLFU(unsigned index) {
  int lowest = 0;
  for(int i = 1 ; i < assoc;i++)
  {
    if(cache[index].block[i].lru.value < cache[index].block[lowest].lru.value)
      lowest = i;
  }
  return lowest;
}


/*Increments all blocks in LRU.value by one, except the one in use*/
void updateLRU(unsigned index,unsigned block){
  for(int i = 0;i<assoc;i++)
  {
    if(block != i)
      cache[index].block[i].lru.value = (cache[index].block[i].lru.value + 1);
  }
}

/*Searches for block index that contains the same tag as the one given.*/
int searchTag(unsigned index,unsigned tag) {
  for(int i = 0;i<assoc;i++)
  {
    if(cache[index].block[i].tag == tag)
    {
      return i;
    }
  }
  return -1;
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
  append_log("Access MEM Call\n");
  /* Declare variables here */
  unsigned offset_bits = uint_log2(block_size);
  unsigned index_bits = uint_log2(set_count);
  unsigned tag_bits = 32 - offset_bits - index_bits;

  /*bit mask except this everytinh will be zero*/
  unsigned offset = addr&((1<<offset_bits)-1);
  /*shifts to the right to ignore offset, masks bits*/
  unsigned index = (addr >> offset_bits) & ((1<<index_bits)-1);
  unsigned tag = addr >> (offset_bits + index_bits) & ((1 << tag_bits)-1);
  unsigned addr_no_offset = addr&~offset;

  /*Transfer out 4 bytes or one word*/
  unsigned int transfer_size = 4;

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }


  /* Determine whether this function reads or writes */
  int block = searchTag(index,tag);
  int hit = 0;



  /*If search fails (Cache miss?)*/
  if (block == -1 || cache[index].block[block].valid == INVALID)
  {
    if(block == -1)
    {
      switch(policy) 
      {
        case LRU:
          block = handleLRU(index);
          break;
        case LFU:
          block = handleLFU(index);
          break;
        case RANDOM:
          block = handleRandom();
          break;
        default:
          printf("Invalid Replacement Policy\n");
      }
    }
    highlight_block(index,block);
    highlight_offset(index,block,offset,MISS);
    accessDRAM(addr_no_offset,cache[index].block[block].data,uint_log2(block_size),READ);
    cache[index].block[block].valid = VALID;
    cache[index].block[block].dirty = VIRGIN;
    cache[index].block[block].tag = tag;
    cache[index].block[block].accessCount = 0;
    hit = -1;
  }
  else if(memory_sync_policy == WRITE_BACK && cache[index].block[block].dirty == DIRTY)
  {
    accessDRAM(addr_no_offset,cache[index].block[block].data,uint_log2(block_size),WRITE);
  }
  if (hit != -1)
  {
    highlight_block(index,block);
    highlight_offset(index,block,offset,HIT);
  }

  cache[index].block[block].lru.value = 0;
  cache[index].block[block].accessCount++;
  updateLRU(index,block);
  if(we == READ)
  {
    memcpy(data,cache[index].block[block].data+offset,transfer_size);
  }
  else 
  {
    memcpy(cache[index].block[block].data+offset,data,transfer_size);
    if(memory_sync_policy == WRITE_THROUGH)
    {
      /*Transfers whohle block content. Transfers full block to memory */
      accessDRAM(addr_no_offset,cache[index].block[block].data,uint_log2(MAX_BLOCK_SIZE),WRITE);
      cache[index].block[block].dirty = VIRGIN;
    }
    else
      cache[index].block[block].dirty = DIRTY;

  }

}
