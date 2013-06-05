#if FFT_SUPPORT

// C
#include <cassert>
#include <climits>

// C++
#include <algorithm>
#include <complex>
#include <numeric>
#include <string>
#include <vector>

// NFFT
#include <nfft3.h>

// Local
#include "DataSample.h"
#include "fft.h"
#include "TileIndex.h"

#define NRESULTS 25

// When we convert to a string to pass to the client, we discretize
// into NUM_FFT_STEPS steps.
#define NUM_FFT_STEPS 256

// Helper function declarations
inline double find_min_time(const std::vector<DataSample<double> > &samples);
inline double find_max_time(const std::vector<DataSample<double> > &samples);

// Implementation

void take_fft(const std::vector<DataSample<double> > &samples,
        const TileIndex &requested_index,
        const TileIndex &client_tile_index,
        std::vector<double> &fft) {
  if (samples.size() == 0)
    return;

  nfft_plan plan;
  nfft_init_1d(&plan, NRESULTS, samples.size());

  const double min_time = find_min_time(samples);
  const double max_time = find_max_time(samples);

  assert (samples.size() == (unsigned)(plan.M_total));
  for (unsigned i = 0; i < samples.size(); i++) {
    // NFFT requires X-values to be between -0.5 and 0.5
    plan.x[i] = ((samples[i].time - min_time) / max_time) - 0.5;
    plan.f[i][0] = samples[i].value;
    plan.f[i][1] = 0.0;
  }

  nfft_adjoint(&plan);

  // Copy results into fft
  for (unsigned i = 0; i < (unsigned)plan.N_total; i++) {
    std::complex<double> sample(plan.f_hat[i][0], plan.f_hat[i][1]);
    fft.push_back(abs(sample));
  }

  nfft_finalize(&plan);
}

void shift_fft(const std::vector<double> &fft,
    std::vector<double> &shifted,
    int &num_values) {
  num_values = NUM_FFT_STEPS;

  std::vector<double> fft_copy; // Copy of everything but the DC channel
  for (unsigned i = 0; i < fft.size() - 1; i++)
      fft_copy.push_back(fft[i]);

  double max_value = std::accumulate(fft_copy.begin(), fft_copy.end(),
      fft_copy[0], std::max<double>);
  if (max_value <= 0.0) {
    // Push all the values verbatim (they should all be 0) rather than
    // divide by 0
    for (int i = fft_copy.size() - 1; i >= 0; i--)
      shifted.push_back(fft_copy[i]);
  } else {
    for (int i = fft_copy.size() - 1; i >= 0; i--) {
      // The set of possible values is [0, 1, ..., NUM_FFT_STEPS - 1],
      // for a total of NUM_FFT_STEPS potential items
      double v = (fft_copy[i] / max_value) * (NUM_FFT_STEPS - 1);
      shifted.push_back(v);
    }
  }
}

std::string fft_to_string(const std::vector<double> &fft, int &num_values) {
  num_values = NUM_FFT_STEPS;

  std::string result;
  double max_value = std::accumulate(fft.begin(), fft.end(), fft[0], std::max<double>);
  for (unsigned i = 0; i < fft.size(); i++) {
    // The set of possible values is [0, 1, ..., NUM_FFT_STEPS - 1],
    // for a total of NUM_FFT_STEPS potential items
    double v = (fft[i] / max_value) * (NUM_FFT_STEPS - 1);
    result.append(std::string(1, (char)v));
  }
  return result;
}

inline double find_min_time(const std::vector<DataSample<double> > &samples) {
  double best_time = samples[0].time;
  for (unsigned i = 0; i < samples.size(); i++) {
    if (samples[i].time < best_time)
      best_time = samples[i].time;
  }
  return best_time;
}

inline double find_max_time(const std::vector<DataSample<double> > &samples) {
  double best_time = samples[0].time;
  for (unsigned i = 0; i < samples.size(); i++) {
    if (samples[i].time > best_time)
      best_time = samples[i].time;
  }
  return best_time;
}

#endif /* FFT_SUPPORT */

