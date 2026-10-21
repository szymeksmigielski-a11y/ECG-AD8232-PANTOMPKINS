//ECGRecorder.cpp
#include <charconv>
#include "ECGRecorder.hpp"
#include "SerialPort.hpp"

using Clock = std::chrono::steady_clock;

ECGRecorder::~ECGRecorder() {
    stop();
}

void ECGRecorder::start() {
    if (running) return;
    running = true;
    worker = std::thread(&ECGRecorder::run, this);
}

void ECGRecorder::stop() {
    running = false;
    recording = false;
    if (worker.joinable())
        worker.join();
    status = DeviceStatus::NO_DEVICE;
}

DeviceStatus ECGRecorder::getStatus() const {
    return status;
}

std::string ECGRecorder::getPortName() const {
    std::lock_guard<std::mutex> lock(mtx);
    return portName;
}

bool ECGRecorder::startRecording() {
    if (status != DeviceStatus::READY || recording)
        return false;
    {
        std::lock_guard<std::mutex> lock(mtx);
        samples.clear();
    }
    recordStart = Clock::now();
    recording = true;
    return true;
}

std::vector<ECGSample> ECGRecorder::stopRecording() {
    recording = false;
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<ECGSample> result = std::move(samples);
    samples.clear();
    return result;
}

bool ECGRecorder::isRecording() const {
    return recording;
}

size_t ECGRecorder::getSampleCount() const {
    std::lock_guard<std::mutex> lock(mtx);
    return samples.size();
}

double ECGRecorder::getRecordingSeconds() const {
    if (!recording) return 0.0;
    return std::chrono::duration<double>(Clock::now() - recordStart).count();
}

bool ECGRecorder::parseTuple(const std::string& line, int& value, long long& timestamp) {
    size_t open = line.find('(');
    size_t comma = line.find(',', open);
    size_t close = line.find(')', comma);
    if (open == std::string::npos || comma == std::string::npos || close == std::string::npos)
        return false;

    auto parseNumber = [&](size_t from, size_t to, auto& out) {
        while (from < to && line[from] == ' ') from++;
        while (to > from && line[to - 1] == ' ') to--;
        if (from == to) return false;
        auto res = std::from_chars(line.data() + from, line.data() + to, out);
        return res.ec == std::errc() && res.ptr == line.data() + to;
    };
    return parseNumber(open + 1, comma, value) && parseNumber(comma + 1, close, timestamp);
}

void ECGRecorder::run() {
    const auto PROBE_TIME    = std::chrono::milliseconds(1500);  // nasluch na kazdym porcie przy szukaniu
    const auto DATA_TIMEOUT  = std::chrono::milliseconds(2000);  // tyle bez danych = urzadzenie odlaczone
    const auto STATUS_WINDOW = std::chrono::milliseconds(1000);  // okno oceny stanu elektrod

    SerialPort port;
    std::vector<std::string> lines;
    Clock::time_point lastData, lastTuple, lastNonZero;

    while (running) {

        // --- SZUKANIE URZADZENIA: po kolei nasluchujemy na kazdym porcie ---
        if (!port.isOpen()) {
            status = DeviceStatus::NO_DEVICE;
            for (const std::string& name : SerialPort::listPorts()) {
                if (!running) break;
                if (!port.open(name)) continue;

                bool gotData = false;
                auto probeStart = Clock::now();
                while (running && Clock::now() - probeStart < PROBE_TIME) {
                    lines.clear();
                    int n = port.readLines(lines);
                    if (n < 0) break;
                    if (n > 0) { gotData = true; break; }
                }
                if (gotData) {
                    std::lock_guard<std::mutex> lock(mtx);
                    portName = name;
                    break;
                }
                port.close();
            }
            if (!port.isOpen()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            lastData = Clock::now();
            lastTuple = lastNonZero = Clock::time_point();
            status = DeviceStatus::CONNECTED;
        }

        // --- ODCZYT DANYCH ---
        lines.clear();
        int n = port.readLines(lines);
        auto now = Clock::now();
        if (n < 0) {
            port.close();
            continue;
        }
        if (n > 0) lastData = now;

        for (const std::string& line : lines) {
            int value = 0;
            long long timestamp = 0;
            if (!parseTuple(line, value, timestamp))
                continue;
            lastTuple = now;
            if (value != 0) lastNonZero = now;
            if (recording) {
                std::lock_guard<std::mutex> lock(mtx);
                samples.push_back({ timestamp, (double)value });
            }
        }

        if (now - lastData > DATA_TIMEOUT) {
            port.close();
            continue;
        }

        if (now - lastNonZero < STATUS_WINDOW)    status = DeviceStatus::READY;
        else if (now - lastTuple < STATUS_WINDOW) status = DeviceStatus::NO_ELECTRODES;
        else                                      status = DeviceStatus::CONNECTED;
    }
}
