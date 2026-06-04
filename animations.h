// bool_animation.h — simple time-based animations → scalar(float) for your own Mat4 composition.
//
// Flow: ignite(t_start) → each frame scalar(glfwGetTime()) → use with Mat4::translate/rotate/scale in main.
//
// AnimSpec:
//   durationSec — one cycle (bounce: up+down; linear non-bounce: 0→amplitude)
//   speed       — multiplier on elapsed time (>1 faster)
//   amplitude   — scales triangle/linear output
//   bounce      — true: triangle 0→1→0; false: saw ramp (infinite) or 0→1 once
//   infinite    — false: one shot then idle at 0; true: repeat forever while ignited
//
// AnimGroup:
//   Exclusive — ignite(i) stops all members, then starts i (one-at-a-time)
//   Together  — igniteAll(t) starts every member with the same start time
//
// Example (see Project_09/main.cpp): keys 1–4 exclusive, 5 all together, 6 stop all.

#ifndef ANIMATION_H_INCLUDED
#define ANIMATION_H_INCLUDED

#include <cmath>
#include <cstddef>
#include <vector>

// -----------------------------------------------------------------------------

struct AnimSpec {
    float durationSec = 1.0f;   // One full cycle length (bounce = up+down, linear = ramp)
    float speed = 1.0f;         // Higher = plays faster (time multiplier)
    float amplitude = 0.5f;     // Scales scalar output (pass into your transforms)
    bool bounce = true;        // Triangle 0→1→0 vs linear ramp 0→1 per cycle edge
    bool infinite = false;       // Repeat forever while ignited vs one shot then idle
};

// -----------------------------------------------------------------------------

inline float frac01(float x) {
    return x - std::floor(x);
}

// -----------------------------------------------------------------------------

class Anim {
public:
    explicit Anim(const AnimSpec& s = AnimSpec{}) : spec_(s) {}

    void setSpec(const AnimSpec& s) { spec_ = s; }
    AnimSpec spec() const { return spec_; }

    void ignite(double timeSec) {
        alive_ = true;
        const float period = std::max(spec_.durationSec, 1e-6f);
        const float speedAbs = std::max(std::fabs(spec_.speed), 1e-6f);
        if (holdValue_) {
            // Resume from the held phase/value instead of restarting from zero.
            t0_ = timeSec - static_cast<double>(heldPhase_ * period / speedAbs);
        } else {
            t0_ = timeSec;
        }
        holdValue_ = false;
    }

    void stop() {
        alive_ = false;
        holdValue_ = false;
    }

    // Stop but keep the current scalar value (useful for hold-to-move translations).
    void stopAndHold(double nowSec) {
        heldScalar_ = scalar(nowSec);
        heldPhase_ = phase01(nowSec);
        alive_ = false;
        holdValue_ = true;
    }

    bool isPlaying() const { return alive_; }

    // Normalized oscillation scalar in [0, amplitude]; exact 0 when stopped or idle.
    float scalar(double nowSec) const {
        if (!alive_)
            return holdValue_ ? heldScalar_ : 0.f;
        float u = phase01(nowSec); // normalized phase in [0,1] for finite, wrapped for infinite

        if (!spec_.infinite) {
            if (spec_.bounce) {
                if (u >= 1.0f) {
                    alive_ = false;
                    u = 1.0f;
                }
                const float bounce = 1.0f - std::fabs(2.0f * u - 1.0f);
                return spec_.amplitude * bounce;
            }
            // Linear once 0→1
            if (u >= 1.0f) {
                alive_ = false;
                return spec_.amplitude * 1.0f;
            }
            return spec_.amplitude * u;
        }

        // Infinite play
        u = frac01(u);
        if (spec_.bounce) {
            const float bounce = 1.0f - std::fabs(2.0f * u - 1.0f);
            return spec_.amplitude * bounce;
        }
        return spec_.amplitude * u;
    }

private:
    float phase01(double nowSec) const {
        const float period = std::max(spec_.durationSec, 1e-6f);
        const float speedAbs = std::max(std::fabs(spec_.speed), 1e-6f);
        float u = float((nowSec - t0_) * static_cast<double>(speedAbs) / static_cast<double>(period));
        if (spec_.infinite) return frac01(u);
        if (u < 0.0f) return 0.0f;
        if (u > 1.0f) return 1.0f;
        return u;
    }

    AnimSpec spec_;
    mutable bool alive_ = false;
    double t0_ = 0.0;
    mutable float heldScalar_ = 0.0f;
    float heldPhase_ = 0.0f;
    bool holdValue_ = false;
};

// -----------------------------------------------------------------------------

// Hold-to-slide on one axis: no AnimSpec. Each frame: step(glfwGetTime(), dir) with dir in {-1,0,+1}.
class LinearHoldAnim {
public:
    float speed = 1.0f;

    float offset() const { return offset_; }

    float step(double nowSec, int dir) {
        const int d = dir > 0 ? 1 : (dir < 0 ? -1 : 0);
        if (!inited_) {
            inited_ = true;
            tPrev_ = nowSec;
            return offset_;
        }
        if (d != 0)
            offset_ += static_cast<float>(d) * speed * static_cast<float>(nowSec - tPrev_);
        tPrev_ = nowSec;
        return offset_;
    }

    void reset(double nowSec = 0.0) {
        offset_ = 0.f;
        tPrev_ = nowSec;
        inited_ = false;
    }

private:
    float offset_ = 0.f;
    double tPrev_ = 0.0;
    bool inited_ = false;
};

// -----------------------------------------------------------------------------

// Group: Exclusive = ignite one stops others instantly; Together = ignite many with same timestamp.
enum class AnimGroupMode { Exclusive, Together };

class AnimGroup {
public:
    AnimGroup(AnimGroupMode mode, std::vector<Anim*> members)
        : mode_(mode), members_(std::move(members)) {}

    void ignite(std::size_t index, double nowSec) {
        if (members_.empty() || index >= members_.size())
            return;
        if (mode_ == AnimGroupMode::Exclusive)
            stopAll();
        members_[index]->ignite(nowSec);
    }

    void igniteAll(double nowSec) {
        for (Anim* a : members_)
            if (a) a->ignite(nowSec);
    }

    void stopAll() {
        for (Anim* a : members_)
            if (a) a->stop();
    }

    AnimGroupMode mode() const { return mode_; }

private:
    AnimGroupMode mode_;
    std::vector<Anim*> members_;
};

#endif
