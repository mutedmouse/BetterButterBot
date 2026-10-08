#include "IRSignalProc.h"
#include <algorithm>
#include <vector>
#include "PSRAMAllocator.h"
#include <cstring>
#include <cstdio>
#include <Log/Log.h>

DEFINE_LOG(IRSignalProc)

static constexpr uint32_t CarrierGapMaxUs = 100;       // ends an envelope mark (carrier spaces ~16 µs, envelope spaces >=400 µs)
static constexpr uint32_t DurationTolerancePct = 25;   // frame similarity, per symbol
static constexpr uint32_t DurationToleranceFloorUs = 200;
static constexpr uint32_t SnapGroupTolerancePct = 20;  // duration histogram grouping
static constexpr uint16_t MinDurationTicks = 2;        // a zero mid-frame duration terminates RMT TX
static constexpr uint32_t MinPeriodUs = 15000;
static constexpr uint32_t MaxPeriodUs = 250000;        // longer gaps mean separate presses, not repeats
static constexpr uint32_t FrameIdleThresholdUs = 10000; // signal_range_max_ns in RM_IRModule
static constexpr uint16_t PlausibleMinSymbols = 8;     // no supported protocol frame is shorter, unless repeated (RC5: 7)
static constexpr uint16_t StubMaxSymbols = 4;          // repeated frames this short are repeat stubs (NEC: 2), not commands
static constexpr uint16_t MinMarkUs = 150;             // shortest real protocol envelope mark is ~414 µs (RC6)
static constexpr uint16_t MaxDistinctRepeatSymbols = 32; // distinct repeat frames (e.g. NEC's) are short stubs
static constexpr uint8_t IdenticalRepeatSends = 5;     // frames per press for identical-repeat protocols (SIRC needs >=3)

// Demodulated view of a capture; mirrors IRCapture but with envelope frames.
struct EnvCapture {
    IRSignal frames[IRMaxCaptureFrames];
    int64_t doneTimestampsUs[IRMaxCaptureFrames];
    uint8_t rawIndex[IRMaxCaptureFrames]; // position in the raw capture, for consecutiveness checks
    uint8_t count = 0;
};

struct CarrierStats {
    uint64_t periodSumUs = 0;
    uint64_t onSumUs = 0;
    uint32_t cycles = 0;
};

// Merges carrier cycles into envelope marks (level 1 — TX modulates the carrier onto the high level).
static bool demodulate(const IRRawFrame& raw, IRSignal& env, CarrierStats& stats){
    env.count = 0;
    if(raw.count == 0 || raw.count >= IRRawMaxSymbols){
        // full buffer = truncated frame
        return false;
    }

    uint32_t markUs = 0;
    for(uint16_t i = 0; i < raw.count; ++i){
        const uint32_t d0 = raw.symbols[i].duration0;
        const uint32_t d1 = raw.symbols[i].duration1;
        const bool last = (i + 1 == raw.count) || d1 == 0;

        if(!last && d1 < CarrierGapMaxUs){
            markUs += d0 + d1;
            stats.periodSumUs += d0 + d1;
            stats.onSumUs += d0;
            stats.cycles++;
            continue;
        }

        markUs += d0;
        if(env.count >= IRMaxSymbols){
            return false; // more envelope symbols than any supported protocol — noise or unsupported
        }
        env.symbols[env.count].level0 = 1;
        env.symbols[env.count].duration0 = static_cast<uint16_t>(std::min<uint32_t>(markUs, 32767));
        env.symbols[env.count].level1 = 0;
        env.symbols[env.count].duration1 = last ? 0 : static_cast<uint16_t>(std::min<uint32_t>(d1, 32767));
        env.count++;
        markUs = 0;

        if(last){
            break;
        }
    }

    return env.count > 0;
}

static uint32_t frameDurationUs(const IRSignal& frame){
    uint32_t total = 0;
    for(uint16_t i = 0; i < frame.count; ++i){
        total += frame.symbols[i].duration0 + frame.symbols[i].duration1;
    }
    return total;
}

static bool durationsSimilar(uint32_t a, uint32_t b){
    const uint32_t tolerance = std::max(DurationToleranceFloorUs, std::max(a, b) * DurationTolerancePct / 100);
    return (a > b ? a - b : b - a) <= tolerance;
}

static bool framesSimilar(const IRSignal& a, const IRSignal& b){
    if(a.count != b.count || a.count == 0){
        return false;
    }
    for(uint16_t i = 0; i < a.count; ++i){
        if(!durationsSimilar(a.symbols[i].duration0, b.symbols[i].duration0)){
            return false;
        }
        // The final space is truncated by the idle threshold — don't compare it.
        if(i + 1 < a.count && !durationsSimilar(a.symbols[i].duration1, b.symbols[i].duration1)){
            return false;
        }
    }
    return true;
}

static bool frameLooksValid(const IRSignal& frame){
    if(frame.count == 0){
        return false;
    }
    for(uint16_t i = 0; i < frame.count; ++i){
        if(frame.symbols[i].duration0 < MinMarkUs){
            return false;
        }
    }
    return true;
}

