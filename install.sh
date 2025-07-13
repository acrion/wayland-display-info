#!/usr/bin/env bash
# Copyright (c) 2025 acrion innovations GmbH
# Authors: Stefan Zipproth, s.zipproth@acrion.ch
#
# This file is part of wayland-display-info, see https://github.com/acrion/wayland-display-info
#
# wayland-display-info is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# wayland-display-info is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with wayland-display-info. If not, see <https://www.gnu.org/licenses/>.

# Installation script for wayland-display-info

set -eu

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)" 
   exit 1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Installing wayland-display-info...${NC}"

# Check if required binary exists
if [[ ! -f "wayland-display-info" ]]; then
    echo -e "${RED}Error: Binary 'wayland-display-info' not found!${NC}"
    echo "Please run ./generate-protocol-stubs.sh and ./build.sh first"
    exit 1
fi

# Check if required files exist
REQUIRED_FILES=(
    "wayland-display-info.service"
    "README.md"
    "LICENSE"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [[ ! -f "$file" ]]; then
        echo -e "${RED}Error: Required file '$file' not found!${NC}"
        exit 1
    fi
done

# Create directory for executable
echo "Creating directories..."
mkdir -p /usr/lib/wayland-display-info

# Install executable
echo "Installing executable..."
install -Dm755 wayland-display-info /usr/lib/wayland-display-info/wayland-display-info

# Install systemd service
echo "Installing systemd service..."
install -Dm644 wayland-display-info.service /usr/lib/systemd/user/wayland-display-info.service

# Install documentation
echo "Installing documentation..."
install -Dm644 README.md /usr/share/doc/wayland-display-info/README.md
install -Dm644 LICENSE   /usr/share/licenses/wayland-display-info/LICENSE

# Install man page if it exists
if [[ -f "wayland-display-info.1" ]]; then
    echo "Installing man page..."
    install -Dm644 wayland-display-info.1 /usr/share/man/man1/wayland-display-info.1
fi

# Reload systemd
echo "Reloading systemd..."
systemctl daemon-reload

# Enable service
echo "Enabling service..."
systemctl --global enable wayland-display-info.service

# Create cache directory
echo "Creating cache directory..."
mkdir -p /var/cache/wayland-display-info
chmod 1777 /var/cache/wayland-display-info

echo -e "${GREEN}Installation complete!${NC}"
echo
echo "The service will start automatically on next login."
echo
echo "You can check the service status with:"
echo "  systemctl --user status wayland-display-info.service"
echo
echo "View current display information with:"
echo "  cat /var/cache/wayland-display-info/display-info"