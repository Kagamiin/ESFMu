 
#include <stdint.h>


typedef struct _esfm_slot esfm_slot;
typedef struct _esfm_slot_internal esfm_slot_internal;
typedef struct _esfm_channel esfm_channel;
typedef struct _esfm_chip esfm_chip;

void ESFM_init (esfm_chip *chip);
void ESFM_write_reg (esfm_chip *chip, int16_t address, uint8_t data);
void ESFM_write_port (esfm_chip *chip, uint8_t offset, uint8_t data);
uint8_t ESFM_readback_reg (esfm_chip *chip, int16_t address);
uint8_t ESFM_read_port (esfm_chip *chip, uint8_t offset);


// These are fake types just for syntax sugar.
// Beware of their underlying types when reading/writing to them.
#ifndef __FAST_TYPES
#define __FAST_TYPES
#endif

#ifdef __FAST_TYPES
typedef uint_fast8_t bool;
typedef uint_fast8_t uint2;
typedef uint_fast8_t uint3;
typedef uint_fast8_t uint4;
typedef uint_fast8_t uint6;
typedef uint_fast8_t uint8;
typedef uint_fast16_t uint9;
typedef uint_fast16_t uint10;
typedef uint_fast16_t uint11;
typedef uint_fast16_t uint12;

typedef int_fast16_t int12;

#else
typedef uint8_t bool;
typedef uint8_t uint2;
typedef uint8_t uint3;
typedef uint8_t uint4;
typedef uint8_t uint6;
typedef uint8_t uint8;
typedef uint16_t uint9;
typedef uint16_t uint10;
typedef uint16_t uint11;
typedef uint16_t uint12;

typedef int16_t int12;

#endif


typedef struct _esfm_slot_internal
{
	uint9 eg_position;
	uint9 eg_ksl_offset;
	uint10 eg_output;
	
	uint4 keyscale;
	
	int12 output;
	int12 prev_output;
	int12 feedback_buf;
	int12 *mod_input;
	
	uint10 phase_acc;
	bool phase_reset;
	bool *key_on;

	uint2 eg_state;
	bool eg_delay_run;
	uint9 eg_delay_counter;
	
} esfm_slot_internal;

struct _esfm_slot
{
	// Metadata
	esfm_channel *channel;
	esfm_chip *chip;
	uint2 slot_idx;
	
	// Register data
	int12 out_enable[2];
	uint10 f_num;
	uint3 block;
	uint3 output_level;
	uint3 mod_in_level;
	
	uint6 t_level;
	uint4 mult;
	uint3 waveform;
	// Only for 4th slot
	uint2 rhy_noise;
	
	uint4 attack_rate;
	uint4 decay_rate;
	uint4 sustain_lvl;
	uint4 release_rate;
	
	bool tremolo_en;
	bool tremolo_deep;
	bool vibrato_en;
	bool vibrato_deep;
	bool env_sustaining;
	bool ksr;
	uint2 ksl;
	uint3 env_delay;
	
	// Internal state
	esfm_slot_internal internal;
};

struct _esfm_channel
{
	esfm_chip *chip;
	esfm_slot slots[4];
	int16_t output[2];
	bool key_on;
	bool emu_mode_4op_enable;
	// Only for channels 17 and 18
	bool key_on_2;
	bool emu_mode_4op_enable_2;
};

struct _esfm_chip
{
	esfm_channel channels[18];
	int32_t output_accm[2];
	uint_fast16_t addr_latch;
	
	bool emu_newmode;
	bool native_mode;
	
	bool keyscale_mode;
	
	// Global state
	uint64_t eg_timer;
	uint10 global_timer;
	uint8 eg_clocks;
	bool eg_tick;
	uint8 tremolo;
	uint8 tremolo_pos;
	
	uint8 timers[2];
	bool timer_enable[2];
	bool timer_mask[2];
	bool timer_overflow[2];
	bool irq_bit;
	
	bool test_bit_distort;
	bool test_bit_attenuate;
	bool test_bit_mute;
};
