$c = Get-Content -Raw -Encoding UTF8 "src/main.c"

# 1. math.h
$c = $c.Replace('#include "raylib.h"', "#include "raylib.h"
#include <math.h>")

# 2. GameMaster gm = {0};
$c = $c.Replace("GameMaster gm;", "GameMaster gm = {0};")

# 3. V3 Event application
$c = $c.Replace("ResultView_Apply(gm.activeResult, &gm, &playerArmy);", "Event_ApplyResult(gm.activeResult, &playerArmy, &gm.infamy);")
$ev_apply = "EventView_ApplyResult(chosenOpt, &gm, &playerArmy);"
$c = $c.Replace("$ev_apply
                        gm.phase = PHASE_PLAYER;", "gm.phase = PHASE_PLAYER;")

# 4. Result loop
$c = $c.Replace("for (int k = 0; k < 128; k++) {", "for (int k = 0; k < gm.resultCount; k++) {")

# 5. Click logic replacement
$old_click = "if (tile->coord.q == mouseHex.q && tile->coord.r == mouseHex.r) {
                    if (tile == selectedTile) {
                        if (gm.hasRolledThisTurn && GameMaster_AttemptMove(&gm, &levelMap, &playerArmy, tile)) {
                            selectedTile = NULL;
                            Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm));
                        }
                    } else {
                        selectedTile = tile;
                    }
                    break;
                }"

$new_click = "if (tile->coord.q == mouseHex.q && tile->coord.r == mouseHex.r) {
                    if (tile == selectedTile && !playerArmy.isMoving) {
                        if (gm.hasRolledThisTurn) {
                            HexTile* startTile = NULL;
                            for (int k = 0; k < levelMap.tileCount; k++) {
                                if (levelMap.tiles[k].coord.q == playerArmy.position.q && levelMap.tiles[k].coord.r == playerArmy.position.r) {
                                    startTile = &levelMap.tiles[k]; break;
                                }
                            }
                            if (startTile) {
                                HexTile* path[128];
                                int cost = 0;
                                int pathLen = Map_FindPath(&levelMap, startTile, tile, &cost, path);
                                if (pathLen > 0 && gm.currentActionPoints >= cost) {
                                    for (int p = 0; p < pathLen; p++) {
                                        playerArmy.movePath[p] = path[p]->coord;
                                    }
                                    playerArmy.pathLength = pathLen;
                                    playerArmy.currentPathIndex = 0;
                                    playerArmy.moveTimer = 0.0f;
                                    playerArmy.isMoving = true;
                                    selectedTile = NULL;
                                }
                            }
                        }
                    } else {
                        selectedTile = tile;
                    }
                    break;
                }"
$c = $c.Replace($old_click, $new_click)

# 6. Arrow drawing
$old_draw = "if (selectedTile != NULL)
        {
            MapView_DrawHexHighlight(
                selectedTile,
                hexRadius,
                origin,
                GREEN
            );
        }"

$new_draw = "if (selectedTile != NULL && !playerArmy.isMoving)
        {
            HexTile* startTile = NULL;
            for (int k = 0; k < levelMap.tileCount; k++) {
                if (levelMap.tiles[k].coord.q == playerArmy.position.q && levelMap.tiles[k].coord.r == playerArmy.position.r) {
                    startTile = &levelMap.tiles[k]; break;
                }
            }
            if (startTile) {
                HexTile* path[128];
                int cost = 0;
                int pathLen = Map_FindPath(&levelMap, startTile, selectedTile, &cost, path);
                
                Color pathColor = (gm.currentActionPoints >= cost) ? GREEN : RED;
                
                if (pathLen > 0) {
                    MapView_DrawHexHighlight(selectedTile, hexRadius, origin, pathColor);
                    
                    Vector2 p1 = HexToPixel(playerArmy.position, hexRadius);
                    p1.x += origin.x; p1.y += origin.y;
                    
                    Vector2 p2 = HexToPixel(selectedTile->coord, hexRadius);
                    p2.x += origin.x; p2.y += origin.y;
                    
                    DrawLineEx(p1, p2, 6.0f, pathColor);
                    
                    float angle = atan2f(p2.y - p1.y, p2.x - p1.x);
                    float arrowSize = 20.0f;
                    Vector2 t1 = { p2.x - arrowSize * cosf(angle - 0.5f), p2.y - arrowSize * sinf(angle - 0.5f) };
                    Vector2 t2 = { p2.x - arrowSize * cosf(angle + 0.5f), p2.y - arrowSize * sinf(angle + 0.5f) };
                    DrawLineEx(p2, t1, 6.0f, pathColor);
                    DrawLineEx(p2, t2, 6.0f, pathColor);
                    
                    Vector2 midPx = { (p1.x + p2.x) / 2.0f, (p1.y + p2.y) / 2.0f };
                    DrawCircleV(midPx, 18.0f, DARKGRAY);
                    DrawCircleLines(midPx.x, midPx.y, 18.0f, pathColor);
                    const char* text = TextFormat("%d PA", cost);
                    int textW = MeasureText(text, 10);
                    DrawText(text, midPx.x - textW/2, midPx.y - 5, 10, WHITE);
                }
            }
        }"
$c = $c.Replace($old_draw, $new_draw)

# 7. Movement animation block
$anim = "/* ========================================================
           MOVIMIENTO DEL EJERCITO
           ======================================================== */
        if (playerArmy.isMoving && gm.phase != PHASE_EVENT) {
            playerArmy.moveTimer += GetFrameTime();
            if (playerArmy.moveTimer >= 0.25f) { // 0.25 seg por tile
                playerArmy.moveTimer = 0.0f;
                HexCoord nextPos = playerArmy.movePath[playerArmy.currentPathIndex];
                
                HexTile* targetTile = NULL;
                for (int i = 0; i < levelMap.tileCount; i++) {
                    if (levelMap.tiles[i].coord.q == nextPos.q && levelMap.tiles[i].coord.r == nextPos.r) {
                        targetTile = &levelMap.tiles[i];
                        break;
                    }
                }
                
                if (targetTile) {
                    if (GameMaster_AttemptMove(&gm, &levelMap, &playerArmy, targetTile)) {
                        Map_UpdateVision(&levelMap, playerArmy.position, Army_GetVision(&playerArmy) + GameMaster_GetVisionModifier(&gm));
                        playerArmy.currentPathIndex++;
                        if (playerArmy.currentPathIndex >= playerArmy.pathLength) {
                            playerArmy.isMoving = false;
                        } else if (gm.phase == PHASE_EVENT) {
                            playerArmy.isMoving = false; 
                        }
                    } else {
                        playerArmy.isMoving = false;
                    }
                } else {
                    playerArmy.isMoving = false;
                }
            }
        }

"
$c = $c.Replace("CameraController_Update(&cc, GetFrameTime());", $anim + "        CameraController_Update(&cc, GetFrameTime());")

$c | Set-Content -Encoding UTF8 "src/main.c"
