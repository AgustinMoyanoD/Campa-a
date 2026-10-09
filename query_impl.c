int Modifier_Query(ModifierHook hook, EffectType stat, ModSide side, const ModContext* ctx, int base, ModBreakdown* outOpt) {
    if (!ctx || !ctx->reg || !ctx->army) return base;
    
    if (outOpt) {
        outOpt->base = base;
        outOpt->count = 0;
        outOpt->final = base;
    }
    
    int absoluteBonus = 0;
    int percentageBonus = 0;
    
    ActiveMod activeMods[32];
    int count = Modifier_CollectActive(ctx->reg, ctx->army, ctx->enemy, activeMods, 32);
    
    for (int i = 0; i < count; i++) {
        const ModifierDef* mod = Modifier_GetById(ctx->reg->defs, ctx->reg->count, activeMods[i].id);
        if (!mod) continue;
        
        for (int r = 0; r < mod->ruleCount; r++) {
            const ModifierRule* rule = &mod->rules[r];
            if (rule->hook != hook) continue;
            
            if (!Condition_Eval(&rule->condition, ctx)) continue;
            
            for (int e = 0; e < rule->effectCount; e++) {
                const EffectDef* effect = &rule->effects[e];
                if (effect->type != stat) continue;
                if (effect->side != side) continue;
                
                if (effect->mode == EFFECT_MODE_ABSOLUTE) {
                    absoluteBonus += effect->value;
                    if (outOpt && outOpt->count < 12) {
                        strncpy(outOpt->entries[outOpt->count].source, mod->name, 47);
                        outOpt->entries[outOpt->count].flat = effect->value;
                        outOpt->entries[outOpt->count].percent = 0;
                        outOpt->count++;
                    }
                } else if (effect->mode == EFFECT_MODE_PERCENTAGE) {
                    percentageBonus += effect->value;
                    if (outOpt && outOpt->count < 12) {
                        strncpy(outOpt->entries[outOpt->count].source, mod->name, 47);
                        outOpt->entries[outOpt->count].flat = 0;
                        outOpt->entries[outOpt->count].percent = effect->value;
                        outOpt->count++;
                    }
                }
            }
        }
    }
    
    int finalValue = base + absoluteBonus;
    if (percentageBonus != 0) {
        finalValue += (finalValue * percentageBonus) / 100;
    }
    
    if (outOpt) outOpt->final = finalValue;
    return finalValue;
}
