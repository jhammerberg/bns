
#include "hub75.h"
#include "hub75_text.h"
#include "game.h"


void start_game(struct Player *player1, struct Player *player2) {
    player1->health = 100;
    player1->mana = 100;
    player1->inputCount = 0;
    player1->shieldActive = false;
    player1->shieldHealth = 0;

    player2->health = 100;
    player2->mana = 100;
    player2->inputCount = 0;
    player2->shieldActive = false;
    player2->shieldHealth = 0;

    // start countdown timer for game start
    countdown();

    run_game(player1, player2);


}

void reset_game(struct Player *player1, struct Player *player2) {
    if(player1->reset && player2->reset) {
        // Reset both players
        start_game(player1, player2);
    } 
}

void countdown() {
    // display countdown on screen
    hub75_draw_char('3', 0, 0, HUB75_WHITE, HUB75_BLACK);
    hub75_refresh();
    // delay(1000);
    hub75_draw_char('2', 0, 0, HUB75_WHITE, HUB75_BLACK);
    hub75_refresh();
    // delay(1000);
    hub75_draw_char('1', 0, 0, HUB75_WHITE, HUB75_BLACK);
    hub75_refresh();
    // delay(1000);
}



void end_game(int winner, struct Player *player1, struct Player *player2) {
    // Display end game screen
    if(winner == 2) {
        hub75_draw_text("Player 2 Wins!", 0, 0, HUB75_RED, HUB75_BLACK);
    } else {
        hub75_draw_text("Player 1 Wins!", 0, 0, HUB75_GREEN, HUB75_BLACK);
    }

    while(1) {
        // Wait for reset input from both players
        if(player1->reset && player2->reset) {
            // Reset the game
            start_game(player1, player2);
            break;
        }
    }
}

// maybe add inputs for controls?
void run_game(struct Player *player1, struct Player *player2) {
    while(player1->health > 0 && player2->health > 0) {
        // Game loop
        // Handle player inputs and game logic here
        player1->mana += MANA_REGEN_RATE;
        player2->mana += MANA_REGEN_RATE;
    }

    if(player1->health <= 0) {
        end_game(2, player1, player2); // Player 2 wins
    } else {
        end_game(1, player1, player2); // Player 1 wins
    }
}



