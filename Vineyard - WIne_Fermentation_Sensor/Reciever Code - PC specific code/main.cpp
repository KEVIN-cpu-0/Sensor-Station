/*
  Vineyard Sensor Station — PC Receiver
  =======================================
  Listens on a serial port for binary frames forwarded by the
  receiver Arduino over USB Serial, then deserializes and
  pretty-prints each sensor snapshot.

  Cross-platform: works on Windows, Linux, and macOS.

  Build:
    g++ -std=c++17 -o receiver main.cpp

  Run (Windows):
    ./receiver [port name and number]
  Run (Linux/macOS):
    ./receiver [port name and number] /dev/ttyACM0

  Using the makefile:
    make - compiles the code and creates receiver.exe
    make run PORT=[port name and number] - compiles the code and runs receiver.exe
  
  Find your port:
    Windows  : Device Manager → Ports (COM & LPT)
    Linux    : ls /dev/ttyACM* or ls /dev/ttyUSB*
    macOS    : ls /dev/tty.usbmodem*
*/

#include "sensor_station.cpp"
#include "Sensor_Codec.cpp"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>

// ----------------------------------------------------------------
//  Cross-platform serial includes
// ----------------------------------------------------------------
#ifdef _WIN32
  #include <windows.h>
  using SerialHandle = HANDLE;
  const SerialHandle INVALID_SERIAL = INVALID_HANDLE_VALUE;
#else
  #include <fcntl.h>
  #include <termios.h>
  #include <unistd.h>
  using SerialHandle = int;
  const SerialHandle INVALID_SERIAL = -1;
#endif

// ----------------------------------------------------------------
//  Pretty-print a hex dump
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
//  Pretty-print a decoded snapshot
// ----------------------------------------------------------------
static void print_snapshot(const StationSnapshot& snap) {
    std::time_t t = snap.unix_ts;
    char tbuf[32];
    std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", std::gmtime(&t));

    std::cout << "\n┌─ Station #" << snap.station_id
              << "  @  " << tbuf << " UTC\n";
    for (const auto& r : snap.readings) {
        float v = r.temperature_c;  // all values share the same union offset
        std::cout << "│  " << std::left  << std::setw(12) << r.label()
                  << std::right << std::setw(8) << std::fixed
                  << std::setprecision(2) << v
                  << " " << r.unit() << "\n";
    }
    std::cout << "└──────────────────────────────\n";
}

// ----------------------------------------------------------------
//  Open serial port at 115200 8N1 — cross-platform
// ----------------------------------------------------------------
static SerialHandle open_serial(const std::string& port) {
#ifdef _WIN32
    std::string full_port = "\\\\.\\" + port;
    HANDLE h = CreateFileA(full_port.c_str(), GENERIC_READ, 0, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERROR] Could not open " << port
                  << " (error " << GetLastError() << ")\n";
        return INVALID_HANDLE_VALUE;
    }
    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(h, &dcb);
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(h, &dcb);

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout        = 50;
    timeouts.ReadTotalTimeoutConstant   = 2000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &timeouts);
    return h;

#else
    int fd = open(port.c_str(), O_RDONLY | O_NOCTTY);
    if (fd == -1) {
        std::cerr << "[ERROR] Could not open " << port << "\n";
        return -1;
    }
    termios tty = {};
    tcgetattr(fd, &tty);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;  // 8-bit
    tty.c_cflag &= ~PARENB;                        // no parity
    tty.c_cflag &= ~CSTOPB;                        // 1 stop bit
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 20;  // 2 second timeout
    tcsetattr(fd, TCSANOW, &tty);
    return fd;
#endif
}

// ----------------------------------------------------------------
//  Close serial port
// ----------------------------------------------------------------
static void close_serial(SerialHandle h) {
#ifdef _WIN32
    CloseHandle(h);
#else
    close(h);
#endif
}

