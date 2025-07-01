#ifndef TD2000_H
#define TD2000_H
#include <cstdint>

namespace td2000
{
    namespace commands
    {
        constexpr std::array<uint8_t,1> invalidate{0x00};
        constexpr std::array<uint8_t,2> initialize{0x1B,0x40};
        constexpr std::array<uint8_t,3> status_information_request{0x1B,0x69,0x53};
        constexpr std::array<uint8_t,3> switch_dynamic_command_mode{0x1B,0x69,0x61};
        constexpr std::array<uint8_t,5> additional_media_information{0x1B,0x69,0x55,0x77,0x01};
        constexpr std::array<uint8_t,3> print_information{0x1B,0x69,0x7A};
        constexpr std::array<uint8_t,3> various_mode_settings{0x1B,0x69,0x4D};
        constexpr std::array<uint8_t,3> specify_margin_amount{0x1B,0x69,0x64};
        constexpr std::array<uint8_t,1> select_compression_mode{0x4D};
        constexpr std::array<uint8_t,3> raster_graphics_transfer{0x67};
        constexpr std::array<uint8_t,1> zero_raster_graphics{0x5A};
        constexpr std::array<uint8_t,1> print{0x0C};
        constexpr std::array<uint8_t,1> print_with_feeding{0x1A};
    }

    enum class ModelCode : uint8_t
    {
        Td2000 = 0x33,
        Td2120N = 0x35,
        Td2130N = 0x36,
        Td2030N = 0x44,
        Td2125N = 0x45,
        Td2125Nwb = 0x46,
        Td2135N = 0x47,
        Td2135Nwb = 0x48,
    };

    enum class MediaType : uint8_t
    {
        NoMedia = 0x00,
        ContinuousLengthTape = 0x4A,
        DieCutLabels = 0x4B,
    };

    enum class StatusType : uint8_t
    {
        ReplyToStatusRequest = 0x00,
        PrintingCompleted = 0x01,
        ErrorOccurred = 0x02,
        ExitIfMode = 0x03,
        TurnedOff = 0x04,
        Notification = 0x05,
        PhaseChange = 0x06
    };

    enum class Phase : uint8_t
    {
        ReceivingState = 0x00,
        PrintingState = 0x01
    };

    enum class NotificationNumber : uint8_t
    {
        NotAvailable = 0x00,
        CoolingStarted = 0x03,
        CoolingFinished = 0x04,
        WaitingForPeeling = 0x05,
        FinishedWaitingForPeeling = 0x06,
        PrinterPaused = 0x07,
        FinishedPrinterPaused = 0x08
    };

    enum class BatteryLevel : uint8_t
    {
        Full = 0x00,
        Half = 0x01,
        Low = 0x02,
        ChargingRequired = 0x03,
        AcAdapterInUse = 0x04,
    };

    enum class ContinuosLengthTape : uint8_t
    {
        _57MM,
        _58MM
    };

    enum class DieCutLabels : uint8_t
    {
        _51x26MM,
        _30x30MM,
        _40x40MM,
        _40x50MM,
        _40x60MM,
        _50x30MM,
        _60x60MM
    };

    enum class CommandMode : uint8_t
    {
        ESCP = 0x00,
        Raster = 0x01,
        PTouch = 0x03
    };

    enum class PageType : uint8_t
    {
        startingPage,
        otherPage
    };

    enum class PrintInfoFlags : uint8_t
    {
        pi_kind = 0x02,
        pi_width = 0x04,
        pi_length = 0x08,
        pi_quality = 0x40,
        pi_revovery = 0x80
    };

    union PrintInfo
    {
        struct
        {
            uint8_t validFields:8;
            uint8_t mediaType:8;
            uint8_t mediaWith:8;
            uint8_t mediaLength:8;
            uint8_t rasterNumber[4];
            PageType pageType;
            uint8_t :8;
        } __attribute__((packed));
        std::array<uint8_t, 10> raw;
    };

    union ErrorInformation1
    {
        struct
        {
            uint8_t noMedia : 1;
            uint8_t endOfMedia : 1;
            uint8_t  : 1;
            uint8_t  : 1;
            uint8_t printerInUser;
            uint8_t  : 1;
            uint8_t  : 1;
            uint8_t  : 1;
        } __attribute__((packed));

        uint8_t raw;
    };

    union ErrorInformation2
    {
        struct
        {
            uint8_t replacingMedia : 1;
            uint8_t  : 1;
            uint8_t communicationError : 1;
            uint8_t  : 1;
            uint8_t coverOpen : 1;
            uint8_t  : 1;
            uint8_t mediaCannotBeFed : 1;
            uint8_t systemError : 1;
        } __attribute__((packed));

        uint8_t raw;
    };

    constexpr std::array<uint8_t, 127> _58mm = {
        0x3F, 0x04, 0x3A, 0x00, 0x00, 0x3A, 0x04, 0x00, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x44, 0x20,
        0x35, 0x38, 0x6D, 0x6D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x2E, 0x32,
        0x38, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
    };

    union MediaInfo
    {

    };

    union PrinterInfo
    {
        struct
        {
            uint8_t printHeadMark : 8;
            uint8_t size : 8;
            uint8_t  : 8;
            uint8_t seriesCode : 8;
            ModelCode model;
            uint8_t  : 8;
            BatteryLevel batteryLevel : 8;
            uint8_t  : 8;
            ErrorInformation1 errorInformation1;
            ErrorInformation2 errorInformation2;
            uint8_t mediaWidth : 8;
            MediaType mediaType;
            uint8_t  : 8;
            uint8_t  : 8;
            uint8_t  : 8;
            uint8_t mode : 8;
            uint8_t  : 8;
            uint8_t mediaLength : 8;
            StatusType statusType;
            Phase phaseType : 8;
            uint8_t phaseNumber_lo : 8;
            uint8_t phaseNumber_hi : 8;
            NotificationNumber notificationNumber;
            uint8_t  : 8;
            uint8_t  : 8;
        } __attribute__((packed));

        uint8_t raw[32];
    };
} // namespace td2000

#endif //TD2000_H
