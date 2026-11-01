#include "map_generator.h"
#include "models/balance_config.h"

#include <stdlib.h>
#include <stdio.h>
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

static HexTile* Map_PlaceBastion(
    Map* map,
    int maxOffset,
    int* outDisplacement
);

static HexTile* Map_GetStartCoastTile(
    Map* map,
    HexCoord bastionCoord,
    float minDays,
    int hexPerDay
);

static void Map_CreateRiver(
    Map* map,
    HexTile* start
);

static void Map_GenerateRivers(
    Map* map
);

static bool CanRoadPassEx(
    const HexTile* tile,
    const HexTile* end
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


static bool Map_IsValidBastionSite(const Map* map, const HexTile* center) {
    if (!center || center->terrain != TERRAIN_LAND || center->biome == BIOME_WATER) {
        return false;
    }
    int landNeighbors = 0;
    for (int d = 0; d < 6; d++) {
        const HexTile* nb = center->neighbors[d];
        if (!nb || nb->terrain != TERRAIN_LAND || nb->biome == BIOME_WATER) {
            return false;
        }
        landNeighbors++;
    }
    if (landNeighbors != 6) return false;

    /* Verificar que pertenezca a la masa continental principal (al menos 300 casillas de tierra conectadas) */
    if (!map || map->tileCount <= 0) return true;
    bool* visited = (bool*)calloc(map->tileCount, sizeof(bool));
    const HexTile** queue = (const HexTile**)malloc(sizeof(HexTile*) * map->tileCount);
    if (!visited || !queue) {
        free(visited);
        free((void*)queue);
        return true;
    }

    int qStart = 0, qEnd = 0;
    int sIdx = (int)(center - map->tiles);
    visited[sIdx] = true;
    queue[qEnd++] = center;
    int landCount = 0;

    while (qStart < qEnd && landCount < 300) {
        const HexTile* curr = queue[qStart++];
        landCount++;
        for (int d = 0; d < 6; d++) {
            const HexTile* n = curr->neighbors[d];
            if (!n || n->terrain != TERRAIN_LAND || n->biome == BIOME_WATER) continue;
            int nIdx = (int)(n - map->tiles);
            if (!visited[nIdx]) {
                visited[nIdx] = true;
                queue[qEnd++] = n;
            }
        }
    }

    free(visited);
    free((void*)queue);
    return (landCount >= 300);
}

/*
    Ubica el bastión central (huella 7 amurallada) en (0, 0)
    o el sitio válido más cercano en espiral.
*/
static HexTile* Map_PlaceBastion(
    Map* map,
    int maxOffset,
    int* outDisplacement
)
{
    if (map == NULL || map->tileCount <= 0) return NULL;
    if (outDisplacement) *outDisplacement = 0;

    HexCoord centerCoord = HexCoord_Create(0, 0);
    HexTile* centerTile = Map_FindTile(map, centerCoord);

    if (centerTile != NULL && Map_IsValidBastionSite(map, centerTile)) {
        if (outDisplacement) *outDisplacement = 0;
        centerTile->structure.type = STRUCT_TOWN_CASTLE;
        centerTile->structure.settlementType = SETTLEMENT_BASTION;
        centerTile->structure.isBastion = true;
        centerTile->structure.settlementId = 1;
        centerTile->structure.isGuarded = true;
        centerTile->structure.isBesieged = false;
        centerTile->footprintRole = FOOTPRINT_ROLE_ANCHOR;
        centerTile->footprintAnchor = centerTile->coord;
        centerTile->visited = true;

        for (int d = 0; d < 6; d++) {
            HexTile* nb = centerTile->neighbors[d];
            if (nb) {
                nb->structure = centerTile->structure;
                nb->footprintRole = FOOTPRINT_ROLE_WALL;
                nb->footprintAnchor = centerTile->coord;
                nb->visited = true;
            }
        }
        map->bastionCoord = centerTile->coord;
        return centerTile;
    }

    /* Búsqueda en espiral hacia afuera */
    HexTile* bestCandidate = NULL;
    int bestDist = 999999;

    for (int i = 0; i < map->tileCount; i++) {
        HexTile* t = &map->tiles[i];
        int dist = HexDistance(t->coord, centerCoord);
        if (dist > maxOffset) continue;

        if (Map_IsValidBastionSite(map, t)) {
            if (dist < bestDist) {
                bestDist = dist;
                bestCandidate = t;
                if (bestDist == 1) break;
            }
        }
    }

    if (bestCandidate != NULL) {
        if (outDisplacement) *outDisplacement = bestDist;
        bestCandidate->structure.type = STRUCT_TOWN_CASTLE;
        bestCandidate->structure.settlementType = SETTLEMENT_BASTION;
        bestCandidate->structure.isBastion = true;
        bestCandidate->structure.settlementId = 1;
        bestCandidate->structure.isGuarded = true;
        bestCandidate->structure.isBesieged = false;
        bestCandidate->footprintRole = FOOTPRINT_ROLE_ANCHOR;
        bestCandidate->footprintAnchor = bestCandidate->coord;
        bestCandidate->visited = true;

        for (int d = 0; d < 6; d++) {
            HexTile* nb = bestCandidate->neighbors[d];
            if (nb) {
                nb->structure = bestCandidate->structure;
                nb->footprintRole = FOOTPRINT_ROLE_WALL;
                nb->footprintAnchor = bestCandidate->coord;
                nb->visited = true;
            }
        }
        map->bastionCoord = bestCandidate->coord;
        return bestCandidate;
    }

    return NULL;
}

static bool IsCoastTile(const Map* map, const HexTile* tile) {
    (void)map;
    if (!tile || tile->terrain != TERRAIN_LAND || tile->biome == BIOME_WATER) return false;
    for (int d = 0; d < 6; d++) {
        HexTile* nb = tile->neighbors[d];
        if (nb == NULL || nb->terrain == TERRAIN_WATER || nb->biome == BIOME_WATER) {
            return true;
        }
    }
    return false;
}

/*
    Busca un tile de inicio costero a distancia >= minDays en un ángulo aleatorio,
    perteneciente a la misma masa continental del Bastión (conectado por tierra).
*/
static HexTile* Map_GetStartCoastTile(
    Map* map,
    HexCoord bastionCoord,
    float minDays,
    int hexPerDay
)
{
    if (map == NULL || map->tileCount <= 0) return NULL;

    HexTile* bastionTile = Map_FindTile(map, bastionCoord);
    if (bastionTile == NULL) return NULL;

    /* Flood fill desde el bastión para identificar la masa continental conectada por tierra */
    bool* onMainland = (bool*)calloc(map->tileCount, sizeof(bool));
    HexTile** queue = (HexTile**)malloc(sizeof(HexTile*) * map->tileCount);
    if (onMainland == NULL || queue == NULL)
    {
        free(onMainland);
        free(queue);
        return NULL;
    }

    int qStart = 0, qEnd = 0;
    int bIdx = (int)(bastionTile - map->tiles);
    onMainland[bIdx] = true;
    queue[qEnd++] = bastionTile;

    while (qStart < qEnd)
    {
        HexTile* curr = queue[qStart++];
        for (int d = 0; d < 6; d++)
        {
            HexTile* n = curr->neighbors[d];
            if (!n || n->terrain != TERRAIN_LAND || n->biome == BIOME_WATER) continue;
            int nIdx = (int)(n - map->tiles);
            if (!onMainland[nIdx])
            {
                onMainland[nIdx] = true;
                queue[qEnd++] = n;
            }
        }
    }
    free(queue);

    int minHexDist = (int)(minDays * (float)hexPerDay + 0.5f);
    if (minHexDist <= 0) minHexDist = 32;

    float targetAngle = ((float)(rand() % 628)) / 100.0f;

    HexTile* bestTile = NULL;
    float bestAngleDiff = 999.0f;
    int fallbackMaxDist = -1;
    HexTile* fallbackTile = NULL;

    for (int i = 0; i < map->tileCount; i++) {
        if (!onMainland[i]) continue;
        HexTile* tile = &map->tiles[i];
        if (!IsCoastTile(map, tile)) continue;

        int dist = HexDistance(tile->coord, bastionCoord);
        if (dist > fallbackMaxDist) {
            fallbackMaxDist = dist;
            fallbackTile = tile;
        }

        if (dist >= minHexDist) {
            float px = sqrtf(3.0f) * ((float)(tile->coord.q - bastionCoord.q) + (float)(tile->coord.r - bastionCoord.r) / 2.0f);
            float py = 1.5f * (float)(tile->coord.r - bastionCoord.r);
            float angle = atan2f(py, px);
            if (angle < 0.0f) angle += 6.2831853f;

            float diff = fabsf(angle - targetAngle);
            if (diff > 3.14159265f) diff = 6.2831853f - diff;

            if (diff < bestAngleDiff) {
                bestAngleDiff = diff;
                bestTile = tile;
            }
        }
    }

    free(onMainland);

    if (bestTile != NULL) return bestTile;
    if (fallbackTile != NULL) return fallbackTile;
    return Map_GetRandomStartTile(map);
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
        (map->bastionCoord.q != 0 || map->bastionCoord.r != 0)
        ? map->bastionCoord
        : HexCoord_Create(0, 0);


    for (int step = 0;
         step < map->tileCount;
         step++)
    {
        /* El río frena a distancia 3 del bastión (Decisión 6) */
        if (HexDistance(current->coord, center) <= 3)
        {
            break;
        }

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
                el río que acabamos de crear ni atravesar huellas de estructuras.
            */
            if (neighbor->river ==
                RIVER_PRESENT)
            {
                continue;
            }

            if (neighbor->footprintRole != FOOTPRINT_ROLE_NONE ||
                neighbor->structure.type != STRUCT_NONE)
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
            El río termina a distancia 3 del centro/bastión.
        */
        if (HexDistance(
                current->coord,
                center
            ) <= 3)
        {
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
static bool CanRoadPassEx(
    const HexTile* tile,
    const HexTile* end
)
{
    if (tile == NULL)
        return false;

    if (tile->terrain == TERRAIN_WATER)
        return false;

    /* Las murallas son infranqueables salvo para acceder a la estructura destino (bastión) */
    if (tile->footprintRole == FOOTPRINT_ROLE_WALL)
    {
        if (end != NULL && (tile == end || (tile->footprintAnchor.q == end->coord.q && tile->footprintAnchor.r == end->coord.r)))
        {
            return true;
        }
        return false;
    }

    return true;
}

static bool CanRoadPass(
    const HexTile* tile
)
{
    return CanRoadPassEx(tile, NULL);
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


            if (!CanRoadPassEx(neighbor, end))
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
    if (!connectToMainOnly && origin->road != ROAD_NONE && origin->structure.settlementId == 0)
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
            if (origin->structure.settlementId > 0 && current->structure.settlementId == origin->structure.settlementId)
            {
                /* Es un camino interno de la misma aldea, continuar buscando hacia afuera */
            }
            else if (connectToMainOnly && current->road == ROAD_MAIN)
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


/*
    Coloca una huella de 3 casillas mutuamente adyacentes (un triángulo hexagonal)
    donde 'anchor' es la casilla central/referencia y 2 vecinos contiguos son el cuerpo.
*/
static bool Map_TryPlaceFootprint3(
    Map* map,
    HexTile* anchor,
    StructureType type,
    SettlementType sType,
    int settlementId,
    int factionId,
    bool isGuarded
)
{
    (void)map;
    if (anchor == NULL || anchor->terrain != TERRAIN_LAND ||
        anchor->river != RIVER_NONE || anchor->special != TILE_SPECIAL_NONE ||
        anchor->structure.type != STRUCT_NONE || anchor->footprintRole != FOOTPRINT_ROLE_NONE)
    {
        return false;
    }

    int startD = rand() % 6;
    for (int step = 0; step < 6; step++)
    {
        int d1 = (startD + step) % 6;
        int d2 = (d1 + 1) % 6;
        HexTile* n1 = anchor->neighbors[d1];
        HexTile* n2 = anchor->neighbors[d2];

        if (n1 != NULL && n2 != NULL &&
            n1->terrain == TERRAIN_LAND && n2->terrain == TERRAIN_LAND &&
            n1->special == TILE_SPECIAL_NONE && n2->special == TILE_SPECIAL_NONE &&
            n1->river == RIVER_NONE && n2->river == RIVER_NONE &&
            n1->structure.type == STRUCT_NONE && n2->structure.type == STRUCT_NONE &&
            n1->footprintRole == FOOTPRINT_ROLE_NONE && n2->footprintRole == FOOTPRINT_ROLE_NONE)
        {
            anchor->structure.type = type;
            anchor->structure.settlementId = settlementId;
            anchor->structure.settlementType = sType;
            anchor->structure.factionId = factionId;
            anchor->structure.isGuarded = isGuarded;
            anchor->structure.isBesieged = false;
            anchor->footprintRole = FOOTPRINT_ROLE_ANCHOR;
            anchor->footprintAnchor = anchor->coord;

            n1->structure = anchor->structure;
            n1->footprintRole = FOOTPRINT_ROLE_BODY;
            n1->footprintAnchor = anchor->coord;

            n2->structure = anchor->structure;
            n2->footprintRole = FOOTPRINT_ROLE_BODY;
            n2->footprintAnchor = anchor->coord;

            return true;
        }
    }

    return false;
}

static void Map_PlaceFootprint1(
    HexTile* anchor,
    StructureType type,
    SettlementType sType,
    int settlementId,
    int factionId,
    bool isGuarded
)
{
    if (anchor == NULL) return;
    anchor->structure.type = type;
    anchor->structure.settlementId = settlementId;
    anchor->structure.settlementType = sType;
    anchor->structure.factionId = factionId;
    anchor->structure.isGuarded = isGuarded;
    anchor->structure.isBesieged = false;
    anchor->footprintRole = FOOTPRINT_ROLE_ANCHOR;
    anchor->footprintAnchor = anchor->coord;
}

/*
    Busca una casilla a distancia 2-3 del centro y coloca una estructura
    con la huella deseada (3 o 1 casilla). Si la de 3 no cabe tras buscar,
    se reduce a 1 casilla y se registra en debug.
*/
static HexTile* Map_PlaceServiceStructure(
    Map* map,
    HexTile* center,
    StructureType type,
    SettlementType sType,
    int settlementId,
    int factionId,
    bool isGuarded,
    int desiredFootprintSize
)
{
    if (map == NULL || center == NULL) return NULL;

    HexTile* candidates[64];
    int candCount = 0;

    for (int i = 0; i < map->tileCount; i++)
    {
        HexTile* t = &map->tiles[i];
        if (t->terrain != TERRAIN_LAND || t->river != RIVER_NONE ||
            t->special != TILE_SPECIAL_NONE || t->structure.type != STRUCT_NONE ||
            t->footprintRole != FOOTPRINT_ROLE_NONE)
        {
            continue;
        }
        int dist = HexDistance(t->coord, center->coord);
        if (dist >= 2 && dist <= 3)
        {
            candidates[candCount++] = t;
            if (candCount >= 64) break;
        }
    }

    if (candCount == 0) return NULL;

    /* Mezclar candidatos */
    for (int i = candCount - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        HexTile* tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }

    if (desiredFootprintSize == 3)
    {
        for (int i = 0; i < candCount; i++)
        {
            if (Map_TryPlaceFootprint3(map, candidates[i], type, sType, settlementId, factionId, isGuarded))
            {
                return candidates[i];
            }
        }
        printf("[DEBUG] Footprint 3 no cupo para estructura tipo %d en asentamiento %d; reduciendo a 1 casilla.\n",
               type, settlementId);
    }

    Map_PlaceFootprint1(candidates[0], type, sType, settlementId, factionId, isGuarded);
    return candidates[0];
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
    if (map == NULL || center == NULL || center->terrain != TERRAIN_LAND)
        return;

    HexTile* serviceAnchors[6];
    int serviceCount = 0;

    if (settlementType == SETTLEMENT_PRIMARY)
    {
        /* Ciudad Principal:
           - Castillo: huella 3 (sin muralla)
           - Mercado: huella 3 a distancia 2-3
           - Armería: huella 3 a distancia 2-3
           - Casas: 2 de huella 1 a distancia 2-3 */
        bool castleGuarded = ((rand() % 100) < (35 + provinceLevel * 15));
        if (!Map_TryPlaceFootprint3(map, center, STRUCT_TOWN_CASTLE, SETTLEMENT_PRIMARY, settlementId, factionId, castleGuarded))
        {
            Map_PlaceFootprint1(center, STRUCT_TOWN_CASTLE, SETTLEMENT_PRIMARY, settlementId, factionId, castleGuarded);
        }
        center->structure.enemySpawnRate = castleGuarded ? 0.04f : 0.0f;

        HexTile* market = Map_PlaceServiceStructure(map, center, STRUCT_TOWN_MARKET, SETTLEMENT_PRIMARY, settlementId, factionId, false, 3);
        if (market) serviceAnchors[serviceCount++] = market;

        bool armoryGuarded = ((rand() % 100) < (25 + provinceLevel * 10));
        HexTile* armory = Map_PlaceServiceStructure(map, center, STRUCT_TOWN_ARMORY, SETTLEMENT_PRIMARY, settlementId, factionId, armoryGuarded, 3);
        if (armory) serviceAnchors[serviceCount++] = armory;

        HexTile* h1 = Map_PlaceServiceStructure(map, center, STRUCT_TOWN_HOUSES, SETTLEMENT_PRIMARY, settlementId, factionId, false, 1);
        if (h1) serviceAnchors[serviceCount++] = h1;

        HexTile* h2 = Map_PlaceServiceStructure(map, center, STRUCT_TOWN_HOUSES, SETTLEMENT_PRIMARY, settlementId, factionId, false, 1);
        if (h2) serviceAnchors[serviceCount++] = h2;
    }
    else
    {
        /* Poblado Secundario:
           - Centro: huella 1
           - Servicio (Mercado o Armería): huella 1 a distancia 2-3
           - Casas: huella 1 a distancia 2-3 */
        bool castleGuarded = ((rand() % 100) < (25 + provinceLevel * 15));
        Map_PlaceFootprint1(center, STRUCT_TOWN_CASTLE, SETTLEMENT_SECONDARY, settlementId, factionId, castleGuarded);
        center->structure.enemySpawnRate = castleGuarded ? 0.04f : 0.0f;

        StructureType sType = (rand() % 2 == 0) ? STRUCT_TOWN_MARKET : STRUCT_TOWN_ARMORY;
        HexTile* svc = Map_PlaceServiceStructure(map, center, sType, SETTLEMENT_SECONDARY, settlementId, factionId, false, 1);
        if (svc) serviceAnchors[serviceCount++] = svc;

        HexTile* houses = Map_PlaceServiceStructure(map, center, STRUCT_TOWN_HOUSES, SETTLEMENT_SECONDARY, settlementId, factionId, false, 1);
        if (houses) serviceAnchors[serviceCount++] = houses;
    }

    /* Conectar el centro del cluster con cada servicio mediante caminos secundarios */
    for (int i = 0; i < serviceCount; i++)
    {
        if (serviceAnchors[i] != NULL)
        {
            Map_CreateRoad(map, center, serviceAnchors[i], ROAD_SECONDARY);
        }
    }
}

/*
    Calcula a qué sector angular (0, 1 o 2) pertenece una casilla respecto al origen (Bastión).
*/
static int Map_GetAngleSector(HexCoord origin, HexCoord coord, float baseAngle)
{
    float dx = sqrtf(3.0f) * ((float)(coord.q - origin.q) + (float)(coord.r - origin.r) / 2.0f);
    float dy = 1.5f * (float)(coord.r - origin.r);
    float angle = atan2f(dy, dx) - baseAngle;
    while (angle < 0.0f) angle += 2.0f * 3.14159265f;
    while (angle >= 2.0f * 3.14159265f) angle -= 2.0f * 3.14159265f;
    int sector = (int)(angle / (2.0f * 3.14159265f / 3.0f));
    if (sector < 0) sector = 0;
    if (sector > 2) sector = 2;
    return sector;
}

static bool Map_IsInConcentricBand(int dist, int band)
{
    if (band == 0) return (dist >= 28 && dist <= 35); // Interior
    if (band == 1) return (dist >= 36 && dist <= 45); // Medio
    if (band == 2) return (dist >= 46 && dist <= 65); // Exterior
    return false;
}

static void Map_GenerateStructures(
    Map* map,
    int provinceLevel
)
{
    if (map == NULL || map->tileCount <= 0)
        return;

    const BalanceConfig* bCfg = Balance_GetGlobal();
    int minClusterDist = (bCfg && bCfg->minClusterDistance > 0) ? bCfg->minClusterDistance : 22;
    int minBastionDist = (bCfg && bCfg->minBastionDistance > 0) ? bCfg->minBastionDistance : 28;
    int minStartDist   = (bCfg && bCfg->minStartClusterDistance > 0) ? bCfg->minStartClusterDistance : 20;

    HexCoord bastionCoord = map->bastionCoord;
    HexCoord startCoord   = map->startCoord;

    /* Identificar masa continental conectada al Bastión */
    HexTile* bastionTile = Map_FindTile(map, bastionCoord);
    bool* onMainland = (bool*)calloc(map->tileCount, sizeof(bool));
    HexTile** mainlandQueue = (HexTile**)malloc(sizeof(HexTile*) * map->tileCount);
    if (bastionTile && onMainland && mainlandQueue) {
        int mqStart = 0, mqEnd = 0;
        int bIdx = (int)(bastionTile - map->tiles);
        onMainland[bIdx] = true;
        mainlandQueue[mqEnd++] = bastionTile;
        while (mqStart < mqEnd) {
            HexTile* curr = mainlandQueue[mqStart++];
            for (int d = 0; d < 6; d++) {
                HexTile* n = curr->neighbors[d];
                if (!n || n->terrain != TERRAIN_LAND || n->biome == BIOME_WATER) continue;
                int nIdx = (int)(n - map->tiles);
                if (!onMainland[nIdx]) {
                    onMainland[nIdx] = true;
                    mainlandQueue[mqEnd++] = n;
                }
            }
        }
    }
    free(mainlandQueue);

    /* Ángulo base aleatorio para los 3 sectores angulares */
    float baseAngle = ((float)(rand() % 360)) * (3.14159265f / 180.0f);

    /* 3 sectores angulares asignados a los 3 clusters */
    int clusterSectors[3] = { 0, 1, 2 };
    for (int i = 2; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int tmp = clusterSectors[i];
        clusterSectors[i] = clusterSectors[j];
        clusterSectors[j] = tmp;
    }

    /* 3 bandas concéntricas:
       Cluster 0 = Ciudad Principal (SETTLEMENT_PRIMARY) -> preferentemente Medio (1) o Interior (0).
       Clusters 1 y 2 = Poblados Secundarios (SETTLEMENT_SECONDARY) -> bandas restantes. */
    int primaryBand = (rand() % 2 == 0) ? 0 : 1;
    int clusterBands[3];
    clusterBands[0] = primaryBand;
    if (primaryBand == 0)
    {
        clusterBands[1] = 1;
        clusterBands[2] = 2;
    }
    else
    {
        clusterBands[1] = 0;
        clusterBands[2] = 2;
    }
    if (rand() % 2 == 0)
    {
        int tmp = clusterBands[1];
        clusterBands[1] = clusterBands[2];
        clusterBands[2] = tmp;
    }

    HexTile* placedCenters[3] = { NULL, NULL, NULL };
    int nextSettlementId = 1;

    for (int k = 0; k < 3; k++)
    {
        SettlementType sType = (k == 0) ? SETTLEMENT_PRIMARY : SETTLEMENT_SECONDARY;
        int targetSector = clusterSectors[k];
        int targetBand = clusterBands[k];
        HexTile* chosenCenter = NULL;

        /* Búsqueda en 4 etapas para garantizar colocación robusta:
           Etapa 1: Sector estricto, banda estricta, distancias completas
           Etapa 2: Sector estricto, banda relajada, distancias completas
           Etapa 3: Sector estricto, distancias reducidas
           Etapa 4: Sector relajado, cualquier tierra válida */
        for (int stage = 1; stage <= 4 && chosenCenter == NULL; stage++)
        {
            int maxAttempts = (stage == 1) ? 600 : ((stage == 2) ? 600 : 400);

            for (int att = 0; att < maxAttempts; att++)
            {
                int idx = rand() % map->tileCount;
                if (onMainland && !onMainland[idx]) continue;
                HexTile* t = &map->tiles[idx];

                if (t->terrain != TERRAIN_LAND || t->special != TILE_SPECIAL_NONE ||
                    t->river == RIVER_PRESENT || t->structure.type != STRUCT_NONE ||
                    t->footprintRole != FOOTPRINT_ROLE_NONE)
                {
                    continue;
                }

                int distB = HexDistance(t->coord, bastionCoord);
                int distS = HexDistance(t->coord, startCoord);
                int sector = Map_GetAngleSector(bastionCoord, t->coord, baseAngle);

                int reqBastionDist = (stage >= 3) ? 20 : minBastionDist;
                int reqStartDist   = (stage >= 3) ? 14 : minStartDist;
                int reqClusterDist = (stage >= 3) ? 16 : minClusterDist;

                if (distB < reqBastionDist || distS < reqStartDist) continue;

                if (stage <= 3 && sector != targetSector) continue;
                if (stage == 1 && !Map_IsInConcentricBand(distB, targetBand)) continue;

                bool tooCloseToOther = false;
                for (int p = 0; p < k; p++)
                {
                    if (placedCenters[p] != NULL && HexDistance(t->coord, placedCenters[p]->coord) < reqClusterDist)
                    {
                        tooCloseToOther = true;
                        break;
                    }
                }
                if (tooCloseToOther) continue;

                /* Para Ciudad Principal (huella 3 de castillo), verificar que tenga vecinos libres */
                if (sType == SETTLEMENT_PRIMARY)
                {
                    int freeN = 0;
                    for (int d = 0; d < 6; d++)
                    {
                        HexTile* n = t->neighbors[d];
                        if (n && n->terrain == TERRAIN_LAND && n->special == TILE_SPECIAL_NONE &&
                            n->river == RIVER_NONE && n->structure.type == STRUCT_NONE &&
                            n->footprintRole == FOOTPRINT_ROLE_NONE)
                        {
                            freeN++;
                        }
                    }
                    if (freeN < 2) continue;
                }

                chosenCenter = t;
                break;
            }
        }

        if (chosenCenter != NULL)
        {
            placedCenters[k] = chosenCenter;
            int fId = k; /* Cada cluster representa una facción (0, 1, 2) */
            Map_GenerateSettlement(map, chosenCenter, nextSettlementId++, sType, provinceLevel, fId);
        }
    }

    /* 3. Altares Divinos (3 altares, huella 1) */
    int altarsToPlace = 3;
    int placedAltars = 0;
    for (int attempt = 0; attempt < 800 && placedAltars < altarsToPlace; attempt++)
    {
        int idx = rand() % map->tileCount;
        if (onMainland && !onMainland[idx]) continue;
        HexTile* tile = &map->tiles[idx];

        if (tile->terrain != TERRAIN_LAND || tile->special != TILE_SPECIAL_NONE ||
            tile->river == RIVER_PRESENT || tile->structure.type != STRUCT_NONE ||
            tile->footprintRole != FOOTPRINT_ROLE_NONE)
        {
            continue;
        }

        bool nearTown = false;
        for (int p = 0; p < 3; p++)
        {
            if (placedCenters[p] != NULL && HexDistance(tile->coord, placedCenters[p]->coord) < 4)
            {
                nearTown = true;
                break;
            }
        }
        if (nearTown) continue;

        tile->structure.type = STRUCT_ALTAR;
        tile->structure.isGuarded = ((rand() % 100) < (45 + provinceLevel * 15));
        tile->structure.divineCardId = (rand() % 6) + 1;
        tile->footprintRole = FOOTPRINT_ROLE_ANCHOR;
        tile->footprintAnchor = tile->coord;
        placedAltars++;
    }

    /* 4. Atalayas (3 atalayas, huella 1) */
    int towersToPlace = 3;
    int placedTowers = 0;
    for (int attempt = 0; attempt < 800 && placedTowers < towersToPlace; attempt++)
    {
        int idx = rand() % map->tileCount;
        if (onMainland && !onMainland[idx]) continue;
        HexTile* tile = &map->tiles[idx];

        if (tile->terrain != TERRAIN_LAND || tile->special != TILE_SPECIAL_NONE ||
            tile->river == RIVER_PRESENT || tile->structure.type != STRUCT_NONE ||
            tile->footprintRole != FOOTPRINT_ROLE_NONE)
        {
            continue;
        }

        bool nearTown = false;
        for (int p = 0; p < 3; p++)
        {
            if (placedCenters[p] != NULL && HexDistance(tile->coord, placedCenters[p]->coord) < 4)
            {
                nearTown = true;
                break;
            }
        }
        if (nearTown) continue;

        tile->structure.type = STRUCT_WATCHTOWER;
        tile->structure.isGuarded = ((rand() % 100) < (25 + provinceLevel * 10));
        tile->footprintRole = FOOTPRINT_ROLE_ANCHOR;
        tile->footprintAnchor = tile->coord;
        placedTowers++;
    }

    free(onMainland);
}


/*
    Asigna entre 1 y 3 puertas (FOOTPRINT_ROLE_GATE) en el perímetro del Bastión,
    ubicadas donde los caminos conectan con el bastión.
*/
static void Map_AssignBastionGates(Map* map, HexTile* bastionTile)
{
    if (map == NULL || bastionTile == NULL) return;

    int gateCount = 0;
    HexTile* walls[6];
    int wallCount = 0;

    for (int d = 0; d < 6; d++)
    {
        HexTile* n = bastionTile->neighbors[d];
        if (n && n->footprintRole == FOOTPRINT_ROLE_WALL)
        {
            walls[wallCount++] = n;
            if (n->isRoad)
            {
                if (gateCount < 3)
                {
                    n->footprintRole = FOOTPRINT_ROLE_GATE;
                    gateCount++;
                }
            }
        }
    }

    /* Asegurar entre 1 y 3 puertas: si ningún camino pisó la muralla directamente,
       convertir el muro más cercano a un camino en puerta */
    if (gateCount == 0 && wallCount > 0)
    {
        int bestIdx = 0;
        int minRoadDist = 999999;
        for (int i = 0; i < wallCount; i++)
        {
            for (int k = 0; k < map->tileCount; k++)
            {
                if (map->tiles[k].isRoad)
                {
                    int dist = HexDistance(walls[i]->coord, map->tiles[k].coord);
                    if (dist < minRoadDist)
                    {
                        minRoadDist = dist;
                        bestIdx = i;
                    }
                }
            }
        }
        walls[bestIdx]->footprintRole = FOOTPRINT_ROLE_GATE;
        walls[bestIdx]->isRoad = true;
        gateCount = 1;
    }
}

/*
    Verifica si existe al menos una ruta 100% segura (sin ríos sin puente y sin murallas)
    desde 'from' hasta 'to'.
*/
bool Map_HasSafePath(const Map* map, const HexTile* from, const HexTile* to)
{
    if (map == NULL || from == NULL || to == NULL) return false;
    if (from == to) return true;

    bool* visited = (bool*)calloc(map->tileCount, sizeof(bool));
    const HexTile** queue = (const HexTile**)malloc(sizeof(HexTile*) * map->tileCount);
    if (!visited || !queue)
    {
        free(visited);
        free((void*)queue);
        return false;
    }

    int qStart = 0, qEnd = 0;
    int sIdx = (int)(from - map->tiles);
    visited[sIdx] = true;
    queue[qEnd++] = from;
    bool found = false;

    while (qStart < qEnd)
    {
        const HexTile* curr = queue[qStart++];
        if (curr == to)
        {
            found = true;
            break;
        }

        for (int d = 0; d < 6; d++)
        {
            const HexTile* n = curr->neighbors[d];
            if (!n) continue;
            if (n->terrain == TERRAIN_WATER && n->bridge == BRIDGE_NONE) continue;
            if (n->footprintRole == FOOTPRINT_ROLE_WALL) continue;
            if (n->river == RIVER_PRESENT && n->bridge == BRIDGE_NONE) continue;

            int nIdx = (int)(n - map->tiles);
            if (!visited[nIdx])
            {
                visited[nIdx] = true;
                queue[qEnd++] = n;
            }
        }
    }

    free(visited);
    free((void*)queue);
    return found;
}

/*
    Si no existe una ruta segura hasta 'to', busca el camino más corto
    y coloca puentes en los ríos que corte para garantizar la conectividad segura.
*/
static void Map_EnsureSafePathTo(Map* map, HexTile* from, HexTile* to, int provinceLevel)
{
    if (map == NULL || from == NULL || to == NULL) return;
    if (Map_HasSafePath(map, from, to)) return;

    bool* visited = (bool*)calloc(map->tileCount, sizeof(bool));
    HexTile** prev = (HexTile**)calloc(map->tileCount, sizeof(HexTile*));
    HexTile** queue = (HexTile**)malloc(sizeof(HexTile*) * map->tileCount);
    if (!visited || !prev || !queue)
    {
        free(visited);
        free(prev);
        free(queue);
        return;
    }

    int qStart = 0, qEnd = 0;
    int sIdx = (int)(from - map->tiles);
    visited[sIdx] = true;
    queue[qEnd++] = from;
    bool found = false;

    while (qStart < qEnd)
    {
        HexTile* curr = queue[qStart++];
        if (curr == to)
        {
            found = true;
            break;
        }

        for (int d = 0; d < 6; d++)
        {
            HexTile* n = curr->neighbors[d];
            if (!n) continue;
            if (n->terrain == TERRAIN_WATER && n->bridge == BRIDGE_NONE) continue;
            if (n->footprintRole == FOOTPRINT_ROLE_WALL) continue;

            int nIdx = (int)(n - map->tiles);
            if (!visited[nIdx])
            {
                visited[nIdx] = true;
                prev[nIdx] = curr;
                queue[qEnd++] = n;
            }
        }
    }

    if (found)
    {
        int bridgesAdded = 0;
        HexTile* curr = to;
        while (curr != NULL && curr != from)
        {
            if (curr->river == RIVER_PRESENT && curr->bridge == BRIDGE_NONE)
            {
                curr->bridge = BRIDGE_ABANDONED;
                curr->structure.type = STRUCT_BRIDGE;
                curr->structure.tollCost = 10 + provinceLevel * 4;
                bridgesAdded++;
            }
            int idx = (int)(curr - map->tiles);
            curr = prev[idx];
        }
        // printf("[DEBUG] Map_EnsureSafePathTo added %d bridges.\n", bridgesAdded);
    }
    else
    {
        printf("[DEBUG] Map_EnsureSafePathTo: NO path found between from (%d, %d) and to (%d, %d)!\n",
               from->coord.q, from->coord.r, to->coord.q, to->coord.r);
    }

    free(visited);
    free(prev);
    free(queue);
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
    map.startCoord = HexCoord_Create(0, 0);
    map.bastionCoord = HexCoord_Create(0, 0);


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
       5. BASTIÓN CENTRAL (HUELLA 7 AMURALLADA)
       ======================================================== */

    const BalanceConfig* bCfg = Balance_GetGlobal();
    int maxBastionOffset = (bCfg && bCfg->bastionCenterMaxOffset > 0) ? bCfg->bastionCenterMaxOffset : 8;
    float minStartDays = (bCfg && bCfg->minStartDistDays > 0.01f) ? bCfg->minStartDistDays : 3.5f;
    int hexPerDay = (bCfg && bCfg->hexPerDay > 0) ? bCfg->hexPerDay : 9;

    int bastionDisplacement = 0;
    HexTile* bastionTile = Map_PlaceBastion(&map, maxBastionOffset, &bastionDisplacement);

    /* ========================================================
       6. CREAR INICIO DE COSTA (DISTANCIA >= 3.5 DÍAS)
       ======================================================== */

    HexTile* start = NULL;
    if (bastionTile != NULL) {
        start = Map_GetStartCoastTile(&map, bastionTile->coord, minStartDays, hexPerDay);
    } else {
        start = Map_GetRandomStartTile(&map);
    }

    if (start != NULL) {
        start->special = TILE_SPECIAL_START;
        map.startCoord = start->coord;
    }

    /* ========================================================
       7. RÍOS (FRENAN A DISTANCIA 3 DEL BASTIÓN)
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

    /* 9a. Camino Principal: De Inicio al Bastión */
    if (start != NULL && bastionTile != NULL)
    {
        Map_CreateRoad(
            &map,
            start,
            bastionTile,
            ROAD_MAIN
        );
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

    /* 9g. Asignación de Puertas del Bastión (Fase 5) */
    Map_AssignBastionGates(&map, bastionTile);


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

    /* 10b. Verificación y garantía de conectividad segura a Bastión y clusters (Fase 4) */
    HexTile* clusterAnchors[3] = { NULL, NULL, NULL };
    int clusterCount = 0;
    for (int i = 0; i < map.tileCount && clusterCount < 3; i++)
    {
        HexTile* t = &map.tiles[i];
        if (t->structure.type == STRUCT_TOWN_CASTLE && !t->structure.isBastion && t->footprintRole == FOOTPRINT_ROLE_ANCHOR)
        {
            clusterAnchors[clusterCount++] = t;
        }
    }
    if (bastionTile != NULL && start != NULL)
    {
        Map_EnsureSafePathTo(&map, start, bastionTile, provinceLevel);
        for (int k = 0; k < clusterCount; k++)
        {
            if (clusterAnchors[k] != NULL)
            {
                Map_EnsureSafePathTo(&map, start, clusterAnchors[k], provinceLevel);
            }
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

void MapConfig_InitDefault(MapConfig* cfg, int provinceLevel) {
    if (!cfg) return;
    cfg->radius = 70;
    cfg->landProbabilityPercent = 55;
    cfg->exitCount = 3;
    cfg->provinceLevel = (provinceLevel >= 1 && provinceLevel <= 4) ? provinceLevel : 1;
}

void Map_Init(Map* map, const MapConfig* cfg) {
    if (!map) return;
    if (map->tiles != NULL) {
        Map_Free(map);
    }

    MapConfig defaultCfg;
    if (!cfg) {
        MapConfig_InitDefault(&defaultCfg, 1);
        cfg = &defaultCfg;
    }

    *map = Map_GenerateProcedural(
        cfg->radius,
        cfg->landProbabilityPercent,
        cfg->exitCount,
        cfg->provinceLevel
    );
}

HexCoord Map_GetStartCoord(const Map* map) {
    if (!map || !map->tiles) return HexCoord_Create(0, 0);
    if (map->startCoord.q != 0 || map->startCoord.r != 0) {
        return map->startCoord;
    }
    for (int i = 0; i < map->tileCount; i++) {
        if (map->tiles[i].special == TILE_SPECIAL_START) {
            return map->tiles[i].coord;
        }
    }
    return HexCoord_Create(0, 0);
}

HexCoord Map_GetBastionCoord(const Map* map) {
    if (!map || !map->tiles) return HexCoord_Create(0, 0);
    if (map->bastionCoord.q != 0 || map->bastionCoord.r != 0) {
        return map->bastionCoord;
    }
    for (int i = 0; i < map->tileCount; i++) {
        if (map->tiles[i].structure.isBastion && map->tiles[i].footprintRole == FOOTPRINT_ROLE_ANCHOR) {
            return map->tiles[i].coord;
        }
    }
    return HexCoord_Create(0, 0);
}

HexTile* Map_GetBastionTile(const Map* map) {
    if (!map || !map->tiles) return NULL;
    HexCoord bCoord = Map_GetBastionCoord(map);
    return Map_FindTile((Map*)map, bCoord);
}

float Map_DifficultyAt(const Map* map, HexCoord coord) {
    if (!map) return 1.0f;
    const BalanceConfig* cfg = Balance_GetGlobal();
    float diffBastion = (cfg && cfg->difficultyBastion > 0.0f) ? cfg->difficultyBastion : 2.0f;
    float diffInterior = (cfg && cfg->difficultyInterior > 0.0f) ? cfg->difficultyInterior : 1.6f;
    float diffBorder = (cfg && cfg->difficultyBorder > 0.0f) ? cfg->difficultyBorder : 1.0f;

    HexCoord bCoord = Map_GetBastionCoord(map);
    int dist = HexDistance(coord, bCoord);
    HexCoord sCoord = Map_GetStartCoord(map);
    int startDist = HexDistance(sCoord, bCoord);
    float maxDist = (float)startDist;
    if (maxDist < 25.0f) maxDist = 35.0f;

    float norm = (float)dist / maxDist;
    if (norm > 1.0f) norm = 1.0f;
    if (norm < 0.0f) norm = 0.0f;

    /* Gradiente: norm = 0 en Bastión (2.0), norm = 0.5 interior (1.6), norm = 1.0 borde/inicio (1.0) */
    if (norm <= 0.5f) {
        float u = norm / 0.5f;
        return diffBastion + u * (diffInterior - diffBastion);
    } else {
        float u = (norm - 0.5f) / 0.5f;
        return diffInterior + u * (diffBorder - diffInterior);
    }
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
            
            // Reconstruir camino al reves y calcular coste real de PA
            int realCost = 0;
            int w = length - 1;
            while (curr->cameFrom != NULL) {
                pathOut[w] = curr->tile;
                int c = Biome_GetProperties(curr->tile->biome).actionPointCost;
                if (curr->tile->isRoad && c > 1) c -= 1;
                realCost += c;
                // Buscar el cameFrom en el array
                for (int i=0; i<map->tileCount; i++) {
                    if (nodes[i].tile == curr->cameFrom->tile) {
                        curr = &nodes[i];
                        break;
                    }
                }
                w--;
            }
            *outTotalCost = realCost;
            
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

            // Intransitable: muralla (salvo puertas)
            if (neighbor->footprintRole == FOOTPRINT_ROLE_WALL) {
                continue;
            }

            // Solo atravesar casillas conocidas para el jugador
            if (!ignoreFog) {
                if (!neighbor->visited && !neighbor->visible && neighbor != end) continue;
            }
            
            // Coste de entrar al vecino
            int cost = Biome_GetProperties(neighbor->biome).actionPointCost;
            if (neighbor->isRoad) cost = (cost > 1) ? cost - 1 : 1; // Basic road discount

            // Preferir desvíos de hasta 6 PA antes de cruzar río sin puente (Fase 6)
            int astarWeight = cost;
            if (neighbor->river == RIVER_PRESENT && neighbor->bridge == BRIDGE_NONE) {
                astarWeight += 6;
            }
            
            // Encontrar vecino en nodes
            int nIdx = -1;
            for (int j = 0; j < map->tileCount; j++) {
                if (nodes[j].tile == neighbor) {
                    nIdx = j;
                    break;
                }
            }
            
            if (nIdx == -1 || nodes[nIdx].closed) continue;
            
            int tentativeG = nodes[currentIdx].g + astarWeight;
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

    /* 1. Identificar casillas de la ruta A (start) -> B (Bastión) */
    bool* onRoute = (bool*)calloc(map->tileCount, sizeof(bool));
    if (!onRoute) return;

    HexTile* end = Map_GetBastionTile(map);
    if (end != NULL) {
        HexTile** queue = (HexTile**)malloc(map->tileCount * sizeof(HexTile*));
        HexTile** prev = (HexTile**)calloc(map->tileCount, sizeof(HexTile*));
        bool* visited = (bool*)calloc(map->tileCount, sizeof(bool));
        if (queue && prev && visited) {
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

    /* 1. Cruces de caminos (>= 3 vecinos con isRoad)
       Garantía anti-duplicados: Si una casilla ya tiene un poste vecino adyacente (< 2 hexes),
       no colocamos otro para evitar racimos de postes en bifurcaciones compuestas. */
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
            bool hasNeighborSignpost = false;
            for (int d = 0; d < 6; d++) {
                if (tile->neighbors[d] && tile->neighbors[d]->structure.type == STRUCT_SIGNPOST) {
                    hasNeighborSignpost = true;
                    break;
                }
            }
            if (!hasNeighborSignpost) {
                tile->structure.type = STRUCT_SIGNPOST;
                tile->structure.settlementId = 0;
                tile->structure.settlementType = SETTLEMENT_NONE;
                tile->structure.isGuarded = false;
            }
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

