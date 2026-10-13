// Unit tests for electro library
// Uses Catch2 test framework with compile-time static_asserts

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <electro/electro>

using namespace electro;
using namespace electro_literals;

// =============================================================================
// Compile-time tests (static_assert)
// These verify correctness at compile time - if this file compiles, they pass.
// =============================================================================

// Construction and count
static_assert(volts(5).count() == 5);
static_assert(millivolts(3300).count() == 3300);
static_assert(microamperes(250).count() == 250);
static_assert(kiloohms(10).count() == 10);
static_assert(megawatts(5).count() == 5);
static_assert(picofarads(22).count() == 22);
static_assert(nanohenries(100).count() == 100);
static_assert(ampere_hours(2).count() == 2);
static_assert(kilowatt_hours(10).count() == 10);

// Same-unit arithmetic
static_assert(volts(5) + volts(3) == volts(8));
static_assert(volts(5) - volts(3) == volts(2));
static_assert(volts(5) * 2 == volts(10));
static_assert(2 * volts(5) == volts(10));
static_assert(volts(10) / 2 == volts(5));
static_assert(volts(10) / volts(2) == 5);
static_assert(volts(10) % 3 == volts(1));
static_assert(volts(10) % volts(3) == volts(1));
static_assert(-volts(5) == volts(-5));
static_assert(+volts(5) == volts(5));

// Precision conversions (equality across precisions)
static_assert(volts(1) == millivolts(1000));
static_assert(millivolts(1) == microvolts(1000));
static_assert(kilovolts(1) == volts(1000));
static_assert(amperes(1) == milliamperes(1000));
static_assert(milliamperes(1) == microamperes(1000));
static_assert(ohms(1) == milliohms(1000));
static_assert(kiloohms(1) == ohms(1000));
static_assert(megaohms(1) == kiloohms(1000));
static_assert(gigaohms(1) == megaohms(1000));
static_assert(watts(1) == milliwatts(1000));
static_assert(kilowatts(1) == watts(1000));
static_assert(farads(1) == millifarads(1000));
static_assert(microfarads(1) == picofarads(1000000));
static_assert(henries(1) == millihenries(1000));

// Hour-based charge and energy precisions
static_assert(ampere_hours(1) == coulombs(3600));
static_assert(ampere_hours(1) == milliampere_hours(1000));
static_assert(milliampere_hours(1000) == coulombs(3600));
static_assert(watt_hours(1) == joules(3600));
static_assert(kilowatt_hours(1) == watt_hours(1000));

// Implicit lossless conversions
constexpr millivolts mv_from_v = volts(1);
static_assert(mv_from_v.count() == 1000);
constexpr microamperes ua_from_ma = milliamperes(2);
static_assert(ua_from_ma.count() == 2000);
constexpr coulombs c_from_ah = ampere_hours(1);
static_assert(c_from_ah.count() == 3600);
constexpr milliampere_hours mah_from_ah = ampere_hours(2);
static_assert(mah_from_ah.count() == 2000);
constexpr millicoulombs mc_from_mah = milliampere_hours(1);
static_assert(mc_from_mah.count() == 3600);

// Explicit lossy conversions
static_assert(volts(millivolts(1500)).count() == 1);
static_assert(kiloohms(ohms(1500)).count() == 1);
static_assert(ampere_hours(coulombs(7200)).count() == 2);

// quantity_cast
static_assert(quantity_cast<millivolts>(volts(5)).count() == 5000);
static_assert(quantity_cast<volts>(millivolts(5500)).count() == 5);

// Comparison operators
static_assert(volts(5) == volts(5));
static_assert(volts(5) != volts(6));
static_assert(volts(5) < volts(6));
static_assert(volts(6) > volts(5));
static_assert(volts(5) <= volts(5));
static_assert(volts(5) >= volts(5));
static_assert(millivolts(999) < volts(1));

