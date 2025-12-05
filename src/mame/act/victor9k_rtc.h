// license:BSD-3-Clause
// copyright-holders:Curt Coder
/*******************************************************************************

    Victor 9000 Real Time Clock Expansion Card

    The RTC expansion card uses a HD146818 (Hitachi version of MC146818)
    Real Time Clock Plus RAM chip with 128 bytes of battery-backed RAM.

*******************************************************************************/

#ifndef MAME_ACT_VICTOR9K_RTC_H
#define MAME_ACT_VICTOR9K_RTC_H

#pragma once

#include "machine/mc146818.h"

//******************************************************************************
//  TYPE DEFINITIONS
//******************************************************************************

// ======================> victor9k_rtc_device

class victor9k_rtc_device : public device_t
{
public:
	// construction/destruction
	victor9k_rtc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

	// memory handlers
	uint8_t read(offs_t offset);
	void write(offs_t offset, uint8_t data);

protected:
	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
	required_device<mc146818_device> m_rtc;
};

// device type definition
DECLARE_DEVICE_TYPE(VICTOR9K_RTC, victor9k_rtc_device)

#endif // MAME_ACT_VICTOR9K_RTC_H
