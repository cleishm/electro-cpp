// Unit tests for the electro decibel types
// Uses Catch2 test framework with compile-time static_asserts

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <electro/decibel>

using namespace electro;
using namespace electro_literals;

// =============================================================================
// Compile-time tests (static_assert)
// =============================================================================

// Construction and count
static_assert(decibels(10).count() == 10);
static_assert(dbm(-73).count() == -73);
static_assert(centi_dbm(1301).count() == 1301);
static_assert(dbm(decibels(20)) == dbm(20));
static_assert(dbm().count() == 0);
static_assert(dbm::reference_level() == dbm(0));

// Literals
static_assert(20_dB == decibels(20));
static_assert(1050_cdB == centidecibels(1050));
static_assert(20_dBm == dbm(20));
static_assert(-73_dBm == dbm(-73));
static_assert(1301_cdBm == centi_dbm(1301));
static_assert(10_dBW == dbw(10));
static_assert(6_dBV == dbv(6));
static_assert(40_dBmV == dbmv(40));
static_assert(120_dBuV == dbuv(120));
static_assert(120_dBµV == dbuv(120));

// Gain is an ordinary quantity: it adds, subtracts, negates and scales
static_assert(10_dB + 10_dB == 20_dB);
static_assert(20_dB - 5_dB == 15_dB);
static_assert(10_dB * 2 == 20_dB);
static_assert(2 * 10_dB == 20_dB);
static_assert(20_dB / 2 == 10_dB);
static_assert(20_dB / 10_dB == 2);
static_assert(-10_dB == decibels(-10));

// Affine algebra: level +/- gain -> level, level - level -> gain
static_assert(20_dBm + 3_dB == dbm(23));
static_assert(3_dB + 20_dBm == dbm(23));
static_assert(20_dBm - 3_dB == dbm(17));
static_assert(20_dBm - 5_dBm == 15_dB);
static_assert(-66_dBm - -100_dBm == 34_dB);
static_assert(-(-73_dBm) == dbm(73));
static_assert(+20_dBm == dbm(20));

// A complete link budget, evaluated at compile time
static_assert(20_dBm + 6_dB - 2_dB - 90_dB == dbm(-66));

// Comparisons
static_assert(20_dBm > 5_dBm);
static_assert(-73_dBm < -50_dBm);
static_assert(dbm(10) == centi_dbm(1000));
static_assert(dbm(10) != centi_dbm(1001));

// Mixed precision resolves through std::common_type
static_assert(20_dBm + 50_cdB == centi_dbm(2050));
static_assert(std::is_same_v<std::common_type_t<dbm, centi_dbm>, centi_dbm>);
static_assert(std::is_same_v<decltype(20_dBm - 5_dBm), decibels>);
static_assert(std::is_same_v<decltype(20_dBm + 50_cdB), centi_dbm>);

// Lossless conversions are implicit; lossy ones are explicit
static_assert(std::is_convertible_v<dbm, centi_dbm>);
static_assert(!std::is_convertible_v<centi_dbm, dbm>);
static_assert(std::is_constructible_v<dbm, centi_dbm>);
static_assert(dbm(centi_dbm(1301)) == dbm(13)); // truncates
static_assert(level_cast<dbm>(centi_dbm(1399)) == dbm(13));

// Rounding
static_assert(floor<dbm>(centi_dbm(1350)) == dbm(13));
static_assert(ceil<dbm>(centi_dbm(1301)) == dbm(14));
static_assert(round<dbm>(centi_dbm(1350)) == dbm(14)); // tie: 13 is odd, round to even
static_assert(round<dbm>(centi_dbm(1250)) == dbm(12)); // tie: 12 is even
static_assert(floor<dbm>(centi_dbm(-50)) == dbm(-1));
static_assert(ceil<dbm>(centi_dbm(-50)) == dbm(0));

// Traits
static_assert(is_quantity_v<decibels>);
static_assert(!is_quantity_v<dbm>);
static_assert(is_level_v<dbm>);
static_assert(is_level_v<centi_dbuv>);
static_assert(!is_level_v<decibels>);
static_assert(!is_level_v<volts>);
static_assert(decibel_reference<milliwatt_reference>);
static_assert(!decibel_reference<volt_unit>);
static_assert(is_power_reference_v<milliwatt_reference>);
static_assert(is_power_reference_v<watt_reference>);
static_assert(!is_power_reference_v<microvolt_reference>);
static_assert(is_field_reference_v<microvolt_reference>);
static_assert(is_field_reference_v<volt_reference>);
// The reference traits are total: a non-reference type is simply false
static_assert(!is_power_reference_v<volt_unit>);
static_assert(!is_field_reference_v<volt_unit>);
static_assert(!is_power_reference_v<int>);
static_assert(!is_field_reference_v<double>);

