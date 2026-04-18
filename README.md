# Wayland Display Info

`wayland-display-info` is a lightweight user daemon that monitors your Wayland
compositor through the **wlr-output-management** protocol and keeps the file

```
/var/cache/wayland-display-info/display-info
```

up-to-date. Each line describes a connected output in the form:

```
<OUTPUT_NAME> <DPI> <WIDTH_PX> <HEIGHT_PX>
```

**Note:** The output lines are automatically sorted in descending order by the physical size (diagonal) of the displays. This ensures that your largest (and often primary) screen is listed first.

---

## How DPI is computed

* **Letter-/Pillar-boxing aware** – when the selected mode’s aspect ratio differs
  from the panel’s native ratio and the compositor inserts black bars, the
  daemon reports the *larger* of the horizontal or vertical DPI.
  This keeps the figure constant for the axis that is fully utilised, so the
  value always reflects the density of the visible image.
* **Rotation neutral** – a panel rotated by 90 ° or 270 ° still shows the same
  number of pixels on the same physical surface; therefore rotation has no
  effect on pixel density and requires no special handling.

---

## 🚀 Installation

### Arch Linux (AUR)

```bash
pikaur -S wayland-display-info
```

The PKGBUILD installs

* the binary in `/usr/lib/wayland-display-info/`
* the user service `wayland-display-info.service`
* a tmpfiles entry that creates `/var/cache/wayland-display-info` at boot
  (mode 1777)

After installation the service is enabled automatically; log out and back in,
or start it straight away:

```bash
systemctl --user start wayland-display-info.service
```

### From source

```bash
git clone https://github.com/acrion/wayland-display-info
cd wayland-display-info
./generate-protocol-stubs.sh
./build.sh
sudo ./install.sh
```

---

## 🔎 Status and diagnostics

```bash
systemctl --user status wayland-display-info.service
cat /var/cache/wayland-display-info/display-info
```
