// license:GPL-2.0+
// copyright-holders:Dirk Best, Paul Devine
/***************************************************************************

    Victor 9000 / Sirius 1 Expansion Slot

    (rear of computer)
  
    BDO — 25   26 — BD1
    BD2 — 24   27 — BD3
    BD4 — 23   28 — BD5
    BD6 — 22   29 — BD7
   XACK — 21   30 — EXTIO
PHASE 2 — 20   31 — Ground
   CSEN — 19   32 — -12 volts
   HLDA — 18   33 — DLATCH
   HOLD — 17   34 — +12 volts 
    IRQ — 16   35 — Ground 
    NMI — 15   36 — +5 volts
     WR — 14   37 — +5 volts 
  Reset — 13   38 — CLK5
   DT/R — 12   39 — Ground
     RD — 11   40 — CLK15B
   IO/M — 10   41 — Ready
    ALE —  9   42 — IR5
    DEN —  8   43 — IR4
    SSO —  7   44 — Ground
     A8 —  6   45 —A9
    A10 —  5   46 — All
    A12 —  4   47 — A13
    A14 —  3   48 — A15
    A16 —  2   49 — A17
    A18 —  1   50 — A19

    (front of computer)

***************************************************************************/

#include "emu.h"
#include "expansion.h"


//**************************************************************************
//  EXPANSION SLOT DEVICE
//**************************************************************************

DEFINE_DEVICE_TYPE(VICTOR9K_EXPANSION_SLOT, victor9k_expansion_slot_device, "victor9k_exp_slot", "Victor 9000 / Sirius 1 Expansion Slot")

//-------------------------------------------------
//  victor9k_expansion_slot_device - constructor
//-------------------------------------------------

victor9k_expansion_slot_device::victor9k_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	victor9k_expansion_slot_device(mconfig, VICTOR9K_EXPANSION_SLOT, tag, owner, clock)
{
}

victor9k_expansion_slot_device::victor9k_expansion_slot_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_single_card_slot_interface<device_victor9k_expansion_card_interface>(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_expansion_slot_device::device_start()
{
	device_victor9k_expansion_card_interface *dev = get_card_device();

	if (dev)
	{
		victor9k_expansion_bus_device *bus = downcast<victor9k_expansion_bus_device *>(m_owner);
		bus->add_card(dev);
	}
}


//**************************************************************************
//  EXPANSION BUS DEVICE
//**************************************************************************

DEFINE_DEVICE_TYPE(VICTOR9K_EXPANSION_BUS, victor9k_expansion_bus_device, "victor9k_exp_bus", "Apricot Expansion Bus")

//-------------------------------------------------
//  victor9k_expansion_bus_device - constructor
//-------------------------------------------------

victor9k_expansion_bus_device::victor9k_expansion_bus_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, VICTOR9K_EXPANSION_BUS, tag, owner, clock),
	m_program(*this, finder_base::DUMMY_TAG, -1),
	m_io(*this, finder_base::DUMMY_TAG, -1),
	m_program_iop(*this, finder_base::DUMMY_TAG, -1),
	m_io_iop(*this, finder_base::DUMMY_TAG, -1),
	m_dma1_handler(*this),
	m_dma2_handler(*this),
	m_ext1_handler(*this),
	m_ext2_handler(*this),
	m_int2_handler(*this),
	m_int3_handler(*this)
{
}

//-------------------------------------------------
//  victor9k_expansion_bus_device - destructor
//-------------------------------------------------

victor9k_expansion_bus_device::~victor9k_expansion_bus_device()
{
	m_dev.detach_all();
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_expansion_bus_device::device_start()
{
	// resolve callbacks
	m_dma1_handler.resolve_safe();
	m_dma2_handler.resolve_safe();
	m_ext1_handler.resolve_safe();
	m_ext2_handler.resolve_safe();
	m_int2_handler.resolve_safe();
	m_int3_handler.resolve_safe();
}

//-------------------------------------------------
//  add_card - add new card to our bus
//-------------------------------------------------

void victor9k_expansion_bus_device::add_card(device_victor9k_expansion_card_interface *card)
{
	card->set_bus_device(this);
	m_dev.append(*card);
}

// callbacks from slot device to the host
WRITE_LINE_MEMBER( victor9k_expansion_bus_device::dma1_w ) { m_dma1_handler(state); }
WRITE_LINE_MEMBER( victor9k_expansion_bus_device::dma2_w ) { m_dma2_handler(state); }
WRITE_LINE_MEMBER( victor9k_expansion_bus_device::ext1_w ) { m_ext1_handler(state); }
WRITE_LINE_MEMBER( victor9k_expansion_bus_device::ext2_w ) { m_ext2_handler(state); }
WRITE_LINE_MEMBER( victor9k_expansion_bus_device::int2_w ) { m_int2_handler(state); }
WRITE_LINE_MEMBER( victor9k_expansion_bus_device::int3_w ) { m_int3_handler(state); }

//-------------------------------------------------
//  install_ram - attach ram to cpu/iop
//-------------------------------------------------

void victor9k_expansion_bus_device::install_ram(offs_t addrstart, offs_t addrend, void *baseptr)
{
	m_program->install_ram(addrstart, addrend, baseptr);

	if (m_program_iop)
		m_program_iop->install_ram(addrstart, addrend, baseptr);
}


//**************************************************************************
//  CARTRIDGE INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_victor9k_expansion_card_interface - constructor
//-------------------------------------------------

device_victor9k_expansion_card_interface::device_victor9k_expansion_card_interface(const machine_config &mconfig, device_t &device) :
	device_interface(device, "victor9kexp"),
	m_bus(nullptr),
	m_next(nullptr)
{
}

//-------------------------------------------------
//  ~device_victor9k_expansion_card_interface - destructor
//-------------------------------------------------

device_victor9k_expansion_card_interface::~device_victor9k_expansion_card_interface()
{
}

void device_victor9k_expansion_card_interface::set_bus_device(victor9k_expansion_bus_device *bus)
{
	m_bus = bus;
}
