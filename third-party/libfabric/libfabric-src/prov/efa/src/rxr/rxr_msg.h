/*
 * Copyright (c) 2019-2020 Amazon.com, Inc. or its affiliates.
 * All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

static inline
void rxr_msg_construct(struct fi_msg *msg, const struct iovec *iov, void **desc,
		       size_t count, fi_addr_t addr, void *context, uint32_t data)
{
	msg->msg_iov = iov;
	msg->desc = desc;
	msg->iov_count = count;
	msg->addr = addr;
	msg->context = context;
	msg->data = data;
}

/**
 * multi recv related functions
 */


bool rxr_msg_multi_recv_buffer_available(struct rxr_ep *ep,
					 struct rxr_op_entry *rx_entry);

void rxr_msg_multi_recv_handle_completion(struct rxr_ep *ep,
					  struct rxr_op_entry *rx_entry);

void rxr_msg_multi_recv_free_posted_entry(struct rxr_ep *ep,
					  struct rxr_op_entry *rx_entry);

/**
 * functions to allocate rx_entry for two sided operations
 */
struct rxr_op_entry *rxr_msg_alloc_rx_entry(struct rxr_ep *ep,
					    const struct fi_msg *msg,
					    uint32_t op, uint64_t flags,
					    uint64_t tag, uint64_t ignore);

struct rxr_op_entry *rxr_msg_alloc_unexp_rx_entry_for_msgrtm(struct rxr_ep *ep,
							     struct rxr_pkt_entry **pkt_entry);

struct rxr_op_entry *rxr_msg_alloc_unexp_rx_entry_for_tagrtm(struct rxr_ep *ep,
							     struct rxr_pkt_entry **pkt_entry);

struct rxr_op_entry *rxr_msg_split_rx_entry(struct rxr_ep *ep,
					    struct rxr_op_entry *posted_entry,
					    struct rxr_op_entry *consumer_entry,
					    struct rxr_pkt_entry *pkt_entry);
/*
 * The following 2 OP structures are defined in rxr_msg.c and is
 * used by rxr_endpoint()
 */
extern struct fi_ops_msg rxr_ops_msg;

extern struct fi_ops_tagged rxr_ops_tagged;

ssize_t rxr_msg_post_medium_rtm(struct rxr_ep *ep, struct rxr_op_entry *tx_entry);

ssize_t rxr_msg_post_medium_rtm_or_queue(struct rxr_ep *ep, struct rxr_op_entry *tx_entry);
