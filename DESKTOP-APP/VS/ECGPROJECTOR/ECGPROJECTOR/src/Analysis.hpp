//Analysis.hpp
#ifndef ANALYSIS_HPP
#define ANALYSIS_HPP
#include <vector>
#include <cmath>
#include "ECGSample.hpp"
#include "IFilter.hpp"

class Differentiation : public IFilter {
public:
    std::vector<ECGSample> process(const std::vector<ECGSample>& rawdata) override;
};

class Squaring : public IFilter {
public:
    std::vector<ECGSample> process(const std::vector<ECGSample>& rawdata) override;
};

class MovingWindowIntegration : public IFilter {
private:
    int width;
public:
    MovingWindowIntegration(int width_m);
    std::vector<ECGSample> process(const std::vector<ECGSample>& rawdata) override;
};

class AdaptiveThreshold : public IFilter {
private:
    double spki = 0.0;
    double npki = 0.0;
    double th1 = 0.0;
    int refractoryPeriod = 40;
public:
    std::vector<ECGSample> process(const std::vector<ECGSample>& rawdata) override;
};

#endif