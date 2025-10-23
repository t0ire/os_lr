#include <iostream>
#include <cstdlib>
#include <ctime>  
#include <thread>   
#include <climits> 
#include <iomanip>
#include <chrono>

#include "maxmin.h"

int* create_arr (int size) {
    if (size <= 0) {
        std::cerr << "size must be positive" << std::endl;
        return nullptr;
    }
    
    int* arr = new int[size];

    if (arr == nullptr) {
        std::cerr << "memory allocation error" << std::endl;
        exit(1);
    }

    std::srand(std::time(nullptr));

    for (int i = 0; i< size; ++i) {
        arr[i] = (std::rand() % 1000) + 1; //от 1 до 1000
    }

    // для проверки
    //arr[0] = 0;
    //arr[size - 1] = 2000;

    return arr;
}

int main(int argc, char* argv[]) { // 0 - имя проги, 1 - рфзмер arr, 2 - колво потоков
    if (argc < 3) {
        std::cout << "argument error" << std::endl;
        return 1;
    }

    int arr_size = std::atoi(argv[1]);
    int k_threads = std::atoi(argv[2]);

    if (arr_size <= 0 || k_threads <= 0) {
        std::cout << "argument error" << std::endl;
        return 1;       
    }

    if (arr_size < k_threads) {
        k_threads = arr_size;
    }

    int* arr = create_arr(arr_size);
    if (arr == nullptr) {
    return 1;
    }

    std::thread* arr_thread = new std::thread[k_threads]; //массив потоков

    int* local_max = new int[k_threads];
    int* local_min = new int[k_threads];
    int gl_max = INT_MIN;
    int gl_min = INT_MAX;

    //время
    auto start_time = std::chrono::high_resolution_clock::now();

    int elem_thread = arr_size / k_threads;
    int remainder_elem = arr_size % k_threads; //ост
    int start_i = 0;

    for (int i = 0; i < k_threads; ++i) {
        int end_i = start_i + elem_thread;
        
        if (i < remainder_elem) { //расперед ост
            end_i++;
        }

        arr_thread[i] = std::thread(maxmin, arr, start_i, end_i, &local_max[i], &local_min[i]);
        start_i = end_i;
    }

    for (int i = 0; i < k_threads; ++i) {
        arr_thread[i].join();//
    }

    //время
    auto end_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i< k_threads; ++i) {
        if (gl_max < local_max[i]) {
            gl_max = local_max[i];
        }
        if (gl_min > local_min[i]) {
            gl_min = local_min[i];
        }
    }

    //время
    auto multi_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    //однопоточка
    int one_max, one_min;
    auto start_single = std::chrono::high_resolution_clock::now();

    maxmin(arr, 0, arr_size, &one_max, &one_min);

    auto end_single = std::chrono::high_resolution_clock::now();
    auto single_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_single - start_single);

    double speedup = (double)single_duration.count() / multi_duration.count();
    double efficiency = (speedup / k_threads) * 100;


    std::cout  << k_threads << " потоков:" << std::endl;
    std::cout << "  минимум: " << gl_min << std::endl;
    std::cout << "  максимум: " << gl_max << std::endl;
    std::cout << "  время выполнения: " << multi_duration.count() << " мксек" << std::endl;
    
    std::cout << "\nоднопоточный поиск:" << std::endl;
    std::cout << "  минимум: " << one_min << std::endl;
    std::cout << "  максимум: " << one_max << std::endl;
    std::cout << "  время выполнения: " << single_duration.count() << " мксек" << std::endl;

    std::cout << "\nускорение: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    std::cout << "эффективность: " << std::setprecision(2) << efficiency << "%" << std::endl;
    
    if (speedup > 1.0) {
        std::cout << "многопоточная версия быстрее в " << speedup << " раз" << std::endl;
    } else if (speedup < 1.0) {
        std::cout << "многопоточная версия медленнее" << std::endl;
    } else {
        std::cout << "одинаково" << std::endl;
    }

    delete[] arr;
    delete[] arr_thread;
    delete[] local_min;
    delete[] local_max;

    return 0;

}