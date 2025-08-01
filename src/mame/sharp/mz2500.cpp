// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/**************************************************************************************************

Sharp MZ-2500 (c) 1985 Sharp Corporation

Computer box contains 2 floppy drives on the right, and a front-loading cassette player on the left.
Keyboard is separate, and the key layout is very similar to the fm7.

TODO:
- Kanji text is cutted in half when font_size is 1 / interlace is disabled, different ROM used?
  (check Back to the Future);
- Add remaining missing peripherals, SIO, HDD and w1300a network;
- reverse / blanking tvram attributes;
- Implement backward compatibility with MZ-2000/MZ-80B;
- Implement expansion box unit;
- Remaining SIO ports (DA-25 and DE-9);
- mouse: missing select line from OPN Port A;
- dustbx**: cannot detect mouse reliably from bootstrap, works in main menu regardless;

**************************************************************************************************/

#include "emu.h"
#include "mz2500.h"

#include "softlist_dev.h"


/* machine stuff */

#define WRAM_RESET 0
#define IPL_RESET 1

static constexpr u8 bank_reset_val[2][8] =
{
	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 },
	{ 0x34, 0x35, 0x36, 0x37, 0x04, 0x05, 0x06, 0x07 }
};

/* video stuff */

void mz2500_state::video_start()
{
	std::fill(std::begin(m_cg_reg), std::end(m_cg_reg), 0);

}

/*
[0] xxxx xxxx tile
[1] ---- -xxx color offset
[1] ---- x--- PCG combine mode
[1] --xx ---- PCG select
[1] xx-- ---- reverse / blink attributes
[2] --xx xxxx tile bank (for kanji ROMs)
[2] xx-- ---- kanji select
*/

/* helper function, to draw stuff without getting crazy with height / width conditions :) */
void mz2500_state::draw_pixel(bitmap_ind16 &bitmap,int x,int y,uint16_t  pen,u8 width,u8 height)
{
	if(width && height)
	{
		bitmap.pix(y*2+0, x*2+0) = m_palette->pen(pen);
		bitmap.pix(y*2+0, x*2+1) = m_palette->pen(pen);
		bitmap.pix(y*2+1, x*2+0) = m_palette->pen(pen);
		bitmap.pix(y*2+1, x*2+1) = m_palette->pen(pen);
	}
	else if(width)
	{
		bitmap.pix(y, x*2+0) = m_palette->pen(pen);
		bitmap.pix(y, x*2+1) = m_palette->pen(pen);
	}
	else if(height)
	{
		bitmap.pix(y*2+0, x) = m_palette->pen(pen);
		bitmap.pix(y*2+1, x) = m_palette->pen(pen);
	}
	else
		bitmap.pix(y, x) = m_palette->pen(pen);
}

void mz2500_state::draw_80x25(bitmap_ind16 &bitmap,const rectangle &cliprect,uint16_t map_addr)
{
	u8 *vram = m_tvram;
	int x,y,count,xi,yi;
	u8 *gfx_data;
	u8 y_step;
	u8 s_y;
	u8 y_height;

	count = (map_addr & 0x7ff);

	y_step = (m_text_font_reg) ? 1 : 2;
	y_height = (m_text_reg[0] & 0x10) ? 10 : 8;
	s_y = m_text_reg[9] & 0xf;

	for (y=0;y<26*y_step;y+=y_step)
	{
		for (x=0;x<80;x++)
		{
			int tile = vram[count+0x0000] & 0xff;
			int attr = vram[count+0x0800];
			int tile_bank = vram[count+0x1000] & 0x3f;
			int gfx_sel = (attr & 0x38) | (vram[count+0x1000] & 0xc0);
			int color = attr & 7;
			int inv_col = (attr & 0x40) >> 6;

			if(gfx_sel & 8) // Xevious, PCG 8 colors have priority above kanji roms
				gfx_data = m_pcg_ram.get();
			else if(gfx_sel == 0x80)
			{
				gfx_data = m_kanji_rom;
				tile|= tile_bank << 8;
				if(y_step == 2)
					tile &= 0x3ffe;
			}
			else if(gfx_sel == 0xc0)
			{
				gfx_data = m_kanji_rom;
				tile|= (tile_bank << 8);
				if(y_step == 2)
					tile &= 0x3ffe;
				tile|=0x4000;
			}
			else
			{
				gfx_data = m_pcg_ram.get();
			}

			for(yi=0;yi<8*y_step;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					u8 pen_bit[3],pen;
					int res_x,res_y;

					res_x = x*8+xi;
					res_y = y*y_height+yi-s_y;

					/* check TV window boundaries */
					if(res_x < m_tv_hs || res_x >= m_tv_he || res_y < m_tv_vs || res_y >= m_tv_ve)
						continue;

					if(gfx_sel & 0x8)
					{
						pen_bit[0] = ((gfx_data[tile*8+yi+0x1800]>>(7-xi)) & 1) ? (4+8) : 0; //G
						pen_bit[1] = ((gfx_data[tile*8+yi+0x1000]>>(7-xi)) & 1) ? (2+8) : 0; //R
						pen_bit[2] = ((gfx_data[tile*8+yi+0x0800]>>(7-xi)) & 1) ? (1+8) : 0; //B

						pen = (pen_bit[0]|pen_bit[1]|pen_bit[2]);

						//if(inv_col) { pen ^= 7; } breaks Mappy
					}
					else
						pen = (((gfx_data[tile*8+yi+((gfx_sel & 0x30)<<7)]>>(7-xi)) & 1) ^ inv_col) ? (color+8) : 0;

					if(pen)
					{
						if((res_y) >= 0 && (res_y) < 200*y_step)
							draw_pixel(bitmap,res_x,res_y,pen+(m_pal_select ? 0x00 : 0x10),0,0);
					}
				}
			}

			count++;
			count&=0x7ff;
		}
	}
}

void mz2500_state::draw_40x25(bitmap_ind16 &bitmap,const rectangle &cliprect,int plane,uint16_t map_addr)
{
	u8 *vram = m_tvram;
	int x,y,count,xi,yi;
	u8 *gfx_data;
	u8 y_step;
	u8 s_y;
	u8 y_height;

	count = (((plane * 0x400) + map_addr) & 0x7ff);

	y_step = (m_text_font_reg) ? 1 : 2;
	y_height = (m_text_reg[0] & 0x10) ? 10 : 8;
	s_y = m_text_reg[9] & 0xf;

	for (y=0;y<26*y_step;y+=y_step)
	{
		for (x=0;x<40;x++)
		{
			int tile = vram[count+0x0000] & 0xff;
			int attr = vram[count+0x0800];
			int tile_bank = vram[count+0x1000] & 0x3f;
			int gfx_sel = (attr & 0x38) | (vram[count+0x1000] & 0xc0);
			//int gfx_num;
			int color = attr & 7;
			int inv_col = (attr & 0x40) >> 6;

			if(gfx_sel & 8) // Xevious, PCG 8 colors have priority above kanji roms
				gfx_data = m_pcg_ram.get();
			else if(gfx_sel == 0x80)
			{
				gfx_data = m_kanji_rom;
				tile|= tile_bank << 8;
				if(y_step == 2)
					tile &= 0x3ffe;
			}
			else if(gfx_sel == 0xc0)
			{
				gfx_data = m_kanji_rom;
				tile|= (tile_bank << 8);
				if(y_step == 2)
					tile &= 0x3ffe;
				tile|=0x4000;
			}
			else
			{
				gfx_data = m_pcg_ram.get();
			}

			for(yi=0;yi<8*y_step;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					u8 pen_bit[3],pen;
					int res_x,res_y;

					res_x = x*8+xi;
					res_y = y*y_height+yi-s_y;

					/* check TV window boundaries */
					if(res_x < m_tv_hs || res_x >= m_tv_he || res_y < m_tv_vs || res_y >= m_tv_ve)
						continue;

					if(gfx_sel & 0x8)
					{
						pen_bit[0] = ((gfx_data[tile*8+yi+0x1800]>>(7-xi)) & 1) ? (4+8) : 0; //G
						pen_bit[1] = ((gfx_data[tile*8+yi+0x1000]>>(7-xi)) & 1) ? (2+8) : 0; //R
						pen_bit[2] = ((gfx_data[tile*8+yi+0x0800]>>(7-xi)) & 1) ? (1+8) : 0; //B

						pen = (pen_bit[0]|pen_bit[1]|pen_bit[2]);

						//if(inv_col) { pen ^= 7; } breaks Mappy
					}
					else
						pen = (((gfx_data[tile*8+yi+((gfx_sel & 0x30)<<7)]>>(7-xi)) & 1) ^ inv_col) ? (color+8) : 0;

					if(pen)
					{
						if((res_y) >= 0 && (res_y) < 200*y_step)
							draw_pixel(bitmap,res_x,res_y,pen+(m_pal_select ? 0x00 : 0x10),m_scr_x_size == 640,0);
					}
				}
			}

			count++;
			count&=0x7ff;
		}
	}
}

