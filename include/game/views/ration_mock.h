#ifndef RATION_MOCK_H
#define RATION_MOCK_H

#include "models/ration.h"

/* ============================================================
   IMPLEMENTACIÓN FALSA (MOCK) DEL CONTRATO DE RACIONES
   Permite testear todos los estados de la UI:
   - Abundancia (confirmación compacta)
   - Faltante (modal completo con toma de decisiones)
   - Hambre en un grupo (riesgo de deserción)
   - Satisfacción en cero (descontento de sargentos)
   ============================================================ */

typedef enum {
    MOCK_SCENARIO_ABUNDANCE = 0,
    MOCK_SCENARIO_SHORTAGE,
    MOCK_SCENARIO_STARVING_GROUP,
    MOCK_SCENARIO_ZERO_SATISFACTION
} RationMockScenario;

void RationMock_SetScenario(RationMockScenario sc);
RationMockScenario RationMock_GetScenario(void);

/* Consultas del mock */
RationNeeds   RationMock_GetNeeds(const Army* army, const Map* map, HexCoord pos);
float         RationMock_GetStock(const Army* army, RationResource r);
GroupState    RationMock_GetGroupState(const Army* army, RationGroup g);
RationPlan    RationMock_Auto(const Army* army, const RationNeeds* needs);
RationPlan    RationMock_Repeat(const Army* army);
bool          RationMock_Validate(const Army* army, const RationPlan* plan, char* err, int cap);
RationPreview RationMock_Preview(const Army* army, const RationPlan* plan);
bool          RationMock_NeedsDecision(const Army* army, const Map* map, HexCoord pos);

#endif /* RATION_MOCK_H */
