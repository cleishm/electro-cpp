#pragma once

/**
 * @file decibel.hpp
 * @brief Logarithmic ratios (dB) and absolute levels (dBm, dBW, dBµV, ...).
 *
 * Decibels come in two flavours, and conflating them is the classic source of
 * bugs. This header keeps them apart with the same distinction std::chrono
 * draws between a duration and a time_point:
 *
 * - A **gain** (dB) is a dimensionless *ratio* - the duration analog. Gains
 *   add, subtract, and scale by a scalar.
 * - A **level** (dBm, dBW, dBµV, ...) is an *absolute* point on a logarithmic
 *   scale, measured against a reference - the time_point analog.
 *
 * The resulting algebra is enforced at compile time:
 *
 * @code
 * using namespace electro;
 * using namespace electro_literals;
 *
 * auto tx   = 20_dBm;
 * auto eirp = tx + 6_dB - 2_dB;   // level +/- gain  -> level
 * auto rx   = eirp - 90_dB;       // path loss       -> -66 dBm
 * auto margin = rx - (-100_dBm);  // level - level   -> 34 dB
 *
 * // tx + tx;   // ill-formed: adding two levels is meaningless
 * // tx * 2;    // ill-formed: scaling an absolute level is meaningless
 * @endcode
 *
 * ## Crossing into the linear domain
 *
 * Conversions to and from linear quantities are explicit, named functions
 * rather than constructors, because they are non-constexpr (they need
 * std::log10 and std::pow), they are nonlinear (so the implicit/explicit
 * constructor convention used elsewhere in electro - which signals *precision*
 * loss - would be misleading), and they have domain errors that a constructor
 * cannot report.
 *
 * @code
 * to_linear(30_dBm);            // 1000.0 mW, as power<double, std::milli>
 * to_level<dbm>(milliwatts(1)); // 0 dBm
 * @endcode
 *
 * ## Power references and field references
 *
 * A reference tag records both the reference quantity and whether the
 * underlying linear quantity is a *power* (10*log10, e.g. dBm) or a *field*
 * such as voltage (20*log10, e.g. dBµV). Callers never have to remember which
 * factor applies; it travels with the type.
 *
 * For the same reason a bare dB value cannot be turned into a linear ratio
 * unambiguously - 3 dB is a factor of 2 in power but ~1.41 in amplitude - so
 * power_ratio() and amplitude_ratio() are separate, explicitly named functions.
 */

#include <cassert>
#include <cmath>
#include <electro/electro.hpp>

