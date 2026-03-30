#include "ISExtractor.h"


void ISExtractor::SetSource(const std::string &archivePath) { FArchive = archivePath; }
void ISExtractor::SetTarget(const std::string &installDir)  { FTarget = installDir; }
void ISExtractor::SetCallback(ProgressCallback cb)          { FCallback = std::move(cb); }


void ISExtractor::Start() {
    // TODO: spawn thread, iterate archive entries, extract to FTarget
    // call FCallback(percent, currentFile) on each file
    // support FCancelled / FPaused flags
    // on completion set FFinished = true
    // on error set FError = true
    FRunning  = true;
    FFinished = false;
    FError    = false;
}


void ISExtractor::Pause()  { FPaused = true; }
void ISExtractor::Resume() { FPaused = false; }
void ISExtractor::Cancel() { FCancelled = true; }