void mz2500_state::draw_cg4_screen(bitmap_ind16 &bitmap,const rectangle &cliprect,int pri)
{
	uint32_t count;
	u8 *vram = m_cgram;
	u8 pen,pen_bit[2];
	int x,y,xi,pen_i;
	int res_x,res_y;

	count = 0x0000;

	for(y=0;y<400;y++)
	{
		for(x=0;x<640;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				res_x = x+xi;
				res_y = y;

				/* check window boundaries */
				//if(res_x < m_cg_hs || res_x >= m_cg_he || res_y < m_cg_vs || res_y >= m_cg_ve)
				//  continue;

				/* TODO: very preliminary, just Yukar K2 uses this so far*/
				pen_bit[0] = (vram[count + 0x00000]>>(xi)) & 1 ? 7 : 0; // B
				pen_bit[1] = (vram[count + 0x0c000]>>(xi)) & 1 ? 7 : 0; // R

				pen = 0;
				for(pen_i=0;pen_i<2;pen_i++)
					pen |= pen_bit[pen_i];

				{
					//if(pri == ((m_clut256[pen] & 0x100) >> 8))
					draw_pixel(bitmap,res_x,res_y,pen,0,0);
				}
			}
			count++;
		}
	}
}

void mz2500_state::draw_cg16_screen(bitmap_ind16 &bitmap,const rectangle &cliprect,int plane,int x_size,int pri)
{
	uint32_t count;
	u8 *vram = m_cgram; //TODO
	u8 pen,pen_bit[4];
	int x,y,xi,pen_i;
	uint32_t wa_reg;
	u8 s_x;
	u8 base_mask;
	int res_x,res_y;
	u8 cg_interlace;
	u8 pen_mask;

	base_mask = (x_size == 640) ? 0x3f : 0x1f;

	count = (m_cg_reg[0x10]) | ((m_cg_reg[0x11] & base_mask) << 8);
	wa_reg = (m_cg_reg[0x12]) | ((m_cg_reg[0x13] & base_mask) << 8);
	/* TODO: layer 2 scrolling */
	s_x = (m_cg_reg[0x0f] & 0xf);
	cg_interlace = m_text_font_reg ? 1 : 2;
	pen_mask = (m_cg_reg[0x18] >> ((plane & 1) * 4)) & 0x0f;

//  popmessage("%d %d %d %d",m_cg_hs,m_cg_he,m_cg_vs,m_cg_ve);

	for(y=0;y<200;y++)
	{
		for(x=0;x<x_size;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				res_x = x+xi+s_x;
				res_y = y;

				/* check window boundaries */
				if(res_x < m_cg_hs || res_x >= m_cg_he || res_y < m_cg_vs || res_y >= m_cg_ve)
					continue;

				pen_bit[0] = (vram[count+0x0000+((plane & 1) * 0x2000)+(((plane & 2)>>1) * 0x10000)]>>(xi)) & 1 ? (pen_mask & 0x01) : 0; //B
				pen_bit[1] = (vram[count+0x4000+((plane & 1) * 0x2000)+(((plane & 2)>>1) * 0x10000)]>>(xi)) & 1 ? (pen_mask & 0x02) : 0; //R
				pen_bit[2] = (vram[count+0x8000+((plane & 1) * 0x2000)+(((plane & 2)>>1) * 0x10000)]>>(xi)) & 1 ? (pen_mask & 0x04) : 0; //G
				pen_bit[3] = (vram[count+0xc000+((plane & 1) * 0x2000)+(((plane & 2)>>1) * 0x10000)]>>(xi)) & 1 ? (pen_mask & 0x08) : 0; //I

				pen = 0;
				for(pen_i=0;pen_i<4;pen_i++)
					pen |= pen_bit[pen_i];

				if(pri == ((m_clut16[pen] & 0x10) >> 4) && m_clut16[pen] != 0x00 && pen_mask) //correct?
					draw_pixel(bitmap,res_x,res_y,(m_clut16[pen] & 0x0f)+0x10,(x_size == 320 && m_scr_x_size == 640),cg_interlace == 2);
			}
			count++;
			count&=((base_mask<<8) | 0xff);
			if(count > wa_reg)
				count = 0;
		}
	}
}

void mz2500_state::draw_cg256_screen(bitmap_ind16 &bitmap,const rectangle &cliprect,int plane,int pri)
{
	uint32_t count;
	u8 *vram = m_cgram;
	u8 pen,pen_bit[8];
	int x,y,xi,pen_i;
	uint32_t wa_reg;
	u8 s_x;
	u8 base_mask;
	int res_x,res_y;
	u8 cg_interlace;

	base_mask = 0x3f; //no x_size == 640

	count = (m_cg_reg[0x10]) | ((m_cg_reg[0x11] & base_mask) << 8);
	wa_reg = (m_cg_reg[0x12]) | ((m_cg_reg[0x13] & base_mask) << 8);
	/* TODO: layer 2 scrolling */
	s_x = (m_cg_reg[0x0f] & 0xf);
	cg_interlace = m_text_font_reg ? 1 : 2;

	for(y=0;y<200;y++)
	{
		for(x=0;x<320;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				res_x = x+xi+s_x;
				res_y = y;

				/* check window boundaries */
				if(res_x < m_cg_hs || res_x >= m_cg_he || res_y < m_cg_vs || res_y >= m_cg_ve)
					continue;

				pen_bit[0] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0x2000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x10) : 0; // B1
				pen_bit[1] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0x0000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x01) : 0; // B0
				pen_bit[2] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0x6000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x20) : 0; // R1
				pen_bit[3] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0x4000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x02) : 0; // R0
				pen_bit[4] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0xa000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x40) : 0; // G1
				pen_bit[5] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0x8000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x04) : 0; // G0
				pen_bit[6] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0xe000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x80) : 0; // I1
				pen_bit[7] = (vram[count + 0x0000 + (((plane & 2)>>1) * 0x10000) + 0xc000]>>(xi)) & 1 ? (m_cg_reg[0x18] & 0x08) : 0; // I0

				pen = 0;
				for(pen_i=0;pen_i<8;pen_i++)
					pen |= pen_bit[pen_i];

				if(pri == ((m_clut256[pen] & 0x100) >> 8))
					draw_pixel(bitmap,res_x,res_y,(m_clut256[pen] & 0xff)+0x100,m_scr_x_size == 640,cg_interlace == 2);
			}
			count++;
			count&=((base_mask<<8) | 0xff);
			if(count > wa_reg)
				count = 0;
		}
	}
}

void mz2500_state::draw_tv_screen(bitmap_ind16 &bitmap,const rectangle &cliprect)
{
	uint16_t base_addr;

	base_addr = m_text_reg[1] | ((m_text_reg[2] & 0x7) << 8);

//  popmessage("%02x",m_clut16[0]);
//  popmessage("%d %d %d %d",m_tv_hs,(m_tv_he),m_tv_vs,(m_tv_ve));

	if(m_text_col_size)
		draw_80x25(bitmap,cliprect,base_addr);
	else
	{
		int tv_mode;

		tv_mode = m_text_reg[0] >> 2;

		switch(tv_mode & 3)
		{
			case 0: //mixed 6bpp mode, TODO
				draw_40x25(bitmap,cliprect,0,base_addr);
				break;
			case 1:
				draw_40x25(bitmap,cliprect,0,base_addr);
				break;
			case 2:
				draw_40x25(bitmap,cliprect,1,base_addr);
				break;
			case 3:
				draw_40x25(bitmap,cliprect,1,base_addr);
				draw_40x25(bitmap,cliprect,0,base_addr);
				break;
			//default: popmessage("%02x %02x %02x",tv_mode & 3,m_text_reg[1],m_text_reg[2]); break;
		}
	}
}

void mz2500_state::draw_cg_screen(bitmap_ind16 &bitmap,const rectangle &cliprect,int pri)
{
	//popmessage("%02x %02x",m_cg_reg[0x0e],m_cg_reg[0x18]);

	switch(m_cg_reg[0x0e])
	{
		case 0x00:
			break;
		case 0x03:
			draw_cg4_screen(bitmap,cliprect,0);
			break;
		case 0x14:
			draw_cg16_screen(bitmap,cliprect,0,320,pri);
			draw_cg16_screen(bitmap,cliprect,1,320,pri);
			break;
		case 0x15:
			draw_cg16_screen(bitmap,cliprect,1,320,pri);
			draw_cg16_screen(bitmap,cliprect,0,320,pri);
			break;
		case 0x17:
			draw_cg16_screen(bitmap,cliprect,0,640,pri);
			break;
		case 0x1d:
			draw_cg256_screen(bitmap,cliprect,0,pri);
			break;
		case 0x97:
			draw_cg16_screen(bitmap,cliprect,2,640,pri);
			break;
		default:
			popmessage("Unsupported CG mode %02x",m_cg_reg[0x0e]);
			break;
	}
}