// Rounding conversions
static_assert(floor<volts>(millivolts(1500)) == volts(1));
static_assert(floor<volts>(millivolts(-1500)) == volts(-2));
static_assert(ceil<volts>(millivolts(1500)) == volts(2));
static_assert(ceil<volts>(millivolts(-1500)) == volts(-1));
static_assert(round<volts>(millivolts(1400)) == volts(1));
static_assert(round<volts>(millivolts(1600)) == volts(2));
static_assert(round<volts>(millivolts(1500)) == volts(2)); // tie to even
static_assert(round<volts>(millivolts(2500)) == volts(2)); // tie to even
static_assert(abs(millivolts(-5)) == millivolts(5));
static_assert(abs(millivolts(5)) == millivolts(5));

// zero/min/max
static_assert(volts::zero().count() == 0);
static_assert(volts::max().count() == std::numeric_limits<int64_t>::max());
static_assert(volts::min().count() == std::numeric_limits<int64_t>::lowest());

// =============================================================================
// Cross-unit arithmetic
// =============================================================================

// Ohm's law: V = I x R
static_assert(amperes(2) * ohms(6) == volts(12));
static_assert(ohms(6) * amperes(2) == volts(12));
static_assert(milliamperes(100) * kiloohms(2) == volts(200));
static_assert(milliamperes(100) * ohms(50) == millivolts(5000));
static_assert(microamperes(10) * megaohms(1) == volts(10));

// Ohm's law: I = V / R
static_assert(volts(12) / ohms(4) == amperes(3));
static_assert(millivolts(5000) / ohms(100) == milliamperes(50));
static_assert(volts(5) / kiloohms(10) == milliamperes(0)); // truncation at this precision
static_assert(millivolts(5000) / kiloohms(10) == microamperes(500));

// Ohm's law: R = V / I
static_assert(volts(12) / amperes(3) == ohms(4));
static_assert(millivolts(100) / amperes(2) == milliohms(50));

// Power: P = V x I
static_assert(volts(12) * amperes(2) == watts(24));
static_assert(amperes(2) * volts(12) == watts(24));
static_assert(volts(5) * milliamperes(500) == milliwatts(2500));
static_assert(kilovolts(11) * amperes(3) == kilowatts(33));

// Power: I = P / V, V = P / I
static_assert(watts(60) / volts(12) == amperes(5));
static_assert(milliwatts(2500) / volts(5) == milliamperes(500));
static_assert(watts(60) / amperes(5) == volts(12));

// Charge: Q = I x t
static_assert(amperes(2) * std::chrono::hours(3) == ampere_hours(6));
static_assert(std::chrono::hours(3) * amperes(2) == ampere_hours(6));
static_assert(milliamperes(100) * std::chrono::hours(2) == milliampere_hours(200));
static_assert(amperes(2) * std::chrono::seconds(5) == coulombs(10));
static_assert(milliamperes(100) * std::chrono::hours(2) == coulombs(720));

// Charge: I = Q / t, t = Q / I
static_assert(coulombs(10) / std::chrono::seconds(2) == amperes(5));
static_assert(ampere_hours(2) / amperes(1) == std::chrono::hours(2));
static_assert(milliampere_hours(2000) / milliamperes(100) == std::chrono::hours(20));

// Energy: E = P x t
static_assert(watts(100) * std::chrono::hours(5) == watt_hours(500));
static_assert(std::chrono::hours(5) * watts(100) == watt_hours(500));
static_assert(watts(100) * std::chrono::seconds(10) == joules(1000));
static_assert(kilowatts(2) * std::chrono::hours(3) == kilowatt_hours(6));

// Energy: P = E / t, t = E / P
static_assert(joules(1000) / std::chrono::seconds(10) == watts(100));
static_assert(watt_hours(500) / watts(100) == std::chrono::hours(5));

// Energy / charge / voltage relationships
static_assert(watt_hours(100) / volts(5) == ampere_hours(20));
static_assert(joules(100) / coulombs(20) == volts(5));

// Capacitance: Q = C x V
static_assert(farads(2) * volts(5) == coulombs(10));
static_assert(volts(5) * farads(2) == coulombs(10));
static_assert(microfarads(100) * volts(5) == microcoulombs(500));
static_assert(coulombs(10) / volts(5) == farads(2));
static_assert(coulombs(10) / farads(2) == volts(5));

