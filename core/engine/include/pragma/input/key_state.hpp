#ifndef __KEY_STATE_HPP__
#define __KEY_STATE_HPP__

#include "pragma/definitions.h"
#include <cinttypes>

// These have to correspond to GLFW::KeyState!
enum class DLLENGINE KeyState : uint32_t
{
	Invalid = std::numeric_limits<std::underlying_type_t<KeyState>>::max(),
	Press = 1,
	Release = 0,
	Repeat = 2,
	Held = Repeat +1
};

#endif