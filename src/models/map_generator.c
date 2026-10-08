#include "map_generator.h"

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>


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
    HexTile* end,
    RoadType roadType
);

static bool Map_ConnectToNearestRoad(
    Map* map,
    HexTile* origin,
    RoadType roadType,
    bool connectToMainOnly
);

static void Map_CreateAlternativeRoads(
    Map* map
);

static void Map_GenerateBiomes(
    Map* map,
    int provinceLevel
);

static void Map_GenerateSettlement(
    Map* map,
    HexTile* center,
    int settlementId,
    SettlementType settlementType,
    int provinceLevel
);

static void Map_GenerateStructures(
    Map* map,
    int provinceLevel
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
    Genera un camino entre dos tiles mediante BFS con tipo de carretera.
*/
static bool Map_CreateRoad(
    Map* map,
    HexTile* start,
    HexTile* end,
    RoadType roadType
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
        reconstruimos el camino asignando el tipo de ruta.
    */
    if (found)
    {
        HexTile* current =
            end;


        while (current != NULL)
        {
            if (roadType == ROAD_MAIN)
            {
                current->road = ROAD_MAIN;
            }
            else if (current->road == ROAD_NONE)
            {
                current->road = ROAD_SECONDARY;
            }
            current->isRoad = true;


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
    Conecta 'origin' al camino más cercano mediante BFS.
    Si 'connectToMainOnly' es true, busca conectarse a un ROAD_MAIN.
    De lo contrario, conecta a cualquier camino existente.
*/
static bool Map_ConnectToNearestRoad(
    Map* map,
    HexTile* origin,
    RoadType roadType,
    bool connectToMainOnly
)
{
    if (map == NULL || origin == NULL)
        return false;

    if (connectToMainOnly && origin->road == ROAD_MAIN)
        return true;
    if (!connectToMainOnly && origin->road != ROAD_NONE)
        return true;

    int tileCount = map->tileCount;
    bool* visited = calloc(tileCount, sizeof(bool));
    HexTile** previous = calloc(tileCount, sizeof(HexTile*));
    HexTile** queue = malloc(sizeof(HexTile*) * tileCount);

    if (visited == NULL || previous == NULL || queue == NULL)
    {
        free(visited);
        free(previous);
        free(queue);
        return false;
    }

    int queueStart = 0;
    int queueEnd = 0;

    int startIndex = (int)(origin - map->tiles);
    visited[startIndex] = true;
    queue[queueEnd++] = origin;

    HexTile* targetRoad = NULL;

    while (queueStart < queueEnd)
    {
        HexTile* current = queue[queueStart++];

        if (current != origin)
        {
            if (connectToMainOnly && current->road == ROAD_MAIN)
            {
                targetRoad = current;
                break;
            }
            else if (!connectToMainOnly && current->road != ROAD_NONE)
            {
                targetRoad = current;
                break;
            }
        }

        for (int d = 0; d < 6; d++)
        {
            HexTile* neighbor = current->neighbors[d];
            if (neighbor == NULL)
                continue;

            if (!CanRoadPass(neighbor))
                continue;

            int index = (int)(neighbor - map->tiles);
            if (visited[index])
                continue;

            visited[index] = true;
            previous[index] = current;
            queue[queueEnd++] = neighbor;
        }
    }

    bool success = false;
    if (targetRoad != NULL)
    {
        success = true;
        HexTile* curr = targetRoad;
        while (curr != NULL)
        {
            if (roadType == ROAD_MAIN)
            {
                curr->road = ROAD_MAIN;
            }
            else if (curr->road == ROAD_NONE)
            {
                curr->road = ROAD_SECONDARY;
            }
            curr->isRoad = true;

            if (curr == origin)
                break;

            int idx = (int)(curr - map->tiles);
            curr = previous[idx];
        }
    }

    free(visited);
    free(previous);
    free(queue);

    return success;
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
        Cantidad de caminos alternativos secundarios.
    */
    int alternatives = 3;


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
                ) < 4 ||
                HexDistance(
                    start->coord,
                    tile->coord
                ) > 18)
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
                end,
                ROAD_SECONDARY
            );
        }
    }
}


/* ============================================================
   GENERACIÓN DE BIOMAS Y ESTRUCTURAS
   ============================================================ */

