// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, Paul Devine
/***************************************************************************

        ISA bus device

***************************************************************************/

#include "emu.h"
#include "expansion.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(VICTOR9K_SLOT, victor9k_slot_device, "victor9k_slot", "8-bit Victor 9K Expansion slot")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  victor9k_slot_device - constructor
//-------------------------------------------------
victor9k_slot_device::victor9k_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	victor9k_slot_device(mconfig, VICTOR9K_SLOT, tag, owner, clock)
{
}

victor9k_slot_device::victor9k_slot_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_slot_interface(mconfig, *this),
	m_victor9k_bus(*this, finder_base::DUMMY_TAG)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_slot_device::device_start()
{
	device_victor9k_card_interface *const dev = dynamic_cast<device_victor9k_card_interface *>(get_card_device());
	const device_victor9k_card_interface *intf;
	if (get_card_device() && get_card_device()->interface(intf))
		fatalerror("ISA16 device in VICTOR9K slot\n");

	if (dev) dev->set_victor9kbus(m_victor9k_bus);

	// tell Victor9k bus that there is one slot with the specified tag
	downcast<victor9k_device &>(*m_victor9k_bus).add_slot(tag());
}


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(VICTOR9K, victor9k_device, "victor9k_bus", "8-bit Victor9k bus")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  victor9k_device - constructor
//-------------------------------------------------

victor9k_device::victor9k_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	victor9k_device(mconfig, VICTOR9K, tag, owner, clock)
{
}

victor9k_device::victor9k_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_memory_interface(mconfig, *this),
	m_mem_config("mem8", ENDIANNESS_LITTLE, 8, 24, 0, address_map_constructor()),
	m_io_config("io8", ENDIANNESS_LITTLE, 8, 16, 0, address_map_constructor()),
	m_memspace(*this, finder_base::DUMMY_TAG, -1),
	m_iospace(*this, finder_base::DUMMY_TAG, -1),
	m_memwidth(0),
	m_iowidth(0),
	m_allocspaces(false),
	m_out_irq2_cb(*this),
	m_out_irq3_cb(*this),
	m_out_irq4_cb(*this),
	m_out_irq5_cb(*this),
	m_out_irq6_cb(*this),
	m_out_irq7_cb(*this),
	m_out_drq1_cb(*this),
	m_out_drq2_cb(*this),
	m_out_drq3_cb(*this),
	m_write_iochrdy(*this),
	m_write_iochck(*this)
{
	std::fill(std::begin(m_dma_device), std::end(m_dma_device), nullptr);
	std::fill(std::begin(m_dma_eop), std::end(m_dma_eop), false);
}

device_memory_interface::space_config_vector victor9k_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(AS_ISA_MEM,    &m_mem_config),
		std::make_pair(AS_ISA_IO,     &m_io_config),
		std::make_pair(AS_ISA_MEMALT, &m_mem16_config),
		std::make_pair(AS_ISA_IOALT,  &m_io16_config)
	};
}

device_memory_interface::space_config_vector victor9k_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(AS_ISA_MEM,    &m_mem16_config),
		std::make_pair(AS_ISA_IO,     &m_io16_config),
		std::make_pair(AS_ISA_MEMALT, &m_mem_config),
		std::make_pair(AS_ISA_IOALT,  &m_io_config)
	};
}

uint8_t victor9k_device::mem_r(offs_t offset)
{
	return m_memspace->read_byte(offset);
}

void victor9k_device::mem_w(offs_t offset, uint8_t data)
{
	m_memspace->write_byte(offset, data);
}

uint8_t victor9k_device::io_r(offs_t offset)
{
	return m_iospace->read_byte(offset);
}

void victor9k_device::io_w(offs_t offset, uint8_t data)
{
	m_iospace->write_byte(offset, data);
}

void victor9k_device::set_dma_channel(uint8_t channel, device_victor9k_card_interface *dev, bool do_eop)
{
	m_dma_device[channel] = dev;
	m_dma_eop[channel] = do_eop;
}

void victor9k_device::add_slot(const char *tag)
{
	device_t *dev = subdevice(tag);
	//printf(tag);
	add_slot(dynamic_cast<device_slot_interface *>(dev));
}

void victor9k_device::add_slot(device_slot_interface *slot)
{
	m_slot_list.push_front(slot);
}

void victor9k_device::remap(int space_id, offs_t start, offs_t end)
{
	for (device_slot_interface *sl : m_slot_list)
	{
		device_t *dev = sl->get_card_device();
		device_victor9k_card_interface *victor9kdev = dynamic_cast<device_victor9k_card_interface *>(dev);
		victor9kbusdev->remap(space_id, start, end);
	}
}

