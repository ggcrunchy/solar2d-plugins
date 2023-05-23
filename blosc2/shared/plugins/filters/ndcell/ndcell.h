/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Copyright (c) 2021  The Blosc Developers <blosc@blosc.org>
  https://blosc.org
  License: BSD 3-Clause (see LICENSE.txt)

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#ifndef B2ND_NDCELL_H
#define B2ND_NDCELL_H

#include <blosc2.h>

#define NDCELL_MAX_DIM 8


int ndcell_encoder(const uint8_t* input, uint8_t* output, int32_t length, uint8_t meta, blosc2_cparams* cparams, uint8_t id);

int ndcell_decoder(const uint8_t* input, uint8_t* output, int32_t length, uint8_t meta, blosc2_dparams* dparams, uint8_t id);

#endif //B2ND_NDCELL_H


