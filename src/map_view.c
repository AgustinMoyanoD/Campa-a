#include "map_view.h"
#include <stddef.h>
#include <math.h>

/* ============================================================
   CONVERSIÓN HEX -> PIXEL
   ============================================================ */

Vector2 HexToPixel(
    HexCoord coord,
    float radius
)
{
    Vector2 position;

    /*
     * Hexágonos pointy-top.
     */
    position.x =
        radius *
        sqrtf(3.0f) *
        ((float)coord.q +
         (float)coord.r / 2.0f);

    position.y =
        radius *
        1.5f *
        (float)coord.r;

    return position;
}

/* ============================================================
   CONVERSIÓN PIXEL -> HEX
   ============================================================ */

HexCoord PixelToHex(
    Vector2 position,
    float radius
)
{
    float q =
        (sqrtf(3.0f) / 3.0f * position.x
         - 1.0f / 3.0f * position.y)
        / radius;

    float r =
        (2.0f / 3.0f * position.y)
        / radius;

    /*
     * Conversión axial -> cúbica.
     */
    float x = q;
    float z = r;
    float y = -x - z;

    int rx = (int)roundf(x);
    int ry = (int)roundf(y);
    int rz = (int)roundf(z);

    float dx = fabsf((float)rx - x);
    float dy = fabsf((float)ry - y);
    float dz = fabsf((float)rz - z);

    if (dx > dy && dx > dz)
    {
        rx = -ry - rz;
    }
    else if (dy > dz)
    {
        ry = -rx - rz;
    }
    else
    {
        rz = -rx - ry;
    }

    HexCoord coord;

    coord.q = rx;
    coord.r = rz;

    return coord;
}

/* ============================================================
   POSICIÓN VISUAL
   ============================================================ */

static Vector2 MapView_GetTilePosition(
    const HexTile* tile,
    float radius,
    Vector2 origin
)
{
    Vector2 position =
        HexToPixel(
            tile->coord,
            radius
        );

    position.x += origin.x;
    position.y += origin.y;

    return position;
}

/* ============================================================
   DISTANCIA HEXAGONAL
   ============================================================ */

static int MapView_HexDistance(
    HexCoord a,
    HexCoord b
)
{
    int dq =
        a.q - b.q;

    int dr =
        a.r - b.r;

    int ds =
        (a.q + a.r) -
        (b.q + b.r);

    if (dq < 0)
        dq = -dq;

    if (dr < 0)
        dr = -dr;

    if (ds < 0)
        ds = -ds;

    return (dq + dr + ds) / 2;
}

/* ============================================================
   ¿EL TILE ESTÁ DENTRO DEL ÁREA DE RENDER?
   ============================================================ */

static bool MapView_IsTileInRange(
    const HexTile* tile,
    const HexTile* centerTile,
    int renderRadius
)
{
    /*
     * Si no hay tile central, dibujamos todo.
     */
    if (centerTile == NULL)
        return true;

    /*
     * Un radio negativo también significa
     * que no queremos limitar el render.
     */
    if (renderRadius < 0)
        return true;

    return
        MapView_HexDistance(
            tile->coord,
            centerTile->coord
        ) <= renderRadius;
}

/* ============================================================
   DIBUJAR HEXÁGONO
   ============================================================ */

void MapView_DrawHex(
    const HexTile* tile,
    float radius,
    Vector2 origin
)
{
    if (tile == NULL)
        return;

    Vector2 position =
        MapView_GetTilePosition(
            tile,
            radius,
            origin
        );

    Color fillColor;
    Color borderColor;

    /*
     * Colores del terreno.
     *
     * El borde es una versión más oscura
     * del mismo color.
     */
    if (tile->terrain == TERRAIN_WATER)
    {
        fillColor = SKYBLUE;
        borderColor = BLUE;
    }
    else
    {
        fillColor = LIME;
        borderColor = DARKGREEN;
    }

    /*
     * Borde exterior:
     * hexágono ligeramente más grande.
     */
    DrawPoly(
        position,
        6,
        radius + 3.5f,
        30.0f,
        borderColor
    );

    /*
     * Hexágono interior.
     */
    DrawPoly(
        position,
        6,
        radius,
        30.0f,
        fillColor
    );

    /*
     * Inicio.
     */
    if (tile->special ==
        TILE_SPECIAL_START)
    {
        DrawCircle(
            (int)position.x,
            (int)position.y,
            radius * 0.35f,
            GOLD
        );
    }

    /*
     * Final.
     */
    if (tile->special ==
        TILE_SPECIAL_END)
    {
        DrawCircle(
            (int)position.x,
            (int)position.y,
            radius * 0.35f,
            RED
        );
    }
}

/* ============================================================
   DIBUJAR RÍOS
   ============================================================ */

static void MapView_DrawRivers(
    const Map* map,
    float radius,
    Vector2 origin,
    const HexTile* centerTile,
    int renderRadius
)
{
    if (map == NULL)
        return;

    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        const HexTile* tile =
            &map->tiles[i];

        /*
         * No dibujar ríos fuera del rango.
         */
        if (!MapView_IsTileInRange(
                tile,
                centerTile,
                renderRadius))
        {
            continue;
        }

        if (tile->river !=
            RIVER_PRESENT)
        {
            continue;
        }

        Vector2 start =
            MapView_GetTilePosition(
                tile,
                radius,
                origin
            );

        for (int d = 0;
             d < 6;
             d++)
        {
            const HexTile* neighbor =
                tile->neighbors[d];

            if (neighbor == NULL)
                continue;

            if (neighbor->river !=
                RIVER_PRESENT)
            {
                continue;
            }

            /*
             * No dibujar conexiones fuera
             * del área de render.
             */
            if (!MapView_IsTileInRange(
                    neighbor,
                    centerTile,
                    renderRadius))
            {
                continue;
            }

            /*
             * Evitar dibujar la misma conexión dos veces.
             */
            if (neighbor < tile)
                continue;

            Vector2 end =
                MapView_GetTilePosition(
                    neighbor,
                    radius,
                    origin
                );

            DrawLineEx(
                start,
                end,
                radius * 0.28f,
                SKYBLUE
            );
        }
    }
}

