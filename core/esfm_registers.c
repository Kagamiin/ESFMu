/*
 * ESFMu: emulator for the ESS "ESFM" enhanced OPL3 clone
 * Copyright (C) 2023 Kagamiin~ and contributors
 * 
 * ---------------------------------------------------------------------------
 * 
 * esfm_register.c - functions for handling register and port writes and reads
 * 
 * ---------------------------------------------------------------------------
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
#include <stdlib.h>
#include <stddef.h>
#include <string.h>


/*
	ksl table
*/

static const int16 kslrom[16] = {
	0, 32, 40, 45, 48, 51, 53, 55, 56, 58, 59, 60, 61, 62, 63, 64
};

/* ------------------------------------------------------------------------- */
static void
ESFM_envelope_update_ksl(esfm_slot *slot)
{
	uint9 ksl = (kslrom[slot->f_num >> 6] << 2) - ((0x08 - slot->block) << 5);
	if (ksl < 0)
	{
		ksl = 0;
	}
	slot->in.eg_ksl_offset = ksl;
}

/* ------------------------------------------------------------------------- */
static inline uint8_t
ESFM_slot_readback (esfm_slot *slot, uint8_t register_idx)
{
	uint8_t data = 0;
	switch (register_idx & 0x07)
	{
		case 0x00:
			data |= (slot->tremolo_en != 0) << 7;
			data |= (slot->vibrato_en != 0) << 6;
			data |= (slot->env_sustaining != 0) << 5;
			data |= (slot->vibrato_en != 0) << 4;
			data |= slot->mult & 0x0f;
			break;
		case 0x01:
			data |= slot->ksl << 6;
			data |= slot->t_level & 0x3f;
			ESFM_envelope_update_ksl(slot);
			break;
		case 0x02:
			data |= slot->attack_rate << 4;
			data |= slot->decay_rate & 0x0f;
			break;
		case 0x03:
			data |= slot->sustain_lvl << 4;
			data |= slot->release_rate & 0x0f;
			break;
		case 0x04:
			data = slot->f_num & 0xff;
			break;
		case 0x05:
			data |= slot->env_delay << 5;
			data |= (slot->block & 0x07) << 2;
			data |= (slot->f_num >> 8) & 0x03;
			break;
		case 0x06:
			data |= (slot->tremolo_deep != 0) << 7;
			data |= (slot->vibrato_deep != 0) << 6;
			data |= (slot->out_enable[0] != 0) << 5;
			data |= (slot->out_enable[1] != 0) << 4;
			data |= (slot->mod_in_level & 0x07) << 1;
			break;
		case 0x07:
			data |= slot->output_level << 5;
			data |= (slot->rhy_noise & 0x03) << 3;
			data |= slot->waveform & 0x07;
			break;
	}
	return data;
}

/* ------------------------------------------------------------------------- */
static inline void
ESFM_slot_write (esfm_slot *slot, uint8_t register_idx, uint8_t data)
{
	switch (register_idx & 0x07)
	{
		case 0x00:
			slot->tremolo_en = (data & 0x80) != 0;
			slot->vibrato_en = (data & 0x40) != 0;
			slot->env_sustaining = (data & 0x20) != 0;
			slot->ksr = (data & 0x10) != 0;
			slot->mult = data & 0x0f;
			break;
		case 0x01:
			slot->ksl = data >> 6;
			slot->t_level = data & 0x3f;
			break;
		case 0x02:
			slot->attack_rate = data >> 4;
			slot->decay_rate = data & 0x0f;
			break;
		case 0x03:
			slot->sustain_lvl = data >> 4;
			slot->release_rate = data & 0x0f;
			break;
		case 0x04:
			slot->f_num = (slot->f_num & 0x300) | data;
			break;
		case 0x05:
			slot->env_delay = data >> 5;
			slot->block = (data >> 2) & 0x07;
			slot->f_num = (slot->f_num & 0xff) | ((data & 0x03) << 8);
			break;
		case 0x06:
			slot->tremolo_deep = (data & 0x80) != 0;
			slot->vibrato_deep = (data & 0x40) != 0;
			slot->out_enable[0] = (data & 0x20) ? ~((int12) 0) : 0;
			slot->out_enable[1] = (data & 0x10) ? ~((int12) 0) : 0;
			slot->mod_in_level = (data >> 1) & 0x07;
			break;
		case 0x07:
			slot->output_level = data >> 5;
			slot->rhy_noise = (data >> 3) & 0x03;
			slot->waveform = data & 0x07;
			break;
	}
}

#define KEY_ON_REGS_START (18 * 4 * 8)
#define TIMER1_REG (0x402)
#define TIMER2_REG (0x403)
#define TIMER_SETUP_REG (0x404)
#define CONFIG_REG (0x408)
#define TEST_REG (0x501)

