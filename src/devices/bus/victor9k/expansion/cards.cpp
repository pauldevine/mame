// license:GPL-2.0+
// copyright-holders:Dirk Best, Paul Devine
/***************************************************************************

    Victor 9000 / Sirius 1 Expansion Slot Devices

***************************************************************************/

#include "emu.h"
#include "cards.h"
#include "winchester.h"

void victor9k_expansion_cards(device_slot_interface &device)
{
	//device.option_add("rtc", VICTOR9K_RTC);
	device.option_add("winchester", VICTOR9K_WINCHESTER);
}
