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

static int testee_init(struct command_context *cmd_ctx, struct target *target)
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
static int testee_reset_assert(struct target *target)
{
	target->state = TARGET_RESET;
	return ERROR_OK;
}
static int testee_reset_deassert(struct target *target)
{
	target->state = TARGET_RUNNING;
	return ERROR_OK;
}
struct target_type vortex86_target = {
	.name = "vortex86",
	.commands = vortex86_command_handlers,

	.init_target = &testee_init,
	.poll = &testee_poll,
	.halt = &testee_halt,
	.assert_reset = &testee_reset_assert,
	.deassert_reset = &testee_reset_deassert,
};
