#include "map_generator.h"

#include <stdlib.h>
#include <stdbool.h>


/* ============================================================
   PROTOTIPOS INTERNOS
   ============================================================ */

static float RandomTerrainValue(void);

static void SmoothTerrain(
    Map* map,
    float* values,
    int iterations
);

static HexTile* Map_FindTile(
    Map* map,
    HexCoord coord
);

static int HexDistance(
    HexCoord a,
    HexCoord b
);

static bool IsBoundaryTile(
    const Map* map,
    const HexTile* tile
);

static bool ShareBoundarySide(
    const HexTile* a,
    const HexTile* b
);

static HexTile* Map_GetRandomStartTile(
    Map* map
);

static HexTile* Map_GetRandomEndTile(
    Map* map,
    const HexTile* start
);

static void Map_CreateRiver(
    Map* map,
    HexTile* start
);

static void Map_GenerateRivers(
    Map* map
);

static bool CanRoadPass(
    const HexTile* tile
);

static bool Map_CreateRoad(
    Map* map,
    HexTile* start,
    HexTile* end
);

static void Map_CreateAlternativeRoads(
    Map* map
);


/* ============================================================
   UTILIDADES
   ============================================================ */

/*
    Busca un tile por sus coordenadas.
*/
static HexTile* Map_FindTile(
    Map* map,
    HexCoord coord
)
{
    if (map == NULL)
        return NULL;

    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        if (map->tiles[i].coord.q == coord.q &&
            map->tiles[i].coord.r == coord.r)
        {
            return &map->tiles[i];
        }
    }

    return NULL;
}


/*
    Distancia entre dos hexágonos en coordenadas axiales.
*/
static int HexDistance(
    HexCoord a,
    HexCoord b
)
{
    int dq = a.q - b.q;
    int dr = a.r - b.r;
    int ds = (a.q + a.r) - (b.q + b.r);

    if (dq < 0)
        dq = -dq;

    if (dr < 0)
        dr = -dr;

    if (ds < 0)
        ds = -ds;

    return (dq + dr + ds) / 2;
}


/* ============================================================
   TERRENO PROCEDURAL
   ============================================================ */

/*
    Genera el valor aleatorio inicial de un tile.

    Este valor todavía no determina si el tile es tierra
    o agua. Primero será suavizado.
*/
static float RandomTerrainValue(void)
{
    return (float)(rand() % 101);
}


/*
    Suaviza los valores del terreno.

    El tile actual tiene peso 2.
    Cada vecino tiene peso 1.

    Esto hace que los valores de tiles cercanos tiendan
    a parecerse entre sí.

    Más iteraciones = regiones más grandes y suaves.
*/
static void SmoothTerrain(
    Map* map,
    float* values,
    int iterations
)
{
    if (map == NULL ||
        values == NULL ||
        map->tileCount <= 0)
    {
        return;
    }

    float* newValues =
        malloc(
            sizeof(float) * map->tileCount
        );

    if (newValues == NULL)
        return;


    for (int iteration = 0;
         iteration < iterations;
         iteration++)
    {
        /*
            Calculamos TODOS los valores nuevos
            utilizando únicamente los valores
            de la iteración anterior.
        */
        for (int i = 0;
             i < map->tileCount;
             i++)
        {
            HexTile* tile =
                &map->tiles[i];

            /*
                El propio tile tiene peso 2.
            */
            float sum =
                values[i] * 2.0f;

            float weight =
                2.0f;


            /*
                Agregamos los vecinos.
            */
            for (int d = 0;
                 d < 6;
                 d++)
            {
                HexTile* neighbor =
                    tile->neighbors[d];

                if (neighbor == NULL)
                    continue;

                int neighborIndex =
                    (int)(neighbor - map->tiles);

                sum +=
                    values[neighborIndex];

                weight += 1.0f;
            }


            newValues[i] =
                sum / weight;
        }


        /*
            Reemplazamos los valores anteriores
            por los nuevos.
        */
        for (int i = 0;
             i < map->tileCount;
             i++)
        {
            values[i] =
                newValues[i];
        }
    }


    free(newValues);
}


