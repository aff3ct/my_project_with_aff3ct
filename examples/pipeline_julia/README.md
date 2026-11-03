# Repetition Code Pipeline Example with StreamPU × Julia (`Stateless_Julia`)

This example demonstrates a 3-stage multi-threaded **Repetition Code** digital communication chain built with **AFF3CT** and **StreamPU**, showcasing a pedagogical 5-step optimization ladder (`v0` to `v4`) of a **Julia** repetition decoder (`src/rep_decoder.jl`) wrapped in `spu::module::Stateless_Julia`, compared against AFF3CT's native C++ scalar (`STD`) and SIMD (`FAST` / MIPP) decoders.

## Build

```bash
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -Wall -funroll-loops -march=native" \
         -DAFF3CT_DIR=$(pwd)/../../../aff3ct_install/lib/cmake/aff3ct \
         -Dcpptrace_DIR=$(pwd)/../../../aff3ct_install/lib/cmake/cpptrace
cmake --build . -j 4
```

## Run

* **AFF3CT C++ Decoders (`STD` scalar and `FAST` SIMD)**:
  ```bash
  ./bin/my_project -K 128 -N 512 --src-implem FAST --mdm-implem FAST --chn-implem FAST --dec-implem STD  -m 0.0 -M 6.0 -s 1.0 -e 100 -t 8
  ./bin/my_project -K 128 -N 512 --src-implem FAST --mdm-implem FAST --chn-implem FAST --dec-implem FAST -m 0.0 -M 6.0 -s 1.0 -e 100 -t 8
  ```

* **StreamPU × Julia Decoder (`v0` naive, `v1` zero-alloc scalar, `v2` SIMD safe, `v3` SIMD unsafe, `v4` SIMD unsafe + `single_wave` inter-frame batching)**:
  ```bash
  ./bin/my_project -K 32 -N 128 --src-implem FAST --mdm-implem FAST --chn-implem FAST --dec-jl-ver v0 -m 2.0 -M 2.0 --mnt-max-fra 65536 -t 8 -F 1
  ./bin/my_project -K 32 -N 128 --src-implem FAST --mdm-implem FAST --chn-implem FAST --dec-jl-ver v4 -m 2.0 -M 2.0 --mnt-max-fra 65536 -t 8 -F 32
  ```

