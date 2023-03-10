/*
 * ESFMu: emulator for the ESS "ESFM" enhanced OPL3 clone
 * Copyright (C) 2023 Kagamiin~ and contributors
 * 
 * This file includes code and data from the Nuked OPL3 project, copyright (C)
 * 2013-2023 Nuke.YKT. Its usage, modification and redistribution is allowed
 * under the terms of the GNU Lesser General Public License version 2.1 or
 * later.
 * 
 * ESFMu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 2.1
 * of the License, or (at your option) any later version.
 *
 * ESFMu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with ESFMu. If not, see <https://www.gnu.org/licenses/>.
 */

#include "esfm.h"
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#define RSM_FRAC    10

/* Channel types */

enum {
	ch_2op = 0,
	ch_4op = 1,
	ch_4op2 = 2,
	ch_drum = 3
};

/* Envelope key types */

enum {
	egk_norm = 0x01,
	egk_drum = 0x02
};


/*
	logsin table
*/

static const uint16_t logsinrom[256] = {
	0x859, 0x6c3, 0x607, 0x58b, 0x52e, 0x4e4, 0x4a6, 0x471,
	0x443, 0x41a, 0x3f5, 0x3d3, 0x3b5, 0x398, 0x37e, 0x365,
	0x34e, 0x339, 0x324, 0x311, 0x2ff, 0x2ed, 0x2dc, 0x2cd,
	0x2bd, 0x2af, 0x2a0, 0x293, 0x286, 0x279, 0x26d, 0x261,
	0x256, 0x24b, 0x240, 0x236, 0x22c, 0x222, 0x218, 0x20f,
	0x206, 0x1fd, 0x1f5, 0x1ec, 0x1e4, 0x1dc, 0x1d4, 0x1cd,
	0x1c5, 0x1be, 0x1b7, 0x1b0, 0x1a9, 0x1a2, 0x19b, 0x195,
	0x18f, 0x188, 0x182, 0x17c, 0x177, 0x171, 0x16b, 0x166,
	0x160, 0x15b, 0x155, 0x150, 0x14b, 0x146, 0x141, 0x13c,
	0x137, 0x133, 0x12e, 0x129, 0x125, 0x121, 0x11c, 0x118,
	0x114, 0x10f, 0x10b, 0x107, 0x103, 0x0ff, 0x0fb, 0x0f8,
	0x0f4, 0x0f0, 0x0ec, 0x0e9, 0x0e5, 0x0e2, 0x0de, 0x0db,
	0x0d7, 0x0d4, 0x0d1, 0x0cd, 0x0ca, 0x0c7, 0x0c4, 0x0c1,
	0x0be, 0x0bb, 0x0b8, 0x0b5, 0x0b2, 0x0af, 0x0ac, 0x0a9,
	0x0a7, 0x0a4, 0x0a1, 0x09f, 0x09c, 0x099, 0x097, 0x094,
	0x092, 0x08f, 0x08d, 0x08a, 0x088, 0x086, 0x083, 0x081,
	0x07f, 0x07d, 0x07a, 0x078, 0x076, 0x074, 0x072, 0x070,
	0x06e, 0x06c, 0x06a, 0x068, 0x066, 0x064, 0x062, 0x060,
	0x05e, 0x05c, 0x05b, 0x059, 0x057, 0x055, 0x053, 0x052,
	0x050, 0x04e, 0x04d, 0x04b, 0x04a, 0x048, 0x046, 0x045,
	0x043, 0x042, 0x040, 0x03f, 0x03e, 0x03c, 0x03b, 0x039,
	0x038, 0x037, 0x035, 0x034, 0x033, 0x031, 0x030, 0x02f,
	0x02e, 0x02d, 0x02b, 0x02a, 0x029, 0x028, 0x027, 0x026,
	0x025, 0x024, 0x023, 0x022, 0x021, 0x020, 0x01f, 0x01e,
	0x01d, 0x01c, 0x01b, 0x01a, 0x019, 0x018, 0x017, 0x017,
	0x016, 0x015, 0x014, 0x014, 0x013, 0x012, 0x011, 0x011,
	0x010, 0x00f, 0x00f, 0x00e, 0x00d, 0x00d, 0x00c, 0x00c,
	0x00b, 0x00a, 0x00a, 0x009, 0x009, 0x008, 0x008, 0x007,
	0x007, 0x007, 0x006, 0x006, 0x005, 0x005, 0x005, 0x004,
	0x004, 0x004, 0x003, 0x003, 0x003, 0x002, 0x002, 0x002,
	0x002, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};

