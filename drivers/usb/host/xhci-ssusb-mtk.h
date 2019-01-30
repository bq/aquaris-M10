/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Zhigang.Wun <zhigang.wei@mediatek.com>
 *  Chunfeng.Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _XHCI_MTK_H_
#define _XHCI_MTK_H_

#include "xhci.h"

/**
 * To simplify scheduler algorithm, set a upper limit for ESIT,
 * if a synchromous ep's ESIT is larger than @XHCI_MTK_MAX_ESIT,
 * round down to the limit value, that means allocating more
 * bandwidth to it.
 */
#define XHCI_MTK_MAX_ESIT	64

/**
 * struct mu3h_sch_bw_info: schedule information for bandwidth domain
 *
 * @bus_bw: array to keep track of bandwidth already used at each uframes
 * @bw_ep_list: eps in the bandwidth domain
 *
 * treat a HS root port as a bandwidth domain, but treat a SS root port as
 * two bandwidth domains, one for IN eps and another for OUT eps.
 */
struct mu3h_sch_bw_info {
	u32 bus_bw[XHCI_MTK_MAX_ESIT];
	struct list_head bw_ep_list;
};

/**
 * struct mu3h_sch_ep_info: schedule information for endpoint
 *
 * @esit: unit is 125us, equal to 2 << Interval field in ep-context
 * @num_budget_microframes: number of continuous uframes
 *		(@repeat==1) scheduled within the interval
 * @ep: address of usb_host_endpoint
 * @offset: which uframe of the interval that transfer should be
 *		scheduled first time within the interval
 * @repeat: the time gap between two uframes that transfers are
 *		scheduled within a interval. in the simple algorithm, only
 *		assign 0 or 1 to it; 0 means using only one uframe in a
 *		interval, and1 means using @num_budget_microframes
 *		continuous uframes
 * @pkts: number of packets to be transferred in the scheduled uframes
 * @cs_count: number of CS that host will trigger
 *		distribute the bMaxBurst+1 packets for a single burst
 *		according to @pkts and @repeat, repeate the burst multiple
 *		times; 1: distribute the (bMaxBurst+1)*(Mult+1) packets
 *		according to @pkts and @repeat. normal mode is used by
 *		default
 */
struct mu3h_sch_ep_info {
	u32 esit;
	u32 num_budget_microframes;
	u32 bw_cost_per_microframe;
	struct list_head endpoint;
	void *ep;
	/*
	 * mtk xHCI scheduling information put into reserved DWs
	 * in ep context
	 */
	u32 offset;
	u32 repeat;
	u32 pkts;
	u32 cs_count;
	u32 burst_mode;
};


#if IS_ENABLED(CONFIG_SSUSB_MTK_XHCI)

int xhci_mtk_init_quirk(struct xhci_hcd *xhci);
void xhci_mtk_exit_quirk(struct xhci_hcd *xhci);
int xhci_mtk_add_ep_quirk(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep);
void xhci_mtk_drop_ep_quirk(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep);
u32 xhci_mtk_td_remainder_quirk(unsigned int td_running_total,
	unsigned trb_buffer_length, struct urb *urb);

#else
static inline int xhci_mtk_init_quirk(struct xhci_hcd *xhci)
{
	return 0;
}

static inline void xhci_mtk_exit_quirk(struct xhci_hcd *xhci)
{
}

static inline int xhci_mtk_add_ep_quirk(struct usb_hcd *hcd,
	struct usb_device *udev, struct usb_host_endpoint *ep)
{
	return 0;
}

static inline void xhci_mtk_drop_ep_quirk(struct usb_hcd *hcd,
	struct usb_device *udev, struct usb_host_endpoint *ep)
{
	return 0;
}

u32 xhci_mtk_td_remainder_quirk(unsigned int td_running_total,
	unsigned trb_buffer_length, struct urb *urb)
{
	return 0;
}

#endif

#endif		/* _XHCI_MTK_H_ */
