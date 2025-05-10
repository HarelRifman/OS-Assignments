#!/bin/bash

# Script to create gladiator files G1.txt through G4.txt

# Clear any existing files
rm -f G1.txt G2.txt G3.txt G4.txt

# Create G1.txt - Maximus
# Format: Health, Attack, Opponent1, Opponent2, Opponent3
echo "1500, 100, 3, 2, 4" > G1.txt
echo "Created G1.txt (Maximus): Health=1500, Attack=100, Opponents=3,2,4"

# Create G2.txt - Lucius
# Making him a bit stronger with more health
echo "1800, 100, 1, 3, 4" > G2.txt
echo "Created G2.txt (Lucius): Health=1800, Attack=100, Opponents=1,3,4"

# Create G3.txt - Commodus
echo "1400, 90, 4, 1, 2" > G3.txt
echo "Created G3.txt (Commodus): Health=1400, Attack=90, Opponents=4,1,2"

# Create G4.txt - Spartacus
echo "1600, 110, 2, 3, 1" > G4.txt
echo "Created G4.txt (Spartacus): Health=1600, Attack=110, Opponents=2,3,1"

echo "All gladiator files created successfully!"
echo "You can now compile and run the tournament program."

