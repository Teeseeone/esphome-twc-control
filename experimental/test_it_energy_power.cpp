#include "it_energy_power.h"
#include <cassert>
#include <iostream>

int main() {
  twc_it::EnergyPower p;
  assert(std::isnan(p.value(0)));
  // Verified 215 Wh / 120 s measurement, sampled at 10 s cadence.
  for (int i = 0; i <= 12; ++i)
    p.sample(1000.0 + 215.0 * i / 12, 1000 + 10*i, 2000 + 10*i, true, 10000*i);
  assert(std::fabs(p.value(120000) - 6450) < 0.001);
  assert(std::isnan(p.value(150001)));  // Lost HTTP data cannot remain current.
  p.sample(1233, 1130, 2130, true, 160000);
  assert(std::isnan(p.value(160000)));  // Gap requires a new baseline.
  p.sample(0, 0, 0, true, 170000);
  assert(std::isnan(p.value(170000)));  // Reboot/session reset.
  p.sample(0, 1, 1, false, 180000);
  assert(p.value(180000) == 0);
  p.sample(0, 2, 2, true, 190000);
  assert(std::isnan(p.value(190000)));  // New session has no average yet.
  p.invalidate();
  assert(!p.fresh(190000));
  p.sample(NAN, 3, 3, true, 200000);
  assert(std::isnan(p.value(200000)));
  // Frozen session counter despite successful HTTP responses.
  for (int i = 0; i <= 3; ++i) p.sample(1, 10, 20+i*10, true, 210000+i*10000);
  assert(std::isnan(p.value(240000)));
  // millis() wrap remains safe for intervals shorter than uint32_t range.
  twc_it::EnergyPower wrap;
  for (uint32_t i = 0; i <= 12; ++i)
    wrap.sample(215.0*i/12, 10*i, 10*i, true, UINT32_MAX-60000 + 10000*i);
  assert(std::fabs(wrap.value(UINT32_MAX-60000 + 120000) - 6450) < 0.001);
  std::cout << "Energy average, resets, stale data, frozen counter and timer wrap passed\n";
}
