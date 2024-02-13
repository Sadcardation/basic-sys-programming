Name: Zedian Shao

NetID: zs100

### Overview of Implementation

The integration of both lock-based and thread-local storage strategies for memory allocation is managed through function pointers and conditional logic. This setup provides a flexible framework for selecting the synchronization strategy at runtime based on the application's threading model and specific memory allocation patterns.

- **Global and Thread-Local Variables:** The file defines a global free memory block list (`global_free_mem_block_list`) for the lock-based version and a thread-local free memory block list (`ts_local_free_mem_blocks`) for the thread-local storage version. This ensures that the lock-based version has a shared state across threads, while the thread-local version maintains separate states for each thread.
- **Function Pointer for Memory Block List Selection:** The `get_mem_block_list` function pointer is used to dynamically select between the global and thread-local memory block list. This decision is based on runtime conditions, specifically whether thread-local storage is being used.
- **Dynamic Selection Functions:** Two functions, `get_global_mem_block_list` and `get_thread_local_mem_block_list`, return pointers to the global and thread-local memory block lists, respectively. These functions encapsulate the logic for accessing the appropriate memory block list based on the execution context.
- **Setting the Memory Block List:** The `set_mem_block_list` function allows for dynamic switching between the lock-based and thread-local versions by changing the `get_mem_block_list` function pointer. The `use_thread_local` parameter determines which memory block list to use: if `use_thread_local` is true, the thread-local list is used; otherwise, the global list is employed.

#### Lock-Based Synchronization

The first version employs lock-based synchronization to ensure thread safety. It uses a global mutex (`pthread_mutex_t lock`) to protect access to the global free memory block list (`global_free_mem_block_list`). Before a thread modifies the list, it locks the mutex, effectively blocking other threads from making simultaneous changes. This method prevents race conditions but could lead to contention, where multiple threads wait to acquire the lock, potentially degrading performance.

- **`ts_malloc_lock(size_t size)`:** Locks the mutex before searching for a suitable free block or allocating a new one. It ensures that only one thread can modify the memory structure at a time.
- **`ts_free_lock(void *ptr)`:** Also locks the mutex before inserting the freed block back into the global list, ensuring thread-safe deallocation.

The critical sections identified include the process of finding a free memory block that fits the requested size (`find_free_block`), allocating a new memory block if none is available (`new_mem_block`), and inserting or deleting memory blocks from the global free memory block list. This strategy uses a mutex (`pthread_mutex_t lock`) to protect the critical sections. The lock is acquired before entering a critical section and released after leaving it. This ensures that only one thread can modify the global free memory block list at any given time, preventing race conditions.

#### Non-Lock Approach (Thread-Local Storage)

The second version avoids lock-based synchronization by using thread-local storage, significantly reducing contention and improving performance in multi-threaded applications. Each thread maintains its local free memory block list (`ts_local_free_mem_blocks`), managed without locks since no other thread can access this private list.

- **`ts_malloc_nolock(size_t size)`:** Allocates memory from the thread's local list without requiring locks. If the local list does not have a suitable block, it falls back to allocating a new block without affecting other threads' lists.
- **`ts_free_nolock(void *ptr)`:** Inserts the freed block into the thread's local list. This operation is safe without locks as it pertains only to the thread's local state.

The use of thread-local storage inherently avoids critical sections in the context of global data, as each thread operates on its own private data. However, threads still need to manage their own free list correctly to avoid issues within their execution context. This strategy utilizes thread-local storage (`__thread mem_block *ts_local_free_mem_blocks`) to keep separate lists of free memory blocks for each thread. This approach eliminates the need for mutex locks, as there's no shared data between threads that could lead to race conditions.

### Results of Experiments

One hundred separated experiments are run for each version of implemented malloc and free.

- T: Execution Time (seconds)
- D: Data Segment Size (KB)

|                                 | T (min)  | T (max)  | T (avg)  | D (min) | D (max) | D(avg) |
| :-----------------------------: | :------: | :------: | :------: | :-----: | :-----: | :----: |
| Lock-Based Synchronization (V1) | 0.114254 | 0.912498 | 0.344654 | 42,151  | 43,714  | 42,670 |
|    Thread-Local Storage(V2)     | 0.090971 | 0.322048 | 0.160505 | 42,158  | 43,977  | 42,698 |

### Analysis and Discussion

The experimental results provide a comparative analysis of the lock-based synchronization (V1) and thread-local storage (V2) implementations of thread-safe `malloc` and `free` functions in terms of execution time and data segment size.

- **Execution Time:** V2 (Thread-Local Storage) shows superior performance with both minimum and maximum execution times being lower than V1 (Lock-Based Synchronization). The average execution time of V2 is nearly half that of V1, highlighting the efficiency of using thread-local storage over lock-based synchronization in reducing execution overhead. The broader range of execution times (from 0.114254 to 0.912498 seconds) suggests that lock contention significantly impacts performance, especially under high concurrency. The locks serialize access to the memory management functions, causing threads to wait, which increases execution times, while the use of thread-local storage minimizes contention and waiting times since each thread operates on its own independent set of data, leading to faster execution on average.
- **Data Segment Size:** The data segment sizes for both versions are relatively close, with V2 showing a slightly higher maximum data segment size but nearly identical average sizes. This indicates that both versions manage memory with similar efficiency, with no significant difference in memory usage. The data segment sizes between V1 and V2 are comparable, indicating that both methods are similarly efficient in terms of memory usage. This similarity suggests that the main difference lies in how each method handles concurrency rather than memory usage efficiency.

For tradeoff related considerations, V1 sacrifices execution speed for thread safety by introducing locks that serialize access to memory allocation and deallocation, leading to higher execution times. V2, however, achieves thread safety with less overhead by using thread-local storage, allowing for more concurrent operations without the need for locks, resulting in significantly improved execution times. However, implementing V2 can be more complex but offers better performance for memory-intensive applications with high concurrency. V1 is simpler to implement but can become a performance bottleneck in multi-threaded applications. The choice between lock-based synchronization and thread-local storage for implementing thread-safe memory allocation functions involves a tradeoff between ease of implementation and performance. Thread-local storage offers significant performance advantages in terms of execution time without major sacrifices in memory efficiency, making it a preferable choice for high-concurrency applications.