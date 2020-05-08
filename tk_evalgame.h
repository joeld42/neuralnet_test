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

enum {
	IN_PROGRESS,
	WINNER_X,
	WINNER_O,
	TIE_GAME
};

enum {
	SIM_RANDOM,
	NET_EVAL,

	NUM_SIM_MODES
};

struct GameState {
	uint8_t to_move;
	uint8_t square[9]; 
	uint8_t winner;
	uint8_t win0, win2;	// helper to draw the win line	
};

struct GameAnalysis {
	float x_win_chance;
	float o_win_chance;
	float tie_chance;
};

#define NUM_MCTS_NODE (5000)

struct MCTSNode
{
	GameState state;
	int parentNdx;

	int childNdx[9]; // Note 0 means no child because root is 0
	
	float totalWins;
	float totalVisits;
	int moveNum; // move that got us here

	// for dbg draw
	int level;
	int xval;
	bool selected;
	bool expanded;
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


void LoadWeights( GameState &state, double *inputs );
GameAnalysis AnalyzeGame( GameAppInfo &app, GameState game );
GameState ApplyMove( GameState prevState, int moveLoc );
GameState CheckWinner( GameState game );
int MakeNode( GameAppInfo &app );
float NodeValUCB1( GameAppInfo &app, MCTSNode &node );
int RolloutOnce( GameState state, int player );
int Rollout( GameAppInfo &app, GameState state, int runs, int player );
int ChooseRandomMove( const GameState &game );
int TreeSearchMove( GameAppInfo &app, const GameState &game );
void TrainHistory( GameAppInfo &app );
void TrainOneStep( GameAppInfo &app, float temperature );
void ResetGame( GameAppInfo &app );

float UniformRandom();

#endif