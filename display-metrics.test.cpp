// Copyright (c) 2026 acrion innovations GmbH
// Authors: Stefan Zipproth, s.zipproth@acrion.ch
//
// This file is part of wayland-display-info, see https://github.com/acrion/wayland-display-info
//
// wayland-display-info is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// wayland-display-info is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with wayland-display-info. If not, see <https://www.gnu.org/licenses/>.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "display-metrics.hpp"

TEST_CASE("a 1920x1080 mode on a 480x270 mm panel has 101.6 DPI") {
    const auto dpi = output_dpi(1920, 1080, 480, 270);

    REQUIRE(dpi);
    CHECK(*dpi == doctest::Approx(101.6));
}

// A 16:9 mode on a 16:10 panel results in bars above and below the image;
// the horizontal density corresponds to the visible pixels.
TEST_CASE("a letterboxed mode reports the larger of both densities") {
    const auto dpi = output_dpi(1920, 1080, 480, 300);

    REQUIRE(dpi);
    CHECK(*dpi == doctest::Approx(101.6));
}

// Some EDIDs give a width yet a height of 0 mm. Up to 1.0.8 the vertical
// density was divided by that height, display-info received "inf", and both
// wayland-font-dpi services ended on it and were restarted in a loop.
TEST_CASE("an output with a physical height of 0 mm has no DPI") {
    CHECK_FALSE(output_dpi(3840, 2160, 600, 0));
}

TEST_CASE("an output with a physical width of 0 mm has no DPI") {
    CHECK_FALSE(output_dpi(3840, 2160, 0, 340));
}

TEST_CASE("an output of unknown physical size has no DPI") {
    CHECK_FALSE(output_dpi(1920, 1080, 0, 0));
}

TEST_CASE("an output whose mode size is not known yet has no DPI") {
    CHECK_FALSE(output_dpi(0, 0, 480, 270));
}

TEST_CASE("a negative physical size gives no DPI") {
    CHECK_FALSE(output_dpi(1920, 1080, -480, 270));
}
