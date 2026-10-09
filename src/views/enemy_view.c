#include "views/enemy_view.h"
#include "views/map_view.h"

void EnemyView_DrawOnMap(const EnemySquad* enemy, Vector2 origin, int hexRadius, const GameAssets* assets) {
    if (!enemy || !enemy->isActive) return;
    
    // Calcular posicion en pantalla
    Vector2 pos = HexToPixel(enemy->position, hexRadius);
    pos.x += origin.x;
    pos.y += origin.y - 10.0f; // Ajuste vertical basico para perspectiva
    
    // Dibujar un marcador simple (un estandarte rojo)
    if (assets && assets->enemyToken.id != 0) {
        float scale = 30.0f / assets->enemyToken.width;
        DrawTextureEx(assets->enemyToken, (Vector2){pos.x - 15.0f, pos.y - 15.0f}, 0.0f, scale, WHITE);
    } else {
        DrawCircleV(pos, 15.0f, MAROON);
        DrawCircleLines(pos.x, pos.y, 15.0f, BLACK);
    }
    
    // Un icono simple o inicial
    const char* init = "?";
    if (enemy->name && enemy->name[0] != '\0') {
        // Initials (just the first letter)
        // Wait, Raylib DrawText is simple. 
        char buf[2] = { enemy->name[0], '\0' };
        int w = MeasureText(buf, 14);
        DrawText(buf, pos.x - w/2, pos.y - 7, 14, WHITE);
    }
    
    // Si el mouse esta cerca, mostrar tooltip basico
    Vector2 mouse = GetMousePosition();
    // Ajustar si la camara esta activa? En general esta funcion se llama dentro de BeginMode2D, 
    // asi que GetScreenToWorld2D podria ser necesario, pero origin ya esta aplicado.
    // Asumiremos que el caller controla el hover o lo dibujamos bruto por ahora.
}


