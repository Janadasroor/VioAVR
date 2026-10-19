#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega8.hpp"

#include <string_view>
#include <stdexcept>

TEST_CASE("Intel HEX Image Loader Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    SUBCASE("Load valid HEX string") {
        const std::string_view valid_hex =
            ":040000000C945C0000\n"
            ":00000001FF\n";

        const auto image = HexImageLoader::load_text(valid_hex, atmega8);
        
        REQUIRE(image.flash_words.size() >= 2U);
        CHECK(image.flash_words[0] == 0x940CU);
        CHECK(image.flash_words[1] == 0x005CU);

        MemoryBus bus {atmega8};
        bus.load_flash(image.flash_words);
        CHECK(bus.read_program_word(0U) == 0x940CU);
        CHECK(bus.flash_words().size() == atmega8.flash_words);
    }

    SUBCASE("Trailing zero data is preserved") {
        const std::string_view hex_with_zero_tail =
            ":04000000AABB000097\n"
            ":00000001FF\n";

        const auto image = HexImageLoader::load_text(hex_with_zero_tail, atmega8);

        REQUIRE(image.flash_words.size() == 2U);
        CHECK(image.flash_words[0] == 0xBBAAU);
        CHECK(image.flash_words[1] == 0x0000U);

        MemoryBus bus {atmega8};
        bus.load_flash(image.flash_words);
        CHECK(bus.read_program_word(1U) == 0x0000U);
    }

    SUBCASE("Unprogrammed gap bytes read as erased flash") {
        const std::string_view hex_with_gap =
            ":02000000AABB99\n"
            ":02000400CCDD51\n"
            ":00000001FF\n";

        const auto image = HexImageLoader::load_text(hex_with_gap, atmega8);

        REQUIRE(image.flash_words.size() == 3U);
        CHECK(image.flash_words[0] == 0xBBAAU);
        CHECK(image.flash_words[1] == 0xFFFFU);
        CHECK(image.flash_words[2] == 0xDDCCU);

        MemoryBus bus {atmega8};
        bus.load_flash(image.flash_words);
        CHECK(bus.read_program_word(1U) == 0xFFFFU);
    }

    SUBCASE("Invalid checksum throws") {
        // Last byte '01' instead of '00' for checksum
        const std::string_view invalid_checksum = ":040000000C945C0001\n:00000001FF\n";
        CHECK_THROWS_AS(HexImageLoader::load_text(invalid_checksum, atmega8), std::exception);
    }

    SUBCASE("Out of range address throws") {
        // Address 0x00010000+ is out of ATmega8 range (8KB flash)
        const std::string_view out_of_range = ":020000040001F9\n:01000000AA55\n:00000001FF\n";
        CHECK_THROWS_AS(HexImageLoader::load_text(out_of_range, atmega8), std::exception);
    }

    SUBCASE("Missing EOF record throws") {
        const std::string_view missing_eof = ":040000000C945C0000\n";
        CHECK_THROWS_AS(HexImageLoader::load_text(missing_eof, atmega8), std::exception);
    }
}
