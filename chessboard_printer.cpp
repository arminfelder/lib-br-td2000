#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>

class TD2020Printer {
private:
    std::vector<uint8_t> printData;
    
    void addInvalidateCommand() {
        // Send 200 NULL bytes to reset printer
        for (int i = 0; i < 200; i++) {
            printData.push_back(0x00);
        }
    }
    
    void addInitializeCommand() {
        // ESC @ - Initialize command
        printData.push_back(0x1B);
        printData.push_back(0x40);
    }
    
    void addSwitchDynamicModeCommand() {
        // ESC i a 01h - Switch to raster mode
        printData.push_back(0x1B);
        printData.push_back(0x69);
        printData.push_back(0x61);
        printData.push_back(0x01);
    }
    
    void addPrintInfoCommand() {
        // ESC i z - Print information command for 58mm continuous tape (300dpi)
        printData.push_back(0x1B);
        printData.push_back(0x69);
        printData.push_back(0x7A);
        printData.push_back(0xC6); // Print information type
        printData.push_back(0x0A); // Print area width (low byte)
        printData.push_back(0x3A); // Print area width (high byte)
        printData.push_back(0x00); // Print area length (low byte)
        printData.push_back(0x0A); // Print area length (high byte)
        printData.push_back(0x01); // Number of labels
        printData.push_back(0x00); // Starting page
        printData.push_back(0x00); // Reserved
        printData.push_back(0x00); // Reserved
        printData.push_back(0x00); // Reserved
    }
    
    void addMarginCommand() {
        // ESC i d - Specify margin amount (3mm for 300dpi = 35 dots)
        printData.push_back(0x1B);
        printData.push_back(0x69);
        printData.push_back(0x64);
        printData.push_back(0x23); // 35 dots (3mm at 300dpi)
        printData.push_back(0x00);
    }
    
    void addCompressionModeCommand() {
        // M 02h - Select TIFF compression mode
        printData.push_back(0x4D);
        printData.push_back(0x02);
    }
    
    void addRasterLine(const std::vector<uint8_t>& lineData) {
        // g command - Raster graphics transfer
        printData.push_back(0x67);
        printData.push_back(0x00); // Compression count (uncompressed)
        printData.push_back(lineData.size()); // Data length
        
        // Add the raster line data
        for (uint8_t byte : lineData) {
            printData.push_back(byte);
        }
    }
    
    void addZeroRasterLine() {
        // Z - Zero raster graphics (empty line)
        printData.push_back(0x5A);
    }
    
    void addPrintWithFeedingCommand() {
        // Control-Z - Print command with feeding
        printData.push_back(0x1A);
    }

public:
    void generateChessboardPattern() {
        // Clear any existing data
        printData.clear();
        
        // (1) Initialization commands
        addInvalidateCommand();
        addInitializeCommand();
        
        // (2) Control codes
        addSwitchDynamicModeCommand();
        addPrintInfoCommand();
        addMarginCommand();
        addCompressionModeCommand();
        
        // (3) Generate chessboard raster data
        // For 300dpi TD-2030A: 84 bytes per raster line (672 pins total)
        // For 203dpi TD-2020: 56 bytes per raster line (448 pins total)
        const int bytesPerLine = 84; // For 300dpi model
        const int totalLines = 400;  // Adjust for desired height
        const int squareSize = 8;    // Size of each chess square in pixels
        
        for (int line = 0; line < totalLines; line++) {
            std::vector<uint8_t> lineData(bytesPerLine, 0x00);
            
            // Determine if we're in a black or white row
            int rowSquare = line / squareSize;
            bool rowOdd = (rowSquare % 2) == 1;
            
            // Generate the pattern for this line
            for (int byteIdx = 0; byteIdx < bytesPerLine; byteIdx++) {
                uint8_t byteValue = 0x00;
                
                for (int bit = 0; bit < 8; bit++) {
                    int pixelX = byteIdx * 8 + bit;
                    int colSquare = pixelX / squareSize;
                    bool colOdd = (colSquare % 2) == 1;
                    
                    // XOR to create chessboard pattern
                    bool isBlack = rowOdd ^ colOdd;
                    
                    if (isBlack) {
                        // Set the bit (MSB first)
                        byteValue |= (0x80 >> bit);
                    }
                }
                
                lineData[byteIdx] = byteValue;
            }
            
            // Add the raster line
            addRasterLine(lineData);
        }
        
        // (4) Print command
        addPrintWithFeedingCommand();
    }
    
    void saveToFile(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Could not create file " << filename << std::endl;
            return;
        }
        
        file.write(reinterpret_cast<const char*>(printData.data()), printData.size());
        file.close();
        
        std::cout << "Print data saved to: " << filename << std::endl;
        std::cout << "Total bytes: " << printData.size() << std::endl;
    }
    
    void printToDevice(const std::string& devicePath) {
        std::ofstream device(devicePath, std::ios::binary);
        if (!device) {
            std::cerr << "Error: Could not open device " << devicePath << std::endl;
            return;
        }
        
        device.write(reinterpret_cast<const char*>(printData.data()), printData.size());
        device.close();
        
        std::cout << "Data sent to printer: " << devicePath << std::endl;
    }
};

int main() {
    TD2020Printer printer;
    
    std::cout << "Generating chessboard pattern for TD-2020A printer..." << std::endl;
    
    // Generate the chessboard pattern
    printer.generateChessboardPattern();
    
    // Save to file for inspection or later use
    printer.saveToFile("chessboard_pattern.prn");
    
    // Uncomment the line below to send directly to printer
    // Replace "/dev/usb/lp0" with your actual printer device path
    // printer.printToDevice("/dev/usb/lp0");
    
    std::cout << "Chessboard pattern generated successfully!" << std::endl;
    std::cout << "You can send the .prn file directly to your TD-2020A printer." << std::endl;
    
    return 0;
}