uint32_t mz2500_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->pen(0), cliprect); //TODO: correct?

	if(m_screen_enable)
		return 0;

	draw_cg_screen(bitmap,cliprect,0);
	draw_tv_screen(bitmap,cliprect);
	draw_cg_screen(bitmap,cliprect,1);
	//  popmessage("%02x (%02x %02x) (%02x %02x) (%02x %02x) (%02x %02x)",m_cg_reg[0x0f],m_cg_reg[0x10],m_cg_reg[0x11],m_cg_reg[0x12],m_cg_reg[0x13],m_cg_reg[0x14],m_cg_reg[0x15],m_cg_reg[0x16],m_cg_reg[0x17]);
	//  popmessage("%02x",m_text_reg[0x0f]);


	return 0;
}

void mz2500_state::crtc_reconfigure_screen()
{
	rectangle visarea;

	if((m_cg_reg[0x0e] & 0x1f) == 0x17 || (m_cg_reg[0x0e] & 0x1f) == 0x03 || m_text_col_size)
		m_scr_x_size = 640;
	else
		m_scr_x_size = 320;

	if((m_cg_reg[0x0e] & 0x1f) == 0x03)
		m_scr_y_size = 400;
	else
		m_scr_y_size = 200 * ((m_text_font_reg) ? 1 : 2);

	visarea.set(0, m_scr_x_size - 1, 0, m_scr_y_size - 1);

	//popmessage("%d %d %d %d %02x",vs,ve,hs,he,m_cg_reg[0x0e]);

	m_screen->configure(720, 480, visarea, m_screen->frame_period().attoseconds());

	/* calculate CG window parameters here */
	m_cg_vs = m_cg_reg[0x08] | (BIT(m_cg_reg[0x09], 0)<<8);
	m_cg_ve = m_cg_reg[0x0a] | (BIT(m_cg_reg[0x0b], 0)<<8);
	m_cg_hs = (m_cg_reg[0x0c] & 0x7f)*8;
	m_cg_he = (m_cg_reg[0x0d] & 0x7f)*8;

	if(m_scr_x_size == 320)
	{
		m_cg_hs /= 2;
		m_cg_he /= 2;
	}

	/* calculate TV window parameters here */
	{
		int x_offs,y_offs;

		m_monitor_type = ((m_text_reg[0x0f] & 0x08) >> 3);

		switch((m_monitor_type|m_text_col_size<<1) & 3)
		{
			default:
			case 0: x_offs = 64; break;
			case 1: x_offs = 80; break;
			case 2: x_offs = 72; break;
			case 3: x_offs = 88; break;
		}
		//logerror("%d %d %d\n",x_offs,(m_text_reg[7] & 0x7f) * 8,(m_text_reg[8] & 0x7f)* 8);

		y_offs = (m_monitor_type) ? 76 : 34;

		m_tv_hs = ((m_text_reg[7] & 0x7f)*8) - x_offs;
		m_tv_he = ((m_text_reg[8] & 0x7f)*8) - x_offs;
		m_tv_vs = (m_text_reg[3]*2) - y_offs;
		m_tv_ve = (m_text_reg[5]*2) - y_offs;

		if(m_scr_x_size == 320)
		{
			m_tv_hs /= 2;
			m_tv_he /= 2;
		}

		if(m_scr_y_size == 200)
		{
			m_tv_vs /= 2;
			m_tv_ve /= 2;
		}
	}
}

u8 mz2500_state::cg_latch_compare()
{
	u8 compare_val = m_cg_reg[0x07] & 0xf;
	u8 pix_val;
	u8 res;
	uint16_t i;
	res = 0;

	for(i=1;i<0x100;i<<=1)
	{
		pix_val = ((m_cg_latch[0] & i) ? 1 : 0) | ((m_cg_latch[1] & i) ? 2 : 0) | ((m_cg_latch[2] & i) ? 4 : 0) | ((m_cg_latch[3] & i) ? 8 : 0);
		if(pix_val == compare_val)
			res|=i;
	}

	return res;
}

u8 mz2500_state::bank_addr_r()
{
	return m_bank_addr;
}

void mz2500_state::bank_addr_w(u8 data)
{
//  logerror("%02x\n",data);
	m_bank_addr = data & 7;
}

u8 mz2500_state::bank_data_r()
{
	u8 res;

	res = m_bank_val[m_bank_addr];

	m_bank_addr++;
	m_bank_addr&=7;

	return res;
}

void mz2500_state::bank_data_w(u8 data)
{
	m_bank_val[m_bank_addr] = data & 0x3f;
	m_rambank[m_bank_addr]->set_bank(m_bank_val[m_bank_addr]);

//  if((data*2) >= 0x70)
//  logerror("%s %02x\n",bank_name[m_bank_addr],m_bank_val[m_bank_addr]*2);

//  membank(bank_name[m_bank_addr])->set_base(&m_main_ram[m_bank_val[m_bank_addr]*0x2000]);

	m_bank_addr++;
	m_bank_addr&=7;
}

void mz2500_state::bank_mode_w(u8 data)
{
	// TODO: controls memory banking for MZ-2000/MZ-80B compatibility
}

void mz2500_state::kanji_bank_w(u8 data)
{
	m_kanji_bank = data;
}

void mz2500_state::dictionary_bank_w(u8 data)
{
	m_dic_bank = data;
}

/* 0xf4 - 0xf7 all returns vblank / hblank states */
u8 mz2500_state::crtc_hvblank_r()
{
	u8 vblank_bit, hblank_bit;

	vblank_bit = m_screen->vblank() ? 0 : 1;
	hblank_bit = m_screen->hblank() ? 0 : 2;

	return vblank_bit | hblank_bit;
}

/*
TVRAM / CRTC registers

[0x00] ---x ---- line height (0) 16 (1) 20
[0x00] ---- xx-- 40 column mode (0) 64 colors (1) screen 1 (2) screen 2 (3) screen 1 + screen 2
[0x00] ---- --x- (related to the transparent pen)
[0x01] xxxx xxxx TV map offset low address value
[0x02] ---- -xxx TV map offset high address value
[0x03] xxxx xxxx CRTC vertical start register

[0x05] xxxx xxxx CRTC vertical end register

[0x07] -xxx xxxx CRTC horizontal start register
[0x08] -xxx xxxx CRTC horizontal end register
[0x09] ---- xxxx vertical scrolling shift position
[0x0a] --GG RRBB 256 color mode
[0x0b] -r-- ---- Back plane red gradient
[0x0b] ---- -b-- Back plane blue gradient
[0x0b] ---- ---i Back plane i gradient
[0x0c] ---- ---g Back plane green gradient

[0x0f] ---- x--- sets monitor type interlace / progressive
*/

u8 mz2500_state::pal_256_param(int index, int param)
{
	u8 val = 0;

	switch(param & 3)
	{
		case 0: val = index & 0x80 ? 1 : 0; break;
		case 1: val = index & 0x08 ? 1 : 0; break;
		case 2: val = 1; break;
		case 3: val = 0; break;
	}

	return val;
}

void mz2500_state::tv_crtc_w(offs_t offset, u8 data)
{
	switch(offset)
	{
		case 0: m_text_reg_index = data; break;
		case 1:
			m_text_reg[m_text_reg_index] = data;

			#if 0
			//logerror("[%02x] <- %02x\n",m_text_reg_index,data);
			popmessage("(%02x %02x) (%02x %02x %02x %02x) (%02x %02x %02x) (%02x %02x %02x %02x)"
			,m_text_reg[0] & ~0x1e,m_text_reg[3]
			,m_text_reg[4],m_text_reg[5],m_text_reg[6],m_text_reg[7]
			,m_text_reg[8],m_text_reg[10],m_text_reg[11]
			,m_text_reg[12],m_text_reg[13],m_text_reg[14],m_text_reg[15]);

			#endif
			//popmessage("%d %02x %d %02x %d %d",m_text_reg[3],m_text_reg[4],m_text_reg[5],m_text_reg[6],m_text_reg[7]*8,m_text_reg[8]*8);

			crtc_reconfigure_screen();

			if(m_text_reg_index == 0x0a) // set 256 color palette
			{
				int i,r,g,b;
				u8 b_param,r_param,g_param;

				b_param = (data & 0x03) >> 0;
				r_param = (data & 0x0c) >> 2;
				g_param = (data & 0x30) >> 4;

				for(i = 0;i < 0x100;i++)
				{
					int bit0,bit1,bit2;

					bit0 = pal_256_param(i,b_param) ? 1 : 0;
					bit1 = i & 0x01 ? 2 : 0;
					bit2 = i & 0x10 ? 4 : 0;
					b = bit0|bit1|bit2;
					bit0 = pal_256_param(i,r_param) ? 1 : 0;
					bit1 = i & 0x02 ? 2 : 0;
					bit2 = i & 0x20 ? 4 : 0;
					r = bit0|bit1|bit2;
					bit0 = pal_256_param(i,g_param) ? 1 : 0;
					bit1 = i & 0x04 ? 2 : 0;
					bit2 = i & 0x40 ? 4 : 0;
					g = bit0|bit1|bit2;

					m_palette->set_pen_color(i+0x100,pal3bit(r),pal3bit(g),pal3bit(b));
				}
			}
			if(m_text_reg_index >= 0x80 && m_text_reg_index <= 0x8f) //Bitmap 16 clut registers
			{
				/*
				---x ---- priority
				---- xxxx clut number
				*/
				m_clut16[m_text_reg_index & 0xf] = data & 0x1f;
				//logerror("%02x -> [%02x]\n",m_text_reg[m_text_reg_index],m_text_reg_index);

				{
					int i;

					for(i=0;i<0x10;i++)
					{
						m_clut256[(m_text_reg_index & 0xf) | (i << 4)] = (((data & 0x1f) << 4) | i);
					}
				}
			}
			break;
		case 2: /* CG Mask reg (priority mixer) */
			m_cg_mask = data;
			break;
		case 3:
			/* Font size reg */
			m_text_font_reg = data & 1;
			crtc_reconfigure_screen();
			break;
	}
}

