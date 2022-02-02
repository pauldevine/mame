// license:BSD-3-Clause
// copyright-holders:Barry Rodewald, Paul Devine
/*
 *  * Victor 9000 RAM & RTC board, 
 * mimicing Valid Technologies C3-1000 RTC and Memory Expansion
 * described here:
 * https://jeromevernet.pagesperso-orange.fr/www.actsirius1.co.uk/pages/clock3.htm
 *    - RTC (Dallas DS1287A)
 *    - 512kB RAM expansion
 * 
 * code adapted from:
 * symbfac2.cpp
 *  Created on: 2/08/2014
 */

#include "emu.h"
#include "c3_1000.h"


DEFINE_DEVICE_TYPE(VICTOR9K_C3_1000, victor9k_c3_1000_device, "victor9k_c3_1000", "Victor 9000 Valid Technologies C3-1000 RAM & RTC board")

//**************************************************************************
//  DEVICE CONFIG INTERFACE
//**************************************************************************

// device machine config
void cpc_symbiface2_device::device_add_mconfig(machine_config &config)
{
	DS12885(config, m_rtc, XTAL(32'768));
	NVRAM(config, m_nvram, nvram_device::DEFAULT_ALL_1);
	// no pass-through
}

ioport_constructor cpc_symbiface2_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( cpc_symbiface2 );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

cpc_symbiface2_device::cpc_symbiface2_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, CPC_SYMBIFACE2, tag, owner, clock),
	device_victor9k_expansion_card_interface(mconfig, *this),
	m_slot(nullptr),
	m_rtc(*this,"rtc"),
	m_nvram(*this,"nvram"),
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cpc_symbiface2_device::device_start()
{
	m_slot = dynamic_cast<cpc_expansion_slot_device *>(owner());
	address_space &space = m_slot->cpu().space(AS_IO);
	space.install_readwrite_handler(0xfd14,0xfd15, read8sm_delegate(*this, FUNC(cpc_symbiface2_device::rtc_r)), write8sm_delegate(*this, FUNC(cpc_symbiface2_device::rtc_w)));
	m_nvram->set_base(&m_rom_space[0],m_rom_space.size());
	save_item(NAME(m_rom_space));

}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cpc_symbiface2_device::device_reset()
{

}

// IDE controller (custom)
// #FD00-07 - CS1
// #FD08-0F - CS0
uint8_t cpc_symbiface2_device::ide_cs0_r(offs_t offset)
{
	// data is returned in words, so it must be buffered
	if(offset == 0x00) // data register
	{
		if(m_iohigh)
		{
			m_iohigh = false;
			return m_ide_data >> 8;
		}
		else
		{
			m_iohigh = true;
			m_ide_data = m_ide->cs0_r(offset);
			return m_ide_data & 0xff;
		}
	}
	else
		return m_ide->cs0_r(offset);
}

void cpc_symbiface2_device::ide_cs0_w(offs_t offset, uint8_t data)
{
	m_ide->cs0_w(offset, data);
}

uint8_t cpc_symbiface2_device::ide_cs1_r(offs_t offset)
{
	return m_ide->cs1_r(offset);
}

void cpc_symbiface2_device::ide_cs1_w(offs_t offset, uint8_t data)
{
	m_ide->cs1_w(offset, data);
}

// RTC (Dallas DS1287A)
// #FD15 (write only) register select
// #FD14 (read/write) read from or write into selected register
uint8_t cpc_symbiface2_device::rtc_r(offs_t offset)
{
	return m_rtc->read(~offset & 0x01);
}

void cpc_symbiface2_device::rtc_w(offs_t offset, uint8_t data)
{
	m_rtc->write(~offset & 0x01, data);
}

// PS/2 Mouse connector
// #FD10 (read only) read mouse status
/*
    Status byte
    Bit 76543210
    Use mmDDDDDD

    m: Mode
    D: Use-Data

    If read and...

    m = 00 -> no more data available, you can stop reading the status for a while
    m = 01 -> D = X offset (signed); you will receive positive values, if the user
                                     is moving the mouse to the right
    m = 10 -> D = Y offset (signed); you will receive positive values, if the user
                                     is moving the mouse upwards
    m = 11 -> D[bit5] = 0 -> D[bit0]   = left button
                             D[bit1]   = right button
                             D[bit2]   = middle button
                             D[bit3]   = forward button
                             D[bit4]   = backward button
              D[bit5] = 1 -> D[bit0-4] = scroll wheel offset (signed)
 */
uint8_t cpc_symbiface2_device::mouse_r()
{
	uint8_t ret = 0;
	int input;
	int input_diff;

	switch(m_mouse_state)
	{
	case PS2_MOUSE_IDLE:
		m_mouse_state = PS2_MOUSE_IDLE;
		ret = 0;
		break;
	case PS2_MOUSE_X:
		input = m_mouse_x->read() & 0x3f;
		input_diff = m_input_x - input;
		ret = 0x40 | (input_diff & 0x3f);
		m_input_x = input;
		m_mouse_state = PS2_MOUSE_Y;
		break;
	case PS2_MOUSE_Y:
		input = m_mouse_y->read() & 0x3f;
		input_diff = m_input_y - input;
		ret = 0x80 | (input_diff & 0x3f);
		m_input_y = input;
		m_mouse_state = PS2_MOUSE_BUTTONS;
		break;
	case PS2_MOUSE_BUTTONS:
		ret = 0xc0 | (m_mouse_buttons->read() & 0x1f);
		m_mouse_state = PS2_MOUSE_IDLE;
		break;
	case PS2_MOUSE_SCROLL:
		m_mouse_state = PS2_MOUSE_IDLE;
		break;  // TODO
	}
	//popmessage("Mouse: X: %02x  Y: %02x\n",m_input_x,m_input_y);
	return ret;
}

INPUT_CHANGED_MEMBER(cpc_symbiface2_device::mouse_change_x)
{
	m_mouse_state = PS2_MOUSE_X;
}

INPUT_CHANGED_MEMBER(cpc_symbiface2_device::mouse_change_y)
{
	m_mouse_state = PS2_MOUSE_Y;
}

INPUT_CHANGED_MEMBER(cpc_symbiface2_device::mouse_change_buttons)
{
	m_mouse_state = PS2_MOUSE_BUTTONS;
}

// #FD17 (read) - map currently selected ROM to 0x4000 for read/write
uint8_t cpc_symbiface2_device::rom_rewrite_r()
{
	uint8_t bank = get_rom_bank();

	if(bank >= 32 || machine().side_effects_disabled())
		return 0xff;

	m_4xxx_ptr_r = (uint8_t*)machine().root_device().membank("bank3")->base();
	m_4xxx_ptr_w = (uint8_t*)machine().root_device().membank("bank11")->base();
	m_6xxx_ptr_r = (uint8_t*)machine().root_device().membank("bank4")->base();
	m_6xxx_ptr_w = (uint8_t*)machine().root_device().membank("bank12")->base();
	machine().root_device().membank("bank3")->set_base(&m_rom_space[bank*16384]);
	machine().root_device().membank("bank4")->set_base(&m_rom_space[bank*16384+8192]);
	machine().root_device().membank("bank11")->set_base(&m_rom_space[bank*16384]);
	machine().root_device().membank("bank12")->set_base(&m_rom_space[bank*16384+8192]);

	return 0xff;
}

// #FD17 (write) - unmap selected ROM at 0x4000
void cpc_symbiface2_device::rom_rewrite_w(uint8_t data)
{
	machine().root_device().membank("bank3")->set_base(m_4xxx_ptr_r);
	machine().root_device().membank("bank4")->set_base(m_6xxx_ptr_r);
	machine().root_device().membank("bank11")->set_base(m_4xxx_ptr_w);
	machine().root_device().membank("bank12")->set_base(m_4xxx_ptr_w);
}
