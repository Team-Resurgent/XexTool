#pragma once

#ifdef _WIN32
#define TR_EXPORT extern "C" __declspec(dllexport)
#else
#define TR_EXPORT extern "C" __attribute__((visibility("default")))
#endif

