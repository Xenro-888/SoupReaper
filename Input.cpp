#include "Input.h"

std::string GetValidInput(const std::function<std::tuple<bool, std::string>(std::string &input)>& validator) {
    std::string input;
    bool valid = false;
    std::string errorMesssage;

    while (!valid) {
        std::getline(std::cin, input);
        std::tie(valid, errorMesssage) = validator(input);
        if (!valid) {
            std::cout << errorMesssage << std::endl;
        }
    }

    return input;
}

bool YesOrNo() {
    std::string finalInput = GetValidInput([](const std::string& input) -> ValidatorResult {
        if (input == "Y" || input == "y" || input == "N" || input == "n")
            return {true, ""};
        return {false, "TRY AGAIN"};
    });

    std::cout << "FINAL INPUT: " << finalInput << "\n";

    if (finalInput == "Y" || finalInput == "y")
        return true;
    else
        return false;
}

