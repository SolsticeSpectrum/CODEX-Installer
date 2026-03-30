#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>


// TODO: implement actual .bin archive extraction (ISDone.dll equivalent)
// should support: 7z, RAR, ISArc, precomp, srep, xdelta
// progress callback reports current file + overall percentage
class ISExtractor {
public:
    using ProgressCallback = std::function<void(int percent, const std::string &file)>;

    void SetSource(const std::string &archivePath);
    void SetTarget(const std::string &installDir);
    void SetCallback(ProgressCallback cb);

    // TODO: run extraction in a thread, call callback on progress
    void Start();
    void Pause();
    void Resume();
    void Cancel();

    bool IsRunning() const  { return FRunning; }
    bool IsFinished() const { return FFinished; }
    bool HasError() const   { return FError; }

private:
    std::string FArchive;
    std::string FTarget;
    ProgressCallback FCallback;

    std::atomic<bool> FRunning  = false;
    std::atomic<bool> FPaused   = false;
    std::atomic<bool> FCancelled = false;
    std::atomic<bool> FFinished = false;
    std::atomic<bool> FError    = false;
};
