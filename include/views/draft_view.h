#ifndef DRAFT_VIEW_H
#define DRAFT_VIEW_H

#include "raylib.h"
#include "modifier.h"
#include "army.h"
#include "presentation/audio_manager.h"

typedef struct {
    ModifierDef* choices[3];
    int selectedChoice; // -1 if none
    bool wantsToReplace;
    int replaceIndex; // -1 if none
    const ModifierRegistry* registry;
} DraftContext;

void DraftView_Init(DraftContext* ctx, const ModifierRegistry* reg);
int DraftView_UpdateDraw(DraftContext* ctx, Army* army, int SW, int SH);

#endif
