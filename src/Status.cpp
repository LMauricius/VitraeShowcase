#include "Status.hpp"

Status::Status()
    : totalSumFrameDuration(0.0s), totalFrameCount(0), totalAvgFrameDuration(0.0s), totalFPS(0.0f),
      currentAvgFrameDuration(0.0s), currentFPS(0.0f),
      currentTimeStamp(std::chrono::steady_clock::now()), trackingSumFrameDuration(0.0s),
      trackingFrameCount(0), pipelineSumFrameDuration(0.0s), pipelineAvgFrameDuration(0.0s),
      pipelineFrameCount(0),
      pipelineFPS(0.0f)
{}

void Status::update(std::chrono::duration<float> lastFrameDuration)
{
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    totalSumFrameDuration += lastFrameDuration;
    totalFrameCount++;
    totalAvgFrameDuration = totalSumFrameDuration / totalFrameCount;
    totalFPS = 1.0f / totalAvgFrameDuration.count();

    trackingSumFrameDuration += lastFrameDuration;
    trackingFrameCount++;

    pipelineSumFrameDuration += lastFrameDuration;
    pipelineFrameCount++;
    pipelineAvgFrameDuration = pipelineSumFrameDuration / (double)pipelineFrameCount;
    pipelineFPS = 1.0f / pipelineAvgFrameDuration.count();

    if (now - currentTimeStamp >= 1s) {
        currentAvgFrameDuration = trackingSumFrameDuration / trackingFrameCount;
        currentFPS = 1.0f / currentAvgFrameDuration.count();
        trackingSumFrameDuration = 0s;
        trackingFrameCount = 0;
        currentTimeStamp = now;

        std::stringstream ss;
        ss << "Current:" << std::endl;
        MMeter::getThreadLocalTreePtr()->outputBranchPercentagesToOStream(ss);

        aggregateTree.merge(*MMeter::getThreadLocalTreePtr());
        MMeter::getThreadLocalTreePtr()->reset();

        ss << "Total:" << std::endl
           << aggregateTree;

        ss << "Total flat:" << std::endl
           << aggregateTree.totalsByDurationStr() << "\n\n\n"
           << std::endl;

        mmeterMetrics = ss.str();
    }
}

void Status::resetPipeline() {
    pipelineSumFrameDuration = 0s;
    pipelineFrameCount = 0;
    aggregateTree.reset();
}
