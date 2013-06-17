#ifndef FFT_INCLUDE_H
#define FFT_INCLUDE_H

#if FFT_SUPPORT

// C++
#include <vector>

// Local
#include "DataSample.h"
#include "TileIndex.h"

double min_time_required(const TileIndex &requested_index);
double max_time_required(const TileIndex &requested_index);

void windowed_fft(const std::vector<DataSample<double> > &samples,
    const TileIndex &requested_index,
    std::vector<std::vector<double> > &fft);

void present_fft(const std::vector<std::vector<double> > &fft,
    std::vector<std::vector<double> > &shifted,
    int &num_values);

#endif /* FFT_SUPPORT */

#endif /* FFT_INCLUDE_H */

