#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <thread>
#include <bitset>

#include "td2000.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <cmath>
#include <sstream>
#include <string>

int fd;


bool write_sequence(const std::vector<uint8_t>& buffer)
{
    if (write(fd, buffer.data(), buffer.size()) != buffer.size())
    {
        return false;
    }
    return true;
}

void write_invalidate()
{
    const std::vector<uint8_t> data(200, 0);
    write_sequence(data);
}

void write_init_sequence()
{
    const std::vector<uint8_t> data(td2000::commands::initialize.begin(),td2000::commands::initialize.end());
    write_sequence(data);
}

td2000::PrinterInfo get_status_request()
{
    const std::vector<uint8_t> data{
        td2000::commands::status_information_request.begin(),td2000::commands::status_information_request.end()
    };

    td2000::PrinterInfo printer_info{};
    if (write_sequence(data))
    {
        if (const auto read_bytes = read(fd, printer_info.raw, sizeof(printer_info.raw)); read_bytes != sizeof(printer_info.raw))
        {
            std::cerr << "Failed to read printer status!\n";
            std::cerr << errno << '\n';
            std::cerr << strerror(errno);
        }
        std::cout << "Printer info: " << printer_info.raw[0] << '\n';
    }

    return printer_info;
}

void write_switch_dynamic_mode(td2000::CommandMode mode)
{
    std::vector<uint8_t> data = {
        td2000::commands::switch_dynamic_command_mode.begin(), td2000::commands::switch_dynamic_command_mode.end(),
    };
    data.push_back(static_cast<uint8_t>(mode));
    write_sequence(data);
}

void write_additional_media(const std::vector<uint8_t>& media_info)
{
    if (media_info.size() != 127)
    {
        std::cerr << "Media info must be 127 bytes long!\n";
        return;
    }
    std::vector<uint8_t> data = {
        td2000::commands::additional_media_information.begin(), td2000::commands::additional_media_information.end()
    };

    data.insert(data.end(), media_info.begin(), media_info.end());
    write_sequence(data);
}

void write_print_info(const td2000::PrintInfo& info)
{
    std::vector<uint8_t> data = {
        td2000::commands::print_information.begin(), td2000::commands::print_information.end()
    };

    data.insert(data.end(), info.raw.begin(), info.raw.end());
    data.insert(data.end(),info.raw.begin(),info.raw.end());
    write_sequence(data);
}

void write_various_mode_settings(const td2000::VariousModeSettings& settings)
{
    std::vector<uint8_t> data = {
        td2000::commands::various_mode_settings.begin(), td2000::commands::various_mode_settings.end()
    };

    data.push_back(settings.raw);
    write_sequence(data);
}

void write_specify_margin_amount(const uint16_t margin)
{
    std::vector<uint8_t> data = {
        td2000::commands::specify_margin_amount.begin(), td2000::commands::specify_margin_amount.end()
    };
    data.push_back(margin);
    data.push_back(margin >> 8);

    write_sequence(data);
}

void write_set_compression_mode(const td2000::CompressionMode mode)
{
    std::vector<uint8_t> data = {
        td2000::commands::select_compression_mode.begin(), td2000::commands::select_compression_mode.end()
    };
    data.push_back(static_cast<uint8_t>(mode));
    write_sequence(data);
}

void write_graphics_transfer(const std::vector<uint8_t>& line)
{
    const std::vector<uint8_t> buffer = {
        0x67, 0x00, 56
    };
   // write_sequence(buffer);
   // write_sequence(line);

    std::vector<uint8_t> data = {
        td2000::commands::raster_graphics_transfer.begin(), td2000::commands::raster_graphics_transfer.end()
    };

    data.push_back(0x00);
    data.push_back(56);
    data.insert(data.end(), line.begin(), line.end());

    write_sequence(data);
}

void write_image(const std::vector<uint8_t>& data)
{
    constexpr size_t chunk_size = 56;
    for (int offset = 0; offset < data.size(); offset += chunk_size)
    {
        const unsigned int current_chunk_size = std::min(chunk_size, data.size() - offset);
        const std::vector<uint8_t> chunk(data.begin() + offset, data.begin() + offset + current_chunk_size);
        write_graphics_transfer(chunk);
        break;
    }
}

void write_zero_raster_graphics()
{
    const std::vector<uint8_t> data = {
        td2000::commands::zero_raster_graphics.begin(), td2000::commands::zero_raster_graphics.end()
    };
    write_sequence(data);
}

void write_print_command()
{
    const std::vector<uint8_t> data = {
        td2000::commands::print.begin(), td2000::commands::print.end()
    };
    write_sequence(data);
}

void write_print_command_with_feeding()
{
    const std::vector<uint8_t> data = {
        td2000::commands::print_with_feeding.begin(), td2000::commands::print_with_feeding.end()
    };
    write_sequence(data);
}


