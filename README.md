# ESFMu-fast

<p align=center>
  <img src="assets/logo.png" height="300px">
</p>

An emulator for the ESS "ESFM" enhanced OPL3 clone, based on Nuke.YKT's **Nuked OPL3** and reverse-engineering efforts from the community.

This is a performance-optimized fork of [ESFMu](https://github.com/Kagamiin/ESFMu). Its audio output is bit-for-bit identical to upstream; on real ESFM music it renders roughly 2.2x faster (x86-64, GCC -O2). The function interface is unchanged, so it is a drop-in replacement.

Changes relative to upstream:

- Write-time caches (`pg_inc`, `eg_tl_ksl`, `eg_ks` in `esfm_slot`): the non-vibrato phase increment, the total-level + KSL envelope offset and the key-scale rate shift are recomputed on register writes instead of on every sample.
- Interleaved feedback (`ESFM_process_feedback`): the per-channel inline-asm 29-iteration feedback loop was replaced with a C implementation that processes four channels in lockstep. The chain is serial through two dependent table loads per iteration, so interleaving independent channels gets around the load latency. This is where most of the speedup comes from.
- Silent-slot fast paths: `exprom`'s maximum entry is 0xff4 (< 2^12), so a slot with `eg_output >= 0x180` produces exactly zero output for any phase. Slot generation and the feedback loop skip the table lookups for such slots.
- Envelope fast paths (`ESFM_envelope_calc`): released slots (key off, envelope at 0x1ff, delay logic quiescent) and held sustain (key on, sustaining envelope, delay settled) skip the rate/state machine. The function is also specialized per chip mode.
- Batched noise LFSR: the LFSR is only read by slot-3 noise modes or the OPL emu rhythm slots. When nothing can read it, the 72 per-slot steps per sample are replaced with one precomputed 72-step jump.
- Smaller items: an early-out in `ESFM_update_write_buffer`, `__builtin_ctzll` for the envelope timer clocks, and a branch tremolo wrap.

Bit-perfect output is verified against upstream with register traces of four Furnace ESFM songs, as well as a deterministic register-write fuzzer covering emulation mode, native noise slots, mode switches, and both chip revisions.

## Acknowledgements

I'd like to thank:

- **Nuke.YKT**
  - Developer of **Nuked OPL3**, which was the basis for **ESFMu**'s code and also a great learning resource on Yamaha FM synthesis for myself.
  - Nuke.YKT also gives shoutouts on behalf of **Nuked OPL3** to:
    >- MAME Development Team(Jarek Burczynski, Tatsuyuki Satoh):
    >    - Feedback and Rhythm part calculation information.
    >- forums.submarine.org.uk(carbon14, opl3):
    >    - Tremolo and phase generator calculation information.
    >- OPLx decapsulated(Matthew Gambrell, Olli Niemitalo):
    >    - OPL2 ROMs.
    >- siliconpr0n.org(John McMaster, digshadow):
    >    - YMF262 and VRC VII decaps and die shots.
- **rainwarrior**
  - For performing the initial research on ESFM drivers and documenting ESS's patent on native mode operator organization.
- **jwt27**
  - For kickstarting the ESFM research project and compiling rainwarrior's findings and more in an accessible document ("ESFM Demystified").
- **pachuco/CatButts**
  - For documenting ESS's patent on ESFM's feedback implementation, which was vital in getting **ESFMu**'s sound output to be accurate.
- **akumanatt**
  - For helping out with code optimization.
- **shadex/shidzy13**
  - For helping out with the nifty logo above, thanks!
- And everybody who helped out with real hardware testing

## Usage

To use **ESFMu**:

- include the **esfm.h** header file into your source code
- include the **esfm.c** and **esfm_registers.c** files into your build and link process
- declare or allocate a variable of type `esfm_chip` somewhere in your code - this will hold the chip's state
- use the function interface defined in **esfm.h** to interact with the `esfm_chip` structure

## Function interface

If you're familiar with **Nuked OPL3**, you'll find many similarities in the function interface provided by **ESFMu**. There are a few things to point out, however:

### Buffered writes

Just like **Nuked OPL3**, **ESFMu** offers buffered register writes. However, it offers them in two flavors: "legacy" and fast.

The fast buffered register writes (`ESFM_write_reg_buffered_fast`) are recommended, since they offer minimal latency which is close to the behavior you'd get with the actual ESS drivers on Windows.

The "legacy" buffered register writes are only recommended for specific cases, such as programs seeking for a shortcut to emulate the write delays from some sound drivers.

### Port-level access

Unlike **Nuked OPL3**, **ESFMu** actually allows port-level access to the ESFM interface. This is relevant because the ESFM port interface is actually modal, meaning that its behavior changes depending on whether the chip is set to emulation (OPL3 compatibility) mode or native (ESFM) mode.

Using port-level access allows for applications to not need to keep track of whether the chip is in native mode or not, nor to perform the port handling logic on their side.

Applications that use the register-level access, on the other hand, need to take care to either stick to only one of the operating modes (either native or emulation), or handle the port mapping logic on their own side.

### Register readback

ESFM allows for register contents to be read back through its ports, and **ESFMu** implements this functionality, both via dedicated register read functions and via the port read interface.

Note that in ESFM, register contents can only be read back when the chip is set to native (ESFM) mode, not when the chip is in emulation mode (i.e. OPL3 compatibility mode).

## Licensing

**ESFMu** is highly based on **Nuked OPL3**, which is licensed under the GNU Lesser General Public License version 2.1 or later. Therefore, **ESFMu** is licensed under the same license.

If you'd like to obtain a grant to use **ESFMu** under different terms, you should get in contact with [Nuke.YKT](https://github.com/nukeykt) (author of **Nuked OPL3**) as well as with [Kagamiin~](https://github.com/Kagamiin) (yours truly).
