
#ifndef __MODEL_H__
#define __MODEL_H__

#define __PREFIX(x) Model ## x

#include "Gap.h"

#ifdef __EMUL__
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <string.h>
#endif

#ifndef DONT_DUMP
#ifndef TENSOR_DUMP_FILE
    #define TENSOR_DUMP_FILE "tensor_dump_file.dat"
#endif
#include "helpers.h"
#endif

extern AT_HYPERFLASH_FS_EXT_ADDR_TYPE model_L3_Flash;

#endif