void mz2500_state::irq_sel_w(u8 data)
{
	m_irq_sel = data;
	//logerror("%02x\n",m_irq_sel);
	// activeness is trusted, see Tower of Druaga
	m_irq_mask[0] = (data & 0x08); //CRTC
	m_irq_mask[1] = (data & 0x04); //i8253
	m_irq_mask[2] = (data & 0x02); //printer
	m_irq_mask[3] = (data & 0x01); //RP5c15
}

void mz2500_state::irq_data_w(u8 data)
{
	if(m_irq_sel & 0x80)
		m_irq_vector[0] = data; //CRTC
	if(m_irq_sel & 0x40)
		m_irq_vector[1] = data; //i8253
	if(m_irq_sel & 0x20)
		m_irq_vector[2] = data; //printer
	if(m_irq_sel & 0x10)
		m_irq_vector[3] = data; //RP5c15

//  popmessage("%02x %02x %02x %02x",m_irq_vector[0],m_irq_vector[1],m_irq_vector[2],m_irq_vector[3]);
}

void mz2500_state::floppy_select_w(u8 data)
{
	m_selected_floppy = m_floppy[(data & 0x03) ^ m_fdc_reverse]->get_device();
	m_fdc->set_floppy(m_selected_floppy);

	if (m_selected_floppy)
		m_selected_floppy->mon_w(!BIT(data, 7));
}

void mz2500_state::floppy_side_w(u8 data)
{
	if (m_selected_floppy)
		m_selected_floppy->ss_w(BIT(data, 0));
}

void mz2500_state::floppy_dden_w(u8 data)
{
	m_fdc->dden_w(BIT(data, 0));
}

u8 mz2500_state::rmw_r(offs_t offset)
{
	// TODO: correct?
	if(m_cg_reg[0x0e] == 0x3)
		return 0xff;

	int plane;
	m_cg_latch[0] = m_cgram[offset+0x0000]; //B
	m_cg_latch[1] = m_cgram[offset+0x4000]; //R
	m_cg_latch[2] = m_cgram[offset+0x8000]; //G
	m_cg_latch[3] = m_cgram[offset+0xc000]; //I
	plane = m_cg_reg[0x07] & 3;

	if(m_cg_reg[0x07] & 0x10)
		return cg_latch_compare();

	return m_cg_latch[plane];
}

void mz2500_state::rmw_w(offs_t offset, u8 data)
{
	// TODO: correct?
	if(m_cg_reg[0x0e] == 0x3)
		return;

	if((m_cg_reg[0x05] & 0xc0) == 0x00) //replace
	{
		if(m_cg_reg[5] & 1) //B
		{
			m_cgram[offset+0x0000] &= ~m_cg_reg[6];
			m_cgram[offset+0x0000] |= (m_cg_reg[4] & 1) ? (data & m_cg_reg[0] & m_cg_reg[6]) : 0;
		}
		if(m_cg_reg[5] & 2) //R
		{
			m_cgram[offset+0x4000] &= ~m_cg_reg[6];
			m_cgram[offset+0x4000] |= (m_cg_reg[4] & 2) ? (data & m_cg_reg[1] & m_cg_reg[6]) : 0;
		}
		if(m_cg_reg[5] & 4) //G
		{
			m_cgram[offset+0x8000] &= ~m_cg_reg[6];
			m_cgram[offset+0x8000] |= (m_cg_reg[4] & 4) ? (data & m_cg_reg[2] & m_cg_reg[6]) : 0;
		}
		if(m_cg_reg[5] & 8) //I
		{
			m_cgram[offset+0xc000] &= ~m_cg_reg[6];
			m_cgram[offset+0xc000] |= (m_cg_reg[4] & 8) ? (data & m_cg_reg[3] & m_cg_reg[6]) : 0;
		}
	}
	else if((m_cg_reg[0x05] & 0xc0) == 0x40) //pset
	{
		if(m_cg_reg[5] & 1) //B
		{
			m_cgram[offset+0x0000] &= ~data;
			m_cgram[offset+0x0000] |= (m_cg_reg[4] & 1) ? (data & m_cg_reg[0]) : 0;
		}
		if(m_cg_reg[5] & 2) //R
		{
			m_cgram[offset+0x4000] &= ~data;
			m_cgram[offset+0x4000] |= (m_cg_reg[4] & 2) ? (data & m_cg_reg[1]) : 0;
		}
		if(m_cg_reg[5] & 4) //G
		{
			m_cgram[offset+0x8000] &= ~data;
			m_cgram[offset+0x8000] |= (m_cg_reg[4] & 4) ? (data & m_cg_reg[2]) : 0;
		}
		if(m_cg_reg[5] & 8) //I
		{
			m_cgram[offset+0xc000] &= ~data;
			m_cgram[offset+0xc000] |= (m_cg_reg[4] & 8) ? (data & m_cg_reg[3]) : 0;
		}
	}
}

u8 mz2500_state::kanji_pcg_r(offs_t offset)
{
	// kanji ROM
	if(m_kanji_bank & 0x80)
		return m_kanji_rom[(offset & 0x7ff)+((m_kanji_bank & 0x7f)*0x800)];

	// PCG RAM
	return m_pcg_ram[offset];
}

void mz2500_state::kanji_pcg_w(offs_t offset, u8 data)
{
	// PCG RAM
	if((m_kanji_bank & 0x80) == 0)
	{
		m_pcg_ram[offset] = data;
		if((offset & 0x1800) == 0x0000)
			m_gfxdecode->gfx(3)->mark_dirty((offset) >> 3);
		else
			m_gfxdecode->gfx(4)->mark_dirty((offset & 0x7ff) >> 3);
	}
	// kanji ROM is read only
}

u8 mz2500_state::dict_rom_r(offs_t offset)
{
	return m_dic_rom[(offset & 0x1fff) + ((m_dic_bank & 0x1f)*0x2000)];
}

/* sets 16 color entries out of 4096 possible combinations */
void mz2500_state::palette4096_io_w(offs_t offset, u8 data)
{
	u8 pal_index;
	u8 pal_entry;

	pal_index = (offset >> 8) & 0xff;
	pal_entry = (pal_index & 0x1e) >> 1;

	if(pal_index & 1)
		m_pal[pal_entry].g = (data & 0x0f);
	else
	{
		m_pal[pal_entry].r = (data & 0xf0) >> 4;
		m_pal[pal_entry].b = data & 0x0f;
	}

	m_palette->set_pen_color(pal_entry+0x10, pal4bit(m_pal[pal_entry].r), pal4bit(m_pal[pal_entry].g), pal4bit(m_pal[pal_entry].b));
}

u8 mz2500_state::bplane_latch_r()
{
	if(m_cg_reg[7] & 0x10)
		return cg_latch_compare();
	else
		return m_cg_latch[0];
}


u8 mz2500_state::rplane_latch_r()
{
	if(m_cg_reg[0x07] & 0x10)
	{
		u8 vblank_bit;

		vblank_bit = m_screen->vblank() ? 0 : 0x80 | m_cg_clear_flag;

		return vblank_bit;
	}
	else
		return m_cg_latch[1];
}

u8 mz2500_state::gplane_latch_r()
{
	return m_cg_latch[2];
}

u8 mz2500_state::iplane_latch_r()
{
	return m_cg_latch[3];
}

