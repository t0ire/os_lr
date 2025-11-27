#pragma once

#ifdef _WIN32
    #ifdef MATHLIB_EXPORTS
        #define MATHLIB_API __declspec(dllexport)
    #else
        #define MATHLIB_API __declspec(dllimport)
    #endif
#else
    #define MATHLIB_API
#endif

#ifdef __cplusplus
extern "C" { //чтобы не изменялось название контрактов когда в либ передаем  
#endif

MATHLIB_API float Pi(int K); //контракт 1 - интерфейс для одинак сигнатуры

MATHLIB_API int* Sort(int* array, int size); //2

#ifdef __cplusplus
}
#endif