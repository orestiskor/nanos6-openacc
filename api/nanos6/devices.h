/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2018 Barcelona Supercomputing Center (BSC)
*/

#ifndef NANOS6_DEVICES_H
#define NANOS6_DEVICES_H

#include "major.h"

#if NANOS6_CUDA
#include "cuda_device.h"
#endif

#if NANOS6_OPENACC
#include "openacc_device.h"
#endif

#if NANOS6_OPENCL
#include "opencl_device.h"
#endif

#if (NANOS6_OPENACC || NANOS6_CUDA)
#include "device_mem_man.h"
#endif

#endif /* NANOS6_DEVICES_H */

