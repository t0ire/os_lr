#include <algorithm>

#include "mathlib.h"

MATHLIB_API float Pi(int K) { //лейбниц
    float pi = 0.0f;
    int sign = 1;
    
    for (int i = 0; i < K; ++i) {
        float term = 1.0f / (2 * i + 1);
        pi += sign * term;
        sign *= -1;
    }
    
    return 4 * pi;
}

MATHLIB_API int* Sort(int* array, int size) {//пузырек
    if (array == nullptr || size <= 0) return array;
    
    for (int i = 0; i < size - 1; ++i) {
        for (int j = 0; j < size - i - 1; ++j) {
            if (array[j] > array[j + 1]) {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
    return array;
}