namespace electro {

// ============================================================================
// Gain: a dimensionless logarithmic ratio
// ============================================================================

/**
 * @brief A dimensionless logarithmic ratio, measured in decibels.
 *
 * This is an ordinary electro::quantity unit: gains add, subtract, negate and
 * scale by a scalar, exactly as a std::chrono::duration does. Cascading two
 * amplifiers adds their gains; doubling a gain squares the underlying ratio.
 */
struct decibel_unit {
    static constexpr const char* symbol = "dB";
};

/** @brief A logarithmic ratio with configurable representation and precision. */
template<typename Rep, typename Precision = std::ratio<1>>
using gain = quantity<decibel_unit, Rep, Precision>;

/** @brief Gain with 1 dB precision. */
using decibels = gain<int64_t>;
/** @brief Gain with 0.01 dB precision. */
using centidecibels = gain<int64_t, std::centi>;
/** @brief Gain with 0.001 dB precision. */
using millidecibels = gain<int64_t, std::milli>;

// ============================================================================
// References
// ============================================================================

/**
 * @defgroup References Decibel Reference Points
 * @{
 *
 * Tag types identifying what a level is measured against. Each names the
 * linear unit, the precision of the reference quantity (so that 1 count of
 * that precision is the 0 dB point), the logarithmic factor (10 for power
 * quantities, 20 for field quantities), and the display symbol.
 */

/** @brief Reference of 1 mW, for dBm. */
struct milliwatt_reference {
    using unit = watt_unit;
    using precision = std::milli;
    static constexpr int factor = 10;
    static constexpr const char* symbol = "dBm";
};

/** @brief Reference of 1 W, for dBW. */
struct watt_reference {
    using unit = watt_unit;
    using precision = std::ratio<1>;
    static constexpr int factor = 10;
    static constexpr const char* symbol = "dBW";
};

/** @brief Reference of 1 V, for dBV. */
struct volt_reference {
    using unit = volt_unit;
    using precision = std::ratio<1>;
    static constexpr int factor = 20;
    static constexpr const char* symbol = "dBV";
};

/** @brief Reference of 1 mV, for dBmV. */
struct millivolt_reference {
    using unit = volt_unit;
    using precision = std::milli;
    static constexpr int factor = 20;
    static constexpr const char* symbol = "dBmV";
};

/** @brief Reference of 1 µV, for dBµV. */
struct microvolt_reference {
    using unit = volt_unit;
    using precision = std::micro;
    static constexpr int factor = 20;
    static constexpr const char* symbol = "dBµV";
};

/** @} */ // end of References group

/**
 * @brief Concept for decibel reference tags.
 *
 * A reference names a linear unit, the precision at which one count is the
 * 0 dB point, a factor of 10 (power quantity) or 20 (field quantity), and a
 * display symbol.
 */
template<typename Reference>
concept decibel_reference = requires {
    typename Reference::unit;
    typename Reference::precision;
    { Reference::factor } -> std::convertible_to<int>;
    { Reference::symbol } -> std::convertible_to<const char*>;
} && (Reference::factor == 10 || Reference::factor == 20);

/** @brief True if the reference measures a power quantity (10*log10). */
template<typename Reference>
inline constexpr bool is_power_reference_v = false;

template<decibel_reference Reference>
inline constexpr bool is_power_reference_v<Reference> = Reference::factor == 10;

/** @brief True if the reference measures a field quantity (20*log10). */
template<typename Reference>
inline constexpr bool is_field_reference_v = false;

template<decibel_reference Reference>
inline constexpr bool is_field_reference_v<Reference> = Reference::factor == 20;

// ============================================================================
// level
// ============================================================================

template<typename Reference, typename Rep, typename Precision = std::ratio<1>>
class level;

/**
 * @brief Trait to detect level specializations.
 * @tparam T Type to check.
 */
template<typename T>
struct is_level : std::false_type {};

template<typename Reference, typename Rep, typename Precision>
struct is_level<level<Reference, Rep, Precision>> : std::true_type {};

template<typename T>
inline constexpr bool is_level_v = is_level<T>::value;

/**
 * @brief An absolute point on a logarithmic scale, relative to a reference.
 *
 * The time_point analog: a level plus or minus a gain is a level, and the
 * difference of two levels is a gain. Adding two levels, or scaling a level by
 * a scalar, is deliberately ill-formed.
 *
 * @code
 * dbm signal{-73};                  // -73 dBm
 * auto amplified = signal + 20_dB;  // -53 dBm
 * auto delta = amplified - signal;  // 20 dB
 * @endcode
 *
 * @tparam Reference The reference tag (milliwatt_reference, ...).
 * @tparam Rep       Arithmetic type for the offset count.
 * @tparam Precision A std::ratio giving the offset precision, in dB.
 */
template<typename Reference, typename Rep, typename Precision>
class level {
    static_assert(decibel_reference<Reference>, "Reference must satisfy the decibel_reference concept");

public:
    /** @brief The reference tag type. */
    using reference = Reference;
    /** @brief The representation type. */
    using rep = Rep;
    /** @brief The offset precision, in dB, as a std::ratio. */
    using precision = typename Precision::type;
    /** @brief The gain type measuring the offset from the reference. */
    using offset = gain<Rep, Precision>;

    /** @brief Constructs a level at the reference point (0 dB offset). */
    constexpr level() = default;
    level(const level&) = default;

    /**
     * @brief Constructs from an offset count in dB.
     *
     * Always explicit: a bare number is not a level.
     *
     * @tparam Rep2 The source representation type.
     * @param r     The offset from the reference, in units of Precision.
     */
    template<typename Rep2>
        requires std::convertible_to<const Rep2&, rep> && (treat_as_inexact_v<rep> || !treat_as_inexact_v<Rep2>)
    constexpr explicit level(const Rep2& r)
        : _o(static_cast<rep>(r)) {}

    /** @brief Constructs from an offset relative to the reference. */
    constexpr explicit level(const offset& o)
        : _o(o) {}

