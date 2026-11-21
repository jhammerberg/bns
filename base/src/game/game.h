#ifndef GAMES_H
#define GAMES_H

#include "player.h"

// 3-2-1 countdown before each game. The simulator builds with
// -DGAME_COUNTDOWN=0 so edits show up immediately.
#ifndef GAME_COUNTDOWN
#define GAME_COUNTDOWN 1
#endif

void start_game(struct Player *player1, struct Player *player2);
void reset_game(struct Player *player1, struct Player *player2);
void countdown(void);
void run_game(struct Player *player1, struct Player *player2);
void end_game(int winner, struct Player *player1, struct Player *player2);

#endif // GAMES_H
