#include "PluginProcessor.h"
#include "PluginEditor.h"

// Thread que analisa arquivos de áudio e gera UM alvo (média das curvas; tolerância = desvio entre arquivos).
struct RefProProcessor::Builder : public juce::Thread
{
    Builder (RefProProcessor& o, juce::Array<juce::File> f, juce::String n)
        : juce::Thread ("RefPro target builder"), owner (o), files (std::move (f)), name (std::move (n)) {}

    void run() override
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();

        std::vector<std::array<float, rp::numBands>> curves;
        double psrSum = 0.0, widSum = 0.0, vocSum = 0.0;
        int nVoc = 0;

        for (int fi = 0; fi < files.size() && ! threadShouldExit(); ++fi)
        {
            std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (files[fi]));
            if (rd == nullptr || rd->lengthInSamples <= 0 || rd->sampleRate <= 0.0) continue;

            auto an = std::make_unique<TonalAnalyzer>();
            an->prepare (rd->sampleRate);
            an->setAveragingSeconds (0.0f);                 // média de todo o arquivo

            juce::AudioBuffer<float> buf (2, 16384);
            juce::int64 pos = 0;
            while (pos < rd->lengthInSamples && ! threadShouldExit())
            {
                const int num = (int) juce::jmin ((juce::int64) 16384, rd->lengthInSamples - pos);
                buf.clear();
                rd->read (&buf, 0, num, pos, true, rd->numChannels > 1);
                if (rd->numChannels < 2) buf.copyFrom (1, 0, buf, 0, 0, num);
                an->process (buf.getReadPointer (0), buf.getReadPointer (1), num);
                pos += num;
                owner.buildProgress.store (((float) fi + (float) pos / (float) rd->lengthInSamples) / (float) files.size());
            }
            if (threadShouldExit() || ! an->hasData.load()) continue;

            std::array<float, rp::numBands> c {};
            for (int i = 0; i < rp::numBands; ++i) c[(size_t) i] = an->avgDb[(size_t) i].load();
            curves.push_back (c);
            psrSum += an->avgPsr.load();
            widSum += an->avgWidthDb.load();
            if (an->vocalConf.load() >= 0.12f) { vocSum += an->vocalDb.load(); ++nVoc; }
        }

        if (threadShouldExit()) { owner.finishBuild ({}, "Analysis cancelled"); return; }
        if (curves.empty())     { owner.finishBuild ({}, "No valid audio found"); return; }

        const int n = (int) curves.size();
        TargetData t;
        t.name = name;
        t.builtIn = false;
        for (int i = 0; i < rp::numBands; ++i)
        {
            double m = 0.0;
            for (auto& c : curves) m += c[(size_t) i];
            m /= n;
            double var = 0.0;
            for (auto& c : curves) var += (c[(size_t) i] - m) * (c[(size_t) i] - m);
            t.centre[(size_t) i] = (float) m;
            t.tol[(size_t) i] = n >= 3 ? juce::jlimit (1.0f, 4.0f, (float) std::sqrt (var / n)) : 1.5f;
        }
        Targets::smooth (t.tol, 3, 2);
        Targets::normalise (t.centre);
        t.psr = (float) (psrSum / n);
        t.widthDb = (float) (widSum / n);
        t.vocalDb = nVoc > 0 ? (float) (vocSum / nVoc) : -6.0f;
        owner.finishBuild (t, "Target created from " + juce::String (n) + (n == 1 ? " file" : " files"));
    }

    RefProProcessor& owner;
    juce::Array<juce::File> files;
    juce::String name;
};

juce::AudioProcessorValueTreeState::ParameterLayout RefProProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "target", 1 }, "Target", 0, blendIndex, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "avg", 1 }, "Averaging",
                    juce::StringArray { "Short (3 s)", "Medium (10 s)", "Long (30 s)", "Infinite" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "eqOn", 1 }, "EQ On", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "anPost", 1 }, "Analysis Point",
                    juce::StringArray { "Pre-EQ", "Post-EQ" }, 1));

    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "dynOn", 1 }, "Dynamic EQ On", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dynAmount", 1 }, "Dynamic Amount",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String ((int) std::round (v)) + " %"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dynMax", 1 }, "Dynamic Max Cut",
                    juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f), 4.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dynTol", 1 }, "Dynamic Tolerance",
                    juce::NormalisableRange<float> (0.0f, 6.0f, 0.1f), 0.5f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return "+" + juce::String (v, 1) + " dB"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dynBoost", 1 }, "Dynamic Max Boost",
                    juce::NormalisableRange<float> (0.0f, 9.0f, 0.1f), 3.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return "+" + juce::String (v, 1) + " dB"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dynAdapt", 1 }, "Dynamic Adapt",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String ((int) std::round (v)) + " %"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "dynComp", 1 }, "Dynamic Level Comp", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "vocRide", 1 }, "Vocal Auto Adjust", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "vocMax", 1 }, "Vocal Auto Max",
                    juce::NormalisableRange<float> (0.0f, 4.0f, 0.1f), 2.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "dynSpeed", 1 }, "Dynamic Speed",
                    juce::StringArray { "Fast", "Medium", "Slow" }, 1));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "outGain", 1 }, "EQ Output Gain",
                    juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "blendA", 1 }, "Blend Target A", 0, maxCaptures, 0));
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "blendB", 1 }, "Blend Target B", 0, maxCaptures, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "blendMix", 1 }, "Blend Mix",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String ((int) std::round (v)) + " % B"; })
                                                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));

    auto freqText = [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String ((int) std::round (v)) + " Hz"; };
    auto freqVal  = [] (const juce::String& s)
    {
        const auto l = s.trim().toLowerCase();
        float v = l.getFloatValue();
        if (l.contains ("k")) v *= 1000.0f;
        return v;
    };

    {
        juce::NormalisableRange<float> fr (20.0f, 20000.0f, 1.0f);
        fr.setSkewForCentre (1000.0f);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dynLo", 1 }, "Dynamic Low Limit", fr, 20.0f,
                        juce::AudioParameterFloatAttributes().withStringFromValueFunction (freqText).withValueFromStringFunction (freqVal)));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dynHi", 1 }, "Dynamic High Limit", fr, 20000.0f,
                        juce::AudioParameterFloatAttributes().withStringFromValueFunction (freqText).withValueFromStringFunction (freqVal)));
    }

    for (int b = 0; b < Eq::numBands; ++b)
    {
        const auto name = "Band " + juce::String (b + 1) + " ";
        juce::NormalisableRange<float> fr (20.0f, 20000.0f, 0.01f);
        fr.setSkewForCentre (1000.0f);
        juce::NormalisableRange<float> qr (0.1f, 10.0f, 0.01f);
        qr.setSkewForCentre (1.0f);

        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bandId (b, "on"), 1 }, name + "On", false));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { bandId (b, "type"), 1 }, name + "Type", Eq::typeNames(), 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { bandId (b, "freq"), 1 }, name + "Freq", fr, Eq::defaultFreqs()[b],
                        juce::AudioParameterFloatAttributes().withStringFromValueFunction (freqText).withValueFromStringFunction (freqVal)));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { bandId (b, "gain"), 1 }, name + "Gain",
                        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f,
                        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })
                                                              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { bandId (b, "q"), 1 }, name + "Q", qr, 1.0f,
                        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); })
                                                              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bandId (b, "dyn"), 1 }, name + "Dynamic", false));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { bandId (b, "thresh"), 1 }, name + "Threshold",
                        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -30.0f,
                        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })
                                                              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
        juce::NormalisableRange<float> rr (1.0f, 20.0f, 0.1f);
        rr.setSkewForCentre (4.0f);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { bandId (b, "ratio"), 1 }, name + "Ratio", rr, 3.0f,
                        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + ":1"; })
                                                              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { bandId (b, "slope"), 1 }, name + "Slope",
                        juce::StringArray { "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" }, 0));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { bandId (b, "mode"), 1 }, name + "Channel",
                        juce::StringArray { "Stereo", "Mid", "Side" }, 0));
    }
    return { p.begin(), p.end() };
}

