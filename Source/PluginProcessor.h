#pragma once

#include "Common.h"
#include "Analyzer.h"
#include "Targets.h"
#include "Eq.h"
#include "LoopbackCapture.h"

class RefProProcessor : public juce::AudioProcessor,
                        private juce::AsyncUpdater,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    RefProProcessor();
    ~RefProProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RefPro SILVER"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- alvos ---------------------------------------------------------
    static constexpr int maxCaptures = 32;
    static constexpr int blendIndex  = maxCaptures + 1;       // alvo especial "Blend" (mistura de dois alvos)
    enum class CaptureResult { ok, noData, full };

    int        getNumTargets() const;
    TargetData getTarget (int index) const;
    CaptureResult captureCurrent();            // thread de mensagem
    void       deleteCapture (int targetIndex);
    bool       renameTarget (int targetIndex, const juce::String& newName);
    bool       exportTarget (int targetIndex, const juce::File& file) const;
    int        importTargetFile (const juce::File& file);      // devolve o índice novo ou -1
    int        addTarget (TargetData t);                       // devolve o índice novo ou -1 (cheio)
    bool       isFavorite (const juce::String& name) const;
    void       setFavorite (const juce::String& name, bool fav);
    std::atomic<int> targetsVersion { 0 }, blendVersion { 0 };
    int        consumeNewIndex() { return newIndex.exchange (-1); }

    // análise de arquivos de áudio -> novo alvo (thread em segundo plano)
    void startBuild (const juce::Array<juce::File>& files, const juce::String& name);
    void cancelBuild();
    juce::String getBuildMessage() const;
    std::atomic<bool> buildBusy { false };
    std::atomic<float> buildProgress { 0.0f };
    void finishBuild (const TargetData& t, const juce::String& message);   // chamado pela thread

    // --- equalizador ---------------------------------------------------
    static juce::String bandId (int band, const char* what) { return "b" + juce::String (band + 1) + "_" + what; }
    Eq::Params getBandParams (int band) const;
    std::array<std::atomic<float>, Eq::numBands> bandDynGain;   // ganho dinâmico atual de cada banda (dB)
    std::atomic<int> soloBand { -1 };                           // banda em solo (-1 = nenhuma)
    void abSwitch (int slot);                                   // A/B do equalizador
    int  abSlot() const { return abCurrent; }

    // --- EQ dinâmico automático ("seguir a curva") -----------------------
    static constexpr int dynBands = 31;                       // 1/3 de oitava, 20 Hz .. 20 kHz
    static float dynFreq (int j) { return 20.0f * std::pow (2.0f, (float) j / 3.0f); }
    std::array<std::atomic<float>, dynBands> dynGainDb;       // ganho aplicado por banda (dB; < 0 corte, > 0 boost)
    std::array<std::atomic<float>, dynBands> dynRefShift;     // quanto a referência "se moveu" por banda (dB)
    std::atomic<float> dynCompOut { 0.0f };                   // compensação de nível aplicada (dB)
    std::atomic<float> vocRideOut { 0.0f };                   // ajuste automático da voz aplicado (dB)

    juce::AudioProcessorValueTreeState apvts;
    TonalAnalyzer analyzer;

    // Modo "áudio do sistema": analisa o que toca no Spotify/YouTube (loopback WASAPI, só Windows). Não grava nada.
    bool startLoopback (juce::String& error);
    void stopLoopback();
    bool isLoopbackOn() const            { return loopbackOn.load(); }
    bool isLoopbackAvailable() const     { return LoopbackCapture::isSupported(); }
    juce::String getLoopbackDevice() const { return juce::String (loopback.getDeviceName()); }
    bool isStandaloneApp() const         { return wrapperType == wrapperType_Standalone; }

