/**
 * @file hsm_forwards.h
 *
 * Unwind dependencies b/t hsm header definitions in a friendly way.
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#pragma once
#ifndef __HSM_FORWARDS_H__
#define __HSM_FORWARDS_H__

#include "hsm_types.h"

/**
 * Pointer to an hsm_event_rec.
 */
typedef const struct hsm_event_rec *hsm_event;

/**
 * Pointer to an hsm_state_rec.
 */
typedef const struct hsm_state_rec *hsm_state;


/**
 * Pointer to an hsm_context_rec.
 */
typedef struct hsm_context_rec *hsm_context;

/**
 * Pointer to an hsm_machine_rec.
 */
typedef struct hsm_machine_rec *hsm_machine;

/**
 * Pointer to callback info.
 */
typedef const struct hsm_status_rec *hsm_status;

#endif // #ifndef __HSM_FORWARDS_H__
