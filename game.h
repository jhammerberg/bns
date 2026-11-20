#ifndef GAMES_H
#define GAMES_H

#include "player.h"

void start_game(struct Player *player1, struct Player *player2);
void reset_game(struct Player *player1, struct Player *player2);
void countdown(void);
void run_game(struct Player *player1, struct Player *player2);
void end_game(int winner, struct Player *player1, struct Player *player2);

#endif // GAMES_H
