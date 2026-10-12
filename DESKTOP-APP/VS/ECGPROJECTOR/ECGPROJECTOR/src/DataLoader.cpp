//DataLoader.cpp
#include <string>
#include <sstream>
#include "ECGSample.hpp"
#include <fstream>
#include "DataLoader.hpp"
#include "ECGException.hpp"
#include <iostream>
#include <filesystem>
#include <cstring>
#include <cstdint>

std::vector<ECGSample> DataLoader::loadFromCSV(const std::string& filename) {
    std::vector<ECGSample> data;
    std::ifstream file(filename);

    if (!file.is_open())
        throw ECGException("Nie mozna otworzyc pliku: " + filename);  // <-- rzucamy

    std::string line;
    std::getline(file, line); // pomiń nagłówek

    double last_valid_value = 0.0;
    const double MAX_ADC_VALUE = 4095.0;
    int lineNumber = 1;

    while (std::getline(file, line)) {
        lineNumber++;
        std::stringstream ss(line);
        std::string ts_str, val_str;
        if (std::getline(ss, ts_str, ',') && std::getline(ss, val_str, ',')) {
            try {
                long long ts = std::stoll(ts_str);
                double val = std::stod(val_str);
                if (val > MAX_ADC_VALUE || val <= 0)
                    val = last_valid_value;
                else
                    last_valid_value = val;
                data.push_back({ ts, val });
            }
            catch (const std::invalid_argument&) {
                throw ECGException("Nieprawidlowy format danych w linii " + std::to_string(lineNumber));
            }
            catch (const std::out_of_range&) {
                throw ECGException("Wartosc poza zakresem w linii " + std::to_string(lineNumber));
            }
        }
    }

    if (data.empty())
        throw ECGException("Plik nie zawiera zadnych danych: " + filename);

    std::cout << "Wczytano " << data.size() << " probek." << std::endl;
    return data;
}

std::vector<std::string> DataLoader::scanFolder(const std::string& folderPath) {
    if (!std::filesystem::exists(folderPath))
        throw ECGException("Folder nie istnieje: " + folderPath);

    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension();
            if (ext == ".csv" || ext == ".ekg")   
                files.push_back(entry.path().filename().string());
        }
    }
    if (files.empty())
        throw ECGException("Brak plikow z danymi w folderze: " + folderPath);

    std::cout << "Znaleziono " << files.size() << " plikow." << std::endl;
    return files;
}

std::vector<ECGSample> DataLoader::loadFromBinary(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw ECGException("Nie mozna otworzyc pliku binarnego: " + filename);

    char magic[4];
    file.read(magic, 4);
    if (file.gcount() != 4 || std::strncmp(magic, "EKGB", 4) != 0)
        throw ECGException("Nieprawidlowy format pliku binarnego: " + filename);

    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file)
        throw ECGException("Uszkodzony naglowek pliku: " + filename);

    std::vector<ECGSample> data(count);
    file.read(reinterpret_cast<char*>(data.data()), count * sizeof(ECGSample));

    if ((size_t)file.gcount() != count * sizeof(ECGSample))
        throw ECGException("Plik binarny jest uszkodzony lub niekompletny: " + filename);

    if (data.empty())
        throw ECGException("Plik binarny nie zawiera danych: " + filename);

    std::cout << "Wczytano " << data.size() << " probek (binarnie)." << std::endl;
    return data;
}

void DataLoader::saveToBinary(const std::string& filename,
    const std::vector<ECGSample>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw ECGException("Nie mozna utworzyc pliku binarnego: " + filename);

    file.write("EKGB", 4);
    uint32_t count = static_cast<uint32_t>(data.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(ECGSample));
}

void DataLoader::convertCsvToBinary(const std::string& csvPath) {
    std::vector<ECGSample> data = loadFromCSV(csvPath);

    std::filesystem::path p(csvPath);
    p.replace_extension(".ekg");

    saveToBinary(p.string(), data);

    std::cout << "Skonwertowano " << csvPath << " -> " << p.string() << std::endl;
}