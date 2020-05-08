#include <assert.h>
#include <math.h>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION            // Avoid including raygui implementation again

#include "genann.h"

#include "tk_evalgame.h"

// TODO: move these somewhere
Rectangle g_screenRect[9];

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


struct DbgTreeRect
{
	Rectangle rect;
	int nodeNdx;
};



float min_float( float a, float b )
{
	if ( a < b) return a;
	else return b;
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
				g_screenRect[sqNdx] = subRect;
			
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

    app.nodes = (MCTSNode*)malloc(sizeof(MCTSNode) * NUM_MCTS_NODE );
    app.numNodes = 0;
    app.simMode = NET_EVAL;

    genann_randomize( app.net );

	ResetGame( app );    
    //ResetGallery( app );

    int autoTrain = 0;
    float temperature = 0.0;

    // Main game loop
    while (!exitWindow)    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        exitWindow = WindowShouldClose();
       
         BeginDrawing();

		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		if (IsKeyPressed( KEY_TAB )) {

			app.mode++;
			if (app.mode == NUM_MODES) {
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

			if (IsKeyPressed(KEY_S)) {
				// Tree search
				printf("Tree search....\n");
				int aiMove = TreeSearchMove( app, app.gameHistory[app.currMove] );
				if (aiMove < 0) {
					printf("No valid moves.\n");
				} else {
					printf("AI move on square %d\n", aiMove );
					int nextMove = app.currMove+1;
					app.gameHistory[nextMove] = ApplyMove( app.gameHistory[app.currMove], aiMove );
					app.currMove = nextMove;
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
					autoTrain = 500;
				}
			}

			if (IsKeyPressed(KEY_B)) {
				autoTrain = 1000;
			}

			if (IsKeyPressed(KEY_L)) {
				app.simMode++;
				if (app.simMode == NUM_SIM_MODES) {
					app.simMode = 0;
				}
			}

			if (IsKeyPressed(KEY_T)) {
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
					if (((s % 100) == 0) && (s>0)) {
						printf("train %d\n", s );
					}
				}

				// Don't train continously for 
				if (autoTrain > 999) {
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
							} else if (CheckCollisionRecs( g_screenRect[i], mouseRect )) {

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

			DrawText( app.simMode==SIM_RANDOM?"Rand":"Eval", 600, 10, 20, BLACK);

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
		
		} else if (app.mode==MODE_TREE) {

			for (int ndx=0; ndx < app.numNodes; ndx++) {
				MCTSNode &curr = app.nodes[ndx];
				
				if (ndx != 0) {
					curr.level = app.nodes[curr.parentNdx].level + 1;					
				}			
			}

			int rowHite = 85;
			app.nodes[0].expanded = true;
			for (int level=0; level < 6; level++) {
				float xval = 10.0f;

				for (int ndx=0; ndx < app.numNodes; ndx++) {
					MCTSNode &curr = app.nodes[ndx];
					if (curr.level != level) continue;

					// DBG skip unexplored nodes
					if (curr.totalVisits < 3) continue;

					if (!app.nodes[curr.parentNdx].expanded) continue;

					Rectangle rect;
					rect.x = xval;
					rect.y = 10 + rowHite * level;
					rect.width = 50;
					rect.height = 50;

					Vector2 mousePos = GetMousePosition();
					Rectangle mouseRect = (Rectangle){ mousePos.x, mousePos.y, 1, 1 };
					if (IsMouseButtonPressed(0) && CheckCollisionRecs( rect, mouseRect )) {
						curr.expanded = !curr.expanded;
					}

					float ucb = NodeValUCB1( app, curr );

					curr.xval = rect.x + (rect.width/2.0f);
					if (level >0) {
						float lasty = 10 + rowHite * (level-1);
						MCTSNode &parent = app.nodes[curr.parentNdx];
						DrawLineEx( (Vector2){ rect.x + rect.width/2, rect.y + rect.height/2 },
									(Vector2){ (float)(parent.xval), lasty + rect.height/2 },
									2, GRAY );
					}


					if (curr.selected)
					{
						DrawRectangleRec( rect, GREEN );
					} else {
						DrawRectangleRec( rect, ORANGE );
					}

					Rectangle boardRect = rect;
					boardRect.y += 15;
					boardRect.height -= 16;
					DrawBoard( boardRect, curr.state, 0.5f, 0.5f, 0.0f, NULL, false );

					if (curr.totalVisits==0) {
						DrawText( "UnVis",rect.x + 2, rect.y+2, 12, BLACK );	
					} else {
						DrawText( TextFormat("%d", (int)curr.totalVisits ),				
							rect.x+2, rect.y+2, 12, BLACK );

						DrawText( TextFormat("%d/%d", 
							(int)curr.totalWins, (int)curr.totalVisits),
							rect.x+2, rect.y+rect.height, 12, BLACK );

						DrawText( TextFormat("(%3.1f) %3.2f", 
							curr.totalWins / curr.totalVisits, ucb ),
							rect.x+2, rect.y+rect.height+13, 12, BLACK );
					}
					// if (ucb > 9999.0) {							
					// 	DrawText( "INF",rect.x + 2, rect.y+2, 12, BLACK );
					// } else {										
					// 	DrawText( TextFormat("%3.1f", ucb ),				
					// 		rect.x+2, rect.y+2, 12, BLACK );
					// }
					

					//xval += rect.width + 10;
					xval += rect.width * 1.5;
					// if (xval > 500) break;
				}
			}


		}
 
        EndDrawing();
	}

 	CloseWindow();    
}