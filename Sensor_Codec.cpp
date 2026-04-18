#pragma once

#include "sensor_station.cpp"

// ============================================================
//  Serializer  (sensor data → binary frame)
// ============================================================
class SensorSerializer {
public:
    // Serialize a StationSnapshot into a binary frame.
    // Returns the number of bytes written, or 0 on failure.
    static size_t serialize(const StationSnapshot& snap,
                            uint8_t* out, size_t out_capacity) {

        // Build TLV payload first
        uint8_t payload[MAX_PAYLOAD];
        size_t  payload_len = 0;

        for (const auto& r: snap.readings) {
            if (!encode_tlv(r, payload, payload_len))
                return 0;
        }

        // Total frame size = header + payload + crc
        const size_t frame_size = sizeof(FrameHeader) + payload_len + 2;
        if (frame_size > out_capacity) return 0;

        // Write header
        size_t pos = 0;
        write_le16(out + pos, MAGIC);            pos += 2;
        out[pos++] = PROTO_VERSION;
        write_le16(out + pos, snap.station_id);  pos += 2;
        write_le32(out + pos, snap.unix_ts);     pos += 4;
        write_le16(out + pos, static_cast<uint16_t>(payload_len)); pos += 2;

        // Write payload
        std::memcpy(out + pos, payload, payload_len);
        pos += payload_len;

        // CRC over header + payload
        uint16_t crc = crc16(out, pos);
        write_le16(out + pos, crc); pos += 2;

        return pos;
    }

private:
    static bool encode_tlv(const SensorReading& r,
                           uint8_t* payload, size_t& len) {
        const uint8_t type   = static_cast<uint8_t>(r.id);
        const uint8_t vlen   = 2; // all wire types are 2 bytes
        if (len + 2 + vlen > MAX_PAYLOAD) return false;

        payload[len++] = type;
        payload[len++] = vlen;

        switch (r.id) {
            case SensorID::TEMPERATURE: {
                auto raw = static_cast<int16_t>(r.temperature_c * 100.0f);
                payload[len++] = raw & 0xFF;
                payload[len++] = (raw >> 8) & 0xFF;
                break;
            }
            case SensorID::CO: {
                auto raw = static_cast<uint16_t>(r.co_ppm * 10.0f);
                write_le16(payload + len, raw); len += 2;
                break;
            }
            case SensorID::HUMIDITY: {
                auto raw = static_cast<uint16_t>(r.humidity_pct * 100.0f);
                write_le16(payload + len, raw); len += 2;
                break;
            }
            case SensorID::PH: {
                auto raw = static_cast<uint16_t>(r.ph * 100.0f);
                write_le16(payload + len, raw); len += 2;
                break;
            }
            default:
                return false;
        }
        return true;
    }
};

// ============================================================
//  Deserializer  (binary frame → StationSnapshot)
// ============================================================
class SensorDeserializer {
public:
    struct Result {
        bool            ok      = false;
        std::string     error;
        StationSnapshot snapshot;
    };

    static Result deserialize(const uint8_t* frame, size_t frame_len) {
        Result res;

        // Minimum: header(11) + crc(2)
        if (frame_len < sizeof(FrameHeader) + 2) {
            res.error = "Frame too short";
            return res;
        }

        // Parse header
        size_t pos = 0;
        const uint16_t magic = read_le16(frame + pos); pos += 2;
        if (magic != MAGIC) { res.error = "Bad magic"; return res; }

        const uint8_t version = frame[pos++];
        if (version != PROTO_VERSION) {
            res.error = "Unsupported version: " + std::to_string(version);
            return res;
        }

        res.snapshot.station_id = read_le16(frame + pos); pos += 2;
        res.snapshot.unix_ts    = read_le32(frame + pos); pos += 4;
        const uint16_t payload_len = read_le16(frame + pos); pos += 2;

        // Validate sizes
        if (pos + payload_len + 2 > frame_len) {
            res.error = "Declared payload length exceeds frame";
            return res;
        }

        // Verify CRC (header + payload, before the CRC bytes themselves)
        const size_t crc_offset = pos + payload_len;
        const uint16_t expected_crc = crc16(frame, crc_offset);
        const uint16_t actual_crc   = read_le16(frame + crc_offset);
        if (expected_crc != actual_crc) {
            res.error = "CRC mismatch";
            return res;
        }

        // Decode TLV payload
        const uint8_t* payload = frame + pos;
        size_t ppos = 0;
        while (ppos < payload_len) {
            if (ppos + 2 > payload_len) { res.error = "Truncated TLV"; return res; }

            const uint8_t type = payload[ppos++];
            const uint8_t vlen = payload[ppos++];

            if (ppos + vlen > payload_len) { res.error = "TLV value overrun"; return res; }

            SensorReading reading;
            if (!decode_tlv(static_cast<SensorID>(type),
                            payload + ppos, vlen, reading)) {
                res.error = "Unknown sensor type: " + std::to_string(type);
                return res;
            }
            res.snapshot.readings.push_back(reading);
            ppos += vlen;
        }

        res.ok = true;
        return res;
    }

private:
    static bool decode_tlv(SensorID id, const uint8_t* val,
                           uint8_t vlen, SensorReading& out) {
        if (vlen < 2) return false;
        out.id = id;

        switch (id) {
            case SensorID::TEMPERATURE: {
                auto raw = static_cast<int16_t>(
                    static_cast<uint16_t>(val[0]) | (static_cast<uint16_t>(val[1]) << 8));
                out.temperature_c = raw / 100.0f;
                return true;
            }
            case SensorID::CO: {
                out.co_ppm = read_le16(val) / 10.0f;
                return true;
            }
            case SensorID::HUMIDITY: {
                out.humidity_pct = read_le16(val) / 100.0f;
                return true;
            }
            case SensorID::PH: {
                out.ph = read_le16(val) / 100.0f;
                return true;
            }
            default:
                return false;
        }
    }
};
