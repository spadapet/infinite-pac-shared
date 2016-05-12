#pragma once

// C-Runtime includes
// Precompiled header only

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <queue>
#include <stack>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

#define MAKE_POD(name) \
	template<> struct std::is_pod<name> \
	{ static const bool value = true; }
