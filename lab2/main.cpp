#include <iostream>
#include <cstdlib>
#include <ctime>  
#include <climits> 
#include <iomanip>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm> 

#include "maxmin.h"
#include "thread.h"

struct MaxMinThreadData {
    int* arr;
    int start;
    int end;
    int local_max;
    int local_min;
};

int* create_arr(int size) {
    if (size <= 0) {
        std::cerr << "size must be positive" << std::endl;
        return nullptr;
    }
    
    int* arr = new int[size];

    if (arr == nullptr) {
        std::cerr << "memory allocation error" << std::endl;
        exit(1);
    }

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (int i = 0; i < size; ++i) {
        arr[i] = (std::rand() % 1000) + 1; 
    }

    return arr;
}

void* maxmin_thread_wrapper(void* arg) {
    MaxMinThreadData* data = static_cast<MaxMinThreadData*>(arg);
    maxmin(data->arr, data->start, data->end, &data->local_max, &data->local_min);
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <array_size> <number_of_threads>" << std::endl;
        return 1;
    }

    int arr_size = std::atoi(argv[1]);
    int k_threads = std::atoi(argv[2]);

    if (arr_size <= 0 || k_threads <= 0) {
        std::cout << "Arguments must be positive integers" << std::endl;
        return 1;       
    }

    if (arr_size < k_threads) {
        k_threads = arr_size;
        std::cout << "Warning: Reduced number of threads to " << k_threads 
                  << " (array size)" << std::endl;
    }

    int* arr = create_arr(arr_size);
    if (arr == nullptr) {
        return 1;
    }

    std::vector<thread::Thread> threads;
    std::vector<MaxMinThreadData> thread_data(k_threads);
    
    int gl_max = INT_MIN;
    int gl_min = INT_MAX;

    auto start_time = std::chrono::high_resolution_clock::now();

    int elem_thread = arr_size / k_threads;
    int remainder_elem = arr_size % k_threads;
    int start_i = 0;

    try {
        for (int i = 0; i < k_threads; ++i) {
            int end_i = start_i + elem_thread;
            
            if (i < remainder_elem) {
                end_i++;
            }

            if (end_i > arr_size) {
                end_i = arr_size;
            }

            thread_data[i].arr = arr;
            thread_data[i].start = start_i;
            thread_data[i].end = end_i;
            thread_data[i].local_max = INT_MIN;
            thread_data[i].local_min = INT_MAX;

            threads.emplace_back(maxmin_thread_wrapper);
            threads.back().Run(&thread_data[i]);
            start_i = end_i;
        }

        for (auto& thread : threads) {
            thread.Join();
        }

    } catch (const std::exception& e) {
        std::cerr << "Thread error: " << e.what() << std::endl;
        delete[] arr;
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < k_threads; ++i) {
        if (gl_max < thread_data[i].local_max) {
            gl_max = thread_data[i].local_max;
        }
        if (gl_min > thread_data[i].local_min) {
            gl_min = thread_data[i].local_min;
        }
    }

    auto multi_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    int one_max, one_min;
    auto start_single = std::chrono::high_resolution_clock::now();
    maxmin(arr, 0, arr_size, &one_max, &one_min);
    auto end_single = std::chrono::high_resolution_clock::now();
    auto single_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_single - start_single);

    bool results_match = (gl_max == one_max) && (gl_min == one_min);
    
    double speedup = (single_duration.count() > 0) ? 
        (double)single_duration.count() / multi_duration.count() : 0.0;
    double efficiency = (speedup > 0 && k_threads > 0) ? (speedup / k_threads) * 100 : 0.0;

    std::cout << "\n" << k_threads << " потоков:" << std::endl;
    std::cout << "  минимум: " << gl_min << std::endl;
    std::cout << "  максимум: " << gl_max << std::endl;
    std::cout << "  время выполнения: " << multi_duration.count() << " мксек" << std::endl;
    
    std::cout << "\nОднопоточный поиск:" << std::endl;
    std::cout << "  минимум: " << one_min << std::endl;
    std::cout << "  максимум: " << one_max << std::endl;
    std::cout << "  время выполнения: " << single_duration.count() << " мксек" << std::endl;

    std::cout << "\nРезультаты " << (results_match ? "совпадают" : "НЕ СОВПАДАЮТ!") << std::endl;
    std::cout << "Ускорение: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    std::cout << "Эффективность: " << std::setprecision(2) << efficiency << "%" << std::endl;
    
    if (speedup > 1.0) {
        std::cout << "Многопоточная версия быстрее в " << speedup << " раз" << std::endl;
    } else if (speedup < 1.0) {
        std::cout << "Многопоточная версия медленнее" << std::endl;
    } else {
        std::cout << "Производительность одинакова" << std::endl;
    }

    delete[] arr;
    return 0;
}