    /**
     * @brief Converts from a level with the same reference.
     *
     * Implicit when the conversion is lossless, mirroring quantity.
     */
    template<typename Rep2, typename Precision2>
        requires std::convertible_to<gain<Rep2, Precision2>, offset>
    constexpr level(const level<Reference, Rep2, Precision2>& l)
        : _o(l.offset_from_reference()) {}

    /** @brief Explicit constructor for lossy precision conversions. */
    template<typename Rep2, typename Precision2>
        requires(!std::is_same_v<level, level<Reference, Rep2, Precision2>>) &&
        (!std::convertible_to<gain<Rep2, Precision2>, offset>) &&
        std::constructible_from<offset, gain<Rep2, Precision2>>
    constexpr explicit level(const level<Reference, Rep2, Precision2>& l)
        : _o(offset(l.offset_from_reference())) {}

    ~level() = default;
    level& operator=(const level&) = default;

    /** @brief Returns the offset from the reference point. */
    constexpr offset offset_from_reference() const { return _o; }

    /** @brief Returns the raw offset count, in units of Precision. */
    constexpr rep count() const { return _o.count(); }

    /**
     * @brief Reflects the level about its reference point.
     *
     * Provided so that negative literals such as `-73_dBm` parse. Note that
     * std::chrono::time_point offers no such operator; a level is an absolute
     * point, and negating one is only meaningful as "the same magnitude of
     * offset, on the other side of the reference".
     */
    constexpr level operator-() const { return level(-_o); }
    constexpr level operator+() const { return *this; }

    constexpr level& operator+=(const offset& o) {
        _o += o;
        return *this;
    }

    constexpr level& operator-=(const offset& o) {
        _o -= o;
        return *this;
    }

    /** @brief Returns the reference point itself (a 0 dB offset). */
    static constexpr level reference_level() noexcept { return level(offset::zero()); }

    /** @brief Returns the minimum representable level. */
    static constexpr level min() noexcept { return level(offset::min()); }