private:
    LoopbackCapture loopback;
    std::atomic<bool> loopbackOn { false };
    juce::SpinLock analyzerLock;
    double loopbackRate = 0.0;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleAsyncUpdate() override;
    void parameterChanged (const juce::String& id, float) override { if (id.startsWith ("blend")) ++blendVersion; triggerAsyncUpdate(); }
    TargetData getTargetNoBlend (int index) const;
    TargetData makeBlend() const;
    juce::String uniqueName (const juce::String& wanted) const;
    void runEq (juce::AudioBuffer<float>&, int numSamples, int numCh);
    void publishDynTarget();
    void runDynamic (juce::AudioBuffer<float>&, int numSamples, int numCh);
    void runDynEq (juce::AudioBuffer<float>&, int numSamples, int numCh, bool dynOn);
    void runVocalRide (juce::AudioBuffer<float>&, int numSamples, int numCh, bool rideOn);

    mutable juce::CriticalSection capLock;
    std::vector<TargetData> captured;
    int captureCounter = 0;
    TargetData allPurpose;
    juce::StringArray favorites;
    std::atomic<int> newIndex { -1 };
    struct Builder;
    std::unique_ptr<Builder> builder;
    mutable juce::CriticalSection pendingLock;
    TargetData pendingTarget;
    bool hasPending = false;
    juce::String buildMessage;
    std::atomic<float>* blendAParam = nullptr;
    std::atomic<float>* blendBParam = nullptr;
    std::atomic<float>* blendMixParam = nullptr;
    std::atomic<float>* outGainParam = nullptr;
    std::atomic<float>* avgParam = nullptr;
    std::atomic<float>* eqOnParam = nullptr;
    std::atomic<float>* postParam = nullptr;
    std::array<std::atomic<float>*, Eq::numBands> pOn {}, pType {}, pFreq {}, pGain {}, pQ {}, pDyn {}, pThresh {}, pRatio {}, pSlope {}, pMode {};

    struct BandState
    {
        Eq::Params last;
        Eq::Cascade cas;
        bool active = false;
        double z1[4][2] {}, z2[4][2] {};
        Eq::Coeffs bp;                        // detector da banda dinâmica
        double bz1 = 0.0, bz2 = 0.0, env = 0.0, gSmooth = 0.0, coefGain = 0.0;
        void resetStates()
        {
            for (int i = 0; i < 4; ++i) for (int c = 0; c < 2; ++c) { z1[i][c] = 0.0; z2[i][c] = 0.0; }
            bz1 = bz2 = env = gSmooth = 0.0;
        }
    };
    static void applyBand (BandState& st, int mode, float* d0, float* d1, int len);
    std::array<BandState, Eq::numBands> eq;
    TonalAnalyzer detector;                                   // mede a entrada do estágio dinâmico
    std::array<std::atomic<float>, rp::numBands> dynTc, dynTt;   // alvo atual (centro / tolerância)
    std::atomic<float> *dynOnParam = nullptr, *dynAmountParam = nullptr, *dynMaxParam = nullptr,
                       *dynTolParam = nullptr, *dynSpeedParam = nullptr, *dynLoParam = nullptr, *dynHiParam = nullptr,
                       *dynBoostParam = nullptr, *dynAdaptParam = nullptr, *dynCompParam = nullptr;
    struct DynBand
    {
        Eq::Coeffs c;
        double z1[2] { 0, 0 }, z2[2] { 0, 0 };
        float gainDb = 0.0f, coefGain = 0.0f;
        bool active = false;
    };
    std::array<DynBand, dynBands> dyn;
    bool dynWasOn = false, detWasOn = false;
    std::atomic<float> dynTv { -6.0f };                       // alvo do Vocal Balance (dB)
    std::atomic<float> *vocRideParam = nullptr, *vocMaxParam = nullptr;
    struct RideBq { Eq::Coeffs c; double z1 = 0.0, z2 = 0.0; };
    std::array<RideBq, 2> ride;
    float rideDb = 0.0f, rideCoefDb = 999.0f;
    bool rideActive = false;
    float dynCompDb = 0.0f, dynCompLin = 1.0f;
    double currentSampleRate = 44100.0;
    bool forceEqUpdate = true;

    // A/B, solo e ganho de saída do EQ
    std::vector<juce::String> eqParamIds;
    std::array<std::vector<float>, 2> abSlots;
    int abCurrent = 0;
    juce::AudioBuffer<float> soloBuf;
    double soloZ1[2] { 0, 0 }, soloZ2[2] { 0, 0 };
    int lastSolo = -1;
    float outGainLin = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RefProProcessor)
};
