# 5G NR / Generic LDPC Belief Propagation Flooding Sum-Product Algorithm (SPA) Decoder
# Implemented as a StreamPU Stateful_Julia module with:
#   1. Shared read-only state (`LDPC_Graph`, shared = true): parity-check matrix H topology & parameters
#   2. Per-thread mutable state (`LDPC_Workspace`, shared = false): BP extrinsic message buffers

# Immutable read-only Tanner graph representation of the LDPC parity-check matrix H
# and decoder configuration parameters. Shared across all worker threads (`shared = true`).
# - `n_ite`: maximum number of Belief Propagation (BP) flooding iterations.
# - `enable_syndrome`: whether early termination via soft syndrome checking is enabled.
# - `syndrome_depth`: number of consecutive valid syndromes required to trigger early stop.
# - `var_offsets`: 1-based CSR/CSC offsets of variable node edges (`length = n_var_nodes + 1`).
# - `chk_offsets`: 1-based CSR/CSC offsets of check node edges (`length = n_chk_nodes + 1`).
# - `transpose`: maps each check-node branch index `k` (`1..n_branches`) to its corresponding
#                variable-node branch index (`1..n_branches`) in `msg_var_to_chk` / `msg_chk_to_var`.
# - `chk_to_var`: maps each check-node branch index `k` (`1..n_branches`) to its 1-based variable node index.
# - `info_bits_pos`: 1-based positions of the `K` systematic information bits within the `N_cw` codeword.
struct LDPC_Graph
    n_ite::Int32
    enable_syndrome::Bool
    syndrome_depth::Int32
    var_offsets::Vector{Int32}
    chk_offsets::Vector{Int32}
    transpose::Vector{Int32}
    chk_to_var::Vector{Int32}
    info_bits_pos::Vector{Int32}
end

# Constructs the `LDPC_Graph` Tanner graph structures (CSR/CSC offsets, edge transpose permutation,
# check-to-variable node map, and 1-based information bit indices) directly from raw C++ pointers
# to the packed check-node adjacency list (`H.get_col_to_rows()`) and `info_bits_pos` vector.
function LDPC_Graph(
    n_ite::Int32,
    enable_syndrome::Bool,
    syndrome_depth::Int32,
    n_var_nodes::Int32,
    n_chk_nodes::Int32,
    h_cols_addr::UInt,    h_cols_len::Int32,
    info_bits_addr::UInt, K::Int32
)
    h_cols        = unsafe_wrap(Array, Ptr{UInt32}(h_cols_addr),    Int(h_cols_len))
    info_bits_raw = unsafe_wrap(Array, Ptr{UInt32}(info_bits_addr), Int(K))

    # 1. Count variable node degrees and build chk_offsets (1-based CSR/CSC offsets)
    var_degrees = zeros(Int32, Int(n_var_nodes))
    chk_offsets = Vector{Int32}(undef, Int(n_chk_nodes) + 1)
    chk_offsets[1] = Int32(1)

    idx = 1
    for c in 1:Int(n_chk_nodes)
        deg = Int32(h_cols[idx])
        idx += 1
        chk_offsets[c + 1] = chk_offsets[c] + deg
        for _ in 1:deg
            v = Int(h_cols[idx]) + 1
            var_degrees[v] += Int32(1)
            idx += 1
        end
    end

    var_offsets = Vector{Int32}(undef, Int(n_var_nodes) + 1)
    var_offsets[1] = Int32(1)
    for v in 1:Int(n_var_nodes)
        var_offsets[v + 1] = var_offsets[v] + var_degrees[v]
    end

    # 2. Build transpose and chk_to_var (1-based indices)
    n_branches  = Int(chk_offsets[end] - 1)
    transpose   = Vector{Int32}(undef, n_branches)
    chk_to_var  = Vector{Int32}(undef, n_branches)
    connections = zeros(Int32, Int(n_var_nodes))

    idx = 1
    k   = 1
    for c in 1:Int(n_chk_nodes)
        deg = Int(h_cols[idx])
        idx += 1
        for _ in 1:deg
            v = Int(h_cols[idx]) + 1
            idx += 1
            branch_id = var_offsets[v] + connections[v]
            connections[v] += Int32(1)
            transpose[k]  = branch_id
            chk_to_var[k] = Int32(v)
            k += 1
        end
    end

    # 3. Convert 0-based C++ info_bits_pos to 1-based Julia indices
    info_bits_pos = Vector{Int32}(undef, Int(K))
    for i in 1:Int(K)
        info_bits_pos[i] = Int32(info_bits_raw[i]) + Int32(1)
    end

    return LDPC_Graph(
        n_ite,
        enable_syndrome,
        syndrome_depth,
        var_offsets,
        chk_offsets,
        transpose,
        chk_to_var,
        info_bits_pos
    )
end

