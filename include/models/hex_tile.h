#ifndef HEX_TILE_H
#define HEX_TILE_H

#include <stdbool.h>

typedef struct {
    int q;
    int r;
} HexCoord;


typedef enum {
    HEX_RIGHT = 0,
    HEX_DOWN_RIGHT,
    HEX_DOWN_LEFT,
    HEX_LEFT,
    HEX_UP_LEFT,
    HEX_UP_RIGHT
} HexDirection;


typedef enum {
    TERRAIN_LAND,
    TERRAIN_WATER
} TerrainType;


typedef enum {
    TILE_SPECIAL_NONE,
    TILE_SPECIAL_START,
    TILE_SPECIAL_END
} TileSpecial;


typedef enum {
    RIVER_NONE,
    RIVER_PRESENT
} RiverType;


typedef enum {
    BRIDGE_NONE,
    BRIDGE_ABANDONED,
    BRIDGE_OCCUPIED
} BridgeType;


typedef enum {
    ROAD_NONE = 0,
    ROAD_MAIN,              /* Camino principal: ancho, transitado, conecta centros urbanos principales */
    ROAD_SECONDARY          /* Camino secundario / sendero: angosto, conecta altares, atalayas y aldeas */
} RoadType;


typedef enum {
    SETTLEMENT_NONE = 0,
    SETTLEMENT_PRIMARY,     /* Ciudad principal / centro provincial */
    SETTLEMENT_SECONDARY    /* Poblado / aldea secundaria */
} SettlementType;


typedef enum {
    BIOME_PLAINS = 0,
    BIOME_FOREST,
    BIOME_DESERT,
    BIOME_JUNGLE,
    BIOME_MOUNTAIN,
    BIOME_WATER
} BiomeType;


typedef enum {
    STRUCT_NONE = 0,
    STRUCT_ALTAR,           /* Altar con carta divina */
    STRUCT_BRIDGE,          /* Puente sobre río */
    STRUCT_WATCHTOWER,      /* Atalaya (revela mapa) */
    STRUCT_TOWN_CASTLE,     /* Centro de gobierno / cuartel / centro urbano */
    STRUCT_TOWN_MARKET,     /* Mercado de provisiones y ganado */
    STRUCT_TOWN_ARMORY,     /* Armería y equipo */
    STRUCT_TOWN_HOUSES,     /* Casas / suburbio */
    STRUCT_SIGNPOST         /* Poste indicador en cruce de caminos (D2) */
} StructureType;


typedef struct {
    StructureType type;
    bool isGuarded;         /* true = custodiado por enemigos o milicia */
    int settlementId;       /* 0 = independiente; >0 = id del asentamiento (cluster) */
    SettlementType settlementType; /* Primario o Secundario */
    int divineCardId;       /* Para STRUCT_ALTAR: carta que contiene */
    int tollCost;           /* Para STRUCT_BRIDGE: coste del peaje en monedas */
    int factionId;          /* ID de la faccion propietaria (0..2) */
    bool isBesieged;        /* true = asediada / ocupada */
    float enemySpawnRate;   /* Probabilidad por turno de spawnear un escuadrón enemigo (ej: 0.04f) */
} TileStructure;


typedef struct {
    const char* name;
    int actionPointCost;      /* Coste base de Puntos de Acción para entrar */
    float waterConsumption;   /* Multiplicador de consumo de agua (1.0 = normal) */
    float foodConsumption;    /* Multiplicador de consumo de comida (1.0 = normal) */
    float cattlePenalty;      /* Penalización al paso/salud de ganado vivo */
    int stealthBonus;         /* +1 sigilo favorable, 0 neutral, -1 terreno expuesto */
    int visionModifier;       /* +1 visión alta, 0 normal, -1 visión obstruida */
} BiomeProperties;


typedef struct HexTile {

    HexCoord coord;

    int nivel;

    TerrainType terrain;

    BiomeType biome;

    bool isRoad;

    RoadType road;

    TileSpecial special;

    RiverType river;

    BridgeType bridge;

    TileStructure structure;

    struct HexTile* neighbors[6];

    bool visited;

    bool visible;

    bool isRumor;

} HexTile;


BiomeProperties Biome_GetProperties(
    BiomeType biome
);

const char* Structure_GetName(
    StructureType type
);

const char* Settlement_GetTypeName(
    SettlementType type
);

const char* Road_GetTypeName(
    RoadType road
);


HexCoord HexCoord_Create(
    int q,
    int r
);


HexCoord HexCoord_GetNeighbor(
    HexCoord coord,
    HexDirection direction
);


int HexCoord_Distance(HexCoord a, HexCoord b);

HexTile HexTile_Create(
    HexCoord coord,
    int nivel,
    TerrainType terrain
);


void HexTile_Connect(
    HexTile* tileA,
    HexDirection direction,
    HexTile* tileB
);

#endif
