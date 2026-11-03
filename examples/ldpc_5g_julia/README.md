# 5G NR UL-SCH LDPC Pipeline Example with StreamPU × Julia (`Stateful_Julia`)

This example demonstrates a 3-stage multi-threaded **5G NR UL-SCH LDPC** digital communication chain built with **AFF3CT** and **StreamPU**, where the **LDPC Belief Propagation (BP) Flooding Sum-Product Algorithm (SPA)** decoder can be executed either with AFF3CT's native C++ implementation or with a **Julia** implementation (`src/ldpc_bp_flooding_spa.jl`) wrapped in `spu::module::Stateful_Julia`.

## Build

```bash
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-Wall -funroll-loops -march=native" \
         -DAFF3CT_DIR=$(pwd)/../../../aff3ct_install/lib/cmake/aff3ct \
         -Dcpptrace_DIR=$(pwd)/../../../aff3ct_install/lib/cmake/cpptrace
cmake --build . -j 4
```

## Run

### 1. 5G NR UL-SCH LDPC BFER Simulation over SNR Range (C++ vs. Julia Decoder)

* **AFF3CT C++ Decoder (`BP_FLOODING` + `SPA`)**:
  ```bash
  ./bin/my_project -K 512 -N 1024 --enc-type LDPC_5G --src-implem FAST --mdm-implem FAST --chn-implem FAST --dec-type BP_FLOODING --dec-implem SPA -i 20 -m 1.0 -M 2.5 -s 0.5 -e 50 -t 8
  ```

* **StreamPU × Julia Decoder (`Stateful_Julia`, `--dec-jl` or `--dec-implem SPA_JL`)**:
  ```bash
  ./bin/my_project -K 512 -N 1024 --enc-type LDPC_5G --src-implem FAST --mdm-implem FAST --chn-implem FAST --dec-type BP_FLOODING --dec-jl -i 20 -m 1.0 -M 2.5 -s 0.5 -e 50 -t 8
  ```

### 2. Binary File Transfer over the 5G NR LDPC Pipeline

```bash
./bin/my_project -K 512 -N 1024 --enc-type LDPC_5G --mdm-implem FAST --chn-implem FAST --dec-jl -i 20 -m 5.0 -M 5.0 -t 8 --src-type USER_BIN --src-path ./bin/my_project --src-no-reset --snk-type USER_BIN --snk-path ./my_project_received
diff ./bin/my_project ./my_project_received
```