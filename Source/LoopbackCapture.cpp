#include "LoopbackCapture.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>

#if defined (_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
 #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
 #define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

#if defined (_MSC_VER)
 #pragma comment (lib, "ole32.lib")
#endif

namespace
{
    template <typename T>
    void safeRelease (T*& p) { if (p != nullptr) { p->Release(); p = nullptr; } }

    std::string wideToUtf8 (const wchar_t* w)
    {
        if (w == nullptr) return {};
        const int len = WideCharToMultiByte (CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1) return {};
        std::string s ((size_t) len - 1, '\0');
        WideCharToMultiByte (CP_UTF8, 0, w, -1, &s[0], len, nullptr, nullptr);
        return s;
    }

    // Uma sessão de captura em um dispositivo de saída (render endpoint).
    struct Session
    {
        IMMDeviceEnumerator* enumerator = nullptr;
        IMMDevice* device = nullptr;
        IAudioClient* client = nullptr;
        IAudioCaptureClient* capture = nullptr;
        WAVEFORMATEX* fmt = nullptr;
        std::wstring deviceId;
        std::string name;
        bool isFloat = true;
        int bits = 32, channels = 2, bytesPerFrame = 8;
        double rate = 48000.0;

        ~Session() { close(); }

        void close()
        {
            if (client != nullptr) client->Stop();
            safeRelease (capture);
            safeRelease (client);
            if (fmt != nullptr) { CoTaskMemFree (fmt); fmt = nullptr; }
            safeRelease (device);
            safeRelease (enumerator);
        }

        // Id do dispositivo de saída padrão atual (vazio se não houver)
        static std::wstring currentDefaultId (IMMDeviceEnumerator* en)
        {
            std::wstring out;
            IMMDevice* d = nullptr;
            if (SUCCEEDED (en->GetDefaultAudioEndpoint (eRender, eConsole, &d)) && d != nullptr)
            {
                LPWSTR id = nullptr;
                if (SUCCEEDED (d->GetId (&id)) && id != nullptr) { out = id; CoTaskMemFree (id); }
                d->Release();
            }
            return out;
        }

        bool open (std::string& err)
        {
            close();
            HRESULT hr = CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                           __uuidof (IMMDeviceEnumerator), (void**) &enumerator);
            if (FAILED (hr)) { err = "Could not create the audio device enumerator."; return false; }

            hr = enumerator->GetDefaultAudioEndpoint (eRender, eConsole, &device);
            if (FAILED (hr) || device == nullptr) { err = "No audio output device found."; return false; }

            LPWSTR id = nullptr;
            if (SUCCEEDED (device->GetId (&id)) && id != nullptr) { deviceId = id; CoTaskMemFree (id); }

            IPropertyStore* props = nullptr;
            if (SUCCEEDED (device->OpenPropertyStore (STGM_READ, &props)) && props != nullptr)
            {
                PROPVARIANT v;
                PropVariantInit (&v);
                if (SUCCEEDED (props->GetValue (PKEY_Device_FriendlyName, &v)) && v.vt == VT_LPWSTR)
                    name = wideToUtf8 (v.pwszVal);
                PropVariantClear (&v);
                props->Release();
            }

            hr = device->Activate (__uuidof (IAudioClient), CLSCTX_ALL, nullptr, (void**) &client);
            if (FAILED (hr) || client == nullptr) { err = "Could not open the audio output device."; return false; }

            hr = client->GetMixFormat (&fmt);
            if (FAILED (hr) || fmt == nullptr) { err = "Could not read the output device format."; return false; }

            channels = std::max<int> (1, fmt->nChannels);
            bits = fmt->wBitsPerSample;
            bytesPerFrame = fmt->nBlockAlign;
            rate = (double) fmt->nSamplesPerSec;

            WORD tag = fmt->wFormatTag;
            if (tag == WAVE_FORMAT_EXTENSIBLE && fmt->cbSize >= 22)
                tag = (WORD) reinterpret_cast<WAVEFORMATEXTENSIBLE*> (fmt)->SubFormat.Data1;   // 1 = PCM, 3 = float
            isFloat = (tag == WAVE_FORMAT_IEEE_FLOAT);
            if (! isFloat && tag != WAVE_FORMAT_PCM) { err = "Unsupported output format."; return false; }
            if (isFloat && bits != 32) { err = "Unsupported output format."; return false; }
            if (! isFloat && bits != 16 && bits != 24 && bits != 32) { err = "Unsupported output format."; return false; }

            // 200 ms de buffer, modo compartilhado + LOOPBACK (captura o que está sendo reproduzido)
            hr = client->Initialize (AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 2000000, 0, fmt, nullptr);
            if (FAILED (hr)) { err = "Could not start system audio capture."; return false; }

            hr = client->GetService (__uuidof (IAudioCaptureClient), (void**) &capture);
            if (FAILED (hr) || capture == nullptr) { err = "Could not get the capture service."; return false; }

            hr = client->Start();
            if (FAILED (hr)) { err = "Could not start the capture stream."; return false; }
            return true;
        }

