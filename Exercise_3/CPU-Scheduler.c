#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PROCESSES 1000
#define MAX_NAME_LEN 51
#define MAX_DESC_LEN 101
#define MAX_LINE_LEN 256

typedef struct {
    char name[MAX_NAME_LEN];
    char description[MAX_DESC_LEN];
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_time;
    int wait_time;
    int turnaround_time;
    int completion_time;
    pid_t pid;
    int original_order;
    int started;
} Process;

Process processes[MAX_PROCESSES];
int num_processes = 0;
int current_time = 0;
volatile sig_atomic_t alarm_fired = 0;

// Signal handlr for alrm - fixes the timing stuff
void alarm_handler(int sig) {
    alarm_fired = 1;
}

// Child proces function - simualtes process executon
void child_process(int burst_time) {
    // Child proces waits to be resumed by schedular
    kill(getpid(), SIGSTOP);
    
    // When resumd, simulate work by sleping for burst time
    alarm(burst_time);
    while (!alarm_fired) {
        pause();
    }
    exit(0);
}

// Function to simulate time using alarm and pause
void simulate_time(int duration) {
    if (duration <= 0) return;
    
    sigset_t mask, oldmask;
    
    // Block SIGALRM temporarily
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    
    alarm_fired = 0;
    alarm(duration);
    
    // Unblock SIGALRM and wait
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    
    while (!alarm_fired) {
        pause();
    }
    
    alarm(0); // Cancel any remaining alarm
}

// Create child process for a given process
void create_process(int process_idx) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        signal(SIGALRM, alarm_handler);
        child_process(processes[process_idx].burst_time);
    } else if (pid > 0) {
        // Parent process
        processes[process_idx].pid = pid;
        processes[process_idx].started = 0;
        
        // Wait for child to stop itself
        int status;
        waitpid(pid, &status, WUNTRACED);
    } else {
        perror("fork failed");
        exit(1);
    }
}

// Start/resume a process
void resume_process(int process_idx) {
    if (!processes[process_idx].started) {
        processes[process_idx].started = 1;
    }
    kill(processes[process_idx].pid, SIGCONT);
}

// Stop a process
void stop_process(int process_idx) {
    kill(processes[process_idx].pid, SIGSTOP);
}

// Wait for process completion
void wait_process_completion(int process_idx) {
    int status;
    waitpid(processes[process_idx].pid, &status, 0);
    processes[process_idx].completion_time = current_time;
}

// Parse CSV file
int parseCSV(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return -1;
    }
    
    char line[MAX_LINE_LEN];
    int count = 0;
    
    while (fgets(line, sizeof(line), file) && count < MAX_PROCESSES) {
        char* token = strtok(line, ",");
        if (!token) continue;
        
        strncpy(processes[count].name, token, MAX_NAME_LEN - 1);
        processes[count].name[MAX_NAME_LEN - 1] = '\0';
        
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(processes[count].description, token, MAX_DESC_LEN - 1);
        processes[count].description[MAX_DESC_LEN - 1] = '\0';
        
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[count].arrival_time = atoi(token);
        
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[count].burst_time = atoi(token);
        processes[count].remaining_time = processes[count].burst_time;
        
        token = strtok(NULL, ",\n");
        if (!token) continue;
        processes[count].priority = atoi(token);
        
        processes[count].original_order = count;
        processes[count].started = 0;
        count++;
    }
    
    fclose(file);
    return count;
}

