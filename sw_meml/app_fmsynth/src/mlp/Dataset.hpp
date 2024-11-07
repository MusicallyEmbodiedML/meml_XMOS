#ifndef __DATASET_HPP__
#define __DATASET_HPP__

#include "../chans_and_data.h"
#include <vector>
#include <cstdint>
#include <cstddef>

class Dataset {
 public:
    using DatasetVector = std::vector< std::vector<float> >;
    static void Add(std::vector<float> &feature, std::vector<float> &label);
    static void Train();
    static void Clear();
    static void Load(std::vector< std::vector<float> > &features,
                     std::vector< std::vector<float> > &labels);
    static void Fetch(const std::vector< std::vector<float> > *features,
                     const std::vector< std::vector<float> > *labels);
    static DatasetVector& GetFeatures();
    static DatasetVector& GetLabels();
};

#endif  // __DATASET_HPP__
