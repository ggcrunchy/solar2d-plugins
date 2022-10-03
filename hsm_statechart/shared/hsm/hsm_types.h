/**
 * @file hsm_types.h
 *
 * Typedefs and macros for common primitive types used by hsm-statechart.
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#pragma once
#ifndef __HSM_TYPES_H__
#define __HSM_TYPES_H__

#include <stddef.h>


/**
 * @def HSM_TRUE
 * 1
 */

 /**
 * @def HSM_FALSE
 * 0
 */


#ifdef __cplusplus
#define HSM_TRUE  true
#define HSM_FALSE false
typedef bool hsm_bool;
#else
typedef int hsm_bool;

#define HSM_TRUE  1
#define HSM_FALSE 0
#endif


/**
 * 32 bit unsigned integer
 */
typedef unsigned long hsm_uint32;

/**
 * 16 bit unsigned integer
 */
typedef unsigned short hsm_uint16;

/**
 * @brief 16
 *
 * Maximum depth of a machine with context.
 * 16 is a decent amt of hiearchy depth.
 * nesting of regions will yield new sets of 16.
 */
#define HSM_MAX_DEPTH 16



#if (_DEBUG || DEBUG)
#define HSM_ASSERT assert
#else
/**
 * maps to an assert in debug builds
 * compiles out in non-debug builds
 */
#define HSM_ASSERT(x) (void)(0)
#endif


#endif // #ifndef __HSM_TYPES_H__