// =============================================================================
// Ill-formed operations
//
// These are the whole point of modelling a level as a time_point rather than a
// duration. A positive control (level + gain) guards against the detection
// concepts being vacuously true.
// =============================================================================

template<typename A, typename B>
concept can_add = requires(A a, B b) { a + b; };
template<typename A, typename B>
concept can_subtract = requires(A a, B b) { a - b; };
template<typename A, typename B>
concept can_multiply = requires(A a, B b) { a * b; };
template<typename A, typename B>
concept can_divide = requires(A a, B b) { a / b; };
template<typename A>
concept can_modulo = requires(A a) { a % a; };
template<typename A>
concept can_add_powers = requires(A a) { add_powers(a, a); };

// Positive controls
static_assert(can_add<dbm, decibels>);
static_assert(can_add<decibels, dbm>);
static_assert(can_subtract<dbm, decibels>);
static_assert(can_subtract<dbm, dbm>);
static_assert(can_add<decibels, decibels>);
static_assert(can_multiply<decibels, int>);

// Adding two absolute levels is meaningless; use add_powers()
static_assert(!can_add<dbm, dbm>);
// Scaling an absolute level is meaningless
static_assert(!can_multiply<dbm, int>);
static_assert(!can_divide<dbm, int>);
static_assert(!can_divide<dbm, dbm>);
static_assert(!can_modulo<dbm>);
// A level is not a gain, and references do not mix
static_assert(!can_add<dbm, dbw>);
static_assert(!can_subtract<dbm, dbw>);
static_assert(!can_add<dbm, dbuv>);
static_assert(!can_subtract<dbm, dbuv>);
// Levels do not mix with linear quantities
static_assert(!can_add<dbm, watts>);
static_assert(!can_add<dbm, volts>);
static_assert(!can_add<decibels, volts>);
// A bare number is not a level
static_assert(!std::is_convertible_v<int, dbm>);
static_assert(std::is_constructible_v<dbm, int>);
// add_powers is only defined for power references
static_assert(can_add_powers<dbm>);
static_assert(can_add_powers<dbw>);
static_assert(!can_add_powers<dbuv>);
static_assert(!can_add_powers<dbv>);

// =============================================================================
// Runtime tests
// =============================================================================

TEST_CASE("compound assignment applies gains", "[level]") {
    dbm signal{-73};
    signal += 20_dB;
    REQUIRE(signal.count() == -53);
    signal -= 3_dB;
    REQUIRE(signal.count() == -56);
}

TEST_CASE("to_linear expresses a level at its reference precision", "[level][linear]") {
    // dBm references 1 mW, so to_linear yields milliwatts
    REQUIRE(to_linear(0_dBm).count() == Catch::Approx(1.0));
    REQUIRE(to_linear(30_dBm).count() == Catch::Approx(1000.0));
    REQUIRE(to_linear(-30_dBm).count() == Catch::Approx(0.001));
    REQUIRE(to_linear(dbm::reference_level()).count() == Catch::Approx(1.0));
    static_assert(std::is_same_v<decltype(to_linear(0_dBm)), power<double, std::milli>>);

    // dBW references 1 W
    REQUIRE(to_linear(0_dBW).count() == Catch::Approx(1.0));
    REQUIRE(to_linear(10_dBW).count() == Catch::Approx(10.0));
    static_assert(std::is_same_v<decltype(to_linear(0_dBW)), power<double>>);

    // Field references use 20*log10, and yield their own unit
    REQUIRE(to_linear(120_dBuV).count() == Catch::Approx(1000000.0));
    REQUIRE(to_linear(0_dBV).count() == Catch::Approx(1.0));
    REQUIRE(to_linear(6_dBV).count() == Catch::Approx(1.9952623));
    static_assert(std::is_same_v<decltype(to_linear(0_dBuV)), voltage<double, std::micro>>);

    // quantity_cast retargets the result
    REQUIRE(quantity_cast<watts>(to_linear(30_dBm)) == watts(1));
}