std::vector<uint8_t> load_pbm_with_padding(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Cannot open PBM file");
    }
    constexpr auto pbm_magic = "P4";
    std::string magic;
    file >> magic;
    if (magic != pbm_magic)
    {
        throw std::runtime_error("Invalid PBM format");
    }

    int width, height;
    file >> width >> height;
    file.get();

    const int line_bytes = width/8 ;

    std::vector<uint8_t> pixels;
    std::vector<uint8_t> row_data(line_bytes);
    
    for (int y = 0; y < height; ++y)
    {
        file.read(reinterpret_cast<char*>(row_data.data()), row_data.size());

        for (const auto& byte : std::vector<uint8_t>(row_data.rbegin(), row_data.rend()))
        {
            auto byte_reversed = static_cast<uint8_t>(
                ((byte & 0x01) << 7) | ((byte & 0x02) << 5) |
                ((byte & 0x04) << 3) | ((byte & 0x08) << 1) |
                ((byte & 0x10) >> 1) | ((byte & 0x20) >> 3) |
                ((byte & 0x40) >> 5) | ((byte & 0x80) >> 7)
            );
            pixels.push_back(byte_reversed);
        }
        //pixels.insert(pixels.end(), row_data.begin(), row_data.end());
        pixels.push_back(0);
    }
    auto padding_bits_count = 56 - (pixels.size() % 56);
    for (auto i=0; i<padding_bits_count; i++)
    {
        pixels.push_back(0);
    }
    
    return pixels;
}

std::vector<uint8_t> generate_chessboard_pattern(const int width, const int height)
{
    constexpr auto lineHeight = 4;
    const auto lines = height;
    std::vector<uint8_t> pixels(lines * width); // Reserve space for efficiency

    bool flip = false;
    for (auto i = 0; i < lines; i++)
    {
        if (i % (lineHeight) == 0)
        {
            flip = !flip;
        }
        for (auto j = 0; j < width; j++)
        {
            if (flip)
            {
                pixels.push_back(0xF0);
            }
            else
            {
                pixels.push_back(0x0F);
            }
        }
    }


    return pixels;
}

std::vector<uint8_t> compress_packbits(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> compressed_data;

    for (size_t i = 0; i < data.size();)
    {
        uint8_t currentByte = data[i];
        size_t runLength = 1;

        // Check for run of identical bytes
        while (i + runLength < data.size() && data[i + runLength] == currentByte && runLength < 128)
        {
            runLength++;
        }

        if (runLength >= 2)
        {
            // Run of identical bytes: use RLE
            // For runs of n identical bytes, store (257-n) followed by the byte
            compressed_data.push_back(static_cast<uint8_t>(257 - runLength));
            compressed_data.push_back(currentByte);
            i += runLength;
        }
        else
        {
            // Look for literal run (different consecutive bytes)
            size_t literalStart = i;
            size_t literalLength = 1;

            // Find how many non-repeating bytes we have
            while (i + literalLength < data.size() && literalLength < 128)
            {
                // Check if next byte starts a run of 2 or more
                if (i + literalLength + 1 < data.size() &&
                    data[i + literalLength] == data[i + literalLength + 1])
                {
                    break;
                }
                literalLength++;
            }

            // Store literal run: length-1 followed by the bytes
            compressed_data.push_back(static_cast<uint8_t>(literalLength - 1));
            for (size_t j = 0; j < literalLength; j++)
            {
                compressed_data.push_back(data[i + j]);
            }
            i += literalLength;
        }
    }

    return compressed_data;
}

int main(const int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./pdf2td2020 <output_device>\n";
        return 1;
    }

    const std::vector<uint8_t> test{
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x22, 0x22, 0x23, 0xBA, 0xBF,
        0xA2, 0x22, 0x2B
    };

    auto compressed_test = compress_packbits(test);

    const std::string device_path = argv[1];

    fd = open(device_path.c_str(), O_RDWR);
    if (fd < 0)
    {
        std::cerr << "Cannot open output: " << device_path << ", " << strerror(errno) << '\n';
        return 2;
    }

    const auto pbm = load_pbm_with_padding("../rar-1.pbm");
    std::vector<uint8_t> pixels;

    constexpr auto linewidth = 56;
    const auto lines = pbm.size()/linewidth;

    write_invalidate();
    write_init_sequence();
    write_switch_dynamic_mode(td2000::CommandMode::Raster);
    get_status_request();

    td2000::PrintInfo print_info{};
    print_info.validFields = static_cast<uint8_t>(td2000::PrintInfoFlags::pi_kind) | static_cast<uint8_t>(
        td2000::PrintInfoFlags::pi_width) | static_cast<uint8_t>(td2000::PrintInfoFlags::pi_revovery);
    print_info.mediaType = 0x0A;
    print_info.mediaWith = 58;
    print_info.rasterNumber[0] = lines;
    print_info.rasterNumber[1] = (lines >> 8) & 0xFF;
    print_info.rasterNumber[2] = (lines >> 16) & 0xFF;


    write_print_info(print_info);
    constexpr td2000::VariousModeSettings various{};
    write_various_mode_settings(various);
    write_additional_media(std::vector<uint8_t>(td2000::MediaInfo::_58mm.begin(), td2000::MediaInfo::_58mm.end()));
    write_set_compression_mode(td2000::CompressionMode::NoCompression);


    for (int i = 0; i < pbm.size(); i += linewidth) {
        const auto pos = pbm.begin() + i;
        std::vector<uint8_t> line(pos, pos+linewidth);
        write_graphics_transfer(line);
    }
    write_print_command_with_feeding();
    get_status_request();

    close(fd);
    std::cout << "Sent label to printer (test pattern)!\n";
    return 0;
}