static void Map_GenerateBiomes(
    Map* map,
    int provinceLevel
)
{
    if (map == NULL || map->tileCount <= 0)
        return;

    float* biomeValues =
        malloc(sizeof(float) * map->tileCount);

    if (biomeValues == NULL)
        return;

    for (int i = 0; i < map->tileCount; i++)
    {
        biomeValues[i] = RandomTerrainValue();
    }

    SmoothTerrain(map, biomeValues, 3);

    for (int i = 0; i < map->tileCount; i++)
    {
        HexTile* tile = &map->tiles[i];

        if (tile->terrain == TERRAIN_WATER)
        {
            tile->biome = BIOME_WATER;
            continue;
        }

        float v = biomeValues[i];

        switch (provinceLevel)
        {
            case 2:
                /* Nivel 2: Árido / Desiertos y Cañones */
                if (v < 55.0f)
                    tile->biome = BIOME_DESERT;
                else if (v < 80.0f)
                    tile->biome = BIOME_PLAINS;
                else
                    tile->biome = BIOME_MOUNTAIN;
                break;

            case 3:
                /* Nivel 3: Selvas, Junglas y Pantanos */
                if (v < 50.0f)
                    tile->biome = BIOME_JUNGLE;
                else if (v < 75.0f)
                    tile->biome = BIOME_FOREST;
                else if (v < 85.0f)
                    tile->biome = BIOME_PLAINS;
                else
                    tile->biome = BIOME_MOUNTAIN;
                break;

            case 4:
                /* Nivel 4: Metrópolis / Tierras fortificadas */
                if (v < 40.0f)
                    tile->biome = BIOME_PLAINS;
                else if (v < 65.0f)
                    tile->biome = BIOME_FOREST;
                else
                    tile->biome = BIOME_MOUNTAIN;
                break;

            case 1:
            default:
                /* Nivel 1: Templado / Llanuras y Bosques */
                if (v < 55.0f)
                    tile->biome = BIOME_PLAINS;
                else if (v < 85.0f)
                    tile->biome = BIOME_FOREST;
                else
                    tile->biome = BIOME_MOUNTAIN;
                break;
        }
    }

    free(biomeValues);
}


static void Map_GenerateSettlement(
    Map* map,
    HexTile* center,
    int settlementId,
    SettlementType settlementType,
    int provinceLevel
)
{
    (void)map;
    if (center == NULL || center->terrain != TERRAIN_LAND)
        return;

    /* Centro urbano del poblado: cuartel / ayuntamiento / castillo */
    center->structure.type = STRUCT_TOWN_CASTLE;
    center->structure.settlementId = settlementId;
    center->structure.settlementType = settlementType;

    if (settlementType == SETTLEMENT_PRIMARY)
    {
        center->structure.isGuarded = ((rand() % 100) < (35 + provinceLevel * 15));
        /* Tasa porcentual de spawn para escuadrón enemigo si está ocupado */
        center->structure.enemySpawnRate = center->structure.isGuarded ? 0.04f : 0.0f;
    }
    else
    {
        center->structure.isGuarded = ((rand() % 100) < (25 + provinceLevel * 15));
        center->structure.enemySpawnRate = center->structure.isGuarded ? 0.04f : 0.0f;
    }

    HexTile* available[6];
    int count = 0;

    for (int d = 0; d < 6; d++)
    {
        HexTile* n = center->neighbors[d];
        if (n != NULL &&
            n->terrain == TERRAIN_LAND &&
            n->special == TILE_SPECIAL_NONE &&
            n->river == RIVER_NONE &&
            n->structure.type == STRUCT_NONE)
        {
            available[count++] = n;
        }
    }

    /* Mezclar vecinos */
    for (int i = count - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        HexTile* tmp = available[i];
        available[i] = available[j];
        available[j] = tmp;
    }

    if (settlementType == SETTLEMENT_PRIMARY)
    {
        /* Ciudad Principal: centro urbano + mercado + armería + viviendas */
        if (count >= 1)
        {
            available[0]->structure.type = STRUCT_TOWN_MARKET;
            available[0]->structure.settlementId = settlementId;
            available[0]->structure.settlementType = settlementType;
            available[0]->structure.isGuarded = false;
        }
        if (count >= 2)
        {
            available[1]->structure.type = STRUCT_TOWN_ARMORY;
            available[1]->structure.settlementId = settlementId;
            available[1]->structure.settlementType = settlementType;
            available[1]->structure.isGuarded = ((rand() % 100) < (25 + provinceLevel * 10));
        }
        if (count >= 3)
        {
            available[2]->structure.type = STRUCT_TOWN_HOUSES;
            available[2]->structure.settlementId = settlementId;
            available[2]->structure.settlementType = settlementType;
            available[2]->structure.isGuarded = false;
        }
        if (count >= 4)
        {
            available[3]->structure.type = STRUCT_TOWN_HOUSES;
            available[3]->structure.settlementId = settlementId;
            available[3]->structure.settlementType = settlementType;
            available[3]->structure.isGuarded = false;
        }
    }
    else
    {
        /* Poblado Secundario: escala menor (1 servicio + 1 vivienda) */
        if (count >= 1)
        {
            available[0]->structure.type = (rand() % 2 == 0) ? STRUCT_TOWN_MARKET : STRUCT_TOWN_ARMORY;
            available[0]->structure.settlementId = settlementId;
            available[0]->structure.settlementType = settlementType;
            available[0]->structure.isGuarded = false;
        }
        if (count >= 2)
        {
            available[1]->structure.type = STRUCT_TOWN_HOUSES;
            available[1]->structure.settlementId = settlementId;
            available[1]->structure.settlementType = settlementType;
            available[1]->structure.isGuarded = false;
        }
    }
}