/*
"GDE" CRTC registers

0x00: CG B - Replace / Pset register data mask
0x01: CG R - Replace / Pset register data mask
0x02: CG G - Replace / Pset register data mask
0x03: CG I - Replace / Pset register data mask
0x04: CG Replace / Pset active pixel
0x05: CG Replace / Pset mode register
0x06: CG Replace data register
0x07: compare CG buffer register
0x08: CG window vertical start lo reg
0x09: CG window vertical start hi reg (upper 1 bit)
0x0a: CG window vertical end lo reg
0x0b: CG window vertical end hi reg (upper 1 bit)
0x0c: CG window horizontal start reg (7 bits, val x 8)
0x0d: CG window horizontal end reg (7 bits, val x 8)
0x0e: CG mode
0x0f: vertical scroll shift (---- xxxx)
0x10: CG map base lo reg
0x11: CG map base hi reg
0x12: CG layer 0 end address lo reg
0x13: CG layer 0 end address hi reg
0x14: CG layer 1 start address lo reg
0x15: CG layer 1 start address hi reg
0x16: CG layer 1 y pixel start lo reg
0x17: CG layer 1 y pixel start hi reg
0x18: CG color masking
*/

void mz2500_state::cg_addr_w(u8 data)
{
	m_cg_reg_index = data;
}

void mz2500_state::cg_data_w(u8 data)
{
	m_cg_reg[m_cg_reg_index & 0x1f] = data;

	if((m_cg_reg_index & 0x1f) == 0x08) //accessing VS LO reg clears VS HI reg
		m_cg_reg[0x09] = 0;

	if((m_cg_reg_index & 0x1f) == 0x0a) //accessing VE LO reg clears VE HI reg
		m_cg_reg[0x0b] = 0;

	if((m_cg_reg_index & 0x1f) == 0x05 && (m_cg_reg[0x05] & 0xc0) == 0x80) //clear bitmap buffer
	{
		uint32_t i;
		u8 *vram = m_cgram;
		uint32_t layer_bank;

		layer_bank = (m_cg_reg[0x0e] & 0x80) ? 0x10000 : 0x00000;

		/* TODO: this isn't yet 100% accurate */
		if(m_cg_reg[0x05] & 1)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0x0000+layer_bank] = 0x00; //clear B
		}
		if(m_cg_reg[0x05] & 2)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0x4000+layer_bank] = 0x00; //clear R
		}
		if(m_cg_reg[0x05] & 4)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0x8000+layer_bank] = 0x00; //clear G
		}
		if(m_cg_reg[0x05] & 8)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0xc000+layer_bank] = 0x00; //clear I
		}
		m_cg_clear_flag = 1;
	}

	{
		crtc_reconfigure_screen();
	}

	if(m_cg_reg_index & 0x80) //enable auto-inc
		m_cg_reg_index = (m_cg_reg_index & 0xfc) | ((m_cg_reg_index + 1) & 0x03);
}

void mz2500_state::timer_w(u8 data)
{
	m_pit->write_gate0(1);
	m_pit->write_gate1(1);
	m_pit->write_gate0(0);
	m_pit->write_gate1(0);
	m_pit->write_gate0(1);
	m_pit->write_gate1(1);
}


u8 mz2500_state::joystick_r()
{
	u8 res,dir_en,in_r;

	res = 0xff;
	in_r = ~m_joy[BIT(m_joy_mode, 6)]->read();

	if(m_joy_mode & 0x40)
	{
		if(!(m_joy_mode & 0x04)) res &= ~0x20;
		if(!(m_joy_mode & 0x08)) res &= ~0x10;
		dir_en = (m_joy_mode & 0x20) ? 0 : 1;
	}
	else
	{
		if(!(m_joy_mode & 0x01)) res &= ~0x20;
		if(!(m_joy_mode & 0x02)) res &= ~0x10;
		dir_en = (m_joy_mode & 0x10) ? 0 : 1;
	}

	if(dir_en)
		res &= (~((in_r) & 0x0f));

	res &= (~((in_r) & 0x30));

	return res;
}

void mz2500_state::joystick_w(u8 data)
{
	m_joy_mode = data;
	m_joy[0]->pin_6_w(BIT(data, 0));
	m_joy[0]->pin_7_w(BIT(data, 1));
	m_joy[1]->pin_6_w(BIT(data, 2));
	m_joy[1]->pin_7_w(BIT(data, 3));
	m_joy[0]->pin_8_w(BIT(data, 4));
	m_joy[1]->pin_8_w(BIT(data, 5));
}


u8 mz2500_state::kanji_r(offs_t offset)
{
	logerror("Read from kanji 2 ROM\n");

	return m_kanji2_rom[(m_kanji_index << 1) | (offset & 1)];
}

void mz2500_state::kanji_w(offs_t offset, u8 data)
{
	(offset & 1) ? (m_kanji_index = (data << 8) | (m_kanji_index & 0xff)) : (m_kanji_index = (data & 0xff) | (m_kanji_index & 0xff00));
}

u8 mz2500_state::rp5c15_8_r(offs_t offset)
{
	u8 rtc_index = (offset >> 8) & 0xff;

	return m_rtc->read(rtc_index);
}

void mz2500_state::rp5c15_8_w(offs_t offset, u8 data)
{
	u8 rtc_index = (offset >> 8) & 0xff;

	m_rtc->write(rtc_index, data);
}

template <unsigned N> u8 mz2500_state::sio_access_r(address_space &space, offs_t offset)
{
	if (N != m_sio_access_bit)
		return space.unmap();
	return m_sio->ba_cd_r(offset);
}

template <unsigned N> void mz2500_state::sio_access_w(address_space &space, offs_t offset, u8 data)
{
	if (N == m_sio_access_bit)
		m_sio->ba_cd_w(offset, data);
}

/*
 * x--- ---- Select SIO port (0) $a0-$a3 (1) $b0-$b3
 * --xx x--- Baud rate for Ch. A
 * ---- -xxx Baud rate for Ch. B
 * ---- -000 307'000 Hz / 19200 bps
 * ...
 * ---- -111 2400 Hz / 150 bps
 */
void mz2500_state::sio_setup_w(u8 data)
{
	m_sio_access_bit = !!(BIT(data, 7));

	attotime serial_clock_a = attotime::from_hz((4'000'000 / 13) / (1 << ((data >> 3) & 7)));
	attotime serial_clock_b = attotime::from_hz((4'000'000 / 13) / (1 << ((data >> 0) & 7)));

	m_sio_timer[0]->adjust(serial_clock_a, 0, serial_clock_a);
	m_sio_timer[1]->adjust(serial_clock_b, 0, serial_clock_b);
}

void mz2500_state::z80_map(address_map &map)
{
	map(0x0000, 0x1fff).m(m_rambank[0], FUNC(address_map_bank_device::amap8));
	map(0x2000, 0x3fff).m(m_rambank[1], FUNC(address_map_bank_device::amap8));
	map(0x4000, 0x5fff).m(m_rambank[2], FUNC(address_map_bank_device::amap8));
	map(0x6000, 0x7fff).m(m_rambank[3], FUNC(address_map_bank_device::amap8));
	map(0x8000, 0x9fff).m(m_rambank[4], FUNC(address_map_bank_device::amap8));
	map(0xa000, 0xbfff).m(m_rambank[5], FUNC(address_map_bank_device::amap8));
	map(0xc000, 0xdfff).m(m_rambank[6], FUNC(address_map_bank_device::amap8));
	map(0xe000, 0xffff).m(m_rambank[7], FUNC(address_map_bank_device::amap8));
}

void mz2500_state::bank_window_map(address_map &map)
{
	// 0x00-0x1f
	map(0x00000, 0x3ffff).ram().share("wram");
	// 0x20-0x2f
	map(0x40000, 0x5ffff).ram().share("cgram");
	// 0x30-0x33
	map(0x60000, 0x67fff).rw(FUNC(mz2500_state::rmw_r),FUNC(mz2500_state::rmw_w));
	// 0x34-0x37
	map(0x68000, 0x6ffff).rom().region("ipl", 0);
	// 0x38
	map(0x70000, 0x71fff).ram().share("tvram");
	// 0x39
	map(0x72000, 0x73fff).rw(FUNC(mz2500_state::kanji_pcg_r),FUNC(mz2500_state::kanji_pcg_w));
	// 0x3a
	map(0x74000, 0x75fff).r(FUNC(mz2500_state::dict_rom_r));
	// 0x3b
//  map(0x76000, 0x77fff).noprw();
	// 0x3c-0x3f
	map(0x78000, 0x7ffff).rom().region("phone", 0);
}

