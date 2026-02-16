#include <iostream>
#include <string>
#include <vector>

using namespace std; //allows to ommit the ever so annoying std::, no other libraries used here so no conflict due to the namespace, just saves time

//================================
//=======Class setup section======
//================================

//Default values here but can also be changed when called in main() to make a differently sized room

class Game {
public:
    //Room size
    int number_of_rows = 10;
    int number_of_columns = 15;

    //Player coordinates
    int player_row = 0;
    int player_column = 0;
    
    //Tracks state of play
    bool playing = true;
};


//Function used for room drawing - uses game instance
void draw_room(Game& game) {
    //Shortcut for initializing string with column number times the second char '-'
    string horizontal_boundaries(game.number_of_columns, '-');

    cout << "+" << horizontal_boundaries << "+" << endl;

    //Nested FOR loops checking player row and column position
    for(int i = 0; i < game.number_of_rows; i++) {
        cout << "|";
        for(int j = 0; j < game.number_of_columns; j++) {
            if(game.player_row == i && game.player_column == j) {
                cout << "@"; //Player char
            }
            else {
                cout << "."; //Default empty space char
            }
        }
        cout << "|" << endl;
    }
    
    cout << "+" << horizontal_boundaries << "+" << endl;

}

//This function prompts the user for a terminal input direction in the WASD style (we're all gamers here)
bool player_move(Game& game) {

    //Terminal interface
    cout << "Press WASD to move. Press Q to quit ";
    char input = ' ';
    cin >> input;

    //User input & Player coordinate check - Both upper and lower case accepted
    //Each switch case uses the game instance to check whether the player is in bounds
    // ----if in bounds then move player and the new draw_room() will update position
    //-----if not in bounds then print the message saying the corresponding wall was reached
    switch (input) {
    case 'W':
    case 'w':
        if (game.player_row > 0)
            game.player_row--;
        else
            cout << "\n" 
            << "========================================\n"
            << "Sorry, you have reached the North Wall.\n" 
            << "========================================\n"
            << endl;
        break;

    case 'S':
    case 's':
        if (game.player_row < game.number_of_rows - 1)
            game.player_row++;
        else
            cout << "\n" 
            << "========================================\n"
            << "Sorry, you have reached the South Wall.\n" 
            << "========================================\n"
            << endl;
        break;

    case 'A':
    case 'a':
        if (game.player_column > 0)
            game.player_column--;
        else
            cout << "\n" 
            << "========================================\n"
            << "Sorry, you have reached the West Wall.\n" 
            << "========================================\n"
            << endl;
        break;

    case 'D':
    case 'd':
        if (game.player_column < game.number_of_columns - 1)
            game.player_column++;
        else
            cout << "\n" 
            << "========================================\n"
            << "Sorry, you have reached the East Wall.\n" 
            << "========================================\n"
            << endl;
        break;
    
    //Game Termination - used in main to cancel the while loop
    case 'q': case 'Q': 
        return false;

    default:
        //Restart again if the user types incorrectly, which is definitely possible given each input has to be followed by Enter
        cout << "Invalid input.\n";
    }

    //Keep while loop running
    return true;

}

int main() {    

    //instantiates a Game object with default parameters specified in class declaration
    Game current_game;

    //Class boolean keeps track of game state
    while(current_game.playing) {
        draw_room(current_game);
        //If statement triggered when player presses Q -> terminates while loop
        if(!player_move(current_game)) {
            current_game.playing = false;
        };
    }
    
}

