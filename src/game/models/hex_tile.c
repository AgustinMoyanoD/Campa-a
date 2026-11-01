#include "hex_tile.h"
#include "balance_config.h"
#include <stdlib.h>
#include <stddef.h>

BiomeProperties Biome_GetProperties(BiomeType biome)
{
    const BalanceConfig* cfg = Balance_GetGlobal();
    if (cfg && (int)biome >= 0 && (int)biome < 6) {
        return cfg->biomes[(int)biome];
    }

    BiomeProperties props;
    switch (biome)
    {
        case BIOME_FOREST:
            props = (BiomeProperties){ "Bosque", 2, 1.0f, 1.0f, 0.2f, 1, -1 };
            break;
        case BIOME_DESERT:
            props = (BiomeProperties){ "Desierto", 2, 2.5f, 1.0f, 0.5f, -1, 1 };
            break;
        case BIOME_JUNGLE:
            props = (BiomeProperties){ "Jungla", 3, 1.5f, 1.2f, 0.8f, 2, -2 };
            break;
        case BIOME_MOUNTAIN:
            props = (BiomeProperties){ "Montana", 3, 1.2f, 1.2f, 0.9f, 0, 2 };
            break;
        case BIOME_WATER:
            props = (BiomeProperties){ "Agua", 99, 0.0f, 0.0f, 1.0f, 0, 0 };
            break;
        case BIOME_PLAINS:
        default:
            props = (BiomeProperties){ "Llanura", 1, 1.0f, 1.0f, 0.0f, 0, 0 };
            break;
    }
    return props;
}


const char* Structure_GetName(StructureType type)
{
    switch (type)
    {
        case STRUCT_ALTAR:       return "Altar Divino";
        case STRUCT_BRIDGE:      return "Puente";
        case STRUCT_WATCHTOWER:  return "Atalaya";
        case STRUCT_TOWN_CASTLE: return "Poblado - Cuartel";
        case STRUCT_TOWN_MARKET: return "Poblado - Mercado";
        case STRUCT_TOWN_ARMORY: return "Poblado - Armeria";
        case STRUCT_TOWN_HOUSES: return "Poblado - Viviendas";
        case STRUCT_SIGNPOST:    return "Poste Indicador";
        case STRUCT_NONE:
        default:                 return "Ninguna";
    }
}


const char* Settlement_GetTypeName(SettlementType type)
{
    switch (type)
    {
        case SETTLEMENT_PRIMARY:   return "Ciudad Principal";
        case SETTLEMENT_SECONDARY: return "Poblado Secundario";
        case SETTLEMENT_BASTION:   return "Bastion Central";
        case SETTLEMENT_NONE:
        default:                   return "Ninguno";
    }
}


const char* Road_GetTypeName(RoadType road)
{
    switch (road)
    {
        case ROAD_MAIN:      return "Camino Principal";
        case ROAD_SECONDARY: return "Camino Secundario";
        case ROAD_NONE:
        default:             return "Sin camino";
    }
}


HexTile HexTile_Create(
    HexCoord coord,
    int nivel,
    TerrainType terrain
)
{
    HexTile tile;

    tile.coord = coord;

    tile.nivel = nivel;

    tile.terrain = terrain;

    tile.biome = (terrain == TERRAIN_WATER) ? BIOME_WATER : BIOME_PLAINS;

    tile.isRoad = false;

    tile.road = ROAD_NONE;

    tile.special = TILE_SPECIAL_NONE;

    tile.river = RIVER_NONE;

    tile.bridge = BRIDGE_NONE;

    tile.structure.type = STRUCT_NONE;
    tile.structure.isGuarded = false;
    tile.structure.settlementId = 0;
    tile.structure.settlementType = SETTLEMENT_NONE;
    tile.structure.divineCardId = 0;
    tile.structure.tollCost = 0;
    tile.structure.factionId = 0;
    tile.structure.isBesieged = false;
    tile.structure.isBastion = false;
    tile.structure.enemySpawnRate = 0.0f;

    tile.footprintRole = FOOTPRINT_ROLE_NONE;
    tile.footprintAnchor = coord;
    tile.isFord = false;

    tile.visited = false;
    tile.visible = false;
    tile.isRumor = false;

    for (int i = 0; i < 6; i++)
    {
        tile.neighbors[i] = NULL;
    }

    return tile;
}


void HexTile_Connect(
    HexTile* tileA,
    HexDirection direction,
    HexTile* tileB
)
{
    if (tileA == NULL || tileB == NULL)
        return;


    tileA->neighbors[direction] = tileB;


    /*
        Las direcciones opuestas están
        separadas por 3 posiciones.
    */

    HexDirection opposite =
        (direction + 3) % 6;


    tileB->neighbors[opposite] = tileA;
}