/*
	exp table
*/

static const uint16_t exprom[256] = {
	0x7fa, 0x7f5, 0x7ef, 0x7ea, 0x7e4, 0x7df, 0x7da, 0x7d4,
	0x7cf, 0x7c9, 0x7c4, 0x7bf, 0x7b9, 0x7b4, 0x7ae, 0x7a9,
	0x7a4, 0x79f, 0x799, 0x794, 0x78f, 0x78a, 0x784, 0x77f,
	0x77a, 0x775, 0x770, 0x76a, 0x765, 0x760, 0x75b, 0x756,
	0x751, 0x74c, 0x747, 0x742, 0x73d, 0x738, 0x733, 0x72e,
	0x729, 0x724, 0x71f, 0x71a, 0x715, 0x710, 0x70b, 0x706,
	0x702, 0x6fd, 0x6f8, 0x6f3, 0x6ee, 0x6e9, 0x6e5, 0x6e0,
	0x6db, 0x6d6, 0x6d2, 0x6cd, 0x6c8, 0x6c4, 0x6bf, 0x6ba,
	0x6b5, 0x6b1, 0x6ac, 0x6a8, 0x6a3, 0x69e, 0x69a, 0x695,
	0x691, 0x68c, 0x688, 0x683, 0x67f, 0x67a, 0x676, 0x671,
	0x66d, 0x668, 0x664, 0x65f, 0x65b, 0x657, 0x652, 0x64e,
	0x649, 0x645, 0x641, 0x63c, 0x638, 0x634, 0x630, 0x62b,
	0x627, 0x623, 0x61e, 0x61a, 0x616, 0x612, 0x60e, 0x609,
	0x605, 0x601, 0x5fd, 0x5f9, 0x5f5, 0x5f0, 0x5ec, 0x5e8,
	0x5e4, 0x5e0, 0x5dc, 0x5d8, 0x5d4, 0x5d0, 0x5cc, 0x5c8,
	0x5c4, 0x5c0, 0x5bc, 0x5b8, 0x5b4, 0x5b0, 0x5ac, 0x5a8,
	0x5a4, 0x5a0, 0x59c, 0x599, 0x595, 0x591, 0x58d, 0x589,
	0x585, 0x581, 0x57e, 0x57a, 0x576, 0x572, 0x56f, 0x56b,
	0x567, 0x563, 0x560, 0x55c, 0x558, 0x554, 0x551, 0x54d,
	0x549, 0x546, 0x542, 0x53e, 0x53b, 0x537, 0x534, 0x530,
	0x52c, 0x529, 0x525, 0x522, 0x51e, 0x51b, 0x517, 0x514,
	0x510, 0x50c, 0x509, 0x506, 0x502, 0x4ff, 0x4fb, 0x4f8,
	0x4f4, 0x4f1, 0x4ed, 0x4ea, 0x4e7, 0x4e3, 0x4e0, 0x4dc,
	0x4d9, 0x4d6, 0x4d2, 0x4cf, 0x4cc, 0x4c8, 0x4c5, 0x4c2,
	0x4be, 0x4bb, 0x4b8, 0x4b5, 0x4b1, 0x4ae, 0x4ab, 0x4a8,
	0x4a4, 0x4a1, 0x49e, 0x49b, 0x498, 0x494, 0x491, 0x48e,
	0x48b, 0x488, 0x485, 0x482, 0x47e, 0x47b, 0x478, 0x475,
	0x472, 0x46f, 0x46c, 0x469, 0x466, 0x463, 0x460, 0x45d,
	0x45a, 0x457, 0x454, 0x451, 0x44e, 0x44b, 0x448, 0x445,
	0x442, 0x43f, 0x43c, 0x439, 0x436, 0x433, 0x430, 0x42d,
	0x42a, 0x428, 0x425, 0x422, 0x41f, 0x41c, 0x419, 0x416,
	0x414, 0x411, 0x40e, 0x40b, 0x408, 0x406, 0x403, 0x400
};

