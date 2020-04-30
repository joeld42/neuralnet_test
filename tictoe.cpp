#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION            // Avoid including raygui implementation again

#include "genann.h"

// -------------------------------------------
// TODO: Move this out 

enum {
	MODE_PLAY,
	MODE_GALLERY
};

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


// -------------------------------------------

float UniformRandom()
{
	return (float)rand() / (float)RAND_MAX;
}

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

struct GameAppInfo 
{
	int mode = MODE_PLAY;
	int currMove;
    GameState gameHistory[10];
    GameAnalysis gameHistory_A[10];

    GameState gridBoards[100];
    GameAnalysis gridBoards_A[100];

    Rectangle screenRect[9];
    genann *net;
    double *inputs;
    double *outputs;
    int trainCount;
    int gameCount;
    int winXcount;
    int winOcount;
    int tieCount;

    GameAnalysis preview[9]; // preview for all squares
};


float min_float( float a, float b )
{
	if ( a < b) return a;
	else return b;
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

void LoadWeights( GameState &state, double *inputs )
{
	/* two weights per square */
	#if 0
	for (int i=0; i < 9; i++)
	{
		int ndx = i*2;
		float w0 = 0.0f;
		float w1 = 0.0f;
		if (state.square[i]==SQUARE_X) {
			w0 = 1.0f;
		} else if (state.square[i]==SQUARE_O) {
			w1 = 1.0f;
		}
		inputs[ndx+0] = w0;
		inputs[ndx+1] = w1;
	}
	#endif

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

// float NormOutput( float v )
// {
// 	if (v < -1.0) v = -1.0;
// 	if (v > 1.0) v = 1.0;
// 	return (v + 1.0) / 2.0f;
// }

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

Color GetWinColor( int winner, float x_win_chance, float o_win_chance, float tie_chance )
{
	Color win_c = WHITE;
	Color winCol_O = (Color){ 200,200,255,255 };
	Color winCol_X = (Color){ 255,200,200,255 };
	Color tieCol = (Color){ 200,255,200,255 };

	if (winner == WINNER_O) {
		win_c = winCol_O;
	} else if (winner == WINNER_X) {
		win_c = winCol_X;
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

		//win_c = (Color){  128 + (uint8_t)(x_win_chance*128), 200, 128 + (uint8_t)(x_win_chance*128), 255 };
		//win_c.r = 200 + (uint8_t)(x_win_chance*55.0f);		
		//win_c.b = 200 + (uint8_t)(o_win_chance*55.0f);
		//win_c.g = (win_c.r + win_c.b) / 2;
		//win_c.g = 0;
	}
	return win_c;
}

void DrawBoard( Rectangle outer_rect, GameState state, 
				float x_win_chance, float o_win_chance, float tie_chance,
				GameAppInfo *app, bool showPreview )
{
	// Find the best-fit square inside rect

	// DBG outer rect
	//DrawRectangleLines( outer_rect.x, outer_rect.y, outer_rect.width, outer_rect.height, RED );

	// Inner Rect
	float sz = min_float( outer_rect.width, outer_rect.height );
	float hmarg = (outer_rect.width - sz) / 2.0;
	float vmarg = (outer_rect.height - sz ) /2.0;
	Rectangle rect = {};
	rect.x = outer_rect.x + hmarg;
	rect.y = outer_rect.y + vmarg;
	rect.width = sz;
	rect.height = sz;

	Color win_c = GetWinColor( state.winner, x_win_chance, o_win_chance, tie_chance );
	//Color win_c = { (uint8_t)(x_win_chance*255), 255, (uint8_t)(o_win_chance*255), 255 };	

	DrawRectangle( rect.x, rect.y, rect.width, rect.height, win_c );

	float rectSz3 = rect.width / 3.0f;
	
	// Draw marks on board
	Vector2 wA, wB;
	for (int j=0; j < 3; j++) {
		for (int i=0; i < 3; i++) {
			int sqNdx = j*3 + i;
			float rad = 1.0;
			Rectangle subRect = { rect.x + rectSz3 * i,
								  rect.y + rectSz3 * j,
								  rectSz3, rectSz3 };

			if (app) {
				app->screenRect[sqNdx] = subRect;
			
				if (showPreview) {
					GameAnalysis &ga = app->preview[sqNdx];
					if ((ga.x_win_chance > 0.0) || (ga.o_win_chance > 0.0)) {
						Color sqCol = GetWinColor( state.winner, ga.x_win_chance, ga.o_win_chance, ga.tie_chance );
						DrawRectangleRec( subRect, sqCol );
					}
				}
			}

			rad = subRect.width * 0.5f;
			if (state.square[sqNdx] == SQUARE_O) {
				// draw O				
				for (int w=0; w < 3; w++) {
					DrawCircleLines( subRect.x + rad, subRect.y + rad, rad-(2+w), BLUE );
				}

			} else if (state.square[sqNdx] == SQUARE_X) {
				// Draw X
				DrawLineEx( (Vector2){subRect.x + 4, subRect.y + subRect.height - 4}, 
							(Vector2){subRect.x + subRect.width-4, subRect.y + 4}, 3, RED );
				DrawLineEx( (Vector2){subRect.x + 4, subRect.y + 4}, 
					(Vector2){ subRect.x + subRect.width-4, subRect.y + subRect.height - 4}, 3, RED );
				
			} else {
				// It's a blank, don't draw anything
			}
			
			if (state.win0==sqNdx) {
				wA = (Vector2){subRect.x + rad, subRect.y + rad};
			} else if (state.win2==sqNdx) {
				wB = (Vector2){subRect.x + rad, subRect.y + rad};
			}
		}
	}
	
	// draw tictactoe lines
	float lw = (rect.width > 80) ? 4 : 2;	
	for (int i=1; i < 3; i++)
	{
		DrawLineEx( (Vector2){rect.x, rect.y + i * rectSz3}, (Vector2){rect.x + rect.width, rect.y + i * rectSz3}, lw, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR))  );
		DrawLineEx( (Vector2){rect.x + i * rectSz3, rect.y}, (Vector2){rect.x + i * rectSz3, rect.y + rect.width}, lw, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR))  );
	}

	if ((state.winner) && (state.winner != TIE_GAME)) 
	{
		Color winCol;
		if (state.winner == WINNER_O) {
			winCol = BLUE;
		} else if (state.winner == WINNER_X) {
			winCol = RED;
		}
		DrawRectangleLines( rect.x, rect.y, rect.width, rect.height, winCol );
		winCol.a = 100;
		DrawLineEx( wA, wB, 8, winCol );
	}
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

