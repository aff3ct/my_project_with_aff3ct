#include <iostream>
#include <string>
#include <vector>

#include <aff3ct.hpp>

using namespace aff3ct;

/**
 * @brief Generate frozen bits for a specific SNR
 *
 * @param k The number of information bits
 * @param n The codeword length
 * @param snr_max Estimated SNR (in dB)
 * @return auto std::vector<bool> of frozen bits
 */
auto generate_frozen_bits(const int k, const int n, const float snr_max) {
  auto r = static_cast<float>(k * 1.0 / n);
  const auto esn0 = tools::ebn0_to_esn0(snr_max, r);
  const auto sigma = tools::esn0_to_sigma(esn0);

  tools::Frozenbits_generator_GA frozen_bits_generator(k, n);
  auto noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
  noise->set_noise(sigma, snr_max, esn0);

  std::vector<bool> frozen_bits(n);
  frozen_bits_generator.set_noise(*noise);
  frozen_bits_generator.generate(frozen_bits);

  return frozen_bits;
}

/**
 * @brief Polar encoder for single frame
 *
 * @param k The number of information bits
 * @param n The codeword length
 * @param frozen_bits std::vector<bool>, frozen bits (length k)
 * @param info_bits std::vector<int>, information bits (length k)
 * @return auto Encoded codewords, std::vector<int> of length n
 */
auto polar_encode(const int k, const int n,
                  const std::vector<bool> &frozen_bits,
                  const std::vector<int> &info_bits) {
  std::vector<int> encoded_bits(n);
  module::Encoder_polar<int> polar_encoder(k, n, frozen_bits);
  polar_encoder.encode(info_bits, encoded_bits);
  return encoded_bits;
}

/**
 * @brief Polar encoder for multiple frames
 *
 * @param k The number of information bits
 * @param n The codeword length
 * @param frozen_bits std::vector<bool>, frozen bits (length k)
 * @param info_bits std::vector<int>, information bits (length k)
 * @return auto Encoded codewords, std::vector<int> of length n
 */
auto polar_encode_multiple(const int k, const int n,
                           const std::vector<bool> &frozen_bits,
                           const std::vector<std::vector<int>> &info_bits) {
  module::Encoder_polar<int> polar_encoder(k, n, frozen_bits);
  std::vector<std::vector<int>> encoded_bits;

  {
    for (auto frame : info_bits) {
      std::vector<int> encoded(n);
      polar_encoder.encode(frame, encoded);
      encoded_bits.push_back(encoded);
    }
  }
  return encoded_bits;
}

/**
 * @brief Polar decoder for single frame
 *
 * @param k The number of information bits
 * @param n The codeword length
 * @param frozen_bits std::vector<bool>, frozen bits (length k)
 * @param received std::vector<float> soft symbols, BPSK, length n
 * @return auto Decoded information bits, std::vector<int> of length k
 */
auto polar_decode(const int k, const int n,
                  const std::vector<bool> &frozen_bits,
                  const std::vector<float> &received) {
  std::vector<int> decoded_bits(k);
  module::Decoder_polar_SC_naive<int> polar_decoder(k, n, frozen_bits);
  polar_decoder.decode_siho(received, decoded_bits);
  return decoded_bits;
}

/**
 * @brief Polar decoder for multiframe
 *
 * @param k The number of information bits
 * @param n The codeword length
 * @param frozen_bits std::vector<bool>, frozen bits (length k)
 * @param received std::vector<float> soft symbols, BPSK, n_frame rows, n
 * columns
 * @return auto Decoded information bits, std::vector<std::vector<int>> of
 * n_frame rows, k columns
 */
auto polar_decode_multiple(const int k, const int n,
                           const std::vector<bool> &frozen_bits,
                           const std::vector<std::vector<float>> &received) {
  module::Decoder_polar_SC_naive<int> polar_decoder(k, n, frozen_bits);
  std::vector<std::vector<int>> decoded_bits;

  {
    for (auto frame : received) {
      std::vector<int> decoded(k);
      polar_decoder.decode_siho(frame, decoded);
      decoded_bits.push_back(decoded);
    }
  }
  return decoded_bits;
}