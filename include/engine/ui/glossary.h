#ifndef GLOSSARY_H
#define GLOSSARY_H

#include <stdbool.h>

#define GLOSSARY_MAX_ENTRIES 64
#define GLOSSARY_ID_LEN 32
#define GLOSSARY_TITLE_LEN 64
#define GLOSSARY_TEXT_LEN 256

typedef struct {
    char id[GLOSSARY_ID_LEN];
    char title[GLOSSARY_TITLE_LEN];
    char text[GLOSSARY_TEXT_LEN];
} GlossaryEntry;

/* Inicializa y parsea el archivo de glosario */
bool Glossary_Init(const char* filepath);

/* Libera recursos del glosario */
void Glossary_Shutdown(void);

/* Retorna la entrada para un ID dado, o NULL si no existe */
const GlossaryEntry* Glossary_Get(const char* id);

/* Retorna true si el ID existe en el glosario */
bool Glossary_Has(const char* id);

/* Valida que todos los IDs obligatorios existan. Retorna false si falta alguno (Sec B1). */
bool Glossary_ValidateAll(void);

/* Retorna la cantidad de entradas cargadas */
int Glossary_GetCount(void);

#endif