// RC time constant
static_assert(kiloohms(47) * microfarads(10) == std::chrono::milliseconds(470));
static_assert(microfarads(10) * kiloohms(47) == std::chrono::milliseconds(470));
static_assert(ohms(1000) * microfarads(1000) == std::chrono::seconds(1));

// L/R time constant
static_assert(millihenries(10) / ohms(2) == std::chrono::milliseconds(5));
static_assert(microhenries(100) / milliohms(50) == std::chrono::milliseconds(2));

// Cross-unit result types
static_assert(std::is_same_v<decltype(amperes(2) * ohms(6)), volts>);
static_assert(std::is_same_v<decltype(millivolts(5000) / ohms(100)), milliamperes>);
static_assert(std::is_same_v<decltype(volts(12) * amperes(2)), watts>);
static_assert(std::is_same_v<decltype(amperes(2) * std::chrono::hours(3)), charge<int64_t, std::ratio<3600>>>);

// =============================================================================
// Floating-point representations
// =============================================================================

static_assert(voltage<double>(2.5).count() == 2.5);
static_assert(voltage<double>(5.0) / resistance<double>(2.0) == current<double>(2.5));
static_assert(voltage<double>(volts(5)).count() == 5.0); // implicit int -> double
static_assert(voltage<double>(millivolts(2500)).count() == 2.5);

// =============================================================================
// common_type
// =============================================================================

static_assert(std::is_same_v<std::common_type_t<volts, millivolts>, millivolts>);
static_assert(std::is_same_v<std::common_type_t<kiloohms, ohms>, ohms>);
static_assert(std::is_same_v<std::common_type_t<volts, volts>, volts>);
static_assert(std::is_same_v<std::common_type_t<ampere_hours, coulombs>, coulombs>);

// =============================================================================
// User-defined literals
// =============================================================================

static_assert(5_V == volts(5));
static_assert(3300_mV == millivolts(3300));
static_assert(500_uV == microvolts(500));
static_assert(11_kV == kilovolts(11));
static_assert(2_A == amperes(2));
static_assert(150_mA == milliamperes(150));
static_assert(250_uA == microamperes(250));
static_assert(220_Ohm == ohms(220));
static_assert(50_mOhm == milliohms(50));
static_assert(10_kOhm == kiloohms(10));
static_assert(1_MOhm == megaohms(1));
#if ELECTRO_HAS_UTF8_LITERALS
static_assert(220_Ω == ohms(220));
static_assert(50_mΩ == milliohms(50));
static_assert(10_kΩ == kiloohms(10));
static_assert(1_MΩ == megaohms(1));
#endif
static_assert(60_W == watts(60));
static_assert(250_mW == milliwatts(250));
static_assert(500_uW == microwatts(500));
static_assert(2_kW == kilowatts(2));
static_assert(5_MW == megawatts(5));
static_assert(10_C == coulombs(10));
static_assert(2000_mAh == milliampere_hours(2000));
static_assert(2_Ah == ampere_hours(2));
static_assert(100_J == joules(100));
static_assert(500_mJ == millijoules(500));
static_assert(4_kJ == kilojoules(4));
static_assert(100_Wh == watt_hours(100));
static_assert(10_kWh == kilowatt_hours(10));
static_assert(22_pF == picofarads(22));
static_assert(100_nF == nanofarads(100));
static_assert(10_uF == microfarads(10));
static_assert(1_F == farads(1));
static_assert(100_nH == nanohenries(100));
static_assert(47_uH == microhenries(47));
static_assert(10_mH == millihenries(10));
static_assert(1_H == henries(1));

// Literals in expressions
static_assert(100_mA * 2_kOhm == 200_V);
static_assert(12_V * 2_A == 24_W);
static_assert(5000_mV / 100_Ohm == 50_mA);

// =============================================================================
// Traits
// =============================================================================

