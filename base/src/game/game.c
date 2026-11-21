
#include "hub75.h"
#include "hub75_text.h"
#include "game.h"
#include "game_assets.h"
#include "timer.h"


void start_game(struct Player *player1, struct Player *player2) {
    player1->health = 100;
    player1->mana = 100;
    player1->inputCount = 0;
    player1->shieldActive = false;
    player1->shieldHealth = 0;
    player1->maxShieldHealth = 0;

    player2->health = 100;
    player2->mana = 100;
    player2->inputCount = 0;
    player2->shieldActive = false;
    player2->shieldHealth = 0;
    player2->maxShieldHealth = 0;

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
    uint32_t start = millis();
    draw_countdown_number('3');
    while(millis() - start < 1000) {
        hub75_refresh();
    }
    start = millis();
    draw_countdown_number('2');
    while(millis() - start < 1000) {
        hub75_refresh();
    }

    start = millis();
    draw_countdown_number('1');
    while(millis() - start < 1000) {
        hub75_refresh();
    }
    return;
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
    hub75_clear();
    uint32_t last_mana_regen = millis();
    
    while(player1->health > 0 && player2->health > 0) {
        // Game loop
        // Handle player inputs and game logic here

        if(millis() - last_mana_regen >= 1000) {
            last_mana_regen = millis();
            player1->mana += player1->mana == MAX_MANA ? 0 : MANA_REGEN_RATE;
            player2->mana += player2->mana == MAX_MANA ? 0 : MANA_REGEN_RATE;
        }
        
        

        draw_game(player1, player2);
        hub75_refresh();
    }       

    

    if(player1->health <= 0) {
        end_game(2, player1, player2); // Player 2 wins
    } else {
        end_game(1, player1, player2); // Player 1 wins
    }
}

   



