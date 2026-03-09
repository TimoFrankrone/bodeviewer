#pragma once
#include <vector>
#include <complex>

/**
 * @brief Repräsentiert eine Übertragungsfunktion G(s) = N(s)/D(s)
 *        über Polynom-Koeffizienten (absteigend nach Potenzen).
 *
 * Beispiel:
 *  D(s) = s^2 + 3s + 2  => {1, 3, 2}
 */
class TransferFunction {
public:
    TransferFunction(std::vector<double> num = {}, std::vector<double> den = {});

    void setNumerator(const std::vector<double>& num);
    void setDenominator(const std::vector<double>& den);

    const std::vector<double>& numerator() const;
    const std::vector<double>& denominator() const;

    /**
     * @brief Auswertung der Übertragungsfunktion bei s = j*omega
     * @param omega Kreisfrequenz [rad/s]
     */
    std::complex<double> eval_jw(double omega) const;

    /**
     * @brief Einfache Validierung: Nenner vorhanden und nicht nur Nullen.
     */
    bool isValid() const;

private:
    std::vector<double> m_num;
    std::vector<double> m_den;

    /**
     * @brief Polynom-Auswertung via Horner-Schema.
     */
    static std::complex<double> evalPoly(const std::vector<double>& coeffs,
                                         const std::complex<double>& s);
};
