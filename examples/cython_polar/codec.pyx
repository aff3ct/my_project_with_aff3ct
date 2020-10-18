cimport cython


def py_generate_frozen_bits(k, n, snr_max):
    """Generate Frozen Bits following Arikan's method

    Parameters
    ----------
    k : int
        The length of information bits in a codeword
    n : int
        Codeword length
    snr_max : float
        estimated SNR in dB

    Returns
    -------
    list of booleans, size (n,)
        The frozen bits
    """
    return generate_frozen_bits(k, n, snr_max)


def py_polar_encode(k, n, frozen_bits, info_bits):
    """Polar encode

    Parameters
    ----------
    k : int
        The length of information bits in a codeword
    n : int
        Codeword length
    frozen_bits : list of booleans, size (n,)
        The frozen bits, maybe generated from `py_generate_frozen_bits`
    info_bits : list (or ndarray) of information bits of shape (k,)
        The information bits pending to be encoded

    Returns
    -------
    list of encoded bits, size (n,)
        The polar encoded bits
    """
    return polar_encode(k, n, frozen_bits, info_bits)


def py_polar_encode_multiple(k, n, frozen_bits, info_bits):
    """Polar encode for multiple frames

    Parameters
    ----------
    k : int
        The length of information bits in a codeword
    n : int
        Codeword length
    frozen_bits : list of booleans, size (n,)
        The frozen bits, maybe generated from `py_generate_frozen_bits`
    info_bits : list (or ndarray) of information bits of shape (n_frame, k)
        The information bits pending to be encoded

    Returns
    -------
    list of encoded bits, size (n_frame, n)
        The polar encoded bits
    """
    assert info_bits.shape[1] > 0
    return polar_encode_multiple(k, n, frozen_bits, info_bits)


def py_polar_decode(k, n, frozen_bits, received):
    """Polar decode

    Parameters
    ----------
    k : int
        The length of informatio bits in a codeword
    n : int
        Codeword length
    frozen_bits : list of booleans, size (n,)
        The frozen bits, maybe generated from `py_generate_frozen_bits`
    received : list (or ndarray) of received LLRs, size (n,)
        The received log-likelihood ratios (LLRs)

    Returns
    -------
    list of decoded bits, size (k,)
        The polar decoded bits
    """
    return polar_decode(k, n, frozen_bits, received)


def py_polar_decode_multiple(k, n, frozen_bits, received):
    """Polar decode for multiple frames

    Parameters
    ----------
    k : int
        The length of informatio bits in a codeword
    n : int
        Codeword length
    frozen_bits : list of booleans, size (n,)
        The frozen bits, maybe generated from `py_generate_frozen_bits`
    received : list (or ndarray) of received LLRs, size (n_frame, n)
        The received log-likelihood ratios (LLRs)

    Returns
    -------
    list of decoded bits, size (n_frame, k)
        The polar decoded bits
    """
    assert received.shape[1] > 0
    return polar_decode_multiple(k, n, frozen_bits, received)
