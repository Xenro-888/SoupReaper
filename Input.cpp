#include "Input.h"

std::string GetValidInput(const std::function<std::tuple<bool, std::string>(std::string &input)>& validator) {
    std::string input;
    bool valid = false;
    std::string errorMessage;

    while (!valid) {
        std::getline(std::cin, input);
        std::tie(valid, errorMessage) = validator(input);
        if (!valid) {
            std::cout << errorMessage << std::endl;
        }
    }

    return input;
}

bool YesOrNo() {
    const std::string finalInput = GetValidInput([](const std::string& input) -> ValidatorResult {
        if (input == "Y" || input == "y" || input == "N" || input == "n")
            return {true, ""};
        return {false, "TRY AGAIN"};
    });

    std::cout << "FINAL INPUT: " << finalInput << "\n";

    if (finalInput == "Y" || finalInput == "y")
        return true;
    return false;
}