        float sampleAt (const BYTE* frame, int ch) const
        {
            const BYTE* p = frame + (size_t) ch * (size_t) (bits / 8);
            if (isFloat) { float f; std::memcpy (&f, p, 4); return f; }
            if (bits == 16) { short s; std::memcpy (&s, p, 2); return (float) s / 32768.0f; }
            if (bits == 24)
            {
                int v = (int) p[0] | ((int) p[1] << 8) | ((int) p[2] << 16);
                if (v & 0x800000) v |= ~0xffffff;
                return (float) v / 8388608.0f;
            }
            int v; std::memcpy (&v, p, 4);
            return (float) ((double) v / 2147483648.0);
        }
    };
}

bool LoopbackCapture::isSupported() { return true; }

bool LoopbackCapture::start (Callback cb, std::string& error)
{
    stop();
    callback = std::move (cb);
    stopFlag.store (false);
    running.store (false);
    { std::lock_guard<std::mutex> lk (nameLock); lastError.clear(); deviceName.clear(); }

    worker = std::thread ([this] { threadMain(); });

    // Espera a thread abrir o dispositivo (ou falhar) por até ~3 s
    for (int i = 0; i < 300; ++i)
    {
        if (running.load()) return true;
        { std::lock_guard<std::mutex> lk (nameLock); if (! lastError.empty()) break; }
        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    if (! running.load())
    {
        { std::lock_guard<std::mutex> lk (nameLock); error = lastError.empty() ? "Timed out while opening the audio device." : lastError; }
        stop();
        return false;
    }
    return true;
}

void LoopbackCapture::stop()
{
    stopFlag.store (true);
    if (worker.joinable()) worker.join();
    running.store (false);
    sampleRate.store (0.0);
}

std::string LoopbackCapture::getDeviceName() const
{
    std::lock_guard<std::mutex> lk (nameLock);
    return deviceName;
}

void LoopbackCapture::threadMain()
{
    const HRESULT coHr = CoInitializeEx (nullptr, COINIT_MULTITHREADED);
    const bool coOk = SUCCEEDED (coHr);

    std::vector<float> L, R;
    bool firstTry = true;

    while (! stopFlag.load())
    {
        Session s;
        std::string err;
        if (! s.open (err))
        {
            if (firstTry) { std::lock_guard<std::mutex> lk (nameLock); lastError = err; break; }   // 1ª tentativa: avisa o usuário
            running.store (false);                                                                  // depois: tenta de novo (troca de dispositivo)
            for (int i = 0; i < 100 && ! stopFlag.load(); ++i) std::this_thread::sleep_for (std::chrono::milliseconds (10));
            continue;
        }
        firstTry = false;
        sampleRate.store (s.rate);
        { std::lock_guard<std::mutex> lk (nameLock); deviceName = s.name; }
        running.store (true);

        int ticks = 0;
        bool restart = false;
        while (! stopFlag.load() && ! restart)
        {
            std::this_thread::sleep_for (std::chrono::milliseconds (10));

            // a cada ~1 s confere se o dispositivo de saída padrão mudou (ex.: fones conectados)
            if (++ticks >= 100)
            {
                ticks = 0;
                const auto cur = Session::currentDefaultId (s.enumerator);
                if (! cur.empty() && cur != s.deviceId) { restart = true; break; }
            }

            UINT32 packet = 0;
            HRESULT hr = s.capture->GetNextPacketSize (&packet);
            while (SUCCEEDED (hr) && packet > 0 && ! stopFlag.load())
            {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                hr = s.capture->GetBuffer (&data, &frames, &flags, nullptr, nullptr);
                if (FAILED (hr) || frames == 0) break;

                if ((int) frames > (int) L.size()) { L.resize (frames); R.resize (frames); }
                if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr)
                {
                    std::fill (L.begin(), L.begin() + frames, 0.0f);
                    std::fill (R.begin(), R.begin() + frames, 0.0f);
                }
                else
                {
                    for (UINT32 i = 0; i < frames; ++i)
                    {
                        const BYTE* fr = data + (size_t) i * (size_t) s.bytesPerFrame;
                        const float l = s.sampleAt (fr, 0);
                        L[i] = l;
                        R[i] = s.channels > 1 ? s.sampleAt (fr, 1) : l;
                    }
                }
                s.capture->ReleaseBuffer (frames);

                if (callback) callback (L.data(), R.data(), (int) frames, s.rate);
                hr = s.capture->GetNextPacketSize (&packet);
            }
            if (FAILED (hr)) restart = true;       // dispositivo invalidado: reabre
        }
        running.store (false);
    }

    running.store (false);
    if (coOk) CoUninitialize();
}

#else   // ------------------------------------------------------------ fora do Windows

bool LoopbackCapture::isSupported() { return false; }

bool LoopbackCapture::start (Callback, std::string& error)
{
    error = "System audio capture is only available on Windows.";
    return false;
}

void LoopbackCapture::stop()
{
    stopFlag.store (true);
    if (worker.joinable()) worker.join();
    running.store (false);
}

std::string LoopbackCapture::getDeviceName() const { return {}; }
void LoopbackCapture::threadMain() {}

#endif
