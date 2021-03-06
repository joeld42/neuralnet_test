#include "tk_evalgame.h"

#include <stdlib.h>
#include <math.h>

#include "rnd.h"

#define MAX_BRANCH (20)

// TODO borrowing this from raylib -- reimplement
extern "C" {
int GetRandomValue( int minVal, int maxVal );
}

void CallGeneratePossibleMoves( GameAppInfo &app, const GameState &game, GameStateArray &moves );

// TODO: make this support multi-player, this should be generic enough
GameAnalysis AnalyzeGame( GameAppInfo &app, GameState game )
{
	// random for now
	GameAnalysis result;

	// Analysis pretty easy if somene won
	if (game.gameResult == RESULT_WINNER)
	{
		if (game.winner == PLAYER_1) {
			result.plr[0].win_chance = 1.0;
			result.plr[1].win_chance = 0.0;
			result.tie_chance = 0.0;
		} else if (game.winner == PLAYER_2) {
			result.plr[0].win_chance = 0.0;
			result.plr[1].win_chance = 1.0;
			result.tie_chance = 0.0;
		}
	} else if (game.gameResult == RESULT_TIE_GAME ) {

		result.tie_chance = 1.0;
	} else {
        assert (app.info.gameFunc_LoadWeights != NULL);
		app.info.gameFunc_LoadWeights( game, app.inputs );
		const double *outs = genann_run( app.net, app.inputs );

		// also copy to app
		app.outputs[0] = outs[0];
		app.outputs[1] = outs[1];
		app.outputs[2] = outs[2];

		// Normalize and store 
		float a = outs[0];
		float b = outs[1];
		float c = outs[2];
		float total = a+b+c;
		if (total < 0.001)
		{
			total = 1.0f;
		}
		result.plr[0].win_chance = a / total;
		result.plr[1].win_chance = b / total;
		result.tie_chance = c / total;
	}
	return result;
}

// This assumes all the nodes get zeroed out
int MakeNode( GameAppInfo &app )
{
    assert( app.numNodes < NUM_MCTS_NODE );
    return app.numNodes++;
}

