#pragma once

// pipeline chaining for "rep+lzma" etc

#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>

#include "Compression/Compression.h"

struct MCParam {
    std::thread        thr;
    int                idx, total;
    char              *method;
    CALLBACK_FUNC     *origCb;
    void              *origAux;

    // inter-thread pipe
    uint8_t           *pipeBuf;
    int                pipeLen; // -1 = EOF
    std::mutex         mtx;
    std::condition_variable cvR, cvW;
    bool               rReady, wReady;
};

static int mcCallback(const char *what, void *data, int size, void *aux) {
    auto *p = (MCParam*)aux;

    if (strcmp(what,"write")==0 && p->idx < p->total-1) {
        // pass to next thread
        p->pipeBuf = (uint8_t*)data;
        p->pipeLen = size;
        { std::lock_guard<std::mutex> lk(p->mtx); p->rReady = true; }
        p->cvR.notify_one();
        std::unique_lock<std::mutex> lk(p->mtx);
        p->cvW.wait(lk, [p]{ return p->wReady; });
        p->wReady = false;
        return p->pipeLen < 0 ? FREEARC_ERRCODE_WRITE : size;
    }
    else if (strcmp(what,"read")==0 && p->idx > 0) {
        auto *prev = p-1;
        int total = 0;
        auto *dst = (uint8_t*)data;
        while (total < size) {
            std::unique_lock<std::mutex> lk(prev->mtx);
            prev->cvR.wait(lk, [prev]{ return prev->rReady; });
            if (prev->pipeLen < 0) { prev->rReady = true; prev->cvR.notify_one(); return total; }
            int n = std::min(size-total, prev->pipeLen);
            memcpy(dst+total, prev->pipeBuf, n);
            prev->pipeBuf += n;
            prev->pipeLen -= n;
            total += n;
            if (prev->pipeLen == 0) {
                prev->rReady = false;
                prev->wReady = true;
                prev->cvW.notify_one();
            } else {
                prev->rReady = true; prev->cvR.notify_one();
            }
        }
        return total;
    }
    else {
        return p->origCb(what, data, size, p->origAux);
    }
}

static int MultiCompress(char *str, CALLBACK_FUNC *cb, void *aux) {
    char *m = strdup(str);
    char *methods[32]; int N = 0;
    for (char *t = strtok(m,"+"); t && N < 32; t = strtok(nullptr,"+"))
        methods[N++] = t;
    if (N <= 1) { int r = Compress(m, cb, aux); free(m); return r; }

    MCParam params[32];
    std::atomic<int> ret{0};
    for (int i = 0; i < N; i++) {
        params[i].idx = i; params[i].total = N;
        params[i].method = methods[i];
        params[i].origCb = cb; params[i].origAux = aux;
        params[i].rReady = false; params[i].wReady = false;
    }
    for (int i = 0; i < N; i++) {
        params[i].thr = std::thread([&params,i,&ret](){
            int r = Compress(params[i].method, mcCallback, &params[i]);
            if (r < 0) ret = r;
            if (i < params[i].total-1) {
                std::lock_guard<std::mutex> lk(params[i].mtx);
                params[i].pipeLen = -1;
                params[i].rReady = true;
                params[i].cvR.notify_one();
            }
        });
    }
    for (int i = 0; i < N; i++) params[i].thr.join();
    free(m);
    return ret;
}