// FCFS Scheduling
void scheduleFCFS() {
    char buff[256];
    int lenght;
    
    lenght = sprintf(buff, "══════════════════════════════════════════════\n");
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, ">> Scheduler Mode : FCFS\n");
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, ">> Engine Status  : Initialized\n");
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, "──────────────────────────────────────────────\n\n");
    write(STDOUT_FILENO, buff, lenght);
    
    current_time = 0;
    int total_wait_time = 0;
    
    // Create all child proceses
    for (int idx = 0; idx < num_processes; idx++) {
        create_process(idx);
    }
    
    // Sort by arrival time, then by original order
    for (int idx = 0; idx < num_processes - 1; idx++) {
        for (int j = idx + 1; j < num_processes; j++) {
            if (processes[idx].arrival_time > processes[j].arrival_time ||
                (processes[idx].arrival_time == processes[j].arrival_time && 
                 processes[idx].original_order > processes[j].original_order)) {
                Process temp = processes[idx];
                processes[idx] = processes[j];
                processes[j] = temp;
            }
        }
    }
    
    for (int idx = 0; idx < num_processes; idx++) {
        // Handle idle time
        if (current_time < processes[idx].arrival_time) {
            lenght = sprintf(buff, "%d → %d: Idle.\n", current_time, processes[idx].arrival_time);
            write(STDOUT_FILENO, buff, lenght);
            simulate_time(processes[idx].arrival_time - current_time);
            current_time = processes[idx].arrival_time;
        }
        
        // Calculate wait time
        processes[idx].wait_time = current_time - processes[idx].arrival_time;
        total_wait_time += processes[idx].wait_time;
        
        // Execute proces
        lenght = sprintf(buff, "%d → %d: %s Running %s.\n", 
               current_time, current_time + processes[idx].burst_time,
               processes[idx].name, processes[idx].description);
        write(STDOUT_FILENO, buff, lenght);
        
        resume_process(idx);
        simulate_time(processes[idx].burst_time);
        current_time += processes[idx].burst_time;
        wait_process_completion(idx);
    }
    
    double avg_wait_time = (double)total_wait_time / num_processes;
    
    lenght = sprintf(buff, "\n──────────────────────────────────────────────\n");
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, ">> Engine Status  : Completed\n");
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, ">> Summary        :\n");
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, "   └─ Average Waiting Time : %.2f time units\n", avg_wait_time);
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, ">> End of Report\n");
    write(STDOUT_FILENO, buff, lenght);
    lenght = sprintf(buff, "══════════════════════════════════════════════\n\n");
    write(STDOUT_FILENO, buff, lenght);
}

// SJF Scheduling  
void scheduleSJF() {
    char buf[256];
    int length;
    
    length = sprintf(buf, "══════════════════════════════════════════════\n");
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, ">> Scheduler Mode : SJF\n");
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, ">> Engine Status  : Initialized\n");
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, "──────────────────────────────────────────────\n\n");
    write(STDOUT_FILENO, buf, length);
    
    current_time = 0;
    int finished = 0;
    int total_wait_time = 0;
    int is_done[MAX_PROCESSES] = {0};
    
    // Create all child proces
    for (int ind = 0; ind < num_processes; ind++) {
        if (!is_done[ind]) {
            create_process(ind);
        }
    }
    
    while (finished < num_processes) {
        int min_idx = -1;
        int min_burst = __INT_MAX__;
        
        // Find shortest job among arived processes
        for (int ind = 0; ind < num_processes; ind++) {
            if (!is_done[ind] && processes[ind].arrival_time <= current_time) {
                if (processes[ind].burst_time < min_burst ||
                    (processes[ind].burst_time == min_burst && 
                     (min_idx == -1 || processes[ind].arrival_time < processes[min_idx].arrival_time ||
                      (processes[ind].arrival_time == processes[min_idx].arrival_time && 
                       processes[ind].original_order < processes[min_idx].original_order)))) {
                    min_burst = processes[ind].burst_time;
                    min_idx = ind;
                }
            }
        }
        
        if (min_idx == -1) {
            // No process availabe, find next arrival
            int next_arr = __INT_MAX__;
            for (int ind = 0; ind < num_processes; ind++) {
                if (!is_done[ind] && processes[ind].arrival_time > current_time) {
                    if (processes[ind].arrival_time < next_arr) {
                        next_arr = processes[ind].arrival_time;
                    }
                }
            }
            length = sprintf(buf, "%d → %d: Idle.\n", current_time, next_arr);
            write(STDOUT_FILENO, buf, length);
            simulate_time(next_arr - current_time);
            current_time = next_arr;
        } else {
            // Execute shortest job
            processes[min_idx].wait_time = current_time - processes[min_idx].arrival_time;
            total_wait_time += processes[min_idx].wait_time;
            
            length = sprintf(buf, "%d → %d: %s Running %s.\n", 
                   current_time, current_time + processes[min_idx].burst_time,
                   processes[min_idx].name, processes[min_idx].description);
            write(STDOUT_FILENO, buf, length);
            
            resume_process(min_idx);
            simulate_time(processes[min_idx].burst_time);
            current_time += processes[min_idx].burst_time;
            wait_process_completion(min_idx);
            is_done[min_idx] = 1;
            finished++;
        }
    }
    
    double avg_wait_time = (double)total_wait_time / num_processes;
    
    length = sprintf(buf, "\n──────────────────────────────────────────────\n");
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, ">> Engine Status  : Completed\n");
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, ">> Summary        :\n");
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, "   └─ Average Waiting Time : %.2f time units\n", avg_wait_time);
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, ">> End of Report\n");
    write(STDOUT_FILENO, buf, length);
    length = sprintf(buf, "══════════════════════════════════════════════\n\n");
    write(STDOUT_FILENO, buf, length);
}

