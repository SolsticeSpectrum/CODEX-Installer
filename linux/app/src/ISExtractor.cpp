#include "ISExtractor.h"
#include <SDL.h>


void ISExtractor::SetSource(const std::string &archivePath) { FArchive = archivePath; }
void ISExtractor::SetTarget(const std::string &installDir)  { FTarget = installDir; }
void ISExtractor::SetOnProgress(ProgressCallback cb)        { FOnProgress = std::move(cb); }
void ISExtractor::SetOnFinish(FinishCallback cb)            { FOnFinish = std::move(cb); }


void ISExtractor::Start() {
    FRunning      = true;
    FPaused       = false;
    FMockProgress = 0;
    FMockFile     = 0;
    FLastTick     = SDL_GetTicks();
}


void ISExtractor::Pause()  { FPaused = true; }
void ISExtractor::Resume() { FPaused = false; }

void ISExtractor::Cancel() {
    FRunning = false;
    FPaused  = false;
}


// TODO: replace mockup with threaded extraction
// read .bin archive, extract entries to FTarget
// call FOnProgress(percent, filename) per file
// call FOnFinish(success) when done
void ISExtractor::Tick() {
    if (!FRunning || FPaused) return;

    uint32_t now = SDL_GetTicks();
    if (now - FLastTick < 50) return;
    FLastTick = now;

    FMockProgress += 5;

    if (FMockProgress % 10 == 0) {
        FMockFile++;
        if (FOnProgress)
            FOnProgress(FMockProgress, "Extracting file " + std::to_string(FMockFile) + " of 100...");
    }

    if (FMockProgress > 1000) {
        FRunning = false;
        if (FOnFinish) FOnFinish(true);
    }
}