GameState RandomBoard()
{
	GameState game = {};
	int moves[9] = { 0,1,2,3,4,5,6,7,8 };
	for (int i=8; i > 0; i--) {
		int n = GetRandomValue( 0, i );
		int t = moves[n];
		moves[n] = moves[i];
		moves[i] = t;
	}	

	int numMoves = GetRandomValue( 0, 8 );
	for (int i=0; i < numMoves; i++ )
	{
		game = ApplyMove( game, moves[i] );

		// Check for end game and bail early
		game = CheckWinner( game );
		if (game.winner != SQUARE_BLANK) {
			break;
		}
	}	

	return game;

}

void ResetGallery( GameAppInfo &app )
{
	for (int i=0; i < 100; i++) {
		app.gridBoards[i] = RandomBoard();
		app.gridBoards_A[i] = AnalyzeGame( app, app.gridBoards[i] );
	}
}

void ResetGame( GameAppInfo &app )
{
	app.currMove = 0;
	app.gameHistory[0] = (GameState){};
	app.gameHistory[0].to_move = SQUARE_X; // TODO: random
}

int ChooseRandomMove( GameAppInfo &app, const GameState &game )
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

int PreviewBestMove( GameAppInfo &app, const GameState &game )
{
	if (game.winner) {
		printf("preview best: game already over\n");
		return -1;
	}

	int activePlayer = game.to_move;
	int bestMove = -1;
	float bestMoveDiff = -1.0f;

	for (int i=0; i < 9; i++) {
		if (game.square[i] == SQUARE_BLANK) {
			GameState evalGame = ApplyMove(game,  i );
			GameAnalysis ga = AnalyzeGame( app, evalGame );
			

			app.preview[i] = ga;
			
			/*
			float my_win, other_win;
			if (game.to_move == SQUARE_X) {
				my_win = ga.x_win_chance;
				other_win = ga.o_win_chance;
			}
			else if (game.to_move == SQUARE_O) {
				my_win = ga.o_win_chance;
				other_win = ga.x_win_chance;
			}

			float moveDiff = my_win - other_win;
			if (moveDiff > bestMoveDiff) {				
				bestMoveDiff = moveDiff;					
				bestMove = i;
			}
			*/

		} else {
			app.preview[i] = (GameAnalysis){};
		}
	}

	return bestMove;
}



