#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <cstdint>


// TODO: replace mockup with actual .bin archive extraction
// should support: 7z, RAR, ISArc, precomp, srep, xdelta
class ISExtractor {
public:
    using ProgressCallback = std::function<void(int percent, const std::string &file)>;
    using FinishCallback   = std::function<void(bool success)>;

    void SetSource(const std::string &archivePath);
    void SetTarget(const std::string &installDir);
    void SetOnProgress(ProgressCallback cb);
    void SetOnFinish(FinishCallback cb);

    void Start();
    void Pause();
    void Resume();
    void Cancel();

    // call from main loop each frame
    void Tick();

    bool IsRunning() const  { return FRunning; }
    bool IsPaused() const   { return FPaused; }

private:
    std::string FArchive;
    std::string FTarget;
    ProgressCallback FOnProgress;
    FinishCallback   FOnFinish;

    std::atomic<bool> FRunning = false;
    std::atomic<bool> FPaused  = false;

    // mockup state
    int  FMockProgress = 0;
    int  FMockFile     = 0;
    uint32_t FLastTick = 0;
};
