#include "core.h"
#include "coeffs.h"
#include <opxy/q31.hpp>

namespace eq8 {
namespace {
const opxy::i32 kMaximum = opxy::q31::maximum, kMinimum = -kMaximum - 1;
const opxy::i32 kFlat = 1 << 30;  // control 0.5 in Q31: 0 dB

opxy::i32 add(opxy::i32 a, opxy::i32 b) {  // saturating
    if (b > 0 && a > kMaximum - b) return kMaximum;
    if (b < 0 && a < kMinimum - b) return kMinimum;
    return a + b;
}

// 4 * acc, clamped to full scale. The stored coefficients are scaled down by 4
// (to hold +/-24 dB peaks within Q31), so the biquad accumulator is a quarter
// of the output; this restores it. The argument is the sum of five Q31
// products, which fits an i64 without per-term saturation.
opxy::i32 scale_clamp(opxy::i64 acc) {
    acc <<= 2;
    if (acc > kMaximum) return kMaximum;
    if (acc < kMinimum) return kMinimum;
    return static_cast<opxy::i32>(acc);
}

// About a 1.5 ms glide toward a new control level, so gain changes do not
// click. Both are non-negative, so their difference cannot overflow; the last
// steps move by one so the level arrives exactly.
opxy::i32 smooth(opxy::i32 value, opxy::i32 target) {
    const opxy::i32 difference = target - value, step = difference / 64;
    return value + (step ? step : (difference > 0) - (difference < 0));
}

// Interpolate one halved-Q31 coefficient for a band at gain control `g`.
// kSteps - 1 == 64 intervals, so the integer step is g >> 25 (0..63) and the
// low 25 bits, shifted up to Q31, are the fraction within the interval.
struct Coeffs { opxy::i32 b0, b1, b2, a1, a2; };

Coeffs band_coeffs(unsigned band, opxy::i32 g) {
    if (g < 0) g = 0;
    const unsigned idx = static_cast<opxy::u32>(g) >> 25;        // 0..63
    const unsigned next = idx + 1 < kSteps ? idx + 1 : idx;      // clamp top
    const opxy::i32 frac = (static_cast<opxy::u32>(g) & ((1 << 25) - 1)) << 6;  // Q31
    const opxy::i32* lo = kCoeffs[band][idx];
    const opxy::i32* hi = kCoeffs[band][next];
    Coeffs c;
    opxy::i32* out = &c.b0;
    for (unsigned k = 0; k < 5; ++k)
        out[k] = add(lo[k], opxy::q31::multiply(frac, hi[k] - lo[k]));
    return c;
}

// Direct Form I peaking biquad over one 64-frame channel in place, with its
// own two-sample history. Coefficients are scaled down by 4, so scale_clamp()
// scales back. The five Q31 products accumulate in an i64 (single MAC each on
// the Blackfin) and clamp once, with no per-term saturation.
void run_band(const Coeffs& c, opxy::Channel ch,
              opxy::i32& x1, opxy::i32& x2, opxy::i32& y1, opxy::i32& y2) {
    opxy::i32 s1 = x1, s2 = x2, o1 = y1, o2 = y2;
    for (unsigned i = 0; i < opxy::block_frames; ++i) {
        const opxy::i32 x = ch[i];
        opxy::i64 acc = opxy::q31::multiply(c.b0, x);
        acc += opxy::q31::multiply(c.b1, s1);
        acc += opxy::q31::multiply(c.b2, s2);
        acc -= opxy::q31::multiply(c.a1, o1);
        acc -= opxy::q31::multiply(c.a2, o2);
        const opxy::i32 y = scale_clamp(acc);
        s2 = s1; s1 = x;
        o2 = o1; o1 = y;
        ch[i] = y;
    }
    x1 = s1; x2 = s2; y1 = o1; y2 = o2;
}
}

void initialize(State& state) {
    for (unsigned b = 0; b < 8; ++b) {
        state.gain[b] = kFlat;  // start flat so a reset does not jump
        state.lx1[b] = state.lx2[b] = state.ly1[b] = state.ly2[b] = 0;
        state.rx1[b] = state.rx2[b] = state.ry1[b] = state.ry2[b] = 0;
    }
}

// Eight-band graphic EQ: each control boosts or cuts its fixed band by up to
// +/-12 dB (0.5 flat). Bands run in series; gains smooth per block, so the
// biquad coefficients stay constant within a block.
void process(State& state, opxy::Channel left, opxy::Channel right, opxy::Parameters parameters) {
    for (unsigned b = 0; b < 8; ++b) {
        opxy::i32 target = parameters[b];
        if (target < 0) target = 0;
        state.gain[b] = smooth(state.gain[b], target);
        const Coeffs c = band_coeffs(b, state.gain[b]);
        run_band(c, left, state.lx1[b], state.lx2[b], state.ly1[b], state.ly2[b]);
        run_band(c, right, state.rx1[b], state.rx2[b], state.ry1[b], state.ry2[b]);
    }
}
}
