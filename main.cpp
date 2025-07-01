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
    // for (const auto& c : buffer)
    // {
    //     if (write(fd, &c, 1) != 1)
    //         return false;
    // }
    auto size = buffer.size();
    if (write(fd, buffer.data(), buffer.size()) != buffer.size())
    {
        return false;
    }
    return true;
}

void write_invalidate()
{
    const std::vector<uint8_t> buffer(200, 0);

    write_sequence(buffer);
}

void write_init_sequence()
{
    const std::vector<uint8_t> buffer(td2000::commands::initialize.begin(), td2000::commands::initialize.end());
    write_sequence(buffer);
}

td2000::PrinterInfo get_status_request()
{
    const std::vector<uint8_t> status_request(td2000::commands::status_information_request.begin(),
                                              td2000::commands::status_information_request.end());

    td2000::PrinterInfo printer_info{};
    if (write_sequence(status_request))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto read_bytes = read(fd, printer_info.raw, 32);
        if (read_bytes != 32)
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
    const std::vector<uint8_t> mode_request = {
        0x1B, 0x69, 0x61,
        static_cast<uint8_t>(mode)
    };
    write_sequence(mode_request);
}

void write_additional_media(const std::vector<uint8_t>& media_info)
{
    if (media_info.size() != 127)
    {
        std::cerr << "Media info must be 127 bytes long!\n";
        return;
    }
    const std::vector<uint8_t> media_info_command = {
        td2000::commands::additional_media_information.begin(), td2000::commands::additional_media_information.end()
    };

    write_sequence(media_info_command);
    write_sequence(media_info);
}

void write_print_info(const td2000::PrintInfo& info)
{
    const std::vector<uint8_t> label_info = {
        0x1B, 0x69, 0x7A,
    };
    const std::vector<uint8_t> buffer(info.raw.begin(), info.raw.end());
    write_sequence(label_info);
    write_sequence(buffer);
}

void write_various_mode_settings()
{
    const std::vector<uint8_t> label_info = {
        0x1B, 0x69, 0x4D, 0x00
    };
    write_sequence(label_info);
}

void write_specify_margin_amount()
{
    const std::vector<uint8_t> label_info = {
        0x1B, 0x69, 0x64, 24, 0x00
    };
    write_sequence(label_info);
}

void write_set_compression_mode()
{
    const std::vector<uint8_t> label_info = {
        0x4D, 0x00
    };
    write_sequence(label_info);
}

void write_graphics_transfer(const std::vector<uint8_t>& line)
{
    const std::vector<uint8_t> buffer = {
        0x67, 0x00, 56
    };
    write_sequence(buffer);
    write_sequence(line);
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
    const std::vector<uint8_t> buffer = {
        0x5A
    };
    write_sequence(buffer);
}

void write_print_command()
{
    const std::vector<uint8_t> buffer = {
        0x0C
    };
    write_sequence(buffer);
}

void write_print_command_with_feeding()
{
    const std::vector<uint8_t> buffer = {
        0x1A
    };
    write_sequence(buffer);
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

std::vector<std::vector<uint8_t>> chessboard(const int size)
{
    std::vector<std::vector<uint8_t>> pixels;

    const auto lines = size / 56;

    bool flip = false;
    for (auto i = 0; i < lines; i++)
    {
        if (i % 8 == 0)
        {
            flip = !flip;
        }
        auto line = std::vector<uint8_t>(56);
        for (auto j = 0; j < 56; j++)
        {
            if (flip)
            {
                line[j] = 0xF0;
            }
            else
            {
                line[j] = 0x0F;
            }
        }
        pixels.push_back(line);
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
    pixels.reserve(lines * linewidth); // Reserve space for efficiency

    constexpr auto lineheigth = 4;

    bool flip = false;
    for (auto i = 0; i < lines; i++)
    {
        if (i % (lineheigth) == 0)
        {
            flip = !flip;
        }
        for (auto j = 0; j < linewidth; j++)
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

    //auto compressed = compress_packbits(pixels);
    auto compressed = compress_packbits(pbm);

    write_invalidate();
    write_init_sequence();
    write_switch_dynamic_mode(td2000::CommandMode::Raster);
    write_additional_media(std::vector<uint8_t>(td2000::_58mm.begin(), td2000::_58mm.end()));

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
    write_various_mode_settings();
    write_additional_media(std::vector<uint8_t>(td2000::_58mm.begin(), td2000::_58mm.end()));
    write_set_compression_mode();


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