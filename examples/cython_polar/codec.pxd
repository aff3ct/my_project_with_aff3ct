from libcpp.vector cimport vector
from libcpp cimport bool

cdef extern from "src/codec.hpp":
    vector[bool] generate_frozen_bits(const int, const int, const float)
    vector[int] polar_encode(const int, const int, const vector[bool] &, const vector[int] &)
    vector[vector[int]] polar_encode_multiple(const int, const int, const vector[bool] &, const vector[vector[int]] &)
    vector[int] polar_decode(const int, const int, const vector[bool] &, const vector[float] &)
    vector[vector[int]] polar_decode_multiple(const int, const int, const vector[bool] &, const vector[vector[float]] &)
