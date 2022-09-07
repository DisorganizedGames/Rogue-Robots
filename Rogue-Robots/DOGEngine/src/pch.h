#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <xaudio2.h>
#include <x3daudio.h>

#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <memory>
#include <thread>

#include "common/Utils.h"