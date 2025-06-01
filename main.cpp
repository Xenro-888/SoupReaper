#include <iostream>
#include <map>
#include <vector>
#include <fstream>

#include "Windows.h"
#include "MemoryTools.h"
#include "Input.h"


std::vector<uintptr_t> FindNewAddresses(HANDLE victimProcess, DataType dataType) {
    // GET ADDRESS LIST //
    // ask the user what value they'd like to look for
    std::cout << "WHAT VALUE DOST THOU SEEK?\n";
    const std::string seekedValueInput = GetValidInput([&dataType](const std::string& input) -> ValidatorResult {
        if (dataType == DataType::INT && !IsStringInt(input))
            return {false, "DID NOT ENTER INT."};
        if (dataType == DataType::FLOAT && !IsStringFloat(input)) {
            return {false, "DID NOT ENTER FLOAT."};
        }
        return {true, ""};
    });

    // get the main address list
    std::vector<uintptr_t> hotAddresses = GetFirstHotAddresses(victimProcess, dataType, seekedValueInput);
    std::cout << hotAddresses.size() << " ADDRESSES FOUND WITH GIVEN VALUE.\n";
    if (hotAddresses.empty()) {
        std::cout << "CLOSING UP SHOP. TRY TO PROVIDE AN ACTUAL NUMBER IN RAM NEXT TIME, DIPSHIT.\n";
        std::cin.get();
        return {};
    }

    // ALLOW USER TO WIDDLE DOWN ADDRESSES //
    // sieve, widdle, etc
    ManipulateAddresses(victimProcess, hotAddresses, dataType);
    if (hotAddresses.empty()) {
        std::cout << "CLOSING UP SHOP. WE JUST WOULDN'T FIND SHIT.\n";
        std::cin.get();
        return {};
    }

    return hotAddresses;
}

int main() {
    HANDLE victimProcess = nullptr;
    std::vector<uintptr_t> addresses;

    // determine victim process
    std::cout << "ENTER THE VICTIM PROCESS NAME.\n";
    const std::string victimName = GetValidInput([&victimProcess](const std::string& input) -> ValidatorResult {
        victimProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetProcessPIDByName(input));
        if (victimProcess)
            return {true, ""};
        return {false, "COULD NOT FIND VICTIM PROCESS. OPEN IT AND PRESS ANY KEY."};
    });
    std::cout << "VICTIM PROCESS FOUND: " << victimProcess << "\n";
    std::cin.get();
    ClearScreen();

    while (true) {
        // ask the user what they want to do
        std::cout << "WHAT WOULD YOU LIKE TO DO?\n";
        std::cout << "ACTIONS ARE: FIND_NEW, OPEN_CACHE, CLOSE\n";
        std::string actionInput = GetValidInput([](const std::string& input) -> ValidatorResult {
            if (input == "FIND_NEW" || input == "OPEN_CACHE" || input == "CLOSe")
                return {true, ""};
            return {false, "ACTIONS ARE: FIND_NEW, OPEN_CACHE"};
        });

        DataType dataType = GetUserDataType();
        if (actionInput == "CLOSE")
            break;
        if (actionInput == "FIND_NEW") {
            addresses = FindNewAddresses(victimProcess, dataType);
        }
        else if (actionInput == "OPEN_CACHE") {
            std::cout << "ENTER ADDRESS KEY:\n";
            std::string input;
            std::getline(std::cin, input);
            addresses = GetCachedAddressesByName(input);
        }

        if (addresses.empty()) {
            std::cout << "NO ADDRESSES FOUND. TRY AGAIN\n";
            continue;
        }

        // WRITE THE NEW MEMORY //
        // cool console stuff
        ClearScreen();
        std::cout << "HERE WE FUCKIN' GO.\n";
        std::cout << addresses.size() << " ADDRESSES\n";

        // ask the user what they want to write
        std::cout << "WHAT SHOULD THE VALUE BE NOW?\n";
        const std::string writeInput = GetValidInput([&dataType](const std::string& input) -> ValidatorResult {
            if (dataType == DataType::INT && !IsStringInt(input))
                return {false, "DID NOT ENTER INT."};
            if (dataType == DataType::FLOAT && !IsStringFloat(input))
                return {false, "DID NOT ENTER FLOAT."};
            if (dataType == DataType::DOUBLE && !IsStringDouble(input))
                return {false, "DID NOT ENTER DOUBLE."};
            return {true, ""};
        });

        // memory write flags
        std::cout << "WOULD YOU LIKE TO WATCH THE ADDRESSES?\n";
        const bool watchAddresses = YesOrNo();
        std::cout << "WOULD YOU LIKE TO STEP THROUGH EACH ADDRESSES?\n";
        const bool stepThroughAddresses = YesOrNo();

        // address caching
        std::cout << "WOULD YOU LIKE TO CACHE THE ADDRESSES?\n";
        if (YesOrNo()) {
            std::ofstream writeCashFile;
            std::ifstream readCacheFile;

            writeCashFile.open("SavedAddresses.txt", std::ofstream::app);
            readCacheFile.open("SavedAddresses.txt");

            std::string fileContents;
            readCacheFile >> fileContents;
            const std::string addressKey = GetValidInput([&fileContents](const std::string& input) -> ValidatorResult {
                if (!fileContents.find(input))
                    return {true, ""};
                return {false, "KEY ALREADY EXISTS"};
            });

            writeCashFile << addressKey <<  ":";
            for (const uintptr_t& address : addresses)
                writeCashFile << address << (address != addresses.back() ? "." : "\n");
            writeCashFile << "\n";
            writeCashFile.close();
        }

        for (uintptr_t add : addresses)
            std::cout << add << "\n";

        // actually write the memory
        switch (dataType) {
            case DataType::INT: {
                WriteMemory(victimProcess, addresses, std::stoi(writeInput), watchAddresses, stepThroughAddresses);
                break;
            }
            case DataType::FLOAT: {
                WriteMemory(victimProcess, addresses, std::stof(writeInput), watchAddresses, stepThroughAddresses);
                break;
            }
            case DataType::DOUBLE: {
                WriteMemory(victimProcess, addresses, std::stod(writeInput), watchAddresses, stepThroughAddresses);
                break;
            }
        }

        std::cout << "WRITTEN TO " << addresses.size() << " ADDRESSES\n";
        std::cout << "ENJOY THE EXPLOIT YOU FILTHY HACKER.\n";
    }
    std::cin.get();
}