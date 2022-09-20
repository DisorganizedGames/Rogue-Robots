#pragma once

//#ifndef NDEBUG
//#define NDEBUG
//#endif

#ifndef UNICODE
#define UNICODE
#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <assert.h>
#include <stdint.h>

#include <xaudio2.h>
#include <x3daudio.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include <d3d12.h>
#pragma warning(push, 0)
#include <DirectXTK/SimpleMath.h>
#pragma warning(pop)
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl.h>

#include <cmath>
#include <iostream>
#include <iosfwd>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <stack>
#include <memory>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <semaphore>
#include <atomic>
#include <future>
#include <functional>
#include <filesystem>
#include <span>
#include <numeric>
#include <utility>
#include <random>
#include <tuple>
#include <variant>
#include <optional>
#include <DirectXMath.h>
#include <bitset>
#include <barrier>
#include <timeapi.h>
#include <cstdio>
#include <cstdarg>
#include <queue>

#include "common/Utils.h"

#include "common/ErrorTypes.h"