static_assert(is_quantity_v<volts>);
static_assert(is_quantity_v<milliampere_hours>);
static_assert(!is_quantity_v<int>);
static_assert(!is_quantity_v<std::chrono::seconds>);
static_assert(treat_as_inexact_v<double>);
static_assert(treat_as_inexact_v<float>);
static_assert(!treat_as_inexact_v<int64_t>);

// =============================================================================
// Durations: only std::chrono::duration takes part in charge/energy arithmetic
// =============================================================================

namespace {

// A type with the shape of a std::chrono::duration (count(), rep, period) that
// is not one, such as a length quantity from a sibling unit library.
struct chrono_shaped {
    using rep = int64_t;
    using period = std::ratio<1>;
    constexpr rep count() const { return 3; }
};

template<typename A, typename B>
concept multipliable = requires(const A& a, const B& b) { a * b; };

template<typename A, typename B>
concept divisible = requires(const A& a, const B& b) { a / b; };

} // namespace

static_assert(!multipliable<amperes, chrono_shaped>);
static_assert(!multipliable<chrono_shaped, amperes>);
static_assert(!multipliable<watts, chrono_shaped>);
static_assert(!multipliable<chrono_shaped, watts>);
static_assert(!divisible<coulombs, chrono_shaped>);
static_assert(!divisible<joules, chrono_shaped>);

static_assert(multipliable<amperes, std::chrono::seconds>);
static_assert(multipliable<std::chrono::hours, watts>);
static_assert(multipliable<current<double>, std::chrono::duration<double, std::milli>>);
static_assert(divisible<coulombs, std::chrono::minutes>);
static_assert(divisible<joules, std::chrono::duration<float>>);
static_assert(amperes(2) * std::chrono::duration<int, std::milli>(500) == millicoulombs(1000));

// =============================================================================
// Runtime tests
// =============================================================================

TEST_CASE("compound assignment operators", "[quantity]") {
    millivolts v{1000};
    v += millivolts{500};
    REQUIRE(v.count() == 1500);
    v -= millivolts{300};
    REQUIRE(v.count() == 1200);
    v *= 2;
    REQUIRE(v.count() == 2400);
    v /= 4;
    REQUIRE(v.count() == 600);
    v %= 7;
    REQUIRE(v.count() == 5);
}

TEST_CASE("increment and decrement operators", "[quantity]") {
    volts v{5};
    REQUIRE((++v).count() == 6);
    REQUIRE((v++).count() == 6);
    REQUIRE(v.count() == 7);
    REQUIRE((--v).count() == 6);
    REQUIRE((v--).count() == 6);
    REQUIRE(v.count() == 5);
}

TEST_CASE("floating-point cross-unit arithmetic", "[quantity]") {
    voltage<double> v{5.0};
    resistance<double> r{3.0};
    auto i = v / r;
    REQUIRE(i.count() == Catch::Approx(5.0 / 3.0));

    auto p = v * current<double>{2.5};
    REQUIRE(p.count() == Catch::Approx(12.5));
}

TEST_CASE("to_string produces unit suffixes", "[format]") {
    REQUIRE(to_string(millivolts(3300)) == "3300mV");
    REQUIRE(to_string(volts(5)) == "5V");
    REQUIRE(to_string(kilovolts(11)) == "11kV");
    REQUIRE(to_string(microamperes(250)) == "250µA");
    REQUIRE(to_string(milliamperes(150)) == "150mA");
    REQUIRE(to_string(amperes(2)) == "2A");
    REQUIRE(to_string(milliohms(50)) == "50mΩ");
    REQUIRE(to_string(ohms(220)) == "220Ω");
    REQUIRE(to_string(kiloohms(10)) == "10kΩ");
    REQUIRE(to_string(megaohms(1)) == "1MΩ");
    REQUIRE(to_string(watts(60)) == "60W");
    REQUIRE(to_string(gigawatts(1)) == "1GW");
    REQUIRE(to_string(coulombs(10)) == "10C");
    REQUIRE(to_string(milliampere_hours(2000)) == "2000mAh");
    REQUIRE(to_string(ampere_hours(2)) == "2Ah");
    REQUIRE(to_string(joules(100)) == "100J");
    REQUIRE(to_string(watt_hours(500)) == "500Wh");
    REQUIRE(to_string(kilowatt_hours(10)) == "10kWh");
    REQUIRE(to_string(picofarads(22)) == "22pF");
    REQUIRE(to_string(nanofarads(100)) == "100nF");
    REQUIRE(to_string(microfarads(10)) == "10µF");
    REQUIRE(to_string(microhenries(47)) == "47µH");
    REQUIRE(to_string(henries(1)) == "1H");
}