RefProProcessor::RefProProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "RefProSilver", createLayout()),
      allPurpose (Targets::makeAllPurpose())
{
    for (auto& v : dynGainDb) v.store (0.0f);
    for (auto& v : bandDynGain) v.store (0.0f);
    for (auto& v : dynRefShift) v.store (0.0f);
    for (auto& v : dynTc) v.store (0.0f);
    for (auto& v : dynTt) v.store (1.5f);
    detector.setVocalEnabled (true);      // o detector também alimenta o ajuste automático da voz
    dynOnParam     = apvts.getRawParameterValue ("dynOn");
    dynAmountParam = apvts.getRawParameterValue ("dynAmount");
    dynMaxParam    = apvts.getRawParameterValue ("dynMax");
    dynTolParam    = apvts.getRawParameterValue ("dynTol");
    dynSpeedParam  = apvts.getRawParameterValue ("dynSpeed");
    dynLoParam     = apvts.getRawParameterValue ("dynLo");
    dynHiParam     = apvts.getRawParameterValue ("dynHi");
    dynBoostParam  = apvts.getRawParameterValue ("dynBoost");
    dynAdaptParam  = apvts.getRawParameterValue ("dynAdapt");
    dynCompParam   = apvts.getRawParameterValue ("dynComp");
    vocRideParam   = apvts.getRawParameterValue ("vocRide");
    vocMaxParam    = apvts.getRawParameterValue ("vocMax");
    for (const char* id : { "target", "blendA", "blendB", "blendMix" }) apvts.addParameterListener (id, this);
    eqParamIds.push_back ("outGain");
    publishDynTarget();
    avgParam  = apvts.getRawParameterValue ("avg");
    eqOnParam = apvts.getRawParameterValue ("eqOn");
    postParam = apvts.getRawParameterValue ("anPost");
    blendAParam   = apvts.getRawParameterValue ("blendA");
    blendBParam   = apvts.getRawParameterValue ("blendB");
    blendMixParam = apvts.getRawParameterValue ("blendMix");
    outGainParam  = apvts.getRawParameterValue ("outGain");
    for (int b = 0; b < Eq::numBands; ++b)
    {
        pOn[(size_t) b]   = apvts.getRawParameterValue (bandId (b, "on"));
        pType[(size_t) b] = apvts.getRawParameterValue (bandId (b, "type"));
        pFreq[(size_t) b] = apvts.getRawParameterValue (bandId (b, "freq"));
        pGain[(size_t) b] = apvts.getRawParameterValue (bandId (b, "gain"));
        pQ[(size_t) b]    = apvts.getRawParameterValue (bandId (b, "q"));
        pDyn[(size_t) b]    = apvts.getRawParameterValue (bandId (b, "dyn"));
        pThresh[(size_t) b] = apvts.getRawParameterValue (bandId (b, "thresh"));
        pRatio[(size_t) b]  = apvts.getRawParameterValue (bandId (b, "ratio"));
        pSlope[(size_t) b]  = apvts.getRawParameterValue (bandId (b, "slope"));
        pMode[(size_t) b]   = apvts.getRawParameterValue (bandId (b, "mode"));
        for (const char* what : { "on", "type", "freq", "gain", "q", "dyn", "thresh", "ratio", "slope", "mode" })
            eqParamIds.push_back (bandId (b, what));
    }
}

RefProProcessor::~RefProProcessor()
{
    for (const char* id : { "target", "blendA", "blendB", "blendMix" }) apvts.removeParameterListener (id, this);
    cancelPendingUpdate();
    loopback.stop();
    if (builder != nullptr) builder->stopThread (10000);
    builder.reset();
}