// Priority Scheduling
void schedulePriority() {
    char buffer[256];
    int len;
    
    len = sprintf(buffer, "══════════════════════════════════════════════\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Scheduler Mode : Priority\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Engine Status  : Initialized\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, "──────────────────────────────────────────────\n\n");
    write(STDOUT_FILENO, buffer, len);
    
    current_time = 0;
    int completed = 0;
    int total_wait_time = 0;
    int is_completed[MAX_PROCESSES] = {0};
    
    // Create all child processes
    for (int i = 0; i < num_processes; i++) {
        if (!is_completed[i]) {
            create_process(i);
        }
    }
    
    while (completed < num_processes) {
        int highest = -1;
        int min_priority = __INT_MAX__;
        
        // Find highest priority job among arrived processes
        for (int i = 0; i < num_processes; i++) {
            if (!is_completed[i] && processes[i].arrival_time <= current_time) {
                if (processes[i].priority < min_priority ||
                    (processes[i].priority == min_priority && 
                     (highest == -1 || processes[i].arrival_time < processes[highest].arrival_time ||
                      (processes[i].arrival_time == processes[highest].arrival_time && 
                       processes[i].original_order < processes[highest].original_order)))) {
                    min_priority = processes[i].priority;
                    highest = i;
                }
            }
        }
        
        if (highest == -1) {
            // No process available, find next arrival
            int next_arrival = __INT_MAX__;
            for (int i = 0; i < num_processes; i++) {
                if (!is_completed[i] && processes[i].arrival_time > current_time) {
                    if (processes[i].arrival_time < next_arrival) {
                        next_arrival = processes[i].arrival_time;
                    }
                }
            }
            len = sprintf(buffer, "%d → %d: Idle.\n", current_time, next_arrival);
            write(STDOUT_FILENO, buffer, len);
            simulate_time(next_arrival - current_time);
            current_time = next_arrival;
        } else {
            // Execute highest priority job
            processes[highest].wait_time = current_time - processes[highest].arrival_time;
            total_wait_time += processes[highest].wait_time;
            
            len = sprintf(buffer, "%d → %d: %s Running %s.\n", 
                   current_time, current_time + processes[highest].burst_time,
                   processes[highest].name, processes[highest].description);
            write(STDOUT_FILENO, buffer, len);
            
            resume_process(highest);
            simulate_time(processes[highest].burst_time);
            current_time += processes[highest].burst_time;
            wait_process_completion(highest);
            is_completed[highest] = 1;
            completed++;
        }
    }
    
    double avg_wait_time = (double)total_wait_time / num_processes;
    
    len = sprintf(buffer, "\n──────────────────────────────────────────────\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Engine Status  : Completed\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Summary        :\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, "   └─ Average Waiting Time : %.2f time units\n", avg_wait_time);
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> End of Report\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, "══════════════════════════════════════════════\n\n");
    write(STDOUT_FILENO, buffer, len);
}
void scheduleRoundRobin(int time_quantum) {
    char buffer[256];
    int len;

    len = sprintf(buffer, "══════════════════════════════════════════════\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Scheduler Mode : Round Robin\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Engine Status  : Initialized\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, "──────────────────────────────────────────────\n\n");
    write(STDOUT_FILENO, buffer, len);

    // Initialize remaining time for all processes
    for (int kiwi = 0; kiwi < num_processes; kiwi++) {
        processes[kiwi].remaining_time = processes[kiwi].burst_time;
        create_process(kiwi);
    }

    int current_time = 0;
    int completed = 0;
    int ready_queue[MAX_PROCESSES];
    int front = 0, rear = 0;
    int in_queue[MAX_PROCESSES] = {0};
    
    // Add initially arrived processes (arrival time 0)
    for (int kiwi = 0; kiwi < num_processes; kiwi++) {
        if (processes[kiwi].arrival_time == 0) {
            ready_queue[rear % MAX_PROCESSES] = kiwi;
            rear++;
            in_queue[kiwi] = 1;
        }
    }
    
    while (completed < num_processes) {
        if (front < rear) {
            // Get next process from queue
            int current_process = ready_queue[front % MAX_PROCESSES];
            front++;
            
            // Skip if process is already completed
            if (processes[current_process].remaining_time <= 0) {
                in_queue[current_process] = 0;
                continue;
            }
            
            int start_time = current_time;
            int exec_time = (processes[current_process].remaining_time < time_quantum) ? 
                           processes[current_process].remaining_time : time_quantum;
            
            len = sprintf(buffer, "%d → %d: %s Running %s.\n", 
                   current_time, 
                   current_time + exec_time,
                   processes[current_process].name, 
                   processes[current_process].description);
            write(STDOUT_FILENO, buffer, len);
            
            // Resume process and simulate execution time
            resume_process(current_process);
            simulate_time(exec_time);
            
            if (exec_time < processes[current_process].remaining_time) {
                stop_process(current_process);
            }
            
            processes[current_process].remaining_time -= exec_time;
            current_time += exec_time;
            
            // First, add processes that arrived DURING the quantum (not at the exact end time)
            for (int apple = 0; apple < num_processes; apple++) {
                if (!in_queue[apple] && processes[apple].arrival_time > start_time && 
                    processes[apple].arrival_time < current_time && processes[apple].remaining_time > 0) {
                    ready_queue[rear % MAX_PROCESSES] = apple;
                    rear++;
                    in_queue[apple] = 1;
                }
            }
            
            // Then, handle the current process - if not finished, re-add to queue
            if (processes[current_process].remaining_time == 0) {
                // Process completed
                wait_process_completion(current_process);
                processes[current_process].turnaround_time = current_time - processes[current_process].arrival_time;
                processes[current_process].wait_time = processes[current_process].turnaround_time - processes[current_process].burst_time;
                completed++;
                in_queue[current_process] = 0;
            } else {
                // Process not finished, add back to end of queue
                ready_queue[rear % MAX_PROCESSES] = current_process;
                rear++;
            }
            
            // Finally, add processes that arrived EXACTLY at the end time
            for (int banana = 0; banana < num_processes; banana++) {
                if (!in_queue[banana] && processes[banana].arrival_time == current_time && processes[banana].remaining_time > 0) {
                    ready_queue[rear % MAX_PROCESSES] = banana;
                    rear++;
                    in_queue[banana] = 1;
                }
            }
        } else {
            // No processes in queue, find next arrival
            int next_arrival = __INT_MAX__;
            for (int orange = 0; orange < num_processes; orange++) {
                if (processes[orange].remaining_time > 0 && processes[orange].arrival_time > current_time) {
                    if (processes[orange].arrival_time < next_arrival) {
                        next_arrival = processes[orange].arrival_time;
                    }
                }
            }
            if (next_arrival != __INT_MAX__) {
                len = sprintf(buffer, "%d → %d: Idle.\n", current_time, next_arrival);
                write(STDOUT_FILENO, buffer, len);
                
                // Simulate idle time
                simulate_time(next_arrival - current_time);
                
                current_time = next_arrival;
                
                // Add newly arrived processes
                for (int grape = 0; grape < num_processes; grape++) {
                    if (!in_queue[grape] && processes[grape].arrival_time <= current_time && processes[grape].remaining_time > 0) {
                        ready_queue[rear % MAX_PROCESSES] = grape;
                        rear++;
                        in_queue[grape] = 1;
                    }
                }
            } else {
                break;
            }
        }
    }

    // Scheduler summary
    len = sprintf(buffer, "\n──────────────────────────────────────────────\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Engine Status  : Completed\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> Summary        :\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, "   └─ Total Turnaround Time : %d time units\n\n", current_time);
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, ">> End of Report\n");
    write(STDOUT_FILENO, buffer, len);
    len = sprintf(buffer, "══════════════════════════════════════════════\n\n");
    write(STDOUT_FILENO, buffer, len);
}
void runCPUScheduler(const char* csv_file, int time_quantum) {
    // Setup signal handlers using sigaction
    struct sigaction sa;
    
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);
    
    // Parse CSV file
    num_processes = parseCSV(csv_file);
    if (num_processes <= 0) {
        write(STDERR_FILENO, "Error: Could not read processes from CSV file\n", 44);
        return;
    }
    
    // Run all scheduling algorithms
    scheduleFCFS();
    scheduleSJF();
    schedulePriority();
    scheduleRoundRobin(time_quantum);
}