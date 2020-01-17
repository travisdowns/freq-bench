#ifndef BASIC_IMPLS_H_
#define BASIC_IMPLS_H_

#include "common-cxx.hpp"

bench_fn dummy;

bench_fn vporxmm;
bench_fn vporymm;
bench_fn vporzmm;

bench_fn vporxmm_vz;
bench_fn vporymm_vz;
bench_fn vporzmm_vz;

bench_fn vporxmm_vz100;
bench_fn vporymm_vz100;
bench_fn vporzmm_vz100;

bench_fn vporxmm_tput_vz100;
bench_fn vporymm_tput_vz100;
bench_fn vporzmm_tput_vz100;

bench_fn vpermdxmm_vz100;
bench_fn vpermdymm_vz100;
bench_fn vpermdzmm_vz100;

bench_fn vpermdxmm_tput_vz100;
bench_fn vpermdymm_tput_vz100;
bench_fn vpermdzmm_tput_vz100;

bench_fn vporxymm250;
bench_fn vporyzmm250;

#define DEFINE250(rep) bench_fn vporxymm250_##rep;

ALL_RATIOS_X(DEFINE250)

bench_fn mulxymm250_10;

#endif
