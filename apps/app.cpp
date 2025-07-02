//
// Created by armin on 02.07.25.
//

#include <fstream>

#include "td2000/td2000.h"

int main()
{
    std::ofstream stream("/dev/usb/td2020");
    td2000::Printer printer(td2000::ModelCode::Td2000 ,stream);

    const auto data = printer.load_pbm_with_padding("../../rar-1.pbm");
    constexpr auto linewidth = 56;
    const auto lines = data.size()/linewidth;

    printer.invalidate();
    printer.initialize();
    printer.switch_dynamic_command_mode(td2000::CommandMode::Raster);
    printer.status_information_request();
    td2000::PrintInfo info{};
    info.validFields = static_cast<uint8_t>(td2000::PrintInfoFlags::pi_kind) | static_cast<uint8_t>(
        td2000::PrintInfoFlags::pi_width) | static_cast<uint8_t>(td2000::PrintInfoFlags::pi_revovery);
    info.mediaType = static_cast<uint8_t>(td2000::MediaType::ContinuousLengthTape);
    info.mediaWith = 58;
    info.rasterNumber[0] = lines;
    info.rasterNumber[1] = (lines >> 8) & 0xFF;
    info.rasterNumber[2] = (lines >> 16) & 0xFF;


    printer.print_information(info);
    constexpr td2000::VariousModeSettings settings{};
    printer.various_mode_settings(settings);
    printer.additional_media_information(td2000::MediaInfo::_58mm);
    printer.set_compression_mode(td2000::CompressionMode::NoCompression);

    for (int i = 0; i < data.size(); i += linewidth) {
        const auto pos = data.begin() + i;
        std::vector<uint8_t> line(pos, pos+linewidth);
        printer.graphics_transfer(line);
    }

    printer.print_with_feeding();

    stream.close();
}