void AddChildNode( GameAppInfo &app, MCTSNode &curr, int childNdx )
{
    // Add a child if we don't have one
    if (curr.leftChildNdx == 0) {
        curr.leftChildNdx = childNdx;
    } else {
        int ndx = curr.leftChildNdx;
        
        while (1) {
            MCTSNode &currChild = app.nodes[ndx];
            ndx = currChild.rightSiblingNdx;
            if (ndx == 0) {
                currChild.rightSiblingNdx = childNdx;
                break;
            }
        };
    }
    
    // TODO: Add parent index here
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

int RolloutOnce( GameAppInfo &app, GameState state, int player )
{
	GameState curr = state;
    GameStateArray possibleMoves = {};
	while (!curr.gameResult) {
		//int randMove = ChooseRandomMove( curr );
		//curr = ApplyMove( curr, randMove );
        
        CallGeneratePossibleMoves( app, curr, possibleMoves );
        curr = ChooseRandomMove( app, possibleMoves );
	}
    FreeArray( &possibleMoves );
    
    if (curr.gameResult == RESULT_TIE_GAME)
    {
        return 0;
    } else {
        assert( curr.gameResult == RESULT_WINNER );
        if (curr.winner == player) {
            return 1;
        } else {
            return -1;
        }
    }
}

int Rollout( GameAppInfo &app, GameState state, int runs, int player )
{		
	int result = 0;
	if ((state.gameResult == RESULT_WINNER) && (state.winner == player)) {
		// If this is a winning move for player, count it as all wins		
		result = runs;
	} else if (state.gameResult == RESULT_TIE_GAME) {
		result = 0;
	} else if (state.gameResult != RESULT_IN_PROGRESS) {
		// other player won
		result = -runs;
	}
    else
	{
        // state is still an in-progress game, simulate it until we get an outcome
		if (app.simMode == SIM_RANDOM)
		{
			// Otherwise
			int wins = 0;
			for (int i=0; i < runs; i++) {
				wins += RolloutOnce( app, state, player );
			}
			result = wins;

		} else {
			// Use the net to do the sim
			GameAnalysis ga = AnalyzeGame( app, state );
			int wins;
			if (player==PLAYER_1) {
				wins = (ga.plr[0].win_chance * runs) + (ga.plr[1].win_chance * -runs);
			} else {
				wins = (ga.plr[0].win_chance * -runs) + (ga.plr[1].win_chance * runs);
			}
			result = wins;
		}
	}

	//printf("Rollout: %d/%d wins\n", wins, runs );
	return result;
}

GameState ChooseRandomMove( GameAppInfo &app, GameStateArray &possibleMoves )
{
    
    assert( possibleMoves.size > 0);
    size_t moveNdx = GetRandomValue( 0, possibleMoves.size - 1);
    GameState result = possibleMoves.data[moveNdx];
    
//    // Remove the chosen move from the list
//    possibleMoves.data[moveNdx] = possibleMoves.data[possibleMoves.size - 1];
//    possibleMoves.size -= 1;
//
    return result;
}


void CallGeneratePossibleMoves( GameAppInfo &app, const GameState &game, GameStateArray &moves )
{
    ClearArray( &moves );
    
    if (game.gameResult != RESULT_IN_PROGRESS) {
        return;
    }
    
    // Call the client generator
    assert( app.info.gameFunc_GeneratePossibleMoves != NULL );
    app.info.gameFunc_GeneratePossibleMoves( game, moves );
}

// Helper that generates the possible moves and picks one at random
// Not efficient if you are choosing multiple moves
GameState ChooseRandomMoveSimple( GameAppInfo &app, const GameState &prevState )
{
    GameState result = {};
    GameStateArray possibleMoves= {};
    CallGeneratePossibleMoves( app, prevState, possibleMoves );
    if (possibleMoves.size > 0) {
        result = ChooseRandomMove( app, possibleMoves );
    }
    FreeArray( &possibleMoves );
    return result;
}


GameState TreeSearchMove( GameAppInfo &app, const GameState &game )
{    
	if (game.gameResult != RESULT_IN_PROGRESS) {
        return game;
	}

	// initiaze the search
	memset( app.nodes, 0, sizeof(MCTSNode) * NUM_MCTS_NODE );
	app.numNodes = 0;

	int to_play = game.to_move;

	// start with the current state
	int rootNdx = MakeNode( app );
	app.nodes[rootNdx].state = game;

    // Temporary to hold potential moves
    GameStateArray potentialMoves = {};
    
	int iter = 5000; 
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

			if (curr.state.gameResult == RESULT_IN_PROGRESS)
			{
				// find the child with the best UCB 
				isLeaf = false;
				float bestScore = -999999.0f;
				int bestChildNdx = 0;
                
                if (curr.leftChildNdx==0) {
                    isLeaf = true;
                } else {
                    int currChildNdx = curr.leftChildNdx;
                    while (currChildNdx > 0) {
                        MCTSNode &currChild = app.nodes[ currChildNdx ];
                        float ucb = NodeValUCB1( app, currChild );
                        if (ucb > bestScore) {
                            bestScore = ucb;
                            bestChildNdx = currChildNdx;
                        }
                        currChildNdx = currChild.rightSiblingNdx;
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
        int pickMoves[MAX_BRANCH];
		int simNdx = 0;
		if ((app.nodes[ currNdx ].totalVisits > 0) && (app.nodes[ currNdx ].state.gameResult == RESULT_IN_PROGRESS))
		{
			// Expand the possible moves from best child
			MCTSNode &curr = app.nodes[currNdx];
//            printf("Expanding node %d\n", currNdx );

			if (curr.state.gameResult != RESULT_IN_PROGRESS) {
				printf("Hmmm.. trying to expand a terminal\n");
			}
            
            // Haven't expanded this node yet, generate the potential move list
            CallGeneratePossibleMoves( app, curr.state, potentialMoves );

            // truncate the potential moves if too many
            if (potentialMoves.size >= MAX_BRANCH) {
                printf("WARN: too much branching, increase MAX_BRANCH\n");
                potentialMoves.size = MAX_BRANCH-1;
            }
            
            for (int i=0; i < potentialMoves.size; i++) {
                GameState nextMove = potentialMoves.data[i];
                
                // Make a child node for it
                int cndx = MakeNode( app );
                MCTSNode &child = app.nodes[cndx];
                child.state = nextMove;
                child.parentNdx = currNdx;
                child.moveNum = i;
                pickMoves[i] = cndx;
                
                AddChildNode( app, curr, cndx );
            }
            
            if (potentialMoves.size > 0) {
                // Now simulate one of the newly added nodes
                int pick = GetRandomValue( 0, potentialMoves.size-1 );
                simNdx = pickMoves[pick];
            } else {
               // Use the current node?? I guess??
                simNdx = currNdx;
                printf("BAD??? Will sim current I guess\n");
            }
		} else {
            //printf("Simulating node %d\n", currNdx );
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
    
    int childNdx = root.leftChildNdx;
	while (childNdx != 0) {
        MCTSNode &child = app.nodes[childNdx];
        if (child.totalVisits > 0) {
            float val = child.totalWins / child.totalVisits;
            if (val > bestMoveVal) {
                bestMoveVal = val;
                bestMoveNum = child.moveNum;
                bestMoveNdx = childNdx;
            }
        }
        childNdx = child.rightSiblingNdx;
	}
	
	app.nodes[bestMoveNdx].selected = true;

	if (app.numNodes >= NUM_MCTS_NODE-2) {
		printf("Warning: out of nodes\n");
	}

	//printf("iter %d Total Nodes %d Best Move num; %d\n", iter, app.numNodes, bestMoveNum );
    return app.nodes[bestMoveNdx].state;
}

// ZARDOZ: clean up winner stuff. Might not want to train on
// move history anyways, but instead on the result of TreeSearch
void TrainHistory( GameAppInfo &app )
{
	// First load the results 
	GameState &gameEnd = app.gameHistory[ app.currMove ];
    
    assert( gameEnd.gameResult != RESULT_IN_PROGRESS );
    
	if ((gameEnd.gameResult == RESULT_WINNER) && (gameEnd.winner == PLAYER_1)) {
		app.outputs[0] = 1.0f;
		app.outputs[1] = 0.0f;
		app.outputs[2] = 0.0f; // tie
		app.winXcount++;
    } else if ((gameEnd.gameResult == RESULT_WINNER) && (gameEnd.winner == PLAYER_2)) {
		app.outputs[0] = 0.0f;
		app.outputs[1] = 1.0f;
		app.outputs[2] = 0.0f; // tie
		app.winOcount++;
	} else if (gameEnd.gameResult == RESULT_TIE_GAME) {
		app.outputs[0] = 0.1f;
		app.outputs[1] = 0.1f;
		app.outputs[2] = 1.0f; // tie
		app.tieCount++;
	}

	// Now train it on every game state
	for (int i=0; i <= app.currMove; i++)
	{
        assert( app.info.gameFunc_LoadWeights != NULL );
		app.info.gameFunc_LoadWeights( app.gameHistory[i], app.inputs );
		genann_train( app.net, app.inputs, app.outputs, 0.5 );
		app.trainCount++;
	}
	app.gameCount++;
}

// TODO: clean up winner stuff. Might not want to train on
// move history anyways, but instead on the result of TreeSearch
void TrainOneStep( GameAppInfo &app, float temperature )
{
	// Auto Play
	if (!app.gameHistory[app.currMove].gameResult)
	{
		// No winner yet, make the next move
        GameState aiMove;
        
		//if ((app.currMove==0)||(UniformRandom() > temperature)) {

		// choose hte first five moves at random, then use the MCTS
		if (app.currMove < 5) {
			//aiMove = ChooseBestMove( app, app.gameHistory[app.currMove] );
			aiMove = TreeSearchMove( app, app.gameHistory[app.currMove] );
		} else {
			aiMove = ChooseRandomMoveSimple( app, app.gameHistory[app.currMove] );
		}
		
        int nextMove = app.currMove+1;
        app.gameHistory[nextMove] = aiMove;
        app.currMove = nextMove;
		
	} else {
		// We have a winner, train the game
		TrainHistory( app );
		ResetGame( app );
	}
}

void SaveTrainingData( GameAppInfo &app, char *filename )
{
    FILE *fp = fopen( filename, "wt" );
    if (!fp) {
        printf("ERROR: save failed, could not open file\n" );
        return;
    }
    
    // Write some extra data on top of the genann training set
    fprintf(fp, "%d %d\n", app.trainCount, app.gameCount );
    
    genann_write( app.net, fp );
    fclose(fp);
    
    printf("Saved training data %s\n", filename );
}

void LoadTrainingData( GameAppInfo &app, char *filename )
{
    FILE *fp = fopen( filename, "rt" );
    if (!fp) {
        printf("Could not load training data\n" );
        return;
    }
    
    if (app.net) {
        genann_free( app.net );
        app.net = NULL;
    }
    
    // read our extra data
    fscanf(fp, "%d %d\n", &(app.trainCount), &(app.gameCount) );
    
    app.net = genann_read( fp );
    fclose(fp);
    
    printf("Loaded training data %s\n", filename );
    
    
    
//    app.net = genann_init( app.info.net_inputs,
//                          app.info.net_hidden_layers,
//                          app.info.net_hidden_layer_size,
//                          3 ); // 3 outs: win chance, lose chance, tie chance
    
    //int inputs, hidden_layers, hidden, outputs;
    if ( (app.net->inputs != app.info.net_inputs) ||
         (app.net->hidden_layers != app.info.net_hidden_layers) ||
         (app.net->hidden != app.info.net_hidden_layer_size) ||
        (app.net->outputs != 3) ) {
        printf("LOAD Error: loaded net sizes do not match expected sizes, not using it.\n");
        genann_free( app.net );
        app.net = NULL;
    }
}


void ResetGame( GameAppInfo &app )
{
	app.currMove = 0;
	app.gameHistory[0] = (GameState){};
	app.gameHistory[0].to_move = PLAYER_1; // TODO: random starting player
}

// Initializes the game state. Expects some stuff to be filled out (todo: document/assert this)
void GameInit( GameAppInfo &app )
{
    app.net = genann_init( app.info.net_inputs,
                          app.info.net_hidden_layers,
                          app.info.net_hidden_layer_size,
                          3 ); // 3 outs: win chance, lose chance, tie chance
    
    app.inputs = (double*)malloc( sizeof(double) * 11 );
    app.outputs = (double*)malloc( sizeof(double) * 3 );

    app.nodes = (MCTSNode*)malloc(sizeof(MCTSNode) * NUM_MCTS_NODE );
    app.numNodes = 0;
    app.simMode = NET_EVAL;

    genann_randomize( app.net );

    ResetGame( app );
}

// -------------------------------------------
//  Utilities
// -------------------------------------------
// TODO: use a different random state thing.
float UniformRandom()
{
	return (float)rand() / (float)RAND_MAX;
}


void PushGameState( GameStateArray *games, GameState state )
{
    // Need to grow array?
    if (games->size >= games->capacity) {
        // start with size of 16
        size_t newSize = games->capacity;
        if (newSize == 0) {
            newSize = 16;
        } else {
            // TODO: might want to grow linearly after some point here
            newSize = newSize * 2;
        }
        games->data = (GameState*)realloc( games->data, newSize * sizeof( GameState) );
        games->capacity = newSize;
    }
    
    size_t newIndex = games->size;
    games->size++;
    games->data[ newIndex ] = state;
}

void ClearArray( GameStateArray *games )
{
    if (games->capacity > 0) {
        memset( games->data, 0, sizeof(GameState)*games->capacity );
    }
    games->size = 0;
}

void FreeArray( GameStateArray *games )
{
    free( games->data );
    games->capacity = 0;
    games->size = 0;
    games->data = NULL;
}
