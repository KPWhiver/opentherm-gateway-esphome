#pragma once

#include <bitset>
#include <cstdint>
#include <utility>

namespace esphome {
namespace otgw {
namespace data_types {

enum class Interval : uint16_t {
  FAST = 1,
  MEDIUM = 10,
  SLOW = 100,
  ONCE = 1000,
};

template<uint8_t id, typename Type>
struct WriteOnly {
  constexpr static uint8_t ID = id;
  constexpr static uint16_t INTERVAL = 0;

  static std::string as_str() {
    return std::to_string(static_cast<uint16_t>(ID));
  }
};

template<uint8_t id, Interval interval, typename Type>
struct Readable : public WriteOnly<id, Type> {
  constexpr static uint8_t ID = id;
  constexpr static uint16_t INTERVAL = static_cast<uint16_t>(interval);

  static std::string as_str() {
    return std::to_string(static_cast<uint16_t>(ID));
  }
};

using Status =                         Readable <0,    Interval::FAST,     std::bitset<16>>;
using ControlSetpoint =                WriteOnly<1,                        float>;
using MasterConfiguration =            WriteOnly<2,                        float>;
using SlaveConfiguration =             Readable <3,    Interval::ONCE,     std::bitset<8>>;
using RemoteRequest =                  WriteOnly<4,                        std::pair<uint8_t, uint8_t>>;
using FaultFlags =                     Readable <5,    Interval::SLOW,     std::bitset<8>>;
using RemoteParameters =               Readable <6,    Interval::ONCE,     std::bitset<16>>;
using CoolingControl =                 WriteOnly<7,                        float>;
using ControlSetpoint2 =               WriteOnly<8,                        float>;
using RemoteOverrideRoomSetpoint =     Readable <9,    Interval::FAST,     float>;
using NumberOfSlaveParameters =        Readable <10,   Interval::ONCE,     std::pair<uint8_t, uint8_t>>;
// TSP-index / TSP-value                         11
using FaultHistoryBufferSize =         Readable <12,   Interval::ONCE,     std::pair<uint8_t, uint8_t>>;
// FHB-index / FHB-value                         13
using MaxRelativeModulationLevel =     WriteOnly<14,                       float>;
using MaxBoilerCapMinModulationLevel = Readable <15,   Interval::ONCE,     std::pair<uint8_t, uint8_t>>;
using RoomSetpoint =                   WriteOnly<16,                       float>;
using RelativeModulationLevel =        Readable <17,   Interval::FAST,     float>;
using CHWaterPressure =                Readable <18,   Interval::MEDIUM,   float>;
using DHWFlowRate =                    Readable <19,   Interval::FAST,     float>;
// Day-Time                                      20
// Date                                          21
// Year                                          22
using RoomSetpoint2 =                  WriteOnly<23,                       float>;
using RoomTemperature =                WriteOnly<24,                       float>;
using BoilerFlowWaterTemperature =     Readable <25,   Interval::FAST,     float>;
using DHWTemperature =                 Readable <26,   Interval::FAST,     float>;
using OutsideTemperature =             Readable <27,   Interval::MEDIUM,   float>;
using ReturnWaterTemperature =         Readable <28,   Interval::FAST,     float>;
using SolarStorageTemperature =        Readable <29,   Interval::FAST,     float>;
using SolarCollectorTemperature =      Readable <30,   Interval::FAST,     float>;
using FlowTemperatureCH2 =             Readable <31,   Interval::FAST,     float>;
using DHWTemperature2 =                Readable <32,   Interval::FAST,     float>;
using ExhaustTemperature =             Readable <33,   Interval::FAST,     int16_t>;
using BoilerHeatExchangerTemperature = Readable <34,   Interval::FAST,     float>;
using BoilerFanSpeedSetpointActual =   Readable <35,   Interval::FAST,     std::pair<uint8_t, uint8_t>>;
using FlameCurrent =                   Readable <36,   Interval::FAST,     float>;
using RoomTemperatureCH2 =             WriteOnly<37,                       float>;
using RelativeHumidity =               Readable <38,   Interval::FAST,     uint8_t>;
using RemoteOverrideRoomSetpoint2 =    Readable <39,   Interval::FAST,     float>;
using DHWSetpointBounds =              Readable <48,   Interval::ONCE,     std::pair<int8_t, int8_t>>;
using CHSetpointBounds =               Readable <49,   Interval::ONCE,     std::pair<int8_t, int8_t>>;
using DHWSetpoint =                    Readable <56,   Interval::MEDIUM,   float>;
using MaxCHWaterSetpoint =             Readable <57,   Interval::SLOW,     float>;
// Status ventilation / heat recovery            70
// Vset                                          71
// Ventilation ASF-Flags / OEM-fault-code        72
// Ventilation OEM diagnostic code               73
// Ventilation slave config                      74
// Ventilation opentherm version                 75
// Ventilation product version                   76
// Rel-vent-level                                77
// RH-exhaust                                    78
// CO2-exhaust                                   79
// Supply inlet temperature                      80
// Supply outlet temperature                     81
// Exhaust inlet temperature                     82
// Exhaust outlet temperature                    83
// RPM exhaust                                   84
// RPM supply                                    85
// Ventilation remote parameters                 86
// Nominal ventilation value                     87
// Ventilation TSP                               88
// Ventilation TSP-index / TSP-value             89
// Ventilation BHF-size                          90
// Ventilation FHB-index / FHB-value             91
// Brand                                         93
// Brand version                                 94
// Brand serial number                           95
using CoolingOperationHours =          Readable <96,   Interval::MEDIUM,   uint16_t>;
using PowerCycles =                    Readable <97,   Interval::MEDIUM,   uint16_t>;
// RF sensor status information                  98
// Remote Override Operating Mode Heating/DHW    99
// Remote Override Function                      100
// Status Solar Storage                          101
// Solar storage ASF-Flags / OEM-fault-code      102
// Solar storage slave config                    103
// Solar storage version                         104
// Solar storage TSP                             105
// Solar storage TSP-index / TSP-value           106
// Solar storage FHB-size                        107
// Solar storage FHB-index / FHB-value           108
// Electricity producer starts                   109
// Electricity producer hours                    110
// Electricity production                        111
// Cumulative electricity production             112
using UnsuccesfulBurnerStarts =        Readable <113,  Interval::MEDIUM,   uint16_t>;
using TimesFlameSignalLow =            Readable <114,  Interval::MEDIUM,   uint16_t>;
using OEMDiagnosticCode =              Readable <115,  Interval::MEDIUM,   uint16_t>;
using SuccessfulBurnerStarts =         Readable <116,  Interval::MEDIUM,   uint16_t>;
using CHPumpStarts =                   Readable <117,  Interval::MEDIUM,   uint16_t>;
using DHWPumpStarts =                  Readable <118,  Interval::MEDIUM,   uint16_t>;
using DHWBurnerStarts =                Readable <119,  Interval::MEDIUM,   uint16_t>;
using BurnerOperationHours =           Readable <120,  Interval::MEDIUM,   uint16_t>;
using CHPumpOperationHours =           Readable <121,  Interval::MEDIUM,   uint16_t>;
using DHWPumpOperationHours =          Readable <122,  Interval::MEDIUM,   uint16_t>;
using DHWBurnerOperationHours =        Readable <123,  Interval::MEDIUM,   uint16_t>;
using MasterOpenThermVersion =         WriteOnly<124,                      float>;
using SlaveOpenThermVersion =          Readable <125,  Interval::ONCE,     float>;
using MasterProductVersion =           WriteOnly<126,                      std::pair<uint8_t, uint8_t>>;
using SlaveProductVersion =            Readable <127,  Interval::ONCE,     std::pair<uint8_t, uint8_t>>;

}
}
}
