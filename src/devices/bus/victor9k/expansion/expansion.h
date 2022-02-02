// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, Paul Devine
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

#ifndef MAME_BUS_VICTOR9K_VICTOR9K_H
#define MAME_BUS_VICTOR9K_VICTOR9K_H

#pragma once

#include <forward_list>


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class victor9k_device;

class victor9k_slot_device : public device_t, public device_slot_interface
{
public:
				// construction/destruction
				template <typename T, typename U>
				victor9k_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock, T &&victor9k_tag, U &&opts, const char *dflt, bool fixed)
					: victor9k_slot_device(mconfig, tag, owner, clock)
				{
					option_reset();
					opts(*this);
					set_default_option(dflt);
					set_fixed(fixed);
					m_victor9k_bus.set_tag(std::forward<T>(victor9k_tag));
				}
				victor9k_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
				victor9k_slot_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

				// device-level overrides
				virtual void device_start() override;

				// configuration
				required_device<device_t> m_victor9k_bus;
};

// device type definition
DECLARE_DEVICE_TYPE(VICTOR9K_SLOT, victor9k_slot_device)


class device_victor9k_card_interface;
// ======================> victor9k_device
class victor9k_device : public device_t,
					              public device_memory_interface
{
public:
				enum
				{
					AS_VICTOR9K_MEM    = 0,
					AS_VICTOR9K_IO     = 1,
					AS_VICTOR9K_MEMALT = 2,
					AS_VICTOR9K_IOALT  = 3
				};

				// construction/destruction
				victor9k_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

				// inline configuration
				template <typename T> void set_memspace(T &&tag, int spacenum) { m_memspace.set_tag(std::forward<T>(tag), spacenum); }
				template <typename T> void set_iospace(T &&tag, int spacenum) { m_iospace.set_tag(std::forward<T>(tag), spacenum); }
				auto iochrdy_callback() { return m_write_iochrdy.bind(); }
				auto iochck_callback() { return m_write_iochck.bind(); }
				auto irq2_callback() { return m_out_irq2_cb.bind(); }
				auto irq3_callback() { return m_out_irq3_cb.bind(); }
				auto irq4_callback() { return m_out_irq4_cb.bind(); }
				auto irq5_callback() { return m_out_irq5_cb.bind(); }
				auto irq6_callback() { return m_out_irq6_cb.bind(); }
				auto irq7_callback() { return m_out_irq7_cb.bind(); }
				auto drq1_callback() { return m_out_drq1_cb.bind(); }
				auto drq2_callback() { return m_out_drq2_cb.bind(); }
				auto drq3_callback() { return m_out_drq3_cb.bind(); }

				// include this in a driver to have VICTOR9K allocate its own address spaces (e.g. non-x86)
				void set_custom_spaces() { m_allocspaces = true; }

				// for VICTOR9K, only has 8-bit configs
				virtual space_config_vector memory_space_config() const override;

				template<typename R, typename W> void install_device(offs_t start, offs_t end, R rhandler, W whandler)
				{
					install_space(AS_VICTOR9K_IO, start, end, rhandler, whandler);
				}
				template<typename T> void install_device(offs_t addrstart, offs_t addrend, T &device, void (T::*map)(class address_map &map), uint64_t unitmask = ~u64(0))
				{
					m_iospace->install_device(addrstart, addrend, device, map, unitmask);
				}
				void install_bank(offs_t start, offs_t end, uint8_t *data);
				void install_bank(offs_t start, offs_t end, memory_bank *bank);
				void install_rom(device_t *dev, offs_t start, offs_t end, const char *region);
				template<typename R, typename W> void install_memory(offs_t start, offs_t end, R rhandler, W whandler)
				{
					install_space(AS_VICTOR9K_MEM, start, end, rhandler, whandler);
				}

				void unmap_device(offs_t start, offs_t end) const { m_iospace->unmap_readwrite(start, end); }
				void unmap_bank(offs_t start, offs_t end);
				void unmap_rom(offs_t start, offs_t end);
				void unmap_readwrite(offs_t start, offs_t end);
				bool is_option_rom_space_available(offs_t start, int size);

				// FIXME: shouldn't need to expose this
				address_space &memspace() const { return *m_memspace; }

				DECLARE_WRITE_LINE_MEMBER( irq2_w );
				DECLARE_WRITE_LINE_MEMBER( irq3_w );
				DECLARE_WRITE_LINE_MEMBER( irq4_w );
				DECLARE_WRITE_LINE_MEMBER( irq5_w );
				DECLARE_WRITE_LINE_MEMBER( irq6_w );
				DECLARE_WRITE_LINE_MEMBER( irq7_w );

				DECLARE_WRITE_LINE_MEMBER( drq1_w );
				DECLARE_WRITE_LINE_MEMBER( drq2_w );
				DECLARE_WRITE_LINE_MEMBER( drq3_w );

				// 8 bit accessors for VICTOR9K-defined address spaces
				uint8_t mem_r(offs_t offset);
				void mem_w(offs_t offset, uint8_t data);
				uint8_t io_r(offs_t offset);
				void io_w(offs_t offset, uint8_t data);

				uint8_t dack_r(int line);
				void dack_w(int line, uint8_t data);
				void dack_line_w(int line, int state);
				void eop_w(int channels, int state);

				void set_ready(int state);
				void nmi();

				virtual void set_dma_channel(uint8_t channel, device_victor9k_card_interface *dev, bool do_eop);

				void add_slot(const char *tag);
				void add_slot(device_slot_interface *slot);
				virtual void remap(int space_id, offs_t start, offs_t end);

				const address_space_config m_mem_config, m_io_config, m_mem16_config, m_io16_config;

protected:
				victor9k_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

				template<typename R, typename W> void install_space(int spacenum, offs_t start, offs_t end, R rhandler, W whandler);

				// device-level overrides
				virtual void device_config_complete() override;
				virtual void device_resolve_objects() override;
				virtual void device_start() override;
				virtual void device_reset() override;

				// address spaces
				required_address_space m_memspace, m_iospace;
				int m_memwidth, m_iowidth;
				bool m_allocspaces;

				devcb_write_line    m_out_irq2_cb;
				devcb_write_line    m_out_irq3_cb;
				devcb_write_line    m_out_irq4_cb;
				devcb_write_line    m_out_irq5_cb;
				devcb_write_line    m_out_irq6_cb;
				devcb_write_line    m_out_irq7_cb;
				devcb_write_line    m_out_drq1_cb;
				devcb_write_line    m_out_drq2_cb;
				devcb_write_line    m_out_drq3_cb;

				device_victor9k_card_interface *m_dma_device[8];
				bool                        m_dma_eop[8];
				std::forward_list<device_slot_interface *> m_slot_list;

private:
				devcb_write_line m_write_iochrdy;
				devcb_write_line m_write_iochck;
};


// device type definition
DECLARE_DEVICE_TYPE(VICTOR9K8, victor9k_device)

// ======================> device_victor9k_card_interface

// class representing interface-specific live victor9k card
class device_victor9k_card_interface : public device_interface
{
				friend class victor9k_device;
				template <class ElementType> friend class simple_list;
public:
				// construction/destruction
				virtual ~device_victor9k_card_interface();

				device_victor9k_card_interface *next() const { return m_next; }

				void set_victor9k_device();
				// configuration access
				virtual uint8_t dack_r(int line);
				virtual void dack_w(int line, uint8_t data);
				virtual void dack_line_w(int line, int state);
				virtual void eop_w(int state);

				virtual void remap(int space_id, offs_t start, offs_t end) {}

				// inline configuration
				void set_victor9kbus(device_t *victor9k_device) { m_victor9k_dev = victor9k_device; }

public:
				device_victor9k_card_interface(const machine_config &mconfig, device_t &device);

				victor9k_device  *m_victor9k;
				device_t     		 *m_victor9k_dev;

private:
				device_victor9k_card_interface *m_next;
};

#endif // MAME_BUS_VICTOR9K_VICTOR9K_H
