#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "library_loader.h"//при компиляции подключаем либ

using PiFunc = float(*)(int);//объявляем указ на ф
using SortFunc = int*(*)(int*, int);

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

    LibraryLoader loader;//инициализация и загрузка либ
    bool useLib1 = true; //флаг для переключения либ true - lib1 false - lib2
    
    if (!loader.load("lib1")) { //загрузка в память
        std::cerr << "не удалось загрузить lib1: " << loader.getError() << std::endl;
        return 1;
    }
    
    auto pi_func = loader.getFunction<PiFunc>("Pi");//получ указ
    auto sort_func = loader.getFunction<SortFunc>("Sort");
    
    if (!pi_func || !sort_func) {
        std::cerr << "не удалось найти функции в библиотеке" << std::endl;
        return 1;
    }
    
    std::cout << "\n загружена " << loader.getCurrentLibraryPath() << std::endl;
    std::cout << "алгоритмы: ряд Лейбница, пузырьковая сортировка" << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "\n";
        std::getline(std::cin, line);
        
        if (line == "exit") break;
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "0") {
            useLib1 = !useLib1;
            std::string lib_name = useLib1 ? "lib1" : "lib2"; //переключение
            
            std::cout << "переключаем на библиотеку: " << lib_name << std::endl;
            
            if (!loader.load(lib_name)) {
                std::cerr << "ошибка загрузки: " << loader.getError() << std::endl;
                useLib1 = !useLib1;//вернулись к прошлой либ
                continue;
            }
            
            pi_func = loader.getFunction<PiFunc>("Pi");//получ новых указателей на ф из либ2
            sort_func = loader.getFunction<SortFunc>("Sort");
            
            if (!pi_func || !sort_func) {
                std::cerr << "функции нет" << std::endl;
                continue;
            }
            
            std::cout << "загружена: " << loader.getCurrentLibraryPath() << std::endl;
            std::cout << "алгоритмы: " 
                      << (useLib1 ? "ряд Лейбница, пузырьковая сортировка" 
                                  : "формула Валлиса, сортировка Хоара") 
                      << std::endl;
        }
        else if (command == "1") {
            int K;
            if (iss >> K && K > 0) {
                float result = pi_func(K);
                std::cout << "Pi(" << K << ") = " << result << std::endl;
            } else {
                std::cout << "неверный аргумент" << std::endl;
            }
        }
        else if (command == "2") {
            std::vector<int> arr;
            int num;
            std::cout << "введите массив: ";
            while (iss >> num) {
                arr.push_back(num);
            }
            
            if (!arr.empty()) {
                std::cout << "массив: ";
                printArray(arr.data(), arr.size());
            
                sort_func(arr.data(), arr.size());
                
                std::cout << "отсортированный массив: ";
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