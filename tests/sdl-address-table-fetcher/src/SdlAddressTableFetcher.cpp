#include "SdlAddressTableFetcher.h"

static void **m_table;
static uint32_t m_tableSize;

int32_t SDL_DYNAPI_entry(uint32_t apiVer, void *table, uint32_t tableSize)
{
    m_table = reinterpret_cast<void **>(table);
    m_tableSize = tableSize / sizeof(void *);
    return -1;  // return failure so that SDL fills the table internally
}

void **GetFunctionTable(uint32_t *tableSize)
{
    if (tableSize)
        *tableSize = m_tableSize;

    return m_table;
}
