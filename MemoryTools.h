#ifndef MEMORYTOOLS_H
#define MEMORYTOOLS_H
#include <vector>
#include <iostream>
#include <map>
#include "Windows.h"

// ENUMS //
enum class DataType {INT, FLOAT, DOUBLE, BOOL};
enum class SieveRule {ONLY_UP, ONLY_DOWN, STILL};

// FUNCTIONS //
void ClearScreen();
bool IsStringFloat(const std::string& string);
bool IsStringDouble(const std::string& string);
bool IsStringInt(const std::string& string);
double DetermineSearchRange(DataType dataType);
// Returns the PID of the process with the given name.
int GetProcessPIDByName(const std::string& name);
std::vector<uintptr_t> GetFirstHotAddresses(HANDLE victimProcess, DataType dataType, const std::string& seekedValueInput);
void Widdling(HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, DataType dataType);
void Sieving(HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, DataType dataType);
void ManipulateAddresses(HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, DataType dataType);

// TEMPLATE FUNCTIONS //
template<typename T>
void WriteMemory(HANDLE victimProcess, std::vector<uintptr_t>& addresses, T data) {
    for (const uintptr_t address : addresses)
        WriteProcessMemory(victimProcess, reinterpret_cast<LPVOID>(address), &data, sizeof(T), nullptr);
}

template<typename T>
T ReadAddress(HANDLE victimProcess, uintptr_t address) {
    T value;
    ReadProcessMemory(victimProcess, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
    return value;
}

template<typename T>
std::map<uintptr_t, T> MapAddressValues(HANDLE victimProcess, std::vector<uintptr_t>& addresses) {
    std::map<uintptr_t, T> valueMap;
    for (const uintptr_t& address : addresses) {
        T value = ReadAddress<T>(victimProcess, address);
        valueMap.emplace(address, value);
    }
    return valueMap;
}

template<typename T>
void Widdle(const HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, DataType dataType, T value, double range) {
    for (int i = hotAddresses.size() - 1; i >= 0; --i) {
        const uintptr_t address = hotAddresses.at(i);
        T readValue = ReadAddress<T>(victimProcess, address);

        bool inRange = (range > 0) && std::abs(readValue - value) < static_cast<T>(range);
        if (readValue != value || inRange)
            hotAddresses.erase(hotAddresses.begin() + i);
    }
}

// Filters out addresses that change value in the blacklisted manner.
template<typename T>
void SieveAddresses(const HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, SieveRule rule) {
    // map start values
    std::map<uintptr_t, T> valueMap = MapAddressValues<T>(victimProcess, hotAddresses);

    // start sieving
    std::map<SieveRule,bool(*)(T, T)> sieveRuleMap = {
        { SieveRule::ONLY_UP, [](T currentValue, T mappedValue) { return currentValue >= mappedValue; } },
        { SieveRule::ONLY_DOWN, [](T currentValue, T mappedValue) { return currentValue <= mappedValue; } },
        { SieveRule::STILL, [](T currentValue, T mappedValue) { return currentValue == mappedValue; } },
    };
    std::cout << "PRESS \"S\" TO STOP.\n";
    while (true) {
        if (GetKeyState('S') & 0x8000)
            break;

        for (int i = hotAddresses.size() - 1; i >= 0; --i) {
            const uintptr_t address = hotAddresses[i];
            const T value = ReadAddress<T>(victimProcess, address);
            if (sieveRuleMap[rule](value, valueMap[address])) {
                hotAddresses.erase(hotAddresses.begin() + i);
                ClearScreen();
                std::cout << "PRESS \"S\" TO STOP.\n";
                std::cout << hotAddresses.size() << " ADDRESSES LEFT.\n";
            }
        }
    }
}

template<typename T>
std::vector<uintptr_t> ScanMemoryForValue(const HANDLE process, const T valueToFind, const double range) {
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

    MEMORY_BASIC_INFORMATION memoryInfo;
    void* currentAddress = nullptr;

    std::vector<uintptr_t> candidateAddresses;

    // sweep the memory regions
    while (currentAddress < systemInfo.lpMaximumApplicationAddress) {
        // get the page
        const SIZE_T bytesRead = VirtualQueryEx(process, currentAddress, &memoryInfo, sizeof(MEMORY_BASIC_INFORMATION));
        if (!bytesRead)
            return {};

        currentAddress += memoryInfo.RegionSize;

        // check if memory can be read and written
        if (memoryInfo.State != MEM_COMMIT || memoryInfo.Protect != PAGE_READWRITE)
            continue;

        char* buffer = new char[memoryInfo.RegionSize];
        const bool success = ReadProcessMemory(process, memoryInfo.BaseAddress, buffer, memoryInfo.RegionSize, nullptr);
        if (!success) {
            delete[] buffer;
            continue;
        }

        // sweep the editable region
        for (size_t i = 0; i < memoryInfo.RegionSize - sizeof(T); i += sizeof(T)) {
            T value;
            memcpy(&value, buffer + i, sizeof(T));

            // if a decimal value was provided, check if the read value is within the given range
            bool inRange = (range > 0) && std::abs(value - valueToFind) < static_cast<T>(range);
            if (value == valueToFind || inRange)
                candidateAddresses.push_back(reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress) + i);
        }

        delete[] buffer;
    }

    return candidateAddresses;
}

template<typename T>
void WatchAddresses(HANDLE victimProcess, std::vector<uintptr_t>& addresses) {
    std::map<uintptr_t, T> valueMap = MapAddressValues<T>(victimProcess, addresses);

    while (true) {
        std::cout << "PRESS \"S\" TO STOP.\n";
        if (GetKeyState('S') & 0x8000)
            break;

        for (const uintptr_t& address : addresses) {
            T value = ReadAddress<T>(victimProcess, address);
            if (value == valueMap[address])
                continue;

            std::cout << "CHANGE: " << reinterpret_cast<void*>(address) << " -> " << value << std::endl;
            break;
        }
    }
}

#endif //MEMORYTOOLS_H
