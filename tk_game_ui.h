#ifndef TK_GAME_UI_H
#define TK_GAME_UI_H

#include <raylib.h>

// UI Stuff shared between the test games

// -- misc utilities
float min_float( float a, float b );
float Lerp( float a, float b, float t);
Color LerpColor( Color a, Color b, float t);

// Game display stuff
Color GetWinColor( int result, int winner, float x_win_chance, float o_win_chance, float tie_chance );

// Debug MCTS tree
struct DbgTreeRect
{
    Rectangle rect;
    int nodeNdx;
};




#endif
