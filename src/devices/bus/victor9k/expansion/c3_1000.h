// license:BSD-3-Clause
// copyright-holders:Barry Rodewald, Paul Devine
/*
 * Victor 9000 RAM & RTC board, 
 * mimicing Valid Technologies C3-1000 RTC and Memory Expansion
 * described here:
 * https://jeromevernet.pagesperso-orange.fr/www.actsirius1.co.uk/pages/clock3.htm
 *
 * code adapted from:
 *  symbfac2.h
 *  Created on: 2/08/2014
 */

#ifndef MAME_BUS_VICTOR9K_C3_1000_H
#define MAME_BUS_VICTOR9K_C3_1000_H

#pragma once

#include "cpcexp.h"
#include "machine/ds128x.h"
#include "machine/ram.h"

class victor9k_c3_1000_device  : public device_t,
								 public device_victor9k_expansion_card_interface
{
public:
	// construction/destruction
	victor9k_c3_1000_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
	virtual ioport_constructor device_input_ports() const override;

private:
	victor9k_expansion_slot_device *m_slot;
	required_device<ds12885_device> m_rtc;
	required_device<nvram_device> ram;

	std::vector<uint8_t> m_rom_space;
};

// device type definition
DECLARE_DEVICE_TYPE(VICTOR9K_C3_1000, victor9k_c3_1000_device)


#endif // MAME_BUS_VICTOR9K_C3_1000_H