TEST_CASE("to_string of floating-point quantities uses shortest fixed notation", "[format]") {
    REQUIRE(to_string(voltage<double>(2.5)) == "2.5V");
    REQUIRE(to_string(voltage<double>(1000000)) == "1000000V");
    REQUIRE(to_string(voltage<double, std::milli>(0.024)) == "0.024mV");
    REQUIRE(to_string(current<float, std::milli>(0.1f)) == "0.1mA");
    REQUIRE(to_string(voltage<double>(-3.0)) == "-3V");
    REQUIRE(to_string(voltage<double>(0.0)) == "0V");
}

TEST_CASE("to_string with base_unit renders in the base unit", "[format]") {
    REQUIRE(to_string(millivolts(3300), base_unit) == "3.3V");
    REQUIRE(to_string(millivolts(24), base_unit) == "0.024V");
    REQUIRE(to_string(kiloohms(47), base_unit) == "47000Ω");
    REQUIRE(to_string(microamperes(-1500), base_unit) == "-0.0015A");
    REQUIRE(to_string(volts(0), base_unit) == "0V");
    REQUIRE(to_string(voltage<double, std::kilo>(1000), base_unit) == "1000000V");
    // The hour-based precisions render in Ah and Wh
    REQUIRE(to_string(milliampere_hours(200), base_unit) == "0.2Ah");
    REQUIRE(to_string(ampere_hours(2), base_unit) == "2Ah");
    REQUIRE(to_string(watt_hours(500), base_unit) == "500Wh");
    REQUIRE(to_string(kilowatt_hours(2), base_unit) == "2000Wh");
}

TEST_CASE("to_string with scaled picks an SI prefix", "[format]") {
    REQUIRE(to_string(millivolts(3300), scaled) == "3.3V");
    REQUIRE(to_string(millivolts(33), scaled) == "33mV");
    REQUIRE(to_string(ohms(4700), scaled) == "4.7kΩ");
    REQUIRE(to_string(kiloohms(4700), scaled) == "4.7MΩ");
    REQUIRE(to_string(megawatts(2500), scaled) == "2.5GW");
    REQUIRE(to_string(gigawatts(2500), scaled) == "2.5TW");
    REQUIRE(to_string(gigawatts(2500000), scaled) == "2500TW"); // largest prefix
    REQUIRE(to_string(picofarads(100000), scaled) == "100nF");
    REQUIRE(to_string(microfarads(1000), scaled) == "1mF");
    REQUIRE(to_string(microamperes(-2500), scaled) == "-2.5mA");
    REQUIRE(to_string(capacitance<double, std::pico>(0.5), scaled) == "500fF"); // smallest prefix
    REQUIRE(to_string(volts(0), scaled) == "0V");
    REQUIRE(to_string(millivolts(0), scaled) == "0V");
    REQUIRE(to_string(voltage<double>(0.0042), scaled) == "4.2mV");
    // Values landing exactly on a prefix are exactly 1 in it
    REQUIRE(to_string(millivolts(1000), scaled) == "1V");
    REQUIRE(to_string(nanohenries(1000), scaled) == "1µH");
    // The hour-based precisions have no SI prefix family: own unit
    REQUIRE(to_string(milliampere_hours(2500), scaled) == "2500mAh");
    REQUIRE(to_string(ampere_hours(2), scaled) == "2Ah");
    REQUIRE(to_string(watt_hours(1500), scaled) == "1500Wh");
    REQUIRE(to_string(kilowatt_hours(10), scaled) == "10kWh");
}

