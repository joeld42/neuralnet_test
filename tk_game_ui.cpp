
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <raylib.h>
#include <raygui.h>

#include "tk_game_ui.h"
#include "tk_evalgame.h"

static const char *_mode_name[NUM_MODES] =
{
    "Play", "Step", "Gallery", "Tree"
};

Vector2 Gui_MakeVec2( float x, float y )
{
    Vector2 result = {};
    result.x = x;
    result.y = y;
    return result;
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

float min_float( float a, float b )
{
    if ( a < b) return a;
    else return b;
}

Rectangle FitSquareInRect( Rectangle outer_rect )
{
    float sz = min_float( outer_rect.width, outer_rect.height );
    float hmarg = (outer_rect.width - sz) / 2.0;
    float vmarg = (outer_rect.height - sz ) /2.0;
    Rectangle rect = {};
    rect.x = outer_rect.x + hmarg;
    rect.y = outer_rect.y + vmarg;
    rect.width = sz;
    rect.height = sz;
    
    return rect;
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

// Helpers
GameState GameUI_CurrMove( GameUIStuff &gameUI )
{
    return gameUI.app.gameHistory[ gameUI.app.currMove ];
}

GameState &GameUI_CurrMoveRef( GameUIStuff &gameUI )
{
    return gameUI.app.gameHistory[ gameUI.app.currMove ];
}

GameState GameApp_CurrMove( GameAppInfo &gameApp )
{
    return gameApp.gameHistory[ gameApp.currMove ];
}

GameState &GameApp_CurrMoveRef( GameAppInfo &gameApp )
{
    return gameApp.gameHistory[ gameApp.currMove ];
}

// Game analysis stuff
void SetupNeutralAnalysis( GameAnalysis &ga )
{
    ga.plr[0].win_chance = 0.5;
    ga.plr[1].win_chance = 0.5;
    ga.tie_chance = 0.0f;

}

// == UI Stuff
void GameUI_ResetGame( GameUIStuff &gameUI )
{
    // reset game
    gameUI.app.currMove = 0;
    gameUI.app.gameHistory[0] = (GameState){};
    gameUI.app.gameHistory[0].to_move = SQUARE_X;
}

// Uses Tree search to evaluate the best moves but doen't apply it yet
GameState GameUI_EvalMovesWithTreeSearch( GameUIStuff &gameUI )
{
    printf("Tree search....\n");
    GameState currMove = GameUI_CurrMove( gameUI );
    GameState aiMove = TreeSearchMove( gameUI.app, currMove );
    
    return aiMove;
}


// Does EvalMoves and then applies the best one
void GameUI_MoveUsingTreeSearch( GameUIStuff &gameUI )
{
    GameState currMove = GameUI_CurrMove( gameUI );
    GameState aiMove = TreeSearchMove( gameUI.app, currMove );
    if (aiMove.gg.lastMove == currMove.gg.lastMove ) {
        printf("No valid moves.\n");
    } else {
        printf("AI move on square %d\n", aiMove.gg.lastMove );
        int nextMove = gameUI.app.currMove+1;
        gameUI.app.gameHistory[nextMove] = aiMove;
        gameUI.app.currMove = nextMove;
    }
}

int cmpFunc_PotentialMove( const void *pA, const void *pB )
{
    PotentialMove *a = (PotentialMove*)pA;
    PotentialMove *b = (PotentialMove*)pB;
    if (a->value < b->value) {
        return 1;
    } else if (b->value < a->value) {
        return -1;
    } else return 0;
}

void GameUI_GatherPotentialMoves( GameUIStuff &gameUI )
{
    GameState currMove = GameUI_CurrMove( gameUI );
    TreeSearchMove( gameUI.app, currMove );
    
    // Copy potential moves in
    gameUI.numPotentialMoves = 0;
    MCTSNode &root = gameUI.app.nodes[0];
    int childNdx = root.leftChildNdx;
    while (childNdx != 0) {
        MCTSNode &child = gameUI.app.nodes[childNdx];
        if (child.totalVisits > 0) {
            float val = child.totalWins / child.totalVisits;
            
            PotentialMove move = {};
            move.value = val;
            move.move = child.state;
            move.analysis = AnalyzeGame( gameUI.app, child.state );
            
            if ( gameUI.numPotentialMoves >= MAX_UI_MOVES )
            {
                break;
            }
            gameUI.potentialMoves[gameUI.numPotentialMoves++] = move;
            
        }
        childNdx = child.rightSiblingNdx;
    }
    
    // Sort potential moves by value
    qsort( gameUI.potentialMoves, gameUI.numPotentialMoves,
          sizeof(PotentialMove), cmpFunc_PotentialMove );
}

void GameUI_AutoTrain( GameUIStuff &gameUI, int count )
{
    if (gameUI.autoTrain) {
        gameUI.autoTrain = 0;
        printf("Autotrain off\n");
    } else {
        gameUI.autoTrain = count;
        printf("Autotrain set to %d\n", count );
    }
}


void GameUI_DrawGallery( GameUIStuff &gameUI )
{
    for (int j=0; j < 10; j++ ) {
        for (int i=0; i < 10; i++) {

            int ndx = j*10+i;
            DrawBoard (
                (Rectangle){ (690.f-560.f)*0.5f+ 10.f + (56.f*i), 5 + (56.f*j), 50, 50 },
                gameUI.app.gridBoards[ndx],
                gameUI.app.gridBoards_A[ndx],
                NULL, false );
        }
    }
}

void GameUI_DrawTree( GameUIStuff &gameUI )
{
    for (int ndx=0; ndx < gameUI.app.numNodes; ndx++) {
        MCTSNode &curr = gameUI.app.nodes[ndx];
        
        if (ndx != 0) {
            curr.level = gameUI.app.nodes[curr.parentNdx].level + 1;
        }
    }

    int rowHite = 85;
    gameUI.app.nodes[0].ui_expanded = true;
    for (int level=0; level < 6; level++) {
        float xval = 10.0f;

        for (int ndx=0; ndx < gameUI.app.numNodes; ndx++) {
            MCTSNode &curr = gameUI.app.nodes[ndx];
            if (curr.level != level) continue;

            // DBG skip unexplored nodes
            if (curr.totalVisits < 3) continue;

            if (!gameUI.app.nodes[curr.parentNdx].ui_expanded) continue;

            Rectangle rect;
            rect.x = xval;
            rect.y = 10 + rowHite * level;
            rect.width = 50;
            rect.height = 50;

            Vector2 mousePos = GetMousePosition();
            Rectangle mouseRect = (Rectangle){ mousePos.x, mousePos.y, 1, 1 };
            if (IsMouseButtonPressed(0) && CheckCollisionRecs( rect, mouseRect )) {
                curr.ui_expanded = !curr.ui_expanded;
            }

            float ucb = NodeValUCB1( gameUI.app, curr );

            curr.xval = rect.x + (rect.width/2.0f);
            if (level >0) {
                float lasty = 10 + rowHite * (level-1);
                MCTSNode &parent = gameUI.app.nodes[curr.parentNdx];
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
            DrawBoard( boardRect, curr.state, NEUTRAL_ANALYSIS, NULL, false );

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
            xval += rect.width * 1.5;
            // if (xval > 500) break;
        }
    }
}

void GameUI_DrawGameMode( GameUIStuff &gameUI )
{
    Rectangle gameRect = { 100, 100, 500, 300 };

    GameAnalysis analysis = AnalyzeGame( gameUI.app, GameUI_CurrMove(gameUI ) );

    DrawText( gameUI.app.simMode==SIM_RANDOM?"Rand":"Eval", 600, 10, 20, BLACK);

    int promptx = 20;
    int prompty = 50;
    GameState &game = GameUI_CurrMoveRef( gameUI );
    if (game.gameResult == RESULT_TIE_GAME) {
        DrawText( "Tie Game", promptx, prompty, 20, ORANGE );
    } else if ((game.gameResult == RESULT_WINNER) && (game.winner == PLAYER_1)) {
        DrawText( "Winner is X", promptx, prompty, 20, RED);
    } else if ((game.gameResult == RESULT_WINNER) && (game.winner == PLAYER_2)) {
        DrawText( "Winner is O", promptx, prompty, 20, BLUE);
    } else if (game.to_move == SQUARE_X) {
        DrawText( "X to Move", promptx, prompty, 20, RED);
    } else if (game.to_move == SQUARE_O) {
        DrawText( "O to Move", promptx, prompty, 20, BLUE);
    }
    
    prompty += 20;
    char buff[20];
    sprintf( buff, "%d trains (%d games)", gameUI.app.trainCount, gameUI.app.gameCount );
    DrawText( buff, promptx, prompty, 20, ORANGE );

    prompty += 22;
    sprintf( buff, "X %d / O %d / Tie %d\n",
        gameUI.app.winXcount,
        gameUI.app.winOcount,
        gameUI.app.tieCount );
    DrawText( buff, promptx, prompty, 20, ORANGE );

    DrawBoard( gameRect, game,
        analysis, &gameUI, true  );

    prompty += 22;
    Rectangle sliderRect = { 20, 400, 165, 20 };
    sliderRect.y = prompty;
    gameUI.temperature = GuiSlider(sliderRect,
        "T", TextFormat("%0.3f", gameUI.temperature) , gameUI.temperature, 0.0, 1.0);

    prompty += 30;
    DrawText( TextFormat("X: %3.2f O: %3.2f Tie: %3.2f",
        analysis.plr[1].win_chance, analysis.plr[1].win_chance, analysis.tie_chance ),
        promptx, prompty, 20, BLUE );

    // Show history too
    for (int i=0; i <= gameUI.app.currMove; i++) {

        Rectangle boardRect = { 550, 10, 40, 40 };
        boardRect.y += i*45;
        DrawBoard( boardRect, gameUI.app.gameHistory[i], NEUTRAL_ANALYSIS, NULL, false );
    }
    
    // Show possible moves
    Rectangle moveRect = { 30, 420, 60, 60 };
    for (int i=0; i < gameUI.numPotentialMoves; i++) {
        PotentialMove &move = gameUI.potentialMoves[i];
        
        DrawBoard( moveRect, move.move, move.analysis, NULL, false );
        DrawText( TextFormat( "%3.2f", move.value), moveRect.x + 2, moveRect.y-12, 12, BLACK );
        moveRect.x += moveRect.width + 10;
        
        if (moveRect.x > 500) {
            moveRect.x = 30;
            moveRect.y += moveRect.height + 10;
        }
    }

}

void GameUI_ShowModeName( GameUIStuff &gameUI )
{
    char modeStr[50];
    strcpy( modeStr, _mode_name[gameUI.mode] );

    DrawText( modeStr, 20, 520, 20, RED);

}

// Draws a box with an X in it for placement
void DrawPlacementBox( Rectangle rect, Color color )
{
    DrawRectangleLinesEx( rect, 2, color );
    DrawLine( rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, color );
    DrawLine( rect.x + rect.width, rect.y, rect.x, rect.y + rect.height, color );

}

void DrawBoardFake( Rectangle outer_rect )
{
    Rectangle rect = FitSquareInRect( outer_rect );
    
    Color lightOrange = (Color){ 255, 200, 100, 255 };
    DrawRectangleLinesEx( outer_rect, 2, lightOrange );
    DrawPlacementBox( rect, (Color)ORANGE );
}

void GameUI_DrawStepMode( GameUIStuff &gameUI )
{
    Rectangle gameRect = { 0, 10, 690, 300 };

    GameAnalysis analysis = AnalyzeGame( gameUI.app, GameUI_CurrMove(gameUI ) );
    GameState &game = GameUI_CurrMoveRef( gameUI );
    
    //DrawBoard( gameRect, game, analysis, &gameUI, true  );
    DrawBoardFake( gameRect );
    
    // Now draw boards for all the possible moves
    int moveRows = 2;
    int moveCols = 6;
    int showNumMoves = moveCols * moveRows;
    float thumbWidth = gameRect.width / moveCols;
    float moveStartY = gameRect.y + gameRect.height;
    float thumbHite = (570 - moveStartY) / moveRows;
    
    int numMoves = gameUI.numPotentialMoves;
    if (numMoves > showNumMoves) {
        numMoves = showNumMoves;
    }
    for (int i=0; i < numMoves; i++)
    {
        int ii = i % moveCols;
        int jj = i / moveCols;
        Rectangle moveRect = {};
        moveRect.x = ii * thumbWidth;
        moveRect.y = (jj * thumbHite) + moveStartY;
        moveRect.width = thumbWidth;
        moveRect.height = thumbHite;
        DrawBoardFake( moveRect );
        
        char moveN[10];
        sprintf( moveN, "%d", i );
        DrawText( moveN, moveRect.x, moveRect.y, 20, (Color)RED );
    }
    
    if (gameUI.numPotentialMoves > showNumMoves) {
        DrawText( "More Moves Not Shown", 400, 590, 12, (Color)RED );
    }
}

void GameUI_DoCommonUI( GameUIStuff &gameUI )
{
    if (IsKeyPressed( KEY_TAB )) {

        gameUI.mode++;
        if (gameUI.mode == NUM_MODES) {
            gameUI.mode = MODE_PLAY;
        }
    }

    
    if (gameUI.mode==MODE_PLAY) {
        
        // TODO: This should be game-specific play mode, not
        // handled here
        
        GameUI_DrawGameMode( gameUI );

        if (IsKeyPressed(KEY_R)) {
            
            GameUI_ResetGame( gameUI );

        }

        if (IsKeyPressed(KEY_S)) {
            // Tree search and move
            GameUI_MoveUsingTreeSearch( gameUI );
            GameUI_GatherPotentialMoves( gameUI );
        }
        
        if (IsKeyPressed(KEY_A)) {
            // Analyze moves
            GameUI_GatherPotentialMoves( gameUI );
            printf("Got %d potential moves.\n", gameUI.numPotentialMoves );
        }

        if (IsKeyPressed(KEY_T)) {
            if ( GameUI_CurrMoveRef(gameUI).gameResult == RESULT_IN_PROGRESS) {
                printf("Can't train, game is not finished.\n");
            } else {
                printf("Train on this game.\n");
                TrainHistory( gameUI.app );
            }
        }

        if (IsKeyPressed(KEY_M)) {
            GameUI_AutoTrain( gameUI, 1 );
        }
        if (IsKeyPressed(KEY_N)) {
            GameUI_AutoTrain( gameUI, 500 );
        }
        if (IsKeyPressed(KEY_B)) {
            GameUI_AutoTrain( gameUI, 1000 );
        }

        if (IsKeyPressed(KEY_L)) {
            gameUI.app.simMode++;
            if (gameUI.app.simMode == NUM_SIM_MODES) {
                gameUI.app.simMode = 0;
            }
        }

        if (IsKeyPressed(KEY_P)) {
            // print weights
            printf("Inputs: ");
            for (int i=0; i < 11; i++) {
                printf("%3.2lf ", gameUI.app.inputs[i] );
            }
            printf("\nOutputs: %3.2lf %3.2lf\n", gameUI.app.outputs[0], gameUI.app.outputs[1] );
            
            // HERE: Print potential moves
//                for (int i=0; i < 9; i++) {
//                    printf("Square %d: X %3.2f, O %3.2f (sum %3.2f)\n",
//                        i,
//                        gameUI.preview[i].plr[0].win_chance,
//                        gameUI.preview[i].plr[1].win_chance,
//
//                        gameUI.preview[i].plr[0].win_chance + gameUI.preview[i].plr[1].win_chance  );
//                }
        }

        if (gameUI.autoTrain)
        {
            for (int s=0; s < gameUI.autoTrain; s++)
            {
                TrainOneStep( gameUI.app, gameUI.temperature );
                if (((s % 100) == 0) && (s>0)) {
                    printf("train %d\n", s );
                }
            }

            // Don't train continously for
            if (gameUI.autoTrain > 999) {
                gameUI.autoTrain = 0;
                printf("phew\n");
            }

        } else {
            // Manual play mode

            // If the game is not over, allow moves
//                if (gameUI.app.gameHistory[ gameUI.app.currMove].gameResult == RESULT_IN_PROGRESS)
//                {
//                    if (IsMouseButtonPressed(0)) {
//                        // check if we're in a game space
//                        Vector2 mousePos = GetMousePosition();
//                        Rectangle mouseRect = (Rectangle){ mousePos.x, mousePos.y, 1, 1 };
//                        for (int i=0; i < 9; i++) {
//                            if ( gameUI.app.gameHistory[ gameUI.app.currMove].gg.square[i]!=SQUARE_BLANK) {
//                                // Square is occupied
//                                printf("Square is occupied.\n");
//                            } else if (CheckCollisionRecs( gameUI.screenRect[i], mouseRect )) {
//
//                                printf("Clicked on square %d\n", i );
//                                int nextMove = gameUI.app.currMove+1;
//                                gameUI.app.gameHistory[nextMove] = ApplyMove( GameUI_CurrMove( gameUI ), i );
//                                gameUI.app.currMove = nextMove;
//
//                                PreviewBestMove( gameUI, gameUI.app.gameHistory[ gameUI.app.currMove] );
//
//                                break;
//                            }
//                        }
//                    }
//                }
        }
        
    } else if (gameUI.mode==MODE_STEP) {
        
        // STEP is like PLAY mode but only game-agnostic stuff.
        GameUI_DrawStepMode( gameUI );
        
    } else if ( gameUI.mode==MODE_GALLERY) {

        
        GameUI_DrawGallery( gameUI );
        
        
    
    } else if ( gameUI.mode==MODE_TREE) {
        
        GameUI_DrawTree( gameUI );
    }
    
    
    GameUI_ShowModeName( gameUI );
}
