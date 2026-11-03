# Pedagogical Soft-Input Hard-Output (SIHO) Repetition Decoder in Julia (`Stateless_Julia`)
# Demonstrates a 5-step progressive optimization ladder inside StreamPU:
#   - v0: Naive idiomatic Julia (dynamic heap allocations `zeros` + array slices per frame, safe, 1 frame/call)
#   - v1: Zero-allocation scalar loops + concrete types (`Float32`/`Int32`, safe, 1 frame/call)
#   - v2: Register-tiled SIMD (`@inbounds`, `@nexprs 16`, branchless `ifelse`) + `precompile` (safe, 1 frame/call)
#   - v3: Same SIMD kernel as v2 invoked with `jl_safe = false` (unsafe, 1 frame/call)
#   - v4: Same SIMD kernel + `set_single_wave(true)` processing all `n_frames_per_wave` (`F`) frames per call
#
# Both AFF3CT bit layouts are supported:
#   - `*_buf`   : default buffered encoding (`[U_K, U_K, ..., U_K]`, contiguous blocks of `K`)
#   - `*_nobuf` : unbuffered encoding (`--enc-no-buff`, `[b_1, ..., b_1, b_2, ..., b_2, ...]`)

using Base.Cartesian

# ==================================================================================================
# v0: Naive idiomatic implementation (allocates `ext` and slice copies on the heap every frame)
# ==================================================================================================

function decode_rep_v0_buf(sck_Y_N, sck_V_K, frame_id = UInt32(0), n_frames_per_wave = UInt32(1))
    K = length(sck_V_K)
    n_rep = div(length(sck_Y_N), K)
    ext = zeros(Float32, K)
    for r in 0:(n_rep - 1)
        ext .+= sck_Y_N[(r * K + 1):((r + 1) * K)]
    end
    for i in 1:K
        sck_V_K[i] = ext[i] < 0 ? 1 : 0
    end
    return Int32(0)
end

function decode_rep_v0_nobuf(sck_Y_N, sck_V_K, frame_id = UInt32(0), n_frames_per_wave = UInt32(1))
    K = length(sck_V_K)
    n_rep = div(length(sck_Y_N), K)
    ext = zeros(Float32, K)
    for i in 1:K
        off = (i - 1) * n_rep
        ext[i] = sum(sck_Y_N[(off + 1):(off + n_rep)])
    end
    for i in 1:K
        sck_V_K[i] = ext[i] < 0 ? 1 : 0
    end
    return Int32(0)
end

# ==================================================================================================
# v1: Zero-allocation scalar implementation with concrete types (no `@inbounds`, no SIMD tiling)
# ==================================================================================================

function decode_rep_v1_buf(
    sck_Y_N::Vector{Float32},
    sck_V_K::Vector{Int32},
    frame_id::UInt32 = UInt32(0),
    n_frames_per_wave::UInt32 = UInt32(1)
)::Int32
    K = length(sck_V_K)
    n_rep = div(length(sck_Y_N), K)
    for i in 1:K
        s = 0.0f0
        for r in 0:(n_rep - 1)
            s += sck_Y_N[r * K + i]
        end
        sck_V_K[i] = s < 0.0f0 ? Int32(1) : Int32(0)
    end
    return Int32(0)
end

function decode_rep_v1_nobuf(
    sck_Y_N::Vector{Float32},
    sck_V_K::Vector{Int32},
    frame_id::UInt32 = UInt32(0),
    n_frames_per_wave::UInt32 = UInt32(1)
)::Int32
    K = length(sck_V_K)
    n_rep = div(length(sck_Y_N), K)
    for i in 1:K
        off = (i - 1) * n_rep
        s = 0.0f0
        for r in 1:n_rep
            s += sck_Y_N[off + r]
        end
        sck_V_K[i] = s < 0.0f0 ? Int32(1) : Int32(0)
    end
    return Int32(0)
end

# ==================================================================================================
# Shared register-tiled SIMD helper for v2, v3, and v4
# Tiles `i` by 16 contiguous `Float32` lanes (512 bits: 4 NEON / 2 AVX2 / 1 AVX-512 registers),
# mirroring AFF3CT's `Decoder_repetition_fast` (`mipp::Reg<float>`) without any buffer copy.
# ==================================================================================================

