#ifndef TK_EVALGAME_H
#define TK_EVALGAME_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "genann.h"

enum {
	SQUARE_BLANK,
	SQUARE_X,
	SQUARE_O,	
};

// Game Result
enum {
	RESULT_IN_PROGRESS,	
	RESULT_WINNER,
	RESULT_TIE_GAME
};

enum {
    PLAYER_NULL, // Note: still assumes this lines up with SQUARE_ enum
	PLAYER_1,
	PLAYER_2,
    // etc...
};

enum {
	SIM_RANDOM,
	NET_EVAL,

	NUM_SIM_MODES
};

#define MAX_PLAYERS (2)

// E.g. token color, name, etc..
struct PlayerInfo {
    int reserved_todo;
};

struct GameState;
struct GameStateArray;

//typedef returnType (*typeName)(parameterTypes);
typedef void (*LoadWeightsFunc)( GameState&, double *);
typedef void (*GeneratePossibleMovesFunc)( const GameState &game, GameStateArray &moves );

// Setup info for the game that does not
// change over the game, e.g. number of players
struct GameInfo {
    uint8_t numPlayers;
    uint8_t minPlayerCount;
    uint8_t maxPlayerCount;
    PlayerInfo playerInfo[MAX_PLAYERS];
    
    // net hyperparameters
    int net_inputs;
    int net_hidden_layers;
    int net_hidden_layer_size;
        
    // Callbacks for Game Specific stuff
    LoadWeightsFunc gameFunc_LoadWeights;
    GeneratePossibleMovesFunc gameFunc_GeneratePossibleMoves;
};

// Player state info
struct PlayerState {
    
    // todo: move, debug helper to draw the win line
    uint8_t winLine0, winLine2;
};


// TODO: Move to game_tictoe
struct TicToeGameState {
    // tic-toe specific stuff (will split out)
    uint8_t square[9];
    uint8_t win0, win2;    // helper to draw the win line
    uint8_t lastMove; // the last square moves
};

struct GameState {
    
    // generic stuff
	uint8_t to_move;
	uint8_t gameResult;
	uint8_t winner;
    
    TicToeGameState gg; // real game
};

struct PlayerAnalysis {
    float win_chance;
};

struct GameAnalysis {
    PlayerAnalysis plr[MAX_PLAYERS];
	float tie_chance;
};

struct GameStateArray
{
    size_t size;
    size_t capacity;
    GameState *data;
};

// Helpers for GameStateArray
void PushGameState( GameStateArray *games, GameState state );
void FreeArray( GameStateArray *games );
void ClearArray( GameStateArray *games );

#define NUM_MCTS_NODE (5000)

struct MCTSNode
{
	GameState state;
	int parentNdx;

	//int childNdx[9]; // Note 0 means no child because root is 0
    int leftChildNdx;
    int rightSiblingNdx;
	
	float totalWins;
	float totalVisits;
	int moveNum; // move that got us here

	// for dbg draw
	int level;
	int xval;
	bool selected;
    bool ui_expanded;
};


enum {
	MODE_PLAY,
	MODE_GALLERY,
	MODE_TREE,

	NUM_MODES
};

struct GameAppInfo
{
	int mode = MODE_PLAY;
	int currMove;
    GameInfo info;
    GameState gameHistory[10];
    GameAnalysis gameHistory_A[10];

    GameState gridBoards[100];
    GameAnalysis gridBoards_A[100];

    genann *net;
    double *inputs;
    double *outputs;
    int trainCount;
    int gameCount;
    int winXcount;
    int winOcount;
    int tieCount;

    int simMode;

    MCTSNode *nodes;
    int numNodes;
    
    GameAnalysis preview[9]; // preview for all squares
};


void GameInit( GameAppInfo &appInfo );
GameAnalysis AnalyzeGame( GameAppInfo &app, GameState game );
GameState CheckWinner( GameState game );
GameState ChooseRandomMove( GameAppInfo &app, GameStateArray &possibleMoves );
GameState ChooseRandomMoveSimple( GameAppInfo &app, const GameState &prevState );
int MakeNode( GameAppInfo &app );
float NodeValUCB1( GameAppInfo &app, MCTSNode &node );
int RolloutOnce( GameState state, int player );
int Rollout( GameAppInfo &app, GameState state, int runs, int player );
GameState TreeSearchMove( GameAppInfo &app, const GameState &game );
void TrainHistory( GameAppInfo &app );
void TrainOneStep( GameAppInfo &app, float temperature );
void ResetGame( GameAppInfo &app );

float UniformRandom();

#endif
