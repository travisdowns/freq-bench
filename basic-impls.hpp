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

#endif