//-------------------------------------------------
//  device_config_complete - - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void victor9k_device::device_config_complete()
{
	if (m_allocspaces)
	{
		m_memspace.set_tag(*this, DEVICE_SELF, AS_ISA_MEM);
		m_iospace.set_tag(*this, DEVICE_SELF, AS_ISA_IO);
	}
}

//-------------------------------------------------
//  device_resolve_objects - resolve objects that
//  may be needed for other devices to set
//  initial conditions at start time
//-------------------------------------------------

void victor9k_device::device_resolve_objects()
{
	// resolve callbacks
	m_write_iochrdy.resolve_safe();
	m_write_iochck.resolve_safe();

	m_out_irq2_cb.resolve_safe();
	m_out_irq3_cb.resolve_safe();
	m_out_irq4_cb.resolve_safe();
	m_out_irq5_cb.resolve_safe();
	m_out_irq6_cb.resolve_safe();
	m_out_irq7_cb.resolve_safe();
	m_out_drq1_cb.resolve_safe();
	m_out_drq2_cb.resolve_safe();
	m_out_drq3_cb.resolve_safe();

	m_iowidth = m_iospace->data_width();
	m_memwidth = m_memspace->data_width();
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void victor9k_device::device_reset()
{
}


template <typename R, typename W> void victor9k_device::install_space(int spacenum, offs_t start, offs_t end, R rhandler, W whandler)
{
	int buswidth;
	address_space *space;

	if (spacenum == AS_VICTOR9K_IO)
	{
		space = m_iospace.target();
		buswidth = m_iowidth;
	}
	else if (spacenum == AS_VICTOR9K_MEM)
	{
		space = m_memspace.target();
		buswidth = m_memwidth;
	}
	else
	{
		fatalerror("Unknown space passed to victor9k_device::install_space!\n");
	}

	switch (buswidth)
	{
		case 8:
			space->install_read_handler(start, end, rhandler, 0);
			space->install_write_handler(start, end, whandler, 0);
			break;
		default:
			fatalerror("VICTOR9K: Bus width %d not supported\n", buswidth);
	}
}

template void victor9k_device::install_space<read8_delegate,    write8_delegate   >(int spacenum, offs_t start, offs_t end, read8_delegate rhandler,    write8_delegate whandler);
template void victor9k_device::install_space<read8_delegate,    write8m_delegate  >(int spacenum, offs_t start, offs_t end, read8_delegate rhandler,    write8m_delegate whandler);
template void victor9k_device::install_space<read8_delegate,    write8s_delegate  >(int spacenum, offs_t start, offs_t end, read8_delegate rhandler,    write8s_delegate whandler);
template void victor9k_device::install_space<read8_delegate,    write8sm_delegate >(int spacenum, offs_t start, offs_t end, read8_delegate rhandler,    write8sm_delegate whandler);
template void victor9k_device::install_space<read8_delegate,    write8mo_delegate >(int spacenum, offs_t start, offs_t end, read8_delegate rhandler,    write8mo_delegate whandler);
template void victor9k_device::install_space<read8_delegate,    write8smo_delegate>(int spacenum, offs_t start, offs_t end, read8_delegate rhandler,    write8smo_delegate whandler);

template void victor9k_device::install_space<read8m_delegate,   write8_delegate   >(int spacenum, offs_t start, offs_t end, read8m_delegate rhandler,   write8_delegate whandler);
template void victor9k_device::install_space<read8m_delegate,   write8m_delegate  >(int spacenum, offs_t start, offs_t end, read8m_delegate rhandler,   write8m_delegate whandler);
template void victor9k_device::install_space<read8m_delegate,   write8s_delegate  >(int spacenum, offs_t start, offs_t end, read8m_delegate rhandler,   write8s_delegate whandler);
template void victor9k_device::install_space<read8m_delegate,   write8sm_delegate >(int spacenum, offs_t start, offs_t end, read8m_delegate rhandler,   write8sm_delegate whandler);
template void victor9k_device::install_space<read8m_delegate,   write8mo_delegate >(int spacenum, offs_t start, offs_t end, read8m_delegate rhandler,   write8mo_delegate whandler);
template void victor9k_device::install_space<read8m_delegate,   write8smo_delegate>(int spacenum, offs_t start, offs_t end, read8m_delegate rhandler,   write8smo_delegate whandler);

template void victor9k_device::install_space<read8s_delegate,   write8_delegate   >(int spacenum, offs_t start, offs_t end, read8s_delegate rhandler,   write8_delegate whandler);
template void victor9k_device::install_space<read8s_delegate,   write8m_delegate  >(int spacenum, offs_t start, offs_t end, read8s_delegate rhandler,   write8m_delegate whandler);
template void victor9k_device::install_space<read8s_delegate,   write8s_delegate  >(int spacenum, offs_t start, offs_t end, read8s_delegate rhandler,   write8s_delegate whandler);
template void victor9k_device::install_space<read8s_delegate,   write8sm_delegate >(int spacenum, offs_t start, offs_t end, read8s_delegate rhandler,   write8sm_delegate whandler);
template void victor9k_device::install_space<read8s_delegate,   write8mo_delegate >(int spacenum, offs_t start, offs_t end, read8s_delegate rhandler,   write8mo_delegate whandler);
template void victor9k_device::install_space<read8s_delegate,   write8smo_delegate>(int spacenum, offs_t start, offs_t end, read8s_delegate rhandler,   write8smo_delegate whandler);

template void victor9k_device::install_space<read8sm_delegate,  write8_delegate   >(int spacenum, offs_t start, offs_t end, read8sm_delegate rhandler,  write8_delegate whandler);
template void victor9k_device::install_space<read8sm_delegate,  write8m_delegate  >(int spacenum, offs_t start, offs_t end, read8sm_delegate rhandler,  write8m_delegate whandler);
template void victor9k_device::install_space<read8sm_delegate,  write8s_delegate  >(int spacenum, offs_t start, offs_t end, read8sm_delegate rhandler,  write8s_delegate whandler);
template void victor9k_device::install_space<read8sm_delegate,  write8sm_delegate >(int spacenum, offs_t start, offs_t end, read8sm_delegate rhandler,  write8sm_delegate whandler);
template void victor9k_device::install_space<read8sm_delegate,  write8mo_delegate >(int spacenum, offs_t start, offs_t end, read8sm_delegate rhandler,  write8mo_delegate whandler);
template void victor9k_device::install_space<read8sm_delegate,  write8smo_delegate>(int spacenum, offs_t start, offs_t end, read8sm_delegate rhandler,  write8smo_delegate whandler);

template void victor9k_device::install_space<read8mo_delegate,  write8_delegate   >(int spacenum, offs_t start, offs_t end, read8mo_delegate rhandler,  write8_delegate whandler);
template void victor9k_device::install_space<read8mo_delegate,  write8m_delegate  >(int spacenum, offs_t start, offs_t end, read8mo_delegate rhandler,  write8m_delegate whandler);
template void victor9k_device::install_space<read8mo_delegate,  write8s_delegate  >(int spacenum, offs_t start, offs_t end, read8mo_delegate rhandler,  write8s_delegate whandler);
template void victor9k_device::install_space<read8mo_delegate,  write8sm_delegate >(int spacenum, offs_t start, offs_t end, read8mo_delegate rhandler,  write8sm_delegate whandler);
template void victor9k_device::install_space<read8mo_delegate,  write8mo_delegate >(int spacenum, offs_t start, offs_t end, read8mo_delegate rhandler,  write8mo_delegate whandler);
template void victor9k_device::install_space<read8mo_delegate,  write8smo_delegate>(int spacenum, offs_t start, offs_t end, read8mo_delegate rhandler,  write8smo_delegate whandler);

template void victor9k_device::install_space<read8smo_delegate, write8_delegate   >(int spacenum, offs_t start, offs_t end, read8smo_delegate rhandler, write8_delegate whandler);
template void victor9k_device::install_space<read8smo_delegate, write8m_delegate  >(int spacenum, offs_t start, offs_t end, read8smo_delegate rhandler, write8m_delegate whandler);
template void victor9k_device::install_space<read8smo_delegate, write8s_delegate  >(int spacenum, offs_t start, offs_t end, read8smo_delegate rhandler, write8s_delegate whandler);
template void victor9k_device::install_space<read8smo_delegate, write8sm_delegate >(int spacenum, offs_t start, offs_t end, read8smo_delegate rhandler, write8sm_delegate whandler);
template void victor9k_device::install_space<read8smo_delegate, write8mo_delegate >(int spacenum, offs_t start, offs_t end, read8smo_delegate rhandler, write8mo_delegate whandler);
template void victor9k_device::install_space<read8smo_delegate, write8smo_delegate>(int spacenum, offs_t start, offs_t end, read8smo_delegate rhandler, write8smo_delegate whandler);

void victor9k_device::install_bank(offs_t start, offs_t end, uint8_t *data)
{
	m_memspace->install_ram(start, end, data);
}

void victor9k_device::install_bank(offs_t start, offs_t end, memory_bank *bank)
{
	m_memspace->install_readwrite_bank(start, end, bank);
}

void victor9k_device::unmap_bank(offs_t start, offs_t end)
{
	m_memspace->unmap_readwrite(start, end);
}

void victor9k_device::install_rom(device_t *dev, offs_t start, offs_t end, const char *region)
{
	if (machine().root_device().memregion("victor9kbus")) {
		uint8_t *src = dev->memregion(region)->base();
		uint8_t *dest = machine().root_device().memregion("victor9kbus")->base() + start - 0xc0000;
		memcpy(dest,src, end - start + 1);
	} else {
		m_memspace->install_rom(start, end, machine().root_device().memregion(dev->subtag(region).c_str())->base());
		m_memspace->unmap_write(start, end);
	}
}

void victor9k_device::unmap_rom(offs_t start, offs_t end)
{
	m_memspace->unmap_read(start, end);
}

bool victor9k_device::is_option_rom_space_available(offs_t start, int size)
{
	for(int i = 0; i < size; i += 4096) // 4KB granularity should be enough
		if(m_memspace->get_read_ptr(start + i)) return false;
	return true;
}

void victor9k_device::unmap_readwrite(offs_t start, offs_t end)
{
	m_memspace->unmap_readwrite(start, end);
}

// interrupt request from victor9k card
WRITE_LINE_MEMBER( victor9k_device::irq2_w ) { m_out_irq2_cb(state); }
WRITE_LINE_MEMBER( victor9k_device::irq3_w ) { m_out_irq3_cb(state); }
WRITE_LINE_MEMBER( victor9k_device::irq4_w ) { m_out_irq4_cb(state); }
WRITE_LINE_MEMBER( victor9k_device::irq5_w ) { m_out_irq5_cb(state); }
WRITE_LINE_MEMBER( victor9k_device::irq6_w ) { m_out_irq6_cb(state); }
WRITE_LINE_MEMBER( victor9k_device::irq7_w ) { m_out_irq7_cb(state); }

// dma request from victor9k card
WRITE_LINE_MEMBER( victor9k_device::drq1_w ) { m_out_drq1_cb(state); }
WRITE_LINE_MEMBER( victor9k_device::drq2_w ) { m_out_drq2_cb(state); }
WRITE_LINE_MEMBER( victor9k_device::drq3_w ) { m_out_drq3_cb(state); }

uint8_t victor9k_device::dack_r(int line)
{
	if (m_dma_device[line])
		return m_dma_device[line]->dack_r(line);
	return 0xff;
}

void victor9k_device::dack_w(int line, uint8_t data)
{
	if (m_dma_device[line])
		return m_dma_device[line]->dack_w(line,data);
}

void victor9k_device::dack_line_w(int line, int state)
{
	if (m_dma_device[line])
		m_dma_device[line]->dack_line_w(line, state);
}

void victor9k_device::eop_w(int channel, int state)
{
	if (m_dma_eop[channel] && m_dma_device[channel])
		m_dma_device[channel]->eop_w(state);
}

void victor9k_device::set_ready(int state)
{
	m_write_iochrdy(state);
}

void victor9k_device::nmi()
{
	// active low pulse
	m_write_iochck(0);
	m_write_iochck(1);
}

//**************************************************************************
//  DEVICE CONFIG VICTOR9K CARD INTERFACE
//**************************************************************************


//**************************************************************************
//  DEVICE VICTOR9K CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_victor9k_card_interface - constructor
//-------------------------------------------------

device_victor9k_card_interface::device_victor9k_card_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device, "victor9k_bus"),
		m_victor9k_bus(nullptr), m_victor9k_dev(nullptr), m_next(nullptr)
{
}