# Per-thread mutable working buffers (`shared = false`) used during BP flooding iterations.
# Each cloned thread instance owns its own `LDPC_Workspace` so decoding is allocation-free and thread-safe.
# - `msg_chk_to_var`: extrinsic LLR messages from check nodes to variable nodes (`length = n_branches`).
# - `msg_var_to_chk`: extrinsic LLR messages from variable nodes to check nodes (`length = n_branches`).
# - `post`: a posteriori LLR values for each variable node (`length = n_var_nodes`).
# - `values`: temporary scratchpad storing `tanh(|msg| / 2)` along a single check node (`length = max_chk_degree`).
mutable struct LDPC_Workspace
    msg_chk_to_var::Vector{Float32}
    msg_var_to_chk::Vector{Float32}
    post::Vector{Float32}
    values::Vector{Float32}
end

# Pre-allocates zero-initialized working buffers sized for the Tanner graph dimensions.
function LDPC_Workspace(n_var_nodes::Int32, n_branches::Int32, max_chk_degree::Int32)
    return LDPC_Workspace(
        zeros(Float32, Int(n_branches)),
        zeros(Float32, Int(n_branches)),
        zeros(Float32, Int(n_var_nodes)),
        zeros(Float32, Int(max_chk_degree))
    )
end

# Variable-Node (VN) update step of the flooding schedule:
# For each variable node `v`, computes the total a posteriori LLR (`Y_N[v] + sum(msg_chk_to_var)`)
# and subtracts each incoming check-to-variable message to form the extrinsic `msg_var_to_chk` messages.
@inline function _initialize_var_to_chk!(
    var_offsets::Vector{Int32},
    Y_N::AbstractVector{Float32},
    msg_chk_to_var::Vector{Float32},
    msg_var_to_chk::Vector{Float32}
)
    n_var_nodes = length(var_offsets) - 1
    @inbounds for v in 1:n_var_nodes
        e_start = Int(var_offsets[v])
        e_stop  = Int(var_offsets[v + 1]) - 1
        sum_msg = 0.0f0
        for e in e_start:e_stop
            sum_msg += msg_chk_to_var[e]
        end
        tmp = Y_N[v] + sum_msg
        for e in e_start:e_stop
            msg_var_to_chk[e] = tmp - msg_chk_to_var[e]
        end
    end
    return nothing
end

# Check-Node (CN) update step using the exact Sum-Product Algorithm (SPA / tanh-atanh boxplus rule):
# Matches AFF3CT's `Update_rule_SPA` implementation, including exact handling of zero `tanh` factors
# (`n_zeros` counter) and clamping prior to `atanh` to prevent `Inf`/`NaN` saturation.
@inline function _decode_single_ite_spa!(
    chk_offsets::Vector{Int32},
    transpose::Vector{Int32},
    values::Vector{Float32},
    msg_var_to_chk::Vector{Float32},
    msg_chk_to_var::Vector{Float32}
)
    n_chk_nodes = length(chk_offsets) - 1
    max_tanh    = 1.0f0 - eps(Float32)

    @inbounds for c in 1:n_chk_nodes
        k_start = Int(chk_offsets[c])
        k_stop  = Int(chk_offsets[c + 1]) - 1

        sign    = false
        product = 1.0f0
        n_zeros = 0
        idx     = 1

        for k in k_start:k_stop
            var_val = msg_var_to_chk[transpose[k]]
            var_abs = abs(var_val)
            res     = tanh(var_abs * 0.5f0)
            is_zero = (res == 0.0f0)

            sign    = ifelse(is_zero, sign, sign ⊻ signbit(var_val))
            product = ifelse(is_zero, product, product * res)
            n_zeros = ifelse(is_zero, n_zeros + 1, n_zeros)
            values[idx] = res
            idx += 1
        end

        # Branch explicitly on `n_zeros` BEFORE calling `atanh` so `atanh` is only evaluated when
        # its result is actually needed (matching C++ `Update_rule_SPA::compute_chk_node_out` under
        # `-O3`). In 5G NR LDPC, the first `2 * Z_c` systematic variable nodes are punctured (`LLR = 0`),
        # so during early iterations most check nodes have `n_zeros >= 1`. Because Julia's `Base.atanh`
        # includes a `DomainError` check (`abs(x) > 1`), LLVM cannot dead-code-eliminate an unconditional
        # `atanh` call whose result is later discarded in favor of `0.0f0`.
        if n_zeros == 0
            idx = 1
            for k in k_start:k_stop
                t_k     = transpose[k]
                var_val = msg_var_to_chk[t_k]
                val_v   = values[idx]
                res_tmp = product / val_v
                res_tmp = ifelse(res_tmp < 1.0f0, res_tmp, max_tanh)
                res_abs = 2.0f0 * atanh(res_tmp)
                res_sng = sign ⊻ signbit(var_val)
                msg_chk_to_var[t_k] = ifelse(res_sng, -res_abs, res_abs)
                idx += 1
            end
        elseif n_zeros == 1
            res_tmp = ifelse(product < 1.0f0, product, max_tanh)
            res_abs = 2.0f0 * atanh(res_tmp)
            res_val = ifelse(sign, -res_abs, res_abs)
            idx = 1
            for k in k_start:k_stop
                t_k   = transpose[k]
                val_v = values[idx]
                msg_chk_to_var[t_k] = ifelse(val_v == 0.0f0, res_val, 0.0f0)
                idx += 1
            end
        else
            for k in k_start:k_stop
                msg_chk_to_var[transpose[k]] = 0.0f0
            end
        end
    end
    return nothing
