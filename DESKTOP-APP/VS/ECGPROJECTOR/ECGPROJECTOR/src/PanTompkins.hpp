//PanTompkins.hpp
#pragma once
#include <vector>
#include <memory>
#include "ECGSample.hpp"
#include "IFilter.hpp"
#include "Filters.hpp"
#include "Analysis.hpp"

class PanTompkins_Pipeline {
private:
    std::vector<std::unique_ptr<IFilter>> filters;
    int mwiWindowSize;

public:
    struct Result {
        std::vector<ECGSample> integrated;
        std::vector<ECGSample> squared;
        std::vector<ECGSample> qrsMarkers;
    };

    PanTompkins_Pipeline(int windowSize = 30);
    Result run(const std::vector<ECGSample>& rawData);
};