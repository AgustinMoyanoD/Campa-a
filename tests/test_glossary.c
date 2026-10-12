#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "presentation/glossary.h"

int main(void) {
    printf("====================================================\n");
    printf("PRUEBA B1: VALIDACION DEL GLOSARIO (assets/glossary.cfg)\n");
    printf("====================================================\n");

    bool loaded = Glossary_Init("assets/glossary.cfg");
    assert(loaded && "Error al cargar assets/glossary.cfg");
    printf("[1] Archivo cargado exitosamente. Entradas cargadas: %d\n", Glossary_GetCount());
    assert(Glossary_GetCount() >= 19);

    printf("[2] Ejecutando Glossary_ValidateAll()...\n");
    bool valid = Glossary_ValidateAll();
    assert(valid && "Glossary_ValidateAll fallo: faltan terminos obligatorios");
    printf("-> [OK] Todos los terminos obligatorios estan presentes.\n");

    printf("[3] Verificando contenido de terminos clave...\n");
    const GlossaryEntry* fat = Glossary_Get("FATIGADO");
    assert(fat != NULL);
    assert(strlen(fat->title) > 0);
    assert(strlen(fat->text) > 0);
    printf("  [FATIGADO] -> Titulo: '%s', Texto: '%s'\n", fat->title, fat->text);

    const GlossaryEntry* chq = Glossary_Get("choque");
    assert(chq != NULL);
    assert(strcmp(chq->id, "CHOQUE") == 0);
    printf("  [CHOQUE] (case-insensitive) -> Titulo: '%s'\n", chq->title);

    const GlossaryEntry* noc = Glossary_Get("NO_EXISTE_12345");
    assert(noc == NULL);
    printf("  [NO_EXISTE] -> Retorno NULL esperado [OK]\n");

    Glossary_Shutdown();
    assert(Glossary_GetCount() == 0);
    printf("-> [OK] Glossary_Shutdown limpio recursos correctamente.\n");

    printf(">>> PRUEBA B1 PASO AL 100%% [OK] <<<\n");
    return 0;
}