    /** @brief Returns the maximum representable level. */
    static constexpr level max() noexcept { return level(offset::max()); }

private:
    offset _o{};
};

/**
 * @brief Converts a level to a different precision or representation.
 *
 * @tparam ToLevel The target level type (must have the same reference).
 */
template<typename ToLevel, typename Reference, typename Rep, typename Precision>
constexpr ToLevel level_cast(const level<Reference, Rep, Precision>& l) {
    static_assert(
        std::is_same_v<typename ToLevel::reference, Reference>, "level_cast cannot convert between different references"
    );
    return ToLevel(quantity_cast<typename ToLevel::offset>(l.offset_from_reference()));
}

/** @brief Converts a level to the target type, rounding toward negative infinity. */
template<typename ToLevel, typename Reference, typename Rep, typename Precision>
constexpr ToLevel floor(const level<Reference, Rep, Precision>& l) {
    static_assert(std::is_same_v<typename ToLevel::reference, Reference>, "floor cannot convert between references");
    return ToLevel(floor<typename ToLevel::offset>(l.offset_from_reference()));
}

/** @brief Converts a level to the target type, rounding toward positive infinity. */
template<typename ToLevel, typename Reference, typename Rep, typename Precision>
constexpr ToLevel ceil(const level<Reference, Rep, Precision>& l) {
    static_assert(std::is_same_v<typename ToLevel::reference, Reference>, "ceil cannot convert between references");
    return ToLevel(ceil<typename ToLevel::offset>(l.offset_from_reference()));
}

/** @brief Converts a level to the target type, rounding to nearest (ties to even). */
template<typename ToLevel, typename Reference, typename Rep, typename Precision>
constexpr ToLevel round(const level<Reference, Rep, Precision>& l) {
    static_assert(std::is_same_v<typename ToLevel::reference, Reference>, "round cannot convert between references");
    return ToLevel(round<typename ToLevel::offset>(l.offset_from_reference()));
}

// ============================================================================
// Affine arithmetic: level +/- gain -> level, level - level -> gain
// ============================================================================

/** @cond INTERNAL */
template<typename R1, typename P1, typename R2, typename P2>
using _common_gain = std::common_type_t<gain<R1, P1>, gain<R2, P2>>;
/** @endcond */

/** @brief Applies a gain to a level. */
template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
constexpr auto
operator+(const level<Reference, Rep1, Precision1>& l, const quantity<decibel_unit, Rep2, Precision2>& g) {
    using cg = _common_gain<Rep1, Precision1, Rep2, Precision2>;
    using cl = level<Reference, typename cg::rep, typename cg::precision>;
    return cl(cg(l.offset_from_reference()) + cg(g));
}

/** @brief Applies a gain to a level. */
template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
constexpr auto
operator+(const quantity<decibel_unit, Rep2, Precision2>& g, const level<Reference, Rep1, Precision1>& l) {
    return l + g;
}

/** @brief Applies a loss to a level. */
template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
constexpr auto
operator-(const level<Reference, Rep1, Precision1>& l, const quantity<decibel_unit, Rep2, Precision2>& g) {
    using cg = _common_gain<Rep1, Precision1, Rep2, Precision2>;
    using cl = level<Reference, typename cg::rep, typename cg::precision>;
    return cl(cg(l.offset_from_reference()) - cg(g));
}

/**
 * @brief The difference between two levels is a gain.
 *
 * @code
 * auto margin = -66_dBm - -100_dBm;  // 34 dB
 * @endcode
 */
template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
constexpr auto operator-(const level<Reference, Rep1, Precision1>& a, const level<Reference, Rep2, Precision2>& b) {
    return a.offset_from_reference() - b.offset_from_reference();
}

template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
constexpr bool operator==(const level<Reference, Rep1, Precision1>& a, const level<Reference, Rep2, Precision2>& b) {
    return a.offset_from_reference() == b.offset_from_reference();
}

template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
    requires std::three_way_comparable<std::common_type_t<Rep1, Rep2>>
constexpr auto operator<=>(const level<Reference, Rep1, Precision1>& a, const level<Reference, Rep2, Precision2>& b) {
    return a.offset_from_reference() <=> b.offset_from_reference();
}

// ============================================================================
// Linear domain conversions (not constexpr before C++26)
// ============================================================================

/** @cond INTERNAL */
template<typename Offset>
constexpr double _db_value(const Offset& o) {
    return quantity_cast<gain<double>>(o).count();
}

// The inverse of _db_value, rounding to nearest for integral reps. This exists
// because quantity_cast (and floor/ceil/round) truncate when converting from
// an inexact rep, and truncation would round e.g. -73.5 dB the wrong way.
template<typename Offset>
inline Offset _db_offset(double db) {
    if constexpr (treat_as_inexact_v<typename Offset::rep>) {
        return quantity_cast<Offset>(gain<double>(db));
    } else {
        using p = typename Offset::precision;
        const double ticks = db * static_cast<double>(p::den) / static_cast<double>(p::num);
        return Offset(static_cast<typename Offset::rep>(std::llround(ticks)));
    }
}
/** @endcond */

/**
 * @brief Converts a level to the equivalent linear quantity.
 *
 * The result is expressed at the reference's own precision, so to_linear() of
 * a dBm level yields milliwatts and to_linear() of a dBµV level yields
 * microvolts. Use quantity_cast to retarget:
 *
 * @code
 * to_linear(30_dBm);                          // 1000.0 mW
 * quantity_cast<watts>(to_linear(30_dBm));    // 1 W
 * @endcode
 *
 * @note Not constexpr: requires std::pow.
 */
template<typename Reference, typename Rep, typename Precision>
inline auto to_linear(const level<Reference, Rep, Precision>& l) {
    using linear = quantity<typename Reference::unit, double, typename Reference::precision>;
    return linear(std::pow(10.0, _db_value(l.offset_from_reference()) / Reference::factor));
}

/**
 * @brief Converts a linear quantity to a level.
 *
 * The quantity's unit must match the target level's reference unit. Values are
 * rounded to nearest at the target precision, rather than truncated.
 *
 * A zero quantity has no finite logarithm and maps to ToLevel::min(). A
 * negative quantity has no logarithm at all; in a debug build this asserts,
 * and otherwise returns ToLevel::min().
 *
 * @code
 * to_level<dbm>(milliwatts(1));       // 0 dBm
 * to_level<dbm>(watts(1));            // 30 dBm
 * to_level<dbuv>(volts(1));           // 120 dBµV (field reference: 20*log10)
 * @endcode
 *
 * @note Not constexpr: requires std::log10.
 */
template<typename ToLevel, typename Unit, typename Rep, typename Precision>
inline ToLevel to_level(const quantity<Unit, Rep, Precision>& q) {
    using ref = typename ToLevel::reference;
    static_assert(
        std::is_same_v<typename ref::unit, Unit>, "to_level: quantity unit does not match the level reference"
    );

    using linear = quantity<Unit, double, typename ref::precision>;
    const double ratio = quantity_cast<linear>(q).count();
    if (!(ratio > 0.0)) {
        assert(ratio >= 0.0 && "to_level: a negative quantity has no logarithmic level");
        return ToLevel::min();
    }
    return ToLevel(_db_offset<typename ToLevel::offset>(ref::factor * std::log10(ratio)));
}

/**
 * @brief The linear power ratio a gain represents (10^(dB/10)).
 *
 * @code
 * power_ratio(3_dB);   // ~1.995 (a doubling of power)
 * @endcode
 *
 * @note Not constexpr: requires std::pow.
 */
template<typename Rep, typename Precision>
inline double power_ratio(const quantity<decibel_unit, Rep, Precision>& g) {
    return std::pow(10.0, _db_value(g) / 10.0);
}

/**
 * @brief The linear amplitude ratio a gain represents (10^(dB/20)).
 *
 * @code
 * amplitude_ratio(6_dB);  // ~1.995 (a doubling of voltage)
 * @endcode
 *
 * @note Not constexpr: requires std::pow.
 */
template<typename Rep, typename Precision>
inline double amplitude_ratio(const quantity<decibel_unit, Rep, Precision>& g) {
    return std::pow(10.0, _db_value(g) / 20.0);
}

/** @brief The gain corresponding to a linear power ratio (10*log10(ratio)). */
inline gain<double> power_gain(double ratio) {
    assert(ratio > 0.0 && "power_gain: ratio must be positive");
    return gain<double>(10.0 * std::log10(ratio));
}

/** @brief The gain corresponding to a linear amplitude ratio (20*log10(ratio)). */
inline gain<double> amplitude_gain(double ratio) {
    assert(ratio > 0.0 && "amplitude_gain: ratio must be positive");
    return gain<double>(20.0 * std::log10(ratio));
}

/**
 * @brief Combines the powers of two uncorrelated signals.
 *
 * This is what "adding" two levels physically means, and it is deliberately
 * not operator+: 10 dBm combined with 10 dBm is 13.01 dBm, not 20 dBm.
 *
 * Only available for power references. For a field reference such as dBµV,
 * summing the linear amplitudes would model coherent, in-phase addition
 * instead, which is a different operation with a different answer.
 *
 * @code
 * add_powers(centi_dbm(1000), centi_dbm(1000));  // 13.01 dBm
 * @endcode
 *
 * @note Not constexpr: requires std::pow and std::log10.
 */
template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
    requires is_power_reference_v<Reference>
inline auto add_powers(const level<Reference, Rep1, Precision1>& a, const level<Reference, Rep2, Precision2>& b) {
    using cg = _common_gain<Rep1, Precision1, Rep2, Precision2>;
    using result = level<Reference, typename cg::rep, typename cg::precision>;
    return to_level<result>(to_linear(a) + to_linear(b));
}

// ============================================================================
// Type aliases
// ============================================================================

/**
 * @defgroup LevelTypes Standard Level Type Aliases
 * @{
 */

/** @brief A level referenced to 1 mW. */
template<typename Rep, typename Precision = std::ratio<1>>
using dbm_level = level<milliwatt_reference, Rep, Precision>;

/** @brief A level referenced to 1 W. */
template<typename Rep, typename Precision = std::ratio<1>>
using dbw_level = level<watt_reference, Rep, Precision>;

/** @brief A level referenced to 1 V. */
template<typename Rep, typename Precision = std::ratio<1>>
using dbv_level = level<volt_reference, Rep, Precision>;

/** @brief A level referenced to 1 mV. */
template<typename Rep, typename Precision = std::ratio<1>>
using dbmv_level = level<millivolt_reference, Rep, Precision>;

/** @brief A level referenced to 1 µV. */
template<typename Rep, typename Precision = std::ratio<1>>
using dbuv_level = level<microvolt_reference, Rep, Precision>;

/** @brief Power level in dBm, with 1 dB precision. */
using dbm = dbm_level<int64_t>;
/** @brief Power level in dBm, with 0.01 dB precision. */
using centi_dbm = dbm_level<int64_t, std::centi>;
/** @brief Power level in dBW, with 1 dB precision. */
using dbw = dbw_level<int64_t>;
/** @brief Voltage level in dBV, with 1 dB precision. */
using dbv = dbv_level<int64_t>;
/** @brief Voltage level in dBmV, with 1 dB precision. */
using dbmv = dbmv_level<int64_t>;
/** @brief Voltage level in dBµV, with 1 dB precision. */
using dbuv = dbuv_level<int64_t>;
/** @brief Voltage level in dBµV, with 0.01 dB precision. */
using centi_dbuv = dbuv_level<int64_t, std::centi>;

/** @} */ // end of LevelTypes group

// ============================================================================
// String conversion and formatting
// ============================================================================

/**
 * @brief Decibels opt out of the generic SI-prefix suffix machinery.
 *
 * A gain's precision is rendered as decimal places, not as an SI prefix:
 * "10.50dB", never "1050cdB". Leaving this specialization empty makes the
 * generic quantity to_string and formatter unusable for decibel_unit, so the
 * decibel-specific overloads below are the only candidates rather than merely
 * the better-matching ones.
 *
 * Gains and levels always render in the same unit as `{}` (dB, or the level's
 * reference symbol such as dBm): a format spec changes only the number
 * formatting. They have no SI prefix family, so the `b` and `s` format markers
 * and the base_unit and scaled to_string() tags render in that same unit,
 * with the value in the shortest fixed notation that round-trips.
 */
template<typename Precision>
struct quantity_suffix<decibel_unit, Precision> {};

/** @cond INTERNAL */
// The number of decimal places needed to render a count at the given precision
// as a fixed-point decimal, or -1 when the precision is not 1, 1/10, 1/100, ...
consteval int _decimal_places(intmax_t num, intmax_t den) {
    if (num != 1) {
        return -1;
    }
    int places = 0;
    while (den % 10 == 0) {
        den /= 10;
        ++places;
    }
    return den == 1 ? places : -1;
}

template<typename Precision>
inline constexpr bool _is_decimal_precision = _decimal_places(Precision::num, Precision::den) >= 0;

// A count at the given precision, in decibels, as a double.
template<typename Precision, typename Rep>
constexpr double _db_double(const Rep& count) {
    return static_cast<double>(count) * Precision::num / Precision::den;
}

// A count at the given precision rendered without a format spec, in decibels.
// Integer counts are shown exactly with one decimal place per decade of
// precision ("10.50"); floating-point counts in the shortest fixed notation
// that round-trips (never scientific).
template<typename Precision, typename Rep>
inline std::string _format_db(const Rep& count) {
    static_assert(_is_decimal_precision<Precision>, "precision cannot be rendered as a fixed-point decimal");
    if constexpr (std::floating_point<Rep>) {
        Rep value = count;
        if constexpr (Precision::den != 1) {
            value /= static_cast<Rep>(Precision::den);
        }
        return _fixed_string(value);
    } else if constexpr (treat_as_inexact_v<Rep>) {
        return _fixed_string(_db_double<Precision>(count));
    } else {
        constexpr int places = _decimal_places(Precision::num, Precision::den);
        if constexpr (places == 0) {
            return std::to_string(count);
        } else {
            const bool negative = count < Rep(0);
            // Compute the magnitude in unsigned arithmetic so that the most
            // negative representable count does not overflow on negation.
            const unsigned long long magnitude =
                negative ? 0ULL - static_cast<unsigned long long>(count) : static_cast<unsigned long long>(count);

            const unsigned long long whole = magnitude / Precision::den;
            const unsigned long long frac = magnitude % Precision::den;

            std::string frac_digits = std::to_string(frac);
            frac_digits.insert(0, static_cast<size_t>(places) - frac_digits.size(), '0');

            return (negative ? "-" : "") + std::to_string(whole) + "." + frac_digits;
        }
    }
}
/** @endcond */

/**
 * @brief Converts a gain to a string, e.g. "10dB" or "10.50dB".
 *
 * Unlike the generic quantity to_string, a gain's precision is rendered as
 * decimal places rather than as an SI prefix: nobody writes "1050cdB".
 */
template<typename Rep, typename Precision>
    requires _is_decimal_precision<typename Precision::type>
inline std::string to_string(const quantity<decibel_unit, Rep, Precision>& g) {
    return _format_db<typename Precision::type>(g.count()) + decibel_unit::symbol;
}

/**
 * @brief Converts a gain to a string in decibels, e.g. "10.5dB".
 *
 * A gain has no other unit, so this differs from to_string(g) only in
 * rendering the value in the shortest fixed notation that round-trips, as by
 * the `b` format marker.
 */
template<typename Rep, typename Precision>
    requires _is_decimal_precision<typename Precision::type>
inline std::string to_string(const quantity<decibel_unit, Rep, Precision>& g, base_unit_t) {
    return _fixed_string(_db_double<typename Precision::type>(g.count())) + decibel_unit::symbol;
}

/**
 * @brief Converts a gain to a string in decibels, e.g. "10.5dB".
 *
 * Decibels have no SI prefix family, so this is the same as
 * to_string(g, base_unit), as by the `s` format marker.
 */
template<typename Rep, typename Precision>
    requires _is_decimal_precision<typename Precision::type>
inline std::string to_string(const quantity<decibel_unit, Rep, Precision>& g, scaled_t) {
    return to_string(g, base_unit);
}

/**
 * @brief Converts a level to a string, e.g. "-73dBm" or "13.01dBm".
 */
template<typename Reference, typename Rep, typename Precision>
    requires _is_decimal_precision<typename Precision::type>
inline std::string to_string(const level<Reference, Rep, Precision>& l) {
    return _format_db<typename Precision::type>(l.count()) + Reference::symbol;
}

/**
 * @brief Converts a level to a string against its reference, e.g. "13.01dBm".
 *
 * A level has no other unit, so this differs from to_string(l) only in
 * rendering the value in the shortest fixed notation that round-trips, as by
 * the `b` format marker.
 */
template<typename Reference, typename Rep, typename Precision>
    requires _is_decimal_precision<typename Precision::type>
inline std::string to_string(const level<Reference, Rep, Precision>& l, base_unit_t) {
    return _fixed_string(_db_double<typename Precision::type>(l.count())) + Reference::symbol;
}

/**
 * @brief Converts a level to a string against its reference, e.g. "13.01dBm".
 *
 * Levels have no SI prefix family, so this is the same as
 * to_string(l, base_unit), as by the `s` format marker.
 */
template<typename Reference, typename Rep, typename Precision>
    requires _is_decimal_precision<typename Precision::type>
inline std::string to_string(const level<Reference, Rep, Precision>& l, scaled_t) {
    return to_string(l, base_unit);
}

} // namespace electro

