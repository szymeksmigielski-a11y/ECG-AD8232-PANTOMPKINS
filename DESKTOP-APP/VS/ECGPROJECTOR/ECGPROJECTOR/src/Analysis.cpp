//Analysis.cpp
#include "ECGSample.hpp"
#include <vector>
#include "Analysis.hpp"
#include <cmath>

std::vector<ECGSample> Differentiation::process(const std::vector<ECGSample>& rawdata) {
    size_t n = rawdata.size();
    std::vector<ECGSample> filtereddata(n);
    for (int i = 0; i < 4; i++) {
        filtereddata[i].timestamp = rawdata[i].timestamp;
        filtereddata[i].value = 0;
    }
    for (size_t i = 4; i < n; i++) {
        filtereddata[i].value = (2.0 * rawdata[i].value + rawdata[i - 1].value
            - rawdata[i - 3].value - 2.0 * rawdata[i - 4].value) / 8.0;
    }
    return filtereddata;
}

std::vector<ECGSample> Squaring::process(const std::vector<ECGSample>& rawdata) {
    size_t n = rawdata.size();
    std::vector<ECGSample> filtereddata(n);
    for (size_t i = 0; i < n; i++) {
        filtereddata[i].value = rawdata[i].value * rawdata[i].value;
    }
    return filtereddata;
}

MovingWindowIntegration::MovingWindowIntegration(int width_m) {
    width = width_m;
}

std::vector<ECGSample> MovingWindowIntegration::process(const std::vector<ECGSample>& rawdata) {
    size_t n = rawdata.size();
    std::vector<ECGSample> outputData(n);
    double currentSum = 0.0;
    for (size_t i = 0; i < n; i++) {
        outputData[i].timestamp = rawdata[i].timestamp;
        currentSum += rawdata[i].value;
        if (i >= (size_t)width) {
            currentSum -= rawdata[i - width].value;
        }
        int currentWindowWidth = (i < (size_t)width) ? (i + 1) : width;
        outputData[i].value = currentSum / (double)currentWindowWidth;
    }
    return outputData;
}

std::vector<ECGSample> AdaptiveThreshold::process(const std::vector<ECGSample>& rawdata) {
    size_t n = rawdata.size();  // <-- rawdata zamiast integratedData
    std::vector<ECGSample> outputData(n);
    double max_val = 0.0;
    int calibrationSize = std::min<size_t>(n, 400);
    for (int i = 0; i < calibrationSize; ++i) {
        outputData[i].timestamp = rawdata[i].timestamp;  // <-- rawdata
        if (rawdata[i].value > max_val) max_val = rawdata[i].value;  // <-- rawdata
    }
    spki = max_val * 0.5;
    npki = max_val * 0.1;
    th1 = npki + 0.25 * (spki - npki);
    int lastQrsIndex = -refractoryPeriod;
    for (size_t i = 0; i < n; i++) {
        outputData[i].timestamp = rawdata[i].timestamp;  // <-- rawdata
        outputData[i].value = th1;
        if (i >= 2 && i < n - 2) {
            double currentVal = rawdata[i].value;  // <-- rawdata
            if (currentVal > rawdata[i - 1].value && currentVal > rawdata[i - 2].value &&
                currentVal > rawdata[i + 1].value && currentVal > rawdata[i + 2].value) {
                double peak = currentVal;
                if (peak > th1 && ((int)i - lastQrsIndex) > refractoryPeriod) {
                    outputData[i].value = 999999.0;
                    lastQrsIndex = i;
                    spki = 0.125 * peak + 0.875 * spki;
                }
                else {
                    npki = 0.125 * peak + 0.875 * npki;
                }
                th1 = npki + 0.25 * (spki - npki);
            }
        }
    }
    return outputData;
}