// Copyright (c) 2025 acrion innovations GmbH
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
//
// Continuously mirrors live outputs information to /var/cache/wayland-display-info/display-info.
// Starts even if the compositor/socket is not yet ready: it retries every 500 ms.

// Writes /var/cache/wayland-display-info/display-info as soon as *any* Wayland display
// socket appears (even if $WAYLAND_DISPLAY was missing when the service started).
// All progress + errors go to stderr → journald when run as systemd service.

#include <wayland-client.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include "zwlr-output-management-unstable-v1-client-protocol.h"
}

using namespace std::chrono_literals;

static constexpr const char *kCachePath = "/var/cache/wayland-display-info/display-info";
static constexpr const char *kTmpPath   = "/var/cache/wayland-display-info/display-info.tmp";
static constexpr auto kRetryDelay       = 500ms;

/* ------------------------------------------------------------------ */
struct HeadData {
    std::string          name;
    bool                 enabled        = false;
    int32_t              width_mm       = 0;
    int32_t              height_mm      = 0;
    zwlr_output_mode_v1 *current_mode   = nullptr;
    int32_t              transform      = WL_OUTPUT_TRANSFORM_NORMAL;
};
struct ModeInfo { int32_t width_px = 0, height_px = 0; };

static wl_display *g_display                                = nullptr;
static zwlr_output_manager_v1 *g_manager                    = nullptr;
static std::map<zwlr_output_head_v1 *, HeadData> g_heads;
static std::map<zwlr_output_mode_v1 *, ModeInfo> g_modes;

/* --------------------- LOGGING HELPER --------------------- */
static void log(const std::string &msg) {
    std::cerr << "[wayland-display-info] " << msg << "\n";
}

/* Forward decl. */
static void write_display_info();

/* ----------------------------- MODE LISTENER ----------------------------- */
static void mode_handle_size(void *, zwlr_output_mode_v1 *m, int32_t w, int32_t h) { g_modes[m] = {w, h}; }
static void mode_handle_refresh(void *, zwlr_output_mode_v1 *, int32_t) {}
static void mode_handle_preferred(void *, zwlr_output_mode_v1 *) {}
static void mode_handle_finished(void *, zwlr_output_mode_v1 *) {}
static const zwlr_output_mode_v1_listener kModeL = {mode_handle_size, mode_handle_refresh, mode_handle_preferred, mode_handle_finished};

/* ----------------------------- HEAD LISTENER ----------------------------- */
#define HD(x) g_heads[static_cast<zwlr_output_head_v1 *>(d)].x
static void head_handle_name(void *d, zwlr_output_head_v1 *, const char *n) { HD(name) = n; }
static void head_handle_description(void *, zwlr_output_head_v1 *, const char *) {}
static void head_handle_physical_size(void *d, zwlr_output_head_v1 *, int32_t w, int32_t h) { HD(width_mm) = w; HD(height_mm) = h; }
static void head_handle_mode(void *, zwlr_output_head_v1 *, zwlr_output_mode_v1 *m) {
    if (!g_modes.count(m)) { g_modes.emplace(m, ModeInfo{}); zwlr_output_mode_v1_add_listener(m, &kModeL, nullptr); }
}
static void head_handle_enabled(void *d, zwlr_output_head_v1 *, int32_t e) { HD(enabled) = e; }
static void head_handle_current_mode(void *d, zwlr_output_head_v1 *, zwlr_output_mode_v1 *m) { HD(current_mode) = m; }
static void head_handle_position(void *, zwlr_output_head_v1 *, int32_t, int32_t) {}
static void head_handle_transform(void *d, zwlr_output_head_v1 *, int32_t t) { HD(transform) = t; }
static void head_handle_scale(void *, zwlr_output_head_v1 *, wl_fixed_t) {}
static void head_handle_finished(void *, zwlr_output_head_v1 *) {}
static void head_handle_make(void *, zwlr_output_head_v1 *, const char *) {}
static void head_handle_model(void *, zwlr_output_head_v1 *, const char *) {}
static void head_handle_serial(void *, zwlr_output_head_v1 *, const char *) {}
static void head_handle_adaptive_sync(void *, zwlr_output_head_v1 *, uint32_t) {}
#undef HD
static const zwlr_output_head_v1_listener kHeadL = {
    /* order! */
    head_handle_name, head_handle_description, head_handle_physical_size, head_handle_mode, head_handle_enabled,
    head_handle_current_mode, head_handle_position, head_handle_transform, head_handle_scale, head_handle_finished,
    head_handle_make, head_handle_model, head_handle_serial, head_handle_adaptive_sync};

