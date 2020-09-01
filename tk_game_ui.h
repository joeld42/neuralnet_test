#ifndef TK_GAME_UI_H
#define TK_GAME_UI_H


#include <raylib.h>

#include "tk_evalgame.h"

// UI Stuff shared between the test games

extern GameAnalysis NEUTRAL_ANALYSIS;

// Don't forget to add new modes in tk_game_ui.cpp
enum {
    MODE_PLAY,
    MODE_STEP,
    MODE_GALLERY,
    MODE_TREE,

    NUM_MODES
};



// -- misc utilities
float min_float( float a, float b );
float Lerp( float a, float b, float t);
Color LerpColor( Color a, Color b, float t);
Rectangle FitSquareInRect( Rectangle outer_rect );

// Game display stuff
Color GetWinColor( int result, int winner, float x_win_chance, float o_win_chance, float tie_chance );

// Debug MCTS tree
struct DbgTreeRect
{
    Rectangle rect;
    int nodeNdx;
};

#define MAX_UI_MOVES (50)
struct PotentialMove
{
    GameState move;
    GameAnalysis analysis;
    Rectangle screenRect;
    float value;
};

struct GameUIStuff
{
    int mode = MODE_PLAY;
    GameAppInfo app;
    
    int numPotentialMoves;
    PotentialMove potentialMoves[MAX_UI_MOVES];
    
    // TODO: Move mode out here
    int autoTrain;
    float temperature;
};

GameState GameUI_CurrMove( GameUIStuff &gameUI );
GameState &GameUI_CurrMoveRef( GameUIStuff &gameUI );
GameState GameApp_CurrMove( GameAppInfo &gameApp );
GameState &GameApp_CurrMoveRef( GameAppInfo &gameApp );

void SetupNeutralAnalysis( GameAnalysis &ga );

void GameUI_ResetGame( GameUIStuff &gameUI );
void GameUI_MoveUsingTreeSearch( GameUIStuff &gameUI );
void GameUI_AutoTrain( GameUIStuff &gameUI, int count );
void GameUI_GatherPotentialMoves( GameUIStuff &gameUI );

void GameUI_DoCommonUI( GameUIStuff &gameUI );
void GameUI_DrawGallery( GameUIStuff &gameUI );
void GameUI_DrawTree( GameUIStuff &gameUI );

// Defined in game specific stuff, not in GameUI common
void DrawBoard( Rectangle outer_rect, GameState state,
               GameAnalysis ga, GameUIStuff *gameUI, bool showPreview );


#endif