//-------------------------------------------------
//  ~device_victor9k_card_interface - destructor
//-------------------------------------------------

device_victor9k_card_interface::~device_victor9k_card_interface()
{
}

uint8_t device_victor9k_card_interface::dack_r(int line)
{
	return 0;
}

void device_victor9k_card_interface::dack_w(int line, uint8_t data)
{
}

void device_victor9k_card_interface::dack_line_w(int line, int state)
{
}

void device_victor9k_card_interface::eop_w(int state)
{
}

void device_victor9k_card_interface::set_victor9k_bus_device()
{
	m_victor9k_bus = dynamic_cast<victor9k_device *>(m_victor9k_dev);
}


//-------------------------------------------------
//  device_victor9k_card_interface - constructor
//-------------------------------------------------

device_victor9k_card_interface::device_victor9k_card_interface(const machine_config &mconfig, device_t &device)
	: device_victor9k_card_interface(mconfig,device), m_victor9k_bus(nullptr)
{
}


//-------------------------------------------------
//  ~device_victor9k_card_interface - destructor
//-------------------------------------------------

device_victor9k_card_interface::~device_victor9k_card_interface()
{
}

void device_victor9k_card_interface::set_victor9k_bus_device()
{
	m_victor9k_bus = dynamic_cast<victor9k_device *>(m_victor9k_dev);
}