void RefProProcessor::publishDynTarget()
{
    const int idx = (int) std::round (apvts.getRawParameterValue ("target")->load());
    const auto t = getTarget (idx);
    for (int i = 0; i < rp::numBands; ++i)
    {
        dynTc[(size_t) i].store (t.centre[(size_t) i]);
        dynTt[(size_t) i].store (t.tol[(size_t) i]);
    }
    dynTv.store (t.vocalDb);
}

Eq::Params RefProProcessor::getBandParams (int b) const
{
    Eq::Params p;
    p.on   = pOn[(size_t) b]->load() > 0.5f;
    p.type = juce::jlimit (0, (int) Eq::numTypes - 1, (int) std::round (pType[(size_t) b]->load()));
    p.freq = pFreq[(size_t) b]->load();
    p.gain = pGain[(size_t) b]->load();
    p.q    = pQ[(size_t) b]->load();
    p.dyn    = pDyn[(size_t) b]->load() > 0.5f;
    p.thresh = pThresh[(size_t) b]->load();
    p.ratio  = pRatio[(size_t) b]->load();
    p.slope  = juce::jlimit (0, 3, (int) std::round (pSlope[(size_t) b]->load()));
    p.mode   = juce::jlimit (0, 2, (int) std::round (pMode[(size_t) b]->load()));
    return p;
}

void RefProProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    soloBuf.setSize (2, juce::jmax (samplesPerBlock, 8192), false, false, true);
    lastSolo = -1; outGainLin = 1.0f;
    forceEqUpdate = true;
    detector.prepare (sampleRate);
    dynWasOn = false;
    for (auto& d : dyn) d = DynBand {};
    for (auto& v : dynGainDb) v.store (0.0f);
    for (auto& v : dynRefShift) v.store (0.0f);
    dynCompDb = 0.0f; dynCompLin = 1.0f; dynCompOut.store (0.0f);
    detWasOn = false; rideDb = 0.0f; rideCoefDb = 999.0f; rideActive = false; vocRideOut.store (0.0f);
    for (auto& r : ride) r = RideBq {};
    for (auto& b : eq) b.resetStates();
    analyzer.prepare (sampleRate);
}

bool RefProProcessor::startLoopback (juce::String& error)
{
    if (loopbackOn.load()) return true;
    loopbackRate = 0.0;
    std::string err;
    loopbackOn.store (true);     // silencia a entrada do dispositivo (microfone) antes de começar
    const bool ok = loopback.start ([this] (const float* l, const float* r, int n, double sr)
    {
        const juce::SpinLock::ScopedLockType lk (analyzerLock);
        if (std::abs (sr - loopbackRate) > 0.5) { analyzer.prepare (sr); loopbackRate = sr; }
        analyzer.process (l, r, n);
    }, err);
    if (! ok) { loopbackOn.store (false); error = juce::String (err); return false; }
    return true;
}

void RefProProcessor::stopLoopback()
{
    if (! loopbackOn.load()) return;
    loopback.stop();
    loopbackOn.store (false);
    {
        const juce::SpinLock::ScopedLockType lk (analyzerLock);
        analyzer.prepare (currentSampleRate);
    }
    analyzer.requestReset();
}

bool RefProProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainInputChannelSet() != l.getMainOutputChannelSet()) return false;
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void RefProProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() < 1) return;

    static const float taus[] = { 3.0f, 10.0f, 30.0f, 0.0f };

    if (loopbackOn.load())      // modo áudio do sistema: a análise vem da thread de captura; a saída fica muda
    {
        const juce::SpinLock::ScopedTryLockType lk (analyzerLock);
        if (lk.isLocked()) analyzer.setAveragingSeconds (taus[juce::jlimit (0, 3, (int) avgParam->load())]);
        buffer.clear();
        return;
    }

    analyzer.setAveragingSeconds (taus[juce::jlimit (0, 3, (int) avgParam->load())]);

    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const bool post = postParam->load() > 0.5f;

    auto analyse = [&]
    {
        const float* l = buffer.getReadPointer (0);
        const float* r = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : l;
        analyzer.process (l, r, n);
    };

    if (! post) analyse();

    const int solo = soloBand.load();
    const bool soloOk = solo >= 0 && solo < Eq::numBands && n <= soloBuf.getNumSamples();
    if (soloOk)
        for (int ch = 0; ch < numCh; ++ch) soloBuf.copyFrom (ch, 0, buffer, ch, 0, n);   // entrada antes do EQ

    if (eqOnParam->load() > 0.5f) runEq (buffer, n, numCh);

    runDynamic (buffer, n, numCh);

    if (post) analyse();

    // ganho de saída do EQ (com rampa)
    {
        const float g1 = std::pow (10.0f, outGainParam->load() / 20.0f);
        if (std::abs (g1 - 1.0f) > 1.0e-5f || std::abs (outGainLin - 1.0f) > 1.0e-5f)
            for (int ch = 0; ch < numCh; ++ch)
            {
                float* x = buffer.getWritePointer (ch);
                for (int i = 0; i < n; ++i) x[i] *= outGainLin + (g1 - outGainLin) * (float) (i + 1) / (float) n;
            }
        outGainLin = g1;
    }

    // solo de banda: ouve só a região da banda (passa-faixa da entrada), sem afetar a análise
    if (soloOk)
    {
        if (lastSolo != solo) { soloZ1[0] = soloZ1[1] = soloZ2[0] = soloZ2[1] = 0.0; }
        const auto prm = getBandParams (solo);
        const auto c = Eq::makeBandpass (prm.freq, prm.q, currentSampleRate);
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* x = buffer.getWritePointer (ch);
            const float* src = soloBuf.getReadPointer (ch);
            double z1 = soloZ1[ch], z2 = soloZ2[ch];
            for (int i = 0; i < n; ++i)
            {
                const double in = src[i];
                const double y = c.b0 * in + z1;
                z1 = c.b1 * in - c.a1 * y + z2;
                z2 = c.b2 * in - c.a2 * y;
                x[i] = (float) y;
            }
            soloZ1[ch] = z1; soloZ2[ch] = z2;
        }
    }
    lastSolo = soloOk ? solo : -1;
}

