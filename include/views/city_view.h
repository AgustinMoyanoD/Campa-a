#ifndef CITY_VIEW_H
#define CITY_VIEW_H

#include <raylib.h>
#include "models/city.h"
#include "models/army.h"
#include "controllers/game_master.h"

// Dibuja y maneja la interacción con el menú de la ciudad. Retorna true si se cerró el menú.
bool CityView_UpdateDraw(CityContext* ctx, Army* army, GameMaster* gm, int screenWidth, int screenHeight);

#endif /* CITY_VIEW_H */
