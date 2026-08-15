#pragma once

#include "gravimetra/common/status.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::calibration {

constexpr std::size_t kMaximumCalibrationPoints = 7U;
constexpr std::size_t kMaximumPolynomialTerms = 4U;

// Nominal workflow points only. CalibrationPoint::certified_mass_mg must hold
// the actual value from the mass certificate when a point is collected.
inline constexpr std::array<double, kMaximumCalibrationPoints>
    kNominalCalibrationMassesMg{
        0.0, 25'000.0, 50'000.0, 100'000.0,
        150'000.0, 200'000.0, 250'000.0};

struct CalibrationPoint {
    bool valid{false};
    double input_signal{0.0};
    double certified_mass_mg{0.0};
    double repeatability_stddev_mg{0.0};
};

struct PolynomialCalibration {
    bool configured{false};
    bool accepted{false};
    std::uint8_t degree{0U};
    double input_origin{0.0};
    double input_scale{0.0};
    double minimum_input_signal{0.0};
    double maximum_input_signal{0.0};
    // Coefficients are ascending powers of normalized input:
    // z = (input - input_origin) / input_scale.
    std::array<double, kMaximumPolynomialTerms> coefficients{};

    [[nodiscard]] bool contains(double input_signal) const noexcept;
    [[nodiscard]] double evaluate(double input_signal) const noexcept;
};

struct CalibrationQuality {
    std::size_t point_count{0U};
    double input_span{0.0};
    double rms_residual_mg{0.0};
    double maximum_absolute_residual_mg{0.0};
    double maximum_repeatability_stddev_mg{0.0};
};

struct CalibrationAcceptance {
    bool configured{false};
    std::size_t minimum_point_count{0U};
    double minimum_input_span{0.0};
    double maximum_rms_residual_mg{0.0};
    double maximum_absolute_residual_mg{0.0};
    double maximum_repeatability_stddev_mg{0.0};
};

[[nodiscard]] Status fit_polynomial(
    const CalibrationPoint* points,
    std::size_t point_count,
    std::uint8_t degree,
    PolynomialCalibration& model,
    CalibrationQuality& quality) noexcept;

[[nodiscard]] Status evaluate_quality(
    const PolynomialCalibration& model,
    const CalibrationPoint* points,
    std::size_t point_count,
    CalibrationQuality& quality) noexcept;

[[nodiscard]] Status validate_calibration(
    PolynomialCalibration& model,
    const CalibrationPoint* points,
    std::size_t point_count,
    const CalibrationAcceptance& acceptance,
    CalibrationQuality& verified_quality) noexcept;

}  // namespace gravimetra::calibration
