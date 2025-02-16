/*
 * \brief   CPU context of a virtual machine for TrustZone
 * \author  Stefan Kalkowski
 * \author  Martin Stein
 * \date    2013-10-30
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__SPEC__IMX53__VM_STATE_H_
#define _INCLUDE__SPEC__IMX53__VM_STATE_H_

/* Genode includes */
#include <cpu/cpu_state.h>

namespace Genode {

	/**
	 * CPU context of a virtual machine
	 */
	struct Vm_state;
	using Vm_data = Vm_state;
}


struct Genode::Vm_state : Genode::Cpu_state_modes
{
	Genode::addr_t dfar;
	Genode::addr_t ttbr[2];
	Genode::addr_t ttbrc;

	/**
	 * Fpu registers
	 */
	Genode::uint32_t fpscr;
	Genode::uint64_t d0_d31[32];

	Genode::addr_t irq_injection;
};

#endif /* _INCLUDE__SPEC__IMX53__VM_STATE_H_ */
