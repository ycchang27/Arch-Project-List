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
  This function handles random replacement policy. 
*/
void handleRandom() {
  // code here
}

/*
  This function handles LRU (least recently used) replacement policy.
*/
void handleLRU() {
  // code here



}

/*
  This function handles LFU (least frequently used) replacement policy.
*/
void handleLFU() {
  // code here
}

/*
  This function handles write back memory sync policy.
*/
void handleWriteBack() {
  // code here
}

/*
  This function handles write back memory sync policy.
*/
void handleWriteThrough() {
  // code here
}

void switchMode(){
  TranferUnit block_mode;
  switch(block_size)
  {
  	case 1:
  		block_mode = BYTE_SIZE;
  		break;
  	case 2:
  		block_mode = HALF_WORD_SIZE;
  		break;
  	case 4:
  		block_mode = WORD_SIZE;
  		break;
  	case 8:
   		block_mode = DOUBLEWORD_SIZE;
    	break;
  	case 16:
   		block_mode = QUADWORD_SIZE;
    	break;
  	case 32:
    	block_mode = OCTWORD_SIZE;
    	break;
    default:
   		append_log("Invalid transfer block_size for accessMemory\n");
  }
	mode = block_mode;

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


    
  unsigned int vaddr = addr >> 12;
  unsigned int set_number = vaddr % set_count;
  unsigned int block_number = vaddr % assoc;
  unsigned int transfer_size;

  /* Determine number of bytes involved in memory access */
  switch(mode)
  {
  case BYTE_SIZE:
    transfer_size = 1;
    break;
  case HALF_WORD_SIZE:
    transfer_size = 2;
    break;
  case WORD_SIZE:
    transfer_size = 4;
    break;
  case DOUBLEWORD_SIZE:
    transfer_size = 8;
    break;
  case QUADWORD_SIZE:
    transfer_size = 16;
    break;
  case OCTWORD_SIZE:
    transfer_size = 32;
    break;
  default:
    append_log("Invalid transfer mode for accessMemory\n");
  }

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }


  /* Determine whether this function reads or writes */
  switch(we)
  {
  	case READ:
  		if(cache[set_number].block[block_number] != NULL)	
  		{
  			//Transfer block data to data
  			memcpy(data,cache[set_number].block[block_number].data,transfer_size);	
  		}
  		else
  		{
    		TransferUnit temp = mode;
    		switchMode();
    		accessDRAM(addr,data,we);
    		mode = temp;

  		}
  		break;
  	case WRITE:
  		if(cache[set_number].block[block_number] == NULL)
  		{
  			memcpy(cache[set_number].block[block_number].data,data,transfer_size);
  			if(memory_sync_policy == WRITE_THROUGH)
  			{
  				TransferUnit temp = mode;
    			switchMode();
    			accessDRAM(addr,data,we);
    			mode = temp;
				}
				else
				{
					cache[set_number].block[block_number].dirty = DIRTY;
  		  }
  		}
  		else
  		{
  			if(cache[set_number].block[block_number] != NULL)
  			{
  				int i;
  				for(i = block_number+1;i < assoc;i++)
  				{
  					if(cache[set_number].block[i] == NULL)
  					{
  						break;
  					}
  				}
  				if(i != assoc)
  				{
  					memcpy(cache[set_number].block[i].data,data,transfer_size);
  					if(memory_sync_policy == WRITE_THROUGH)
  					{
  						TransferUnit temp = mode;
    					switchMode();
    					accessDRAM(addr,data,we);
    					mode = temp;
						}
						else
						{
							cache[set_number].block[i].dirty = DIRTY;
  		  		}
  				}
  				else
  				{
  					cacheBlock tempBlock;
  					switch(policy)
  					{
  						case LRU:
  							tempBlock = handleLRU();
  							break;
  						case LFU:
  							tempBlock = handleLFU();
  							break;
  						case RANDOM:
  							tempBlock = handleRandom();
  							break;
  						default:
  							append_log("Invalid flag for accessMemory\n");
  					}
  					if(memory_sync_policy == WRITE_BACK && tempBlock.dirty == DIRTY)
						{
							TransferUnit temp = mode;
    					switchMode();
    					accessDRAM(addr,tempBlock,we);
    					mode = temp;  					}
  				}
  			}
  		}
  		break;
  	default:
  		append_log("Invalid flag for accessMemory\n");

  }

  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
  accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}