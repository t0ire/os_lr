#include <iostream>
#include <stdexcept>

#include "maxmin.h"

void maxmin(int* arr, int start, int end, int* local_max_ptr, int* local_min_ptr) {
    if (start >= end) {
        *local_max_ptr = arr[start];
        *local_min_ptr = arr[start];
        return;
    }
    
    int local_max = arr[start];
    int local_min = arr[start];

    for (int i = start + 1; i < end; ++i) {
        if (arr[i] > local_max) {
            local_max = arr[i];
        }
        if (arr[i] < local_min) {
            local_min = arr[i];
        }
    }

    *local_max_ptr = local_max;
    *local_min_ptr = local_min;
}