static void Map_GenerateStructures(
    Map* map,
    int provinceLevel
)
{
    if (map == NULL || map->tileCount <= 0)
        return;

    int nextSettlementId = 1;
    HexTile* settlementCenters[10];
    int placedSettlements = 0;

    /* 1. Ciudad Principal (1 cluster primario con centro urbano) */
    for (int attempt = 0; attempt < 800 && placedSettlements < 1; attempt++)
    {
        int idx = rand() % map->tileCount;
        HexTile* tile = &map->tiles[idx];

        if (tile->terrain != TERRAIN_LAND ||
            tile->special != TILE_SPECIAL_NONE ||
            tile->river == RIVER_PRESENT ||
            tile->structure.type != STRUCT_NONE)
        {
            continue;
        }

        int freeNeighbors = 0;
        for (int d = 0; d < 6; d++)
        {
            HexTile* n = tile->neighbors[d];
            if (n != NULL &&
                n->terrain == TERRAIN_LAND &&
                n->special == TILE_SPECIAL_NONE &&
                n->river == RIVER_NONE &&
                n->structure.type == STRUCT_NONE)
            {
                freeNeighbors++;
            }
        }
        if (freeNeighbors < 3)
            continue;

        settlementCenters[placedSettlements++] = tile;
        Map_GenerateSettlement(map, tile, nextSettlementId++, SETTLEMENT_PRIMARY, provinceLevel);
    }

    /* 2. Poblados Secundarios (2 a 3 clusters secundarios) */
    int secondaryTarget = 2;
    for (int attempt = 0; attempt < 1000 && placedSettlements < (1 + secondaryTarget); attempt++)
    {
        int idx = rand() % map->tileCount;
        HexTile* tile = &map->tiles[idx];

        if (tile->terrain != TERRAIN_LAND ||
            tile->special != TILE_SPECIAL_NONE ||
            tile->river == RIVER_PRESENT ||
            tile->structure.type != STRUCT_NONE)
        {
            continue;
        }

        int freeNeighbors = 0;
        for (int d = 0; d < 6; d++)
        {
            HexTile* n = tile->neighbors[d];
            if (n != NULL &&
                n->terrain == TERRAIN_LAND &&
                n->special == TILE_SPECIAL_NONE &&
                n->river == RIVER_NONE &&
                n->structure.type == STRUCT_NONE)
            {
                freeNeighbors++;
            }
        }
        if (freeNeighbors < 2)
            continue;

        bool tooClose = false;
        for (int s = 0; s < placedSettlements; s++)
        {
            if (HexDistance(tile->coord, settlementCenters[s]->coord) < 14)
            {
                tooClose = true;
                break;
            }
        }
        if (tooClose)
            continue;

        settlementCenters[placedSettlements++] = tile;
        Map_GenerateSettlement(map, tile, nextSettlementId++, SETTLEMENT_SECONDARY, provinceLevel);
    }

    /* 3. Altares Divinos (3 altares) */
    int altarsToPlace = 3;
    int placedAltars = 0;
    for (int attempt = 0; attempt < 800 && placedAltars < altarsToPlace; attempt++)
    {
        int idx = rand() % map->tileCount;
        HexTile* tile = &map->tiles[idx];

        if (tile->terrain != TERRAIN_LAND ||
            tile->special != TILE_SPECIAL_NONE ||
            tile->river == RIVER_PRESENT ||
            tile->structure.type != STRUCT_NONE)
        {
            continue;
        }

        bool nearTown = false;
        for (int d = 0; d < 6; d++)
        {
            if (tile->neighbors[d] != NULL &&
                tile->neighbors[d]->structure.settlementId > 0)
            {
                nearTown = true;
                break;
            }
        }
        if (nearTown)
            continue;

        tile->structure.type = STRUCT_ALTAR;
        tile->structure.isGuarded = ((rand() % 100) < (45 + provinceLevel * 15));
        tile->structure.divineCardId = (rand() % 6) + 1;
        placedAltars++;
    }

    /* 4. Atalayas (3 atalayas) */
    int towersToPlace = 3;
    int placedTowers = 0;
    for (int attempt = 0; attempt < 800 && placedTowers < towersToPlace; attempt++)
    {
        int idx = rand() % map->tileCount;
        HexTile* tile = &map->tiles[idx];

        if (tile->terrain != TERRAIN_LAND ||
            tile->special != TILE_SPECIAL_NONE ||
            tile->river == RIVER_PRESENT ||
            tile->structure.type != STRUCT_NONE)
        {
            continue;
        }

        tile->structure.type = STRUCT_WATCHTOWER;
        tile->structure.isGuarded = ((rand() % 100) < (25 + provinceLevel * 10));
        placedTowers++;
    }
}


