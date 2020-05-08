#include "tk_evalgame.h"

#include <stdlib.h>
#include <math.h>

// TODO from raylib -- reimplement
extern "C" {
int GetRandomValue( int minVal, int maxVal );
}

void LoadWeights( GameState &state, double *inputs )
{
	// one weight per sqaure
	for (int i=0; i < 9; i++) {
		if (state.square[i]==SQUARE_X) {
			inputs[i] = -1.0;
		} else if (state.square[i]==SQUARE_O) {
			inputs[i] = 1.0;
		} else {
			inputs[i] = 0.0;
		}
	}

	// Weight for whose turn it is
	inputs[9] = (state.to_move==SQUARE_X) ? 1.0 : 0.0;
	inputs[10] = (state.to_move==SQUARE_O) ? 1.0 : 0.0;
}


GameAnalysis AnalyzeGame( GameAppInfo &app, GameState game )
{
	// random for now
	GameAnalysis result;

	// Analysis pretty easy if somene one
	if (game.winner == WINNER_X) {
		result.x_win_chance = 1.0;
		result.o_win_chance = 0.0;
		result.tie_chance = 0.0;
	} else if (game.winner == WINNER_O) {
		result.x_win_chance = 0.0;
		result.o_win_chance = 1.0;
		result.tie_chance = 0.0;
	} else {
		// Test: random chance
		//result.p0_win = (float)GetRandomValue(0,100) / 100.0f;
		//result.p1_win = (float)GetRandomValue(0,100) / 100.0f;

		// Use net to evaluate
		// for (int i=0; i < 11; i++) {
		// 	printf("LoadWeights: i %d is %3.2lf\n", i, app.inputs[i] );
		// }
		LoadWeights( game, app.inputs );
		const double *outs = genann_run( app.net, app.inputs );
		// double outs[2];

		// outs[0] = (float)UniformRandom();
		// outs[1] = (float)UniformRandom();

		// also copy to app
		app.outputs[0] = outs[0];
		app.outputs[1] = outs[1];
		app.outputs[2] = outs[2];

		// Normalize and store 
		//float a = NormOutput(outs[0]);
		//float b = NormOutput(outs[1]);
		float a = outs[0];
		float b = outs[1];
		float c = outs[2];
		float total = a+b+c;
		if (total < 0.001)
		{
			total = 1.0f;
		}
		result.x_win_chance = a / total;
		result.o_win_chance = b / total;
		result.tie_chance = c / total;
		
		
	}
	return result;
}

int NextPlayer( int player )
{
	if (player == SQUARE_O) {
		return SQUARE_X;
	} else {
		return SQUARE_O;
	}
}


GameState ApplyMove( GameState prevState, int moveLoc )
{
	GameState result = prevState;
	result.square[moveLoc] = prevState.to_move;
	result.to_move = NextPlayer( prevState.to_move );

	result = CheckWinner( result );

	return result;
}

bool _CheckThree( GameState &game, int a, int b, int c )
{
	if (( game.square[a] != SQUARE_BLANK) &&
		( game.square[a] == game.square[b]) &&
		( game.square[b] == game.square[c]) ) {
		game.winner = game.square[a];
		game.win0 = a;
		game.win2 = c;
		return true;
	}

	return false;
}


// Returns BLANK, X, or O depending on who won
GameState CheckWinner( GameState game )
{
	GameState result = game;
	int rowPatterns[] = {
		// horiz rows
		0, 1, 2,
		3, 4, 5,
		6, 7, 8,
		
		// vert rows
		0, 3, 6,
		1, 4, 7,
		2, 5, 8,

		// diag rows
		0, 4, 8,
		6, 4, 2
	};

	for (int rndx =0; rndx < 8; rndx++)
	{
		int ndx = rndx * 3;
		if (_CheckThree( result, 
			rowPatterns[ndx+0], 
			rowPatterns[ndx+1],
			rowPatterns[ndx+2] )) {
			return result;
		}
	}

	// See if it's a tied game
	bool isTied = true;
	for (int i=0; i < 9; i++) {
		if (game.square[i] == SQUARE_BLANK) {
			isTied = false;
			break;
		}
	}
	if (isTied) {
		result.winner = TIE_GAME;
	}

	return result;
}

// This assumes all the nodes get zeroed out
int MakeNode( GameAppInfo &app )
{
	return app.numNodes++;
}

float NodeValUCB1( GameAppInfo &app, MCTSNode &node )
{
	if (node.totalVisits == 0) {
		// Node hasn't been explored at all yet, UCB value is
		// infinite. Don't return MAX_FLOAT because we might
		// add the wins to this
		return 9999999.0f;
	} 

	float exploitVal = node.totalWins / node.totalVisits;
	float exploreVal = 2.0f * sqrt( log( app.nodes[node.parentNdx].totalVisits ) / (float)node.totalVisits );
	return exploitVal + exploreVal;
}