// Averages per-position durations of all cluster members; levels come from the first member.
static void averageCluster(const EnvCapture& cap, const uint8_t* clusterOf, uint8_t cluster,
                           rmt_symbol_word_t* out, uint16_t& outCount){
    int firstMember = -1;
    for(uint8_t i = 0; i < cap.count; ++i){
        if(clusterOf[i] == cluster){
            firstMember = i;
            break;
        }
    }
    if(firstMember < 0){
        outCount = 0;
        return;
    }

    const uint16_t count = cap.frames[firstMember].count;
    outCount = count;

    for(uint16_t s = 0; s < count; ++s){
        uint32_t sum0 = 0, sum1 = 0, members = 0;
        for(uint8_t i = 0; i < cap.count; ++i){
            if(clusterOf[i] != cluster) continue;
            sum0 += cap.frames[i].symbols[s].duration0;
            sum1 += cap.frames[i].symbols[s].duration1;
            members++;
        }
        out[s].level0 = cap.frames[firstMember].symbols[s].level0;
        out[s].level1 = cap.frames[firstMember].symbols[s].level1;
        out[s].duration0 = static_cast<uint16_t>(std::min<uint32_t>(sum0 / members, 32767));
        out[s].duration1 = static_cast<uint16_t>(std::min<uint32_t>(sum1 / members, 32767));
    }

    // the final space was truncated by the idle threshold anyway
    out[count - 1].duration1 = 0;
}

// Snaps each duration to the mean of its histogram group.
static void snapDurations(rmt_symbol_word_t* symbols, uint16_t count){
    PSRAMVector<uint16_t> values;
    values.reserve(count * 2);
    for(uint16_t i = 0; i < count; ++i){
        if(symbols[i].duration0 > 0) values.push_back(symbols[i].duration0);
        if(symbols[i].duration1 > 0) values.push_back(symbols[i].duration1);
    }
    if(values.empty()) return;
    std::sort(values.begin(), values.end());

    struct Group {
        uint32_t min, max, sum, memberCount;
    };
    PSRAMVector<Group> groups;
    for(const uint16_t v : values){
        if(!groups.empty()){
            Group& g = groups.back();
            const uint32_t mean = g.sum / g.memberCount;
            if(v <= mean + mean * SnapGroupTolerancePct / 100){
                g.max = v;
                g.sum += v;
                g.memberCount++;
                continue;
            }
        }
        groups.push_back({ v, v, v, 1 });
    }

    auto snap = [&groups](uint16_t v) -> uint16_t {
        if(v == 0) return 0;
        for(const Group& g : groups){
            if(v >= g.min && v <= g.max){
                return static_cast<uint16_t>(std::clamp<uint32_t>(g.sum / g.memberCount, MinDurationTicks, 32767));
            }
        }
        return v;
    };

    for(uint16_t i = 0; i < count; ++i){
        symbols[i].duration0 = snap(symbols[i].duration0);
        symbols[i].duration1 = snap(symbols[i].duration1);
    }
}

static uint32_t measurePeriodUs(const EnvCapture& cap){
    if(cap.count < 2) return 0;

    PSRAMVector<int64_t> deltas;
    deltas.reserve(cap.count - 1);
    for(uint8_t i = 1; i < cap.count; ++i){
        // a delta spanning a dropped frame would measure a multiple of the true period
        if(cap.rawIndex[i] != cap.rawIndex[i - 1] + 1){
            continue;
        }
        // Done events fire one idle-threshold after frame end; reconstruct starts since frame lengths differ.
        const int64_t start = cap.doneTimestampsUs[i] - FrameIdleThresholdUs - frameDurationUs(cap.frames[i]);
        const int64_t prevStart = cap.doneTimestampsUs[i - 1] - FrameIdleThresholdUs - frameDurationUs(cap.frames[i - 1]);
        deltas.push_back(start - prevStart);
    }
    if(deltas.empty()){
        return 0;
    }

    std::sort(deltas.begin(), deltas.end());
    const int64_t median = deltas[(deltas.size() - 1) / 2];

    if(median < MinPeriodUs || median > MaxPeriodUs){
        return 0;
    }
    return static_cast<uint32_t>(median);
}

// Bring-up diagnostic: dump the first symbols of a frame as mark/space durations in µs.
static void dumpFrame(const IRSignal& frame){
    char line[128];
    int pos = 0;
    const uint16_t n = std::min<uint16_t>(frame.count, 24);
    for(uint16_t i = 0; i < n; ++i){
        pos += snprintf(line + pos, sizeof(line) - pos, "%u/%u ", frame.symbols[i].duration0, frame.symbols[i].duration1);
        if(pos > (int) sizeof(line) - 16 || i + 1 == n){
            CMF_LOG(IRSignalProc, LogLevel::Info, "env[..%u]: %s", i, line);
            pos = 0;
        }
    }
}