TEST_CASE("to_level converts a linear quantity to a level", "[level][linear]") {
    REQUIRE(to_level<dbm>(milliwatts(1)) == 0_dBm);
    REQUIRE(to_level<dbm>(watts(1)) == 30_dBm);
    REQUIRE(to_level<dbm>(microwatts(1)) == -30_dBm);
    REQUIRE(to_level<dbw>(watts(10)) == 10_dBW);

    // Field references: 1 V is 120 dBµV, not 60
    REQUIRE(to_level<dbuv>(volts(1)) == 120_dBuV);
    REQUIRE(to_level<dbmv>(volts(1)) == 60_dBmV);
    REQUIRE(to_level<dbv>(volts(1)) == 0_dBV);

    // Rounds to nearest rather than truncating
    REQUIRE(to_level<dbm>(milliwatts(2)) == 3_dBm);          // 3.0103 dB
    REQUIRE(to_level<centi_dbm>(milliwatts(2)) == 301_cdBm); // 3.0103 dB
    REQUIRE(to_level<dbm>(microwatts(500)) == -3_dBm);       // -3.0103 dB
}

TEST_CASE("to_level and to_linear round-trip", "[level][linear]") {
    for (int64_t db : {-100, -73, -30, -1, 0, 1, 20, 30}) {
        REQUIRE(to_level<centi_dbm>(to_linear(centi_dbm(db * 100))) == centi_dbm(db * 100));
    }
    REQUIRE(to_level<centi_dbuv>(to_linear(centi_dbuv(12000))) == centi_dbuv(12000));
}

TEST_CASE("a zero quantity has no finite level", "[level][linear]") {
    REQUIRE(to_level<dbm>(milliwatts(0)) == dbm::min());
    REQUIRE(to_level<centi_dbm>(watts(0)) == centi_dbm::min());
}

TEST_CASE("add_powers combines uncorrelated signals", "[level]") {
    // Two equal powers sum to +3.0103 dB, not +10 dB and not double the dBm value
    REQUIRE(add_powers(1000_cdBm, 1000_cdBm) == 1301_cdBm);
    REQUIRE(add_powers(0_dBm, 0_dBm) == 3_dBm);

    // A much weaker signal barely moves the total
    REQUIRE(add_powers(centi_dbm(3000), centi_dbm(-3000)) == centi_dbm(3000));

    // Order does not matter, and the result carries the finer precision
    static_assert(std::is_same_v<decltype(add_powers(dbm(0), centi_dbm(0))), centi_dbm>);
    REQUIRE(add_powers(0_dBm, 1000_cdBm) == add_powers(1000_cdBm, 0_dBm));
}

TEST_CASE("power and amplitude ratios are distinct", "[gain]") {
    // 3 dB doubles power; 6 dB doubles amplitude
    REQUIRE(power_ratio(3_dB) == Catch::Approx(1.9952623));
    REQUIRE(power_ratio(10_dB) == Catch::Approx(10.0));
    REQUIRE(power_ratio(0_dB) == Catch::Approx(1.0));
    REQUIRE(amplitude_ratio(6_dB) == Catch::Approx(1.9952623));
    REQUIRE(amplitude_ratio(20_dB) == Catch::Approx(10.0));

    // And the inverses
    REQUIRE(power_gain(10.0).count() == Catch::Approx(10.0));
    REQUIRE(amplitude_gain(10.0).count() == Catch::Approx(20.0));
    REQUIRE(power_ratio(power_gain(7.5)) == Catch::Approx(7.5));
    REQUIRE(amplitude_ratio(amplitude_gain(7.5)) == Catch::Approx(7.5));

    // Applying a gain to a linear quantity, via the appropriate ratio
    REQUIRE(watts(2) * power_ratio(10_dB) == power<double>(20.0));
}

TEST_CASE("floating-point levels and gains", "[level][gain]") {
    dbm_level<double> signal{-73.5};
    REQUIRE(signal.count() == Catch::Approx(-73.5));

    auto amplified = signal + gain<double>(20.25);
    REQUIRE(amplified.count() == Catch::Approx(-53.25));

    auto delta = amplified - signal;
    REQUIRE(delta.count() == Catch::Approx(20.25));

    // An integer level converts implicitly into a floating-point one
    dbm_level<double> from_int = 20_dBm;
    REQUIRE(from_int.count() == Catch::Approx(20.0));
}

