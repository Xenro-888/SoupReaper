#include <iostream>
#include <map>
#include <variant>
#include <vector>

#include "Windows.h"
#include "MemoryTools.h"
#include "Input.h"

typedef std::variant<int, float, bool, double> VaryingData;

DataType GetUserDataType() {
    // ask the user for the sought after data type
    std::cout << "ENTER THE DATA TYPE OF THE VALUE YOU'RE TRYING TO CHANGE.\n";
    const std::string dataTypeInput = GetValidInput([](const std::string& input)-> ValidatorResult {
        if (input != "INT" && input != "FLOAT" && input != "BOOL" && input != "DOUBLE")
            return {false, "DATA TYPES ARE: INT, FLOAT, BOOL, DOUBLE."};
        return {true, ""};
    });

    // determine the data type
    auto dataType = DataType::INT;
    if (dataTypeInput == "INT")
        dataType = DataType::INT;
    else if (dataTypeInput == "FLOAT")
        dataType = DataType::FLOAT;
    else if (dataTypeInput == "DOUBLE")
        dataType = DataType::DOUBLE;
    else if (dataTypeInput == "BOOL")
        dataType = DataType::BOOL;
    return dataType;
}

int main() {
    // HOOK ONTO THE PROCESS //
    // determine victim process
    HANDLE victimProcess = nullptr;
    while (victimProcess == nullptr) {
        std::cout << "ENTER THE VICTIM PROCESS NAME.\n";
        std::string victimName = GetValidInput([](const std::string&) -> ValidatorResult { return {true, ""}; });
        victimProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetProcessPIDByName(victimName));

        if (victimProcess == nullptr) {
            std::cout << "COULD NOT FIND VICTIM PROCESS. OPEN IT AND PRESS ANY KEY.\n";
            std::cin.get();
        }
        else
            std::cout << "VICTIM PROCESS FOUND: " << victimProcess << "\n";
    }
    ClearScreen();

    // get user wanted data type
    DataType dataType = GetUserDataType();

    // GET ADDRESS LIST //
    // ask the user what value they'd like to look for
    std::cout << "WHAT VALUE DOST THOU SEEK?\n";
    const std::string seekedValueInput = GetValidInput([&dataType](const std::string& input) -> ValidatorResult {
        if (dataType == DataType::INT && (std::isdigit(input[0]) || input[0] == '-') && !std::all_of(input.begin() + 1, input.end(), ::isdigit))
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
        return 0;
    }

    // ALLOW USER TO WIDDLE DOWN ADDRESSES //
    // sieve, widdle, etc
    ManipulateAddresses(victimProcess, hotAddresses, dataType);
    if (hotAddresses.empty()) {
        std::cout << "CLOSING UP SHOP. WE JUST WOULDN'T FIND SHIT.\n";
        std::cin.get();
        return 0;
    }

    // WRITE THE NEW MEMORY //
    // show the bussin' addresses
    ClearScreen();
    std::cout << "HERE WE FUCKIN' GO.\n";
    std::cout << "[THE HOLY ADDRESSES]\n";
    for (const uintptr_t address : hotAddresses)
        std::cout << reinterpret_cast<void*>(address) << "\n";
    std::cout << "\n";

    // ask the user what they want to write
    std::cout << "WHAT SHOULD THE VALUE BE NOW?\n";
    const std::string writeInput = GetValidInput([&dataType](const std::string& input) -> ValidatorResult {
        if (dataType == DataType::INT && !IsStringInt(input))
            return {false, "DID NOT ENTER INT."};
        if (dataType == DataType::FLOAT && !IsStringFloat(input))
            return {false, "DID NOT ENTER FLOAT."};
        return {true, ""};
    });

    // WRITE THE MEMORY
    // decode the data the user wants to write
    std::variant<int, float, double, bool> dataToWrite;
    switch (dataType) {
        case DataType::INT: {
            dataToWrite = std::stoi(writeInput);
            break;
        }
        case DataType::FLOAT: {
            dataToWrite = std::stof(writeInput);
            break;
        }
        case DataType::DOUBLE: {
            dataToWrite = std::stod(writeInput);
            break;
        }
        case DataType::BOOL: {
            dataToWrite = std::stoi(writeInput);
            break;
        }
    }

    // actually write the memory
    std::visit([&victimProcess, &hotAddresses](auto&& value) {
        WriteMemory(victimProcess, hotAddresses, value);
    }, dataToWrite);

    std::cout << "WRITTEN! ENJOY YOUR BUSTED GAME YOU FILTHY HACKER.\n";

    std::cin.get();
    return 0;
}