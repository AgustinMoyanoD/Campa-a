#ifndef DRAFT_VIEW_H
#define DRAFT_VIEW_H

#include "raylib.h"
#include "modifier.h"
#include "army.h"
#include "presentation/audio_manager.h"

typedef enum {
    ALTAR_STATE_OFFER,
    ALTAR_STATE_REVEAL,
    ALTAR_STATE_REPLACE
} AltarState;

typedef struct {
    AltarState state;
    ModifierDef* pulledCard;
    ModifierDef* hiddenCurse;
    const ModifierRegistry* registry;
} DraftContext;

void DraftView_Init(DraftContext* ctx, const ModifierRegistry* reg);
int DraftView_UpdateDraw(DraftContext* ctx, Army* army, int SW, int SH);

#endif
