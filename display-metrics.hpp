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

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

// Returns the pixel density of an output, or nothing when its mode or its
// physical size is not positive. Compositors report 0 mm for outputs of
// unknown size, and some EDIDs give a width but a height of 0 mm; dividing by
// that height wrote "inf" into display-info.
//
// When the mode's aspect ratio differs from the panel's, the compositor adds
// black bars along one axis, and the larger of the two densities is the one of
// the visible image.
inline std::optional<double> output_dpi(int32_t width_px, int32_t height_px, int32_t width_mm, int32_t height_mm) {
    if (width_px <= 0 || height_px <= 0 || width_mm <= 0 || height_mm <= 0) return std::nullopt;

    const double dpi_x = static_cast<double>(width_px)  / (width_mm  / 25.4);
    const double dpi_y = static_cast<double>(height_px) / (height_mm / 25.4);
    return std::max(dpi_x, dpi_y);
}