TEST_CASE("to_string of cross-unit results", "[format]") {
    // Result precisions land on conventional suffixes
    REQUIRE(to_string(milliamperes(100) * kiloohms(2)) == "200V");
    REQUIRE(to_string(millivolts(5000) / ohms(100)) == "50mA");
    REQUIRE(to_string(milliamperes(100) * std::chrono::hours(2)) == "200mAh");
    REQUIRE(to_string(watts(100) * std::chrono::hours(5)) == "500Wh");
}

#if CONFIG_ELECTRO_STD_FORMAT
TEST_CASE("std::format support", "[format]") {
    REQUIRE(std::format("{}", millivolts(3300)) == "3300mV");
    REQUIRE(std::format("{}", kiloohms(10)) == "10kΩ");
    REQUIRE(std::format("{}", milliampere_hours(2000)) == "2000mAh");
    REQUIRE(std::format("{}", kilowatt_hours(10)) == "10kWh");
    REQUIRE(
        std::format("battery: {} at {}", milliampere_hours(2000), millivolts(3700)) == "battery: 2000mAh at 3700mV"
    );
}

TEST_CASE("std::format with spec renders in the own unit", "[format]") {
    // A format spec changes only the number formatting, never the unit.
    REQUIRE(std::format("{:.1f}", millivolts(3300)) == "3300.0mV");
    REQUIRE(std::format("{:.2f}", millivolts(3312)) == "3312.00mV");
    REQUIRE(std::format("{:.1f}", kilovolts(5)) == "5.0kV");
    REQUIRE(std::format("{:g}", microamperes(1500)) == "1500µA");
    REQUIRE(std::format("{:.1f}", milliohms(4700)) == "4700.0mΩ");
    REQUIRE(std::format("{:.1f}", millivolts(-3300)) == "-3300.0mV");
    REQUIRE(std::format("{:.1f}", volts(0)) == "0.0V");
    REQUIRE(std::format("{:.3f}", voltage<double, std::milli>(2.5)) == "2.500mV");

    // Width and alignment pass through to the numeric part.
    REQUIRE(std::format("{:9.1f}", millivolts(3300)) == "   3300.0mV");

    // Hour-based precisions render in their own unit too.
    REQUIRE(std::format("{:.1f}", milliampere_hours(200)) == "200.0mAh");
    REQUIRE(std::format("{:.1f}", ampere_hours(2)) == "2.0Ah");
    REQUIRE(std::format("{:.1f}", watt_hours(500)) == "500.0Wh");
    REQUIRE(std::format("{:.0f}", kilowatt_hours(2)) == "2kWh");
}

TEST_CASE("std::format of floating-point quantities agrees with to_string", "[format]") {
    REQUIRE(std::format("{}", voltage<double>(1000000)) == "1000000V");
    REQUIRE(std::format("{}", voltage<double>(2.5)) == "2.5V");
    REQUIRE(std::format("{}", current<float, std::milli>(0.1f)) == "0.1mA");
    for (double v : {0.0, -0.0, 2.5, -3.0, 1e6, 1e-15, 0.1, 123456.789}) {
        const voltage<double, std::milli> q(v);
        REQUIRE(std::format("{}", q) == to_string(q));
        REQUIRE(std::format("{:b}", q) == to_string(q, base_unit));
        REQUIRE(std::format("{:s}", q) == to_string(q, scaled));
    }
}

