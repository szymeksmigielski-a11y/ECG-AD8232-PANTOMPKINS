//ECGException.hpp
#ifndef ECGEXCEPTION_HPP
#define ECGEXCEPTION_HPP
#include <stdexcept>
#include <string>

class ECGException : public std::runtime_error {
public:
    explicit ECGException(const std::string& msg)
        : std::runtime_error(msg) {}
};

#endif