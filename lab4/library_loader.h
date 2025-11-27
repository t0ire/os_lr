#pragma once

#include <string>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #define LIB_PREFIX ""
    #define LIB_SUFFIX ".dll"
#else
    #include <dlfcn.h>
    #define LIB_PREFIX "lib"
    #define LIB_SUFFIX ".so"
#endif

class LibraryLoader {
private:
#ifdef _WIN32
    HMODULE handle_;
#else
    void* handle_;
#endif

    std::string current_lib_path_;

public:
    LibraryLoader() : handle_(nullptr) {}
    
    ~LibraryLoader() {
        unload();
    }
    
    bool load(const std::string& libraryName) {
        unload();

        std::string full_path = LIB_PREFIX + libraryName + LIB_SUFFIX;
        current_lib_path_ = full_path;
        
        std::cout << "Загружаем библиотеку: " << full_path << std::endl;
        
#ifdef _WIN32
        handle_ = LoadLibraryA(full_path.c_str());
#else
        handle_ = dlopen(full_path.c_str(), RTLD_LAZY);
        if (!handle_) {
            full_path = "./" + full_path;
            handle_ = dlopen(full_path.c_str(), RTLD_LAZY);
        }
#endif
        
        if (!handle_) {
            std::cerr << "Ошибка загрузки: " << getError() << std::endl;
        }
        
        return handle_ != nullptr;
    }
    
    void unload() {
        if (handle_) {
#ifdef _WIN32
            FreeLibrary(handle_);
#else
            dlclose(handle_);
#endif
            handle_ = nullptr;
            current_lib_path_.clear();
        }
    }
    
    template<typename T>
    T getFunction(const std::string& functionName) {
        if (!handle_) {
            std::cerr << "Библиотека не загружена!" << std::endl;
            return nullptr;
        }
        
#ifdef _WIN32
        FARPROC func = GetProcAddress(handle_, functionName.c_str());
#else
        void* func = dlsym(handle_, functionName.c_str());
#endif
        
        if (!func) {
            std::cerr << "Функция '" << functionName << "' не найдена!" << std::endl;
            std::cerr << "Ошибка: " << getError() << std::endl;
            return nullptr;
        }
        
        return reinterpret_cast<T>(func);
    }
    
    std::string getError() const {
#ifdef _WIN32
        DWORD error = GetLastError();
        if (error == 0) return "";
        
        char* msgBuffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
            (LPSTR)&msgBuffer, 0, NULL);
        
        std::string message(msgBuffer, size);
        LocalFree(msgBuffer);
        return message;
#else
        const char* error = dlerror();
        return error ? std::string(error) : "";
#endif
    }
    
    std::string getCurrentLibraryPath() const {
        return current_lib_path_;
    }
    
    bool isLoaded() const {
        return handle_ != nullptr;
    }
};