/*
  Copyright (c) 2021  The Blosc Developers <blosc@blosc.org> 
  https://blosc.org
  License: BSD 3-Clause (see LICENSE.txt)
*/

#include <blosc-private.h>
#include "blosc2/filters-registry.h"
#include "ndmean/ndmean.h"
#include "ndcell/ndcell.h"
#include "bytedelta/bytedelta.h"

void register_filters(void) {

  blosc2_filter ndcell;
  ndcell.id = BLOSC_FILTER_NDCELL;
  ndcell.forward = (blosc2_filter_forward_cb) ndcell_encoder;
  ndcell.backward = (blosc2_filter_backward_cb) ndcell_decoder;
  register_filter_private(&ndcell);

  blosc2_filter ndmean;
  ndmean.id = BLOSC_FILTER_NDMEAN;
  ndmean.forward = (blosc2_filter_forward_cb) ndmean_encoder;
  ndmean.backward = (blosc2_filter_backward_cb) ndmean_decoder;
  register_filter_private(&ndmean);

  blosc2_filter bytedelta;
  bytedelta.id = BLOSC_FILTER_BYTEDELTA;
  bytedelta.forward = (blosc2_filter_forward_cb) bytedelta_encoder;
  bytedelta.backward = (blosc2_filter_backward_cb) bytedelta_decoder;
  register_filter_private(&bytedelta);

}
