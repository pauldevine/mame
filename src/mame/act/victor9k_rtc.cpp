// license:BSD-3-Clause
// copyright-holders:Curt Coder
/*******************************************************************************

    Victor 9000 Real Time Clock Expansion Card

    The RTC expansion card uses a Dallas DS1215/DS1315 "Phantom Time Chip"
    which is a battery-backed RTC that requires no address lines.

    The DS1215/DS1315 is accessed via pattern recognition. The chip monitors
    reads from a specific memory range (typically ROM space). When the correct
    64-bit pattern is recognized, the chip becomes active and provides time
    data for the next 64 read cycles.

    Pattern: 5Ca3_3ca5 (LSB first, reading bit 0 of each address)

    Historical Victor 9000 RTC cards used this approach, and modern
    reproductions (like the Victor9000-RAM project) continue to use
    the DS1315 variant.

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
	// Allocate a small ROM space for the DS1215 to monitor
	// The DS1215 needs to see ROM reads to detect its activation pattern
	m_rom = std::make_unique<uint8_t[]>(8);

	// Fill with 0xFF (unprogrammed ROM pattern)
	std::fill_n(m_rom.get(), 8, 0xff);
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
	DS1215(config, m_rtc, 32.768_kHz_XTAL);
}

//-------------------------------------------------
//  read - memory read
//-------------------------------------------------

uint8_t victor9k_rtc_device::read(offs_t offset)
{
	// The DS1215 monitors reads and provides data when activated
	// We pass through the ROM data with the RTC bit overlaid on bit 0
	uint8_t data = m_rom[offset & 0x07];

	// Read from DS1215 - this returns the time bit when active
	uint8_t rtc_data = m_rtc->read();

	// Combine ROM data with RTC bit 0
	// The DS1215 only uses bit 0 for data I/O
	data = (data & 0xfe) | (rtc_data & 0x01);

	return data;
}

//-------------------------------------------------
//  write - memory write
//-------------------------------------------------

void victor9k_rtc_device::write(offs_t offset, uint8_t data)
{
	// The DS1215 monitors writes for the activation pattern
	// Bit 0 of the written data is used for pattern matching
	m_rtc->write(data);
}
