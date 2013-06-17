#if FFT_SUPPORT

// C
#include <cassert>
#include <climits>
#include <cmath>

// C++
#include <algorithm>
#include <complex>
#include <numeric>
#include <string>
#include <vector>

// FFTW
#include <fftw3.h>

// Local
#include "DataSample.h"
#include "fft.h"
#include "TileIndex.h"

// Each window is four times the width of the included area
#define WINDOW_RATIO 4.0
#define WINDOW_EXTENSION_RATIO (((WINDOW_RATIO) / 2.0) - 0.5)

#define SAMPLE_RATE_HERTZ 10.0
#define SAMPLE_TIME_SECONDS (1.0 / SAMPLE_RATE_HERTZ)

#define MAX_NUM_SAMPLES 100000

#define FREQUENCY_CUTOFF 1000
#define NUM_BINNED_FREQUENCIES 1000

// When we convert to a string to pass to the client, we discretize
// into NUM_FFT_STEPS steps.
#define NUM_FFT_STEPS 256

// Helper function declarations
inline double interp_time(const int interp_idx,
    const int num_interp,
    const double start_time,
    const double end_time);
void take_fft(const int n, const double *samples, std::vector<double> &fft);
void fill_by_bin(const std::vector<double> &nums,
    const unsigned int n,
    const unsigned int nbins,
    std::vector<double> &binned);
inline unsigned min(unsigned a, unsigned b);
double average_bin(const std::vector<double> &nums,
    const unsigned int start_idx,
    const unsigned int binsize);

// Implementation

double min_time_required(const TileIndex &requested_index) {
  return requested_index.start_time()
      - requested_index.duration() * WINDOW_EXTENSION_RATIO;
}

double max_time_required(const TileIndex &requested_index) {
  return requested_index.end_time()
      + requested_index.duration() * WINDOW_EXTENSION_RATIO;
}

void windowed_fft(const std::vector<DataSample<double> > &samples,
    const TileIndex &client_tile_index,
    std::vector<std::vector<double> > &fft) {
  if (samples.size() == 0)
    return;

  for (unsigned window_id = 0; window_id < WINDOW_RATIO; window_id++) {
    double own_start_time = client_tile_index.start_time()
        + window_id * client_tile_index.duration() / WINDOW_RATIO;
    double own_end_time = client_tile_index.start_time()
        + (window_id + 1) * client_tile_index.duration() / WINDOW_RATIO;
    double start_time = own_start_time
        - client_tile_index.duration() * WINDOW_EXTENSION_RATIO;
    double end_time = own_end_time
        + client_tile_index.duration() * WINDOW_EXTENSION_RATIO;

    double window_duration = end_time - start_time;
    int64 raw_num_interp = (int64)ceil(window_duration * SAMPLE_RATE_HERTZ);
    int num_interp = raw_num_interp > MAX_NUM_SAMPLES ? MAX_NUM_SAMPLES : (int)num_interp;

    double *interp = (double *)calloc(num_interp, sizeof(double));
    assert (interp != NULL);

    int interp_idx = 0;
    // Fill in all interpolated samples up to the first given sample
    while (interp_idx < num_interp
        && samples[0].time >= interp_time(interp_idx, num_interp, start_time, end_time)) {
      interp[interp_idx] = samples[0].value;
      interp_idx++;
    }
    // Now fill in interpolated samples after the first given sample
    DataSample<double> prev_sample = samples[0];
    for (unsigned i = 1; i < samples.size(); i++) {
      // Invariant: as we exit a loop iteration, all interpolated samples
      // up to samples[i].time are filled in with values
      DataSample<double> sample = samples[i];
      double time_delta = sample.time - prev_sample.time;
      double value_delta = sample.value - prev_sample.value;

      double interp_idx_time = interp_time(interp_idx, num_interp, start_time, end_time);
      while (interp_idx < num_interp && sample.time >= interp_idx_time) {
        double dist_ratio = (interp_idx_time - sample.time) / time_delta;
        interp[interp_idx] = sample.value + dist_ratio * value_delta;
        interp_idx++;
        interp_idx_time = interp_time(interp_idx, num_interp, start_time, end_time);
      }
      prev_sample = sample;
    }
    // Now fill in interpolated sample after the last given sample
    while (interp_idx < num_interp) {
        interp[interp_idx] = samples[samples.size() - 1].value;
        interp_idx++;
    }

    // Actually take the FFT
    std::vector<double> window;
    take_fft(num_interp, interp, window);
    fft.push_back(window);
    free(interp);
  }
}

