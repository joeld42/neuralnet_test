
#include <assert.h>
#include <math.h>

#include <raylib.h>

#include "tk_game_ui.h"
#include "tk_evalgame.h"

float Lerp( float a, float b, float t)
{
    return (a*(1.0-t)) + (b*t);
}

Color LerpColor( Color a, Color b, float t)
{
    Color result;
    result.r = (int)Lerp( (float)a.r, (float)b.r, t );
    result.g = (int)Lerp( (float)a.g, (float)b.g, t );
    result.b = (int)Lerp( (float)a.b, (float)b.b, t );
    result.a = (int)Lerp( (float)a.a, (float)b.a, t );
    return result;
}

float min_float( float a, float b )
{
    if ( a < b) return a;
    else return b;
}

// FIXME: Change to take a GameAnalysis
Color GetWinColor( int result, int winner, float x_win_chance, float o_win_chance, float tie_chance )
{
    Color win_c = WHITE;
    Color winCol_O = (Color){ 200,200,255,255 };
    Color winCol_X = (Color){ 255,200,200,255 };
    Color tieCol = (Color){ 200,255,200,255 };

    if (result == RESULT_WINNER)
    {
        if (winner == PLAYER_2) {
            win_c = winCol_O;
        } else if (winner == PLAYER_1) {
            win_c = winCol_X;
        }
    } else {
        // no winner yet, color by chance of winner

        if ((x_win_chance > o_win_chance) && (x_win_chance > tie_chance)) {
            return LerpColor( WHITE, winCol_X, x_win_chance );
        } else if ((o_win_chance > x_win_chance) && (o_win_chance > tie_chance)) {
            return LerpColor( WHITE, winCol_X, o_win_chance );
        } else // tie chance
        {
            return LerpColor( WHITE, tieCol, tie_chance );
        }
    }
    return win_c;
}

