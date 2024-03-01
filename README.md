# Operating Systems Assignments

## [Assignment 1](https://github.com/pavlosdais/Operating-Systems/tree/main/assignment1)
A simulation of a voting procedure where different data structures are used in order to maintain an order regarding the voters, and be able to perform (almost all) queries in constant time.

The following data structures are used:

* [Linear Hash Table](https://en.wikipedia.org/wiki/Linear_hashing)
* 2D [List](https://en.wikipedia.org/wiki/List_(abstract_data_type))
  
to store all the info regarding the voting process. 

The queries include adding voters and printing information about the voting process.

## [Assignment 2](https://github.com/pavlosdais/Operating-Systems/tree/main/assignment2)
Implemention of 2 sorting algorithms {quicksort & heapsort}, that work as separate processes from the rest of the program. The algorithms are used to sort a file containing records using the following hierarchy:
1. [Coordinator](https://github.com/pavlosdais/Operating-Systems/blob/main/assignment2/src/coordinator.c)
   
   The coordinator is responsible for creating the splitters. After the splitters are finished, it receives the records then finally merges them to create the final, sorted file.
3. [Splitter](https://github.com/pavlosdais/Operating-Systems/blob/main/assignment2/src/splitter.c)

   The splitter is responsible for creating the sorters. After the sorters are finished, it receives the records then merges them. It then passes the records up to the coordinator.
5. [Sorter (quicksort & heapsort)](https://github.com/pavlosdais/Operating-Systems/blob/main/assignment2/src/quick_sort.c)

   The sorter is responsible for sorting a particular part of a file then passing the records up to the splitter.

The processes communicate via pipes, by passing the different flags and records needed for the sorting.

## [Assignment 3](https://github.com/pavlosdais/Operating-Systems/tree/main/assignment3)
A simulation of the [readers - writers problem](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem) using records from a banking customers file. The "unique" and challenging part of the assignment though, is that we can allow multiple of writers/readers to read/write at the file, at the same time. This is done in a fair & starve-free way, by enforcing a First In First Out (FIFO) system for all processes. In short, the processes follow the following protocol:

1. [Reader](https://github.com/pavlosdais/Operating-Systems/tree/main/assignment3/src/Reader)

  The reader **before** entering to read a range of records, checks for potential writers that block its range. It saves the number of such writers, and if it is more than one it waits. Otherwise it reads.
  
  The reader **after** exiting checks for potential writers that it blocked (because it was reading). For every such writer, it decrements the number of readers its blocked by. If its 0, it wakes up.
  
2. [Writer](https://github.com/pavlosdais/Operating-Systems/tree/main/assignment3/src/Writer)

  The writer **before** entering to write a record at a specific index, checks for potential processes (readers/writers) that block it. It saves the number of such processes, and if it is more than one it waits. Otherwise, it writes.
  
  The writer **after** exiting checks for potential processes that it blocked (because it was writing). For every such process, it decrements the number of writers its blocked by. If its 0, it wakes up.

  A logging system has also been implemented to ensure the correctness of the synchronization.

## [Assignment 4](https://github.com/pavlosdais/Operating-Systems/tree/main/assignment4)
A UNIX command that allows us to (efficiently)
1. [Compare](https://github.com/pavlosdais/Operating-Systems/blob/main/assignment4/src/compare_files.c) **or**
2. [Merge](https://github.com/pavlosdais/Operating-Systems/blob/main/assignment4/src/merge_files.c)

two different directories.

The command works with:
- **Regular files**
- **Directories**
- **Soft links**
- **Hard links**

The assignment was completed in collaboration with [Aristarchos Kaloutas](https://github.com/aristarhoskal).

## About
The programming projects were created for the [Operating Systems - K22](https://www.alexdelis.eu/k22/) course under prof. [Alex Delis](https://www.alexdelis.eu/). Each assignment got a score of 100/100.
