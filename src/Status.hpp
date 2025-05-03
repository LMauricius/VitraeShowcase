#pragma once

#include <chrono>

#include "MMeter.h"

using namespace std::chrono_literals;

struct Status
{
    std::chrono::duration<double> totalSumFrameDuration;
    std::size_t totalFrameCount;
    std::chrono::duration<double> totalAvgFrameDuration;
    float totalFPS;

    std::chrono::duration<double> currentAvgFrameDuration;
    float currentFPS;
    std::chrono::steady_clock::time_point currentTimeStamp;

    std::chrono::duration<double> trackingSumFrameDuration;
    std::size_t trackingFrameCount;

    std::chrono::duration<double> pipelineSumFrameDuration;
    std::chrono::duration<double> pipelineAvgFrameDuration;
    std::size_t pipelineFrameCount;
    float pipelineFPS;

    std::string mmeterMetrics;
    MMeter::FuncProfilerTree aggregateTree;

    Status();
    ~Status() = default;

    void update(std::chrono::duration<float> lastFrameDuration);
    void resetPipeline();
};