void RefProProcessor::applyBand (BandState& st, int mode, float* d0, float* d1, int len)
{
    const int ns = st.cas.n;
    auto biquad = [&st] (int sec, int ch, double x)
    {
        const auto& c = st.cas.c[sec];
        const double y = c.b0 * x + st.z1[sec][ch];
        st.z1[sec][ch] = c.b1 * x - c.a1 * y + st.z2[sec][ch];
        st.z2[sec][ch] = c.b2 * x - c.a2 * y;
        return y;
    };

    if (d1 == nullptr || mode == 0)
    {
        for (int ch = 0; ch < (d1 != nullptr ? 2 : 1); ++ch)
        {
            float* d = ch == 0 ? d0 : d1;
            for (int i = 0; i < len; ++i)
            {
                double x = d[i];
                for (int sec = 0; sec < ns; ++sec) x = biquad (sec, ch, x);
                d[i] = (float) x;
            }
        }
    }
    else
    {
        for (int i = 0; i < len; ++i)
        {
            double m = 0.5 * ((double) d0[i] + (double) d1[i]), s = 0.5 * ((double) d0[i] - (double) d1[i]);
            double& v = mode == 1 ? m : s;
            for (int sec = 0; sec < ns; ++sec) v = biquad (sec, 0, v);
            d0[i] = (float) (m + s);
            d1[i] = (float) (m - s);
        }
    }
}

void RefProProcessor::runEq (juce::AudioBuffer<float>& buffer, int n, int numCh)
{
    float* d0 = buffer.getWritePointer (0);
    float* d1 = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    const double sr = currentSampleRate;
    const double aAtk = 1.0 - std::exp (-1.0 / (0.015 * sr)), aRel = 1.0 - std::exp (-1.0 / (0.150 * sr));

    for (int b = 0; b < Eq::numBands; ++b)
    {
        auto& st = eq[(size_t) b];
        const auto prm = getBandParams (b);
        const bool dynamic = prm.on && prm.dyn && Eq::typeHasGain (prm.type);

        if (forceEqUpdate || prm != st.last)
        {
            const bool structural = forceEqUpdate || prm.type != st.last.type || prm.mode != st.last.mode
                                    || prm.slope != st.last.slope || prm.on != st.last.on || prm.dyn != st.last.dyn;
            st.last = prm;
            st.cas = Eq::makeCascade (prm, sr);
            st.coefGain = prm.gain;
            st.bp = Eq::makeBandpass (prm.freq, prm.q, sr);
            if (structural) st.resetStates();
        }

        st.active = prm.on && (dynamic || Eq::isActive (prm));
        if (! st.active) { bandDynGain[(size_t) b].store (0.0f); continue; }

        if (! dynamic)
        {
            bandDynGain[(size_t) b].store (0.0f);
            applyBand (st, prm.mode, d0, d1, n);
            continue;
        }

        // banda dinâmica: detector passa-faixa -> nível -> ganho (compressor/expansor) -> bell
        const int sub = 64;
        for (int s0 = 0; s0 < n; s0 += sub)
        {
            const int len = std::min (sub, n - s0);
            double e = st.env, z1 = st.bz1, z2 = st.bz2;
            for (int i = s0; i < s0 + len; ++i)
            {
                const double x = d1 != nullptr ? (prm.mode == 2 ? 0.5 * ((double) d0[i] - (double) d1[i])
                                                                  : 0.5 * ((double) d0[i] + (double) d1[i]))
                                               : (double) d0[i];
                const double y = st.bp.b0 * x + z1;
                z1 = st.bp.b1 * x - st.bp.a1 * y + z2;
                z2 = st.bp.b2 * x - st.bp.a2 * y;
                const double pw = y * y;
                e += (pw > e ? aAtk : aRel) * (pw - e);
            }
            st.env = e; st.bz1 = z1; st.bz2 = z2;

            const double level = 10.0 * std::log10 (e + 1.0e-12) + 3.0103;     // dB (pico equivalente)
            const double G = prm.gain, R = std::max (1.0, (double) prm.ratio);
            double g = 0.0;
            if (G < 0.0) { const double over = level - prm.thresh;  if (over > 0.0)  g = std::max (G, -over * (1.0 - 1.0 / R)); }
            else         { const double under = prm.thresh - level; if (under > 0.0) g = std::min (G, under * (1.0 - 1.0 / R)); }
            st.gSmooth += 0.35 * (g - st.gSmooth);

            if (std::abs (st.gSmooth - st.coefGain) > 0.01)
            {
                Eq::Params pp = prm;
                pp.gain = (float) st.gSmooth;
                st.cas = Eq::makeCascade (pp, sr);
                st.coefGain = st.gSmooth;
            }
            bandDynGain[(size_t) b].store ((float) st.gSmooth);
            applyBand (st, prm.mode, d0 + s0, d1 != nullptr ? d1 + s0 : nullptr, len);
        }
    }
    forceEqUpdate = false;
}

void RefProProcessor::abSwitch (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    if (slot == abCurrent) return;

    std::vector<float> snap;
    for (auto& id : eqParamIds) snap.push_back (apvts.getRawParameterValue (id)->load());
    abSlots[(size_t) abCurrent] = snap;
    if (abSlots[(size_t) slot].empty()) abSlots[(size_t) slot] = snap;

    const auto& v = abSlots[(size_t) slot];
    for (size_t i = 0; i < eqParamIds.size() && i < v.size(); ++i)
        if (auto* prm = apvts.getParameter (eqParamIds[i]))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (v[i]));
            prm->endChangeGesture();
        }
    abCurrent = slot;
}