/*
	freq mult table multiplied by 2

	1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15
*/

static const uint8_t mt[16] = {
	1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30
};


static const uint8_t kslshift[4] = {
	8, 1, 2, 0
};

/*
	envelope generator constants
*/

static const uint8_t eg_incstep[4][4] = {
	{ 0, 0, 0, 0 },
	{ 1, 0, 0, 0 },
	{ 1, 0, 1, 0 },
	{ 1, 1, 1, 0 }
};

/*
	Envelope generator
*/

typedef int12(*envelope_sinfunc)(uint10 phase, uint10 envelope);

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_exp(uint16 level)
{
	if (level > 0x1fff)
	{
		level = 0x1fff;
	}
	return (exprom[level & 0xff] << 1) >> (level >> 8);
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin0(uint10 phase, uint10 envelope)
{
	uint16 out = 0;
	uint16 neg = 0;
	phase &= 0x3ff;
	if (phase & 0x200)
	{
		neg = 0xffff;
	}
	if (phase & 0x100)
	{
		out = logsinrom[(phase & 0xff) ^ 0xff];
	}
	else
	{
		out = logsinrom[phase & 0xff];
	}
	return ESFM_envelope_calc_exp(out + (envelope << 3)) ^ neg;
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin1(uint10 phase, uint10 envelope)
{
	uint16 out = 0;
	phase &= 0x3ff;
	if (phase & 0x200)
	{
		out = 0x1000;
	}
	else if (phase & 0x100)
	{
		out = logsinrom[(phase & 0xff) ^ 0xff];
	}
	else
	{
		out = logsinrom[phase & 0xff];
	}
	return ESFM_envelope_calc_exp(out + (envelope << 3));
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin2(uint10 phase, uint10 envelope)
{
	uint16 out = 0;
	phase &= 0x3ff;
	if (phase & 0x100)
	{
		out = logsinrom[(phase & 0xff) ^ 0xff];
	}
	else
	{
		out = logsinrom[phase & 0xff];
	}
	return ESFM_envelope_calc_exp(out + (envelope << 3));
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin3(uint10 phase, uint10 envelope)
{
	uint16 out = 0;
	phase &= 0x3ff;
	if (phase & 0x100)
	{
		out = 0x1000;
	}
	else
	{
		out = logsinrom[phase & 0xff];
	}
	return ESFM_envelope_calc_exp(out + (envelope << 3));
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin4(uint10 phase, uint10 envelope)
{
	uint16 out = 0;
	uint16 neg = 0;
	phase &= 0x3ff;
	if ((phase & 0x300) == 0x100)
	{
		neg = 0xffff;
	}
	if (phase & 0x200)
	{
		out = 0x1000;
	}
	else if (phase & 0x80)
	{
		out = logsinrom[((phase ^ 0xff) << 1) & 0xff];
	}
	else
	{
		out = logsinrom[(phase << 1) & 0xff];
	}
	return ESFM_envelope_calc_exp(out + (envelope << 3)) ^ neg;
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin5(uint10 phase, uint10 envelope)
{
	uint16 out = 0;
	phase &= 0x3ff;
	if (phase & 0x200)
	{
		out = 0x1000;
	}
	else if (phase & 0x80)
	{
		out = logsinrom[((phase ^ 0xff) << 1) & 0xff];
	}
	else
	{
		out = logsinrom[(phase << 1) & 0xff];
	}
	return ESFM_envelope_calc_exp(out + (envelope << 3));
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin6(uint10 phase, uint10 envelope)
{
	uint16 neg = 0;
	phase &= 0x3ff;
	if (phase & 0x200)
	{
		neg = 0xffff;
	}
	return ESFM_envelope_calc_exp(envelope << 3) ^ neg;
}

/* ------------------------------------------------------------------------- */
static int12
ESFM_envelope_calc_sin7(uint10 phase, uint10 envelope)
{
	uint16 out = 0;
	uint16 neg = 0;
	phase &= 0x3ff;
	if (phase & 0x200)
	{
		neg = 0xffff;
		phase = (phase & 0x1ff) ^ 0x1ff;
	}
	out = phase << 3;
	return ESFM_envelope_calc_exp(out + (envelope << 3)) ^ neg;
}

/* ------------------------------------------------------------------------- */
static const envelope_sinfunc envelope_sin[8] =
{
	ESFM_envelope_calc_sin0,
	ESFM_envelope_calc_sin1,
	ESFM_envelope_calc_sin2,
	ESFM_envelope_calc_sin3,
	ESFM_envelope_calc_sin4,
	ESFM_envelope_calc_sin5,
	ESFM_envelope_calc_sin6,
	ESFM_envelope_calc_sin7
};

/* ------------------------------------------------------------------------- */
static void
ESFM_envelope_calc(esfm_slot *slot)
{
	uint8 nonzero;
	uint8 rate;
	uint2 rate_hi;
	uint2 rate_lo;
	uint4 reg_rate = 0;
	uint4 ks;
	uint8 eg_shift, shift;
	bool eg_off;
	uint9 eg_rout;
	int16 eg_inc;
	bool reset = 0;
	slot->in.eg_output = slot->in.eg_position + (slot->t_level << 2)
		+ (slot->in.eg_ksl_offset >> kslshift[slot->ksl]);
	if (slot->tremolo_en)
	{
		uint8 tremolo;
		tremolo = slot->channel->chip->tremolo >> ((!slot->tremolo_deep << 1) + 2);
		slot->in.eg_output += tremolo;
	}
	if (*slot->in.key_on && slot->in.eg_state == EG_RELEASE)
	{
		if (!slot->in.eg_delay_run)
		{
			slot->in.eg_delay_run = 1;
			slot->in.eg_delay_counter = slot->env_delay ? 0x100 : 0;
		}
		
		if (slot->in.eg_delay_counter == 0)
		{
			slot->in.eg_delay_run = 0;
			reset = 1;
			reg_rate = slot->attack_rate;
		}
		else
		{
			if (slot->chip->global_timer & (1 << slot->env_delay))
			{
				slot->in.eg_delay_counter--;
			}
			reg_rate = slot->release_rate;
		}
	}
	else
	{
		switch (slot->in.eg_state)
		{
			case EG_ATTACK:
				reg_rate = slot->attack_rate;
				break;
			case EG_DECAY:
				reg_rate = slot->decay_rate;
				break;
			case EG_SUSTAIN:
				if (!slot->env_sustaining)
				{
					reg_rate = slot->release_rate;
				}
				break;
			case EG_RELEASE:
				reg_rate = slot->release_rate;
				break;
		}
	}
	slot->in.phase_reset = reset;
	ks = slot->in.keyscale >> (!slot->ksr << 1);
	nonzero = (reg_rate != 0);
	rate = ks + (reg_rate << 2);
	rate_hi = rate >> 2;
	rate_lo = rate & 0x03;
	if (rate_hi & 0x10)
	{
		rate_hi = 0x0f;
	}
	eg_shift = rate_hi + slot->chip->eg_clocks;
	shift = 0;
	if (nonzero)
	{
		if (rate_hi < 12)
		{
			if (slot->chip->eg_tick)
			{
				switch (eg_shift)
				{
					case 12:
						shift = 1;
						break;
					case 13:
						shift = (rate_lo >> 1) & 0x01;
						break;
					case 14:
						shift = rate_lo & 0x01;
						break;
					default:
						break;
				}
			}
		}
		else
		{
			shift = (rate_hi & 0x03) 
				+ eg_incstep[rate_lo][slot->chip->global_timer & 0x03];
			if (shift & 0x04)
			{
				shift = 0x03;
			}
			if (!shift)
			{
				shift = slot->chip->eg_tick;
			}
		}
	}
	eg_rout = slot->in.eg_position;
	eg_inc = 0;
	eg_off = 0;
	/* Instant attack */
	if (reset && rate_hi == 0x0f)
	{
		eg_rout = 0x00;
	}
	/* Envelope off */
	if ((slot->in.eg_position & 0x1f8) == 0x1f8)
	{
		eg_off = 1;
	}
	if (slot->in.eg_state != EG_ATTACK && !reset && eg_off)
	{
		eg_rout = 0x1ff;
	}
	switch (slot->in.eg_state)
	{
		case EG_ATTACK:
			if (slot->in.eg_position == 0)
			{
				slot->in.eg_state = EG_DECAY;
			}
			else if (*slot->in.key_on && shift > 0 && rate_hi != 0x0f)
			{
				eg_inc = ~slot->in.eg_position >> (4 - shift);
			}
			break;
		case EG_DECAY:
			if ((slot->in.eg_position >> 4) == slot->sustain_lvl)
			{
				slot->in.eg_state = EG_SUSTAIN;
			}
			else if (!eg_off && !reset && shift > 0)
			{
				eg_inc = 1 << (shift - 1);
			}
			break;
		case EG_SUSTAIN:
		case EG_RELEASE:
			if (!eg_off && !reset && shift > 0)
			{
				eg_inc = 1 << (shift - 1);
			}
			break;
	}
	slot->in.eg_position = (eg_rout + eg_inc) & 0x1ff;
	/* Key off */
	if (reset)
	{
		slot->in.eg_state = EG_ATTACK;
	}
	if (!*slot->in.key_on)
	{
		slot->in.eg_state = EG_RELEASE;
		slot->in.eg_delay_run = 0;
	}
}

/* ------------------------------------------------------------------------- */
static void
ESFM_phase_generate(esfm_slot *slot)
{
	esfm_chip *chip;
	uint10 f_num;
	uint_fast32_t basefreq;
	bool rm_xor, n_bit;
	uint23 noise;
	uint10 phase;

	chip = slot->chip;
	f_num = slot->f_num;
	if (slot->vibrato_en)
	{
		int8_t range;
		uint8_t vibpos;

		range = (f_num >> 7) & 7;
		vibpos = slot->chip->vibrato_pos;

		if (!(vibpos & 3))
		{
			range = 0;
		}
		else if (vibpos & 1)
		{
			range >>= 1;
		}
		range >>= !slot->vibrato_deep;

		if (vibpos & 4)
		{
			range = -range;
		}
		f_num += range;
	}
	basefreq = (f_num << slot->block) >> 1;
	phase = (uint10)(slot->in.phase_acc >> 9);
	if (slot->in.phase_reset)
	{
		slot->in.phase_acc = 0;
	}
	slot->in.phase_acc += (basefreq * mt[slot->mult]) >> 1;
	slot->in.phase_acc &= (1 << 19) - 1;
	slot->in.phase_out = phase;
	/* Noise mode (rhythm) sounds */
	noise = chip->lfsr;
	if (slot->slot_idx == 3 && slot->rhy_noise)
	{
		esfm_slot *prev_slot = &slot->channel->slots[2];
		
		chip->rm_hh_bit2 = (phase >> 2) & 1;
		chip->rm_hh_bit3 = (phase >> 3) & 1;
		chip->rm_hh_bit7 = (phase >> 7) & 1;
		chip->rm_hh_bit8 = (phase >> 8) & 1;
		
		chip->rm_tc_bit3 = (prev_slot->in.phase_out >> 3) & 1;
		chip->rm_tc_bit5 = (prev_slot->in.phase_out >> 5) & 1;
		
		rm_xor = (chip->rm_hh_bit2 ^ chip->rm_hh_bit7)
		       | (chip->rm_hh_bit3 ^ chip->rm_tc_bit5)
		       | (chip->rm_tc_bit3 ^ chip->rm_tc_bit5);
		
		switch(slot->rhy_noise)
		{
			case 1:
				// SD
				slot->in.phase_out = (chip->rm_hh_bit8 << 9)
					| ((chip->rm_hh_bit8 ^ (noise & 1)) << 8);
				break;
			case 2:
				// HH
				slot->in.phase_out = rm_xor << 9;
				if (rm_xor ^ (noise & 1))
				{
					slot->in.phase_out |= 0xd0;
				}
				else
				{
					slot->in.phase_out |= 0x34;
				}
				break;
			case 3:
				// TC
				slot->in.phase_out = (rm_xor << 9) | 0x80;
				break;
		}
	}

	n_bit = ((noise >> 14) ^ noise) & 0x01;
	chip->lfsr = (noise >> 1) | (n_bit << 22);
}

/* ------------------------------------------------------------------------- */
static void
ESFM_slot_generate(esfm_slot *slot)
{
	envelope_sinfunc wavegen = envelope_sin[slot->waveform];
	int16 phase = slot->in.phase_out;
	if (slot->mod_in_level)
	{
		phase += *slot->in.mod_input >> (7 - slot->mod_in_level);
	}
	slot->in.output = wavegen((uint10)(phase & 0x3ff), slot->in.eg_output);
	if (slot->output_level)
	{
		int12 output_value = slot->in.output >> (7 - slot->output_level);
		slot->channel->output[0] += output_value & slot->out_enable[0];
		slot->channel->output[1] += output_value & slot->out_enable[1];
	}
}

/* ------------------------------------------------------------------------- */
static void
ESFM_slot_calc_feedback(esfm_slot *slot)
{
	slot->in.feedback_buf = (slot->in.output + slot->in.prev_output) >> 2;
	slot->in.prev_output = slot->in.output;
}

/* ------------------------------------------------------------------------- */
static void
ESFM_process_channel(esfm_channel *channel)
{
	int slot_idx;
	channel->output[0] = channel->output[1] = 0;
	ESFM_slot_calc_feedback(&channel->slots[0]);
	for (slot_idx = 0; slot_idx < 4; slot_idx++)
	{
		esfm_slot *slot = &channel->slots[slot_idx];
		ESFM_envelope_calc(slot);
		ESFM_phase_generate(slot);
		ESFM_slot_generate(slot);
	}
}

/* ------------------------------------------------------------------------- */
static int16_t
ESFM_clip_sample(int32 sample)
{
	if (sample > 32767)
	{
		sample = 32767;
	}
	else if (sample < -32768)
	{
		sample = -32768;
	}
	return (int16_t)sample;
}

/* ------------------------------------------------------------------------- */
static void
ESFM_update_timers(esfm_chip *chip)
{
	// Tremolo
	if ((chip->global_timer & 0x3f) == 0x3f)
	{
		chip->tremolo_pos = (chip->tremolo_pos + 1) % 210;
		if (chip->tremolo_pos < 105)
		{
			chip->tremolo = chip->tremolo_pos;
		}
		else
		{
			chip->tremolo = (210 - chip->tremolo_pos);
		}
	}
	
	// Vibrato
	if ((chip->global_timer & 0x3ff) == 0x3ff)
	{
		chip->vibrato_pos = (chip->vibrato_pos + 1) & 0x07;
	}
	
	chip->global_timer = (chip->global_timer + 1) & 0x3ff;
	
	// Envelope generator dither clocks
	chip->eg_clocks = 0;
	if (chip->eg_timer)
	{
		uint8 shift = 0;
		while (shift < 36 && ((chip->eg_timer >> shift) & 1) == 0)
		{
			shift++;
		}
		
		if (shift <= 12)
		{
			chip->eg_clocks = shift + 1;
		}
	}
	
	if (chip->eg_tick || chip->eg_timer_overflow)
	{
		if (chip->eg_timer == (1llu << 36) - 1)
		{
			chip->eg_timer = 0;
			chip->eg_timer_overflow = 1;
		}
		else
		{
			chip->eg_timer++;
			chip->eg_timer_overflow = 0;
		}
	}
	
	chip -> eg_tick ^= 1;
}

/* ------------------------------------------------------------------------- */
void
ESFM_generate(esfm_chip *chip, int16_t *buf)
{
	int channel_idx;
	
	chip->output_accm[0] = chip->output_accm[1] = 0;
	for (channel_idx = 0; channel_idx < 18; channel_idx++)
	{
		esfm_channel *channel = &chip->channels[channel_idx];
		ESFM_process_channel(channel);
		chip->output_accm[0] += channel->output[0];
		chip->output_accm[1] += channel->output[1];
	}
	
	buf[0] = ESFM_clip_sample(chip->output_accm[0]);
	buf[1] = ESFM_clip_sample(chip->output_accm[1]);
	
	ESFM_update_timers(chip);
}

/* ------------------------------------------------------------------------- */
void
ESFM_generate_stream(esfm_chip *chip, int16_t *sndptr, uint32_t num_samples)
{
	uint32_t i;
	
	for (i = 0; i < num_samples; i++)
	{
		ESFM_generate(chip, sndptr);
		sndptr += 2;
	}
}
