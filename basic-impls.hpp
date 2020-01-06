#ifndef BASIC_IMPLS_H_
#define BASIC_IMPLS_H_

#include "common-cxx.hpp"

memcpy_fn memcpy_stdlib;
memcpy_fn memmove_stdlib;
memcpy_fn repmovsb;
memcpy_fn dummy;

memcpy_fn vporxmm;
memcpy_fn vporymm;
memcpy_fn vporzmm;

memcpy_fn vporxmm_vz;
memcpy_fn vporymm_vz;
memcpy_fn vporzmm_vz;

memcpy_fn vporxmm_vz100;
memcpy_fn vporymm_vz100;
memcpy_fn vporzmm_vz100;

memcpy_fn vporxmm_tput_vz100;
memcpy_fn vporymm_tput_vz100;
memcpy_fn vporzmm_tput_vz100;

memcpy_fn vpermdxmm_vz100;
memcpy_fn vpermdymm_vz100;
memcpy_fn vpermdzmm_vz100;

memcpy_fn vpermdxmm_tput_vz100;
memcpy_fn vpermdymm_tput_vz100;
memcpy_fn vpermdzmm_tput_vz100;

memcpy_fn vporxymm250;
memcpy_fn vporyzmm250;

#endif
