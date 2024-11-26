//
// Created by idoscharf on 13/06/24.
//

#ifndef OS_EX2_2_UTHREADERROR_H
#define OS_EX2_2_UTHREADERROR_H

#include <stdexcept>
#include <string>


enum ErrorType { LIBRARY_ERROR, SYSTEM_ERROR };

class UThreadError : public std::exception {
public:
    UThreadError(const ErrorType& type, std::string message);
    UThreadError(const std::exception& base_exception, const ErrorType& type, std::string  message);

    ErrorType type;
    std::string message;
};


#endif //OS_EX2_2_UTHREADERROR_H
