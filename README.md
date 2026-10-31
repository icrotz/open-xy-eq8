# Graphic EQ (`eq8`)

An 8-band graphic equaliser effect for the Teenage Engineering **OP-XY**, built
with the [Open-XY SDK](https://github.com/etcetc000/open-xy). Eight fixed
frequency bands, each a clean ±24 dB boost or cut — a fast, surgical tone-shaper
you can drop on any track, bus, or the main output.

## How to use

Load **Graphic EQ** as an effect. The eight bands sit on the FX page across the
four encoders, two bands per encoder:

| Encoder | Default (page 1) | Alternate (press encoder) |
| --- | --- | --- |
| 1 | **63 Hz** | **1 kHz** |
| 2 | **125 Hz** | **2 kHz** |
| 3 | **250 Hz** | **4 kHz** |
| 4 | **500 Hz** | **8 kHz** |

- Each encoder runs from **−24 dB to +24 dB**, centred at **0 dB** (12 o’clock =
  flat, no effect).
- **Press any encoder** to flip the whole page between the low four bands
  (63–500 Hz) and the high four (1–8 kHz).
- With every band at 0 dB the effect is transparent — signal passes untouched.

Bands are **peaking filters** (bell curves), not shelves, so each one lifts or
scoops a region around its centre frequency and leaves the rest alone.

## Concept

Equalisation is just volume, but per frequency. Each of the eight bands is a
**peaking (bell) filter** centred on a fixed, musically spaced frequency
(roughly one octave apart from 63 Hz to 8 kHz). Turn a band up and you add
energy around that pitch; turn it down and you carve it out.

What you hear, band by band:

- **63 / 125 Hz** — weight and rumble. Kick body, bass fundamentals, the “chest”
  of a sound. Cut here to de-mud; boost for power (watch your headroom).
- **250 / 500 Hz** — body and warmth, but also boxiness/mud. The first place to
  cut when a mix feels congested.
- **1 / 2 kHz** — presence and attack. Where snares crack, vocals cut, and
  plucks define their transient.
- **4 / 8 kHz** — air, sizzle, and sibilance. Boost for sheen on hats and vocals;
  cut to tame harshness.

Under the hood each band is a fixed-point RBJ-cookbook biquad. The filter
coefficients for all 65 gain steps per band are **precomputed offline** and
baked into a table, so the device does no trig or division while it runs — it
just looks up the curve for the current knob position. That keeps it cheap
enough to leave running everywhere.

Because it’s **graphic** (fixed frequencies, fixed Q) rather than parametric,
you trade surgical control for speed: there’s nothing to sweep or set up, you
just grab the band nearest the problem and move it.

## Example usage

- **Carve space in a busy loop** — on a sample track, pull **250 Hz** and/or
  **500 Hz** down a few dB to clear mud, nudge **2 kHz** up for presence. Instant
  clarity without touching the arrangement.
- **Lo-fi / telephone effect** — cut **63** and **125 Hz** hard, cut **8 kHz**,
  and push **1 kHz**: the sound collapses to a thin, mid-forward “through a
  speaker” tone.
- **Tame a harsh hat or cymbal** — small cut at **4 kHz** or **8 kHz** takes the
  ice-pick off without dulling the groove.
- **Add weight to a bassline** — modest boost at **63/125 Hz**, slight cut at
  **500 Hz** to keep it from getting woolly.
- **Bus / master polish** — gentle ±2–3 dB moves across a few bands on the main
  output to tilt the overall balance brighter or warmer.

Tip: **cutting usually sounds more natural than boosting.** If a band feels too
loud, try pulling the bands around it down instead of pushing it up — you keep
more headroom and avoid piling everything into a boost.

## Contribute

The machine lives at the repository root: DSP in `fx.cpp` / `core.h`, the control
layout in `definition.json`, and the baked filter table in `coeffs.h`
(regenerate it with `tools/gen_coeffs.py` — don’t hand-edit the header).
Agent/contributor notes are in [AGENTS.md](AGENTS.md).

Keep the [Open-XY SDK](https://github.com/etcetc000/open-xy) checked out beside
this repo as `../open-xy` (branch `staging`) and run the host sound tests:

```sh
python ../open-xy/sdk.py test --machine .
```

Effects have no browser preview, so audition on hardware after building the
firmware — follow the SDK’s [build](https://github.com/etcetc000/open-xy/blob/staging/docs/build.md)
and [hardware profiling](https://github.com/etcetc000/open-xy/blob/staging/docs/profiling.md)
guides, and keep all firmware/stock content **outside** this checkout. The host
CPU estimate is a guide only — catalog submission needs a real on-device
measurement. PRs welcome: new tunings, a shelf mode, or a wider/narrower Q are
all fair game — add a matching test in `tests/`.

## Credits

- Built on the [Open-XY SDK](https://github.com/etcetc000/open-xy) by the Open-XY
  authors.
- Filter design follows the **Robert Bristow-Johnson “Audio EQ Cookbook”**
  peaking-biquad formulas.
- Created by [@icrotz](https://github.com/icrotz) with Claude (Claude Code).
- Licensed under [LICENSE](LICENSE).
