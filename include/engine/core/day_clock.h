#ifndef ENGINE_DAY_CLOCK_H
#define ENGINE_DAY_CLOCK_H

#include <stdbool.h>

/* ============================================================
   ENGINE DAY CLOCK: CONTROLADOR DE CICLO TEMPORAL Y TURNOS
   ============================================================ */

typedef enum {
    TIME_MORNING = 0,
    TIME_MIDDAY,
    TIME_AFTERNOON,
    TIME_NIGHT
} TimeOfDay;

typedef struct {
    int currentDay;
    TimeOfDay timeOfDay;
    int totalTurnsElapsed;
    
    bool slotChanged;
    bool dayEnded;
    TimeOfDay previousTimeOfDay;
} DayClock;

void        DayClock_Init(DayClock* clock, int startDay, TimeOfDay startTime);
void        DayClock_AdvanceSlot(DayClock* clock);
bool        DayClock_HasSlotChanged(const DayClock* clock);
bool        DayClock_HasDayEnded(const DayClock* clock);
void        DayClock_ClearSignals(DayClock* clock);
const char* DayClock_GetTimeName(TimeOfDay time);

#endif /* ENGINE_DAY_CLOCK_H */
