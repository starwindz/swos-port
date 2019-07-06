#pragma once

#include <cstdint>

#ifdef _WINDLL
# define SDL_DYNAPI_FETCHER_API __declspec(dllexport)
#else
# define SDL_DYNAPI_FETCHER_API __declspec(dllimport)
#endif

extern "C" SDL_DYNAPI_FETCHER_API int32_t SDL_DYNAPI_entry(uint32_t apiVer, void *table, uint32_t tableSize);
extern "C" SDL_DYNAPI_FETCHER_API void **GetFunctionTable(uint32_t *tableSize);