/* ------------------------------------------------------------------------- */
static void
ESFM_write_reg_native (esfm_chip *chip, int16_t address, uint8_t data)
{
	address = address & 0x7ff;
	
	if (address < KEY_ON_REGS_START)
	{
		// Slot register write
		size_t channel_idx = address >> 5;
		size_t slot_idx = (address >> 3) & 0x03;
		size_t register_idx = address & 0x07;
		esfm_slot *slot = &chip->channels[channel_idx].slots[slot_idx];
		
		ESFM_slot_write(slot, register_idx, data);
	}
	else if (address < KEY_ON_REGS_START + 16)
	{
		// Key-on registers
		size_t channel_idx = (address - KEY_ON_REGS_START);
		esfm_channel *channel = &chip->channels[channel_idx];
		channel->key_on = data & 0x01;
		channel->emu_mode_4op_enable = (data & 0x02) != 0;
	}
	else if (address < KEY_ON_REGS_START + 20)
	{
		// Key-on channels 17 and 18 (each half)
		size_t channel_idx = 16 + address & 0x01;
		bool second_half = address & 0x03;
		esfm_channel *channel = &chip->channels[channel_idx];
		if (second_half)
		{
			channel->key_on_2 = data & 0x01;
			channel->emu_mode_4op_enable_2 = (data & 0x02) != 0;
		}
		else
		{
			channel->key_on = data & 0x01;
			channel->emu_mode_4op_enable = (data & 0x02) != 0;
		}
	}
	else
	{
		switch (address & 0x5ff)
		{
			case TIMER1_REG:
				chip->timers[0] = data;
				break;
			case TIMER2_REG:
				chip->timers[1] = data;
				break;
			case TIMER_SETUP_REG:
				if (data & 0x80)
				{
					chip->timer_overflow[0] = 0;
					chip->timer_overflow[1] = 0;
					chip->irq_bit = 0;
				}
				chip->timer_enable[0] = (data & 0x01) != 0;
				chip->timer_enable[1] = (data & 0x02) != 0;
				chip->timer_mask[0] = (data & 0x20) != 0;
				chip->timer_mask[1] = (data & 0x40) != 0;
				break;
			case CONFIG_REG:
				chip->keyscale_mode = (data & 0x40) != 0;
				break;
			case TEST_REG:
				chip->test_bit_distort = (data & 0x02) != 0;
				chip->test_bit_attenuate = (data & 0x10) != 0;
				chip->test_bit_mute = (data & 0x40) != 0;
				break;
		}
	}
}

/* ------------------------------------------------------------------------- */
static uint8_t
ESFM_readback_reg_native (esfm_chip *chip, int16_t address)
{
	uint8_t data = 0;
	address = address & 0x7ff;
	
	if (address < KEY_ON_REGS_START)
	{
		// Slot register read
		size_t channel_idx = address >> 5;
		size_t slot_idx = (address >> 3) & 0x03;
		size_t register_idx = address & 0x07;
		esfm_slot *slot = &chip->channels[channel_idx].slots[slot_idx];
		
		data = ESFM_slot_readback(slot, register_idx);
	}
	else if (address < KEY_ON_REGS_START + 16)
	{
		// Key-on registers
		size_t channel_idx = (address - KEY_ON_REGS_START);
		esfm_channel *channel = &chip->channels[channel_idx];
		
		data |= channel->key_on != 0;
		data |= (channel->emu_mode_4op_enable != 0) << 1;
	}
	else if (address < KEY_ON_REGS_START + 20)
	{
		// Key-on channels 17 and 18 (each half)
		size_t channel_idx = 16 + address & 0x03;
		bool second_half = address & 0x01;
		esfm_channel *channel = &chip->channels[channel_idx];
		if (second_half)
		{
			data |= channel->key_on_2 != 0;
			data |= (channel->emu_mode_4op_enable_2 != 0) << 1;
		}
		else
		{
			data |= channel->key_on != 0;
			data |= (channel->emu_mode_4op_enable != 0) << 1;
		}
	}
	else
	{
		switch (address & 0x5ff)
		{
			case TIMER1_REG:
				data = chip->timers[0];
				break;
			case TIMER2_REG:
				data = chip->timers[1];
				break;
			case TIMER_SETUP_REG:
				data |= chip->timer_enable[0] != 0;
				data |= (chip->timer_enable[1] != 0) << 1;
				data |= (chip->timer_mask[0] != 0) << 5;
				data |= (chip->timer_mask[1] != 0) << 6;
				break;
			case CONFIG_REG:
				data |= (chip->keyscale_mode != 0) << 6;
				break;
			case TEST_REG:
				data |= (chip->test_bit_distort != 0) << 1;
				data |= (chip->test_bit_attenuate != 0) << 4;
				data |= (chip->test_bit_mute != 0) << 6;
				break;
		}
	}
	return data;
}