int RolloutOnce( GameState state, int player )
{
	GameState curr = state;
	while (!curr.winner) {
		int randMove = ChooseRandomMove( curr );
		curr = ApplyMove( curr, randMove );
	}

	if (curr.winner == player) {
		return 1;
	} else if (curr.winner == TIE_GAME) {
		return 0;
	} else {
		return -1;
	}
}

int Rollout( GameAppInfo &app, GameState state, int runs, int player )
{		
	int result = 0;
	if (state.winner == player) {
		// If this is a winning move for player, count it as all wins		
		result = runs;
	} else if (state.winner == TIE_GAME) {
		result = 0;
	} else if (state.winner != IN_PROGRESS) {
		// other player won
		result = -runs;
	} else 
	{
		if (app.simMode == SIM_RANDOM)
		{
			// Otherwise
			int wins = 0;
			for (int i=0; i < runs; i++) {
				wins += RolloutOnce( state, player );
			}
			result = wins;

		} else {
			// Use the net to do the sim
			GameAnalysis ga = AnalyzeGame( app, state );
			int wins;
			if (player==SQUARE_X) {
				wins = (ga.x_win_chance * runs) + (ga.o_win_chance * -runs);
			} else {
				wins = (ga.x_win_chance * -runs) + (ga.o_win_chance * runs);
			}
			result = wins;
		}
	}

	// if (state.to_move != player) {
	// 	result = -result;
	// }

	//printf("Rollout: %d/%d wins\n", wins, runs );
	return result;
}

int ChooseRandomMove( const GameState &game )
{
	int possibleMoves[9];
	int numPossibleMoves = 0;

	if (game.winner) {
		return -1;
	}

	for (int i=0; i < 9; i++) {
		if (game.square[i] == SQUARE_BLANK) {
			possibleMoves[numPossibleMoves++] = i;
		}
	}

	if (numPossibleMoves==0) {
		return 0;
	} else {
		return possibleMoves[ GetRandomValue( 0, numPossibleMoves-1) ];
	}
}

int TreeSearchMove( GameAppInfo &app, const GameState &game )
{
	if (game.winner) {
		return -1;
	}

	// initiaze the search
	memset( app.nodes, 0, sizeof(MCTSNode) * NUM_MCTS_NODE );
	app.numNodes = 0;

	int to_play = game.to_move;

	// start with the current state
	int rootNdx = MakeNode( app );
	app.nodes[rootNdx].state = game;

	int iter = 1000; 
	while ((app.numNodes < NUM_MCTS_NODE-2) && (iter>0))
	{
		//printf("\nTree search %d iters left, %d nodes....\n", iter, app.numNodes);
		iter--;

		// Selection -- walk down the tree until we find the best node to explore
		int currNdx = rootNdx;
		bool isLeaf = false;
		int depth = 0;
		while (!isLeaf) {
			MCTSNode &curr = app.nodes[ currNdx ];

			if (curr.state.winner == IN_PROGRESS)
			{
				// find the child with the best UCB 
				isLeaf = true;
				float bestScore = -999999.0f;
				int bestChildNdx = 0;
				for (int i=0; i < 9; i++) {
					if (curr.childNdx[i] != 0) {
						// This is a non-leaf node, potentially expand
						isLeaf = false;
						int childNdx = curr.childNdx[i];
						float ucb = NodeValUCB1( app, app.nodes[ childNdx ] );
						if (ucb > bestScore) {
							bestScore = ucb;
							bestChildNdx = childNdx;
						}
					}
				}

				if (!isLeaf) {
					currNdx = bestChildNdx;
					depth++;
				}
			} else {
				isLeaf = true;
			}
		}

		

		// If this leaf has already been evaluated, expand it
		int simNdx = 0;
		if ((app.nodes[ currNdx ].totalVisits > 0) && (app.nodes[ currNdx ].state.winner == IN_PROGRESS))
		{
			// Expand the possible moves from best child
			MCTSNode &curr = app.nodes[currNdx];

			if (curr.state.winner != IN_PROGRESS) {
				printf("Hmmm.. trying to expand a terminal\n");
			}

			int cc = 0;
			int pickMoves[9];
			for (int i=0; i < 9; i++) {

				// Is this a potential move that we haven't expanded yet?
				if (curr.state.square[i]==SQUARE_BLANK) {
					GameState nextMove = ApplyMove( curr.state, i );
					
					// Make a child node for it
					int cndx = MakeNode( app );
					MCTSNode &child = app.nodes[cndx];
					curr.childNdx[i] = cndx;
					child.state = nextMove;
					child.parentNdx = currNdx;
					child.moveNum = i;
					pickMoves[cc++] = cndx;
				}
			}
			
			// shouldn't be exanding
			assert( cc > 0 );

			// Simulateion step
			if (cc>0)
			{
				// Now simulate one of the newly added nodes
				int pick = GetRandomValue( 0, cc-1 );
				simNdx = curr.childNdx[pick];
			} else {
				// Use the current node?? I guess??
				simNdx = currNdx;
				printf("BAD??? Will sim current I guess\n");
			}
		} else {
			simNdx = currNdx;
		}
		
		// Rollout, sim the node		
		int numRuns = 25;
		MCTSNode &sim = app.nodes[ simNdx ];
		int wins = Rollout( app, sim.state, numRuns, to_play );

		// propogate the results back up to the root
		while (1)
		{
			MCTSNode &propNode = app.nodes[ simNdx ];
			
			if (propNode.state.to_move == to_play) {
				propNode.totalWins -= (float)wins / (float)numRuns;
			} else {
				propNode.totalWins += (float)wins / (float)numRuns;
			}

			propNode.totalVisits += 1.0f;
			if (simNdx==0) break;

			simNdx = propNode.parentNdx;
		}
	}

	// choose the best average child move
	float bestMoveVal = -999999.0f;
	int bestMoveNum = -1;
	int bestMoveNdx = 0;
	MCTSNode &root = app.nodes[rootNdx];
	for (int i=0; i < 9; i++) {
		if (root.childNdx[i] > 0) {
			MCTSNode &child = app.nodes[root.childNdx[i]];
			if (child.totalVisits > 0) {
				float val = child.totalWins / child.totalVisits;
				if (val > bestMoveVal) {
					bestMoveVal = val;
					bestMoveNum = child.moveNum;
					bestMoveNdx = root.childNdx[i];
				}
			} 
		}
	}
	
	app.nodes[bestMoveNdx].selected = true;

	if (app.numNodes >= NUM_MCTS_NODE-2) {
		printf("Warning: out of nodes\n");
	}

	//printf("iter %d Total Nodes %d Best Move num; %d\n", iter, app.numNodes, bestMoveNum );
	return bestMoveNum;
}