TEST_CASE("std::format b marker renders in the base unit", "[format]") {
    REQUIRE(std::format("{:b.1f}", millivolts(3300)) == "3.3V");
    REQUIRE(std::format("{:b.2f}", millivolts(3312)) == "3.31V");
    REQUIRE(std::format("{:b.1f}", kilovolts(5)) == "5000.0V");
    REQUIRE(std::format("{:bg}", microamperes(1500)) == "0.0015A");
    REQUIRE(std::format("{:b.1f}", milliampere_hours(200)) == "0.2Ah");
    REQUIRE(std::format("{:b.0f}", kilowatt_hours(2)) == "2000Wh");
    REQUIRE(std::format("{:b.1f}", millivolts(-3300)) == "-3.3V");
    REQUIRE(std::format("{:b.1f}", millivolts(0)) == "0.0V");
    REQUIRE(std::format("{:b.2e}", microvolts(1500)) == "1.50e-03V");

    // Without a trailing spec: shortest fixed notation, never scientific.
    REQUIRE(std::format("{:b}", millivolts(3300)) == "3.3V");
    REQUIRE(std::format("{:b}", millivolts(24)) == "0.024V");
    REQUIRE(std::format("{:b}", megawatts(5)) == "5000000W");
    REQUIRE(std::format("{:b}", milliampere_hours(200)) == "0.2Ah");
    REQUIRE(std::format("{:b}", millivolts(-24)) == "-0.024V");

    // Width, fill and alignment after the marker.
    REQUIRE(std::format("{:b7.1f}", millivolts(3300)) == "    3.3V");
    REQUIRE(std::format("{:b*<7.1f}", millivolts(3300)) == "3.3****V");
    REQUIRE(std::format("{:b>6}", millivolts(24)) == " 0.024V");
}

TEST_CASE("std::format s marker picks an SI prefix", "[format]") {
    REQUIRE(std::format("{:s}", millivolts(3300)) == "3.3V");
    REQUIRE(std::format("{:s.1f}", millivolts(33)) == "33.0mV");
    REQUIRE(std::format("{:s}", ohms(4700)) == "4.7kΩ");
    REQUIRE(std::format("{:s.2f}", ohms(4700)) == "4.70kΩ");
    REQUIRE(std::format("{:s}", picofarads(4700)) == "4.7nF");
    REQUIRE(std::format("{:s}", microamperes(-2500)) == "-2.5mA");
    REQUIRE(std::format("{:s.1f}", microamperes(-2500)) == "-2.5mA");
    REQUIRE(std::format("{:s}", volts(0)) == "0V");
    REQUIRE(std::format("{:s.1f}", millivolts(0)) == "0.0V");
    REQUIRE(std::format("{:s}", kilowatts(1500)) == "1.5MW");

    // The hour-based precisions have no SI prefix family: own unit.
    REQUIRE(std::format("{:s}", milliampere_hours(2500)) == "2500mAh");
    REQUIRE(std::format("{:s.1f}", watt_hours(1500)) == "1500.0Wh");

    // Width, fill and alignment after the marker.
    REQUIRE(std::format("{:s8.2f}", ohms(4700)) == "    4.70kΩ");
    REQUIRE(std::format("{:s*^8.1f}", ohms(4700)) == "**4.7***kΩ");
}

TEST_CASE("std::format of integer counts is exact", "[format]") {
    REQUIRE(std::format("{}", millivolts(-3300)) == "-3300mV");
    REQUIRE(std::format("{}", volts(0)) == "0V");
    REQUIRE(std::format("{}", megawatts(5)) == "5MW");
}
#endif

TEST_CASE("battery runtime example", "[integration]") {
    // A 2000 mAh battery driving a 100 mA load runs for 20 hours
    auto capacity = 2000_mAh;
    auto load = 100_mA;
    auto runtime = capacity / load;
    REQUIRE(std::chrono::duration_cast<std::chrono::hours>(runtime) == std::chrono::hours(20));

    // A 100 Wh pack at 5 V holds 20 Ah of charge
    auto stored = watt_hours(100) / volts(5);
    REQUIRE(stored == ampere_hours(20));
}

TEST_CASE("voltage divider example", "[integration]") {
    // 12 V across 10 kΩ + 20 kΩ in series
    auto vin = 12_V;
    auto r1 = 10_kOhm;
    auto r2 = 20_kOhm;
    auto total = r1 + r2;
    REQUIRE(total == kiloohms(30));

    // Current through the divider: 12 V / 30 kΩ = 400 µA
    auto i = quantity_cast<microvolts>(vin) / total;
    REQUIRE(i == microamperes(400));

    // Voltage across r2: 400 µA x 20 kΩ = 8 V
    auto v2 = i * r2;
    REQUIRE(v2 == volts(8));
}
