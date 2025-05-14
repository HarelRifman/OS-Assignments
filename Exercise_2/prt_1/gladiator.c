#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_GLADIATORS 4
#define MAX_FILE_NAME 10

int main() {
    char* gladiator_names[NUM_GLADIATORS] = {"Maximus", "Lucius", "Commodus", "Spartacus"};
    char* gladiator_files[NUM_GLADIATORS] = {"G1", "G2", "G3", "G4"};
    
    pid_t gladiator_pids[NUM_GLADIATORS];
    int eliminated_order[NUM_GLADIATORS];
    int elimination_count = 0;
    
    // Fork and exec each gladiator process
    for (int i = 0; i < NUM_GLADIATORS; i++) {
        gladiator_pids[i] = fork();
        
        if (gladiator_pids[i] == 0) {
            // Child process - execute the gladiator program
            char filename[MAX_FILE_NAME];
            sprintf(filename, "%s", gladiator_files[i]);
            char* args[] = {"./gladiator", filename, NULL};
            execvp(args[0], args);
        }
    }

    int status;
    // Wait for gladiators to finish, recording the order they finish in
    while (elimination_count < NUM_GLADIATORS) {
        pid_t finished_pid = wait(&status);
        
        // Find which gladiator just finished
        for (int i = 0; i < NUM_GLADIATORS; i++) {
            if (gladiator_pids[i] == finished_pid) {
                eliminated_order[elimination_count++] = i;
                break;
            }
        }
    }
    
    // The last gladiator to finish is the winner (last element in eliminated_order)
    int winner_index = eliminated_order[NUM_GLADIATORS - 1];
    
    printf("The gods have spoken, the winner of the tournament is %s!\n", 
           gladiator_names[winner_index]);
    
    return 0;
}   