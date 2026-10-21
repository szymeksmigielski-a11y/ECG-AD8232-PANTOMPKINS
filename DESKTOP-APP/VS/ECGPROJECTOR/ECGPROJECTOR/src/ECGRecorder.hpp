//ECGRecorder.hpp
#ifndef ECGRECORDER_HPP
#define ECGRECORDER_HPP

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ECGSample.hpp"

// Stan urzadzenia na podstawie tego, co przychodzi na port COM
enum class DeviceStatus {
    NO_DEVICE,       // nic nie przychodzi            -> "Podlacz Arduino"
    CONNECTED,       // przychodzi cokolwiek          -> "Podlaczono Arduino"
    NO_ELECTRODES,   // krotki z samymi zerami        -> "Podlacz elektrody"
    READY            // krotki (wartosc, timestamp)   -> "Elektrody podlaczone", mozna nagrywac
};

// Odczyt krotek "(wartosc, timestamp_ms)" z ESP32 w osobnym watku
class ECGRecorder {
private:
    std::thread worker;
    std::atomic<bool> running{ false };
    std::atomic<bool> recording{ false };
    std::atomic<DeviceStatus> status{ DeviceStatus::NO_DEVICE };

    mutable std::mutex mtx;            // chroni portName i samples
    std::string portName;
    std::vector<ECGSample> samples;
    std::chrono::steady_clock::time_point recordStart;

    void run();

public:
    ~ECGRecorder();

    void start();   // nasluch portow COM w osobnym watku
    void stop();    // koniec nasluchu (przerywa tez nagrywanie)

    DeviceStatus getStatus() const;
    std::string getPortName() const;

    bool startRecording();                    // tylko w stanie READY
    std::vector<ECGSample> stopRecording();   // zwraca nagrane probki
    bool isRecording() const;
    size_t getSampleCount() const;
    double getRecordingSeconds() const;

    // "(1234, 5678)" -> value = 1234, timestamp = 5678
    static bool parseTuple(const std::string& line, int& value, long long& timestamp);
};

#endif