void mz2500_state::z80_io(address_map &map)
{
	map.unmap_value_high();
//  map(0x60, 0x63).mirror(0xff00).w(FUNC(mz2500_state::w3100a_w));
//  map(0x63, 0x63).mirror(0xff00).r(FUNC(mz2500_state::w3100a_r));
//  map(0x98, 0x99) MZ-1E35 ADPCM
	map(0xa0, 0xa3).mirror(0xff00).rw(FUNC(mz2500_state::sio_access_r<0>), FUNC(mz2500_state::sio_access_w<0>));
	map(0xb0, 0xb3).mirror(0xff00).rw(FUNC(mz2500_state::sio_access_r<1>), FUNC(mz2500_state::sio_access_w<1>));
//  map(0xa4, 0xa5) MZ-1E30 SASI
//  map(0xa8, 0xa9) ^
//  map(0xac, 0xac) MZ-1R37 EMM
//  map(0xad, 0xad) ^
	map(0xae, 0xae).select(0xff00).w(FUNC(mz2500_state::palette4096_io_w));
//  map(0xb0, 0xb3).rw(FUNC(mz2500_state::sio_r), FUNC(mz2500_state::sio_w));
	map(0xb4, 0xb4).mirror(0xff00).rw(FUNC(mz2500_state::bank_addr_r), FUNC(mz2500_state::bank_addr_w));
	map(0xb5, 0xb5).mirror(0xff00).rw(FUNC(mz2500_state::bank_data_r), FUNC(mz2500_state::bank_data_w));
	map(0xb7, 0xb7).mirror(0xff00).w(FUNC(mz2500_state::bank_mode_w));
	map(0xb8, 0xb9).mirror(0xff00).rw(FUNC(mz2500_state::kanji_r), FUNC(mz2500_state::kanji_w));
	map(0xbc, 0xbc).mirror(0xff00).r(FUNC(mz2500_state::bplane_latch_r)).w(FUNC(mz2500_state::cg_addr_w));
	map(0xbd, 0xbd).mirror(0xff00).r(FUNC(mz2500_state::rplane_latch_r)).w(FUNC(mz2500_state::cg_data_w));
	map(0xbe, 0xbe).mirror(0xff00).r(FUNC(mz2500_state::gplane_latch_r));
	map(0xbf, 0xbf).mirror(0xff00).r(FUNC(mz2500_state::iplane_latch_r));
	map(0xc6, 0xc6).mirror(0xff00).w(FUNC(mz2500_state::irq_sel_w));
	map(0xc7, 0xc7).mirror(0xff00).w(FUNC(mz2500_state::irq_data_w));
	map(0xc8, 0xc9).mirror(0xff00).rw("ym", FUNC(ym2203_device::read), FUNC(ym2203_device::write));
//  map(0xca, 0xca).mirror(0xff00).rw(FUNC(mz2500_state::voice_r), FUNC(mz2500_state::voice_w));
	// MZ-1E26
	map(0xca, 0xca).mirror(0xff00).lr8(NAME([] () { return 0x30; }));
	map(0xcc, 0xcc).select(0xff00).rw(FUNC(mz2500_state::rp5c15_8_r), FUNC(mz2500_state::rp5c15_8_w));
	map(0xcd, 0xcd).mirror(0xff00).w(FUNC(mz2500_state::sio_setup_w));
	map(0xce, 0xce).mirror(0xff00).w(FUNC(mz2500_state::dictionary_bank_w));
	map(0xcf, 0xcf).mirror(0xff00).w(FUNC(mz2500_state::kanji_bank_w));
	map(0xd8, 0xdb).mirror(0xff00).rw(m_fdc, FUNC(mb8876_device::read), FUNC(mb8876_device::write));
	map(0xdc, 0xdc).mirror(0xff00).w(FUNC(mz2500_state::floppy_select_w));
	map(0xdd, 0xdd).mirror(0xff00).w(FUNC(mz2500_state::floppy_side_w));
	map(0xde, 0xde).mirror(0xff00).w(FUNC(mz2500_state::floppy_dden_w));
	map(0xe0, 0xe3).mirror(0xff00).rw("ppi", FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0xe4, 0xe7).mirror(0xff00).rw(m_pit, FUNC(pit8253_device::read), FUNC(pit8253_device::write));
	map(0xe8, 0xeb).mirror(0xff00).rw("pio", FUNC(z80pio_device::read_alt), FUNC(z80pio_device::write_alt));
	map(0xef, 0xef).mirror(0xff00).rw(FUNC(mz2500_state::joystick_r), FUNC(mz2500_state::joystick_w));
	map(0xf0, 0xf3).mirror(0xff00).w(FUNC(mz2500_state::timer_w));
	map(0xf4, 0xf7).mirror(0xff00).r(FUNC(mz2500_state::crtc_hvblank_r)).w(FUNC(mz2500_state::tv_crtc_w));
//  map(0xf8, 0xf9).?(0xff00).rw(FUNC(mz2500_state::extrom_r), FUNC(mz2500_state::extrom_w));
//  map(0xfe, 0xff).mirror(0xff00) printer
}


INPUT_CHANGED_MEMBER(mz2500_state::boot_reset_cb)
{
	m_maincpu->set_input_line(INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

INPUT_CHANGED_MEMBER(mz2500_state::ipl_reset_cb)
{
	machine().schedule_soft_reset();
}

TIMER_CALLBACK_MEMBER(mz2500_state::ipl_timer_reset_cb)
{
	logerror("IPL reset kicked in\n");
	machine().schedule_soft_reset();
}

static INPUT_PORTS_START( mz2500 )
	// section 8 of schematics
	// CN1 switches 8-9
	PORT_START("FRONT_PANEL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(mz2500_state::boot_reset_cb), 0) PORT_NAME("Boot Reset")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(mz2500_state::ipl_reset_cb), 0) PORT_NAME("IPL Reset")

	// TODO: generalize keyboard in device (mostly same as other MZ machines)
	PORT_START("KEY0")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))

	PORT_START("KEY1")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("8 (PAD)") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("9 (PAD)") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(", (PAD)") PORT_CODE(KEYCODE_COMMA_PAD)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(". (PAD)") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("+ (PAD)") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("- (PAD)") PORT_CODE(KEYCODE_MINUS_PAD)

	PORT_START("KEY2")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("0 (PAD)") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("1 (PAD)") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("2 (PAD)") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("3 (PAD)") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("4 (PAD)") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("5 (PAD)") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("6 (PAD)") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("7 (PAD)") PORT_CODE(KEYCODE_7_PAD)

	PORT_START("KEY3")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CHAR(3) //PORT_CODE(KEYCODE_ESC)

	PORT_START("KEY4")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("KEY5")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY6")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("KEY7")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("\xEF\xBF\xA5") PORT_CHAR(165) PORT_CHAR('|') //Yen symbol
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("_") PORT_CHAR('_')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("KEY8")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("KEY9")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')  PORT_CHAR('=')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYA")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("COPY")
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CLR")
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("INST")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(27)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("* (PAD)") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("/ (PAD)") PORT_CODE(KEYCODE_SLASH_PAD)

	PORT_START("KEYB")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("GRPH")
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("SLOCK")
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KANA")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0xe0,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYC")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KJ1")
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KJ2")
	PORT_BIT(0xfc,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYD")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("LOGO KEY")
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("HELP")
	PORT_BIT(0xfc,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("UNUSED")
	PORT_BIT(0xff,IP_ACTIVE_LOW,IPT_UNUSED )

	/* this enables HD-loader */
	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "DSW1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "IPLPRO" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x30, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Monitor Interlace" ) //not all games support this
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

void mz2500_state::reset_banks(u8 type)
{
	int i;

	for(i=0;i<8;i++)
	{
		m_bank_val[i] = bank_reset_val[type][i];
		m_rambank[i]->set_bank(m_bank_val[i]);
	}
}

static const gfx_layout pcg_layout_1bpp =
{
	8, 8,
	0x100,
	1,
	{ 0 },
	{ STEP8(0, 1) },
	{ STEP8(0, 8) },
	8 * 8
};

static const gfx_layout pcg_layout_3bpp =
{
	8, 8,
	0x100,
	3,
	{ 0x1800*8, 0x1000*8, 0x800*8 },
	{ STEP8(0, 1) },
	{ STEP8(0, 8) },
	8 * 8
};

void mz2500_state::machine_start()
{
	m_pcg_ram = make_unique_clear<u8[]>(0x2000);
	m_ipl_rom = memregion("ipl")->base();
	m_kanji_rom = memregion("kanji")->base();
	m_kanji2_rom = memregion("kanji2")->base();
	m_dic_rom = memregion("dictionary")->base();
	m_phone_rom = memregion("phone")->base();

	save_pointer(NAME(m_pcg_ram), 0x2000);

	m_gfxdecode->set_gfx(3, std::make_unique<gfx_element>(m_palette, pcg_layout_1bpp, m_pcg_ram.get(), 0, 0x10, 0));
	m_gfxdecode->set_gfx(4, std::make_unique<gfx_element>(m_palette, pcg_layout_3bpp, m_pcg_ram.get(), 0, 4, 0));

	std::fill(std::begin(m_cgram), std::end(m_cgram), 0x00);
	std::fill(std::begin(m_irq_pending), std::end(m_irq_pending), 0);
	std::fill(std::begin(m_irq_mask), std::end(m_irq_mask), 0);

	m_text_col_size = 0;
	m_text_font_reg = 0;

	m_kanji_bank = 0;

	m_cg_clear_flag = 0;

	m_ipl_reset_timer = timer_alloc(FUNC(mz2500_state::ipl_timer_reset_cb), this);

	m_sio_timer[0] = timer_alloc(FUNC(mz2500_state::sio_clock_cb<0>), this);
	m_sio_timer[1] = timer_alloc(FUNC(mz2500_state::sio_clock_cb<1>), this);
}

void mz2500_state::machine_reset()
{
	m_bank_addr = 0;
	reset_banks(IPL_RESET);

	m_ipl_reset_timer->adjust(attotime::never);

	m_sio_access_bit = false;
	m_dac1bit->level_w(0);

//  m_monitor_type = ioport("DSW1")->read() & 0x40 ? 1 : 0;

	joystick_w(0x3f); // LS273 reset
}

static const gfx_layout kanji_cg_layout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(0, 1) },
	{ STEP8(0, 8) },
	8 * 8
};