/* ============================================================
   GENERACIÓN DEL MAPA
   ============================================================ */

Map Map_GenerateProcedural(
    int mapRadius,
    int landProbabilityPercent,
    int endCount,
    int provinceLevel
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
       1. GENERAR TILES CON FORMA ORGÁNICA
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


    /*
        Parámetros para silueta orgánica:
        Deformación armónica basada en ángulo polar desde el centro del mapa.
    */
    float phase1 = ((float)(rand() % 628)) / 100.0f;
    float phase2 = ((float)(rand() % 628)) / 100.0f;
    float phase3 = ((float)(rand() % 628)) / 100.0f;
    float phase4 = ((float)(rand() % 628)) / 100.0f;

    /* Tabla de búsqueda espacial 2D rápida para conectar vecinos en O(1) */
    int gridWidth = 2 * mapRadius + 1;
    HexTile** gridLookup =
        calloc(
            gridWidth * gridWidth,
            sizeof(HexTile*)
        );


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

            /* Coordenadas euclidianas relativas al centro */
            float px =
                sqrtf(3.0f) * ((float)q + (float)r / 2.0f);
            float py =
                1.5f * (float)r;
            float dist =
                sqrtf(px * px + py * py);
            float angle =
                atan2f(py, px);

            /* Límite orgánico con armónicos naturales */
            float maxR =
                (float)mapRadius * 1.55f * (
                    0.74f
                    + 0.16f * sinf(2.0f * angle + phase1)
                    + 0.10f * cosf(3.0f * angle + phase2)
                    + 0.06f * sinf(5.0f * angle + phase3)
                    + 0.04f * cosf(7.0f * angle + phase4)
                );

            if (dist > maxR)
            {
                /* Fuera de la frontera orgánica de la provincia */
                continue;
            }

            terrainValues[
                map.tileCount
            ] =
                RandomTerrainValue();

            map.tiles[
                map.tileCount
            ] =
                HexTile_Create(
                    coord,
                    1,
                    TERRAIN_LAND
                );

            if (gridLookup != NULL)
            {
                int gx = q + mapRadius;
                int gy = r + mapRadius;
                if (gx >= 0 && gx < gridWidth && gy >= 0 && gy < gridWidth)
                {
                    gridLookup[gx * gridWidth + gy] =
                        &map.tiles[map.tileCount];
                }
            }

            map.tileCount++;
        }
    }


    /* ========================================================
       2. CONECTAR VECINOS EN O(1)
       ======================================================== */

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

            HexTile* neighbor = NULL;

            if (gridLookup != NULL)
            {
                int gx = target.q + mapRadius;
                int gy = target.r + mapRadius;

                if (gx >= 0 && gx < gridWidth && gy >= 0 && gy < gridWidth)
                {
                    neighbor =
                        gridLookup[gx * gridWidth + gy];
                }
            }
            else
            {
                neighbor =
                    Map_FindTile(
                        &map,
                        target
                    );
            }

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

    if (gridLookup != NULL)
    {
        free(gridLookup);
    }


    /* ========================================================
       3. SMOOTHING DEL TERRENO
       ======================================================== */

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
       4b. GENERAR BIOMAS POR NIVEL DE PROVINCIA
       ======================================================== */

    Map_GenerateBiomes(
        &map,
        provinceLevel
    );


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
       8. GENERAR ESTRUCTURAS (PUEBLOS, ALTARES, ATALAYAS)
       ======================================================== */

    Map_GenerateStructures(
        &map,
        provinceLevel
    );


    /* ========================================================
       9. CAMINOS PRINCIPALES Y SECUNDARIOS
       ======================================================== */

    /* 9a. Camino Principal: De Inicio a cada Salida */
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
                tile,
                ROAD_MAIN
            );
        }
    }

    /* 9b. Conectar Centro Urbano de Ciudad Principal al Camino Principal */
    for (int i = 0;
         i < map.tileCount;
         i++)
    {
        HexTile* tile =
            &map.tiles[i];

        if (tile->structure.type == STRUCT_TOWN_CASTLE &&
            tile->structure.settlementType == SETTLEMENT_PRIMARY)
        {
            Map_ConnectToNearestRoad(
                &map,
                tile,
                ROAD_MAIN,
                true
            );
        }
    }

    /* 9c. Conectar Centro Urbano de Poblados Secundarios a la red con Camino Secundario */
    for (int i = 0;
         i < map.tileCount;
         i++)
    {
        HexTile* tile =
            &map.tiles[i];

        if (tile->structure.type == STRUCT_TOWN_CASTLE &&
            tile->structure.settlementType == SETTLEMENT_SECONDARY)
        {
            Map_ConnectToNearestRoad(
                &map,
                tile,
                ROAD_SECONDARY,
                false
            );
        }
    }

    /* 9d. Conectar Altares Divinos con Camino Secundario */
    for (int i = 0;
         i < map.tileCount;
         i++)
    {
        HexTile* tile =
            &map.tiles[i];

        if (tile->structure.type == STRUCT_ALTAR)
        {
            Map_ConnectToNearestRoad(
                &map,
                tile,
                ROAD_SECONDARY,
                false
            );
        }
    }

    /* 9e. Conectar Atalayas con Camino Secundario */
    for (int i = 0;
         i < map.tileCount;
         i++)
    {
        HexTile* tile =
            &map.tiles[i];

        if (tile->structure.type == STRUCT_WATCHTOWER)
        {
            Map_ConnectToNearestRoad(
                &map,
                tile,
                ROAD_SECONDARY,
                false
            );
        }
    }

    /* 9f. Senderos alternativos secundarios */
    Map_CreateAlternativeRoads(
        &map
    );


    /* ========================================================
       10. PUENTES
       ======================================================== */

    /*
        Si un camino atraviesa un río,
        marcamos ese tile como puente y fijamos peaje según tipo de camino.
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

            tile->structure.type =
                STRUCT_BRIDGE;

            tile->structure.isGuarded =
                ((rand() % 100) < (30 + provinceLevel * 15));

            tile->structure.tollCost =
                (tile->road == ROAD_MAIN)
                    ? (20 + provinceLevel * 5)
                    : (10 + provinceLevel * 4);
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
void Map_UpdateVision(Map* map, HexCoord center, int visionRange) {
    for (int i = 0; i < map->tileCount; i++) {
        map->tiles[i].visible = false;
    }
    for (int i = 0; i < map->tileCount; i++) {
        int dist = HexCoord_Distance(map->tiles[i].coord, center);
        if (dist <= visionRange) {
            map->tiles[i].visible = true;
            map->tiles[i].visited = true;
        } else if (dist <= visionRange + 2) {
            map->tiles[i].visited = true;
        }
    }
}



