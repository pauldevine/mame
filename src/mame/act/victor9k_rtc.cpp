// license:BSD-3-Clause
// copyright-holders:Curt Coder
/*******************************************************************************

    Victor 9000 Real Time Clock Expansion Card

    The RTC expansion card uses a HD146818 (Hitachi version of MC146818)
    Real Time Clock Plus RAM chip with 128 bytes of battery-backed RAM.

    The MC146818/HD146818 has a standard two-register interface:
    - Offset 0: Address register (write-only)
    - Offset 1: Data register (read/write)

    The chip provides:
    - Real-time clock with alarm
    - 128 bytes of battery-backed CMOS RAM (64 bytes on some variants)
    - Periodic interrupt capability
    - Square wave output

*******************************************************************************/

#include "emu.h"
#include "victor9k_rtc.h"

//******************************************************************************
//  DEVICE DEFINITIONS
//******************************************************************************

DEFINE_DEVICE_TYPE(VICTOR9K_RTC, victor9k_rtc_device, "victor9k_rtc", "Victor 9000 RTC Card")

//******************************************************************************
//  LIVE DEVICE
//******************************************************************************

//-------------------------------------------------
//  victor9k_rtc_device - constructor
//-------------------------------------------------

victor9k_rtc_device::victor9k_rtc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, VICTOR9K_RTC, tag, owner, clock)
	, m_rtc(*this, "rtc")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_rtc_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void victor9k_rtc_device::device_reset()
{
}

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void victor9k_rtc_device::device_add_mconfig(machine_config &config)
{
	MC146818(config, m_rtc, 32.768_kHz_XTAL);
}

//-------------------------------------------------
//  read - memory read
//-------------------------------------------------

uint8_t victor9k_rtc_device::read(offs_t offset)
{
	/*
	    Memory map:
	    offset 0: Address register (write-only, but some clones allow read)
	    offset 1: Data register
	*/

	switch (offset & 1)
	{
	case 0:
		// Address register - typically write-only on real hardware
		// but some systems read it back
		return m_rtc->get_address();

	case 1:
		// Data register
		return m_rtc->data_r();
	}

	return 0xff;
}

//-------------------------------------------------
//  write - memory write
//-------------------------------------------------

void victor9k_rtc_device::write(offs_t offset, uint8_t data)
{
	/*
	    Memory map:
	    offset 0: Address register
	    offset 1: Data register
	*/

	switch (offset & 1)
	{
	case 0:
		// Address register - selects which internal register to access
		m_rtc->address_w(data);
		break;

	case 1:
		// Data register - reads/writes the selected internal register
		m_rtc->data_w(data);
		break;
	}
}
