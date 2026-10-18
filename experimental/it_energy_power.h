#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

namespace twc_it {
// Windowed measured average, independent of phase/neutral interpretation.
// Caller must invalidate on a failed HTTP request or incomplete JSON payload.
class EnergyPower {
 public:
  void invalidate() { anchored_ = false; power_ = nan(); seen_ = false; }

  void sample(double energy_wh, double session_s, double uptime_s,
              bool charging, uint32_t now_ms) {
    if (!std::isfinite(energy_wh) || !std::isfinite(session_s) ||
        !std::isfinite(uptime_s) || energy_wh < 0 || session_s < 0 || uptime_s < 0) {
      invalidate();
      return;
    }
    const bool discontinuity = !seen_ || now_ms - last_ms_ > 30000 ||
        energy_wh < last_energy_ || session_s < last_session_ || uptime_s < last_uptime_ ||
        charging != charging_;
    last_ms_ = now_ms;
    seen_ = true;
    charging_ = charging;
    last_energy_ = energy_wh;
    last_session_ = session_s;
    last_uptime_ = uptime_s;
    if (!charging) {
      anchored_ = false;
      power_ = 0;
      return;
    }
    if (discontinuity || !anchored_) {
      anchor(energy_wh, session_s, now_ms);
      power_ = nan();
      return;
    }
    const double seconds = session_s - anchor_session_;
    const double wall_seconds = (now_ms - anchor_ms_) / 1000.0;
    // Reject frozen or implausible counters; don't label old data as current.
    if (wall_seconds > 180 || seconds > 180 || std::fabs(seconds - wall_seconds) > 15) {
      anchor(energy_wh, session_s, now_ms);
      power_ = nan();
      return;
    }
    if (seconds >= 120) {
      power_ = (energy_wh - anchor_energy_) * 3600.0 / seconds;
      anchor(energy_wh, session_s, now_ms);
    }
  }

  double value(uint32_t now_ms) const {
    return seen_ && now_ms - last_ms_ <= 30000 ? power_ : nan();
  }
  bool fresh(uint32_t now_ms) const { return seen_ && now_ms - last_ms_ <= 30000; }

 private:
  static double nan() { return std::numeric_limits<double>::quiet_NaN(); }
  void anchor(double energy, double seconds, uint32_t now) {
    anchor_energy_ = energy; anchor_session_ = seconds; anchor_ms_ = now; anchored_ = true;
  }
  bool anchored_{false}, seen_{false}, charging_{false};
  double anchor_energy_{0}, anchor_session_{0}, power_{nan()};
  double last_energy_{0}, last_session_{0}, last_uptime_{0};
  uint32_t anchor_ms_{0}, last_ms_{0};
};
}  // namespace twc_it
