#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// Captura o áudio que sai do computador (loopback WASAPI do dispositivo de saída padrão do Windows).
// Serve para MEDIR o que toca no Spotify, YouTube, navegador etc. O áudio NÃO é gravado nem salvo:
// cada bloco é entregue ao callback e descartado.
// Fora do Windows a classe existe, mas start() falha com uma mensagem (isSupported() == false).
class LoopbackCapture
{
public:
    // l / r: amostras float (-1..1), n quadros, sampleRate do dispositivo
    using Callback = std::function<void (const float* l, const float* r, int n, double sampleRate)>;

    LoopbackCapture() = default;
    ~LoopbackCapture() { stop(); }

    static bool isSupported();

    // Inicia a thread de captura. Devolve false (e preenche error) se não for possível.
    bool start (Callback cb, std::string& error);
    void stop();

    bool isRunning() const        { return running.load(); }
    double getSampleRate() const  { return sampleRate.load(); }
    std::string getDeviceName() const;

private:
    void threadMain();

    Callback callback;
    std::thread worker;
    std::atomic<bool> stopFlag { false }, running { false };
    std::atomic<double> sampleRate { 0.0 };
    mutable std::mutex nameLock;
    std::string deviceName, lastError;
};
