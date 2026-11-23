/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2026  mork009 <mork009resynd@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as 
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>. 
 * 
 */

#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <fstream>
#include <cstdint>

#include "rs-utils/io/dernc.h"
#include "rs-utils/io/file.h"

TEST_CASE( "RNC compression and decompression round-trip", "[utils][rnc]" ) {
    SECTION( "Synthetic buffer round-trip across various sizes" ) {
        std::vector<size_t> testSizes = { 20, 64, 128, 512, 1024, 4096, 65536 };
        for (size_t sz : testSizes) {
            std::vector<uint8_t> input(sz);
            for (size_t i = 0; i < sz; ++i) {
                input[i] = static_cast<uint8_t>((i * 37 + 11) & 0xFF);
            }

            std::vector<uint8_t> packed;
            REQUIRE( rnc::pack(input.data(), input.size(), packed, 1) == rnc::kOk );
            REQUIRE( packed.size() >= 18 );
            REQUIRE( rnc::isRncCompressed(packed.data()) );

            size_t unpLen = 0;
            REQUIRE( rnc::unpackedLength(packed.data(), unpLen) == rnc::kOk );
            REQUIRE( unpLen == input.size() );

            std::vector<uint8_t> unpacked(unpLen);
            size_t realLen = 0;
            REQUIRE( rnc::unpack(packed.data(), unpacked.data(), realLen) == rnc::kOk );
            REQUIRE( realLen == input.size() );
            REQUIRE( unpacked == input );
        }
    }

    SECTION( "Reject invalid and too-small input" ) {
        std::vector<uint8_t> packed;
        REQUIRE( rnc::pack(nullptr, 100, packed) != rnc::kOk );

        std::vector<uint8_t> emptyBuf;
        REQUIRE( rnc::pack(emptyBuf.data(), 0, packed) != rnc::kOk );

        std::vector<uint8_t> tooSmall(15, 0xAA);
        REQUIRE( rnc::pack(tooSmall.data(), tooSmall.size(), packed) != rnc::kOk );
    }

    SECTION( "Round-trip on original Syndicate game assets" ) {
        auto verifyAssetRoundTrip = [](const std::string &relPath) {
            std::string fullPath = rs_utl::File::getOriginalDataFullPath(relPath);
            std::ifstream in(fullPath, std::ios::binary);
            if (!in.is_open()) return; // Skip if asset not installed in test environment

            std::vector<uint8_t> origPacked((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();

            if (!rnc::isRncCompressed(origPacked.data())) return;

            size_t unpLen = 0;
            REQUIRE( rnc::unpackedLength(origPacked.data(), unpLen) == rnc::kOk );
            std::vector<uint8_t> origUnpacked(unpLen);
            size_t realLen = 0;
            REQUIRE( rnc::unpack(origPacked.data(), origUnpacked.data(), realLen) == rnc::kOk );

            std::vector<uint8_t> repacked;
            REQUIRE( rnc::pack(origUnpacked.data(), origUnpacked.size(), repacked, 1) == rnc::kOk );
            REQUIRE( rnc::isRncCompressed(repacked.data()) );

            size_t repackedUnpLen = 0;
            REQUIRE( rnc::unpackedLength(repacked.data(), repackedUnpLen) == rnc::kOk );
            REQUIRE( repackedUnpLen == origUnpacked.size() );

            std::vector<uint8_t> reunpacked(repackedUnpLen);
            size_t finalLen = 0;
            REQUIRE( rnc::unpack(repacked.data(), reunpacked.data(), finalLen) == rnc::kOk );
            REQUIRE( finalLen == origUnpacked.size() );
            REQUIRE( reunpacked == origUnpacked );
        };

        verifyAssetRoundTrip("game01.dat");
        verifyAssetRoundTrip("map01.dat");
        verifyAssetRoundTrip("miss01.dat");
    }
}