namespace std {

template<typename Reference, typename Rep1, typename Precision1, typename Rep2, typename Precision2>
struct common_type<electro::level<Reference, Rep1, Precision1>, electro::level<Reference, Rep2, Precision2>> {
private:
    using common_offset = std::common_type_t<electro::gain<Rep1, Precision1>, electro::gain<Rep2, Precision2>>;

public:
    using type = electro::level<Reference, typename common_offset::rep, typename common_offset::precision>;
};

} // namespace std

#if CONFIG_ELECTRO_STD_FORMAT
/** @cond INTERNAL */
namespace electro {

// Shared formatter for gains and levels. "{}" renders the exact value as a
// fixed-point decimal ("10.50dB"). A non-empty spec controls only how the
// value (in decibels, as double) is rendered, never the unit:
// std::format("{:.1f}", 1050_cdB) == "10.5dB". The 'b' and 's' markers are
// accepted for consistency with the other quantities; decibels have no other
// unit, so they render in the same unit, and with nothing after the marker
// the value is rendered in the shortest fixed notation that round-trips:
// std::format("{:b}", 1050_cdB) == "10.5dB".
template<typename Precision>
struct _db_formatter {
private:
    std::formatter<double> _num;
    bool _has_spec = false;
    bool _marker = false;

public:
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && (*it == 'b' || *it == 's')) {
            _marker = true;
            ctx.advance_to(++it);
        }
        if (it == ctx.end() || *it == '}') {
            return it;
        }
        _has_spec = true;
        return _num.parse(ctx);
    }

    template<typename Rep, typename FormatContext>
    auto format(const Rep& count, const char* symbol, FormatContext& ctx) const {
        auto out = ctx.out();
        if (_has_spec) {
            out = _num.format(_db_double<Precision>(count), ctx);
        } else if (_marker) {
            out = std::format_to(out, "{}", _fixed_string(_db_double<Precision>(count)));
        } else {
            out = std::format_to(out, "{}", _format_db<Precision>(count));
        }
        return _format_append(out, symbol);
    }
};

} // namespace electro
/** @endcond */