/* ------------------------------------------------------------------------- */
static void
ESFM_write_reg_emu (esfm_chip *chip, int16_t address, uint8_t data)
{
	bool high = (address & 0x100) != 0;
	bool reg = address & 0xff;
	
	switch(reg & 0xf0)
	{
		case 0x00:
			if (high)
			{
				switch(reg & 0x0f)
				{
				case 0x04:
					// set 4-op channel bits
					break;
				case 0x05:
					chip->emu_newmode = data & 0x01;
					chip->native_mode = (data & 0x80) != 0;
					break;
				}
			}
			else
			{
				switch(reg & 0x0f)
				{
				case 0x08:
					chip->keyscale_mode = (data & 0x40) != 0;
				}
			}
	}
}

/* ------------------------------------------------------------------------- */
void
ESFM_write_reg (esfm_chip *chip, int16_t address, uint8_t data)
{
	if (chip->native_mode)
	{
		return ESFM_write_reg_native(chip, address, data);
	}
	else
	{
		return ESFM_write_reg_emu(chip, address, data);
	}
}

/* ------------------------------------------------------------------------- */
uint8_t
ESFM_readback_reg (esfm_chip *chip, int16_t address)
{
	if (chip->native_mode)
	{
		return ESFM_readback_reg_native(chip, address);
	}
	else
	{
		//return ESFM_readback_reg_emu(chip, address);
		return 0;
	}
}

/* ------------------------------------------------------------------------- */
void
ESFM_write_port (esfm_chip *chip, uint8_t offset, uint8_t data)
{
	if (chip->native_mode)
	{
		switch(offset)
		{
			case 0:
				chip->native_mode = 0;
				break;
			case 1:
				ESFM_write_reg_native(chip, chip->addr_latch, data);
				break;
			case 2:
				chip->addr_latch = (chip->addr_latch & 0xff00) | data;
				break;
			case 3:
				chip->addr_latch = chip->addr_latch & 0xff;
				chip->addr_latch |= (uint_fast16_t)data << 8;
				break;
		}
	}
	else
	{
		switch(offset)
		{
			case 0:
				chip->addr_latch = chip->addr_latch & 0xff;
				chip->addr_latch |= (uint_fast16_t)data << 8;
				break;
			case 1:
				ESFM_write_reg_emu(chip, chip->addr_latch >> 8, data);
				break;
			case 2:
				chip->addr_latch = (chip->addr_latch & 0xff00) | data;
				break;
			case 3:
				ESFM_write_reg_emu(chip, (chip->addr_latch & 0xff) | 0x100, data);
				break;
		}
	}
}

/* ------------------------------------------------------------------------- */
uint8_t
ESFM_read_port (esfm_chip *chip, uint8_t offset)
{
	uint8_t data = 0;
	if (chip->native_mode)
	{
		switch(offset)
		{
			case 0:
				data |= (chip->irq_bit != 0) << 7;
				data |= (chip->timer_overflow[0] != 0) << 6;
				data |= (chip->timer_overflow[1] != 0) << 5;
				break;
			case 1:
				data = ESFM_readback_reg_native(chip, chip->addr_latch);
				break;
		}
	}
	else
	{
		switch(offset)
		{
			case 0:
				data |= (chip->irq_bit != 0) << 7;
				data |= (chip->timer_overflow[0] != 0) << 6;
				data |= (chip->timer_overflow[1] != 0) << 5;
				break;
			case 1:
				//ESFM_readback_reg_emu(chip, chip->addr_latch >> 8);
				break;
			case 3:
				//ESFM_readback_reg_emu(chip, (chip->addr_latch & 0xff) | 0x100);
				break;
		}
	}
	return data;
}

/* ------------------------------------------------------------------------- */
void
ESFM_init (esfm_chip *chip)
{
	esfm_slot *slot;
	esfm_channel *channel;
	size_t channel_idx, slot_idx;
	
	memset(chip, 0, sizeof(esfm_chip));
	for (channel_idx = 0; channel_idx < 18; channel_idx++)
	{
		for (slot_idx = 0; slot_idx < 4; slot_idx++)
		{
			channel = &chip->channels[channel_idx];
			slot = &channel->slots[slot_idx];
			
			channel->chip = chip;
			slot->channel = channel;
			slot->chip = chip;
			slot->slot_idx = slot_idx;
			slot->in.eg_position = slot->in.eg_output = 0x1ff;
			slot->in.eg_state = EG_RELEASE;
			if (slot_idx == 0)
			{
				slot->in.mod_input = &slot->in.feedback_buf;
			}
			else
			{
				esfm_slot *prev_slot = &channel->slots[slot_idx - 1];
				slot->in.mod_input = &prev_slot->in.output;
			}
			
			if (channel_idx > 15 && slot_idx & 0x02)
			{
				slot->in.key_on = &channel->key_on_2;
			}
			else
			{
				slot->in.key_on = &channel->key_on;
			}
			
			slot->out_enable[0] = slot->out_enable[1] = ~((int12) 0);
		}
	}
	
	chip->lfsr = 1;
}

