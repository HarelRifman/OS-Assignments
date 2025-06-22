# OS-Assignments

Operating Systems course assignments implementing core OS concepts including process management, file systems, scheduling algorithms, and synchronization.

## Repository Structure

```
OS-Assignments/
├── Exercise_1/          # Chess Game Simulation & File Synchronization
├── Exercise_2/          # Process Management & File Processing
├── Exercise_3/          # CPU Scheduling Algorithms
└── Exercise_4/          # Producer-Consumer with Bounded Buffer
```

## Assignments

### Exercise 1: Chess Simulation & File Synchronization
- **Chess Simulator** (`chess_sim.py`): Simulates chess games from PGN files
- **File Synchronization** (`file_sync.c`): File synchronization mechanisms
- **PGN Processing**: Parse and split large chess game files

### Exercise 2: Process Management
**Part 1**: Gladiator tournament system with process creation and IPC
**Part 2**: Concurrent file processing system
**Part 3**: File backup utility

### Exercise 3: CPU Scheduling
Implementation of multiple scheduling algorithms:
- First Come First Served (FCFS)
- Shortest Job First (SJF)
- Round Robin (RR)
- Priority Scheduling

### Exercise 4: Producer-Consumer Problem
Thread-safe bounded buffer implementation with:
- Multiple producers/consumers
- Semaphores and mutexes
- Deadlock prevention

## Build Instructions

```bash
# Exercise 1
gcc -o file_sync file_sync.c

# Exercise 2
cd Exercise_2/prt_1
gcc -o gladiator gladiator.c
gcc -o tournament tournament.c

# Exercise 3
gcc -o ex3 ex3.c CPU-Scheduler.c Focus-Mode.c

# Exercise 4
make
```

## Key Concepts

- Process and thread management
- File I/O and synchronization
- CPU scheduling algorithms
- Inter-process communication
- Concurrent programming
- Signaling and alarm control

**Language**: C, Python, Shell Script  
**Platform**: UNIX/Linux