bool processCapture(const IRCapture& cap, ProcessedIR& out){
    if(cap.count == 0){
        return false;
    }

    // Carrier stats only accumulate from surviving frames, so junk doesn't skew the measurement.
    auto env = makePSRAM<EnvCapture>();
    CarrierStats stats;
    for(uint8_t i = 0; i < cap.count; ++i){
        IRSignal& frame = env->frames[env->count];
        CarrierStats frameStats;
        if(!demodulate(cap.frames[i], frame, frameStats) || !frameLooksValid(frame)){
            continue;
        }
        stats.periodSumUs += frameStats.periodSumUs;
        stats.onSumUs += frameStats.onSumUs;
        stats.cycles += frameStats.cycles;
        env->doneTimestampsUs[env->count] = cap.doneTimestampsUs[i];
        env->rawIndex[env->count] = i;
        env->count++;
    }

    if(env->count != cap.count){
        CMF_LOG(IRSignalProc, LogLevel::Warning, "Dropped %d of %d frames as truncated or noise-corrupted",
                cap.count - env->count, cap.count);
    }
    if(env->count == 0){
        return false;
    }

    dumpFrame(env->frames[0]);

    uint8_t clusterOf[IRMaxCaptureFrames];
    uint8_t representatives[IRMaxCaptureFrames];
    uint8_t clusterCount = 0;

    for(uint8_t i = 0; i < env->count; ++i){
        clusterOf[i] = clusterCount;
        for(uint8_t c = 0; c < clusterCount; ++c){
            if(framesSimilar(env->frames[i], env->frames[representatives[c]])){
                clusterOf[i] = c;
                break;
            }
        }
        if(clusterOf[i] == clusterCount){
            representatives[clusterCount++] = i;
        }
    }

    uint8_t clusterSize[IRMaxCaptureFrames]{};
    for(uint8_t i = 0; i < env->count; ++i){
        clusterSize[clusterOf[i]]++;
    }

    // Main frame = earliest command-plausible cluster. Rejects stub-only captures (button held
    // too early) while keeping short repeated frames (RC5). Deliberately not majority-vote: for
    // header-only-first-frame protocols (JVC) the headerless repeats outnumber the true main frame.
    auto plausible = [&](uint8_t c){
        const uint16_t symbols = env->frames[representatives[c]].count;
        return symbols >= PlausibleMinSymbols || (clusterSize[c] >= 2 && symbols > StubMaxSymbols);
    };

    int mainCluster = -1;
    for(uint8_t c = 0; c < clusterCount; ++c){ // clusters are created in first-member order
        if(plausible(c)){
            mainCluster = c;
            break;
        }
    }
    if(mainCluster < 0){
        CMF_LOG(IRSignalProc, LogLevel::Warning, "Rejecting capture: no command-plausible frame cluster");
        return false;
    }

    CMF_LOG(IRSignalProc, LogLevel::Info, "%d frame(s) in %d cluster(s), main cluster size %d",
            env->count, clusterCount, clusterSize[mainCluster]);

    averageCluster(*env, clusterOf, static_cast<uint8_t>(mainCluster), out.frame, out.frameCount);
    snapDurations(out.frame, out.frameCount);

    // Distinct repeat = dominant differing cluster after the main frame. Only short stubs
    // qualify — a long differing "repeat" (Denon's inverted frame pair) is not single-press-safe.
    const uint8_t firstMainMember = representatives[mainCluster];
    uint8_t repeatCluster = static_cast<uint8_t>(mainCluster);
    uint8_t bestSize = 0;
    for(uint8_t c = 0; c < clusterCount; ++c){
        uint8_t size = 0;
        for(uint8_t i = firstMainMember + 1; i < env->count; ++i){
            if(clusterOf[i] == c) size++;
        }
        if(size > bestSize){
            bestSize = size;
            repeatCluster = c;
        }
    }
    const bool distinctRepeat = repeatCluster != mainCluster &&
                                env->frames[representatives[repeatCluster]].count <= MaxDistinctRepeatSymbols;

    // Distinct repeat or non-repeating remote -> one frame registers a press; identical-repeat
    // protocols need a burst.
    out.sendCount = (distinctRepeat || env->count == 1) ? 1 : IdenticalRepeatSends;

    out.periodUs = measurePeriodUs(*env);

    if(stats.periodSumUs > 0){
        const uint32_t hz = static_cast<uint32_t>(stats.cycles * 1000000ULL / stats.periodSumUs);
        if(hz >= 30000 && hz <= 60000){
            out.carrierHz = hz;
            // out-of-range duty = bad measurement; 0 lets TX use its default
            const uint64_t duty = stats.onSumUs * 100 / stats.periodSumUs;
            out.carrierDutyPct = (duty >= 20 && duty <= 50) ? static_cast<uint8_t>(duty) : 0;
        }
    }

    CMF_LOG(IRSignalProc, LogLevel::Info, "Result: frame %d symbols, send count %d, period %lu us, carrier %lu Hz duty %d%%",
            out.frameCount, out.sendCount, static_cast<unsigned long>(out.periodUs),
            static_cast<unsigned long>(out.carrierHz), out.carrierDutyPct);

    return out.frameCount > 0;
}
