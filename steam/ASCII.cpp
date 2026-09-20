// ascii_table.cpp — enumerate and illustrate every ASCII code in C++
// Build: g++ ascii_table.cpp -o ascii_table -std=c++17 -O2
//        cl /EHsc /std:c++17 ascii_table.cpp

#include <iostream>
#include <iomanip>
#include <string>
#include <array>
#include <bitset>
#include <cctype>

// ---------------------------------------------------------------- names
// Traditional ASCII control-code names (C0 range, 0x00–0x1F, plus DEL).
static const char* CTRL_NAMES[32] = {
    "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
    "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
    "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
    "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
};

// C1 range names (0x80–0x9F) for reference in extended tables.
static const char* C1_NAMES[32] = {
    "PAD", "HOP", "BPH", "NBH", "IND", "NEL", "SSA", "ESA",
    "HTS", "HTJ", "VTS", "PLD", "PLU", "RI",  "SS2", "SS3",
    "DCS", "PU1", "PU2", "STS", "CCH", "MW",  "SPA", "EPA",
    "SOS", "SGC", "SCI", "CSI", "ST",  "OSC", "PM",  "APC",
};

// ---------------------------------------------------------------- classify
enum class Kind { Control, Printable, Space, Delete, Extended };

static Kind Classify(int c) {
    if (c < 0x20)              return Kind::Control;
    if (c == 0x20)             return Kind::Space;
    if (c < 0x7F)              return Kind::Printable;
    if (c == 0x7F)             return Kind::Delete;
    return Kind::Extended;
}

// Render a printable char safely — never emit raw control bytes.
static std::string DisplayChar(int c) {
    Kind k = Classify(c);
    switch (k) {
    case Kind::Control:   return std::string("^") + char('@' + c);         // ^A
    case Kind::Space:     return "SP";
    case Kind::Printable: return std::string(1, (char)c);
    case Kind::Delete:    return "^?";
    case Kind::Extended:  return c >= 0xA0 ? std::string(1, (char)c) : "..";
    }
    return "?";
}

// Human-readable name for a code.
static std::string CodeName(int c) {
    if (c < 0x20) return CTRL_NAMES[c];
    if (c == 0x20) return "SPACE";
    if (c == 0x7F) return "DEL";
    if (c >= 0x80 && c <= 0x9F) return C1_NAMES[c - 0x80];
    return "";
}

// ---------------------------------------------------------------- sections
static void PrintHeader() {
    std::cout <<
        "==============================================================\n"
        "  ASCII TABLE  —  American Standard Code for Information\n"
        "  Interchange (ANSI X3.4-1968, unchanged since 1986)\n"
        "  128 codes across 7 bits, split into four blocks:\n"
        "    0x00-0x1F  control codes        (32 codes)\n"
        "    0x20-0x3F  punctuation + digits (32 codes)\n"
        "    0x40-0x5F  uppercase + symbols  (32 codes)\n"
        "    0x60-0x7F  lowercase + symbols  (32 codes)\n"
        "==============================================================\n\n";
}

// Detailed listing: decimal, hex, octal, binary, char, name.
static void PrintDetailed() {
    std::cout << "--------------------------------------------------------------\n"
              << "  DEC   HEX   OCT     BINARY    CH   NAME\n"
              << "--------------------------------------------------------------\n";

    for (int c = 0; c < 128; ++c) {
        std::string shown = DisplayChar(c);
        std::cout << std::setw(5) << std::right << c << "   "
                  << "0x" << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << c
                  << std::setfill(' ') << std::dec << "   "
                  << std::setw(3) << std::oct << c << std::dec << "   "
                  << std::bitset<7>(c) << "    "
                  << std::setw(3) << std::left << shown << "  "
                  << CodeName(c) << "\n";
    }
    std::cout << "\n";
}

// Four compact blocks, each 32 rows × 4 columns (matching the classic layout).
static void PrintBlocks() {
    std::cout << "--------------------------------------------------------------\n"
              << "  COMPACT TABLE — four 32-row blocks, columns are 0-3 offsets\n"
              << "--------------------------------------------------------------\n";

    for (int base = 0; base < 128; base += 32) {
        std::cout << "\n  Base 0x" << std::hex << std::uppercase << base << std::dec
                  << " (" << base << "–" << base + 31 << "):\n";

        for (int row = 0; row < 32; ++row) {
            for (int col = 0; col < 4; ++col) {
                int c = base + row + col * 32;
                if (c >= 128) continue;

                std::cout << "  " << std::setw(3) << std::right << c << " "
                          << "0x" << std::setw(2) << std::setfill('0')
                          << std::hex << std::uppercase << c
                          << std::setfill(' ') << std::dec << "  ";

                std::string shown = DisplayChar(c);
                if (shown.size() == 1) std::cout << "  " << shown << "  ";
                else                   std::cout << shown << (shown.size() == 2 ? "  " : " ");
            }
            std::cout << "\n";
        }
    }
    std::cout << "\n";
}