/* ============================================================
   BORDE
   ============================================================ */

/*
    Determina si el tile está en el borde del mapa.
*/
static bool IsBoundaryTile(
    const Map* map,
    const HexTile* tile
)
{
    if (map == NULL ||
        tile == NULL)
    {
        return false;
    }

    for (int d = 0;
         d < 6;
         d++)
    {
        if (tile->neighbors[d] == NULL)
            return true;
    }

    return false;
}


/*
    Determina si dos tiles pertenecen a la misma cara
    del borde exterior.
*/
static bool ShareBoundarySide(
    const HexTile* a,
    const HexTile* b
)
{
    if (a == NULL ||
        b == NULL)
    {
        return false;
    }

    for (int d = 0;
         d < 6;
         d++)
    {
        if (a->neighbors[d] == NULL &&
            b->neighbors[d] == NULL)
        {
            return true;
        }
    }

    return false;
}


/* ============================================================
   INICIO Y FINALES
   ============================================================ */

/*
    Busca un tile de inicio.

    Condiciones:
        - pertenece al borde
        - es tierra
*/
static HexTile* Map_GetRandomStartTile(
    Map* map
)
{
    if (map == NULL ||
        map->tileCount <= 0)
    {
        return NULL;
    }


    int* candidates =
        malloc(
            sizeof(int) * map->tileCount
        );

    if (candidates == NULL)
        return NULL;


    int count = 0;


    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        HexTile* tile =
            &map->tiles[i];

        if (!IsBoundaryTile(map, tile))
            continue;

        if (tile->terrain != TERRAIN_LAND)
            continue;

        candidates[count++] =
            i;
    }


    if (count == 0)
    {
        free(candidates);
        return NULL;
    }


    int selected =
        rand() % count;

    HexTile* result =
        &map->tiles[
            candidates[selected]
        ];


    free(candidates);

    return result;
}


/*
    Busca un tile de final.

    Condiciones:
        - pertenece al borde
        - es tierra
        - no es otro final
        - no comparte la misma cara exterior que el inicio
*/
static HexTile* Map_GetRandomEndTile(
    Map* map,
    const HexTile* start
)
{
    if (map == NULL ||
        start == NULL ||
        map->tileCount <= 0)
    {
        return NULL;
    }


    int* candidates =
        malloc(
            sizeof(int) * map->tileCount
        );

    if (candidates == NULL)
        return NULL;


    int count = 0;


    /*
        Primera búsqueda:
        aplicamos todas las restricciones.
    */
    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        HexTile* tile =
            &map->tiles[i];


        if (!IsBoundaryTile(map, tile))
            continue;


        if (tile->terrain != TERRAIN_LAND)
            continue;


        /*
            No reutilizamos otro final.
        */
        if (tile->special ==
            TILE_SPECIAL_END)
        {
            continue;
        }


        /*
            El final no puede estar en la misma
            cara exterior que el inicio.
        */
        if (ShareBoundarySide(start, tile))
            continue;


        candidates[count++] =
            i;
    }


    /*
        Si no encontramos suficientes candidatos,
        relajamos la condición de la cara.
    */
    if (count == 0)
    {
        for (int i = 0;
             i < map->tileCount;
             i++)
        {
            HexTile* tile =
                &map->tiles[i];


            if (!IsBoundaryTile(map, tile))
                continue;


            if (tile->terrain != TERRAIN_LAND)
                continue;


            if (tile->special ==
                TILE_SPECIAL_END)
            {
                continue;
            }


            if (tile == start)
                continue;


            candidates[count++] =
                i;
        }
    }


    if (count == 0)
    {
        free(candidates);
        return NULL;
    }


    int selected =
        rand() % count;


    HexTile* result =
        &map->tiles[
            candidates[selected]
        ];


    free(candidates);

    return result;
}


/* ============================================================
   RÍOS
   ============================================================ */

