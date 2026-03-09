#include "TransferFunction.h"
#include <cmath>

TransferFunction::TransferFunction(std::vector<double> num, std::vector<double> den)
    : m_num(std::move(num)), m_den(std::move(den)) {}

void TransferFunction::setNumerator(const std::vector<double>& num) { m_num = num; }
void TransferFunction::setDenominator(const std::vector<double>& den) { m_den = den; }

const std::vector<double>& TransferFunction::numerator() const { return m_num; }
const std::vector<double>& TransferFunction::denominator() const { return m_den; }

bool TransferFunction::isValid() const {
    if (m_den.empty()) return false;

    // Prüfe, ob alle Nennerkoeffizienten 0 wären (dann Division durch 0)
    double sumAbs = 0.0;
    for (double c : m_den) sumAbs += std::abs(c);
    return sumAbs > 0.0;
}

std::complex<double> TransferFunction::evalPoly(const std::vector<double>& coeffs,
                                                const std::complex<double>& s) {
    // Horner-Schema: effizient und numerisch stabil
    std::complex<double> y(0.0, 0.0);
    for (double c : coeffs) {
        y = y * s + c;
    }
    return y;
}

std::complex<double> TransferFunction::eval_jw(double omega) const {
    // s = j*omega
    const std::complex<double> s(0.0, omega);

    const auto num = evalPoly(m_num, s);
    const auto den = evalPoly(m_den, s);

    return num / den;
}
