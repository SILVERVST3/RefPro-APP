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
    //  Curva ORIGINAL desta implementação, desenhada a partir da forma geral
    //  mostrada no Tonal Balance Control 3 (graves elevados, queda suave e
    //  contínua para os agudos). NÃO são dados da iZotope.
    //  Ajuste os pontos abaixo (freq Hz, nível dB, tolerância dB) à vontade.
    // -----------------------------------------------------------------------
    inline TargetData makeAllPurpose()
    {
        static const Pt pts[] = {
            {    20.f,  1.0f, 2.6f }, {    30.f,  2.4f, 2.4f }, {    50.f,  3.2f, 2.0f },
            {    80.f,  3.2f, 1.8f }, {   120.f,  2.8f, 1.5f }, {   200.f,  1.8f, 1.3f },
            {   350.f,  0.9f, 1.1f }, {   600.f,  0.2f, 1.0f }, {  1000.f, -0.6f, 1.0f },
            {  1600.f, -1.4f, 1.0f }, {  2500.f, -2.2f, 1.1f }, {  4000.f, -3.2f, 1.3f },
            {  6000.f, -4.2f, 1.5f }, {  8000.f, -5.2f, 1.8f }, { 12000.f, -7.0f, 2.2f },
            { 16000.f, -9.0f, 2.8f }, { 20000.f, -11.0f, 3.4f }, { 24000.f, -13.0f, 3.8f }
        };
        constexpr int n = (int) (sizeof (pts) / sizeof (pts[0]));

        TargetData t;
        t.name = "All-Purpose";
        t.psr = 11.0f;
        t.widthDb = -8.0f;
        t.vocalDb = -6.0f;
        t.builtIn = true;

        for (int i = 0; i < rp::numBands; ++i)
        {
            const float f = rp::bandFreq (i);
            int k = 0;
            while (k < n - 2 && f > pts[k + 1].f) ++k;
            const float x0 = std::log (pts[k].f), x1 = std::log (pts[k + 1].f);
            const float u = juce::jlimit (0.0f, 1.0f, (std::log (f) - x0) / (x1 - x0));
            t.centre[(size_t) i] = pts[k].db  + u * (pts[k + 1].db  - pts[k].db);
            t.tol[(size_t) i]    = pts[k].tol + u * (pts[k + 1].tol - pts[k].tol);
        }

        smooth (t.centre, 3, 2);
        smooth (t.tol, 3, 2);
        normalise (t.centre);
        return t;
    }
}