end

# Computes the a posteriori LLR (`post[v]`) for every variable node `v` by summing the channel
# intrinsic LLR (`Y_N[v]`) and all incoming check-to-variable extrinsic messages.
@inline function _compute_post!(
    var_offsets::Vector{Int32},
    Y_N::AbstractVector{Float32},
    msg_chk_to_var::Vector{Float32},
    post::Vector{Float32}
)
    n_var_nodes = length(var_offsets) - 1
    @inbounds for v in 1:n_var_nodes
        e_start = Int(var_offsets[v])
        e_stop  = Int(var_offsets[v + 1]) - 1
        sum_msg = 0.0f0
        for e in e_start:e_stop
            sum_msg += msg_chk_to_var[e]
        end
        post[v] = Y_N[v] + sum_msg
    end
    return nothing
end

# Evaluates the parity-check syndrome `H * c_hat == 0 (mod 2)` from the hard decisions of the
# a posteriori LLRs (`post[v] < 0`). Returns `true` if all parity checks are satisfied.
@inline function _check_syndrome_soft(
    chk_offsets::Vector{Int32},
    chk_to_var::Vector{Int32},
    post::Vector{Float32}
)::Bool
    n_chk_nodes = length(chk_offsets) - 1
    @inbounds for c in 1:n_chk_nodes
        k_start = Int(chk_offsets[c])
        k_stop  = Int(chk_offsets[c + 1]) - 1
        sign    = false
        for k in k_start:k_stop
            llr = post[chk_to_var[k]]
            sign ⊻= (llr < 0.0f0)
        end
        if sign
            return false
        end
    end
    return true
end

# StreamPU `Stateful_Julia` codelet bound to the `"decode_siho"` task.
# Receives the shared `LDPC_Graph` state, the thread-local `LDPC_Workspace` state, the input LLR
# socket `sck_Y_N`, and the output decoded bits socket `sck_V_K`. Performs up to `n_ite` BP flooding
# SPA iterations with optional syndrome early stopping and extracts hard decisions on the `K` info bits.
function decode_siho_julia(
    graph::LDPC_Graph,
    work::LDPC_Workspace,
    sck_Y_N::AbstractVector{Float32},
    sck_V_K::AbstractVector{Int32},
    frame_id::UInt32 = UInt32(0),
    n_frames_per_wave::UInt32 = UInt32(1)
)::Int32
    K       = length(graph.info_bits_pos)
    n_ite   = Int(graph.n_ite)
    s_depth = Int(graph.syndrome_depth)

    fill!(work.msg_chk_to_var, 0.0f0)
    cur_syndrome_depth = 0
    valid_synd = true
    ite_done   = 0

    for ite in 1:n_ite
        _initialize_var_to_chk!(graph.var_offsets, sck_Y_N, work.msg_chk_to_var, work.msg_var_to_chk)
        _decode_single_ite_spa!(graph.chk_offsets, graph.transpose, work.values, work.msg_var_to_chk, work.msg_chk_to_var)

        if graph.enable_syndrome && ite != n_ite
            _compute_post!(graph.var_offsets, sck_Y_N, work.msg_chk_to_var, work.post)
            syndrome = _check_syndrome_soft(graph.chk_offsets, graph.chk_to_var, work.post)
            cur_syndrome_depth = syndrome ? ((cur_syndrome_depth + 1) % s_depth) : 0
            valid_synd = syndrome && (cur_syndrome_depth == 0)
            if valid_synd
                ite_done = ite
                break
            end
        end

        if ite == n_ite
            ite_done = n_ite
        end
    end

    if ite_done == n_ite
        _compute_post!(graph.var_offsets, sck_Y_N, work.msg_chk_to_var, work.post)
    end

    @inbounds for v in 1:K
        k = graph.info_bits_pos[v]
        sck_V_K[v] = ifelse(work.post[k] >= 0.0f0, Int32(0), Int32(1))
    end

    return ifelse(valid_synd, Int32(0), Int32(1))
end

# Triggers JIT type inference and compilation of `decode_siho_julia` (and its `@inline` helpers)
# for the exact concrete argument types passed by StreamPU (`LDPC_Graph`, `LDPC_Workspace`,
# 0-copy `Vector{Float32}` / `Vector{Int32}` sockets, and `UInt32` runtime arguments) when the
# script is evaluated, avoiding JIT compilation latency on the first frame in the pipeline.
precompile(decode_siho_julia, (LDPC_Graph, LDPC_Workspace, Vector{Float32}, Vector{Int32}, UInt32, UInt32))