// Detector compartilhado (mede o sinal na entrada do estágio dinâmico) + EQ dinâmico + ajuste automático da voz.
// Ordem: EQ estático -> [detector] -> EQ dinâmico -> ajuste da voz -> análise pós.
void RefProProcessor::runDynamic (juce::AudioBuffer<float>& buffer, int n, int numCh)
{
    const bool dynOn  = dynOnParam->load() > 0.5f;
    const bool rideOn = vocRideParam->load() > 0.5f;

    if (dynOn || rideOn)
    {
        if (! detWasOn) { detector.requestReset(); detWasOn = true; }
        static const float taus[] = { 1.5f, 3.0f, 6.0f };
        detector.setAveragingSeconds (taus[juce::jlimit (0, 2, (int) dynSpeedParam->load())]);
        const float* l = buffer.getReadPointer (0);
        const float* r = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : l;
        detector.process (l, r, n);
    }
    else if (detWasOn)
    {
        detWasOn = false;
        detector.requestReset();
    }

    runDynEq (buffer, n, numCh, dynOn);
    runVocalRide (buffer, n, numCh, rideOn);
}

// Ajuste automático da voz: quando o Vocal Balance sai da faixa ótima (±1,5 dB), aplica até ±2 dB
// de correção no canal CENTRO (mid) da região vocal (~300 Hz - 3,5 kHz). Instrumentos abertos nos
// lados quase não são afetados. A medição vem do detector (antes da correção), sem realimentação.
void RefProProcessor::runVocalRide (juce::AudioBuffer<float>& buffer, int n, int numCh, bool rideOn)
{
    const float dt = (float) n / (float) currentSampleRate;
    float target = 0.0f;
    if (rideOn && detector.hasData.load() && detector.vocalConf.load() >= 0.15f)
    {
        const float dev = detector.vocalDb.load() - dynTv.load();
        const float excess = std::abs (dev) - 1.5f;
        if (excess > 0.0f) target = -(dev > 0.0f ? 1.0f : -1.0f) * std::min (vocMaxParam->load(), excess);
    }
    const float a = 1.0f - std::exp (-dt / (std::abs (target) > std::abs (rideDb) ? 1.5f : 3.0f));
    rideDb += a * (target - rideDb);
    vocRideOut.store (rideDb);

    if (std::abs (rideDb) < 0.03f)
    {
        if (rideActive) { rideActive = false; for (auto& r : ride) r = RideBq {}; rideCoefDb = 999.0f; }
        return;
    }
    if (! rideActive || std::abs (rideDb - rideCoefDb) > 0.02f)
    {
        const float freqs[2] = { 600.0f, 1800.0f };
        for (int k = 0; k < 2; ++k)
        {
            Eq::Params pp;
            pp.on = true; pp.type = Eq::Bell; pp.freq = freqs[k]; pp.gain = 0.7f * rideDb; pp.q = 0.6f;
            ride[(size_t) k].c = Eq::make (pp, currentSampleRate);
        }
        rideCoefDb = rideDb;
    }
    rideActive = true;

    auto filt = [] (RideBq& f, double x)
    {
        const double y = f.c.b0 * x + f.z1;
        f.z1 = f.c.b1 * x - f.c.a1 * y + f.z2;
        f.z2 = f.c.b2 * x - f.c.a2 * y;
        return y;
    };

    float* x0 = buffer.getWritePointer (0);
    if (numCh >= 2)
    {
        float* x1 = buffer.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            const double m = 0.5 * ((double) x0[i] + (double) x1[i]), s = 0.5 * ((double) x0[i] - (double) x1[i]);
            const double mf = filt (ride[1], filt (ride[0], m));
            x0[i] = (float) (mf + s);
            x1[i] = (float) (mf - s);
        }
    }
    else
        for (int i = 0; i < n; ++i)
            x0[i] = (float) filt (ride[1], filt (ride[0], (double) x0[i]));
}

