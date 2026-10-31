#pragma once

#include "Common.h"
#include <complex>

// Equalizador de 10 bandas (biquads RBJ). Compartilhado entre processador (áudio) e interface (curva).
namespace Eq
{
    constexpr int numBands = 10;
    enum Type { Bell = 0, LowShelf, HighShelf, LowCut, HighCut, Notch, numTypes };

    inline juce::StringArray typeNames()
    {
        return { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch" };
    }
    inline bool typeHasGain (int t) { return t == Bell || t == LowShelf || t == HighShelf; }

    inline const float* defaultFreqs()
    {
        static const float f[numBands] = { 30.f, 80.f, 160.f, 320.f, 640.f, 1250.f, 2500.f, 5000.f, 10000.f, 16000.f };
        return f;
    }

    struct Params
    {
        bool  on = false;
        int   type = Bell;
        float freq = 1000.0f, gain = 0.0f, q = 1.0f;
        int   slope = 0;                 // cortes: 0 = 12, 1 = 24, 2 = 36, 3 = 48 dB/oitava
        int   mode = 0;                  // 0 = Stereo, 1 = Mid, 2 = Side
        bool  dyn = false;               // banda dinâmica (Bell / shelves)
        float thresh = -30.0f, ratio = 3.0f;
        bool operator== (const Params& o) const
        {
            return on == o.on && type == o.type && juce::exactlyEqual (freq, o.freq)
                && juce::exactlyEqual (gain, o.gain) && juce::exactlyEqual (q, o.q)
                && slope == o.slope && mode == o.mode && dyn == o.dyn
                && juce::exactlyEqual (thresh, o.thresh) && juce::exactlyEqual (ratio, o.ratio);
        }
        bool operator!= (const Params& o) const { return ! (*this == o); }
    };

    struct Coeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

    inline Coeffs make (const Params& p, double sr)
    {
        const double f  = juce::jlimit (10.0, sr * 0.49, (double) p.freq);
        const double w0 = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double Q  = std::max (0.1, (double) p.q);
        const double al = sw / (2.0 * Q);
        const double A  = std::pow (10.0, (double) p.gain / 40.0);
        const double sA = 2.0 * std::sqrt (A) * al;

        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
        switch (p.type)
        {
            case Bell:
                b0 = 1 + al * A; b1 = -2 * cw; b2 = 1 - al * A;
                a0 = 1 + al / A; a1 = -2 * cw; a2 = 1 - al / A; break;
            case LowShelf:
                b0 = A * ((A + 1) - (A - 1) * cw + sA);
                b1 = 2 * A * ((A - 1) - (A + 1) * cw);
                b2 = A * ((A + 1) - (A - 1) * cw - sA);
                a0 = (A + 1) + (A - 1) * cw + sA;
                a1 = -2 * ((A - 1) + (A + 1) * cw);
                a2 = (A + 1) + (A - 1) * cw - sA; break;
            case HighShelf:
                b0 = A * ((A + 1) + (A - 1) * cw + sA);
                b1 = -2 * A * ((A - 1) + (A + 1) * cw);
                b2 = A * ((A + 1) + (A - 1) * cw - sA);
                a0 = (A + 1) - (A - 1) * cw + sA;
                a1 = 2 * ((A - 1) - (A + 1) * cw);
                a2 = (A + 1) - (A - 1) * cw - sA; break;
            case LowCut:
                b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2;
                a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;
            case HighCut:
                b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2;
                a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;
            case Notch:
            default:
                b0 = 1; b1 = -2 * cw; b2 = 1;
                a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;
        }
        return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
    }

    inline double magnitudeDb (const Coeffs& c, double f, double sr)
    {
        const double w = juce::MathConstants<double>::twoPi * f / sr;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = std::polar (1.0, -2.0 * w);
        const auto num = c.b0 + c.b1 * z1 + c.b2 * z2;
        const auto den = 1.0 + c.a1 * z1 + c.a2 * z2;
        return 20.0 * std::log10 (std::abs (num / den) + 1.0e-20);
    }

    // A banda precisa ser processada? (shelf/bell com 0 dB = identidade)
    inline bool isActive (const Params& p)
    {
        if (! p.on) return false;
        return ! (typeHasGain (p.type) && std::abs (p.gain) < 0.01f);
    }

    // Cortes de 24/36/48 dB/oit = cascata de biquads Butterworth (até 4 seções)
    struct Cascade { int n = 1; Coeffs c[4]; };

    inline Cascade makeCascade (const Params& p, double sr)
    {
        Cascade r;
        if ((p.type == LowCut || p.type == HighCut) && p.slope > 0)
        {
            static const double q4[] = { 0.5412, 1.3066 }, q6[] = { 0.5176, 0.7071, 1.9319 },
                                q8[] = { 0.5098, 0.6013, 0.8999, 2.5629 };
            const double* qs = p.slope == 1 ? q4 : (p.slope == 2 ? q6 : q8);
            r.n = p.slope == 1 ? 2 : (p.slope == 2 ? 3 : 4);
            for (int i = 0; i < r.n; ++i)
            {
                Params pp = p; pp.q = (float) qs[i];
                r.c[i] = make (pp, sr);
            }
        }
        else
            r.c[0] = make (p, sr);
        return r;
    }

    inline double magnitudeDb (const Cascade& cs, double f, double sr)
    {
        double db = 0.0;
        for (int i = 0; i < cs.n; ++i) db += magnitudeDb (cs.c[i], f, sr);
        return db;
    }

    // Passa-faixa (ganho 0 dB no pico) - usado pelo detector das bandas dinâmicas e pelo solo
    inline Coeffs makeBandpass (float freq, float q, double sr)
    {
        const double f = juce::jlimit (10.0, sr * 0.49, (double) freq);
        const double w0 = juce::MathConstants<double>::twoPi * f / sr;
        const double al = std::sin (w0) / (2.0 * juce::jlimit (0.3, 12.0, (double) q));
        const double a0 = 1.0 + al;
        return { al / a0, 0.0, -al / a0, -2.0 * std::cos (w0) / a0, (1.0 - al) / a0 };
    }
}