int ChooseBestMove( GameAppInfo &app, const GameState &game )
{
	if (game.winner) {
		return -1;
	}

	int activePlayer = game.to_move;
	int bestMove = -1;
	float bestMoveDiff = -1.0f;

	for (int i=0; i < 9; i++) {
		if (game.square[i] == SQUARE_BLANK) {
			GameState evalGame = ApplyMove(game,  i );
			GameAnalysis ga = AnalyzeGame( app, evalGame );
			
			float my_win, other_win;
			if (game.to_move == SQUARE_X) {
				my_win = ga.x_win_chance;
				other_win = ga.o_win_chance;
			}
			else if (game.to_move == SQUARE_O) {
				my_win = ga.o_win_chance;
				other_win = ga.x_win_chance;
			}

			float moveDiff = my_win - other_win;
			if (moveDiff > bestMoveDiff) {				
				bestMoveDiff = moveDiff;					
				bestMove = i;
			}
		}
	}

	return bestMove;
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
		if ((app.currMove==0)||(UniformRandom() > temperature)) {
			aiMove = ChooseBestMove( app, app.gameHistory[app.currMove] );
		} else {
			aiMove = ChooseRandomMove( app, app.gameHistory[app.currMove] );
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

int main()
{
    // Initialization
    //---------------------------------------------------------------------------------------
    int screenWidth = 690;
    int screenHeight = 560;

     bool exitWindow = false;
    
    InitWindow(screenWidth, screenHeight, "AlphaToe -- tic tac network");
    SetExitKey(0);
    SetTargetFPS(30);

    GameAppInfo app = {};

    app.net = genann_init( 11, 3, 64, 3 );
    app.inputs = (double*)malloc( sizeof(double) * 11 );
    app.outputs = (double*)malloc( sizeof(double) * 3 );

    genann_randomize( app.net );

	ResetGame( app );    
    //ResetGallery( app );

    int autoTrain = 0;
    float temperature = 0.5;

    // Main game loop
    while (!exitWindow)    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        exitWindow = WindowShouldClose();
       
         BeginDrawing();

		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		if (IsKeyPressed( KEY_TAB )) {
			if (app.mode== MODE_PLAY) {
				app.mode = MODE_GALLERY;
			} else {
				app.mode = MODE_PLAY;
			}
		}

		 //DrawText( "Hello There", 200, 80, 20, RED);
		if (app.mode==MODE_PLAY) {

			if (IsKeyPressed(KEY_R)) {
				// reset game
				app.currMove = 0;
				app.gameHistory[0] = (GameState){};
				app.gameHistory[0].to_move = SQUARE_X;

				PreviewBestMove( app, app.gameHistory[app.currMove] );
			}

			if (IsKeyPressed(KEY_A) && (!autoTrain) ) {
				// Do AI move
				int aiMove = ChooseBestMove( app, app.gameHistory[app.currMove] );
				if (aiMove < 0) {
					printf("No valid moves.\n");
				} else {
					printf("AI move on square %d\n", aiMove );
					int nextMove = app.currMove+1;
					app.gameHistory[nextMove] = ApplyMove( app.gameHistory[app.currMove], aiMove );
					app.currMove = nextMove;

					PreviewBestMove( app, app.gameHistory[app.currMove] );
				}
			}

			if (IsKeyPressed(KEY_T)) {
				if (!app.gameHistory[app.currMove].winner) {
					printf("Can't train, game is not finished.\n");
				} else {
					printf("Train on this game.\n");
					TrainHistory( app );					
				}
			}

			if (IsKeyPressed(KEY_M)) {
				if (autoTrain) {
					autoTrain = 0;
				} else {
					autoTrain = 1;
				}
			}

			if (IsKeyPressed(KEY_N)) {
				if (autoTrain) {
					autoTrain = 0;
				} else {
					autoTrain = 1000;
				}
			}

			if (IsKeyPressed(KEY_B)) {
				autoTrain = 100000;
			}

			if (IsKeyPressed(KEY_P)) {
				// print weights
				printf("Inputs: ");
				for (int i=0; i < 11; i++) {
					printf("%3.2lf ", app.inputs[i] );
				}
				printf("\nOutputs: %3.2lf %3.2lf\n", app.outputs[0], app.outputs[1] );
				for (int i=0; i < 9; i++) {
					printf("Square %d: X %3.2f, O %3.2f (sum %3.2f)\n", 
						i, 
						app.preview[i].x_win_chance, 
						app.preview[i].o_win_chance,

						app.preview[i].x_win_chance + app.preview[i].o_win_chance  );
				}
			}

			if (autoTrain)
			{
				for (int s=0; s < autoTrain; s++)
				{
					TrainOneStep( app, temperature );
					if (((s % 10000) == 0) && (s>0)) {
						printf("train %d\n", s );
					}
				}

				// Don't train continously for 
				if (autoTrain > 10000) {
					autoTrain = 0;
					printf("phew\n");
				}

			} else {
				// Manual play mode

				// If the game is not over, allow moves
				if (!app.gameHistory[app.currMove].winner)
				{
					if (IsMouseButtonPressed(0)) {				
						// check if we're in a game space
						Vector2 mousePos = GetMousePosition();
						Rectangle mouseRect = (Rectangle){ mousePos.x, mousePos.y, 1, 1 };
						for (int i=0; i < 9; i++) {						
							if (app.gameHistory[app.currMove].square[i]!=SQUARE_BLANK) {
								// Square is occupied
								printf("Square is occupied.\n");
							} else if (CheckCollisionRecs( app.screenRect[i], mouseRect )) {

								printf("Clicked on square %d\n", i );
								int nextMove = app.currMove+1;
								app.gameHistory[nextMove] = ApplyMove( app.gameHistory[app.currMove], i );
								app.currMove = nextMove;

								PreviewBestMove( app, app.gameHistory[app.currMove] );

								break;
							}
						}				
					}
				}
			}

			Rectangle gameRect = { 100, 100, 500, 300 };

			GameAnalysis analysis = AnalyzeGame( app, app.gameHistory[ app.currMove] );

			int promptx = 150;
			int prompty = 50;
			GameState &game = app.gameHistory[ app.currMove];
			if (game.winner == TIE_GAME) {
				DrawText( "Tie Game", promptx, prompty, 20, ORANGE );
			} else if (game.winner == SQUARE_X) {
				DrawText( "Winner is X", promptx, prompty, 20, RED);
			} else if (game.winner == SQUARE_O) {
				DrawText( "Winner is O", promptx, prompty, 20, BLUE);
			} else if (game.to_move == SQUARE_X) {
				DrawText( "X to Move", promptx, prompty, 20, RED);
			} else if (game.to_move == SQUARE_O) {
				DrawText( "O to Move", promptx, prompty, 20, BLUE);
			}
			char buff[20];
			sprintf( buff, "%d trains (%d games)", app.trainCount, app.gameCount );
			DrawText( buff, promptx, 400, 20, ORANGE );

			sprintf( buff, "X %d / O %d / Tie %d\n",
				app.winXcount,
				app.winOcount,
				app.tieCount );
			DrawText( buff, promptx, 422, 20, ORANGE );

			DrawBoard( gameRect, game, 
				analysis.x_win_chance, analysis.o_win_chance, analysis.tie_chance,
				&app, true  );

			temperature = GuiSlider((Rectangle){ 150, 450, 165, 20 }, 
				"Temperature", TextFormat("%0.3f", temperature) , temperature, 0.0, 1.0);

			DrawText( TextFormat("X: %3.2f O: %3.2f Tie: %3.2f", 
				analysis.x_win_chance, analysis.o_win_chance, analysis.tie_chance ),
				150, 480, 20, BLUE );

			// Show history too
			for (int i=0; i <= app.currMove; i++) {
				Rectangle boardRect = { 550, 10, 40, 40 };
				boardRect.y += i*45;
				DrawBoard( boardRect, app.gameHistory[i], 0.5f, 0.5f, 0.0f, NULL, false );
			}




		} else if (app.mode==MODE_GALLERY) {

			if (IsKeyPressed(KEY_R)) {
				// reset game
				ResetGallery( app );
			}
			
			for (int j=0; j < 10; j++ ) {
				for (int i=0; i < 10; i++) {

					int ndx = j*10+i;
					DrawBoard ( 
						(Rectangle){ (690.f-560.f)*0.5f+ 10.f + (56.f*i), 5 + (56.f*j), 50, 50 }, 
						app.gridBoards[ndx], 
						app.gridBoards_A[ndx].x_win_chance,
						app.gridBoards_A[ndx].o_win_chance, 
						app.gridBoards_A[ndx].tie_chance, 
						NULL, false );
				}
			}
		}
 
        EndDrawing();
	}

 	CloseWindow();    
}