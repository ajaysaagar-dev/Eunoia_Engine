#pragma once
#include <cassert>
#include <cstdio>
#include <cstdlib>

// ============================================================================
// EngineCore::Assert — debug assertions with descriptive messages
// ============================================================================

#ifdef NDEBUG
#define ECORE_ASSERT(expr, msg) ((void)0)
#else
#define ECORE_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            std::fprintf(stderr, "[ASSERT FAILED] %s\n  Condition: %s\n  File: %s:%d\n", \
                (msg), #expr, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (0)
#endif
