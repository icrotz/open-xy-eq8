// The machine's own sound checks, compiled for the host and run by
// python sdk.py test --machine <folder>. Extend them as the sound grows.
#include "../fx.cpp"
#include <cstdio>
#include <cmath>
#include <cstdlib>

namespace {
const opxy::i32 kMax = opxy::q31::maximum;
const opxy::i32 kFlatCtl = 1 << 30;  // 0.5 control: 0 dB

void flat(opxy::i32 p[8]) { for (int i = 0; i < 8; ++i) p[i] = kFlatCtl; }
void boost(opxy::i32 p[8]) { for (int i = 0; i < 8; ++i) p[i] = kMax; }

bool fail(const char* msg) { std::puts(msg); return false; }

// A pure tone, amplitude a (Q31), frequency f Hz at 44.1 kHz.
opxy::i32 tone(unsigned n, double f, double a) {
    return static_cast<opxy::i32>(a * std::sin(2.0 * M_PI * f * n / 44100.0));
}

bool test_silence() {
    static eq8::State state;
    eq8::initialize(state);
    opxy::i32 p[8]; boost(p);
    for (unsigned block = 0; block < 32; ++block) {
        opxy::i32 l[64] = {0}, r[64] = {0};
        eq8::process(state, l, r, p);
        for (unsigned i = 0; i < 64; ++i)
            if (l[i] || r[i]) return fail("FAIL silence in gives sound out");
    }
    return true;
}

// Flat EQ is mathematically unity (numerator == denominator per band), so the
// output tracks the input within fixed-point rounding after a short warm-up.
bool test_flat_unity() {
    static eq8::State state;
    eq8::initialize(state);
    opxy::i32 p[8]; flat(p);
    const double amp = 0x20000000;  // quarter scale, headroom for the cascade
    unsigned n = 0;
    double err = 0, sig = 0;
    for (unsigned block = 0; block < 64; ++block) {
        opxy::i32 l[64], r[64], in[64];
        for (unsigned i = 0; i < 64; ++i) in[i] = l[i] = r[i] = tone(n + i, 220.0, amp);
        eq8::process(state, l, r, p);
        if (block >= 8)  // after warm-up
            for (unsigned i = 0; i < 64; ++i) {
                err += std::fabs(double(l[i]) - in[i]);
                sig += std::fabs(double(in[i]));
            }
        n += 64;
    }
    if (err > 0.02 * sig) return fail("FAIL flat EQ is not unity passthrough");
    return true;
}

// A stable +12 dB-everywhere filter must not run away: bounded input stays
// bounded and, once input stops, the tail decays toward silence.
bool test_stable() {
    static eq8::State state;
    eq8::initialize(state);
    opxy::i32 p[8]; boost(p);
    unsigned n = 0;
    opxy::i32 last = 0;
    for (unsigned block = 0; block < 96; ++block) {
        opxy::i32 l[64], r[64];
        const bool driven = block < 48;
        for (unsigned i = 0; i < 64; ++i)
            // low level so a +24 dB (~16x) resonant peak stays below full
            // scale and the bound check below is meaningful, not just clamped.
            l[i] = r[i] = driven ? tone(n + i, 1000.0, 0x01000000) : 0;
        eq8::process(state, l, r, p);
        for (unsigned i = 0; i < 64; ++i)
            if (std::llabs(l[i]) > kMax || std::llabs(r[i]) > kMax)
                return fail("FAIL output exceeds full scale");
        last = l[63];
        n += 64;
    }
    if (std::llabs(last) > 0x00010000)  // tail has decayed near zero
        return fail("FAIL tail does not decay after input stops");
    return true;
}
}

int main() {
    if (!test_silence()) return 1;
    if (!test_flat_unity()) return 1;
    if (!test_stable()) return 1;
    return 0;
}
