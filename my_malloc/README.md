Name: Zedian Shao

NetID: zs100

### Overview of Implementation

#### Data Structure

- `mem_block`: This structure represents a block of memory. It likely includes fields like `size`, `next`, and `prev` for maintaining a linked list of memory blocks.
- `free_mem_blocks`: A global pointer that points to the head of a linked list of free memory blocks.
- `size_heap_mem`: A global variable to track the size of the heap memory used.

#### Allocation Strategies

1. **First Fit (`find_free_block_FF`)**: This strategy searches the list of free blocks and allocates the first block that is large enough to satisfy the request. This approach is generally faster but can lead to memory fragmentation over time.
   - Call **`find_free_block_FF`** which iterates through the linked list of free memory blocks. It looks for the first block that is large enough to satisfy the allocation request (`current->size >= size`). Once a suitable block is found, it updates the list using `list_update` to either split the block if it's significantly larger than needed or to simply remove it from the free list.
2. **Best Fit (`find_free_block_BF`)**: This strategy looks for the smallest free block that fits the request. It's more memory-efficient as it minimizes waste but can be slower due to the need to search the entire list.
   - Call **`find_free_block_BF`** which also iterates through the free memory blocks. It searches for the smallest block that is large enough to fit the request. This involves checking all suitable blocks (`current->size >= size`) and keeping track of the one that fits the request most closely (`best_block`). If a block with the exact size is found, it is immediately selected.

#### Other Key Functions

1.  `list_update(mem_block *block, size_t size)`
	- **Purpose**: Updates the free list when a block is allocated.
	- **Logic**: If the current block is large enough to be split, it creates a new block with the remaining space and inserts it into the free list. Then, it removes the allocated block from the free list.

2. `new_mem_block(size_t size)`
   - **Purpose**: Allocates a new memory block using `sbrk`.
   - **Logic**: Increases the program's data space by the requested size plus the size of the metadata. Handles the error if `sbrk` fails.

3. `merge_free_blocks(mem_block *block)`
   - **Purpose**: Merges adjacent free blocks to reduce fragmentation.
   - **Logic**: It checks if the given block can be merged with its neighboring blocks (either to the left or right) and merges them if possible.

4. `insert_block(mem_block *block)` and `delete_block(mem_block *block)`
   - **Purpose**: To insert a free block into the free list or remove a block from the free list, respectively.
   - **Logic**: Both functions handle the linked list pointers appropriately to maintain the list's integrity. `insert_block` also calls `merge_free_blocks` to try and coalesce adjacent free blocks.

### Results of Performance Experiments

- FF: First Fit, BF: Best Fit

|                         | Execution Time (FF) (s) | Fragmentation (FF) | Execution Time (BF) (s) | Fragmentation (BF) |
| :---------------------: | :---------------------: | :----------------: | :---------------------: | :----------------: |
| Small Range Rand Allocs |          10.63          |       0.073        |          3.27           |       0.027        |
| Large Range Rand Allocs |          43.88          |       0.093        |          49.16          |       0.041        |
|    Equal Size Allocs    |          6.52           |       0.450        |          6.85           |       0.450        |

### Analysis of Results

For Small Range Random Allocations, FF takes significantly longer (10.63s) compared to BF (3.27s) and FF shows higher fragmentation (0.073) than BF (0.027). BF's better performance in both time and fragmentation for small range allocations could be because the smaller allocation sizes make it easier to find an optimal block size, reducing both the search time and the fragmentation with a much shorter free memory blocks list for searching. FF's increased execution time and fragmentation might be due to the quick exhaustion of larger blocks, leaving many small, unusable fragments.

For Large Range Random Allocations, BF is slower (49.16s) than FF (43.88s), but BF still performs better in terms of fragmentation (0.041 vs. 0.093). The increased execution time for BF in this scenario is likely due to the more exhaustive search required to find the best-fit block in a larger range of allocation sizes. FF's faster execution time could be due to its simpler allocation strategy, but this comes at the cost of higher fragmentation. BF's lower fragmentation suggests that it still manages to utilize memory more efficiently, even though it takes longer.

For Equal Size Allocations, both strategies show similar execution times (FF: 6.52s, BF: 6.85s), and both strategies have high fragmentation (0.450). The similar execution times suggest that when allocation sizes are uniform, the advantage of BF in finding the smallest sufficient block is negated. And in this special case, since the memory size for malloc and free are the same, so there is no obvious difference between FF and BF in execution. The high fragmentation for both strategies in this scenario might be a result of the allocation pattern, which might be leading to many equally sized free spaces that are not easily reusable.

It seems that the usage of allocation strategies should be task-specific. In general, we always want to make best use of our limited memory resources, and the trade-off for best fit allocation on time is not that much, in this way best fit allocation might be preferred in most cases. However in some extreme cases like Large Range allocation where we have much larger free blocks for small allocations, first fit does not need to iterate that longer.
