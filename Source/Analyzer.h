#pragma once

#include "Common.h"

// Motor de análise (roda na thread de áudio, publica resultados via atomics).
//  - Curva tonal: FFT 16384 pts, bandas de 1/3 de oitava espaçadas em 1/12 de oitava,
//    potência em escala de Q constante, média exponencial + normalização (média 0 dB).
//  - Vocal Balance (heurístico, sem IA): estima o nível da voz em relação ao resto da mix
//    combinando 3 pistas que instrumentos de harmonia (violão/guitarra) em geral NÃO têm juntas:
//      1) centro estéreo  - por bin, nível igual e fase coerente entre L e R (voz no centro);
//      2) tonalidade      - o espectro do centro tem picos harmônicos (não é ruído/bateria);
//      3) modulação silábica - o envelope em dB das bandas 250 Hz-5 kHz oscila entre ~1,2 e 8 Hz.
//  - Dynamics: PSR = pico / RMS de curto prazo (3 s), suavizado.
//  - Stereo Width: energia Side / Mid, suavizada.
class TonalAnalyzer
{
public:
    static constexpr int fftOrder = 14;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int hopSize  = fftSize / 4;
    static constexpr int shortTermBlocks = 30;   // 30 x 100 ms = 3 s
    static constexpr int vOrder = 12;
    static constexpr int vSize  = 1 << vOrder;   // FFT do Vocal Balance (4096)
    static constexpr int vHop   = 1024;
    static constexpr int vBands = 5;

    TonalAnalyzer()
        : fft (fftOrder),
          window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false),
          fifo ((size_t) fftSize, 0.0f),
          fftData ((size_t) fftSize * 2, 0.0f),
          vfft (vOrder),
          vwin ((size_t) vSize, juce::dsp::WindowingFunction<float>::hann, false),
          vRingL ((size_t) vSize, 0.0f), vRingR ((size_t) vSize, 0.0f),
          vL ((size_t) vSize * 2, 0.0f), vR ((size_t) vSize * 2, 0.0f)
    {
        for (auto& v : avgDb)  v.store (0.0f);
        for (auto& v : fastDb) v.store (0.0f);
        doReset();
    }

    // thread de mensagem / preparação
    void prepare (double sr)
    {
        sampleRate = sr;
        subLen     = juce::jmax (1, (int) (sr * 0.1));
        hopSeconds = (double) hopSize / sr;
        vHopSeconds = (double) vHop / sr;
        subSeconds = 0.1;
        doReset();
    }

    void requestReset()                       { resetFlag.store (true); }
    void setVocalEnabled (bool b)             { vocalEnabled = b; }
    void setAveragingSeconds (float seconds)  { tau.store (seconds); }   // 0 = infinito

    // thread de áudio
    void process (const float* l, const float* r, int n)
    {
        if (resetFlag.exchange (false)) doReset();

        for (int i = 0; i < n; ++i)
        {
            const float L = l[i], R = r[i];
            const float m = 0.5f * (L + R), s = 0.5f * (L - R);

            fifo[(size_t) fifoPos] = m;
            fifoPos = (fifoPos + 1) & (fftSize - 1);
            if (++hopCount >= hopSize) { hopCount = 0; analyseFrame(); }

            vRingL[(size_t) vPos] = L;
            vRingR[(size_t) vPos] = R;
            vPos = (vPos + 1) & (vSize - 1);
            if (++vHopCount >= vHop) { vHopCount = 0; analyseVocal(); }

            subMid  += m * m;
            subSide += s * s;
            subLR   += 0.5f * (L * L + R * R);
            subPeak  = std::max (subPeak, std::max (std::abs (L), std::abs (R)));
            if (++subCount >= subLen) finishSubBlock();
        }
    }

    // resultados (leitura em qualquer thread)
    std::array<std::atomic<float>, rp::numBands> avgDb, fastDb;
    std::atomic<float> avgPsr { 11.0f }, avgWidthDb { -8.0f }, vocalDb { -6.0f }, vocalConf { 0.0f };
    std::atomic<bool>  hasData { false }, signalActive { false };

