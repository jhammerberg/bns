
#include "hub75.h"
#include "hub75_text.h"
#include "game.h"
#include "hud.h"
#include "timer.h"
#include "input.h"


void start_game(struct Player *player1, struct Player *player2) {
    player1->health = 100;
    player1->mana = 100;
    player1->shieldActive = false;
    player1->shieldHealth = 0;
    player1->maxShieldHealth = 0;
    player1->lastCast = NULL;

    player2->health = 100;
    player2->mana = 100;
    player2->shieldActive = false;
    player2->shieldHealth = 0;
    player2->maxShieldHealth = 0;
    player2->lastCast = NULL;

    // start countdown timer for game start
    if(GAME_COUNTDOWN) {
        countdown();
    }

    run_game(player1, player2);


}

void reset_game(struct Player *player1, struct Player *player2) {
    if(player1->reset && player2->reset) {
        // Reset both players
        start_game(player1, player2);
    } 
}

// Show one countdown digit on every panel for 1 s, keeping the panels lit.
static void countdown_step(char digit) {
    for(uint32_t panel = 0; panel < HUB75_PANELS; panel++) {
        hub75_select_panel(panel);
        hub75_clear();
        draw_countdown_number(digit);
    }
    hub75_select_panel(0);
    const uint32_t start = millis();
    while(millis() - start < 1000) {
        hub75_refresh();
    }
}

void countdown() {
    countdown_step('3');
    countdown_step('2');
    countdown_step('1');
}



// Each player sees their own panel: player 1 on panel 0, player 2 on panel 1.
// With a single panel only player 1's view is drawn.
static void draw_views(void (*draw)(struct Player *, struct Player *),
                       struct Player *player1, struct Player *player2) {
    hub75_select_panel(0);
    draw(player1, player2);
    if(HUB75_PANELS > 1) {
        hub75_select_panel(1);
        draw(player2, player1);
        hub75_select_panel(0);
    }
}

static void draw_result(struct Player *player, struct Player *opponent) {
    (void)opponent;
    hub75_clear();
    draw_victory_screen(player->health > 0);
}

void end_game(int winner, struct Player *player1, struct Player *player2) {
    (void)winner; // each panel shows win/lose from its own player's health
    draw_views(draw_result, player1, player2);

    // Wait for reset input from both players, keeping the panels lit
    while(!(player1->reset && player2->reset)) {
        hub75_refresh();
    }
    start_game(player1, player2);
}

// Shapes from the wands: wand 1 is player 1, wand 2 is player 2.
static void handle_input(struct Player *player1, struct Player *player2) {
    bns_packet_t pkt;
    while(input_poll(&pkt)) {
        if(pkt.wand_id == 1) {
            castShape(player1, player2, (bns_shape_t)pkt.shape);
        } else if(pkt.wand_id == 2) {
            castShape(player2, player1, (bns_shape_t)pkt.shape);
        }
    }
}

void run_game(struct Player *player1, struct Player *player2) {
    for(uint32_t panel = 0; panel < HUB75_PANELS; panel++) {
        hub75_select_panel(panel);
        hub75_clear();
    }
    hub75_select_panel(0);
    uint32_t last_mana_regen = millis();

    while(player1->health > 0 && player2->health > 0) {
        handle_input(player1, player2);

        if(millis() - last_mana_regen >= 1000) {
            last_mana_regen = millis();
            player1->mana += player1->mana == MAX_MANA ? 0 : MANA_REGEN_RATE;
            player2->mana += player2->mana == MAX_MANA ? 0 : MANA_REGEN_RATE;
        }

        draw_views(draw_game, player1, player2);
        hub75_refresh();
    }

    if(player1->health <= 0) {
        end_game(2, player1, player2); // Player 2 wins
    } else {
        end_game(1, player1, player2); // Player 1 wins
    }
}
