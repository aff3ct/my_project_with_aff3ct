import numpy as np
import matplotlib.pyplot as plt
from codec import *

marker = ["o", "^", "s", "X", "d", "o", "^", "s", "*", "d"]

if __name__ == "__main__":
    n = 512
    n_frame = 10000
    snr_range = np.arange(-5, 5, 0.5)

    all_data = []

    for i, k in enumerate([256, 300, 350, 400, 425, 450, 475, 500]):
        ber_curve = []
        for snr in snr_range:
            frozen_bits = py_generate_frozen_bits(k, n, snr)

            info_bits = np.random.randint(2, size=(n_frame, k))
            encoded = np.array(py_polar_encode_multiple(k, n, frozen_bits, info_bits))

            power = np.var(encoded)
            noise_power = power * 10 ** (-snr / 10.0)
            received = (
                -encoded * 2 + 1 + np.sqrt(noise_power) * np.random.randn(n_frame, n)
            )

            decoded = np.array(py_polar_decode_multiple(k, n, frozen_bits, received))

            ber = np.sum(info_bits != decoded) / info_bits.size
            ber_curve.append(ber)

            print(f"K={k}, SNR={snr:.2f}dB, BER={ber:.2e}")

        plt.plot(snr_range, ber_curve, label=f"R={k * 1.0 / n:.2f}", marker=marker[i])
        all_data.append({k: ber_curve})
    
    plt.title(f'Polar Performance using Python, N = {n}')
    plt.xlabel('SNR [dB]')
    plt.ylabel('BER')
    plt.ylim([1e-4, 0.52])
    plt.yscale('log')
    plt.legend()
    plt.grid(which='both', alpha=0.3, linestyle='-.')

    plt.show()
