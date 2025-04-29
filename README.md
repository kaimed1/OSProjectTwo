# Operating Systems Project 2  
**CS3113 – Introduction to Operating Systems – Spring 2025**

## Project Description  
This project is an extension of Project One and further simulates core concepts of process management in an operating system. Enhancements in this phase include preemptive multitasking through timeouts, context switching, and I/O handling via an I/O waiting queue. The simulation reflects real-world CPU scheduling and job coordination strategies.

## Enhancements Implemented  
1. **CPU Time Allocation and Timeouts**  
   - Each process is allotted a limited number of CPU cycles (`CPUAllocated`) before it is interrupted.  
   - A timeout interrupt causes the process to be moved to the back of the ready queue.  

2. **Context Switching**  
   - A fixed `contextSwitchTime` is added to the global CPU clock during every process switch.  
   - The context switch cost is applied even if the ready queue is temporarily empty.

3. **I/O Interrupts and Waiting Queue**  
   - Print operations (`instruction code 2`) trigger I/O interrupts and move processes to an I/O waiting queue.  
   - The global clock determines when a process is eligible to return from the I/O queue to the ready queue.

4. **Process Lifecycle Logging**  
   - The simulation logs transitions between the NEW, READY, RUNNING, I/O WAITING, and TERMINATED states.  
   - Each process logs the time it entered the RUNNING state and the time it was TERMINATED.  

5. **Total CPU Time Tracking**  
   - The simulation outputs the total CPU time used at the end of execution.

## Source Files  
- `CS3113_Project2.cpp`: Main C++ implementation of the process scheduling simulation.  
- `CS3113-Spring-2025-ProjectTwo.pdf`: Official project specification outlining simulation requirements and input structure.  
- `README.md`: Documentation and usage instructions.

## Input Format  
The program reads from standard input (via redirection). The expected format includes:
1. Maximum size of main memory (in integers)  
2. CPU time allocation limit (`CPUAllocated`)  
3. Context switch time (`contextSwitchTime`)  
4. Number of processes  
5. For each process:
   - `processID maxMemoryNeeded numInstructions`
   - Followed by encoded instructions using the formats below

## Supported Instructions  
| Opcode | Instruction | Format                       | Description                           |
|--------|-------------|------------------------------|---------------------------------------|
| 1      | Compute     | `1 <iterations> <cycles>`    | Simulates computation                 |
| 2      | Print       | `2 <cycles>`                 | Triggers I/O interrupt                |
| 3      | Store       | `3 <value> <address>`        | Stores value in memory                |
| 4      | Load        | `4 <address>`                | Loads value into register             |

## Compilation and Execution  
To compile and run the program:
```bash
g++ CS3113_Project2.cpp -o os_project2
./os_project2 < input.txt
