/**
 * @file hsm_status.h
 *
 * Structure used for consolidating callback parameters
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete statusrmation.
 */

#pragma once
#ifndef __HSM_STATUS_H__
#define __HSM_STATUS_H__


typedef struct hsm_status_rec hsm_status_t;

//---------------------------------------------------------------------------
/**
 * Structure used for consolidating callback parameters.
 *
 * @param hsm The #hsm_machine processing the event.
 * @param ctx The #hsm_context object returned by the state enter callback.
 * @param evt The #hsm_event that needs handling.
 *
 * @see hsm_callback_enter, hsm_callback_process_event, hsm_callback_action, hsm_callback_guard
 */
struct hsm_status_rec
{
    hsm_machine hsm;
    hsm_state   state;
    hsm_context ctx;
    hsm_event   evt;
};


#endif // #ifndef __HSM_STATUS_H__
