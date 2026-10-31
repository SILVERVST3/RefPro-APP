#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <cmath>

// Constantes compartilhadas entre analisador, alvos e interface.
namespace rp
{
    constexpr int   numBands = 121;      // 20 Hz .. ~20 kHz, 12 bandas por oitava
    constexpr float fMin     = 20.0f;
    constexpr float dbTop    = 8.0f;     // limite superior do gráfico
    constexpr float dbBottom = -14.0f;   // limite inferior do gráfico

    inline float bandFreq (int i) { return fMin * std::pow (2.0f, (float) i / 12.0f); }

    // 20 Hz..20 kHz -> 0..1 (escala logarítmica)
    inline float freqToX01 (float f) { return std::log10 (f / fMin) / 3.0f; }
}
