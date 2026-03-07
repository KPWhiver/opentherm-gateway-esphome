#pragma once

#include <bitset>
#include <cstdint>
#include <utility>

enum class DataTypeKind {
  COMMAND,
  MASTER_CONFIG,
  SLAVE_CONFIG,
  SLAVE_STATUS,
  MASTER_STATUS,
  EITHER_STATUS,
};
template<uint8_t data_type, DataTypeKind kind, typename Type>
struct DataType {
  constexpr static uint8_t ID = data_type;

  static std::string as_str() {
    return std::to_string(static_cast<uint16_t>(ID));
  }
};

using Status =                         DataType<0,   DataTypeKind::SLAVE_STATUS,  std::bitset<16>>;
using ControlSetpoint =                DataType<1,   DataTypeKind::COMMAND,       float>;
using MasterConfiguration =            DataType<2,   DataTypeKind::MASTER_CONFIG, float>;
using SlaveConfiguration =             DataType<3,   DataTypeKind::SLAVE_CONFIG,  std::bitset<8>>;
using RemoteRequest =                  DataType<4,   DataTypeKind::COMMAND,       uint8_t>;
using FaultFlags =                     DataType<5,   DataTypeKind::SLAVE_STATUS,  std::bitset<8>>;
using RemoteParameters =               DataType<6,   DataTypeKind::SLAVE_CONFIG,  std::bitset<16>>;
using ControlSetpoint2 =               DataType<8,   DataTypeKind::COMMAND,       float>;
using RemoteOverrideRoomSetpoint =     DataType<9,   DataTypeKind::SLAVE_STATUS,  float>;
using MaxRelativeModulationLevel =     DataType<14,  DataTypeKind::COMMAND,       float>;
using MaxBoilerCapMinModulationLevel = DataType<15,  DataTypeKind::SLAVE_CONFIG,  std::pair<uint8_t, uint8_t>>;
using RoomSetpoint =                   DataType<16,  DataTypeKind::MASTER_STATUS, float>;
using RelativeModulationLevel =        DataType<17,  DataTypeKind::SLAVE_STATUS,  float>;
using CHWaterPressure =                DataType<18,  DataTypeKind::SLAVE_STATUS,  float>;
using DHWFlowRate =                    DataType<19,  DataTypeKind::SLAVE_STATUS,  float>;
using RoomSetpoint2 =                  DataType<23,  DataTypeKind::MASTER_STATUS, float>;
using RoomTemperature =                DataType<24,  DataTypeKind::MASTER_STATUS, float>;
using BoilerFlowWaterTemperature =     DataType<25,  DataTypeKind::SLAVE_STATUS,  float>;
using DHWTemperature =                 DataType<26,  DataTypeKind::SLAVE_STATUS,  float>;
using OutsideTemperature =             DataType<27,  DataTypeKind::EITHER_STATUS, float>;
using ReturnWaterTemperature =         DataType<28,  DataTypeKind::SLAVE_STATUS,  float>;
using SolarStorageTemperature =        DataType<29,  DataTypeKind::SLAVE_STATUS,  float>;
using SolarCollectorTemperature =      DataType<30,  DataTypeKind::SLAVE_STATUS,  float>;
using FlowTemperatureCH2 =             DataType<31,  DataTypeKind::SLAVE_STATUS,  float>;
using DHWTemperature2 =                DataType<32,  DataTypeKind::SLAVE_STATUS,  float>;
using ExhaustTemperature =             DataType<33,  DataTypeKind::SLAVE_STATUS,  int16_t>;
using BoilerHeatExchangerTemperature = DataType<34,  DataTypeKind::SLAVE_STATUS,  float>;
using FlameCurrent =                   DataType<36,  DataTypeKind::SLAVE_STATUS,  float>;
using RoomTemperatureCH2 =             DataType<38,  DataTypeKind::MASTER_STATUS, float>;
using RemoteOverrideRoomSetpoint2 =    DataType<39,  DataTypeKind::SLAVE_STATUS,  float>;
using DHWSetpointBounds =              DataType<48,  DataTypeKind::SLAVE_CONFIG,  std::pair<int8_t, int8_t>>;
using CHSetpointBounds =               DataType<49,  DataTypeKind::SLAVE_CONFIG,  std::pair<int8_t, int8_t>>;
using DHWSetpoint =                    DataType<56,  DataTypeKind::SLAVE_CONFIG,  float>;
using MaxCHWaterSetpoint =             DataType<57,  DataTypeKind::SLAVE_CONFIG,  float>;
using RelativeHumidity =               DataType<78,  DataTypeKind::EITHER_STATUS, uint8_t>;
using CO2Level =                       DataType<79,  DataTypeKind::EITHER_STATUS, uint16_t>;
using PowerCycles =                    DataType<97,  DataTypeKind::SLAVE_STATUS,  uint16_t>;
using UnsuccesfulBurnerStarts =        DataType<113, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using TimesFlameSignalLow =            DataType<114, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using SuccessfulBurnerStarts =         DataType<116, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using CHPumpStarts =                   DataType<117, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using DHWPumpStarts =                  DataType<118, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using DHWBurnerStarts =                DataType<119, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using BurnerOperationHours =           DataType<120, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using CHPumpOperationHours =           DataType<121, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using DHWPumpOperationHours =          DataType<122, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using DHWBurnerOperationHours =        DataType<123, DataTypeKind::SLAVE_STATUS,  uint16_t>;
using MasterOpenThermVersion =         DataType<124, DataTypeKind::MASTER_CONFIG, float>;
using SlaveOpenThermVersion =          DataType<125, DataTypeKind::SLAVE_CONFIG,  float>;
using MasterProductVersion =           DataType<126, DataTypeKind::MASTER_STATUS, std::pair<uint8_t, uint8_t>>;
using SlaveProductVersion =            DataType<127, DataTypeKind::SLAVE_STATUS, std::pair<uint8_t, uint8_t>>;
