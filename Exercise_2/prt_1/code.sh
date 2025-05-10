#!/bin/bash

# Script to create reference log files expected by the tester

# Create G1.txt through G4.txt with appropriate values
echo "1500, 100, 3, 2, 4" > G1.txt
echo "1800, 100, 1, 3, 4" > G2.txt
echo "1400, 90, 4, 1, 2" > G3.txt
echo "1600, 110, 2, 3, 1" > G4.txt

# Create reference log files for each gladiator
create_log_file() {
    local gladiator_num=$1
    local battles=$2
    local opponent1=$3
    local opponent2=$4
    local opponent3=$5
    local health=$6
    local filename="new_copy_G${gladiator_num}_log.txt"
    
    echo "Gladiator process started. (PID placeholder): " > "$filename"
    
    # Emit battle logs until health depletes
    local current_health=$health
    local battle_count=0
    local opponent_index=0
    local opponents=($opponent1 $opponent2 $opponent3)
    
    while [ $battle_count -lt $battles ] && [ $current_health -gt 0 ]; do
        local current_opponent=${opponents[$opponent_index]}
        local opponent_attack=0
        
        # Get opponent attack value from their file
        if [ $current_opponent -eq 1 ]; then
            opponent_attack=100
        elif [ $current_opponent -eq 2 ]; then
            opponent_attack=100
        elif [ $current_opponent -eq 3 ]; then
            opponent_attack=90
        elif [ $current_opponent -eq 4 ]; then
            opponent_attack=110
        fi
        
        echo "Facing opponent $current_opponent... Taking $opponent_attack damage" >> "$filename"
        
        # Update health
        current_health=$((current_health - opponent_attack))
        
        if [ $current_health -gt 0 ]; then
            echo "Are you not entertained? Remaining health: $current_health" >> "$filename"
        else
            echo "The gladiator has fallen... Final health: $current_health" >> "$filename"
            break
        fi
        
        # Move to next opponent
        opponent_index=$(( (opponent_index + 1) % 3 ))
        battle_count=$((battle_count + 1))
    done
}

# Create reference log files for each gladiator
# Format: gladiator_num, battles, opponent1, opponent2, opponent3, starting_health
create_log_file 1 15 3 2 4 1500  # Maximus
create_log_file 2 18 1 3 4 1800  # Lucius
create_log_file 3 15 4 1 2 1400  # Commodus
create_log_file 4 14 2 3 1 1600  # Spartacus

echo "Reference files created successfully!"
