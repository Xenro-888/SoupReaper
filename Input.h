#ifndef INPUT_H
#define INPUT_H
#include <iostream>
#include <functional>

typedef std::tuple<bool, std::string> ValidatorResult;
typedef std::function<ValidatorResult(std::string& input)> Validator;

std::string GetValidInput(const std::function<ValidatorResult(std::string& input)>& validator);

bool YesOrNo();

#endif //INPUT_H
