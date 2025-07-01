#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <memory>

#include "td2000.h"

// Brother raster protocol constants
constexpr uint8_t ESC = 0x1B;

int fd;

bool write_sequence(const std::vector<uint8_t>& buffer) {
    for (const auto& c : buffer)
    {
        if (write(fd, &c, 1) != 1)
            return false;
    }
    return true;
}

void write_invalidate() {
    const std::vector<uint8_t> buffer = {
        0x00
    };
    write_sequence(buffer);
}

void write_init_sequence() {
    const std::vector<uint8_t> init_sequence = {
       ESC, 'i', 'a', 0x00,
       ESC, 'i', 'K', 0x08
    };
    write_sequence(init_sequence);
}

td2000::PrinterInfo get_status_request() {
    const std::vector<uint8_t> status_request = {
        0x1B,0x69,0x53
    };
    td2000::PrinterInfo printer_info{};
    if (write_sequence(status_request))
    {
        read(fd, printer_info.raw, sizeof(printer_info));
        std::cout << "Printer info: " << printer_info.raw[0] << '\n';
    }


    return printer_info;
}

void write_switch_dynamic_mode(td2000::CommandMode mode) {
    const std::vector<uint8_t> mode_request = {
        0x1B,0x69,0x61,
        static_cast<uint8_t>(mode)
    };
    write_sequence(mode_request);
}

void write_additional_media(const std::vector<uint8_t>& media_info) {
    if (media_info.size() != 127)
    {
        std::cerr << "Media info must be 127 bytes long!\n";
        return;
    }
    const std::vector<uint8_t> media_info_command = {
       0x1B,0x69,0x55, 0x77,0x01
    };
    write_sequence(media_info_command);
    write_sequence(media_info);
}

void write_print_info(const td2000::PrintInfo &info) {
    const std::vector<uint8_t> label_info = {
        0x1B, 0x69, 0x7A,
    };
    write_sequence(label_info);
    write_sequence(std::vector<uint8_t>(info.raw.begin(), info.raw.end()));
}

void write_various_mode_settings() {
    const std::vector<uint8_t> label_info = {
        0x1B, 0x69, 0x4D,
    };
}

void write_specify_margin_amount() {
    const std::vector<uint8_t> label_info = {
        0x1B, 0x69, 0x64,
    };
    write_sequence(label_info);
}

void write_set_compression_mode() {
    const std::vector<uint8_t> label_info = {
        0x4D,
    };
    write_sequence(label_info);
}

void write_graphics_transfer( const std::vector<uint8_t> &line) {
    const std::vector<uint8_t> buffer = {
        0x67,0x00,56
    };
    write_sequence(buffer);
    write_sequence(line);
}

void write_image(const std::vector<uint8_t> &data)
{
    const auto lines = data.size() / 56;
    for (int line=0; line<lines; line++)
    {
        write_graphics_transfer(std::vector<uint8_t>(data.begin() + line*56, data.begin() + (line+1)*56));
    }
}

void write_zero_raster_graphics( const std::vector<uint8_t> &line) {
    std::vector<uint8_t> buffer = {
        0x5A
    };
    write_sequence(buffer);
}

void write_print_command( const std::vector<uint8_t> &line) {
    const std::vector<uint8_t> buffer = {
        0x0C
    };
    write_sequence(buffer);
}

void write_print_command_with_feeding() {
    const std::vector<uint8_t> buffer = {
        0x1A
    };
    write_sequence(buffer);
}

std::vector<uint8_t> pack_monochrome_row(const std::vector<uint8_t> &row_pixels) {
    const size_t out_bytes = (row_pixels.size() + 7) / 8;
    std::vector<uint8_t> packed(out_bytes, 0);
    for (size_t i = 0; i < row_pixels.size(); ++i) {
        if (row_pixels[i])
            packed[i / 8] |= (0x80 >> (i % 8));
    }
    return packed;
}



int main(const int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./pdf2td2020 <output_device>\n";
        return 1;
    }
    const std::string device_path = argv[1];

    fd = open(device_path.c_str(), O_RDWR | O_SYNC);
    if (fd < 0) {
        std::cerr << "Cannot open output: " << device_path << ", " << strerror(errno) << '\n';
        return 2;
    }

    int width = 640;
    int height = 400;

    // Instead of PDF, generate a test pattern: checkerboard

    std::vector<uint8_t> pixels(200);
    for (int i = 0; i < pixels.size(); ++i)
    {
        pixels[i] = (i % 2) ? 0xFF : 0x00;
    }

    write_invalidate();
    write_init_sequence();
    get_status_request();
    write_switch_dynamic_mode(td2000::CommandMode::Raster);

    write_image(pixels);
    write_print_command_with_feeding();

    close(fd);
    std::cout << "Sent label to printer (test pattern)!\n";
    return 0;
}