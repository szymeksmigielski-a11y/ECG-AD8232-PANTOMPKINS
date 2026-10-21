//SerialPort.hpp
#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

#include <string>
#include <vector>

// Prosta obsluga portu szeregowego (Windows, WinAPI)
class SerialPort {
private:
    void* handle = nullptr;   // HANDLE z WinAPI (windows.h tylko w .cpp)
    std::string pending;      // niepelna linia z poprzedniego odczytu

public:
    SerialPort() = default;
    ~SerialPort();
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    // nazwy portow COM zarejestrowanych w systemie, np. "COM3"
    static std::vector<std::string> listPorts();

    bool open(const std::string& portName, unsigned long baudRate = 115200);
    void close();
    bool isOpen() const;

    // dopisuje do lines kompletne linie (bez \r\n), czeka na dane max ~100 ms
    // zwraca liczbe odebranych bajtow albo -1 przy bledzie portu (np. odlaczony kabel)
    int readLines(std::vector<std::string>& lines);
};

#endif
