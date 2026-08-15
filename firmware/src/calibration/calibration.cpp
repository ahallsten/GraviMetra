#include "gravimetra/calibration/calibration.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace gravimetra::calibration {
namespace {

[[nodiscard]] bool point_valid(const CalibrationPoint& point) noexcept {
    return point.valid && std::isfinite(point.input_signal) &&
           std::isfinite(point.certified_mass_mg) &&
           std::isfinite(point.repeatability_stddev_mg) &&
           point.repeatability_stddev_mg >= 0.0;
}

[[nodiscard]] bool model_shape_valid(
    const PolynomialCalibration& model) noexcept {
    if (!model.configured || model.degree > 3U ||
        !std::isfinite(model.input_origin) ||
        !std::isfinite(model.input_scale) || model.input_scale <= 0.0 ||
        !std::isfinite(model.minimum_input_signal) ||
        !std::isfinite(model.maximum_input_signal) ||
        model.minimum_input_signal >= model.maximum_input_signal) {
        return false;
    }
    for (const double coefficient : model.coefficients) {
        if (!std::isfinite(coefficient)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool quality_valid(
    const PolynomialCalibration& model,
    const CalibrationQuality& quality) noexcept {
    const std::size_t required_points =
        static_cast<std::size_t>(model.degree) + 1U;
    return quality.point_count >= required_points &&
           quality.point_count <= kMaximumCalibrationPoints &&
           std::isfinite(quality.input_span) && quality.input_span > 0.0 &&
           std::isfinite(quality.rms_residual_mg) &&
           quality.rms_residual_mg >= 0.0 &&
           std::isfinite(quality.maximum_absolute_residual_mg) &&
           quality.maximum_absolute_residual_mg >= 0.0 &&
           std::isfinite(quality.maximum_repeatability_stddev_mg) &&
           quality.maximum_repeatability_stddev_mg >= 0.0;
}

[[nodiscard]] bool spans_coherent(
    const double model_span,
    const double quality_span) noexcept {
    const double magnitude =
        std::max({1.0, std::fabs(model_span), std::fabs(quality_span)});
    const double tolerance =
        128.0 * std::numeric_limits<double>::epsilon() * magnitude;
    return std::fabs(model_span - quality_span) <= tolerance;
}

[[nodiscard]] Status solve_system(
    std::array<std::array<double, kMaximumPolynomialTerms + 1U>,
               kMaximumPolynomialTerms>& augmented,
    const std::size_t term_count,
    std::array<double, kMaximumPolynomialTerms>& solution) noexcept {
    constexpr double kPivotTolerance =
        128.0 * std::numeric_limits<double>::epsilon();
    for (std::size_t column = 0U; column < term_count; ++column) {
        std::size_t pivot = column;
        double largest = std::fabs(augmented[column][column]);
        for (std::size_t row = column + 1U; row < term_count; ++row) {
            const double candidate = std::fabs(augmented[row][column]);
            if (candidate > largest) {
                largest = candidate;
                pivot = row;
            }
        }
        if (!std::isfinite(largest) || largest <= kPivotTolerance) {
            return Status::verification_failed;
        }
        if (pivot != column) {
            std::swap(augmented[pivot], augmented[column]);
        }

        const double divisor = augmented[column][column];
        for (std::size_t entry = column; entry <= term_count; ++entry) {
            augmented[column][entry] /= divisor;
        }
        for (std::size_t row = 0U; row < term_count; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = augmented[row][column];
            for (std::size_t entry = column; entry <= term_count; ++entry) {
                augmented[row][entry] -= factor * augmented[column][entry];
            }
        }
    }
    for (std::size_t index = 0U; index < term_count; ++index) {
        solution[index] = augmented[index][term_count];
        if (!std::isfinite(solution[index])) {
            return Status::verification_failed;
        }
    }
    return Status::ok;
}

}  // namespace

bool PolynomialCalibration::contains(const double input_signal) const noexcept {
    return model_shape_valid(*this) && std::isfinite(input_signal) &&
           input_signal >= minimum_input_signal &&
           input_signal <= maximum_input_signal;
}

double PolynomialCalibration::evaluate(const double input_signal) const noexcept {
    if (!contains(input_signal)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double normalized = (input_signal - input_origin) / input_scale;
    double result = coefficients[degree];
    for (std::size_t index = degree; index > 0U; --index) {
        result = (result * normalized) + coefficients[index - 1U];
    }
    return result;
}

Status evaluate_quality(
    const PolynomialCalibration& model,
    const CalibrationPoint* const points,
    const std::size_t point_count,
    CalibrationQuality& quality) noexcept {
    quality = CalibrationQuality{};
    if (!model.configured) {
        return Status::not_configured;
    }
    if (!model_shape_valid(model)) {
        return Status::invalid_argument;
    }
    if (points == nullptr || point_count == 0U ||
        point_count > kMaximumCalibrationPoints) {
        return Status::invalid_argument;
    }

    double minimum_input = std::numeric_limits<double>::infinity();
    double maximum_input = -std::numeric_limits<double>::infinity();
    double squared_residual_sum = 0.0;
    for (std::size_t index = 0U; index < point_count; ++index) {
        if (!point_valid(points[index])) {
            return Status::invalid_argument;
        }
        const double prediction = model.evaluate(points[index].input_signal);
        const double residual = prediction - points[index].certified_mass_mg;
        if (!std::isfinite(residual)) {
            return Status::verification_failed;
        }
        squared_residual_sum += residual * residual;
        quality.maximum_absolute_residual_mg = std::max(
            quality.maximum_absolute_residual_mg, std::fabs(residual));
        quality.maximum_repeatability_stddev_mg = std::max(
            quality.maximum_repeatability_stddev_mg,
            points[index].repeatability_stddev_mg);
        minimum_input = std::min(minimum_input, points[index].input_signal);
        maximum_input = std::max(maximum_input, points[index].input_signal);
    }
    quality.point_count = point_count;
    quality.input_span = maximum_input - minimum_input;
    quality.rms_residual_mg = std::sqrt(
        squared_residual_sum / static_cast<double>(point_count));
    return std::isfinite(quality.rms_residual_mg) ? Status::ok
                                                  : Status::verification_failed;
}

Status fit_polynomial(
    const CalibrationPoint* const points,
    const std::size_t point_count,
    const std::uint8_t degree,
    PolynomialCalibration& model,
    CalibrationQuality& quality) noexcept {
    model = PolynomialCalibration{};
    quality = CalibrationQuality{};
    const std::size_t term_count = static_cast<std::size_t>(degree) + 1U;
    if (points == nullptr || degree > 3U || point_count < term_count ||
        point_count > kMaximumCalibrationPoints) {
        return Status::invalid_argument;
    }

    double minimum_input = std::numeric_limits<double>::infinity();
    double maximum_input = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0U; index < point_count; ++index) {
        if (!point_valid(points[index])) {
            return Status::invalid_argument;
        }
        minimum_input = std::min(minimum_input, points[index].input_signal);
        maximum_input = std::max(maximum_input, points[index].input_signal);
    }
    const double span = maximum_input - minimum_input;
    if (!std::isfinite(span) || span <= 0.0) {
        return Status::verification_failed;
    }
    const double origin = minimum_input + (span * 0.5);
    const double scale = span * 0.5;

    std::array<std::array<double, kMaximumPolynomialTerms + 1U>,
               kMaximumPolynomialTerms>
        augmented{};
    for (std::size_t point_index = 0U; point_index < point_count;
         ++point_index) {
        const double normalized =
            (points[point_index].input_signal - origin) / scale;
        std::array<double, (2U * kMaximumPolynomialTerms) - 1U> powers{};
        powers[0] = 1.0;
        for (std::size_t power = 1U; power < powers.size(); ++power) {
            powers[power] = powers[power - 1U] * normalized;
        }
        for (std::size_t row = 0U; row < term_count; ++row) {
            for (std::size_t column = 0U; column < term_count; ++column) {
                augmented[row][column] += powers[row + column];
            }
            augmented[row][term_count] +=
                points[point_index].certified_mass_mg * powers[row];
        }
    }

    std::array<double, kMaximumPolynomialTerms> coefficients{};
    Status result = solve_system(augmented, term_count, coefficients);
    if (!is_ok(result)) {
        return result;
    }
    model.configured = true;
    model.degree = degree;
    model.input_origin = origin;
    model.input_scale = scale;
    model.minimum_input_signal = minimum_input;
    model.maximum_input_signal = maximum_input;
    model.coefficients = coefficients;
    result = evaluate_quality(model, points, point_count, quality);
    if (!is_ok(result)) {
        model = PolynomialCalibration{};
    }
    return result;
}

Status validate_calibration(
    PolynomialCalibration& model,
    const CalibrationPoint* const points,
    const std::size_t point_count,
    const CalibrationAcceptance& acceptance,
    CalibrationQuality& verified_quality) noexcept {
    // Acceptance is a publication state, not a sticky property. Any failed
    // revalidation must make the model ineligible for live measurement.
    model.accepted = false;
    verified_quality = CalibrationQuality{};
    if (!model.configured || !acceptance.configured) {
        return Status::not_configured;
    }
    if (!model_shape_valid(model) || acceptance.minimum_point_count == 0U ||
        acceptance.minimum_point_count > kMaximumCalibrationPoints ||
        !std::isfinite(acceptance.minimum_input_span) ||
        acceptance.minimum_input_span <= 0.0 ||
        !std::isfinite(acceptance.maximum_rms_residual_mg) ||
        acceptance.maximum_rms_residual_mg < 0.0 ||
        !std::isfinite(acceptance.maximum_absolute_residual_mg) ||
        acceptance.maximum_absolute_residual_mg < 0.0 ||
        !std::isfinite(acceptance.maximum_repeatability_stddev_mg) ||
        acceptance.maximum_repeatability_stddev_mg < 0.0) {
        return Status::invalid_argument;
    }
    // Quality is deliberately recomputed here. Accepting an independently
    // supplied summary would let stale or fabricated favorable metrics publish
    // a model that the source calibration observations do not support.
    const Status quality_result =
        evaluate_quality(model, points, point_count, verified_quality);
    if (!is_ok(quality_result)) {
        return quality_result;
    }
    if (!quality_valid(model, verified_quality)) {
        verified_quality = CalibrationQuality{};
        return Status::verification_failed;
    }
    const double model_span =
        model.maximum_input_signal - model.minimum_input_signal;
    if (!std::isfinite(model_span) ||
        !spans_coherent(model_span, verified_quality.input_span)) {
        return Status::verification_failed;
    }
    if (verified_quality.point_count < acceptance.minimum_point_count ||
        verified_quality.input_span < acceptance.minimum_input_span ||
        verified_quality.rms_residual_mg > acceptance.maximum_rms_residual_mg ||
        verified_quality.maximum_absolute_residual_mg >
            acceptance.maximum_absolute_residual_mg ||
        verified_quality.maximum_repeatability_stddev_mg >
            acceptance.maximum_repeatability_stddev_mg) {
        return Status::verification_failed;
    }
    model.accepted = true;
    return Status::ok;
}

}  // namespace gravimetra::calibration
