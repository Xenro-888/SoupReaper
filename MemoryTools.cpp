#include "MemoryTools.h"
#include <tlhelp32.h>
#include "Input.h"

void ClearScreen() {
    std::cout << "\x1b[2J";
    std::cout << "\x1b[0;0H";
}

bool IsStringFloat(const std::string& string) {
    try {
        std::stof(string);
        return true;
    } catch (...) {
        return false;
    }
}

bool IsStringDouble(const std::string& string) {
    try {
        std::stod(string);
        return true;
    } catch (...) {
        return false;
    }
}

bool IsStringInt(const std::string& string) {
    try {
        std::stoi(string);
        return true;
    } catch (...) {
        return false;
    }
}

// Returns the PID of the process with the given name.
int GetProcessPIDByName(const std::string& name) {
    PROCESSENTRY32 process;
    process.dwSize = sizeof(PROCESSENTRY32);
    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    do {
        if (process.szExeFile == name) {
            DWORD pid = process.th32ProcessID;
            CloseHandle(processesSnapshot);
            return pid;
        }
    } while (Process32Next(processesSnapshot, &process));

    CloseHandle(processesSnapshot);

    return 0;
}

// Returns a list of addresses containing the value the user asked for.
std::vector<uintptr_t> GetFirstHotAddresses(HANDLE victimProcess, const DataType dataType, const std::string& seekedValueInput) {
    ClearScreen();

    // scan process RAM for addresses with the value
    const double range = DetermineSearchRange(dataType);
    std::cout << "SEARCHING...\n";
    switch (dataType) {
        case DataType::INT:
            return ScanMemoryForValue<int>(victimProcess, std::stoi(seekedValueInput), range);
        case DataType::FLOAT:
            return ScanMemoryForValue<float>(victimProcess, std::stof(seekedValueInput), range);
        case DataType::DOUBLE:
            return ScanMemoryForValue<double>(victimProcess, std::stod(seekedValueInput), range);
        case DataType::BOOL:
            return ScanMemoryForValue<bool>(victimProcess, std::stoi(seekedValueInput), range);
        default: {
            std::cout << "THE FUCK KIND OF DATA TYPE DID YOU JUST GIVE ME?\n";
            return {};
        }
    }
}

void ManipulateAddresses(HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, DataType dataType) {
    while (!hotAddresses.empty()) {
        std::cout << hotAddresses.size() << " ADDRESSES LEFT.\n";
        std::cout << "WHAT WOULD YOU LIKE TO DO WITH THE ADDRESSES?\n";
        std::cout << "SIEVE, WIDDLE, STOP\n";
        std::string actionInput = GetValidInput([](const std::string& input) -> ValidatorResult {
            if (input == "SIEVE" || input == "WIDDLE" || input == "STOP")
                return {true, ""};
            return {false, "VALID ACTIONS ARE: SIEVE, WIDDLE, STOP"};
        });

        if (actionInput == "STOP")
            break;

        if (actionInput == "SIEVE")
            Sieving(victimProcess, hotAddresses, dataType);
        else if (actionInput == "WIDDLE") {
            Widdling(victimProcess, hotAddresses, dataType);
        }
    }
}

double DetermineSearchRange(const DataType dataType) {
    double range = 0;
    if (dataType == DataType::FLOAT || dataType == DataType::DOUBLE) {
        std::cout << "YOU ENTERED A DECIMAL DATA TYPE.\n";
        std::cout << "WHAT'S THE RANGE YOU'D LIKE THE COLLECTED VALUES TO FALL IN?\n";
        std::string rangeInput = GetValidInput([](const std::string& input) -> ValidatorResult {
            if (IsStringFloat(input) || IsStringDouble(input))
                return {true, ""};
            return {false, "DID NOT PROVIDE DECIMAL NUMBER\n"};
        });

        if (dataType == DataType::FLOAT)
            range = std::stod(rangeInput);
        else
            range = std::stod(rangeInput);
    }
    return range;
}

// Prompts the player to sieve and starts the sieving process.
void Sieving(HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, DataType dataType) {
    // ask the user for the sieve rule
    std::cout << "WHAT SIEVE RULE DO YOU WANT TO USE?\nONLYUP, ONLYDOWN, STILL\n";
    std::string sieveRuleInput = GetValidInput([](const std::string& input) -> ValidatorResult {
        if (input == "ONLYUP" || input == "ONLYDOWN" || input == "STILL")
            return {true, ""};
        return {false, "RULE FLAGS ARE: ONLYUP, ONLYDOWN, STILL\n"};
    });

    // determine the sieve rule
    std::map<std::string,SieveRule> sieveRules = {
        {"ONLYUP", SieveRule::ONLY_UP},
        {"ONLYDOWN", SieveRule::ONLY_DOWN},
        {"STILL", SieveRule::STILL},
    };
    const SieveRule rule = sieveRules[sieveRuleInput];

    // start sieving
    switch (dataType) {
        case DataType::INT: {
            SieveAddresses<int>(victimProcess, hotAddresses, rule);
            break;
        }
        case DataType::FLOAT: {
            SieveAddresses<float>(victimProcess, hotAddresses, rule);
            break;
        }
        case DataType::DOUBLE: {
            SieveAddresses<double>(victimProcess, hotAddresses, rule);
            break;
        }
        case DataType::BOOL: {
            SieveAddresses<bool>(victimProcess, hotAddresses, rule);
            break;
        }
        default:
            break;
    }
}

void Widdling(HANDLE victimProcess, std::vector<uintptr_t>& hotAddresses, DataType dataType) {
    while (hotAddresses.size() > 1) {
        ClearScreen();
        // ask if the search should be stopped
        std::cout << hotAddresses.size() << " ADDRESSES LEFT WITH GIVEN VALUE.\n";
        std::cout << "WANNA CALL IT HERE?\n";
        if (YesOrNo())
            break;

        // ask what the new value is
        std::cout << "CHANGE THE VALUE IN-GAME.\n";
        std::cout << "WHAT'S THE VALUE NOW?\n";
        std::string newValueInput = GetValidInput([&dataType](const std::string& input) -> ValidatorResult {
            if (dataType == DataType::INT && (std::isdigit(input[0]) || input[0] == '-') && !std::all_of(input.begin() + 1, input.end(), ::isdigit))
                return {false, "DID NOT ENTER INT."};
            if (dataType == DataType::FLOAT && !IsStringFloat(input)) {
                return {false, "DID NOT ENTER FLOAT."};
            }
            return {true, ""};
        });

        const double range = DetermineSearchRange(dataType);
        switch (dataType) {
            case DataType::INT: {
                Widdle<int>(victimProcess, hotAddresses, dataType, std::stoi(newValueInput), range);
                break;
            }
            case DataType::FLOAT: {
                Widdle<float>(victimProcess, hotAddresses, dataType, std::stof(newValueInput), range);
                break;
            }
            case DataType::DOUBLE: {
                Widdle<double>(victimProcess, hotAddresses, dataType, std::stod(newValueInput), range);
                break;
            }
            case DataType::BOOL: {
                Widdle<bool>(victimProcess, hotAddresses, dataType, std::stoi(newValueInput), range);
                break;
            }
        }
    }
}

