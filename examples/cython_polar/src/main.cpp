#include <cmath>
#include <memory>
#include <iostream>
#include <algorithm>
#include <aff3ct.hpp>
using namespace aff3ct;

#include "codec_polar.hpp"

template <typename T> void print_vector(std::vector<T> vec) {
  std::for_each(vec.begin(), vec.end(),
                [&](const T i) { std::cout << i << ' '; });
  std::cout << std::endl;
}

int main() {
  int k = 401;
  int n = 512;
  float snr_max = 6;

  // generate frozen bits
  auto frozen_bits = generate_frozen_bits(k, n, snr_max);

  // generate information bits
  auto source =
      std::unique_ptr<module::Source_random<>>(new module::Source_random<>(k));
  std::vector<int> info_bits(k);
  source->generate(info_bits);

  // polar encode
  auto encoded_bits = polar_encode(k, n, frozen_bits, info_bits);

  // generate gaussian noise
  const float mean = 0.0;
  const float stddev = std::sqrt(std::pow(10, -snr_max / 10.0));
  std::normal_distribution<float> dist(mean, stddev);
  std::default_random_engine generator;

  // awgn channel
  std::vector<float> received;
  std::transform(
      encoded_bits.begin(), encoded_bits.end(), std::back_inserter(received),
      [&](const int i) -> float { return -i * 2 + 1 + dist(generator); });

  // polar decode
  auto decoded_bits = polar_decode(k, n, frozen_bits, received);

  // print vectors
  std::cout << "--- info bits         ---" << std::endl;
  print_vector(info_bits);
  std::cout << "--- decoded bits      ---" << std::endl;
  print_vector(decoded_bits);

  // check for errors
  std::cout << "--- error positions   ---" << std::endl;
  for (auto i = 0; i < k; i++) {
    if (decoded_bits[i] != info_bits[i]) {
      std::cout << i << ' ';
    }
  }

  return 0;
}
