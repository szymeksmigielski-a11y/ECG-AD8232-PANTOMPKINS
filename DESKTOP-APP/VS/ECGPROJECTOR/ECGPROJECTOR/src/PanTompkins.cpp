//PanTompkins.cpp
#include "PanTompkins.hpp"
#include "Analysis.hpp"



PanTompkins_Pipeline::PanTompkins_Pipeline(int windowSize)
    : mwiWindowSize(windowSize)
{
    filters.push_back(std::make_unique<LowPassFilter>());
    filters.push_back(std::make_unique<HighPassFilter>());
    filters.push_back(std::make_unique<Differentiation>());
    filters.push_back(std::make_unique<Squaring>());
    filters.push_back(std::make_unique<MovingWindowIntegration>(windowSize));
}

PanTompkins_Pipeline::Result PanTompkins_Pipeline::run(const std::vector<ECGSample>& rawData) {
    std::vector<ECGSample> signal = rawData;

    for (auto& filter : filters) {
        signal = filter->process(signal);
    }

    AdaptiveThreshold at;
    auto qrs = at.process(signal);

    return { signal, signal, qrs };
}