#ifndef FFT_INCLUDE_H
#define FFT_INCLUDE_H

#if FFT_SUPPORT

// C++
#include <vector>

// Local
#include "DataSample.h"
#include "TileIndex.h"

void take_fft(const std::vector<DataSample<double> > &samples,
        const TileIndex &requested_index,
        const TileIndex &client_tile_index,
        std::vector<double> &fft);

void shift_fft(const std::vector<double> &fft,
    std::vector<double> &shifted,
    int &num_values);

std::string fft_to_string(const std::vector<double> &fft, int &num_values);

#endif /* FFT_SUPPORT */

#endif /* FFT_INCLUDE_H */

