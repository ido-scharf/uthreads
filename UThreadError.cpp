//
// Created by idoscharf on 13/06/24.
//

#include "UThreadError.h"

#include <utility>

UThreadError::UThreadError(const std::exception &base_exception, const ErrorType &type, std::string message) :
        std::exception(base_exception), type(type), message(std::move(message)) {}

UThreadError::UThreadError(const ErrorType &type, std::string message) : std::exception(),
    type(type), message(std::move(message)) {}