/* ----------------------- MANAGER LISTENER ----------------------- */
static void manager_handle_head(void *, zwlr_output_manager_v1 *, zwlr_output_head_v1 *h) {
    g_heads.emplace(h, HeadData{});
    zwlr_output_head_v1_add_listener(h, &kHeadL, h);
}
static void manager_handle_done(void *, zwlr_output_manager_v1 *, uint32_t) { write_display_info(); }
static const zwlr_output_manager_v1_listener kManagerL = {manager_handle_head, manager_handle_done};

/* ------------------------- REGISTRY HELPER ------------------------- */
static void registry_global(void *, wl_registry *reg, uint32_t name, const char *iface, uint32_t ver) {
    if (strcmp(iface, zwlr_output_manager_v1_interface.name) == 0) {
        g_manager = static_cast<zwlr_output_manager_v1 *>(wl_registry_bind(reg, name, &zwlr_output_manager_v1_interface, std::min<uint32_t>(ver, 3)));
        zwlr_output_manager_v1_add_listener(g_manager, &kManagerL, nullptr);
    }
}
static void registry_global_remove(void *, wl_registry *, uint32_t) {}
static const wl_registry_listener kRegistryL = {registry_global, registry_global_remove};

/* --------------------- ATOMIC FILE WRITER --------------------- */
static void write_display_info() {
    namespace fs = std::filesystem;
    std::ofstream f(kTmpPath, std::ios::trunc);
    if (!f) { std::perror("tmp"); return; }

    for (const auto &[head, hd] : g_heads) {
        if (!hd.enabled || hd.name.empty() || !hd.current_mode) continue;
        auto mi_it = g_modes.find(hd.current_mode);
        if (mi_it == g_modes.end()) continue;
        const auto &mi = mi_it->second;
        if (mi.width_px == 0 || mi.height_px == 0 || hd.width_mm == 0) continue;

        // information if the display is rotated might be useful for future version - currently, it is irrelevant
        // const bool rotated = (hd.transform % 2) == 1;             // 90°/270° (+flipped)
        
        const double dpi_x = static_cast<double>(mi.width_px)  / (hd.width_mm  / 25.4);
        const double dpi_y = static_cast<double>(mi.height_px) / (hd.height_mm / 25.4);
        const double dpi   = std::max(dpi_x, dpi_y);

        f << hd.name << ' ' << std::fixed << std::setprecision(2) << dpi << ' ' << mi.width_px << ' ' << mi.height_px << '\n';
    }
    f.close();
    std::error_code ec; std::filesystem::rename(kTmpPath, kCachePath, ec);
    if (ec) std::perror("rename");
}

/* ------------------ SOCKET SCAN HELPERS ------------------ */
static std::vector<std::string> discover_wayland_sockets() {
    std::vector<std::string> names;
    if (const char *runtime = std::getenv("XDG_RUNTIME_DIR")) {
        for (auto &p : std::filesystem::directory_iterator(runtime)) {
            if (p.path().filename().string().rfind("wayland-", 0) == 0) {
                names.push_back(p.path().filename().string());
            }
        }
        std::sort(names.begin(), names.end());
    }
    return names;
}

/* ------------------ CONNECT / RECONNECT ------------------ */
static bool try_connect_once() {
    const char *env = std::getenv("WAYLAND_DISPLAY");
    std::array<std::string, 1> env_arr{{env ? env : ""}};
    std::vector<std::string> candidates;
    if (env && *env) {
        candidates.assign(env_arr.begin(), env_arr.end());
    } else {
        candidates = discover_wayland_sockets();
    }
    if (candidates.empty()) return false;
    for (const auto &name : candidates) {
        g_display = wl_display_connect(name.empty() ? nullptr : name.c_str());
        if (g_display) {
            log("Connected to display socket '" + (name.empty() ? std::string("default") : name) + "'");
            wl_registry *reg = wl_display_get_registry(g_display);
            wl_registry_add_listener(reg, &kRegistryL, nullptr);
            wl_display_roundtrip(g_display);
            if (!g_manager) {
                log("zwlr_output_manager_v1 not advertised, waiting…");
            }
            wl_display_roundtrip(g_display); // get initial heads if manager exists
            return true;
        }
    }
    return false;
}

static void connect_until_success() {
    while (!try_connect_once()) {
        std::this_thread::sleep_for(kRetryDelay);
    }
}

/* ------------------------------ MAIN ------------------------------ */
int main() {
    log("Starting – waiting for Wayland compositor");
    connect_until_success();

    while (true) {
        if (wl_display_dispatch(g_display) == -1) {
            log("Wayland connection lost – reconnecting…");
            wl_display_disconnect(g_display);
            g_display = nullptr; g_manager = nullptr; g_heads.clear(); g_modes.clear();
            connect_until_success();
        }
    }
}
