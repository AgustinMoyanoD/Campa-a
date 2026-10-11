#include "models/day_clock.h"
#include <string.h>

void DayClock_Init(DayClock* clock, int startDay, TimeOfDay startTime) {
    if (!clock) return;
    memset(clock, 0, sizeof(DayClock));
    clock->currentDay = (startDay > 0) ? startDay : 1;
    clock->timeOfDay = startTime;
    clock->previousTimeOfDay = startTime;
    clock->totalTurnsElapsed = 0;
    clock->slotChanged = false;
    clock->dayEnded = false;
}

void DayClock_AdvanceSlot(DayClock* clock) {
    if (!clock) return;
    clock->previousTimeOfDay = clock->timeOfDay;
    clock->slotChanged = true;
    clock->dayEnded = false;
    clock->totalTurnsElapsed++;

    if (clock->timeOfDay == TIME_NIGHT) {
        clock->timeOfDay = TIME_MORNING;
        clock->currentDay++;
        clock->dayEnded = true;
    } else {
        clock->timeOfDay = (TimeOfDay)((int)clock->timeOfDay + 1);
    }
}

bool DayClock_HasSlotChanged(const DayClock* clock) {
    return clock ? clock->slotChanged : false;
}

bool DayClock_HasDayEnded(const DayClock* clock) {
    return clock ? clock->dayEnded : false;
}

void DayClock_ClearSignals(DayClock* clock) {
    if (!clock) return;
    clock->slotChanged = false;
    clock->dayEnded = false;
}

const char* DayClock_GetTimeName(TimeOfDay time) {
    switch (time) {
        case TIME_MORNING:   return "Manana";
        case TIME_MIDDAY:    return "Mediodia";
        case TIME_AFTERNOON: return "Tarde";
        case TIME_NIGHT:     return "Noche";
        default:             return "Desconocido";
    }
}
