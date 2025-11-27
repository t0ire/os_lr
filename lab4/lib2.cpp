#include <algorithm>

#include "mathlib.h"

MATHLIB_API float Pi(int K) { //валиис
    float pi = 1.0f;
    
    for (int i = 1; i <= K; ++i) {
        float numerator = 4.0f * i * i;
        float denominator = numerator - 1;
        pi *= numerator / denominator;
    }
    
    return 2 * pi;
}

void quicksort(int* array, int low, int high) {//для хоара
    if (low < high) {
        int pivot = array[high];
        int i = low - 1;
        
        for (int j = low; j < high; ++j) {
            if (array[j] <= pivot) {
                i++;
                int temp = array[i];
                array[i] = array[j];
                array[j] = temp;
            }
        }
        
        int temp = array[i + 1];
        array[i + 1] = array[high];
        array[high] = temp;
        
        int pi = i + 1;
        quicksort(array, low, pi - 1);
        quicksort(array, pi + 1, high);
    }
}

MATHLIB_API int* Sort(int* array, int size) {//хоар
    if (array == nullptr || size <= 0) return array;
    
    quicksort(array, 0, size - 1);
    return array;
}