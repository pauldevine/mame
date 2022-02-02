// license:GPL-2.0+
// copyright-holders:Dirk Best
/***************************************************************************

    ACT Apricot RAM Expansions

***************************************************************************/

#include "emu.h"
#include "ram.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(VICTOR9K_256K_RAM, victor9k_256k_ram_device, "victor9k_256k_ram", "Apricot 256K RAM Expansion Board")
DEFINE_DEVICE_TYPE(VICTOR9K_128K_RAM, victor9k_128k_ram_device, "victor9k_128k_ram", "Apricot 128K/512K RAM Expansion Board (128K)")
DEFINE_DEVICE_TYPE(VICTOR9K_512K_RAM, victor9k_512k_ram_device, "victor9k_512k_ram", "Apricot 128K/512K RAM Expansion Board (512K)")


//**************************************************************************
//  VICTOR9K 256K RAM DEVICE
//**************************************************************************

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

static INPUT_PORTS_START( victor9k_256k )
	PORT_START("sw")
	PORT_DIPNAME(0x01, 0x00, "Base Address")
	PORT_DIPSETTING(0x00, "40000H")
	PORT_DIPSETTING(0x01, "80000H")
INPUT_PORTS_END

ioport_constructor victor9k_256k_ram_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( victor9k_256k );
}

//-------------------------------------------------
//  victor9k_256k_ram_device - constructor
//-------------------------------------------------

victor9k_256k_ram_device::victor9k_256k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, VICTOR9K_256K_RAM, tag, owner, clock),
	device_victor9k_expansion_card_interface(mconfig, *this),
	m_sw(*this, "sw")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_256k_ram_device::device_start()
{
	m_ram.resize(0x40000 / 2);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void victor9k_256k_ram_device::device_reset()
{
	if (m_sw->read() == 0)
		m_bus->install_ram(0x40000, 0x7ffff, &m_ram[0]);
	else
		m_bus->install_ram(0x80000, 0xbffff, &m_ram[0]);
}


//**************************************************************************
//  VICTOR9K 128K RAM DEVICE
//**************************************************************************

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

static INPUT_PORTS_START( victor9k_128k )
	PORT_START("strap")
	PORT_DIPNAME(0x03, 0x01, "Base Address")
	PORT_DIPSETTING(0x00, "512K")
	PORT_DIPSETTING(0x01, "256K - 384K")
	PORT_DIPSETTING(0x02, "384K - 512K")
INPUT_PORTS_END

ioport_constructor victor9k_128k_ram_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( victor9k_128k );
}

//-------------------------------------------------
//  victor9k_128_512k_ram_device - constructor
//-------------------------------------------------

victor9k_128k_ram_device::victor9k_128k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, VICTOR9K_128K_RAM, tag, owner, clock),
	device_victor9k_expansion_card_interface(mconfig, *this),
	m_strap(*this, "strap")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_128k_ram_device::device_start()
{
	m_ram.resize(0x20000 / 2);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void victor9k_128k_ram_device::device_reset()
{
	if (m_strap->read() == 1)
		m_bus->install_ram(0x40000, 0x5ffff, &m_ram[0]);
	else if (m_strap->read() == 2)
		m_bus->install_ram(0x60000, 0x7ffff, &m_ram[0]);
}


//**************************************************************************
//  VICTOR9K 512K RAM DEVICE
//**************************************************************************

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

static INPUT_PORTS_START( victor9k_512k )
	PORT_START("strap")
	PORT_DIPNAME(0x03, 0x00, "Base Address")
	PORT_DIPSETTING(0x00, "512K")
	PORT_DIPSETTING(0x01, "256K - 384K")
	PORT_DIPSETTING(0x02, "384K - 512K")
INPUT_PORTS_END

ioport_constructor victor9k_512k_ram_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( victor9k_512k );
}

//-------------------------------------------------
//  victor9k_128_512k_ram_device - constructor
//-------------------------------------------------

victor9k_512k_ram_device::victor9k_512k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, VICTOR9K_512K_RAM, tag, owner, clock),
	device_victor9k_expansion_card_interface(mconfig, *this),
	m_strap(*this, "strap")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_512k_ram_device::device_start()
{
	m_ram.resize(0x80000 / 2);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void victor9k_512k_ram_device::device_reset()
{
	if (m_strap->read() == 0)
		m_bus->install_ram(0x40000, 0xbffff, &m_ram[0]);
}
