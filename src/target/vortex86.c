// SPDX-License-Identifier: GPL-2.0-or-later

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <helper/log.h>
#include "target.h"
#include "target_type.h"
#include "hello.h"
#include "jtag/interface.h"
#include "jtag/jtag.h"
#include "jtag/swim.h"
#include "register.h"
#include "breakpoints.h"
#include "algorithm.h"
#include "vortex86.h"


int vortex86_common_init_arch_info(struct target *t, struct vortex86_common *vortex86)
{
	t->arch_info = vortex86;
	vortex86->common_magic = VORTEX86_COMMON_MAGIC;
	vortex86->curr_tap = t->tap;
	vortex86->flush = 1;
	return ERROR_OK;
}

static struct scan_blk scan;

static int irscan(struct target *t, uint8_t *out,
			uint8_t *in, uint8_t ir_len)
{
	int retval = ERROR_OK;
	struct vortex86_common *vortex86 = target_to_vortex86(t);
	if (!t->tap) {
		retval = ERROR_FAIL;
		LOG_ERROR("%s invalid target tap", __func__);
		return retval;
	}
	if (ir_len != t->tap->ir_length) {
		retval = ERROR_FAIL;
		if (t->tap->enabled)
			LOG_ERROR("%s tap enabled but tap irlen=%d",
					__func__, t->tap->ir_length);
		else
			LOG_ERROR("%s tap not enabled and irlen=%d",
					__func__, t->tap->ir_length);
		return retval;
	}
	struct scan_field *fields = &scan.field;
	fields->num_bits = ir_len;
	fields->out_value = out;
	fields->in_value = in;
	jtag_add_ir_scan(vortex86->curr_tap, fields, TAP_IDLE);
	if (vortex86->flush) {
		retval = jtag_execute_queue();
		if (retval != ERROR_OK)
			LOG_ERROR("%s failed to execute queue", __func__);
	}
	return retval;
}

static int drscan(struct target *t, uint8_t *out, uint8_t *in, uint8_t len)
{
	int retval = ERROR_OK;
	uint64_t data = 0;
	struct vortex86_common *vortex86 = target_to_vortex86(t);
	if (!t->tap) {
		retval = ERROR_FAIL;
		LOG_ERROR("%s invalid target tap", __func__);
		return retval;
	}
	if (len > MAX_SCAN_SIZE || 0 == len) {
		retval = ERROR_FAIL;
		LOG_ERROR("%s data len is %d bits, max is %d bits",
				__func__, len, MAX_SCAN_SIZE);
		return retval;
	}
	struct scan_field *fields = &scan.field;
	fields->out_value = out;
	fields->in_value = in;
	fields->num_bits = len;
	jtag_add_dr_scan(vortex86->curr_tap, 1, fields, TAP_IDLE);
	if (vortex86->flush) {
		retval = jtag_execute_queue();
		if (retval != ERROR_OK) {
			LOG_ERROR("%s drscan failed to execute queue", __func__);
			return retval;
		}
	}
	if (in) {
		if (len >= 8) {
			for (int n = (len / 8) - 1 ; n >= 0; n--)
				data = (data << 8) + *(in+n);
		} else
			LOG_DEBUG("dr in 0x%02" PRIx8, *in);
	} else {
		LOG_ERROR("%s no drscan data", __func__);
		retval = ERROR_FAIL;
	}
	return retval;
}

static uint32_t get_tapstatus(struct target *t)
{
	uint32_t status;
	scan.out[0] = VORTEX86_STATUS;
	if (irscan(t, scan.out, NULL, VORTEX86_IRLEN) != ERROR_OK)
		return 0;
	scan.out[0] = 0;
	scan.in[0] = 0;
	if (drscan(t, scan.out, scan.in, VORTEX86_16BIN) != ERROR_OK)
		return 0;
	status = buf_get_u32(scan.in, 0, 32);
	LOG_INFO("Status =  0x%" PRIx32, status);
	return status;
}

static const struct command_registration vortex86_command_handlers[] = {
	{
		.name = "vortex86",
		.mode = COMMAND_ANY,
		.help = "vortex86 target commands",
		.chain = hello_command_handlers,
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

static int vortex86_init(struct command_context *cmd_ctx, struct target *target)
{
	return ERROR_OK;
}
static int testee_poll(struct target *target)
{
	if ((target->state == TARGET_RUNNING) || (target->state == TARGET_DEBUG_RUNNING))
		target->state = TARGET_HALTED;
	return ERROR_OK;
}
static int testee_halt(struct target *target)
{
	target->state = TARGET_HALTED;
	return ERROR_OK;
}

static void vortex86_break(struct target *target)
{
	LOG_DEBUG("Executing VORTEX low-level break");
	tap_state_t path[3] = {TAP_DRSELECT, TAP_IRSELECT, TAP_RESET};
	interface_jtag_add_pathmove(ARRAY_SIZE(path), path);
	jtag_add_tlr();
	jtag_execute_queue();
	jtag_add_runtest(1, TAP_IDLE);
	jtag_execute_queue();
}

static int vortex86_reset_assert(struct target *target)
{
	int res = ERROR_OK;

	struct vortex86_common *vortex86 = target_to_vortex86(target);
	vortex86->flush = 0;

	vortex86_break(target);

	scan.out[0] = VORTEX86_RESET;
	irscan(target, scan.out, NULL, VORTEX86_IRLEN);
	jtag_add_runtest(1, TAP_IDLE);
	jtag_execute_queue();

	vortex86_break(target);

	irscan(target, scan.out, NULL, VORTEX86_IRLEN);
	jtag_add_runtest(1, TAP_IDLE);
	vortex86->flush = 1;
	get_tapstatus(target);

	target->state = TARGET_RESET;
	target->debug_reason = DBG_REASON_NOTHALTED;

	if (target->reset_halt) {
		res = target_halt(target);
		if (res != ERROR_OK)
			return res;
	}

	return ERROR_OK;
}
static int testee_reset_deassert(struct target *target)
{
	target->state = TARGET_RUNNING;
	return ERROR_OK;
}

static int vortex86_target_create(struct target *t, Jim_Interp *interp)
{
	struct vortex86_common *vortex86 = calloc(1, sizeof(*vortex86));

	if (!vortex86)
		return ERROR_FAIL;

	vortex86_common_init_arch_info(t, vortex86);

	return ERROR_OK;
}

struct target_type vortex86_target = {
	.name = "vortex86",
	.commands = vortex86_command_handlers,

	.target_create = vortex86_target_create,
	.init_target = &vortex86_init,
	.poll = &testee_poll,
	.halt = &testee_halt,
	.assert_reset = &vortex86_reset_assert,
	.deassert_reset = &testee_reset_deassert,
};