@inline function _decode_rep_frame_buf_simd!(
    Y_N::Vector{Float32},
    y_off::Int,
    V_K::Vector{Int32},
    v_off::Int,
    K::Int,
    n_rep::Int
)
    i = 1
    @inbounds while i + 15 <= K
        y_base = y_off + i - 1
        @nexprs 16 w -> (s_w = Y_N[y_base + w])
        for r in 1:(n_rep - 1)
            r_base = y_base + r * K
            @nexprs 16 w -> (s_w += Y_N[r_base + w])
        end
        v_base = v_off + i - 1
        @nexprs 16 w -> (V_K[v_base + w] = ifelse(s_w < 0.0f0, Int32(1), Int32(0)))
        i += 16
    end
    @inbounds while i <= K
        s = Y_N[y_off + i]
        for r in 1:(n_rep - 1)
            s += Y_N[y_off + r * K + i]
        end
        V_K[v_off + i] = ifelse(s < 0.0f0, Int32(1), Int32(0))
        i += 1
    end
    return nothing
end

@inline function _decode_rep_frame_nobuf_simd!(
    Y_N::Vector{Float32},
    y_off::Int,
    V_K::Vector{Int32},
    v_off::Int,
    K::Int,
    n_rep::Int
)
    @inbounds for i in 1:K
        off = y_off + (i - 1) * n_rep
        s = 0.0f0
        @simd for r in 1:n_rep
            s += Y_N[off + r]
        end
        V_K[v_off + i] = ifelse(s < 0.0f0, Int32(1), Int32(0))
    end
    return nothing
end

# ==================================================================================================
# v2 (safe) & v3 (unsafe): SIMD kernel + branchless `ifelse` + `precompile` (1 frame per call)
# ==================================================================================================

function decode_rep_v2_buf(
    sck_Y_N::Vector{Float32},
    sck_V_K::Vector{Int32},
    frame_id::UInt32 = UInt32(0),
    n_frames_per_wave::UInt32 = UInt32(1)
)::Int32
    K = length(sck_V_K)
    n_rep = div(length(sck_Y_N), K)
    _decode_rep_frame_buf_simd!(sck_Y_N, 0, sck_V_K, 0, K, n_rep)
    return Int32(0)
end

function decode_rep_v2_nobuf(
    sck_Y_N::Vector{Float32},
    sck_V_K::Vector{Int32},
    frame_id::UInt32 = UInt32(0),
    n_frames_per_wave::UInt32 = UInt32(1)
)::Int32
    K = length(sck_V_K)
    n_rep = div(length(sck_Y_N), K)
    _decode_rep_frame_nobuf_simd!(sck_Y_N, 0, sck_V_K, 0, K, n_rep)
    return Int32(0)
end

# ==================================================================================================
# v4: Inter-frame wave batching (`set_single_wave(true)`) — processes all `F` frames in a single call
# ==================================================================================================

function decode_rep_v4_buf(
    sck_Y_N::Vector{Float32},
    sck_V_K::Vector{Int32},
    frame_id::UInt32 = UInt32(0),
    n_frames_per_wave::UInt32 = UInt32(1)
)::Int32
    n_fra = Int(n_frames_per_wave)
    K     = div(length(sck_V_K), n_fra)
    N_cw  = div(length(sck_Y_N), n_fra)
    n_rep = div(N_cw, K)
    for f in 0:(n_fra - 1)
        _decode_rep_frame_buf_simd!(sck_Y_N, f * N_cw, sck_V_K, f * K, K, n_rep)
    end
    return Int32(0)
end

function decode_rep_v4_nobuf(
    sck_Y_N::Vector{Float32},
    sck_V_K::Vector{Int32},
    frame_id::UInt32 = UInt32(0),
    n_frames_per_wave::UInt32 = UInt32(1)
)::Int32
    n_fra = Int(n_frames_per_wave)
    K     = div(length(sck_V_K), n_fra)
    N_cw  = div(length(sck_Y_N), n_fra)
    n_rep = div(N_cw, K)
    for f in 0:(n_fra - 1)
        _decode_rep_frame_nobuf_simd!(sck_Y_N, f * N_cw, sck_V_K, f * K, K, n_rep)
    end
    return Int32(0)
end

# Ahead-of-execution compilation for v2/v3 and v4 kernels
precompile(decode_rep_v2_buf,   (Vector{Float32}, Vector{Int32}, UInt32, UInt32))
precompile(decode_rep_v2_nobuf, (Vector{Float32}, Vector{Int32}, UInt32, UInt32))
precompile(decode_rep_v4_buf,   (Vector{Float32}, Vector{Int32}, UInt32, UInt32))
precompile(decode_rep_v4_nobuf, (Vector{Float32}, Vector{Int32}, UInt32, UInt32))