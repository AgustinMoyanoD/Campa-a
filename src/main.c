#include "raylib.h"

#include "map_generator.h"
#include "map_view.h"

#include <stdlib.h>
#include <time.h>


int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    const int mapRadius = 70;
    const int landProbabilityPercent = 55;

    const float hexRadius = 35.0f;


    /* ============================================================
       VENTANA
       ============================================================ */

    InitWindow(
        screenWidth,
        screenHeight,
        "Estrategia Táctica - Nivel 1 Procedural"
    );

    ToggleFullscreen();

    SetTargetFPS(60);


    srand(
        (unsigned int)time(NULL)
    );


    /* ============================================================
       CENTRO DE LA PANTALLA
       ============================================================ */

    Vector2 center = {
        GetScreenWidth() / 2.0f,
        GetScreenHeight() / 2.0f
    };


    /* ============================================================
       MAPA
       ============================================================ */

    Map levelMap =
        Map_GenerateProcedural(
            mapRadius,
            landProbabilityPercent,
            3
        );


    /* ============================================================
       CÁMARA
       ============================================================ */

    Camera2D camera = { 0 };

    camera.target = center;
    camera.offset = center;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;


    /*
     * Origin representa la posición del hexágono (0,0)
     * dentro del mundo.
     *
     * Por ahora coincide con el centro de la pantalla.
     */
    Vector2 origin = center;


    /* ============================================================
       TILE SELECCIONADO
       ============================================================ */

    HexTile* selectedTile = NULL;


    /* ============================================================
       LOOP PRINCIPAL
       ============================================================ */

    while (!WindowShouldClose())
    {
        /* ========================================================
           REGENERAR MAPA
           ======================================================== */

        if (IsKeyPressed(KEY_R))
        {
            Map_Free(&levelMap);

            levelMap =
                Map_GenerateProcedural(
                    mapRadius,
                    landProbabilityPercent,
                    3
                );

            /*
             * El tile seleccionado ya no pertenece al mapa
             * anterior, por lo que debemos eliminar la referencia.
             */
            selectedTile = NULL;
        }


        /* ========================================================
           CÁMARA
           ======================================================== */

        float speed = 15.0f;

        if (IsKeyDown(KEY_D))
            camera.target.x += speed;

        if (IsKeyDown(KEY_A))
            camera.target.x -= speed;

        if (IsKeyDown(KEY_S))
            camera.target.y += speed;

        if (IsKeyDown(KEY_W))
            camera.target.y -= speed;


        /* ========================================================
           ZOOM
           ======================================================== */

        float wheel =
            GetMouseWheelMove();

        if (wheel != 0.0f)
        {
            camera.zoom +=
                wheel * 0.1f;

            if (camera.zoom < 0.2f)
                camera.zoom = 0.2f;

            if (camera.zoom > 3.0f)
                camera.zoom = 3.0f;
        }


        /* ========================================================
           MOUSE INPUT
           ======================================================== */

        if (IsMouseButtonPressed(
                MOUSE_BUTTON_LEFT))
        {
            /*
             * Posición del mouse en pantalla.
             */
            Vector2 mousePosition =
                GetMousePosition();

            /*
             * Convertir de coordenadas de pantalla
             * a coordenadas del mundo teniendo en cuenta
             * la cámara y el zoom.
             */
            Vector2 worldPosition =
                GetScreenToWorld2D(
                    mousePosition,
                    camera
                );

            /*
             * MapView dibuja los hexágonos de esta forma:
             *
             * posición del hexágono =
             * HexToPixel(coord) + origin
             *
             * Por lo tanto, quitamos origin antes
             * de hacer PixelToHex().
             */
            worldPosition.x -= origin.x;
            worldPosition.y -= origin.y;

            /*
             * Convertir posición del mouse
             * a coordenadas hexagonales.
             */
            HexCoord mouseHex =
                PixelToHex(
                    worldPosition,
                    hexRadius
                );


            /*
             * Buscar el HexTile correspondiente.
             */
            selectedTile = NULL;

            for (int i = 0;
                 i < levelMap.tileCount;
                 i++)
            {
                HexTile* tile =
                    &levelMap.tiles[i];

                if (tile->coord.q ==
                        mouseHex.q &&
                    tile->coord.r ==
                        mouseHex.r)
                {
                    selectedTile =
                        tile;

                    break;
                }
            }
        }


        /* ========================================================
           DIBUJAR
           ======================================================== */

        BeginDrawing();

        ClearBackground(
            RAYWHITE
        );


        BeginMode2D(camera);


        MapView_Draw(
            &levelMap,
            hexRadius,
            origin,
            false,
            selectedTile,
            5
        );


        /*
         * Resaltar el tile seleccionado.
         */
        if (selectedTile != NULL)
        {
            MapView_DrawHexHighlight(
                selectedTile,
                hexRadius,
                origin,
                YELLOW
            );
        }


        EndMode2D();


        /* ========================================================
           HUD
           ======================================================== */

        DrawText(
            "Leyenda:",
            20,
            20,
            18,
            BLACK
        );


        DrawText(
            "Amarillo: Inicio",
            20,
            45,
            16,
            ORANGE
        );


        DrawText(
            "Rojo: Fin",
            20,
            65,
            16,
            RED
        );


        DrawText(
            "Marron: Camino",
            20,
            85,
            16,
            DARKBROWN
        );


        DrawText(
            "Azul: Rio",
            20,
            105,
            16,
            BLUE
        );


        DrawText(
            "Marron grueso: Puente",
            20,
            125,
            16,
            BROWN
        );


        DrawText(
            "Celeste: Agua",
            20,
            145,
            16,
            SKYBLUE
        );


        DrawText(
            "WASD: Mover camara",
            20,
            175,
            16,
            DARKGRAY
        );


        DrawText(
            "R: Regenerar mapa",
            20,
            195,
            16,
            DARKGRAY
        );


        DrawText(
            "Mouse Wheel: Zoom",
            20,
            215,
            16,
            DARKGRAY
        );


        /*
         * Información del tile seleccionado.
         */
        if (selectedTile != NULL)
        {
            DrawText(
                TextFormat(
                    "Tile: q=%d r=%d",
                    selectedTile->coord.q,
                    selectedTile->coord.r
                ),
                20,
                250,
                18,
                BLACK
            );
        }


        EndDrawing();
    }


    /* ============================================================
       LIBERAR
       ============================================================ */

    Map_Free(
        &levelMap
    );

    CloseWindow();

    return 0;
}