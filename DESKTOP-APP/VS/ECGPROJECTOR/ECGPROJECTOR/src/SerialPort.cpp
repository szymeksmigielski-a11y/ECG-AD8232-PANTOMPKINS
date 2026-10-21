//SerialPort.cpp
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include "SerialPort.hpp"

SerialPort::~SerialPort() {
    close();
}

std::vector<std::string> SerialPort::listPorts() {
    std::vector<std::string> ports;
    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &key) != ERROR_SUCCESS)
        return ports;

    for (DWORD i = 0;; i++) {
        char name[256];
        char data[256];
        DWORD nameLen = sizeof(name);
        DWORD dataLen = sizeof(data);
        DWORD type = 0;
        LONG res = RegEnumValueA(key, i, name, &nameLen, nullptr, &type, reinterpret_cast<BYTE*>(data), &dataLen);
        if (res == ERROR_NO_MORE_ITEMS) break;
        if (res == ERROR_SUCCESS && type == REG_SZ)
            ports.push_back(std::string(data, strnlen(data, dataLen)));
    }
    RegCloseKey(key);
    return ports;
}

bool SerialPort::open(const std::string& portName, unsigned long baudRate) {
    close();

    std::string path = "\\\\.\\" + portName;   // wymagane dla COM10 i wyzej
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;

    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        return false;
    }
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    // DTR/RTS wylaczone - inaczej otwarcie portu resetuje ESP32
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        return false;
    }

    // ReadFile wraca od razu, gdy sa dane, a gdy ich nie ma - po max 100 ms
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 100;
    SetCommTimeouts(h, &timeouts);
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    handle = h;
    pending.clear();
    return true;
}

void SerialPort::close() {
    if (handle) {
        CloseHandle(static_cast<HANDLE>(handle));
        handle = nullptr;
    }
    pending.clear();
}

bool SerialPort::isOpen() const {
    return handle != nullptr;
}

int SerialPort::readLines(std::vector<std::string>& lines) {
    if (!handle)
        return -1;

    char buffer[512];
    DWORD bytesRead = 0;
    if (!ReadFile(static_cast<HANDLE>(handle), buffer, sizeof(buffer), &bytesRead, nullptr))
        return -1;

    for (DWORD i = 0; i < bytesRead; i++) {
        char c = buffer[i];
        if (c == '\n') {
            if (!pending.empty()) lines.push_back(pending);
            pending.clear();
        }
        else if (c != '\r') {
            pending += c;
        }
    }
    // dane bez konca linii (smieci) - nie zbieramy ich w nieskonczonosc
    if (pending.size() > 1024)
        pending.clear();

    return (int)bytesRead;
}
