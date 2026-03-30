#include <iostream>
#include <string>
#include<cstdlib>

//=================================================
//=========Main Game Class + Helper class==========
//=================================================

//Enum class to keep track of the game state
enum Game_States {
    in_progress,
    player_won,
    computer_won,
    draw
};

//Main class
class Game {
    public:
        //Storing Tic-Tac-Toe board (default empty)
        char board[3][3] = {
            {'.', '.', '.'},
            {'.', '.', '.'},
            {'.', '.', '.'}
        };
        Game_States game_state = in_progress;
        int number_of_plays = 0;
};

//==================================
//=========Helper Functions=========
//==================================

//All helper function require a game instance to be passed by refference

//Function using nested FOR loops printing the game_state
void print_board(const Game& current_game) {

    std::cout << "\n+" << " - - - " << "+" << std::endl;

    //Iterates the Game instance's internal board array and prints it out with nice vertical spacers
    for(int i = 0; i < 3; i++) {
        std::cout << "| ";
        for(int j = 0; j < 3; j++) {
            std::cout << current_game.board[i][j] << " "; 
        }
        std::cout << "|" << std::endl;
    }
    
    std::cout << "+" << " - - - " << "+\n" << std::endl;
}

//Both player and computer use this function to interact with the Game object
void make_move(Game& current_game, bool is_player_turn) {
    //Player Plays
    if(is_player_turn) {
        //While loop ensures a player makes a valid movement -> is exited with break when correct move is made
        while (true) {
            std::cout << "Please select which tile to mark by typing 1 - 9. (Assume 1 is top left)" << std::endl;
            int player_choice = ' ';
            std::cin >> player_choice;

            //Useful math which converts the numbers to the corresponding positions 
            int row = (player_choice - 1) / 3;
            int column = (player_choice - 1) % 3;

            char chosen_tile = current_game.board[row][column];
            
            //Logic for valid or invalid move -> uses Game instance's internal board array
            if(chosen_tile == '.') {
                current_game.board[row][column] = 'O';
                break;
            }
            else {
                std::cout << 
                "\n==================================================\n" <<
                "Sorry, that tile is occupied. Please, try again.\n" <<
                "=================================================="
                << std::endl;
                print_board(current_game);
            }
        }
        
    }
    //Computer Plays
    else {
        while(true) {
            //Exit if board full -> makes sure the CPU doesn't explode if the while loop never exits because the board is full
            if (current_game.number_of_plays >= 9) return;

            //Roll the computer choice 1 - 9
            int computer_choice = (rand() % 9) + 1;  // 1..9

            //Locate tile on the board using the same math logic as before
            int row = (computer_choice - 1) / 3;
            int column = (computer_choice - 1) % 3; 
            char chosen_tile = current_game.board[row][column];
            
            //If the tile isn't empty then if statement isn't passed -> loop repeats
            if(chosen_tile == '.') {
                current_game.board[row][column] = 'X';
                break;
            }
        }
    }

    //Ensure both computer and player increment counter
    current_game.number_of_plays++;
}

//Helper function to check all win conditions
void check_win(Game& game_instance, char comparison_char) {
    // Check All Rows
    for (int row = 0; row < 3; row++) {
        if (game_instance.board[row][0] == comparison_char &&
            game_instance.board[row][1] == comparison_char &&
            game_instance.board[row][2] == comparison_char) {

            game_instance.game_state = (comparison_char == 'O') ? player_won : computer_won;
            return;
        }
    }

    // Check All Columns
    for (int col = 0; col < 3; col++) {
        if (game_instance.board[0][col] == comparison_char &&
            game_instance.board[1][col] == comparison_char &&
            game_instance.board[2][col] == comparison_char) {

            game_instance.game_state = (comparison_char == 'O') ? player_won : computer_won;
            return;
        }
    }

    // Check Diagonals
    if (game_instance.board[0][0] == comparison_char &&
        game_instance.board[1][1] == comparison_char &&
        game_instance.board[2][2] == comparison_char) {

        game_instance.game_state = (comparison_char == 'O') ? player_won : computer_won;
        return;
    }

    if (game_instance.board[0][2] == comparison_char &&
        game_instance.board[1][1] == comparison_char &&
        game_instance.board[2][0] == comparison_char) {

        game_instance.game_state = (comparison_char == 'O') ? player_won : computer_won;
        return;
    }
}


int main() {

    //Initialize Game instance
    Game test_game;

    //Providing a seed value to the random library
    srand((unsigned) time(NULL));

    //Keep playing until game is no longer "in progress" -> tracker by the enum class inside the Game object
    while(test_game.game_state == in_progress) {
        print_board(test_game);
        
        //Player's turn and win check
        make_move(test_game, true); 
        check_win(test_game, 'O');

        //Computer's turn and win check
        make_move(test_game, false); 
        check_win(test_game, 'X'); 

        //Check if board full -> draw (how does one lose draw against computer) (computer still takes a turn after board is full but it doesn't matter as its a draw anyway)
        if(test_game.number_of_plays >= 9 && test_game.game_state == in_progress) {
            test_game.game_state = draw;
        }
    }

    //Game is over but show the last board state before win message
    print_board(test_game);

    //Win/Loss/Draw terminal text
    switch(test_game.game_state) {
        case draw: 
            std::cout <<
            "======================\n" <<
            "=====ITS A DRAW!!!====\n" <<
            "=======================" << std::endl;
            break;
        case player_won: 
            std::cout <<
            "=======================\n" <<
            "======YOU WON!!!!=====\n" <<
            "======================" << std::endl;
            break;
        case computer_won: 
            std::cout <<
            "=======================\n" <<
            "======YOU LOST!!!!=====\n" <<
            "=======================" << std::endl;
            break;
        case in_progress:
            break;
        default:
            std::cout << 
            "How did you get here :D"
            << std::endl;
    }

    return 0;
}

