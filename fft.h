#ifndef FFT_INCLUDE_H
#define FFT_INCLUDE_H

const double MAX_SAMPLE_RATE_HERTZ = 10.0;

// When we convert to a string to pass to the client, we discretize
// into NUM_FFT_STEPS steps.
const int NUM_FFT_STEPS = 256;

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

