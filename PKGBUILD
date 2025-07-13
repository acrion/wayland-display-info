# Maintainer: Stefan Zipproth <s.zipproth@acrion.ch>

pkgname=wayland-display-info
pkgver=1.0.5
pkgrel=1
pkgdesc="Daemon that keeps /var/cache/wayland-display-info/display-info up to date using wlr-output-management"
arch=('any')
url="https://github.com/acrion/wayland-display-info"
license=('AGPL3')
depends=('systemd')
makedepends=('wayland' 'gcc' 'pkgconf' 'wlr-protocols')
install="${pkgname}.install"
source=(
    "wayland-display-info.cpp"
    "generate-protocol-stubs.sh"
    "build.sh"
    "wayland-display-info.service"
    "wayland-display-info.1"
    "wayland-display-info.install"
    "wayland-display-info.conf"
    "LICENSE"
    "README.md"
)
sha256sums=(
    'SKIP' # wayland-display-info.cpp
    'SKIP' # generate-protocol-stubs.sh
    'SKIP' # build.sh
    'SKIP' # wayland-display-info.service
    'SKIP' # wayland-display-info.1
    'SKIP' # wayland-display-info.install
    'SKIP' # wayland-display-info.conf
    'SKIP' # LICENSE
    'SKIP' # README.md
)

build() {
    cd "${srcdir}"
    ./generate-protocol-stubs.sh
    ./build.sh
}

package() {
    # Executable
    install -Dm755 "${srcdir}/wayland-display-info"         "${pkgdir}/usr/lib/${pkgname}/wayland-display-info"

    # Per‑user systemd service
    install -Dm644 "${srcdir}/wayland-display-info.service" "${pkgdir}/usr/lib/systemd/user/wayland-display-info.service"

    # tmpfiles – ensures /var/cache dir exists with 1777 perms at boot
    install -Dm644 "${srcdir}/wayland-display-info.conf"    "${pkgdir}/usr/lib/tmpfiles.d/wayland-display-info.conf"

    # Man page, Documentation & License
    install -Dm644 "${srcdir}/wayland-display-info.1"       "${pkgdir}/usr/share/man/man1/wayland-display-info.1"
    install -Dm644 "${srcdir}/README.md"                    "${pkgdir}/usr/share/doc/${pkgname}/README.md"
    install -Dm644 "${srcdir}/LICENSE"                      "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
