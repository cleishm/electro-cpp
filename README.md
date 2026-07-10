# electro

A type-safe, header-only C++20 library for electrical units, modeled after `std::chrono::duration`.

📚 **[Full API Documentation](https://cleishm.github.io/electro-cpp/)**

## Features

- **Type-safe electrical quantities**: voltage, current, resistance, power, charge, energy, capacitance, inductance
- **Standard SI multiples**: microvolts through kilovolts, milliohms through gigaohms, picofarads through farads, and more
- **Cross-quantity arithmetic** following circuit theory: Ohm's law (`I * R = V`), power (`V * I = P`), and friends
- **`std::chrono` interop**: `mA x hours = mAh`, `W x hours = Wh`, `Ω x F = RC time constant` (as a `std::chrono::duration`)
- **Decibels**: `dB` gains and `dBm`/`dBW`/`dBµV` levels, related as `duration` is to `time_point`, so that `dBm + dBm` is a compile error
- **Integer and floating-point representations** for exact or fractional precision
- **Lossless implicit conversions** (lossy conversions require explicit casts)
- **User-defined literals** for concise notation (`3300_mV`, `10_kΩ`, `2000_mAh`, `-73_dBm`)
- **`std::format` support** (when available)
- **Fully constexpr** - all operations can be evaluated at compile time (except the dB ↔ linear conversions, which need `std::log10`/`std::pow`)

## Requirements

- C++20 compiler (GCC 12+, Clang 15+, MSVC 19.30+)
- **MSVC users:** The `/utf-8` flag is required (the source contains unit symbols such as Ω)

## Installation

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    electro
    GIT_REPOSITORY https://github.com/cleishm/electro-cpp.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(electro)

target_link_libraries(your_target PRIVATE electro::electro)
```

### vcpkg

```bash
vcpkg install cleishm-electro-cpp
```

```cmake
find_package(electro CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE electro::electro)
```

### Manual

Copy the `include/electro/` directory to your project.

## Usage

```cpp
#include <electro/electro>  // or <electro/electro.hpp>

using namespace electro;
using namespace electro_literals;

// Basic quantities
millivolts vbat{3300};
milliamperes load{150};
kiloohms pullup{10};

// Using literals
auto usb = 5_V;
auto shunt = 50_mOhm;   // or 50_mΩ
auto cap = 100_nF;

// Precision control
millivolts precise{3312};
volts coarse = volts(precise);   // explicit lossy conversion (truncates to 3 V)
millivolts back = coarse;        // implicit lossless conversion (3000 mV)

// Same-unit arithmetic
auto total = 10_kOhm + 20_kOhm;  // 30 kΩ
auto scaled = 100_mA * 3;        // 300 mA
auto ratio = 10_kOhm / 5_kOhm;   // 2 (scalar)

// Comparisons work across precisions
if (kilovolts{1} == volts{1000}) {
    // true - same voltage, different precision
}

// String conversion
std::string s = to_string(kiloohms(10));  // "10kΩ"
```

### Cross-Quantity Arithmetic

Quantities of different kinds combine according to circuit theory, producing
results in the correct unit with a precision derived from the operands:

```cpp
// Ohm's law: V = I x R
auto v = 100_mA * 2_kOhm;          // 200 V   (milli x kilo = 1)
auto i = 5000_mV / 100_Ohm;        // 50 mA
auto r = 12_V / 3_A;               // 4 Ω

// Power: P = V x I
auto p = 12_V * 2_A;               // 24 W
auto i2 = 60_W / 12_V;             // 5 A

// Charge and energy via std::chrono
auto q = 100_mA * std::chrono::hours(2);   // 200 mAh
auto e = 100_W * std::chrono::hours(5);    // 500 Wh
auto runtime = 2000_mAh / 100_mA;          // 20 h (a std::chrono::duration)
auto capacity = 100_Wh / 5_V;              // 20 Ah

// Time constants
auto tau = 47_kOhm * 10_uF;        // 470 ms (RC)
auto lr = 10_mH / 2_Ohm;           // 5 ms (L/R)
```

Products are exact for integer representations. Quotients truncate like
integer division, so choose operand precisions accordingly:
`5_V / 3_Ohm` is `1 A`, while `5000_mV / 3_Ohm` is `1666 mA`. Floating-point
representations (e.g. `voltage<double>`) avoid truncation entirely.

## Decibels and Levels

Decibels live in a separate, opt-in header:

```cpp
#include <electro/decibel>  // or <electro/decibel.hpp>
```

They come in two kinds, and conflating them is the classic source of bugs. This
library keeps them apart using the same distinction `std::chrono` draws between
a `duration` and a `time_point`:

| | | Closed under |
|---|---|---|
| **gain** (`dB`) | a dimensionless *ratio* — the `duration` analog | `+`, `-`, `* scalar` |
| **level** (`dBm`, `dBW`, `dBµV`) | an *absolute* point on a log scale — the `time_point` analog | `level ± gain`, `level - level → gain` |

```cpp
auto tx     = 20_dBm;
auto eirp   = tx + 6_dB - 2_dB;    // level ± gain -> level (24 dBm)
auto rx     = eirp - 90_dB;        // path loss             (-66 dBm)
auto margin = rx - -100_dBm;       // level - level -> gain (34 dB)

// tx + tx;        // error: adding two absolute levels is meaningless
// tx * 2;         // error: scaling an absolute level is meaningless
// tx + 10_dBW;    // error: references do not mix
```

That last group matters: a plain `dBm` unit tag would accept all three and
silently compute nonsense (`10 dBm + 10 dBm` is `13.01 dBm`, not `20 dBm`).
Combining the powers of two uncorrelated signals is a named function, never
`operator+`:

```cpp
add_powers(1000_cdBm, 1000_cdBm);  // 13.01 dBm
```

### Crossing into the linear domain

Conversions are explicit, named functions rather than constructors: they are
non-`constexpr`, nonlinear, and have domain errors a constructor cannot report.
A level converts at its own reference precision, so `dBm` yields milliwatts:

```cpp
to_linear(30_dBm);                       // 1000.0 mW, as power<double, std::milli>
quantity_cast<watts>(to_linear(30_dBm)); // 1 W
to_level<dbm>(milliwatts(1));            // 0 dBm
to_level<dbm>(watts(1));                 // 30 dBm
```

A zero quantity has no finite logarithm and maps to `dbm::min()`; a negative one
asserts. Values are rounded to nearest, not truncated.

### Power and field references

A reference records whether its linear quantity is a *power* (`10·log10`) or a
*field* such as voltage (`20·log10`), so callers never have to remember which
factor applies:

```cpp
to_level<dbuv>(volts(1));  // 120 dBµV, not 60
```

For the same reason, a bare `dB` value cannot be turned into a linear ratio
unambiguously — 3 dB is a factor of 2 in power but ~1.41 in amplitude — so the
two conversions are separately named:

```cpp
power_ratio(3_dB);      // ~1.995
amplitude_ratio(6_dB);  // ~1.995
```

`add_powers` is available only for power references; for a field reference,
summing linear amplitudes would model coherent addition instead, which is a
different operation with a different answer.

### Precision

Integer decibels at 1 dB resolution are coarse. Use a finer precision or a
floating-point representation:

```cpp
centi_dbm fine{-7350};        // -73.50 dBm
dbm_level<double> exact{-73.5};
to_string(fine);              // "-73.50dBm"
```

## Quantity Types

All standard aliases use `int64_t` representation. Each quantity also has an
alias template for custom representations and precisions, e.g.
`voltage<double>` or `current<float, std::milli>`.

| Quantity | Alias template | Standard aliases |
|----------|----------------|------------------|
| Voltage (V) | `voltage<Rep, Precision>` | `microvolts`, `millivolts`, `volts`, `kilovolts` |
| Current (A) | `current<Rep, Precision>` | `microamperes`/`microamps`, `milliamperes`/`milliamps`, `amperes`/`amps` |
| Resistance (Ω) | `resistance<Rep, Precision>` | `milliohms`, `ohms`, `kiloohms`, `megaohms`, `gigaohms` |
| Power (W) | `power<Rep, Precision>` | `microwatts`, `milliwatts`, `watts`, `kilowatts`, `megawatts`, `gigawatts` |
| Charge (C) | `charge<Rep, Precision>` | `microcoulombs`, `millicoulombs`, `coulombs`, `milliampere_hours`, `ampere_hours` |
| Energy (J) | `energy<Rep, Precision>` | `millijoules`, `joules`, `kilojoules`, `megajoules`, `watt_hours`, `kilowatt_hours` |
| Capacitance (F) | `capacitance<Rep, Precision>` | `picofarads`, `nanofarads`, `microfarads`, `millifarads`, `farads` |
| Inductance (H) | `inductance<Rep, Precision>` | `nanohenries`, `microhenries`, `millihenries`, `henries` |

### Decibel Types

From `<electro/decibel>`. Levels are `level<Reference, Rep, Precision>`; gains
are ordinary quantities of `decibel_unit`.

| Kind | Alias template | Standard aliases |
|------|----------------|------------------|
| Gain (dB) | `gain<Rep, Precision>` | `decibels`, `centidecibels`, `millidecibels` |
| Level vs 1 mW | `dbm_level<Rep, Precision>` | `dbm`, `centi_dbm` |
| Level vs 1 W | `dbw_level<Rep, Precision>` | `dbw` |
| Level vs 1 V | `dbv_level<Rep, Precision>` | `dbv` |
| Level vs 1 mV | `dbmv_level<Rep, Precision>` | `dbmv` |
| Level vs 1 µV | `dbuv_level<Rep, Precision>` | `dbuv`, `centi_dbuv` |

## Literals

| Quantity | Literals |
|----------|----------|
| Voltage | `_uV`, `_mV`, `_V`, `_kV` |
| Current | `_uA`, `_mA`, `_A` |
| Resistance | `_mOhm`, `_Ohm`, `_kOhm`, `_MOhm` (also `_mΩ`, `_Ω`, `_kΩ`, `_MΩ`) |
| Power | `_uW`, `_mW`, `_W`, `_kW`, `_MW` |
| Charge | `_C`, `_mAh`, `_Ah` |
| Energy | `_mJ`, `_J`, `_kJ`, `_Wh`, `_kWh` |
| Capacitance | `_pF`, `_nF`, `_uF`, `_F` |
| Inductance | `_nH`, `_uH`, `_mH`, `_H` |
| Gain | `_dB`, `_cdB` (0.01 dB steps) |
| Level | `_dBm`, `_cdBm`, `_dBW`, `_dBV`, `_dBmV`, `_dBuV` (also `_dBµV`) |

Literals are integral, so fractional values use the `centi` variants
(`1301_cdBm` is 13.01 dBm) or an explicit floating-point type. Negative levels
such as `-73_dBm` work because `level` provides a unary `operator-`, which
reflects the level about its reference.

## Conversions

Following `std::chrono` semantics:

- **Implicit** conversions are allowed when lossless (finer target precision,
  or floating-point target representation)
- **Explicit** conversions (constructor or `quantity_cast`) are required when
  the conversion may lose precision
- `floor`, `ceil` and `round` provide explicit rounding control

```cpp
millivolts mv = volts(5);              // implicit: 5000 mV
volts v = volts(millivolts(5500));     // explicit: truncates to 5 V
auto r = round<volts>(millivolts(5500)); // 6 V (round half to even)
auto c = quantity_cast<ampere_hours>(coulombs(7200)); // 2 Ah
```

Levels follow the same rules, via `level_cast` and the same `floor`, `ceil` and
`round` overloads:

```cpp
centi_dbm fine = 20_dBm;                  // implicit: 2000 (20.00 dBm)
dbm coarse = dbm(centi_dbm(1301));        // explicit: truncates to 13 dBm
auto n = round<dbm>(centi_dbm(1350));     // 14 dBm
```

## ESP-IDF

The library can be used as an ESP-IDF component. Add it to your project's
`idf_component.yml`:

```yaml
dependencies:
  cleishm/electro: "^0.2.0"
```

`std::format` support is controlled via `CONFIG_ELECTRO_STD_FORMAT` in
menuconfig (under "Component config" → "Electro Library").

## Building and Testing

```bash
cmake -B build -DELECTRO_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## License

MIT License - see [LICENSE](LICENSE) for details.
