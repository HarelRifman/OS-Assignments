#!/bin/bash

# checks if the amount of arguments was correctly given 
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <source_pgn_file> <destination_directory>"
    exit 1
fi

# Check if the source exists 
if [ ! -f "$1" ]; then 
    echo "Error: File '$1' does not exist."
    exit 1
fi

# Check if destination dir exists; if not, make it
if [ ! -d "$2" ]; then 
    mkdir "$2"
    echo "Created directory '$2'."
fi

num=0
current_cont=""

# reading the file line by line 
while IFS= read -r line; do
  if [[ ${line:0:1} == "[" ]]; then 
      # if reached the metadata of a new game, save the previous one and print to the screen
      if [ -n "$current_cont" ]; then 
          ((num+=1))
          echo -e "$current_cont" > "$2/$(basename "$1" .pgn)_$num.pgn"  
          echo "Saved game to $2/$(basename "$1" .pgn)_$num.pgn"
      fi
      # starting reading the next game
      current_cont=""
      while IFS= read -r inner_line && [[ ${inner_line:0:1} == "[" ]]; do
            current_cont+="$inner_line\n"
      done
      current_cont+="\n"
  else 
    current_cont+="$line\n"
  fi
done < "$1"

# Save the last game if there was any remaining content
if [ -n "$current_cont" ]; then 
    ((num+=1))
    echo -e "$current_cont" > "$2/$(basename "$1" .pgn)_$num.pgn"  
    echo "Saved game to $2/$(basename "$1" .pgn)_$num.pgn"
fi

echo "All games have been split and saved to '$2'."