/*
    Genera un río desde un tile de borde
    hacia el centro del mapa.
*/
static void Map_CreateRiver(
    Map* map,
    HexTile* start
)
{
    if (map == NULL ||
        start == NULL)
    {
        return;
    }


    HexTile* current =
        start;


    HexCoord center =
        HexCoord_Create(0, 0);


    for (int step = 0;
         step < map->tileCount;
         step++)
    {
        current->river =
            RIVER_PRESENT;


        HexTile* best =
            NULL;


        int bestDistance =
            HexDistance(
                current->coord,
                center
            );


        /*
            Buscamos el vecino que se acerque
            al centro.
        */
        for (int d = 0;
             d < 6;
             d++)
        {
            HexTile* neighbor =
                current->neighbors[d];


            if (neighbor == NULL)
                continue;


            /*
                No queremos volver a recorrer
                el río que acabamos de crear.
            */
            if (neighbor->river ==
                RIVER_PRESENT)
            {
                continue;
            }


            int distance =
                HexDistance(
                    neighbor->coord,
                    center
                );


            if (distance < bestDistance)
            {
                bestDistance =
                    distance;

                best =
                    neighbor;
            }
        }


        if (best == NULL)
            break;


        current =
            best;


        /*
            El río termina cerca del centro.
        */
        if (HexDistance(
                current->coord,
                center
            ) <= 1)
        {
            current->river =
                RIVER_PRESENT;

            break;
        }
    }
}


/*
    Genera los ríos del mapa.
*/
static void Map_GenerateRivers(
    Map* map
)
{
    if (map == NULL ||
        map->tileCount <= 0)
    {
        return;
    }


    /*
        Por ahora tenemos un río.
        Este número puede convertirse posteriormente
        en un parámetro de generación.
    */
    int riverCount = 3;


    for (int river = 0;
         river < riverCount;
         river++)
    {
        HexTile* start =
            NULL;


        /*
            Buscamos aleatoriamente un tile
            de borde que sea tierra.
        */
        for (int attempts = 0;
             attempts < 100;
             attempts++)
        {
            int index =
                rand() % map->tileCount;


            HexTile* tile =
                &map->tiles[index];


            if (!IsBoundaryTile(map, tile))
                continue;


            if (tile->terrain != TERRAIN_LAND)
                continue;


            start =
                tile;

            break;
        }


        if (start != NULL)
        {
            Map_CreateRiver(
                map,
                start
            );
        }
    }
}


/* ============================================================
   CAMINOS
   ============================================================ */

/*
    Determina si un camino puede pasar por un tile.

    El agua no permite caminos.

    El río SÍ permite caminos porque posteriormente
    ese cruce se convierte en puente.
*/
static bool CanRoadPass(
    const HexTile* tile
)
{
    if (tile == NULL)
        return false;


    if (tile->terrain ==
        TERRAIN_WATER)
    {
        return false;
    }


    return true;
}


/*
    Genera un camino entre dos tiles mediante BFS.
*/
static bool Map_CreateRoad(
    Map* map,
    HexTile* start,
    HexTile* end
)
{
    if (map == NULL ||
        start == NULL ||
        end == NULL)
    {
        return false;
    }


    int tileCount =
        map->tileCount;


    bool* visited =
        calloc(
            tileCount,
            sizeof(bool)
        );


    HexTile** previous =
        calloc(
            tileCount,
            sizeof(HexTile*)
        );


    HexTile** queue =
        malloc(
            sizeof(HexTile*) * tileCount
        );


    if (visited == NULL ||
        previous == NULL ||
        queue == NULL)
    {
        free(visited);
        free(previous);
        free(queue);

        return false;
    }


    int queueStart = 0;
    int queueEnd = 0;


    int startIndex =
        (int)(start - map->tiles);


    visited[startIndex] =
        true;


    queue[queueEnd++] =
        start;


    bool found =
        false;


    while (queueStart < queueEnd)
    {
        HexTile* current =
            queue[queueStart++];


        if (current == end)
        {
            found = true;
            break;
        }


        for (int d = 0;
             d < 6;
             d++)
        {
            HexTile* neighbor =
                current->neighbors[d];


            if (neighbor == NULL)
                continue;


            if (!CanRoadPass(neighbor))
                continue;


            int index =
                (int)(neighbor - map->tiles);


            if (visited[index])
                continue;


            visited[index] =
                true;


            previous[index] =
                current;


            queue[queueEnd++] =
                neighbor;
        }
    }


    /*
        Si encontramos el destino,
        reconstruimos el camino.
    */
    if (found)
    {
        HexTile* current =
            end;


        while (current != NULL)
        {
            current->isRoad =
                true;


            if (current == start)
                break;


            int index =
                (int)(current - map->tiles);


            current =
                previous[index];
        }
    }


    free(visited);
    free(previous);
    free(queue);


    return found;
}


