//Filters.hpp
#ifndef FILTERS_HPP
#define FILTERS_HPP

#include <vector>
#include "ECGSample.hpp"
#include "IFilter.hpp"



class LowPassFilter : public IFilter {
public:
	std::vector<ECGSample> process(const std::vector<ECGSample>& rawdata) override;
};


class HighPassFilter : public IFilter {
public:
	std::vector<ECGSample> process(const std::vector<ECGSample>& rawdata) override;
};

#endif