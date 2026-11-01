#ifndef DAY_CLOCK_H
#define DAY_CLOCK_H

#include <stdbool.h>

/* ============================================================
   MÓDULO DE RELOJ CENTRALIZADO (I4)
   Controla el avance temporal en franjas y emite las señales:
   - SlotChanged: cambió la franja horaria.
   - DayEnded: se completaron las 4 franjas y se cierra el día.
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

#endif /* DAY_CLOCK_H */