void take_fft(const int n, const double *samples, std::vector<double> &fft) {
  fftw_complex *input = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * n);
  assert (input != NULL);
  fftw_complex *output = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * n);
  assert (output != NULL);

  fftw_plan plan = fftw_plan_dft_1d(n, input, output, FFTW_FORWARD, FFTW_ESTIMATE);

  // It's required that you create the plan before filling in the input
  for (int i = 0; i < n; i++) {
    input[i][0] = samples[i];
    input[i][1] = 0.0;
  }

  fftw_execute(plan);

  for (int i = 0; i < n; i++) {
    std::complex<double> sample(output[i][0], output[i][1]);
    fft.push_back(abs(sample));
  }

  fftw_destroy_plan(plan);
  fftw_free(input);
  fftw_free(output);
}

inline double interp_time(const int interp_idx,
    const int num_interp,
    const double start_time,
    const double end_time) {
  double window_duration = end_time - start_time;
  return (interp_idx * window_duration / num_interp) + start_time;
}

void present_fft(const std::vector<std::vector<double> > &fft,
    std::vector<std::vector<double> > &shifted,
    int &num_values) {
  num_values = NUM_FFT_STEPS;

  for (unsigned window_id = 0; window_id < fft.size(); window_id++) {
    std::vector<double> window; // Copy everything but the DC channel
    for (unsigned i = 1; i < fft[window_id].size(); i++)
      window.push_back(fft[window_id][i]);

    // Now actually transform with binning etc. to get a reasonable tile
    std::vector<double> binned_window, shifted_window;
    fill_by_bin(window, FREQUENCY_CUTOFF, NUM_BINNED_FREQUENCIES, binned_window);
    double max_value = std::accumulate( // Take the max with a fold
        binned_window.begin(), binned_window.end(),
        binned_window[0], std::max<double>);
    // Now divide by the max
    if (max_value >= 0.0) {
      // Push all the values verbatim (they should all be 0) rather than
      // divide by 0
      for (unsigned i = 0; i < binned_window.size(); i++)
        shifted_window.push_back(binned_window[i]);
    } else {
      for (unsigned i = 0; i < binned_window.size(); i++) {
        // The set of possible values is [0, 1, ..., NUM_FFT_STEPS - 1],
        // for a total of NUM_FFT_STEPS potential items
        double v = (binned_window[i] / max_value) * (NUM_FFT_STEPS - 1);
        shifted_window.push_back(v);
      }
    }

    shifted.push_back(shifted_window);
  }
}

void fill_by_bin(const std::vector<double> &nums,
    const unsigned int nitems,
    const unsigned int nbins,
    std::vector<double> &binned) {
  unsigned binsize = min(nitems, nums.size()) / nbins;
  for (unsigned i = 0; i < nitems && i < nums.size(); i += binsize)
    binned.push_back(average_bin(nums, i, binsize));
}

inline unsigned min(unsigned a, unsigned b) {
  return a < b ? a : b;
}

double average_bin(const std::vector<double> &nums,
    const unsigned int start_idx,
    const unsigned int binsize) {
  double total = 0.0;
  int nitems = 0;

  for (unsigned i = start_idx; i < nums.size() && i < start_idx + binsize; i++) {
    total += nums[i];
    nitems++;
  }

  // This shouldn't happen
  if (nitems == 0)
    return 0;

  return total / nitems;
}

#endif /* FFT_SUPPORT */

