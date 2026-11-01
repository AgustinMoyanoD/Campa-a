#ifndef EVENT_VIEW_H
#define EVENT_VIEW_H

#include "models/event.h"
#include "controllers/game_master.h"

// Dibuja el modal del evento en pantalla
void EventView_Draw(const EventDef* event, int screenWidth, int screenHeight);

// Maneja los clics en las opciones. Devuelve la opción elegida (0 a 4) o -1 si no se eligió nada.
int EventView_HandleInput(const EventDef* event, int screenWidth, int screenHeight);

// Aplica el resultado del evento seleccionado al GameMaster y al Army


void ResultView_Draw(const EventResult* res, int screenWidth, int screenHeight);
int ResultView_HandleInput(const EventResult* res, int screenWidth, int screenHeight);


#endif /* EVENT_VIEW_H */