/*
    Crea algunas rutas adicionales.
*/
static void Map_CreateAlternativeRoads(
    Map* map
)
{
    if (map == NULL)
        return;


    int roadCount =
        0;


    for (int i = 0;
         i < map->tileCount;
         i++)
    {
        if (map->tiles[i].isRoad)
            roadCount++;
    }


    if (roadCount == 0)
        return;


    /*
        Cantidad de caminos alternativos.
    */
    int alternatives = 2;


    for (int i = 0;
         i < alternatives;
         i++)
    {
        HexTile* start =
            NULL;

        HexTile* end =
            NULL;


        /*
            Elegimos un punto que ya pertenezca
            a algún camino.
        */
        for (int attempts = 0;
             attempts < 100;
             attempts++)
        {
            int index =
                rand() % map->tileCount;


            HexTile* tile =
                &map->tiles[index];


            if (tile->isRoad)
            {
                start =
                    tile;

                break;
            }
        }


        if (start == NULL)
            continue;


        /*
            Buscamos un destino suficientemente
            alejado.
        */
        for (int attempts = 0;
             attempts < 100;
             attempts++)
        {
            int index =
                rand() % map->tileCount;


            HexTile* tile =
                &map->tiles[index];


            if (!CanRoadPass(tile))
                continue;


            if (tile == start)
                continue;


            if (HexDistance(
                    start->coord,
                    tile->coord
                ) < 3)
            {
                continue;
            }


            end =
                tile;

            break;
        }


        if (end != NULL)
        {
            Map_CreateRoad(
                map,
                start,
                end
            );
        }
    }
}


/* ============================================================
   GENERACIÓN DEL MAPA
   ============================================================ */

