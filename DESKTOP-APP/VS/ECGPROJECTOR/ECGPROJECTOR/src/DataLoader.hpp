//Dataloader.hpp
#ifndef DATALOADER_HPP
#define DATALOADER_HPP
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "ECGSample.hpp"
#include "ECGException.hpp"



class DataLoader {
public:
    std::vector<ECGSample> loadFromCSV(const std::string& filename);
    std::vector<ECGSample> loadFromBinary(const std::string& filename);
    void saveToBinary(const std::string& filename,const std::vector<ECGSample>& data);
    void convertCsvToBinary(const std::string& csvPath);
    std::vector<std::string> scanFolder(const std::string& folderPath);
};

#endif