// ---------------------------------------------------------------- EQ dinâmico
// Compara (média de alguns segundos) o espectro medido na ENTRADA deste estágio com o alvo.
//  - Âncora: a mediana dos desvios (robusta a picos isolados) define o "nível" da referência.
//  - Adapt: parte do desvio LARGO (média móvel de ~2,7 oitavas) é absorvida pela referência;
//           só o desvio estreito (picos/buracos) é corrigido. 0 % = curva fixa.
//  - Acima da tolerância: corte (até Max Cut). Abaixo: boost (até Max Boost), com piso de ruído.
//  - Level Comp: ganho automático que compensa a mudança de energia causada pelos cortes/boosts.
void RefProProcessor::runDynEq (juce::AudioBuffer<float>& buffer, int n, int numCh, bool dynOn)
{
    auto rampGain = [&] (float g0, float g1)
    {
        if (std::abs (g1 - 1.0f) < 1.0e-5f && std::abs (g0 - 1.0f) < 1.0e-5f) return;
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* x = buffer.getWritePointer (ch);
            for (int i = 0; i < n; ++i) x[i] *= g0 + (g1 - g0) * (float) (i + 1) / (float) n;
        }
    };

    if (! dynOn)
    {
        if (dynWasOn)
        {
            dynWasOn = false;
            for (int j = 0; j < dynBands; ++j)
            { dyn[(size_t) j] = DynBand {}; dynGainDb[(size_t) j].store (0.0f); dynRefShift[(size_t) j].store (0.0f); }
            rampGain (dynCompLin, 1.0f);
            dynCompDb = 0.0f; dynCompLin = 1.0f; dynCompOut.store (0.0f);
        }
        return;
    }
    dynWasOn = true;

    static const float atks[] = { 0.15f, 0.3f, 0.6f }, rels[] = { 0.5f, 1.0f, 2.0f };
    const int sp = juce::jlimit (0, 2, (int) dynSpeedParam->load());

    const float amount = dynAmountParam->load() * 0.01f, maxCut = dynMaxParam->load(), maxBoost = dynBoostParam->load();
    const float extraTol = dynTolParam->load(), adapt = dynAdaptParam->load() * 0.01f;
    const float lo = dynLoParam->load(), hi = dynHiParam->load();

    auto sstep = [] (float x, float e0, float e1)
    {
        const float u = juce::jlimit (0.0f, 1.0f, (x - e0) / (e1 - e0));
        return u * u * (3.0f - 2.0f * u);
    };

    std::array<float, dynBands> dev {}, meas {}, raw {}, tgt {}, shift {};
    std::array<bool, dynBands> valid {};
    if (detector.hasData.load())
    {
        for (int j = 0; j < dynBands; ++j)
        {
            const float fc = dynFreq (j);
            valid[(size_t) j] = ! (fc < lo * 0.8f || fc > hi * 1.25f) && fc < (float) currentSampleRate * 0.45f;
            float d = 0.0f, m = 0.0f;
            for (int k = 4 * j - 2; k <= 4 * j + 2; ++k)
            {
                const auto kk = (size_t) juce::jlimit (0, rp::numBands - 1, k);
                const float v = detector.avgDb[kk].load();
                m += v; d += v - dynTc[kk].load();
            }
            dev[(size_t) j] = d / 5.0f;
            meas[(size_t) j] = m / 5.0f;
        }

        // âncora: mediana dos desvios das bandas válidas com nível razoável
        std::array<float, dynBands> tmp {};
        int cnt = 0;
        for (int j = 0; j < dynBands; ++j)
            if (valid[(size_t) j] && meas[(size_t) j] > -30.0f) tmp[(size_t) cnt++] = dev[(size_t) j];
        float median = 0.0f;
        if (cnt >= 5)
        {
            std::nth_element (tmp.begin(), tmp.begin() + cnt / 2, tmp.begin() + cnt);
            median = tmp[(size_t) (cnt / 2)];
        }

        for (int j = 0; j < dynBands; ++j)
        {
            if (! valid[(size_t) j]) continue;
            // componente larga do desvio (relativo à âncora)
            float sum = 0.0f; int c = 0;
            for (int k = std::max (0, j - 4); k <= std::min (dynBands - 1, j + 4); ++k)
                if (valid[(size_t) k]) { sum += dev[(size_t) k] - median; ++c; }
            const float broad = c > 0 ? sum / (float) c : 0.0f;

            shift[(size_t) j] = median + adapt * broad;
            const float narrow = dev[(size_t) j] - shift[(size_t) j];
            const float tolTot = dynTt[(size_t) juce::jlimit (0, rp::numBands - 1, 4 * j)].load() + extraTol;

            if (narrow > tolTot)
                raw[(size_t) j] = -std::min (maxCut, amount * (narrow - tolTot));
            else if (narrow < -tolTot)
                raw[(size_t) j] = std::min (maxBoost, amount * (-narrow - tolTot)) * sstep (meas[(size_t) j], -40.0f, -25.0f);
        }
    }
    for (int j = 0; j < dynBands; ++j)
    {
        dynRefShift[(size_t) j].store (shift[(size_t) j]);
        const float a = raw[(size_t) std::max (j - 1, 0)], b = raw[(size_t) j], c = raw[(size_t) std::min (j + 1, dynBands - 1)];
        tgt[(size_t) j] = 0.25f * a + 0.5f * b + 0.25f * c;
    }

    const float dt = (float) n / (float) currentSampleRate;
    const float aA = 1.0f - std::exp (-dt / atks[sp]), aR = 1.0f - std::exp (-dt / rels[sp]);

    for (int j = 0; j < dynBands; ++j)
    {
        auto& d = dyn[(size_t) j];
        float g = d.gainDb;
        const float target = tgt[(size_t) j];
        g += (std::abs (target) > std::abs (g) ? aA : aR) * (target - g);   // afastar de 0 = ataque, voltar = release
        d.gainDb = g;
        dynGainDb[(size_t) j].store (g);

        const float fc = dynFreq (j);
        const bool act = std::abs (g) > 0.05f && fc < (float) currentSampleRate * 0.45f;
        if (! act)
        {
            if (d.active) { d.active = false; d.z1[0] = d.z1[1] = d.z2[0] = d.z2[1] = 0.0; }
            continue;
        }
        if (! d.active || std::abs (g - d.coefGain) > 0.02f)
        {
            Eq::Params pp;
            pp.on = true; pp.type = Eq::Bell; pp.freq = fc; pp.gain = g; pp.q = 4.3f;
            d.c = Eq::make (pp, currentSampleRate);
            d.coefGain = g;
        }
        d.active = true;

        const auto& c = d.c;
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* x = buffer.getWritePointer (ch);
            double z1 = d.z1[ch], z2 = d.z2[ch];
            for (int i = 0; i < n; ++i)
            {
                const double in = x[i];
                const double y = c.b0 * in + z1;
                z1 = c.b1 * in - c.a1 * y + z2;
                z2 = c.b2 * in - c.a2 * y;
                x[i] = (float) y;
            }
            d.z1[ch] = z1; d.z2[ch] = z2;
        }
    }

    // compensação de nível: mantém a energia média (ponderada pelo espectro medido) estável
    float compTarget = 0.0f;
    if (dynCompParam->load() > 0.5f && detector.hasData.load())
    {
        double wSum = 0.0, wGain = 0.0;
        for (int j = 0; j < dynBands; ++j)
        {
            const double w = std::pow (10.0, (double) meas[(size_t) j] / 10.0);
            wSum += w;
            wGain += w * std::pow (10.0, (double) dyn[(size_t) j].gainDb / 10.0);
        }
        if (wSum > 0.0) compTarget = juce::jlimit (-6.0f, 6.0f, (float) (-10.0 * std::log10 (wGain / wSum)));
    }
    dynCompDb += (1.0f - std::exp (-dt / 1.0f)) * (compTarget - dynCompDb);
    const float g1 = std::pow (10.0f, dynCompDb / 20.0f);
    rampGain (dynCompLin, g1);
    dynCompLin = g1;
    dynCompOut.store (dynCompDb);
}

