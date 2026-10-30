#pragma once
#include <opxy/effect.hpp>

namespace eq8 {
// Eight cascaded peaking biquads, one per fixed band. Per band we keep the
// smoothed gain control and two samples of input/output history per channel.
// Storage starts zeroed and is never destroyed, so State holds no resources.
// Size: (1 + 8) * 8 words = 288 bytes, within opxy::effect_state_bytes (476).
struct State {
    opxy::i32 gain[8];                 // smoothed gain control per band (Q31)
    opxy::i32 lx1[8], lx2[8];          // left input history
    opxy::i32 ly1[8], ly2[8];          // left output history
    opxy::i32 rx1[8], rx2[8];          // right input history
    opxy::i32 ry1[8], ry2[8];          // right output history
};
void initialize(State& state);
void process(State& state, opxy::Channel left, opxy::Channel right, opxy::Parameters parameters);
}
