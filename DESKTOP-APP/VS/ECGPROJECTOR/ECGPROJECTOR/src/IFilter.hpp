//IFilter.hpp
#ifndef IFILTER_HPP
#define IFILTER_HPP

#include <vector>
#include "ECGSample.hpp"

class IFilter {
public:
    virtual std::vector<ECGSample> process(const std::vector<ECGSample>& input) = 0;
    virtual ~IFilter() = default;
};

#endif