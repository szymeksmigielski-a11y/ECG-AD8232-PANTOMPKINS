//Stats.hpp
#ifndef STATS_HPP
#define STATS_HPP

#include <vector>
#include <algorithm>
#include "ECGSample.hpp"

class ECGStats {
private:
    double avgHR = 0.0;
    double minHR = 0.0;
    double maxHR = 0.0;
    int    totalBeats = 0;

public:
    double getAvgHR()    const { return avgHR; }
    double getMinHR()    const { return minHR; }
    double getMaxHR()    const { return maxHR; }
    int    getTotalBeats() const { return totalBeats; }
    friend ECGStats computeStats(const std::vector<ECGSample>&, double);
};

ECGStats computeStats(const std::vector<ECGSample>& qrsMarkers, double sampleRate = 200.0) {
    ECGStats stats;

    // zbierz indeksy wszystkich wykrytych QRS
    std::vector<int> qrsIndices;
    for (size_t i = 0; i < qrsMarkers.size(); i++) {
        if (qrsMarkers[i].value == 999999.0) {
            qrsIndices.push_back((int)i);
        }
    }

    stats.totalBeats = (int)qrsIndices.size();
    if (qrsIndices.size() < 2) return stats;

    // oblicz odstepy RR i przelicz na BPM
    std::vector<double> bpmValues;
    for (size_t i = 1; i < qrsIndices.size(); i++) {
        int rrInterval = qrsIndices[i] - qrsIndices[i - 1];
        double bpm = 60.0 * sampleRate / (double)rrInterval;
        // odfiltruj wartosci spoza zakresu fizjologicznego
        if (bpm >= 30.0 && bpm <= 220.0) {
            bpmValues.push_back(bpm);
        }
    }

    if (bpmValues.empty()) return stats;

    double sum = 0.0;
    stats.minHR = bpmValues[0];
    stats.maxHR = bpmValues[0];
    for (double bpm : bpmValues) {
        sum += bpm;
        if (bpm < stats.minHR) stats.minHR = bpm;
        if (bpm > stats.maxHR) stats.maxHR = bpm;
    }
    stats.avgHR = sum / (double)bpmValues.size();
    return stats;
}


#endif