/* gfx1 is mostly 16x16, but there are some 8x8 characters */
static const gfx_layout kanji_8_layout =
{
	8, 8,
	1920,
	1,
	{ 0 },
	{ STEP8(0, 1) },
	{ STEP8(0, 8) },
	8 * 8
};

static const gfx_layout kanji_16_layout =
{
	16, 16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(0, 1), STEP8(128, 1) },
	{ STEP16(0, 8) },
	16 * 16
};

/* these are just for viewer sake, actually they aren't used in drawing routines */
static GFXDECODE_START( gfx_mz2500 )
	GFXDECODE_ENTRY("kanji", 0, kanji_cg_layout, 0, 256)
	GFXDECODE_ENTRY("kanji", 0x4400, kanji_8_layout, 0, 256)
	GFXDECODE_ENTRY("kanji", 0, kanji_16_layout, 0, 256)
//  GFXDECODE_ENTRY("pcg", 0, pcg_layout_1bpp, 0, 0x10)
//  GFXDECODE_ENTRY("pcg", 0, pcg_layout_3bpp, 0, 4)
GFXDECODE_END

INTERRUPT_GEN_MEMBER(mz2500_state::vblank_cb)
{
	if(m_irq_mask[0])
	{
		m_irq_pending[0] = 1;
		m_maincpu->set_input_line(0, ASSERT_LINE);
	}
	m_cg_clear_flag = 0;
}

IRQ_CALLBACK_MEMBER(mz2500_state::irq_ack_cb)
{
	int i;
	for(i=0;i<4;i++)
	{
		if(m_irq_mask[i] && m_irq_pending[i])
		{
			m_irq_pending[i] = 0;
			m_maincpu->set_input_line(0, CLEAR_LINE);
			return m_irq_vector[i];
		}
	}
	return 0;
}

u8 mz2500_state::ppi_porta_r()
{
	logerror("PPI PORTA R\n");

	return 0xff;
}

u8 mz2500_state::ppi_portb_r()
{
	u8 vblank_bit;

	vblank_bit = m_screen->vblank() ? 0 : 1; //Guess: NOBO wants this bit to be high/low

	return 0xfe | vblank_bit;
}

u8 mz2500_state::ppi_portc_r()
{
	logerror("PPI PORTC R\n");

	return 0xff;
}

void mz2500_state::ppi_porta_w(u8 data)
{
	logerror("PPI PORTA W %02x\n",data);
}

void mz2500_state::ppi_portb_w(u8 data)
{
	logerror("PPI PORTB W %02x\n",data);
}

void mz2500_state::ppi_portc_w(u8 data)
{
	/*
	---- x--- 0->1 transition = IPL reset
	---- -x-- beeper state
	---- --x- 0->1 transition = Work RAM reset
	---- ---x screen mask
	*/

	if(BIT(m_old_portc, 3) != BIT(data, 3))
	{
		logerror("PIO PC: IPL reset %s\n", BIT(data, 3) ? "stopped" : "started");
		// TODO: timing
		m_ipl_reset_timer->adjust(!BIT(data, 3) ? attotime::from_hz(100) : attotime::never);
	}

	if(!BIT(m_old_portc, 1) && BIT(data, 1))
	{
		logerror("PIO PC: Work RAM reset\n");
		reset_banks(WRAM_RESET);
		m_maincpu->pulse_input_line(INPUT_LINE_RESET, attotime::zero);
	}

	m_old_portc = data;

	m_dac1bit->level_w(BIT(data, 2));

	m_screen_enable = data & 1;

	if(data & ~0x0f)
		logerror("PPI PORTC W %02x\n",data & ~0x0f);
}

void mz2500_state::pio_porta_w(u8 data)
{
//  logerror("%02x\n",data);

	if(m_prev_col_val != ((data & 0x20) >> 5))
	{
		m_text_col_size = ((data & 0x20) >> 5);
		m_prev_col_val = m_text_col_size;
		crtc_reconfigure_screen();
	}
	m_key_mux = data & 0x1f;
}


u8 mz2500_state::pio_porta_r()
{
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3",
											"KEY4", "KEY5", "KEY6", "KEY7",
											"KEY8", "KEY9", "KEYA", "KEYB",
											"KEYC", "KEYD", "UNUSED", "UNUSED" };

	if(((m_key_mux & 0x10) == 0x00) || ((m_key_mux & 0x0f) == 0x0f)) //status read
	{
		int res,i;

		res = 0xff;
		for(i=0;i<0xe;i++)
			res &= ioport(keynames[i])->read();

		m_pio_latchb = res;

		return res;
	}

	m_pio_latchb = ioport(keynames[m_key_mux & 0xf])->read();

	return ioport(keynames[m_key_mux & 0xf])->read();
}

u8 mz2500_state::opn_porta_r()
{
	return m_ym_porta;
}

void mz2500_state::opn_porta_w(u8 data)
{
	/*
	---- x--- mouse select
	---- -x-- palette bit (16/4096 colors)
	---- --x- floppy reverse bit (controls wd17xx bits in command registers)
	*/

	m_fdc_reverse = data & 2;
	m_pal_select = (data & 4) ? 1 : 0;

	m_ym_porta = data;
}

void mz2500_state::palette_init(palette_device &palette) const
{
	for (int i = 0; i < 0x200; i++)
		palette.set_pen_color(i,pal1bit(0),pal1bit(0),pal1bit(0));

	// set up 8 colors (PCG)
	for (int i = 0; i < 8; i++)
		palette.set_pen_color(i+8,pal1bit((i & 2)>>1),pal1bit((i & 4)>>2),pal1bit((i & 1)>>0));

	// set up 16 colors (PCG / CG)

	// set up 256 colors (CG)
	for (int i = 0; i < 0x100; i++)
	{
		int bit0, bit1, bit2;

		bit0 = pal_256_param(i,0) ? 1 : 0;
		bit1 = i & 0x01 ? 2 : 0;
		bit2 = i & 0x10 ? 4 : 0;
		int const b = bit0|bit1|bit2;
		bit0 = pal_256_param(i,0) ? 1 : 0;
		bit1 = i & 0x02 ? 2 : 0;
		bit2 = i & 0x20 ? 4 : 0;
		int const r = bit0|bit1|bit2;
		bit0 = pal_256_param(i,0) ? 1 : 0;
		bit1 = i & 0x04 ? 2 : 0;
		bit2 = i & 0x40 ? 4 : 0;
		int const g = bit0|bit1|bit2;

		palette.set_pen_color(i + 0x100, pal3bit(r), pal3bit(g), pal3bit(b));
	}
}

/* PIT8253 Interface */

void mz2500_state::pit8253_clk0_irq(int state)
{
	if(m_irq_mask[1] && state & 1)
	{
		m_irq_pending[1] = 1;
		m_maincpu->set_input_line(0, ASSERT_LINE);
	}
}

void mz2500_state::rtc_alarm_irq(int state)
{
	// TODO: doesn't work yet
//  if(m_irq_mask[3] && state & 1)
//      m_maincpu->set_input_line_and_vector(0, HOLD_LINE,drvm_irq_vector[3]); // Z80
}


static void mz2500_floppies(device_slot_interface &device)
{
	device.option_add("525dd", FLOPPY_525_DD);
	device.option_add("3ssdd", FLOPPY_3_SSDD);
	device.option_add("35dd", FLOPPY_35_DD);
}

static void mouse_devices(device_slot_interface &device)
{
	device.option_add("x68k", X68K_MOUSE);
}

template <unsigned N> TIMER_CALLBACK_MEMBER(mz2500_state::sio_clock_cb)
{
	if (N)
	{
		m_sio->rxtxcb_w(0);
		m_sio->rxtxcb_w(1);
	}
	else
	{
		m_sio->rxca_w(0);
		m_sio->rxca_w(1);
		m_sio->txca_w(0);
		m_sio->txca_w(1);
	}
}


static const z80_daisy_config daisy_chain[] =
{
//  { "pio" },
	{ "sio" },
	{ nullptr }
};