// ---------------------------------------------------------------- alvos
void RefProProcessor::handleAsyncUpdate()
{
    publishDynTarget();
    TargetData t;
    bool has = false;
    {
        const juce::ScopedLock sl (pendingLock);
        if (hasPending) { t = pendingTarget; hasPending = false; has = true; }
    }
    if (has) addTarget (t);
}

juce::String RefProProcessor::uniqueName (const juce::String& wanted) const
{
    auto exists = [this] (const juce::String& n)
    {
        if (n.equalsIgnoreCase ("All-Purpose") || n.equalsIgnoreCase ("Blend")) return true;
        for (auto& c : captured) if (c.name == n) return true;
        return false;
    };
    const juce::String n = wanted.trim().isEmpty() ? juce::String ("Target") : wanted.trim();
    if (! exists (n)) return n;
    for (int i = 2; i < 1000; ++i)
    {
        const auto cand = n + " (" + juce::String (i) + ")";
        if (! exists (cand)) return cand;
    }
    return n;
}

int RefProProcessor::getNumTargets() const
{
    const juce::ScopedLock sl (capLock);
    return 1 + (int) captured.size();
}

TargetData RefProProcessor::getTargetNoBlend (int index) const
{
    const juce::ScopedLock sl (capLock);
    if (index <= 0 || index > (int) captured.size()) return allPurpose;
    return captured[(size_t) index - 1];
}

TargetData RefProProcessor::makeBlend() const
{
    const float mix = juce::jlimit (0.0f, 1.0f, blendMixParam->load() * 0.01f);
    const auto A = getTargetNoBlend ((int) std::round (blendAParam->load()));
    const auto B = getTargetNoBlend ((int) std::round (blendBParam->load()));
    TargetData t;
    t.name = "Blend";
    t.builtIn = true;
    for (size_t i = 0; i < (size_t) rp::numBands; ++i)
    {
        t.centre[i] = A.centre[i] * (1.0f - mix) + B.centre[i] * mix;
        t.tol[i]    = A.tol[i]    * (1.0f - mix) + B.tol[i]    * mix;
    }
    Targets::normalise (t.centre);
    t.psr     = A.psr     * (1.0f - mix) + B.psr     * mix;
    t.widthDb = A.widthDb * (1.0f - mix) + B.widthDb * mix;
    t.vocalDb = A.vocalDb * (1.0f - mix) + B.vocalDb * mix;
    return t;
}

TargetData RefProProcessor::getTarget (int index) const
{
    return index == blendIndex ? makeBlend() : getTargetNoBlend (index);
}

int RefProProcessor::addTarget (TargetData t)
{
    int idx;
    {
        const juce::ScopedLock sl (capLock);
        if ((int) captured.size() >= maxCaptures) return -1;
        t.builtIn = false;
        t.name = uniqueName (t.name);
        captured.push_back (t);
        idx = (int) captured.size();
        ++targetsVersion;
    }
    newIndex.store (idx);
    triggerAsyncUpdate();
    return idx;
}

RefProProcessor::CaptureResult RefProProcessor::captureCurrent()
{
    if (! analyzer.hasData.load()) return CaptureResult::noData;

    TargetData t;
    t.name = "Capture " + juce::String (++captureCounter);
    for (int i = 0; i < rp::numBands; ++i)
    {
        t.centre[(size_t) i] = analyzer.avgDb[(size_t) i].load();
        t.tol[(size_t) i] = 1.5f;
    }
    t.psr = analyzer.avgPsr.load();
    t.widthDb = analyzer.avgWidthDb.load();
    t.vocalDb = analyzer.vocalConf.load() >= 0.12f ? analyzer.vocalDb.load() : -6.0f;   // sem voz detectada: mantém o padrão
    return addTarget (t) >= 0 ? CaptureResult::ok : CaptureResult::full;
}

void RefProProcessor::deleteCapture (int targetIndex)
{
    bool removed = false;
    {
        const juce::ScopedLock sl (capLock);
        if (targetIndex >= 1 && targetIndex <= (int) captured.size())
        {
            captured.erase (captured.begin() + (targetIndex - 1));
            ++targetsVersion;
            removed = true;
        }
    }
    if (! removed) return;

    for (const char* id : { "blendA", "blendB" })   // mantém as referências do Blend válidas
        if (auto* prm = apvts.getParameter (id))
        {
            const int v = (int) std::round (apvts.getRawParameterValue (id)->load());
            const int nv = v == targetIndex ? 0 : (v > targetIndex ? v - 1 : v);
            if (nv != v) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) nv));
        }
    ++blendVersion;
    triggerAsyncUpdate();
}

bool RefProProcessor::renameTarget (int targetIndex, const juce::String& newName)
{
    const juce::ScopedLock sl (capLock);
    if (targetIndex < 1 || targetIndex > (int) captured.size() || newName.trim().isEmpty()) return false;
    auto& t = captured[(size_t) targetIndex - 1];
    if (t.name == newName.trim()) return true;
    const auto old = t.name;
    t.name = "";                         // evita colidir consigo mesmo
    t.name = uniqueName (newName);
    if (favorites.contains (old)) { favorites.removeString (old); favorites.addIfNotAlreadyThere (t.name); }
    ++targetsVersion;
    return true;
}

bool RefProProcessor::isFavorite (const juce::String& name) const
{
    const juce::ScopedLock sl (capLock);
    return favorites.contains (name);
}

void RefProProcessor::setFavorite (const juce::String& name, bool fav)
{
    const juce::ScopedLock sl (capLock);
    if (fav) favorites.addIfNotAlreadyThere (name); else favorites.removeString (name);
    ++targetsVersion;
}

