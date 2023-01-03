/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
*   OpenOCD Vortex86 target driver
*   Copyright (C) 2022 Atsuko Ito
*   me@atsukoi.to
*   based on
*   OpenOCD STM8 target driver
*   Copyright (C) 2017  Ake Rehnman
*   ake.rehnman(at)gmail.com
*   Copyright (C) 2009 Zachary T Welch <zw@superlucidity.net>
*   Copyright(c) 2013-2016 Intel Corporation.
*/

#ifndef OPENOCD_TARGET_VORTEX86_H
#define OPENOCD_TARGET_VORTEX86_H

// doge.jpg
#define VORTEX86_COMMON_MAGIC	0x55544D38U


#define VORTEX86_RESET	0x1
#define VORTEX86_HALT	0x3
#define VORTEX86_GO	0x5

#define VORTEX86_STATUS	0xf

#define VORTEX86_IRLEN	9
#define VORTEX86_16BIN	17

#define MAX_SCAN_SIZE	33

struct scan_blk {
	uint8_t out[MAX_SCAN_SIZE]; /* scanned out to the tap */
	uint8_t in[MAX_SCAN_SIZE]; /* in to our capture buf */
	struct scan_field field;
};


struct target;

struct vortex86_common {
	unsigned int common_magic;

	void *arch_info;

	struct jtag_tap *curr_tap;
	int forced_halt_for_reset;
	int flush;
};

static inline struct vortex86_common *
target_to_vortex86(struct target *target)
{
	return target->arch_info;
}
#endif /* OPENOCD_TARGET_VORTEX86_H */
