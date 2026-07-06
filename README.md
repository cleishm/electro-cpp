# electro

A type-safe, header-only C++20 library for electrical units, modeled after `std::chrono::duration`.

📚 **[Full API Documentation](https://cleishm.github.io/electro-cpp/)**

## Features

- **Type-safe electrical quantities**: voltage, current, resistance, power, charge, energy, capacitance, inductance
- **Standard SI multiples**: microvolts through kilovolts, milliohms through gigaohms, picofarads through farads, and more
- **Cross-quantity arithmetic** following circuit theory: Ohm's law (`I * R = V`), power (`V * I = P`), and friends
- **`std::chrono` interop**: `mA x hours = mAh`, `W x hours = Wh`, `Ω x F = RC time constant` (as a `std::chrono::duration`)
- **Integer and floating-point representations** for exact or fractional precision
- **Lossless implicit conversions** (lossy conversions require explicit casts)
- **User-defined literals** for concise notation (`3300_mV`, `10_kΩ`, `2000_mAh`)
- **`std::format` support** (when available)
- **Fully constexpr** - all operations can be evaluated at compile time

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
    GIT_TAG v0.1.0
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

## ESP-IDF

The library can be used as an ESP-IDF component. Add it to your project's
`idf_component.yml`:

```yaml
dependencies:
  cleishm/electro: "^0.1.0"
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