// Categorised breakdown: every ASCII code grouped by role.
static void PrintCategories() {
    std::cout << "--------------------------------------------------------------\n"
              << "  CATEGORIES\n"
              << "--------------------------------------------------------------\n";

    struct Cat { const char* label; int lo, hi; };
    const Cat cats[] = {
        { "C0 controls (non-printable)",  0x00, 0x1F },
        { "Printable punctuation",        0x21, 0x2F },
        { "Digits 0-9",                   0x30, 0x39 },
        { "More punctuation",             0x3A, 0x40 },
        { "Uppercase A-Z",                0x41, 0x5A },
        { "Punctuation + symbols",        0x5B, 0x60 },
        { "Lowercase a-z",                0x61, 0x7A },
        { "Braces + tilde",               0x7B, 0x7E },
        { "Space and delete",             0x20, 0x20 },
    };

    for (const auto& cat : cats) {
        std::cout << "\n  " << cat.label << ":\n    ";
        int count = 0;
        for (int c = cat.lo; c <= cat.hi; ++c) {
            std::cout << DisplayChar(c) << " ";
            if (++count % 16 == 0) std::cout << "\n    ";
        }
        std::cout << "\n";
    }
    std::cout << "\n  DEL (0x7F): " << DisplayChar(0x7F) << "\n\n";
}

// Fun: draw a picture using only ASCII codes looked up by name.
static void PrintNameLookup() {
    std::cout << "--------------------------------------------------------------\n"
              << "  NAME LOOKUP — spell words using only ASCII control names\n"
              << "--------------------------------------------------------------\n";

    // Each entry is a decimal ASCII value. The comments name the code.
    const int secret[] = {
        0x53, 0x54, 0x45, 0x41, 0x4D,   // S T E A M
        0x20,                            // space
        0x52, 0x55, 0x4C, 0x45, 0x53,   // R U L E S
    };
    std::cout << "  ";
    for (int c : secret) std::cout << (char)c;
    std::cout << "\n\n";
}

// Optional: extended ASCII (0x80–0xFF) as OEM/Windows-1252 approximations.
static void PrintExtended() {
    std::cout << "--------------------------------------------------------------\n"
              << "  EXTENDED BYTES (0x80–0xFF) — not ASCII, shown for reference\n"
              << "  Bytes 0x80–0x9F are C1 controls; 0xA0–0xFF are printable\n"
              << "  in most Windows-1252 / Latin-1 code pages.\n"
              << "--------------------------------------------------------------\n";

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 16; ++col) {
            int c = 0x80 + row * 16 + col;
            std::cout << "  " << std::setw(3) << c << " "
                      << "0x" << std::setw(2) << std::setfill('0')
                      << std::hex << std::uppercase << c
                      << std::setfill(' ') << std::dec << " "
                      << DisplayChar(c) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

// ---------------------------------------------------------------- stats
static void PrintStats() {
    int ctrl = 0, print = 0, space = 1, del = 1;
    for (int c = 0; c < 128; ++c) {
        switch (Classify(c)) {
            case Kind::Control:   ++ctrl;  break;
            case Kind::Printable: ++print; break;
            case Kind::Space:     ++space; break;
            case Kind::Delete:    ++del;   break;
            default: break;
        }
    }
    std::cout << "--------------------------------------------------------------\n"
              << "  SUMMARY\n"
              << "--------------------------------------------------------------\n"
              << "  Total ASCII codes : 128\n"
              << "  Control codes     : " << ctrl  << "  (0x00–0x1F)\n"
              << "  Printable chars   : " << print << "  (0x21–0x7E, excluding space)\n"
              << "  Space             : " << space << "\n"
              << "  Delete            : " << del   << "  (0x7F)\n"
              << "  Bits per code     : 7  (original standard)\n"
              << "  Bytes on disk     : 8  (standard storage, top bit unused)\n"
              << "--------------------------------------------------------------\n";
}

// ---------------------------------------------------------------- main
int main() {
    PrintHeader();
    PrintDetailed();
    PrintBlocks();
    PrintCategories();
    PrintNameLookup();
    PrintExtended();
    PrintStats();
    return 0;
}