// ----------------------------------------------------------------
//  Read exactly n bytes — cross-platform
// ----------------------------------------------------------------
static bool read_bytes(SerialHandle h, uint8_t* buf, size_t n) {
    size_t total = 0;
    while (total < n) {
#ifdef _WIN32
        DWORD got = 0;
        if (!ReadFile(h, buf + total, (DWORD)(n - total), &got, NULL))
            return false;
#else
        ssize_t got = read(h, buf + total, n - total);
        if (got <= 0) return false;
#endif
        if (got == 0) return false;
        total += got;
    }
    return true;
}

// ----------------------------------------------------------------
//  Scan incoming bytes until magic bytes 0x4D 0xFE are found
// ----------------------------------------------------------------
static bool sync_to_magic(SerialHandle h) {
    uint8_t prev = 0, curr = 0;
    while (true) {
#ifdef _WIN32
        DWORD got = 0;
        if (!ReadFile(h, &curr, 1, &got, NULL) || got == 0) return false;
#else
        ssize_t got = read(h, &curr, 1);
        if (got <= 0) return false;
#endif
        if (prev == 0x4D && curr == 0xFE) return true;
        prev = curr;
    }
}

// ----------------------------------------------------------------
//  main
// ----------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: receiver <port>\n";
        std::cerr << "  Windows  : receiver COM3\n";
        std::cerr << "  Linux    : receiver /dev/ttyACM0\n";
        std::cerr << "  macOS    : receiver /dev/tty.usbmodem1\n";
        return 1;
    }

    const std::string port = argv[1];
    std::cout << "Opening " << port << " at 115200 baud...\n";

    SerialHandle h = open_serial(port);
    if (h == INVALID_SERIAL) return 1;

    std::cout << "[OK] Port open. Waiting for packets...\n";
    std::cout << "     Press Ctrl+C to exit.\n\n";

    uint32_t packet_count = 0;

    while (true) {
        // Step 1 — scan for magic bytes to find frame start
        if (!sync_to_magic(h)) {
            std::cerr << "[WARN] Lost sync — retrying...\n";
            continue;
        }

        // Step 2 — read remaining 9 header bytes
        // version(1) + station_id(2) + unix_ts(4) + payload_len(2)
        uint8_t header_rest[9];
        if (!read_bytes(h, header_rest, sizeof(header_rest))) {
            std::cerr << "[WARN] Header read timeout\n";
            continue;
        }

        uint8_t  version     = header_rest[0];
        uint16_t payload_len = header_rest[7] | (header_rest[8] << 8);

        if (version != 1) {
            std::cerr << "[WARN] Unknown protocol version: "
                      << (int)version << "\n";
            continue;
        }
        if (payload_len > 512) {
            std::cerr << "[WARN] Payload too large: "
                      << payload_len << " — resyncing\n";
            continue;
        }

        // Step 3 — read payload then CRC
        std::vector<uint8_t> payload(payload_len);
        if (!read_bytes(h, payload.data(), payload_len)) {
            std::cerr << "[WARN] Payload read timeout\n";
            continue;
        }

        uint8_t crc_bytes[2];
        if (!read_bytes(h, crc_bytes, 2)) {
            std::cerr << "[WARN] CRC read timeout\n";
            continue;
        }

        // Step 4 — reassemble full frame for deserializer
        std::vector<uint8_t> frame;
        frame.push_back(0x4D);  // magic low byte (consumed during sync)
        frame.push_back(0xFE);  // magic high byte
        for (auto b : header_rest) frame.push_back(b);
        for (auto b : payload)     frame.push_back(b);
        frame.push_back(crc_bytes[0]);
        frame.push_back(crc_bytes[1]);

        packet_count++;
        std::cout << "--- Packet #" << packet_count
                  << " (" << frame.size() << " bytes) ---\n";
        hex_dump(frame.data(), frame.size());

        // Step 5 — deserialize and print
        auto result = SensorDeserializer::deserialize(
                          frame.data(), frame.size());
        if (result.ok) {
            print_snapshot(result.snapshot);
        } else {
            std::cerr << "[FAIL] " << result.error << "\n";
        }
    }

    close_serial(h);
    return 0;
}