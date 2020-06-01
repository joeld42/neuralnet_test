#include <assert.h>
#include <math.h>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION            // Avoid including raygui implementation again

#include "genann.h"

#include "tk_evalgame.h"
#include "tk_game_ui.h"

#define  RND_IMPLEMENTATION
#include "rnd.h"

void GeneratePossibleMoves_TicToe( const GameState &game, GameStateArray &moves );
GameState ApplyMove( GameState prevState, int moveLoc );

GameAnalysis NEUTRAL_ANALYSIS = {};

bool _CheckThree( GameState &game, int a, int b, int c )
{
    if (( game.gg.square[a] != SQUARE_BLANK) &&
        ( game.gg.square[a] == game.gg.square[b]) &&
        ( game.gg.square[b] == game.gg.square[c]) ) {
        game.gameResult = RESULT_WINNER;
        game.winner = game.gg.square[a];
        game.gg.win0 = a;
        game.gg.win2 = c;
        return true;
    }

    return false;
}

//
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
        if (game.gg.square[i] == SQUARE_BLANK) {
            isTied = false;
            break;
        }
    }
    if (isTied) {
        result.gameResult = RESULT_TIE_GAME;
        //result.winner = TIE_GAME;
    }

    return result;
}


void DrawBoard( Rectangle outer_rect, GameState state, 
				GameAnalysis ga, GameUIStuff *gameUI,
               bool showPreview )
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

	Color win_c = GetWinColor( state.gameResult, state.winner, ga.plr[0].win_chance, ga.plr[1].win_chance, ga.tie_chance );
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

//			if (gameUI) {
//				gameUI->screenRect[sqNdx] = subRect;
			
//				if (showPreview) {
//					GameAnalysis &ga = gameUI->preview[sqNdx];
//					if ((ga.plr[0].win_chance > 0.0) || (ga.plr[1].win_chance > 0.0)) {
//						Color sqCol = GetWinColor( state.gameResult, state.winner,
//                                    ga.plr[0].win_chance, ga.plr[1].win_chance, ga.tie_chance );
//						DrawRectangleRec( subRect, sqCol );
//					}
//				}
//			}

			rad = subRect.width * 0.5f;
			if (state.gg.square[sqNdx] == SQUARE_O) {
				// draw O				
				for (int w=0; w < 3; w++) {
					DrawCircleLines( subRect.x + rad, subRect.y + rad, rad-(2+w), BLUE );
				}

			} else if (state.gg.square[sqNdx] == SQUARE_X) {
				// Draw X
				DrawLineEx( (Vector2){subRect.x + 4, subRect.y + subRect.height - 4}, 
							(Vector2){subRect.x + subRect.width-4, subRect.y + 4}, 3, RED );
				DrawLineEx( (Vector2){subRect.x + 4, subRect.y + 4}, 
					(Vector2){ subRect.x + subRect.width-4, subRect.y + subRect.height - 4}, 3, RED );
				
			} else {
				// It's a blank, don't draw anything
			}
			
			if (state.gg.win0==sqNdx) {
				wA = (Vector2){subRect.x + rad, subRect.y + rad};
			} else if (state.gg.win2==sqNdx) {
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

	if (state.gameResult == RESULT_WINNER)
	{
		Color winCol;
		if (state.winner == PLAYER_2) {
			winCol = BLUE;
		} else if (state.winner == PLAYER_1) {
			winCol = RED;
		}
		DrawRectangleLines( rect.x, rect.y, rect.width, rect.height, winCol );
		winCol.a = 100;
		DrawLineEx( wA, wB, 8, winCol );
	}
}


GameState RandomBoard( GameAppInfo &app )
{
	GameState game = {};

	int numMoves = GetRandomValue( 0, 8 );
	for (int i=0; i < numMoves; i++ )
	{
		//game = ApplyMove( game, moves[i] );
        game = ChooseRandomMoveSimple( app, game );

		// Check for end game and bail early
		game = CheckWinner( game );
		if (game.gameResult != RESULT_IN_PROGRESS) {
			break;
		}
	}	

	return game;

}

void ResetGallery( GameAppInfo &app )
{
	for (int i=0; i < 100; i++) {
		app.gridBoards[i] = RandomBoard( app );
		app.gridBoards_A[i] = AnalyzeGame( app, app.gridBoards[i] );
	}
}

void LoadWeights_TicToe( GameState &state, double *inputs )
{
    // one weight per sqaure
    for (int i=0; i < 9; i++) {
        if (state.gg.square[i]==SQUARE_X) {
            inputs[i] = -1.0;
        } else if (state.gg.square[i]==SQUARE_O) {
            inputs[i] = 1.0;
        } else {
            inputs[i] = 0.0;
        }
    }

    // Weight for whose turn it is
    inputs[9] = (state.to_move==SQUARE_X) ? 1.0 : 0.0;
    inputs[10] = (state.to_move==SQUARE_O) ? 1.0 : 0.0;
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
    result.gg.square[moveLoc] = prevState.to_move;
    result.gg.lastMove = moveLoc;
    result.to_move = NextPlayer( prevState.to_move );
    
    result = CheckWinner( result );

    return result;
}

void GeneratePossibleMoves_TicToe( const GameState &game, GameStateArray &moves )
{
    for (int i=0; i < 9; i++) {
        if (game.gg.square[i] == SQUARE_BLANK) {
            GameState moveState = ApplyMove( game, i );
            PushGameState( &moves, moveState );
        }
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

    GameUIStuff gameUI = {};    
    
    
    // Set up the metadata for the game
    SetupNeutralAnalysis( NEUTRAL_ANALYSIS );
    gameUI.app.info.minPlayerCount = 2;
    gameUI.app.info.maxPlayerCount = 2;
    gameUI.app.info.net_inputs = 11;
    gameUI.app.info.net_hidden_layers = 3;
    gameUI.app.info.net_hidden_layer_size = 64;
    
    gameUI.app.info.gameFunc_LoadWeights = LoadWeights_TicToe;
    gameUI.app.info.gameFunc_GeneratePossibleMoves = GeneratePossibleMoves_TicToe;

    GameInit( gameUI.app );
        
    //ResetGallery( app );

    // Main game loop
    while (!exitWindow)    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        exitWindow = WindowShouldClose();
       
         BeginDrawing();

		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        GameUI_DoCommonUI( gameUI );

        if (IsKeyPressed(KEY_R)) {
            // reset game
            ResetGallery( gameUI.app );
        }
		
 
        EndDrawing();
	}

 	CloseWindow();    
}
