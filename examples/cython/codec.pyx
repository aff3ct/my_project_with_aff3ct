cimport cython


def py_generate_frozen_bits(k, n, snr_max):
    return generate_frozen_bits(k, n, snr_max)


def py_polar_encode(k, n, frozen_bits, info_bits):
    return polar_encode(k, n, frozen_bits, info_bits)


def py_polar_encode_multiple(k, n, frozen_bits, info_bits):
    assert info_bits.shape[1] > 0
    return polar_encode_multiple(k, n, frozen_bits, info_bits)


def py_polar_decode(k, n, frozen_bits, received):
    return polar_decode(k, n, frozen_bits, received)


def py_polar_decode_multiple(k, n, frozen_bits, received):
    assert received.shape[1] > 0
    return polar_decode_multiple(k, n, frozen_bits, received)