private:
    void doReset()
    {
        std::fill (fifo.begin(), fifo.end(), 0.0f);
        fifoPos = hopCount = subCount = ringPos = ringFilled = 0;
        subMid = subSide = subLR = subPeak = 0.0f;
        avgPow.fill (0.0f);
        fastPow.fill (0.0f);
        avgCount = fastCount = psrCount = widthCount = 0;
        psrAvg = 11.0f; midAvg = sideAvg = 0.0f;
        ringE.fill (0.0f); ringPk.fill (0.0f);
        active = false;
        std::fill (vRingL.begin(), vRingL.end(), 0.0f);
        std::fill (vRingR.begin(), vRingR.end(), 0.0f);
        vPos = vHopCount = 0;
        vInit = false; vCount = 0;
        lpFast.fill (0.0f); lpSlow.fill (0.0f); modVar.fill (0.0f);
        gE = gPv = gT = confE = 0.0f;
        vocalDb.store (-6.0f); vocalConf.store (0.0f);
        for (auto& v : avgDb)  v.store (0.0f);
        for (auto& v : fastDb) v.store (0.0f);
        hasData.store (false);
        signalActive.store (false);
    }

    static float sstep (float x, float e0, float e1)
    {
        const float u = juce::jlimit (0.0f, 1.0f, (x - e0) / (e1 - e0));
        return u * u * (3.0f - 2.0f * u);
    }

    static float alphaFor (double dt, float tauSec, int& count)
    {
        ++count;
        const float running = 1.0f / (float) count;           // média corrida no início / modo infinito
        if (tauSec <= 0.0f) return running;
        return std::max (running, 1.0f - std::exp (-(float) dt / tauSec));
    }

    void publish (const std::array<float, rp::numBands>& pow,
                  std::array<std::atomic<float>, rp::numBands>& out)
    {
        std::array<float, rp::numBands> db {}, sm {};
        for (int i = 0; i < rp::numBands; ++i)
            db[(size_t) i] = 10.0f * std::log10 (pow[(size_t) i] + 1.0e-20f);

        float mean = 0.0f;
        for (int i = 0; i < rp::numBands; ++i)
        {
            float sum = 0.0f; int cnt = 0;
            for (int k = std::max (0, i - 2); k <= std::min (rp::numBands - 1, i + 2); ++k)
            { sum += db[(size_t) k]; ++cnt; }
            sm[(size_t) i] = sum / (float) cnt;
            mean += sm[(size_t) i];
        }
        mean /= (float) rp::numBands;

        for (int i = 0; i < rp::numBands; ++i)
            out[(size_t) i].store (sm[(size_t) i] - mean);
    }

    void analyseFrame()
    {
        if (! active) return;

        for (int i = 0; i < fftSize; ++i)
            fftData[(size_t) i] = fifo[(size_t) ((fifoPos + i) & (fftSize - 1))];
        window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

        const int   maxBin = fftSize / 2;
        const float binHz  = (float) sampleRate / (float) fftSize;
        const float norm   = 1.0f / ((float) fftSize * 0.5f);

        auto power = [&] (int k)
        {
            const float mag = fftData[(size_t) juce::jlimit (0, maxBin, k)] * norm;
            return mag * mag;
        };

        std::array<float, rp::numBands> frame {};
        for (int i = 0; i < rp::numBands; ++i)
        {
            const float fc  = rp::bandFreq (i);
            const float lo  = fc * 0.890899f / binHz;     // 2^(-1/6)
            const float hi  = std::min (fc * 1.122462f / binHz, (float) maxBin);   // 2^(+1/6)
            const float wid = std::max (hi - lo, 0.0f);

            float density;
            if (wid < 1.0f)
            {
                const float b = std::min (fc / binHz, (float) maxBin - 1.0f);
                const int   i0 = (int) b;
                const float fr = b - (float) i0;
                density = power (i0) * (1.0f - fr) + power (i0 + 1) * fr;
            }
            else
            {
                const int k0 = (int) std::ceil (lo), k1 = (int) std::floor (hi);
                float sum = 0.0f; int cnt = 0;
                for (int k = k0; k <= k1; ++k) { sum += power (k); ++cnt; }
                density = cnt > 0 ? sum / (float) cnt : power ((int) lo);
            }
            frame[(size_t) i] = density * std::max (wid, 0.05f);   // potência da banda (Q constante)
        }

        const float t = tau.load();
        const float aAvg  = alphaFor (hopSeconds, t, avgCount);
        const float aFast = alphaFor (hopSeconds, 0.4f, fastCount);
        for (int i = 0; i < rp::numBands; ++i)
        {
            avgPow[(size_t) i]  += aAvg  * (frame[(size_t) i] - avgPow[(size_t) i]);
            fastPow[(size_t) i] += aFast * (frame[(size_t) i] - fastPow[(size_t) i]);
        }

        publish (avgPow, avgDb);
        publish (fastPow, fastDb);

        hasData.store (true);
    }

    void analyseVocal()
    {
        if (! active || ! vocalEnabled) return;

        for (int i = 0; i < vSize; ++i)
        {
            vL[(size_t) i] = vRingL[(size_t) ((vPos + i) & (vSize - 1))];
            vR[(size_t) i] = vRingR[(size_t) ((vPos + i) & (vSize - 1))];
        }
        vwin.multiplyWithWindowingTable (vL.data(), (size_t) vSize);
        vwin.multiplyWithWindowingTable (vR.data(), (size_t) vSize);
        std::fill (vL.begin() + vSize, vL.end(), 0.0f);
        std::fill (vR.begin() + vSize, vR.end(), 0.0f);
        vfft.performRealOnlyForwardTransform (vL.data(), true);
        vfft.performRealOnlyForwardTransform (vR.data(), true);

        const int   nyq   = vSize / 2;
        const float binHz = (float) sampleRate / (float) vSize;

        double total = 0.0;
        for (int k = 1; k < nyq; ++k)
        {
            const float lr = vL[(size_t) (2 * k)], li = vL[(size_t) (2 * k + 1)];
            const float rr = vR[(size_t) (2 * k)], ri = vR[(size_t) (2 * k + 1)];
            total += 0.5 * ((double) lr * lr + (double) li * li + (double) rr * rr + (double) ri * ri);
        }
        if (total < 1.0e-9) return;

        static const float edgesHz[vBands + 1] = { 250.0f, 500.0f, 1000.0f, 2000.0f, 3500.0f, 5000.0f };
        int edge[vBands + 1];
        for (int b = 0; b <= vBands; ++b)
            edge[b] = juce::jlimit (1, nyq - 1, (int) std::round (edgesHz[b] / binHz));

        // 1) potência do "centro" por banda  +  espectro mid (para tonalidade)
        std::array<float, vBands> bandP {};
        double midSum = 0.0, allP = 0.0; int nBins = 0;
        for (int b = 0; b < vBands; ++b)
            for (int k = edge[b]; k < edge[b + 1]; ++k)
            {
                const float lr = vL[(size_t) (2 * k)], li = vL[(size_t) (2 * k + 1)];
                const float rr = vR[(size_t) (2 * k)], ri = vR[(size_t) (2 * k + 1)];
                const float a2 = lr * lr + li * li, b2 = rr * rr + ri * ri;
                const float ab = std::sqrt (a2 * b2);
                float pc = 0.0f;
                if (ab > 1.0e-20f)
                {
                    const float bal = 2.0f * ab / (a2 + b2);                       // 1 = mesmo nível em L e R
                    const float coh = (lr * rr + li * ri) / ab;                    // cos(fase L-R)
                    pc = sstep (bal, 0.90f, 0.99f) * sstep (coh, 0.93f, 0.985f) * 0.5f * (a2 + b2);
                }
                allP += 0.5f * (a2 + b2);
                bandP[(size_t) b] += pc;
                midBuf[(size_t) (k - edge[0])] = pc;      // espectro do CENTRO (não do mid)
                midSum += pc; ++nBins;
            }
        if (nBins < 8 || midSum < 1.0e-12) return;

        // 2) tonalidade: planicidade espectral do centro (ruído ~ -2,5 dB; voz/harmônicos < -6 dB)
        const double meanMid = midSum / (double) nBins;
        double logSum = 0.0;
        for (int i = 0; i < nBins; ++i)
            logSum += std::log ((double) midBuf[(size_t) i] + 1.0e-3 * meanMid + 1.0e-30);
        const float flatDb = (float) (10.0 * std::log10 (std::exp (logSum / (double) nBins) / meanMid));
        const float tonal = juce::jlimit (0.0f, 1.0f, (-flatDb - 4.0f) / 8.0f);

        // 3) modulação silábica do envelope (dB) de cada banda do centro
        const float frameRate = (float) (sampleRate / (double) vHop);
        const float a8 = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 8.0f  / frameRate);
        const float a1 = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 1.2f  / frameRate);
        const float aM = 1.0f - std::exp (-1.0f / (frameRate * 2.0f));
        const float totalDb = (float) (10.0 * std::log10 (total + 1.0e-12));

        float wSum = 0.0f, mSum = 0.0f, pCentre = 0.0f;
        for (int b = 0; b < vBands; ++b)
        {
            const float x = std::max (10.0f * std::log10 (bandP[(size_t) b] + 1.0e-12f), totalDb - 40.0f);
            if (! vInit) { lpFast[(size_t) b] = x; lpSlow[(size_t) b] = x; }
            lpFast[(size_t) b] += a8 * (x - lpFast[(size_t) b]);
            lpSlow[(size_t) b] += a1 * (x - lpSlow[(size_t) b]);
            const float y = lpFast[(size_t) b] - lpSlow[(size_t) b];
            modVar[(size_t) b] += aM * (y * y - modVar[(size_t) b]);
            wSum += bandP[(size_t) b];
            mSum += bandP[(size_t) b] * modVar[(size_t) b];
            pCentre += bandP[(size_t) b];
        }
        vInit = true;
        const float depth = wSum > 0.0f ? std::sqrt (mSum / wSum) : 0.0f;     // dB rms
        const float gMod  = juce::jlimit (0.0f, 1.0f, (depth - 2.0f) / 4.0f);

        // Potência "tipo voz" do quadro e peso de confiança g
        const float pv = pCentre * tonal;
        // o centro precisa ser uma parte relevante da faixa 250 Hz-5 kHz (descarta vazamentos de fase casual)
        const float centreShare = allP > 0.0 ? (float) (pCentre / allP) : 0.0f;
        const float g  = gMod * sstep (centreShare, 0.03f, 0.15f);

        const float tauNow = tau.load();
        const float a = alphaFor (vHopSeconds, tauNow, vCount);
        gE   += a * (g - gE);
        gPv  += a * (g * pv - gPv);
        gT   += a * (g * (float) total - gT);
        confE += (1.0f - std::exp (-(float) vHopSeconds / 3.0f)) * (g * tonal - confE);

        vocalConf.store (confE);
        if (gE > 1.0e-3f)
        {
            const float vw = gPv / gE, tw = gT / gE;
            const float rest = std::max (tw - vw, 1.0e-3f * tw);
            vocalDb.store (juce::jlimit (-30.0f, 30.0f, 10.0f * std::log10 ((vw + 1.0e-20f) / (rest + 1.0e-20f))));
        }
    }

    void finishSubBlock()
    {
        const float eLR = subLR / (float) subLen;
        active = eLR > 1.0e-6f;                       // gate ~ -60 dBFS
        signalActive.store (active);

        ringE[(size_t) ringPos]  = eLR;
        ringPk[(size_t) ringPos] = subPeak;
        ringPos = (ringPos + 1) % shortTermBlocks;
        ringFilled = std::min (ringFilled + 1, shortTermBlocks);

        if (active)
        {
            float meanE = 0.0f, pk = 0.0f;
            for (int k = 0; k < ringFilled; ++k)
            { meanE += ringE[(size_t) k]; pk = std::max (pk, ringPk[(size_t) k]); }
            meanE /= (float) ringFilled;

            const float psr = 10.0f * std::log10 ((pk * pk + 1.0e-12f) / (meanE + 1.0e-12f));
            const float t = tau.load();
            psrAvg += alphaFor (subSeconds, t, psrCount) * (psr - psrAvg);
            avgPsr.store (psrAvg);

            const float a = alphaFor (subSeconds, t, widthCount);
            midAvg  += a * (subMid  / (float) subLen - midAvg);
            sideAvg += a * (subSide / (float) subLen - sideAvg);
            avgWidthDb.store (10.0f * std::log10 ((sideAvg + 1.0e-12f) / (midAvg + 1.0e-12f)));
        }

        subMid = subSide = subLR = subPeak = 0.0f;
        subCount = 0;
    }

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> fifo, fftData;

    // Vocal Balance
    juce::dsp::FFT vfft;
    juce::dsp::WindowingFunction<float> vwin;
    std::vector<float> vRingL, vRingR, vL, vR;
    std::array<float, vSize / 2> midBuf {};
    int vPos = 0, vHopCount = 0, vCount = 0;
    double vHopSeconds = 0.0;
    bool vInit = false;
    std::array<float, vBands> lpFast {}, lpSlow {}, modVar {};
    float gE = 0, gPv = 0, gT = 0, confE = 0;

    double sampleRate = 44100.0, hopSeconds = 0.0, subSeconds = 0.1;
    int subLen = 4410;
    int fifoPos = 0, hopCount = 0, subCount = 0, ringPos = 0, ringFilled = 0;
    int avgCount = 0, fastCount = 0, psrCount = 0, widthCount = 0;
    float subMid = 0, subSide = 0, subLR = 0, subPeak = 0;
    float psrAvg = 11.0f, midAvg = 0, sideAvg = 0;
    bool  active = false;
    bool  vocalEnabled = true;

    std::array<float, rp::numBands> avgPow {}, fastPow {};
    std::array<float, shortTermBlocks> ringE {}, ringPk {};

    std::atomic<bool>  resetFlag { false };
    std::atomic<float> tau { 10.0f };
};