bool RefProProcessor::exportTarget (int targetIndex, const juce::File& file) const
{
    const auto t = getTarget (targetIndex);
    auto* o = new juce::DynamicObject();
    juce::var v (o);
    o->setProperty ("format", "RefProSilverTarget");
    o->setProperty ("version", 1);
    o->setProperty ("name", t.name);
    o->setProperty ("psr", (double) t.psr);
    o->setProperty ("width", (double) t.widthDb);
    o->setProperty ("vocal", (double) t.vocalDb);
    juce::Array<juce::var> c, tl;
    for (size_t i = 0; i < (size_t) rp::numBands; ++i) { c.add ((double) t.centre[i]); tl.add ((double) t.tol[i]); }
    o->setProperty ("centre", juce::var (c));
    o->setProperty ("tol", juce::var (tl));
    return file.replaceWithText (juce::JSON::toString (v, false));
}

int RefProProcessor::importTargetFile (const juce::File& file)
{
    const auto v = juce::JSON::parse (file);
    if (! v.isObject()) return -1;
    const auto ce = v.getProperty ("centre", juce::var());
    if (! ce.isArray() || ce.size() != rp::numBands) return -1;
    const auto to = v.getProperty ("tol", juce::var());

    TargetData t;
    t.name = v.getProperty ("name", file.getFileNameWithoutExtension()).toString();
    if (t.name.trim().isEmpty()) t.name = file.getFileNameWithoutExtension();
    for (int i = 0; i < rp::numBands; ++i)
    {
        t.centre[(size_t) i] = (float) (double) ce[i];
        t.tol[(size_t) i] = (to.isArray() && to.size() == rp::numBands) ? (float) (double) to[i] : 1.5f;
    }
    Targets::normalise (t.centre);
    t.psr     = (float) (double) v.getProperty ("psr", 11.0);
    t.widthDb = (float) (double) v.getProperty ("width", -8.0);
    t.vocalDb = (float) (double) v.getProperty ("vocal", -6.0);
    return addTarget (t);
}

void RefProProcessor::startBuild (const juce::Array<juce::File>& files, const juce::String& name)
{
    if (buildBusy.load() || files.isEmpty()) return;
    builder.reset();
    buildBusy.store (true);
    buildProgress.store (0.0f);
    { const juce::ScopedLock sl (pendingLock); buildMessage = "Analyzing..."; }
    builder = std::make_unique<Builder> (*this, files, name);
    builder->startThread();
}

void RefProProcessor::cancelBuild()
{
    if (builder != nullptr) builder->signalThreadShouldExit();
}

juce::String RefProProcessor::getBuildMessage() const
{
    const juce::ScopedLock sl (pendingLock);
    return buildMessage;
}

void RefProProcessor::finishBuild (const TargetData& t, const juce::String& message)
{
    {
        const juce::ScopedLock sl (pendingLock);
        buildMessage = message;
        if (t.name.isNotEmpty()) { pendingTarget = t; hasPending = true; }
    }
    buildBusy.store (false);
    triggerAsyncUpdate();
}

// ---------------------------------------------------------------- estado
void RefProProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("Captures"), nullptr);

    juce::ValueTree caps ("Captures");
    {
        const juce::ScopedLock sl (capLock);
        caps.setProperty ("counter", captureCounter, nullptr);
        caps.setProperty ("favorites", favorites.joinIntoString ("|"), nullptr);
        for (auto& c : captured)
        {
            juce::ValueTree v ("Capture");
            v.setProperty ("name", c.name, nullptr);
            v.setProperty ("psr", c.psr, nullptr);
            v.setProperty ("width", c.widthDb, nullptr);
            v.setProperty ("vocal", c.vocalDb, nullptr);
            juce::StringArray ce, to;
            for (int i = 0; i < rp::numBands; ++i)
            {
                ce.add (juce::String (c.centre[(size_t) i], 2));
                to.add (juce::String (c.tol[(size_t) i], 2));
            }
            v.setProperty ("centre", ce.joinIntoString (" "), nullptr);
            v.setProperty ("tol", to.joinIntoString (" "), nullptr);
            caps.appendChild (v, nullptr);
        }
    }
    state.appendChild (caps, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void RefProProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    auto vt = juce::ValueTree::fromXml (*xml);
    {
        const juce::ScopedLock sl (capLock);
        captured.clear();
        auto caps = vt.getChildWithName ("Captures");
        captureCounter = (int) caps.getProperty ("counter", 0);
        favorites = juce::StringArray::fromTokens (caps.getProperty ("favorites").toString(), "|", "");
        favorites.removeEmptyStrings();
        for (auto v : caps)
        {
            if ((int) captured.size() >= maxCaptures) break;
            TargetData t;
            t.name = v.getProperty ("name", "Capture").toString();
            t.psr = (float) v.getProperty ("psr", 11.0);
            t.widthDb = (float) v.getProperty ("width", -8.0);
            t.vocalDb = (float) v.getProperty ("vocal", -6.0);
            t.builtIn = false;
            auto ce = juce::StringArray::fromTokens (v.getProperty ("centre").toString(), " ", "");
            auto to = juce::StringArray::fromTokens (v.getProperty ("tol").toString(), " ", "");
            for (int i = 0; i < rp::numBands; ++i)
            {
                t.centre[(size_t) i] = i < ce.size() ? ce[i].getFloatValue() : 0.0f;
                t.tol[(size_t) i]    = i < to.size() ? to[i].getFloatValue() : 1.5f;
            }
            captured.push_back (t);
        }
        vt.removeChild (caps, nullptr);
    }
    apvts.replaceState (vt);
    ++targetsVersion;
    triggerAsyncUpdate();
}

juce::AudioProcessorEditor* RefProProcessor::createEditor() { return new RefProEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RefProProcessor(); }