namespace std {

// See electro::_db_formatter: "{}" -> "10.50dB", "{:.1f}" -> "10.5dB".
template<typename Rep, typename Precision>
    requires electro::_is_decimal_precision<typename Precision::type>
struct formatter<electro::quantity<electro::decibel_unit, Rep, Precision>, char>
    : electro::_db_formatter<typename Precision::type> {
    template<typename FormatContext>
    auto format(const electro::quantity<electro::decibel_unit, Rep, Precision>& g, FormatContext& ctx) const {
        return electro::_db_formatter<typename Precision::type>::format(g.count(), electro::decibel_unit::symbol, ctx);
    }
};

// See electro::_db_formatter; the level's reference symbol is appended
// ("13.01dBm", "{:.1f}" -> "13.0dBm").
template<typename Reference, typename Rep, typename Precision>
    requires electro::_is_decimal_precision<typename Precision::type>
struct formatter<electro::level<Reference, Rep, Precision>, char> : electro::_db_formatter<typename Precision::type> {
    template<typename FormatContext>
    auto format(const electro::level<Reference, Rep, Precision>& l, FormatContext& ctx) const {
        return electro::_db_formatter<typename Precision::type>::format(l.count(), Reference::symbol, ctx);
    }
};

} // namespace std
#endif

namespace electro_literals {

/** @brief Literal for decibels (e.g., 20_dB). */
template<char... Digits>
constexpr electro::decibels operator""_dB() {
    return detail::check_overflow<electro::decibels, Digits...>();
}

/** @brief Literal for centidecibels, i.e. 0.01 dB steps (e.g., 1050_cdB is 10.50 dB). */
template<char... Digits>
constexpr electro::centidecibels operator""_cdB() {
    return detail::check_overflow<electro::centidecibels, Digits...>();
}

/** @brief Literal for dBm (e.g., 20_dBm, or -73_dBm). */
template<char... Digits>
constexpr electro::dbm operator""_dBm() {
    return electro::dbm(detail::check_overflow<electro::decibels, Digits...>());
}

/** @brief Literal for dBm in 0.01 dB steps (e.g., 1301_cdBm is 13.01 dBm). */
template<char... Digits>
constexpr electro::centi_dbm operator""_cdBm() {
    return electro::centi_dbm(detail::check_overflow<electro::centidecibels, Digits...>());
}

/** @brief Literal for dBW (e.g., 10_dBW). */
template<char... Digits>
constexpr electro::dbw operator""_dBW() {
    return electro::dbw(detail::check_overflow<electro::decibels, Digits...>());
}

/** @brief Literal for dBV (e.g., 6_dBV). */
template<char... Digits>
constexpr electro::dbv operator""_dBV() {
    return electro::dbv(detail::check_overflow<electro::decibels, Digits...>());
}

/** @brief Literal for dBmV (e.g., 40_dBmV). */
template<char... Digits>
constexpr electro::dbmv operator""_dBmV() {
    return electro::dbmv(detail::check_overflow<electro::decibels, Digits...>());
}

/** @brief Literal for dBµV (e.g., 120_dBuV). */
template<char... Digits>
constexpr electro::dbuv operator""_dBuV() {
    return electro::dbuv(detail::check_overflow<electro::decibels, Digits...>());
}

#if ELECTRO_HAS_UTF8_LITERALS
/** @brief Literal for dBµV (e.g., 120_dBµV). Requires ELECTRO_HAS_UTF8_LITERALS. */
template<char... Digits>
constexpr electro::dbuv operator""_dBµV() {
    return electro::dbuv(detail::check_overflow<electro::decibels, Digits...>());
}
#endif

} // namespace electro_literals
