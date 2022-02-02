// license:GPL-2.0+
// copyright-holders:Dirk Best
/***************************************************************************

    ACT Apricot RAM Expansions

***************************************************************************/

#ifndef MAME_BUS_VICTOR9K_RAM_H
#define MAME_BUS_VICTOR9K_RAM_H

#pragma once

#include "expansion.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************


// ======================> victor9k_256k_ram_device

class victor9k_256k_ram_device : public device_t, public device_victor9k_expansion_card_interface
{
public:
	// construction/destruction
	victor9k_256k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	required_ioport m_sw;

	std::vector<uint16_t> m_ram;
};


// ======================> victor9k_128k_ram_device

class victor9k_128k_ram_device : public device_t, public device_victor9k_expansion_card_interface
{
public:
	// construction/destruction
	victor9k_128k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	required_ioport m_strap;

	std::vector<uint16_t> m_ram;
};


// ======================> victor9k_512k_ram_device

class victor9k_512k_ram_device : public device_t, public device_victor9k_expansion_card_interface
{
public:
	// construction/destruction
	victor9k_512k_ram_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	required_ioport m_strap;

	std::vector<uint16_t> m_ram;
};


// device type definition
DECLARE_DEVICE_TYPE(VICTOR9K_256K_RAM, victor9k_256k_ram_device)
DECLARE_DEVICE_TYPE(VICTOR9K_128K_RAM, victor9k_128k_ram_device)
DECLARE_DEVICE_TYPE(VICTOR9K_512K_RAM, victor9k_512k_ram_device)


#endif // MAME_BUS_VICTOR9K_RAM_H
