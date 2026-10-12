#include "map_generator.h"
#include "models/balance_config.h"

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
    int provinceLevel,
    int factionId
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
    int provinceLevel,
    int factionId
)
{
    (void)map;
    if (center == NULL || center->terrain != TERRAIN_LAND)
        return;

    /* Centro urbano del poblado: cuartel / ayuntamiento / castillo */
    center->structure.type = STRUCT_TOWN_CASTLE;
    center->structure.settlementId = settlementId;
    center->structure.settlementType = settlementType;
    center->structure.factionId = factionId;
    center->structure.isBesieged = false;

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

    for (int i = 0; i < count; i++) {
        available[i]->structure.factionId = center->structure.factionId;
        available[i]->structure.isBesieged = false;
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


static int Map_GetNearestFaction(HexCoord coord, const HexCoord factionSeeds[3])
{
    int bestFaction = 0;
    int minDistance = 999999;
    for (int f = 0; f < 3; f++)
    {
        int dist = HexDistance(coord, factionSeeds[f]);
        if (dist < minDistance)
        {
            minDistance = dist;
            bestFaction = f;
        }
    }
    return bestFaction;
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

    /* 3 semillas espaciales para facciones */
    HexCoord factionSeeds[3];
    int seedsFound = 0;
    for (int attempt = 0; attempt < 1000 && seedsFound < 3; attempt++)
    {
        int idx = rand() % map->tileCount;
        HexTile* t = &map->tiles[idx];
        if (t->terrain != TERRAIN_LAND) continue;
        bool farEnough = true;
        for (int s = 0; s < seedsFound; s++)
        {
            if (HexDistance(t->coord, factionSeeds[s]) < 12)
            {
                farEnough = false;
                break;
            }
        }
        if (farEnough || attempt > 800)
        {
            factionSeeds[seedsFound++] = t->coord;
        }
    }
    while (seedsFound < 3)
    {
        factionSeeds[seedsFound] = map->tiles[rand() % map->tileCount].coord;
        seedsFound++;
    }

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
        int fId = Map_GetNearestFaction(tile->coord, factionSeeds);
        Map_GenerateSettlement(map, tile, nextSettlementId++, SETTLEMENT_PRIMARY, provinceLevel, fId);
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
        int fId = Map_GetNearestFaction(tile->coord, factionSeeds);
        Map_GenerateSettlement(map, tile, nextSettlementId++, SETTLEMENT_SECONDARY, provinceLevel, fId);
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

    /* 11. Garantia de asentamientos (D1) y Postes indicadores (D2) */
    Map_EnsureSettlementGuarantee(&map, start, provinceLevel);
    Map_GenerateSignposts(&map);

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
HexTile* Map_GetTile(const Map* map, HexCoord coord) {
    for (int i = 0; i < map->tileCount; i++) {
        if (map->tiles[i].coord.q == coord.q && map->tiles[i].coord.r == coord.r) return &map->tiles[i];
    }
    return NULL;
}

static bool Structure_IsSettlement(StructureType type) {
    return (type == STRUCT_TOWN_CASTLE ||
            type == STRUCT_TOWN_MARKET ||
            type == STRUCT_TOWN_ARMORY ||
            type == STRUCT_TOWN_HOUSES);
}

void Map_UpdateVision(Map* map, HexCoord center, int visionRange) {
    if (!map) return;

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

    /* D2: Si una casilla con rumor pasa a estar en vista o visitada, se promueve a descubierta */
    for (int i = 0; i < map->tileCount; i++) {
        if (map->tiles[i].isRumor && (map->tiles[i].visible || map->tiles[i].visited)) {
            map->tiles[i].isRumor = false;
        }
    }

    /* D2: Al ver o visitar un poste indicador (STRUCT_SIGNPOST),
       revela el asentamiento mas cercano como rumor (isRumor = true) sin costo */
    for (int i = 0; i < map->tileCount; i++) {
        HexTile* signpost = &map->tiles[i];
        if (signpost->structure.type != STRUCT_SIGNPOST) continue;
        if (!signpost->visible && !signpost->visited) continue;

        /* Buscar el asentamiento mas cercano por red vial */
        HexTile** queue = (HexTile**)malloc(map->tileCount * sizeof(HexTile*));
        bool* visited = (bool*)calloc(map->tileCount, sizeof(bool));
        if (!queue || !visited) {
            free(queue); free(visited);
            continue;
        }

        int qStart = 0, qEnd = 0;
        int sIdx = (int)(signpost - map->tiles);
        visited[sIdx] = true;
        queue[qEnd++] = signpost;

        HexTile* nearestSettlementTile = NULL;

        while (qStart < qEnd) {
            HexTile* curr = queue[qStart++];

            if (curr != signpost && (Structure_IsSettlement(curr->structure.type) || curr->structure.settlementId > 0)) {
                nearestSettlementTile = curr;
                break;
            }

            for (int d = 0; d < 6; d++) {
                HexTile* nb = curr->neighbors[d];
                if (!nb) continue;
                if (!nb->isRoad && !Structure_IsSettlement(nb->structure.type)) continue;
                int nIdx = (int)(nb - map->tiles);
                if (!visited[nIdx]) {
                    visited[nIdx] = true;
                    queue[qEnd++] = nb;
                }
            }
        }

        free(queue);
        free(visited);

        if (nearestSettlementTile != NULL) {
            /* Encontrar el castillo / centro si existe */
            HexTile* markerTile = nearestSettlementTile;
            if (nearestSettlementTile->structure.settlementId > 0) {
                for (int k = 0; k < map->tileCount; k++) {
                    if (map->tiles[k].structure.settlementId == nearestSettlementTile->structure.settlementId &&
                        map->tiles[k].structure.type == STRUCT_TOWN_CASTLE) {
                        markerTile = &map->tiles[k];
                        break;
                    }
                }
            }

            /* Si no ha sido visitado, se marca como rumor */
            if (!markerTile->visited) {
                markerTile->isRumor = true;
            }
        }
    }
}





#define MAX_PATH_LENGTH 128

typedef struct AStarNode_s {
    HexTile* tile;
    int g;
    int f;
    struct AStarNode_s* cameFrom;
    bool closed;
    bool open;
} AStarNode;

// Retorna el tamaño del camino (0 si no hay camino). pathOut debe tener al menos MAX_PATH_LENGTH
int Map_FindPathEx(Map* map, HexTile* start, HexTile* end, bool ignoreFog, int* outTotalCost, HexTile** pathOut) {
    if (!map || !start || !end) return 0;
    if (start == end) return 0;

    // Reservar array temporal para AStarNodes
    AStarNode* nodes = (AStarNode*)calloc(map->tileCount, sizeof(AStarNode));
    for (int i = 0; i < map->tileCount; i++) {
        nodes[i].tile = &map->tiles[i];
        nodes[i].g = 999999;
        nodes[i].f = 999999;
    }

    // Funcion auxiliar para encontrar el nodo AStar de un HexTile
    int startIndex = -1, endIndex = -1;
    for (int i = 0; i < map->tileCount; i++) {
        if (&map->tiles[i] == start) startIndex = i;
        if (&map->tiles[i] == end) endIndex = i;
    }
    if (startIndex == -1 || endIndex == -1) {
        free(nodes);
        return 0;
    }

    nodes[startIndex].g = 0;
    nodes[startIndex].f = HexCoord_Distance(start->coord, end->coord);
    nodes[startIndex].open = true;

    while (true) {
        // Encontrar el nodo abierto con menor F
        int currentIdx = -1;
        int minF = 999999;
        for (int i = 0; i < map->tileCount; i++) {
            if (nodes[i].open && nodes[i].f < minF) {
                minF = nodes[i].f;
                currentIdx = i;
            }
        }

        if (currentIdx == -1) break; // No path found
        
        if (currentIdx == endIndex) {
            // Path found!
            int length = 0;
            AStarNode* curr = &nodes[endIndex];
            
            // Contar longitud (excluyendo el nodo de inicio)
            AStarNode* temp = curr;
            while (temp->cameFrom != NULL) {
                length++;
                temp = temp->cameFrom;
            }
            
            if (length > MAX_PATH_LENGTH) {
                free(nodes);
                return 0; // Camino muy largo
            }
            
            *outTotalCost = curr->g;
            
            // Reconstruir camino al reves
            int w = length - 1;
            while (curr->cameFrom != NULL) {
                pathOut[w] = curr->tile;
                // Buscar el cameFrom en el array
                for (int i=0; i<map->tileCount; i++) {
                    if (nodes[i].tile == curr->cameFrom->tile) {
                        curr = &nodes[i];
                        break;
                    }
                }
                w--;
            }
            
            free(nodes);
            return length;
        }

        nodes[currentIdx].open = false;
        nodes[currentIdx].closed = true;

        // Evaluar vecinos
        for (int i = 0; i < 6; i++) {
            HexTile* neighbor = nodes[currentIdx].tile->neighbors[i];
            if (!neighbor) continue;

            // Intransitable: agua (a menos que haya un puente)
            if ((neighbor->terrain == TERRAIN_WATER || neighbor->biome == BIOME_WATER) && neighbor->bridge == BRIDGE_NONE) {
                continue;
            }

            // Solo atravesar casillas conocidas para el jugador
            if (!ignoreFog) {
                if (!neighbor->visited && !neighbor->visible && neighbor != end) continue;
            }
            
            // Coste de entrar al vecino
            int cost = Biome_GetProperties(neighbor->biome).actionPointCost;
            if (neighbor->isRoad) cost = (cost > 1) ? cost - 1 : 1; // Basic road discount
            
            // Encontrar vecino en nodes
            int nIdx = -1;
            for (int j = 0; j < map->tileCount; j++) {
                if (nodes[j].tile == neighbor) {
                    nIdx = j;
                    break;
                }
            }
            
            if (nIdx == -1 || nodes[nIdx].closed) continue;
            
            int tentativeG = nodes[currentIdx].g + cost;
            if (!nodes[nIdx].open || tentativeG < nodes[nIdx].g) {
                nodes[nIdx].g = tentativeG;
                nodes[nIdx].f = tentativeG + HexCoord_Distance(neighbor->coord, end->coord);
                nodes[nIdx].cameFrom = &nodes[currentIdx];
                nodes[nIdx].open = true;
            }
        }
    }

    free(nodes);
    return 0; // No path
}

int Map_FindPath(Map* map, HexTile* start, HexTile* end, int* outTotalCost, HexTile** pathOut) {
    return Map_FindPathEx(map, start, end, false, outTotalCost, pathOut);
}

int Map_FindPathEnemy(Map* map, HexTile* start, HexTile* end, int* outTotalCost, HexTile** pathOut) {
    return Map_FindPathEx(map, start, end, true, outTotalCost, pathOut);
}

/* ============================================================
   BLOQUE D: GARANTÍA DE ASENTAMIENTOS Y POSTES INDICADORES
   ============================================================ */

void Map_EnsureSettlementGuarantee(Map* map, HexTile* start, int provinceLevel) {
    if (!map || map->tileCount <= 0 || !start) return;

    float maxDays = 2.0f;
    int hexPerDay = 9;
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (cfg != NULL) {
        if (cfg->maxDaysToSettlement > 0.01f) maxDays = cfg->maxDaysToSettlement;
        if (cfg->hexPerDay > 0) hexPerDay = cfg->hexPerDay;
    }
    int maxHexDist = (int)(maxDays * (float)hexPerDay + 0.5f);
    if (maxHexDist <= 0) maxHexDist = 18;

    /* Encontrar el ID de asentamiento más alto */
    int maxSettlementId = 0;
    for (int i = 0; i < map->tileCount; i++) {
        if (map->tiles[i].structure.settlementId > maxSettlementId) {
            maxSettlementId = map->tiles[i].structure.settlementId;
        }
    }
    int nextSettlementId = maxSettlementId + 1;

    /* 1. Identificar casillas de las rutas A (start) -> B (cada exit) */
    bool* onRoute = (bool*)calloc(map->tileCount, sizeof(bool));
    if (!onRoute) return;

    for (int i = 0; i < map->tileCount; i++) {
        HexTile* end = &map->tiles[i];
        if (end->special != TILE_SPECIAL_END) continue;

        HexTile** queue = (HexTile**)malloc(map->tileCount * sizeof(HexTile*));
        HexTile** prev = (HexTile**)calloc(map->tileCount, sizeof(HexTile*));
        bool* visited = (bool*)calloc(map->tileCount, sizeof(bool));
        if (!queue || !prev || !visited) {
            free(queue); free(prev); free(visited);
            continue;
        }

        int qStart = 0, qEnd = 0;
        int sIdx = (int)(start - map->tiles);
        visited[sIdx] = true;
        queue[qEnd++] = start;
        bool found = false;

        while (qStart < qEnd) {
            HexTile* curr = queue[qStart++];
            if (curr == end) {
                found = true;
                break;
            }
            for (int d = 0; d < 6; d++) {
                HexTile* nb = curr->neighbors[d];
                if (!nb || !nb->isRoad) continue;
                int nIdx = (int)(nb - map->tiles);
                if (!visited[nIdx]) {
                    visited[nIdx] = true;
                    prev[nIdx] = curr;
                    queue[qEnd++] = nb;
                }
            }
        }

        if (found) {
            HexTile* curr = end;
            while (curr != NULL) {
                int cIdx = (int)(curr - map->tiles);
                onRoute[cIdx] = true;
                if (curr == start) break;
                curr = prev[cIdx];
            }
        }

        free(queue);
        free(prev);
        free(visited);
    }

    /* 2. Bucle iterativo de garantia */
    int* roadDist = (int*)malloc(map->tileCount * sizeof(int));
    HexTile** bfsQueue = (HexTile**)malloc(map->tileCount * sizeof(HexTile*));
    if (!roadDist || !bfsQueue) {
        free(onRoute); free(roadDist); free(bfsQueue);
        return;
    }

    int safetyMaxIterations = 100;
    while (safetyMaxIterations-- > 0) {
        for (int i = 0; i < map->tileCount; i++) {
            roadDist[i] = -1;
        }

        int qStart = 0, qEnd = 0;
        for (int i = 0; i < map->tileCount; i++) {
            HexTile* t = &map->tiles[i];
            if (Structure_IsSettlement(t->structure.type) || t->structure.settlementId > 0) {
                roadDist[i] = 0;
                bfsQueue[qEnd++] = t;
            }
        }

        while (qStart < qEnd) {
            HexTile* curr = bfsQueue[qStart++];
            int cDist = roadDist[(int)(curr - map->tiles)];
            for (int d = 0; d < 6; d++) {
                HexTile* nb = curr->neighbors[d];
                if (!nb) continue;
                if (!nb->isRoad && !Structure_IsSettlement(nb->structure.type)) continue;
                int nIdx = (int)(nb - map->tiles);
                if (roadDist[nIdx] == -1 || roadDist[nIdx] > cDist + 1) {
                    roadDist[nIdx] = cDist + 1;
                    bfsQueue[qEnd++] = nb;
                }
            }
        }

        int worstDist = -1;
        HexTile* worstTile = NULL;

        for (int i = 0; i < map->tileCount; i++) {
            if (!onRoute[i]) continue;
            int d = roadDist[i];
            if (d == -1) {
                worstDist = 999999;
                worstTile = &map->tiles[i];
                break;
            } else if (d > maxHexDist && d > worstDist) {
                worstDist = d;
                worstTile = &map->tiles[i];
            }
        }

        if (worstTile == NULL || worstDist <= maxHexDist) {
            break; /* Garantia satisfecha */
        }

        /* Colocar Viviendas en casilla disponible en la ruta */
        HexTile* targetCandidate = NULL;
        if (worstTile->structure.type == STRUCT_NONE && worstTile->special == TILE_SPECIAL_NONE) {
            targetCandidate = worstTile;
        } else {
            for (int d = 0; d < 6; d++) {
                HexTile* nb = worstTile->neighbors[d];
                if (nb && onRoute[(int)(nb - map->tiles)] &&
                    nb->structure.type == STRUCT_NONE && nb->special == TILE_SPECIAL_NONE) {
                    targetCandidate = nb;
                    break;
                }
            }
            if (targetCandidate == NULL) {
                for (int i = 0; i < map->tileCount; i++) {
                    if (onRoute[i] && map->tiles[i].structure.type == STRUCT_NONE &&
                        map->tiles[i].special == TILE_SPECIAL_NONE) {
                        targetCandidate = &map->tiles[i];
                        break;
                    }
                }
            }
        }

        if (targetCandidate != NULL) {
            targetCandidate->structure.type = STRUCT_TOWN_HOUSES;
            targetCandidate->structure.settlementId = nextSettlementId++;
            targetCandidate->structure.settlementType = SETTLEMENT_SECONDARY;
            targetCandidate->structure.isGuarded = false;
            targetCandidate->structure.isBesieged = false;
            targetCandidate->structure.factionId = 0;
            targetCandidate->isRoad = true;
            if (targetCandidate->road == ROAD_NONE) {
                targetCandidate->road = ROAD_SECONDARY;
            }
        } else {
            break;
        }
    }

    free(onRoute);
    free(roadDist);
    free(bfsQueue);
}

void Map_GenerateSignposts(Map* map) {
    if (!map || map->tileCount <= 0) return;

    /* 1. Cruces de caminos (>= 3 vecinos con isRoad) */
    for (int i = 0; i < map->tileCount; i++) {
        HexTile* tile = &map->tiles[i];
        if (!tile->isRoad || tile->structure.type != STRUCT_NONE || tile->special != TILE_SPECIAL_NONE) {
            continue;
        }
        int roadNeighbors = 0;
        for (int d = 0; d < 6; d++) {
            if (tile->neighbors[d] && tile->neighbors[d]->isRoad) {
                roadNeighbors++;
            }
        }
        if (roadNeighbors >= 3) {
            tile->structure.type = STRUCT_SIGNPOST;
            tile->structure.settlementId = 0;
            tile->structure.settlementType = SETTLEMENT_NONE;
            tile->structure.isGuarded = false;
        }
    }

    /* 2. Postes espaciados cada ~8 casillas de camino */
    int* distToStruct = (int*)malloc(map->tileCount * sizeof(int));
    HexTile** queue = (HexTile**)malloc(map->tileCount * sizeof(HexTile*));
    if (!distToStruct || !queue) {
        free(distToStruct); free(queue);
        return;
    }

    for (int pass = 0; pass < 20; pass++) {
        for (int i = 0; i < map->tileCount; i++) {
            distToStruct[i] = -1;
        }
        int qStart = 0, qEnd = 0;

        for (int i = 0; i < map->tileCount; i++) {
            HexTile* t = &map->tiles[i];
            if (t->structure.type != STRUCT_NONE) {
                distToStruct[i] = 0;
                queue[qEnd++] = t;
            }
        }

        while (qStart < qEnd) {
            HexTile* curr = queue[qStart++];
            int cDist = distToStruct[(int)(curr - map->tiles)];
            for (int d = 0; d < 6; d++) {
                HexTile* nb = curr->neighbors[d];
                if (!nb || !nb->isRoad) continue;
                int nIdx = (int)(nb - map->tiles);
                if (distToStruct[nIdx] == -1 || distToStruct[nIdx] > cDist + 1) {
                    distToStruct[nIdx] = cDist + 1;
                    queue[qEnd++] = nb;
                }
            }
        }

        int maxDist = 0;
        HexTile* bestCand = NULL;
        for (int i = 0; i < map->tileCount; i++) {
            HexTile* t = &map->tiles[i];
            if (t->isRoad && t->structure.type == STRUCT_NONE && t->special == TILE_SPECIAL_NONE) {
                int d = distToStruct[i];
                if (d >= 8 && d > maxDist) {
                    maxDist = d;
                    bestCand = t;
                }
            }
        }

        if (bestCand != NULL && maxDist >= 8) {
            bestCand->structure.type = STRUCT_SIGNPOST;
            bestCand->structure.settlementId = 0;
            bestCand->structure.settlementType = SETTLEMENT_NONE;
            bestCand->structure.isGuarded = false;
        } else {
            break;
        }
    }

    free(distToStruct);
    free(queue);
}

