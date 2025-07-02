//
// Created by armin on 23.06.25.
//

#include <td2000/td2000.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace td2000;

Printer::Printer(const ModelCode& model, std::ostream& device): model(model), device(device)
{
}

bool Printer::write_sequence(const std::vector<std::uint8_t>& buffer)
{
    device.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    device.flush();
    if (!device.good()) {
        return false;
    }
    return true;
}

void Printer::invalidate()
{
    const std::vector<uint8_t> data(200, 0);
    write_sequence(data);
}

void Printer::initialize()
{
    const std::vector<uint8_t> data(td2000::commands::initialize.begin(), td2000::commands::initialize.end());
    write_sequence(data);
}

td2000::PrinterInfo Printer::status_information_request()
{
    const std::vector<uint8_t> data{
        td2000::commands::status_information_request.begin(), td2000::commands::status_information_request.end()
    };

    td2000::PrinterInfo printer_info{};
    // if (write_sequence(data))
    // {
    //     if (const auto read_bytes = read(fd, printer_info.raw, sizeof(printer_info.raw)); read_bytes != sizeof(
    //         printer_info.raw))
    //     {
    //         std::cerr << "Failed to read printer status!\n";
    //         std::cerr << errno << '\n';
    //         std::cerr << strerror(errno);
    //     }
    //     std::cout << "Printer info: " << printer_info.raw[0] << '\n';
    // }

    return printer_info;
}

void Printer::switch_dynamic_command_mode(td2000::CommandMode mode)
{
    std::vector<uint8_t> data = {
        td2000::commands::switch_dynamic_command_mode.begin(), td2000::commands::switch_dynamic_command_mode.end(),
    };
    data.push_back(static_cast<uint8_t>(mode));
    write_sequence(data);
}

void Printer::additional_media_information(const std::array<uint8_t, 127>& media_info)
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

void Printer::print_information(const td2000::PrintInfo& info)
{
    std::vector<uint8_t> data = {
        td2000::commands::print_information.begin(), td2000::commands::print_information.end()
    };

    data.insert(data.end(), info.raw.begin(), info.raw.end());
    data.insert(data.end(), info.raw.begin(), info.raw.end());
    write_sequence(data);
}

void Printer::various_mode_settings(const td2000::VariousModeSettings& settings)
{
    std::vector<uint8_t> data = {
        td2000::commands::various_mode_settings.begin(), td2000::commands::various_mode_settings.end()
    };

    data.push_back(settings.raw);
    write_sequence(data);
}

void Printer::specify_margin_amount(const uint16_t margin)
{
    std::vector<uint8_t> data = {
        td2000::commands::specify_margin_amount.begin(), td2000::commands::specify_margin_amount.end()
    };
    data.push_back(margin);
    data.push_back(margin >> 8);

    write_sequence(data);
}

void Printer::set_compression_mode(const td2000::CompressionMode mode)
{
    std::vector<uint8_t> data = {
        td2000::commands::select_compression_mode.begin(), td2000::commands::select_compression_mode.end()
    };
    data.push_back(static_cast<uint8_t>(mode));
    write_sequence(data);
}

void Printer::graphics_transfer(const std::vector<uint8_t>& line)
{
    std::vector<uint8_t> data = {
        td2000::commands::raster_graphics_transfer.begin(), td2000::commands::raster_graphics_transfer.end()
    };

    data.push_back(0x00);
    data.push_back(56);
    data.insert(data.end(), line.begin(), line.end());

    write_sequence(data);
}

void Printer::zero_raster_graphics()
{
    const std::vector<uint8_t> data = {
        td2000::commands::zero_raster_graphics.begin(), td2000::commands::zero_raster_graphics.end()
    };
    write_sequence(data);
}

void Printer::print()
{
    const std::vector<uint8_t> data = {
        td2000::commands::print.begin(), td2000::commands::print.end()
    };
    write_sequence(data);
}

void Printer::print_with_feeding()
{
    const std::vector<uint8_t> data = {
        td2000::commands::print_with_feeding.begin(), td2000::commands::print_with_feeding.end()
    };
    write_sequence(data);
}

void Printer::print_pbm(const std::string& filename)
{
    auto pixels = load_pbm_with_padding(filename);

}


std::vector<uint8_t> Printer::load_pbm_with_padding(const std::string& filename)
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

    const int line_bytes = width / 8;

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
    for (auto i = 0; i < padding_bits_count; i++)
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