void mz2500_state::mz2500(machine_config &config)
{
	Z80(config, m_maincpu, 24_MHz_XTAL / 4);
	m_maincpu->set_daisy_config(daisy_chain);
	m_maincpu->set_addrmap(AS_PROGRAM, &mz2500_state::z80_map);
	m_maincpu->set_addrmap(AS_IO, &mz2500_state::z80_io);
	m_maincpu->set_vblank_int("screen", FUNC(mz2500_state::vblank_cb));
	m_maincpu->set_irq_acknowledge_callback(FUNC(mz2500_state::irq_ack_cb));

	for (int bank = 0; bank < 8; bank++)
	{
		ADDRESS_MAP_BANK(config, m_rambank[bank]).set_map(&mz2500_state::bank_window_map).set_options(ENDIANNESS_LITTLE, 8, 16+3, 0x2000);
	}

	i8255_device &ppi(I8255(config, "ppi"));
	ppi.in_pa_callback().set(FUNC(mz2500_state::ppi_porta_r));
	ppi.out_pa_callback().set(FUNC(mz2500_state::ppi_porta_w));
	ppi.in_pb_callback().set(FUNC(mz2500_state::ppi_portb_r));
	ppi.out_pb_callback().set(FUNC(mz2500_state::ppi_portb_w));
	ppi.in_pc_callback().set(FUNC(mz2500_state::ppi_portc_r));
	ppi.out_pc_callback().set(FUNC(mz2500_state::ppi_portc_w));

	z80pio_device& pio(Z80PIO(config, "pio", 24_MHz_XTAL / 4));
	pio.in_pa_callback().set(FUNC(mz2500_state::pio_porta_r));
	pio.out_pa_callback().set(FUNC(mz2500_state::pio_porta_w));
	pio.in_pb_callback().set(FUNC(mz2500_state::pio_porta_r));

	Z80SIO(config, m_sio, 24_MHz_XTAL / 4);

	RP5C15(config, m_rtc, 32.768_kHz_XTAL);
	m_rtc->alarm().set(FUNC(mz2500_state::rtc_alarm_irq));

	PIT8253(config, m_pit);
	m_pit->set_clk<0>(24_MHz_XTAL / 768);
	m_pit->out_handler<0>().set(FUNC(mz2500_state::pit8253_clk0_irq));
	m_pit->out_handler<0>().append(m_pit, FUNC(pit8253_device::write_clk1));
	m_pit->out_handler<1>().set(m_pit, FUNC(pit8253_device::write_clk2)); //CH2, used by Super MZ demo / The Black Onyx and a few others

	MB8876(config, m_fdc, 8_MHz_XTAL / 8); // clocked by MB4107 VFO

	MSX_GENERAL_PURPOSE_PORT(config, m_joy[0], msx_general_purpose_port_devices, "joystick");
	MSX_GENERAL_PURPOSE_PORT(config, m_joy[1], msx_general_purpose_port_devices, "joystick");

	rs232_port_device &mouse(RS232_PORT(config, "mouse_port", mouse_devices, "x68k"));
	mouse.rxd_handler().set(m_sio, FUNC(z80sio_device::rxb_w));
	m_sio->out_dtrb_callback().set(mouse, FUNC(rs232_port_device::write_rts));

	FLOPPY_CONNECTOR(config, "fdc:0", mz2500_floppies, "35dd", floppy_image_device::default_mfm_floppy_formats).enable_sound(true);
	FLOPPY_CONNECTOR(config, "fdc:1", mz2500_floppies, "35dd", floppy_image_device::default_mfm_floppy_formats).enable_sound(true);
	FLOPPY_CONNECTOR(config, "fdc:2", mz2500_floppies, nullptr, floppy_image_device::default_mfm_floppy_formats).enable_sound(true);
	FLOPPY_CONNECTOR(config, "fdc:3", mz2500_floppies, nullptr, floppy_image_device::default_mfm_floppy_formats).enable_sound(true);

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_raw(42.954545_MHz_XTAL / 2, 640+108, 0, 640, 480, 0, 200); //unknown clock / divider
	m_screen->set_screen_update(FUNC(mz2500_state::screen_update));
	m_screen->set_palette(m_palette);

	PALETTE(config, m_palette, FUNC(mz2500_state::palette_init), 0x200);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_mz2500);

	SPEAKER(config, "mono").front_center();

	ym2203_device &ym(YM2203(config, "ym", 24_MHz_XTAL / 12));
	ym.port_a_read_callback().set(FUNC(mz2500_state::opn_porta_r));
	ym.port_b_read_callback().set_ioport("DSW1");
	ym.port_a_write_callback().set(FUNC(mz2500_state::opn_porta_w));
	ym.add_route(0, "mono", 0.25);
	ym.add_route(1, "mono", 0.25);
	ym.add_route(2, "mono", 0.50);
	ym.add_route(3, "mono", 0.50);

	SPEAKER_SOUND(config, m_dac1bit).add_route(ALL_OUTPUTS, "mono", 0.50);

	// MZ-1U09, built-in 2 slots
	for (unsigned i = 0; i < 2; i++)
	{
		MZ80_EXP_SLOT(config, m_exp[i], mz2500_exp_devices, nullptr);
		m_exp[i]->set_iospace(m_maincpu, AS_IO);
	}

	SOFTWARE_LIST(config, "flop_list").set_original("mz2500_flop");
	SOFTWARE_LIST(config, "flop_list2").set_compatible("mz2000_flop");
}



ROM_START( mz2500 )
	ROM_REGION( 0x08000, "ipl", 0 )
	ROM_LOAD( "ipl.rom", 0x00000, 0x8000, CRC(7a659f20) SHA1(ccb3cfdf461feea9db8d8d3a8815f7e345d274f7) )

	// hand made?
	ROM_REGION( 0x1000, "cgrom", 0 )
	ROM_LOAD( "cg.rom", 0x0000, 0x0800, CRC(a082326f) SHA1(dfa1a797b2159838d078650801c7291fa746ad81) )

	ROM_REGION( 0x40000, "kanji", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x40000, CRC(dd426767) SHA1(cc8fae0cd1736bc11c110e1c84d3f620c5e35b80) )

	ROM_REGION( 0x20000, "kanji2", 0 )
	ROM_LOAD( "kanji2.rom", 0x0000, 0x20000, CRC(eaaf20c9) SHA1(771c4d559b5241390215edee798f3bce169d418c) )

	ROM_REGION( 0x40000, "dictionary", 0 )
	ROM_LOAD( "dict.rom", 0x00000, 0x40000, CRC(aa957c2b) SHA1(19a5ba85055f048a84ed4e8d471aaff70fcf0374) )

	ROM_REGION( 0x8000, "phone", ROMREGION_ERASEFF )
	ROM_LOAD( "phone.rom", 0x00000, 0x4000, CRC(8e49e4dc) SHA1(2589f0c95028037a41ca32a8fd799c5f085dab51) )
ROM_END

ROM_START( mz2520 )
	ROM_REGION( 0x08000, "ipl", 0 )
	ROM_LOAD( "ipl2520.rom", 0x00000, 0x8000, CRC(0a126eb2) SHA1(faf71236b3ad82d30184adea951d43d10ced663d) )

	// hand made?
	ROM_REGION( 0x1000, "cgrom", 0 )
	ROM_LOAD( "cg.rom", 0x0000, 0x0800, CRC(a082326f) SHA1(dfa1a797b2159838d078650801c7291fa746ad81) )

	ROM_REGION( 0x40000, "kanji", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x40000, CRC(dd426767) SHA1(cc8fae0cd1736bc11c110e1c84d3f620c5e35b80) )

	ROM_REGION( 0x20000, "kanji2", 0 )
	ROM_LOAD( "kanji2.rom", 0x0000, 0x20000, CRC(eaaf20c9) SHA1(771c4d559b5241390215edee798f3bce169d418c) )

	ROM_REGION( 0x40000, "dictionary", 0 )
	ROM_LOAD( "dict.rom", 0x00000, 0x40000, CRC(aa957c2b) SHA1(19a5ba85055f048a84ed4e8d471aaff70fcf0374) )

	ROM_REGION( 0x8000, "phone", ROMREGION_ERASEFF )
	ROM_LOAD( "phone.rom", 0x00000, 0x4000, CRC(8e49e4dc) SHA1(2589f0c95028037a41ca32a8fd799c5f085dab51) )
ROM_END



COMP( 1985, mz2500, 0,      0, mz2500, mz2500, mz2500_state, empty_init, "Sharp", "MZ-2500", MACHINE_IMPERFECT_GRAPHICS )
COMP( 1985, mz2520, mz2500, 0, mz2500, mz2500, mz2500_state, empty_init, "Sharp", "MZ-2520", MACHINE_IMPERFECT_GRAPHICS ) // stripped down version of the regular MZ-2500, no MZ-2000/MZ-80B compatability and no cassette interface
