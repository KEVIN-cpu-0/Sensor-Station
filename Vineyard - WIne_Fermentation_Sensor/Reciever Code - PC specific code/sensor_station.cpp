//For serialization
//receives stuff from the sensor station and turns it into a binary frame that can be sent over the network. It also includes the CRC calculation for data integrity.

#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <sstream>
#include <iomanip>

// ============================================================
//  Protocol constants
// ============================================================
static constexpr uint16_t MAGIC          = 0xFE4D;   // 'FerM'
static constexpr uint8_t  PROTO_VERSION  = 1;
static constexpr size_t   MAX_PAYLOAD    = 512;

// ============================================================
//  Sensor reading IDs
// ============================================================
enum class SensorID : uint8_t {
    TEMPERATURE = 0x01,
    CO          = 0x02,
    HUMIDITY    = 0x03,
    PH          = 0x04,
};
 
// ============================================================
//  Packed wire types  (all little-endian)
// ============================================================
#pragma pack(push, 1)
 
struct RawTemperature {
    int16_t  celsius_x100;   // e.g. 2315 → 23.15 °C
};
 
struct RawCO {
    uint16_t ppm_x10;        // e.g.  125 → 12.5 ppm
};
 
struct RawHumidity {
    uint16_t percent_x100;   // e.g. 6540 → 65.40 %RH
};
 
struct RawPH {
    uint16_t ph_x100;        // e.g.  452 →  4.52
};
 
// TLV record: [type:1][length:1][value:length]
struct TLVHeader {
    uint8_t type;
    uint8_t length;
};
 
// Full frame layout
//  [magic:2][version:1][station_id:2][timestamp:4][payload_len:2][payload:N][crc16:2]
struct FrameHeader {
    uint16_t magic;
    uint8_t  version;
    uint16_t station_id;
    uint32_t unix_ts;
    uint16_t payload_len;
};
 
#pragma pack(pop)
 
// ============================================================
//  Typed sensor reading (host-side)
// ============================================================
struct SensorReading {
    SensorID id;
    union {
        float temperature_c;   // Celsius
        float co_ppm;          // parts per million
        float humidity_pct;    // 0–100 %RH
        float ph;              // 0–14
    };
 
    std::string label() const {
        switch (id) {
            case SensorID::TEMPERATURE: return "Temperature";
            case SensorID::CO:          return "CO";
            case SensorID::HUMIDITY:    return "Humidity";
            case SensorID::PH:         return "pH";
        }
        return "Unknown";
    }
 
    std::string unit() const {
        switch (id) {
            case SensorID::TEMPERATURE: return "°C";
            case SensorID::CO:          return "ppm";
            case SensorID::HUMIDITY:    return "%RH";
            case SensorID::PH:         return "";
        }
        return "";
    }
};
 
// ============================================================
//  Station snapshot (one full frame of readings)
// ============================================================
struct StationSnapshot {
    uint16_t station_id;
    uint32_t unix_ts;
    std::vector<SensorReading> readings;
 
    std::optional<float> temperature() const { return get(SensorID::TEMPERATURE); }
    std::optional<float> co()          const { return get(SensorID::CO); }
    std::optional<float> humidity()    const { return get(SensorID::HUMIDITY); }
    std::optional<float> ph()          const { return get(SensorID::PH); }
 
private:
    std::optional<float> get(SensorID id) const {
        for (auto& r : readings)
            if (r.id == id) return r.temperature_c; // all values at same offset
        return std::nullopt;
    }
};
 
// ============================================================
//  CRC-16/CCITT-FALSE
// ============================================================
inline uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int b = 0; b < 8; ++b)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}
 
// ============================================================
//  Little-endian write/read helpers
// ============================================================
inline void write_le16(uint8_t* buf, uint16_t v) {
    buf[0] = v & 0xFF; buf[1] = v >> 8;
}
inline void write_le32(uint8_t* buf, uint32_t v) {
    buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
    buf[2] = (v >> 16) & 0xFF; buf[3] = v >> 24;
}
inline uint16_t read_le16(const uint8_t* buf) {
    return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
}
inline uint32_t read_le32(const uint8_t* buf) {
    return static_cast<uint32_t>(buf[0])
         | (static_cast<uint32_t>(buf[1]) << 8)
         | (static_cast<uint32_t>(buf[2]) << 16)
         | (static_cast<uint32_t>(buf[3]) << 24);
}