void TrainHistory( GameAppInfo &app )
{
	// First load the results 
	GameState &gameEnd = app.gameHistory[ app.currMove ];
	if (gameEnd.winner == WINNER_X) {
		app.outputs[0] = 1.0f;
		app.outputs[1] = 0.0f;
		app.outputs[2] = 0.0f; // tie
		app.winXcount++;
	} else if (gameEnd.winner == WINNER_O) {
		app.outputs[0] = 0.0f;
		app.outputs[1] = 1.0f;
		app.outputs[2] = 0.0f; // tie
		app.winOcount++;
	} else if (gameEnd.winner == TIE_GAME) {
		app.outputs[0] = 0.1f;
		app.outputs[1] = 0.1f;
		app.outputs[2] = 1.0f; // tie
		app.tieCount++;
	}

	// Now train it on every game state
	for (int i=0; i <= app.currMove; i++)
	{
		LoadWeights( app.gameHistory[i], app.inputs );
		genann_train( app.net, app.inputs, app.outputs, 0.5 );
		app.trainCount++;
	}
	app.gameCount++;
}

void TrainOneStep( GameAppInfo &app, float temperature )
{
	// Auto Play
	if (!app.gameHistory[app.currMove].winner)
	{
		// No winner yet, make the next move
		int aiMove = -1;
		//if ((app.currMove==0)||(UniformRandom() > temperature)) {

		// choose hte first five moves at random, then use the MCTS
		if (app.currMove < 5) {
			//aiMove = ChooseBestMove( app, app.gameHistory[app.currMove] );
			aiMove = TreeSearchMove( app, app.gameHistory[app.currMove] );
		} else {
			aiMove = ChooseRandomMove(  app.gameHistory[app.currMove] );
		}
		if (aiMove >= 0 ) {
			int nextMove = app.currMove+1;
			app.gameHistory[nextMove] = ApplyMove( app.gameHistory[app.currMove], aiMove );
			app.currMove = nextMove;
		}
	} else {
		// We have a winner, train the game
		TrainHistory( app );
		ResetGame( app );
	}
}

void ResetGame( GameAppInfo &app )
{
	app.currMove = 0;
	app.gameHistory[0] = (GameState){};
	app.gameHistory[0].to_move = SQUARE_X; // TODO: random
}


// -------------------------------------------
//  Utilities
// -------------------------------------------
float UniformRandom()
{
	return (float)rand() / (float)RAND_MAX;
}
