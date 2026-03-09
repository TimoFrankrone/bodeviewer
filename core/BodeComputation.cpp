#include "BodeComputation.h"
#include <cmath>

static double rad2deg(double r) { return r * 180.0 / M_PI; }

BodeData BodeComputation::compute(const TransferFunction& tf,
                                  const std::vector<double>& omega,
                                  bool unwrapPhase) {
    BodeData out;
    out.omega = omega;
    out.mag_db.reserve(omega.size());
    out.phase_deg.reserve(omega.size());

    // Für Phase-Unwrapping:
    double prevPhase = 0.0;
    double phaseOffset = 0.0;

    for (size_t i = 0; i < omega.size(); ++i) {
        const auto G = tf.eval_jw(omega[i]);

        // Betrag und dB
        const double mag = std::abs(G);
        const double magDb = 20.0 * std::log10(mag > 0.0 ? mag : 1e-300);

        // Phase in Grad im Bereich [-180, 180]
        double phase = rad2deg(std::arg(G));

        // Optional: Phase "glätten", indem Sprünge >180° korrigiert werden
        if (unwrapPhase && i > 0) {
            const double delta = phase - prevPhase;
            if (delta > 180.0) phaseOffset -= 360.0;
            else if (delta < -180.0) phaseOffset += 360.0;
        }
        prevPhase = phase;
        phase += phaseOffset;

        out.mag_db.push_back(magDb);
        out.phase_deg.push_back(phase);
    }

    return out;
}
