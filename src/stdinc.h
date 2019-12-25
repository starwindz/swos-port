#include <cstdint>
#include <iostream>
#include <cstdarg>
#include <algorithm>
#include <string>
#include <numeric>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <array>
#include <vector>
#include <functional>
#include <condition_variable>
#include <sys/stat.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SimpleIni.h>
#include <SDL2/SDL.h>
#include <SDL_mixer.h>

#include "assert.h"
#include "swos.h"
#include "swossym.h"
#include "log.h"

#ifdef SWOS_VM
# include "vm.h"
# define __declspec(naked)
#endif
