#pragma once
#include <vector>
#include "TransferFunction.h"

/**
 * @brief Ergebniscontainer für Bode-Daten.
 */
struct BodeData {
    std::vector<double> omega;      // [rad/s]
    std::vector<double> mag_db;     // Betrag in dB
    std::vector<double> phase_deg;  // Phase in Grad
};

/**
 * @brief Berechnet numerisch den Frequenzgang G(jw) und daraus Betrag/Phase.
 */
class BodeComputation {
public:
    /**
     * @param unwrapPhase Wenn true, wird die Phase "ent-sprungen" (unwrap).
     */
    static BodeData compute(const TransferFunction& tf,
                            const std::vector<double>& omega,
                            bool unwrapPhase = true);
};