Map Map_GenerateProcedural(
    int mapRadius,
    int landProbabilityPercent,
    int endCount
)
{
    Map map;

    map.tiles = NULL;
    map.tileCount = 0;


    /*
        Cantidad máxima de hexágonos:

            1 + 3R(R+1)
    */
    int maxTiles =
        1 +
        3 * mapRadius *
        (mapRadius + 1);


    map.tiles =
        malloc(
            sizeof(HexTile) * maxTiles
        );


    if (map.tiles == NULL)
        return map;


    /*
        Limitamos el porcentaje de tierra.
    */
    if (landProbabilityPercent < 0)
        landProbabilityPercent = 0;


    if (landProbabilityPercent > 100)
        landProbabilityPercent = 100;


    /*
        Como mínimo un final.
    */
    if (endCount < 1)
        endCount = 1;


    /* ========================================================
       1. GENERAR TILES Y VALORES ALEATORIOS
       ======================================================== */

    float* terrainValues =
        malloc(
            sizeof(float) * maxTiles
        );


    if (terrainValues == NULL)
    {
        Map_Free(&map);
        return map;
    }


    for (int q = -mapRadius;
         q <= mapRadius;
         q++)
    {
        int r1 =
            (-mapRadius >
             -q - mapRadius)
            ? -mapRadius
            : -q - mapRadius;


        int r2 =
            (mapRadius <
             -q + mapRadius)
            ? mapRadius
            : -q + mapRadius;


        for (int r = r1;
             r <= r2;
             r++)
        {
            HexCoord coord =
                HexCoord_Create(q, r);

            /*
                Guardamos el valor aleatorio
                independientemente del terreno.
            */
            terrainValues[
                map.tileCount
            ] =
                RandomTerrainValue();

            /*
                Inicialmente todos son tierra.
                El terreno real se decide después
                del smoothing.
            */
            map.tiles[
                map.tileCount++
            ] =
                HexTile_Create(
                    coord,
                    1,
                    TERRAIN_LAND
                );
        }
    }


    /* ========================================================
       2. CONECTAR VECINOS
       ======================================================== */

    /*
        Esto debe hacerse ANTES del smoothing.
    */
    for (int i = 0;
         i < map.tileCount;
         i++)
    {
        for (int d = 0;
             d < 6;
             d++)
        {
            HexCoord target =
                HexCoord_GetNeighbor(
                    map.tiles[i].coord,
                    (HexDirection)d
                );


            HexTile* neighbor =
                Map_FindTile(
                    &map,
                    target
                );


            if (neighbor != NULL)
            {
                HexTile_Connect(
                    &map.tiles[i],
                    (HexDirection)d,
                    neighbor
                );
            }
        }
    }


    /* ========================================================
       3. SMOOTHING DEL TERRENO
       ======================================================== */

    /*
        Tres iteraciones generan agrupaciones de agua
        bastante más naturales que el random puro.
    */
    SmoothTerrain(
        &map,
        terrainValues,
        3
    );


    /* ========================================================
       4. CONVERTIR VALORES EN TIERRA / AGUA
       ======================================================== */

    for (int i = 0;
         i < map.tileCount;
         i++)
    {
        if (terrainValues[i] <
            landProbabilityPercent)
        {
            map.tiles[i].terrain =
                TERRAIN_LAND;
        }
        else
        {
            map.tiles[i].terrain =
                TERRAIN_WATER;
        }
    }


    free(terrainValues);


    /* ========================================================
       5. CREAR INICIO
       ======================================================== */

    HexTile* start =
        Map_GetRandomStartTile(
            &map
        );


    if (start != NULL)
    {
        start->special =
            TILE_SPECIAL_START;
    }


    /* ========================================================
       6. CREAR N FINALES
       ======================================================== */

    if (start != NULL)
    {
        for (int i = 0;
             i < endCount;
             i++)
        {
            HexTile* end =
                Map_GetRandomEndTile(
                    &map,
                    start
                );


            if (end == NULL)
                break;


            end->special =
                TILE_SPECIAL_END;
        }
    }


    /* ========================================================
       7. RÍOS
       ======================================================== */

    Map_GenerateRivers(&map);


    /* ========================================================
       8. CAMINOS DEL INICIO A CADA FINAL
       ======================================================== */

    if (start != NULL)
    {
        for (int i = 0;
             i < map.tileCount;
             i++)
        {
            HexTile* tile =
                &map.tiles[i];


            if (tile->special !=
                TILE_SPECIAL_END)
            {
                continue;
            }


            Map_CreateRoad(
                &map,
                start,
                tile
            );
        }
    }


    /* ========================================================
       9. CAMINOS ALTERNATIVOS
       ======================================================== */

    Map_CreateAlternativeRoads(
        &map
    );


    /* ========================================================
       10. PUENTES
       ======================================================== */

    /*
        Si un camino atraviesa un río,
        marcamos ese tile como puente.
    */
    for (int i = 0;
         i < map.tileCount;
         i++)
    {
        HexTile* tile =
            &map.tiles[i];


        if (tile->isRoad &&
            tile->river ==
                RIVER_PRESENT)
        {
            tile->bridge =
                BRIDGE_ABANDONED;
        }
    }


    return map;
}


/* ============================================================
   LIBERAR MAPA
   ============================================================ */

void Map_Free(
    Map* map
)
{
    if (map == NULL)
        return;


    free(map->tiles);


    map->tiles = NULL;
    map->tileCount = 0;
}