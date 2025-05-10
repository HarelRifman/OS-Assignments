#!/bin/bash

# Chess PGN Viewer by LateNightHacker
# v1.2 - The one that finally works with weird PGN formats

# Ensure the required Python script exists
if [[ ! -f parse_moves.py ]]; then
    echo "Error: 'parse_moves.py' script is missing!"
    exit 1
fi

# Validate if the PGN file exists
if [[ ! -f "$1" ]]; then
    echo "File does not exist: $1"
    exit 1
fi

# Display the metadata from the PGN file
echo "Metadata from PGN file:"
grep '^\[' "$1"

# Extract the moves part from the PGN file
# Finally fixed this regex after 3 hours of banging my head against the wall
moves_txt=$(sed -n '/^\[/d; /^$/d; p' "$1" | tr '\n' ' ')


# Convert the moves to UCI format using the provided Python script
uci_stuff=$(python3 parse_moves.py "$moves_txt" 2>/dev/null)

# Check if parse_moves.py was successful
# Reminding that the dollar sign stores the exit status of the last command
if [ $? -ne 0 ]; then
    echo "Error parsing the PGN file. Please check the format."
    exit 1
fi


# Remove brackets and quotes, split into a line
move_sequence=($(echo "$uci_stuff" | tr -d '[]",'  | tr ' ' '\n'))


num_mv=0

# Initialize the chess board
declare -A board
# Set up the initial board position
function init_board() {
    # Clear the board
    for row in {1..8}; do
        for col in {a..h}; do
            board[$col$row]="."
        done
    done

    # Set up white pieces
    board["a1"]="R"
    board["b1"]="N"
    board["c1"]="B"
    board["d1"]="Q"
    board["e1"]="K"
    board["f1"]="B"
    board["g1"]="N"
    board["h1"]="R"

    # Set up black pieces
    board["a8"]="r"
    board["b8"]="n"
    board["c8"]="b"
    board["d8"]="q"
    board["e8"]="k"
    board["f8"]="b"
    board["g8"]="n"
    board["h8"]="r"

    # Set up pawns
    for col in {a..h}; do
        board[$col"2"]="P"
        board[$col"7"]="p"
    done
}

# Function to display the chess board
function board_printer() {
    echo "  a b c d e f g h"
    for row in {8..1}; do
        echo -n "$row "
        for col in {a..h}; do
            piece=${board[$col$row]}
            if [[ "$piece" == "." ]]; then
                echo -n ".  "
            else
                echo -n "$piece  "
            fi
        done
        echo "$row"
    done
    echo "  a b c d e f g h"
}



# Function to apply a move in UCI which recives a move and applay's it to the board 
function apply_move() {
    local move=$1 
    local from="${move:0:2}"
    local to="${move:2:2}"
    local piece=${board[$from]}

    # Check if this is a promotion as if it has five chars
    local promotion=""
    if [[ ${#move} -gt 4 ]]; then
        promotion="${move:4:1}"
    fi
    
    # Handle castling
    if [[ "$from" == "e1" && "$to" == "g1" && "$piece" == "K" ]]; then
        # King-side castling for white
        board["g1"]="K"
        board["f1"]="R"
        board["e1"]="."
        board["h1"]="."
        return
    elif [[ "$from" == "e1" && "$to" == "c1" && "$piece" == "K" ]]; then
        # Queen-side castling for white
        board["c1"]="K"
        board["d1"]="R"
        board["e1"]="."
        board["a1"]="."
        return
    elif [[ "$from" == "e8" && "$to" == "g8" && "$piece" == "k" ]]; then
        # King-side castling for black
        board["g8"]="k"
        board["f8"]="r"
        board["e8"]="."
        board["h8"]="."
        return
    elif [[ "$from" == "e8" && "$to" == "c8" && "$piece" == "k" ]]; then
        # Queen-side castling for black
        board["c8"]="k"
        board["d8"]="r"
        board["e8"]="."
        board["a8"]="."
        return
    fi

    # Check for en passant capture
    if [[ ("$piece" == "P" || "$piece" == "p") && 
          "${from:0:1}" != "${to:0:1}" && 
          "${board[$to]}" == "." ]]; then
        # This ridiculous edge case took me 2 days to debug correctly
        local capture_square="${to:0:1}${from:1:1}"
        if [[ "${board[$capture_square]}" != "." ]]; then
            # There's a piece in the capture square, likely en passant
            board[$capture_square]="."
        fi
    fi
    
    # If this is a promotion, use the promotion piece
    if [[ -n "$promotion" ]]; then
        if [[ "$piece" == "P" ]]; then
            # For white pawns, use uppercase promotion pieces
            case "$promotion" in
                "q") piece="Q";;
                "r") piece="R";;
                "b") piece="B";;
                "n") piece="N";;
            esac
        else
            # For black pawns, use lowercase promotion pieces
            case "$promotion" in
                "q") piece="q";;
                "r") piece="r";;
                "b") piece="b";;
                "n") piece="n";;
            esac
        fi
    fi
    
    # Move the piece
    board[$to]=$piece
    board[$from]="."
}

# Function to move forward
function move_forward() {
    if [[ $num_mv -lt ${#move_sequence[@]} ]]; then
        echo "Applying move: ${move_sequence[$num_mv]}"
        apply_move "${move_sequence[$num_mv]}"
        ((num_mv++))
        echo "Move $num_mv/${#move_sequence[@]}"
        board_printer
    else
        echo "No more moves available."
    fi
}

# Function to go back to the previous move
function go_back_one() {
    if [[ $num_mv -gt 0 ]]; then
        ((num_mv--))
        # Reset the board and replay all moves up to the current position
        init_board
        for ((i=0; i<$num_mv; i++)); do
            apply_move "${move_sequence[$i]}"
        done
        echo "Move $num_mv/${#move_sequence[@]}"
        board_printer
    fi
}

# Function to go to the start of the game
function rewind_to_start() {
    num_mv=0
    init_board
    echo "Move $num_mv/${#move_sequence[@]}"
    board_printer
}

# Function to go to the end of the game
function fast_forward_to_end() {
    init_board
    for ((i=0; i<${#move_sequence[@]}; i++)); do
        apply_move "${move_sequence[$i]}"
    done
    num_mv=${#move_sequence[@]}
    echo "Move $num_mv/${#move_sequence[@]}"
    board_printer
}

# Initialize the board for the start of game
init_board
echo "Move $num_mv/${#move_sequence[@]}"
board_printer

# Main loop to interactively play the game
while true; do
    echo "Press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit:"
    read -n 1 key
    
    case "$key" in
        d) move_forward ;;
        a) go_back_one ;;
        w) rewind_to_start ;;
        s) fast_forward_to_end ;;
        q) echo "Exiting."; echo "End of game."; break ;;
        *) echo "Invalid key pressed: $key" ;;
    esac
done
