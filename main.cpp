#include "sensor_station.cpp"
#include "Sensor_Codec.cpp"
#include <iostream>
#include <iomanip>
#include <ctime>

// ----------------------------------------------------------------
//  Pretty-print a hex dump of the wire frame
// ----------------------------------------------------------------
static void hex_dump(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(buf[i]);
        std::cout << ((i + 1) % 16 == 0 ? "\n" : " ");
    }
    std::cout << std::dec << "\n";
}

// ----------------------------------------------------------------
//  Pretty-print a deserialized snapshot
// ----------------------------------------------------------------
static void print_snapshot(const StationSnapshot& snap) {
    std::time_t t = snap.unix_ts;
    char tbuf[32];
    std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", std::gmtime(&t));

    std::cout << "┌─ Station #" << snap.station_id
              << "  @  " << tbuf << " UTC\n";
    for (const auto& r : snap.readings) {
        float v = r.temperature_c; // all share the same union offset
        std::cout << "│  " << std::left << std::setw(12) << r.label()
                  << std::right << std::setw(8) << std::fixed
                  << std::setprecision(2) << v
                  << " " << r.unit() << "\n";
    }
    std::cout << "└──────────────────────────────\n";
}

// ----------------------------------------------------------------
//  main: build → serialize → dump → deserialize → print
// ----------------------------------------------------------------
int main() {
    // 1. Compose a snapshot (simulated sensor readings)
    StationSnapshot snap;
    snap.station_id = 7;
    snap.unix_ts    = static_cast<uint32_t>(std::time(nullptr));

    SensorReading temp;   temp.id = SensorID::TEMPERATURE; temp.temperature_c = 22.75f;
    SensorReading co;     co.id   = SensorID::CO;          co.co_ppm          = 14.3f;
    SensorReading hum;    hum.id  = SensorID::HUMIDITY;    hum.humidity_pct   = 68.42f;
    SensorReading ph;     ph.id   = SensorID::PH;          ph.ph              = 4.60f;

    snap.readings = { temp, co, hum, ph };

    // 2. Serialize to wire buffer
    uint8_t frame[256];
    size_t  frame_len = SensorSerializer::serialize(snap, frame, sizeof(frame));
    if (frame_len == 0) {
        std::cerr << "Serialization failed!\n";
        return 1;
    }

    std::cout << "=== Serialized frame (" << frame_len << " bytes) ===\n";
    hex_dump(frame, frame_len);

    // 3. Deserialize from wire buffer
    auto result = SensorDeserializer::deserialize(frame, frame_len);
    if (!result.ok) {
        std::cerr << "Deserialization failed: " << result.error << "\n";
        return 1;
    }

    std::cout << "\n=== Decoded snapshot ===\n";
    print_snapshot(result.snapshot);

    // 4. Demonstrate CRC error detection
    std::cout << "\n=== CRC corruption test ===\n";
    uint8_t corrupt[256];
    std::memcpy(corrupt, frame, frame_len);
    corrupt[10] ^= 0xFF;  // flip bits in payload
    auto bad = SensorDeserializer::deserialize(corrupt, frame_len);
    std::cout << (bad.ok ? "ERROR: accepted corrupt frame"
                         : "Correctly rejected: " + bad.error) << "\n";

    return 0;
}
