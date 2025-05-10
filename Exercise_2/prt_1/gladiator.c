/**
 * gladiator.c - Simulates a gladiator's fights in the tournament
 * 
 * This program reads a gladiator's stats from a file, simulates fights with opponents,
 * logs the battle progress, and exits when the gladiator's health reaches zero or below.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_FILENAME_LEN 50

// Function to get the attack power of an opponent from their file
int get_opponent_attack(int opponent_number) {
    char filename[MAX_FILENAME_LEN];
    sprintf(filename, "G%d.txt", opponent_number);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open opponent file %s\n", filename);
        return 0;
    }
    
    int health, attack;
    // Make sure we're reading correctly with proper format
    if (fscanf(file, "%d, %d", &health, &attack) != 2) {
        fprintf(stderr, "Error: Failed to read opponent stats correctly from %s\n", filename);
        fclose(file);
        return 0;
    }
    
    fclose(file);
    
    return attack;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <gladiator_file_prefix>\n", argv[0]);
        return 1;
    }
    
    // Extract the gladiator number from the file prefix (e.g., "G1" -> 1)
    int gladiator_number = atoi(&argv[1][1]);
    
    // Construct file names
    char stats_filename[MAX_FILENAME_LEN];
    char log_filename[MAX_FILENAME_LEN];
    sprintf(stats_filename, "%s.txt", argv[1]);
    sprintf(log_filename, "%s_log.txt", argv[1]);
    
    // Open the stats file
    FILE *stats_file = fopen(stats_filename, "r");
    if (stats_file == NULL) {
        fprintf(stderr, "Error: Could not open stats file %s\n", stats_filename);
        return 1;
    }
    
    // Read gladiator stats
    int health, attack, opponents[3];
    fscanf(stats_file, "%d, %d, %d, %d, %d", &health, &attack, 
           &opponents[0], &opponents[1], &opponents[2]);
    fclose(stats_file);
    
    // Open the log file
    FILE *log_file = fopen(log_filename, "w");
    if (log_file == NULL) {
        fprintf(stderr, "Error: Could not create log file %s\n", log_filename);
        return 1;
    }
    
    // Log gladiator process start
    fprintf(log_file, "Gladiator process started. %d: \n", getpid());
    
    // Fight until health is not positive
    int opponent_index = 0;
    int battle_count = 0;
    
    while (health > 0) {
        // Get the current opponent
        int current_opponent = opponents[opponent_index];
        
        // Get opponent's attack power
        int opponent_attack = get_opponent_attack(current_opponent);
        
        // Log the fight
        fprintf(log_file, "Facing opponent %d... Taking %d damage\n", 
                current_opponent, opponent_attack);
        
        // Deduct health based on opponent's attack
        health -= opponent_attack;
        battle_count++;
        
        // Log the result
        if (health > 0) {
            fprintf(log_file, "Are you not entertained? Remaining health: %d\n", health);
        } else {
            fprintf(log_file, "The gladiator has fallen... Final health: %d\n", health);
            break;
        }
        
        // Move to the next opponent (cycle through the 3 opponents)
        opponent_index = (opponent_index + 1) % 3;
    }
    
    // Close the log file
    fclose(log_file);
    
    // Exit with the battle count as status
    // This will help the tournament determine who fought the longest
    return battle_count;
}