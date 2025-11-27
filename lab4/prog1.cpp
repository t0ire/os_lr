#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "mathlib.h"//напрямую загружаем либ 

void printArray(int* arr, int size) {
    std::cout << "[";
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i];
        if (i < size - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}

int main() {
    std::cout << "  0 - информация" << std::endl;
    std::cout << "  1 [длина]" << std::endl;
    std::cout << "  2 [массив]" << std::endl;
    std::cout << "  exit - выход" << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "\n";
        std::getline(std::cin, line);
        
        if (line == "exit") break;
        if (line == "0") {
            std::cout << "используется библиотека: lib1 (ряд Лейбница, пузырьковая сортировка)" << std::endl;
            continue;
        }
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "1") {
            int K;
            if (iss >> K) {
                float result = Pi(K);
                std::cout << "Pi(" << K << ") = " << result << std::endl;
            } else {
                std::cout << "неверный аргумент" << std::endl;
            }
        }
        else if (command == "2") {
            std::vector<int> arr;
            int num;
            while (iss >> num) {
                arr.push_back(num);
            }
            
            if (!arr.empty()) {
                std::cout << "массив: ";
                printArray(arr.data(), arr.size());
                
                Sort(arr.data(), arr.size());
                
                std::cout << "oтсортированный массив: ";
                printArray(arr.data(), arr.size());
            } else {
                std::cout << "массив пустой" << std::endl;
            }
        }
        else {
            std::cout << "неизвестная команда" << std::endl;
        }
    }
    
    return 0;
}