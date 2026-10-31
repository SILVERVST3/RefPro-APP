#pragma once

#include "Common.h"

// Uma curva de referência (alvo). 'centre' está normalizada (média = 0 dB) e
// 'tol' é a meia-largura da faixa verde/azul em cada banda.
struct TargetData
{
    juce::String name { "All-Purpose" };
    std::array<float, rp::numBands> centre {};
    std::array<float, rp::numBands> tol {};
    float psr     = 11.0f;   // alvo do medidor Dynamics (pico - RMS de curto prazo, dB)
    float widthDb = -8.0f;   // alvo do Stereo Width (energia Side / Mid, dB)
    float vocalDb = -6.0f;   // alvo do Vocal Balance (voz / resto da mix, dB)
    bool  builtIn = true;
};

namespace Targets
{
    struct Pt { float f, db, tol; };

    inline void normalise (std::array<float, rp::numBands>& a)
    {
        float m = 0.0f;
        for (auto v : a) m += v;
        m /= (float) a.size();
        for (auto& v : a) v -= m;
    }

    inline void smooth (std::array<float, rp::numBands>& a, int radius, int passes)
    {
        for (int p = 0; p < passes; ++p)
        {
            auto src = a;
            for (int i = 0; i < rp::numBands; ++i)
            {
                float sum = 0.0f; int cnt = 0;
                for (int k = std::max (0, i - radius); k <= std::min (rp::numBands - 1, i + radius); ++k)
                { sum += src[(size_t) k]; ++cnt; }
                a[(size_t) i] = sum / (float) cnt;
            }
        }
    }

    // -----------------------------------------------------------------------
    //  All-Purpose
    //  Curva calibrada com 7 músicas de referência escolhidas pelo usuário
    //  (média em dB, nível normalizado; tolerância ~ dispersão entre as músicas).
    //  NÃO são dados da iZotope.
    // -----------------------------------------------------------------------
    inline TargetData makeAllPurpose()
    {
        static const float centre[rp::numBands] = {
            -19.17f, -18.85f, -17.95f, -16.93f, -15.61f, -14.33f, -13.00f, -11.77f, -10.41f, -8.92f,
            -7.21f, -5.32f, -3.30f, -1.23f, 0.70f, 2.38f, 3.76f, 4.87f, 5.72f, 6.39f,
            6.94f, 7.47f, 7.93f, 8.34f, 8.62f, 8.78f, 8.78f, 8.66f, 8.46f, 8.14f,
            7.71f, 7.12f, 6.49f, 5.88f, 5.46f, 5.27f, 5.35f, 5.60f, 5.87f, 6.00f,
            5.96f, 5.74f, 5.44f, 5.12f, 4.92f, 4.82f, 4.81f, 4.81f, 4.79f, 4.77f,
            4.74f, 4.73f, 4.72f, 4.69f, 4.70f, 4.78f, 5.00f, 5.34f, 5.73f, 6.11f,
            6.32f, 6.36f, 6.22f, 5.95f, 5.59f, 5.24f, 4.93f, 4.67f, 4.46f, 4.24f,
            4.00f, 3.72f, 3.46f, 3.32f, 3.32f, 3.49f, 3.71f, 3.93f, 4.07f, 4.10f,
            4.02f, 3.80f, 3.41f, 2.80f, 2.10f, 1.40f, 0.83f, 0.44f, 0.23f, 0.12f,
            0.01f, -0.17f, -0.42f, -0.75f, -1.14f, -1.54f, -1.89f, -2.14f, -2.25f, -2.27f,
            -2.27f, -2.32f, -2.49f, -2.78f, -3.20f, -3.70f, -4.29f, -4.94f, -5.68f, -6.55f,
            -7.57f, -8.76f, -10.11f, -11.59f, -13.15f, -14.77f, -16.46f, -18.32f, -19.96f, -21.36f,
            -21.87f,
        };
        static const float tol[rp::numBands] = {
            3.10f, 3.13f, 3.15f, 3.18f, 3.18f, 3.17f, 3.13f, 3.04f, 2.88f, 2.64f,
            2.32f, 1.99f, 1.72f, 1.58f, 1.55f, 1.60f, 1.65f, 1.62f, 1.52f, 1.40f,
            1.29f, 1.25f, 1.29f, 1.34f, 1.35f, 1.35f, 1.32f, 1.26f, 1.21f, 1.20f,
            1.22f, 1.30f, 1.44f, 1.58f, 1.74f, 1.87f, 1.92f, 1.92f, 1.89f, 1.85f,
            1.85f, 1.86f, 1.87f, 1.91f, 1.95f, 2.01f, 2.11f, 2.25f, 2.38f, 2.49f,
            2.55f, 2.53f, 2.49f, 2.45f, 2.42f, 2.41f, 2.39f, 2.33f, 2.29f, 2.30f,
            2.34f, 2.40f, 2.42f, 2.35f, 2.15f, 1.88f, 1.62f, 1.45f, 1.39f, 1.43f,
            1.53f, 1.63f, 1.69f, 1.72f, 1.71f, 1.72f, 1.74f, 1.78f, 1.81f, 1.81f,
            1.78f, 1.71f, 1.65f, 1.62f, 1.61f, 1.61f, 1.61f, 1.58f, 1.52f, 1.43f,
            1.34f, 1.27f, 1.22f, 1.21f, 1.24f, 1.25f, 1.25f, 1.25f, 1.24f, 1.21f,
            1.20f, 1.23f, 1.29f, 1.39f, 1.52f, 1.66f, 1.80f, 1.93f, 2.04f, 2.14f,
            2.26f, 2.42f, 2.62f, 2.86f, 3.09f, 3.28f, 3.41f, 3.49f, 3.50f, 3.50f,
            3.50f,
        };

        TargetData t;
        t.name = "All-Purpose";
        t.psr = 11.9f;
        t.widthDb = -7.0f;
        t.vocalDb = -5.7f;
        t.builtIn = true;
        for (int i = 0; i < rp::numBands; ++i)
        {
            t.centre[(size_t) i] = centre[i];
            t.tol[(size_t) i] = tol[i];
        }
        normalise (t.centre);
        return t;
    }
}