TEST_CASE("to_string renders decibels as decimals", "[format]") {
    REQUIRE(to_string(10_dB) == "10dB");
    REQUIRE(to_string(-10_dB) == "-10dB");
    REQUIRE(to_string(1050_cdB) == "10.50dB");
    REQUIRE(to_string(millidecibels(1234)) == "1.234dB");

    REQUIRE(to_string(20_dBm) == "20dBm");
    REQUIRE(to_string(-73_dBm) == "-73dBm");
    REQUIRE(to_string(1301_cdBm) == "13.01dBm");
    REQUIRE(to_string(centi_dbm(-50)) == "-0.50dBm");
    REQUIRE(to_string(centi_dbm(-1)) == "-0.01dBm");
    REQUIRE(to_string(centi_dbm(0)) == "0.00dBm");

    REQUIRE(to_string(10_dBW) == "10dBW");
    REQUIRE(to_string(0_dBV) == "0dBV");
    REQUIRE(to_string(40_dBmV) == "40dBmV");
    REQUIRE(to_string(120_dBuV) == "120dBµV");
}

#if CONFIG_ELECTRO_STD_FORMAT
TEST_CASE("std::format support for levels and gains", "[format]") {
    REQUIRE(std::format("{}", -73_dBm) == "-73dBm");
    REQUIRE(std::format("{}", 1301_cdBm) == "13.01dBm");
    REQUIRE(std::format("{}", 20_dB) == "20dB");
    REQUIRE(std::format("{}", 1050_cdB) == "10.50dB");
    REQUIRE(std::format("rx {} with {} margin", -66_dBm, 34_dB) == "rx -66dBm with 34dB margin");
}

TEST_CASE("std::format with spec renders gains and levels in decibels", "[format]") {
    // A floating-point format spec renders the value in decibels.
    REQUIRE(std::format("{:.1f}", 1050_cdB) == "10.5dB");
    REQUIRE(std::format("{:.0f}", 20_dB) == "20dB");
    REQUIRE(std::format("{:.1f}", 1301_cdBm) == "13.0dBm");
    REQUIRE(std::format("{:.2f}", -73_dBm) == "-73.00dBm");

    // Width and alignment pass through to the numeric part.
    REQUIRE(std::format("{:6.1f}", 1050_cdB) == "  10.5dB");
}
#endif

TEST_CASE("link budget example", "[integration]") {
    auto tx_power = 20_dBm;
    auto antenna_gain = 6_dB;
    auto cable_loss = 2_dB;
    auto path_loss = 90_dB;

    auto eirp = tx_power + antenna_gain - cable_loss;
    REQUIRE(eirp == 24_dBm);

    auto rx_power = eirp - path_loss;
    REQUIRE(rx_power == -66_dBm);

    auto sensitivity = -100_dBm;
    auto margin = rx_power - sensitivity;
    REQUIRE(margin == 34_dB);
    REQUIRE(rx_power > sensitivity);

    // And the received power really is 251 pW
    REQUIRE(to_linear(rx_power).count() == Catch::Approx(2.5118864e-7));
}

TEST_CASE("amplifier cascade example", "[integration]") {
    // Two identical 12 dB stages, then a 3 dB attenuator
    auto stage = 12_dB;
    auto total = stage * 2 - 3_dB;
    REQUIRE(total == 21_dB);

    auto out = -40_dBm + total;
    REQUIRE(out == -19_dBm);

    // A 1 mW input through 30 dB of gain is 1 W out
    REQUIRE(quantity_cast<watts>(to_linear(to_level<dbm>(milliwatts(1)) + 30_dB)) == watts(1));
}

TEST_CASE("receiver noise floor example", "[integration]") {
    // Thermal noise in a 1 MHz bandwidth is about -114 dBm; a signal 10 dB
    // above it, combined with the noise, is only ~0.4 dB stronger than the
    // signal alone.
    auto noise = -11400_cdBm;
    auto signal = noise + 1000_cdB;
    REQUIRE(signal == -10400_cdBm);

    auto combined = add_powers(signal, noise);
    REQUIRE(combined - signal == 41_cdB); // 0.41 dB
    REQUIRE(combined > signal);
}