/* ============================================================
   DIBUJAR CAMINOS
   ============================================================ */

static void MapView_DrawRoads(
    const Map* map,
    float radius,
    Vector2 origin,
    const HexTile* centerTile,
    int renderRadius
)
{
    if (map == NULL)
        return;

    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        const HexTile* tile =
            &map->tiles[i];

        /*
         * No dibujar caminos fuera del rango.
         */
        if (!MapView_IsTileInRange(
                tile,
                centerTile,
                renderRadius))
        {
            continue;
        }

        if (!tile->isRoad)
            continue;

        Vector2 start =
            MapView_GetTilePosition(
                tile,
                radius,
                origin
            );

        for (int d = 0;
             d < 6;
             d++)
        {
            const HexTile* neighbor =
                tile->neighbors[d];

            if (neighbor == NULL)
                continue;

            if (!neighbor->isRoad)
                continue;

            /*
             * No dibujar conexiones fuera
             * del área de render.
             */
            if (!MapView_IsTileInRange(
                    neighbor,
                    centerTile,
                    renderRadius))
            {
                continue;
            }

            /*
             * Evitar dibujar la misma conexión dos veces.
             */
            if (neighbor < tile)
                continue;

            Vector2 end =
                MapView_GetTilePosition(
                    neighbor,
                    radius,
                    origin
                );

            bool bridge =
                tile->river ==
                    RIVER_PRESENT &&
                neighbor->river ==
                    RIVER_PRESENT;

            if (bridge)
            {
                DrawLineEx(
                    start,
                    end,
                    radius * 0.34f,
                    BROWN
                );
            }
            else
            {
                DrawLineEx(
                    start,
                    end,
                    radius * 0.20f,
                    DARKBROWN
                );
            }
        }
    }
}

/* ============================================================
   DIBUJAR PUENTES
   ============================================================ */

static void MapView_DrawBridges(
    const Map* map,
    float radius,
    Vector2 origin,
    const HexTile* centerTile,
    int renderRadius
)
{
    if (map == NULL)
        return;

    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        const HexTile* tile =
            &map->tiles[i];

        /*
         * No dibujar puentes fuera del rango.
         */
        if (!MapView_IsTileInRange(
                tile,
                centerTile,
                renderRadius))
        {
            continue;
        }

        if (tile->bridge ==
            BRIDGE_NONE)
        {
            continue;
        }

        if (!tile->isRoad)
            continue;

        if (tile->river !=
            RIVER_PRESENT)
        {
            continue;
        }

        Vector2 position =
            MapView_GetTilePosition(
                tile,
                radius,
                origin
            );

        DrawRectangle(
            (int)(position.x -
                  radius * 0.45f),
            (int)(position.y -
                  radius * 0.10f),
            (int)(radius * 0.90f),
            (int)(radius * 0.20f),
            BROWN
        );
    }
}

/* ============================================================
   CONEXIONES DEL GRAFO
   ============================================================ */

void MapView_DrawConnections(
    const Map* map,
    float radius,
    Vector2 origin,
    Color color,
    float thickness
)
{
    if (map == NULL)
        return;

    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        const HexTile* tile =
            &map->tiles[i];

        Vector2 start =
            MapView_GetTilePosition(
                tile,
                radius,
                origin
            );

        for (int d = 0;
             d < 6;
             d++)
        {
            const HexTile* neighbor =
                tile->neighbors[d];

            if (neighbor == NULL)
                continue;

            if (neighbor < tile)
                continue;

            Vector2 end =
                MapView_GetTilePosition(
                    neighbor,
                    radius,
                    origin
                );

            DrawLineEx(
                start,
                end,
                thickness,
                color
            );
        }
    }
}

/* ============================================================
   DIBUJAR MAPA COMPLETO
   ============================================================ */

void MapView_Draw(
    const Map* map,
    float radius,
    Vector2 origin,
    bool drawConnections,
    const HexTile* centerTile,
    int renderRadius
)
{
    if (map == NULL)
        return;

    /*
     * Dibujar solamente los hexágonos
     * dentro del radio indicado.
     */
    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        const HexTile* tile =
            &map->tiles[i];

        if (!MapView_IsTileInRange(
                tile,
                centerTile,
                renderRadius))
        {
            continue;
        }

        MapView_DrawHex(
            tile,
            radius,
            origin
        );
    }

    /*
     * Las conexiones se mantienen como
     * una visualización global opcional.
     */
    if (drawConnections)
    {
        MapView_DrawConnections(
            map,
            radius,
            origin,
            GRAY,
            1.0f
        );
    }

    MapView_DrawRivers(
        map,
        radius,
        origin,
        centerTile,
        renderRadius
    );

    MapView_DrawRoads(
        map,
        radius,
        origin,
        centerTile,
        renderRadius
    );

    MapView_DrawBridges(
        map,
        radius,
        origin,
        centerTile,
        renderRadius
    );
}

/* ============================================================
   HIGHLIGHT
   ============================================================ */

void MapView_DrawHexHighlight(
    const HexTile* tile,
    float radius,
    Vector2 origin,
    Color color
)
{
    if (tile == NULL)
        return;

    Vector2 position =
        MapView_GetTilePosition(
            tile,
            radius,
            origin
        );

    DrawPolyLinesEx(
        position,
        6,
        radius * 0.95f,
        30.0f,
        3.0f,
        color
    );
}