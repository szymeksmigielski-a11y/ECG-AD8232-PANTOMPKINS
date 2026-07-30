//Filters.cpp
#include "Filters.hpp"


std::vector<ECGSample> LowPassFilter::process(const std::vector<ECGSample>& rawdata){
	size_t n = rawdata.size();
	std::vector<ECGSample> filtereddata(n);

	for (int i = 0; i < n; i++) {
		filtereddata[i].timestamp = rawdata[i].timestamp;

		if (i >= 1) filtereddata[i].value += 2 * filtereddata[i - 1].value;
		if (i >= 2) filtereddata[i].value -= filtereddata[i - 2].value;
		filtereddata[i].value += rawdata[i].value;
		if (i >= 6)  filtereddata[i].value -= 2 * rawdata[i - 6].value;
		if (i >= 12) filtereddata[i].value += rawdata[i - 12].value;
	}

	return filtereddata;
}




std::vector<ECGSample> HighPassFilter::process(const std::vector<ECGSample>& rawdata){
	size_t n = rawdata.size();
	std::vector<ECGSample> filtereddata(n);

	for (int i = 0; i < n; i++) {


		if (i >= 1) filtereddata[i].value += filtereddata[i - 1].value;

		filtereddata[i].value -= (rawdata[i].value / 32.0);

		if (i >= 16) filtereddata[i].value += rawdata[i - 16].value;
		if (i >= 17) filtereddata[i].value -= rawdata[i - 17].value;

		if (i >= 32) filtereddata[i].value += (rawdata[i - 32].value / 32.0);

	}
	return filtereddata;
}