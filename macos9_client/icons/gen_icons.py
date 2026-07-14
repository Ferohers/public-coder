#!/usr/bin/env python3
"""
Icon generator for AltanAI Mac OS 9 application.
Converts PNG/GIF sources into Rez hex resource data for all icon family members.
App icon source: icons/new-icons/app-icon.pct (decoded via sips)
Toolbar icons  : new-icons/clean.gif, history.gif, settings.gif
"""

import os
import struct
import subprocess
import sys
import tempfile

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow is not installed. Run: pip3 install Pillow")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Mac OS System color palettes (exact lookup tables used by the system)
# ---------------------------------------------------------------------------

MAC_4BIT_COLORS = [
    (255, 255, 255),  # 0  White
    (255, 204,   0),  # 1  Yellow
    (255, 102,   0),  # 2  Orange
    (221,   0,   0),  # 3  Red
    (255,   0, 153),  # 4  Pink
    (102,   0, 153),  # 5  Purple
    (  0,   0, 221),  # 6  Blue
    (  0, 170, 255),  # 7  Cyan
    (  0, 221,   0),  # 8  Green
    (  0, 102,   0),  # 9  Dark Green
    (102,  51,   0),  # 10 Brown
    (153, 102,  51),  # 11 Tan
    (204, 204, 204),  # 12 Light Gray
    (136, 136, 136),  # 13 Medium Gray
    ( 68,  68,  68),  # 14 Dark Gray
    (  0,   0,   0),  # 15 Black
]


def build_mac_system_8bit_clut():
    colors = [(0, 0, 0)] * 256
    for i, rgb in enumerate(MAC_4BIT_COLORS):
        colors[i] = rgb
    levels = [0, 51, 102, 153, 204, 255]
    idx = 16
    for r in levels:
        for g in levels:
            for b in levels:
                colors[idx] = (r, g, b)
                idx += 1
    for i in range(24):
        gray = i * 255 // 23
        colors[232 + i] = (gray, gray, gray)
    return colors


MAC_8BIT_COLORS = build_mac_system_8bit_clut()

# ---------------------------------------------------------------------------
# Colour matching
# ---------------------------------------------------------------------------

def rgb_dist(a, b):
    dr, dg, db = a[0]-b[0], a[1]-b[1], a[2]-b[2]
    rm = (a[0]+b[0]) / 2
    return (2+rm/256)*dr*dr + 4*dg*dg + (2+(255-rm)/256)*db*db


def nearest(rgb, palette):
    return min(range(len(palette)), key=lambda i: rgb_dist(rgb, palette[i]))

# ---------------------------------------------------------------------------
# Image loading
# ---------------------------------------------------------------------------

def load_rgb_pixels(path, size):
    """Load image, resize to size (W,H), return list of (r,g,b) tuples."""
    img = Image.open(path).convert('RGB')
    if img.size != size:
        img = img.resize(size, Image.Resampling.NEAREST)
    if hasattr(img, 'get_flattened_data'):
        return list(img.get_flattened_data())
    return list(img.getdata())

# ---------------------------------------------------------------------------
# Icon data builders
# ---------------------------------------------------------------------------

def pack_1bit(pixel_rows):
    data = bytearray()
    for row in pixel_rows:
        for x in range(0, len(row), 8):
            val = 0
            for bit in row[x:x+8]:
                val = (val << 1) | (1 if bit else 0)
            data.append(val)
    return bytes(data)


def make_bw_icon(pixels, w, h):
    rows, mask_rows = [], []
    for y in range(h):
        row, mrow = [], []
        for x in range(w):
            r, g, b = pixels[y*w+x]
            lum = (r*299 + g*587 + b*114) // 1000
            row.append(lum < 150)
            mrow.append(True)  # fully opaque mask
        rows.append(row)
        mask_rows.append(mrow)
    return pack_1bit(rows) + pack_1bit(mask_rows)


def make_4bit_icon(pixels):
    idxs = [nearest(rgb, MAC_4BIT_COLORS) for rgb in pixels]
    data = bytearray()
    for i in range(0, len(idxs), 2):
        data.append((idxs[i] << 4) | idxs[i+1])
    return bytes(data)


def make_8bit_icon(pixels):
    return bytes(nearest(rgb, MAC_8BIT_COLORS) for rgb in pixels)


def make_icon_family(path):
    """Return dict of resource_type -> bytes for all icon family members."""
    large = load_rgb_pixels(path, (32, 32))
    small = load_rgb_pixels(path, (16, 16))
    return {
        'ICN#': make_bw_icon(large, 32, 32),
        'ics#': make_bw_icon(small, 16, 16),
        'icl4': make_4bit_icon(large),
        'ics4': make_4bit_icon(small),
        'icl8': make_8bit_icon(large),
        'ics8': make_8bit_icon(small),
    }

# ---------------------------------------------------------------------------
# PICT file handling (via sips)
# ---------------------------------------------------------------------------

def pict_to_png(pict_path):
    """Use macOS sips to decode a PICT file to a temporary PNG. Returns path."""
    tmp = tempfile.NamedTemporaryFile(suffix='.png', delete=False)
    tmp.close()
    result = subprocess.run(
        ['sips', '-s', 'format', 'png', pict_path, '--out', tmp.name],
        capture_output=True
    )
    if result.returncode != 0 or not os.path.exists(tmp.name):
        raise RuntimeError(f"sips failed: {result.stderr.decode()}")
    return tmp.name

# ---------------------------------------------------------------------------
# Rez output helpers
# ---------------------------------------------------------------------------

def format_hex_rez(data):
    hex_str = data.hex().upper()
    lines = []
    for i in range(0, len(hex_str), 32):
        chunk = hex_str[i:i+32]
        formatted = ' '.join(chunk[j:j+4] for j in range(0, len(chunk), 4))
        lines.append(f'\t$"{formatted}"')
    return '\n'.join(lines)


def write_data_resource(out, rtype, rid, label, data):
    out.write(f"/* {label} */\n")
    out.write(f"data '{rtype}' ({rid}, purgeable) {{\n")
    out.write(format_hex_rez(data))
    out.write("\n};\n\n")


def make_icns_blob(family):
    """Build an icns container from an icon family dict."""
    order = ['ICN#', 'ics#', 'icl4', 'ics4', 'icl8', 'ics8']
    total = 8 + sum(8 + len(family[k]) for k in order if k in family)
    blob = b'icns' + struct.pack('>I', total)
    for k in order:
        if k in family:
            blob += k.encode('mac_roman') + struct.pack('>I', 8 + len(family[k])) + family[k]
    return blob


def write_icon_family(out, label, rid, family):
    for rtype in ('ICN#', 'ics#', 'icl4', 'ics4', 'icl8', 'ics8'):
        write_data_resource(out, rtype, rid, f"{label} {rtype}", family[rtype])
    write_data_resource(out, 'icns', rid, f"{label} icns family", make_icns_blob(family))

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    pict_path    = os.path.join(base, 'icons', 'new-icons', 'app-icon.pct')
    new_icons    = os.path.join(base, 'new-icons')
    output_r     = os.path.join(base, 'rsrc', 'icons.r')

    # ---- App icon: decode PICT via sips, fall back to GIF ----
    app_png = None
    if os.path.exists(pict_path) and os.path.getsize(pict_path) > 512:
        try:
            app_png = pict_to_png(pict_path)
            print(f"Decoded PICT app icon: {pict_path}")
        except Exception as e:
            print(f"WARNING: PICT decode failed ({e}), falling back to GIF")

    if app_png is None:
        fallback = os.path.join(new_icons, 'XMODEM Tool 2.gif')
        app_png = fallback
        print(f"Using fallback app icon: {fallback}")

    print("Building icon families…")
    app_family      = make_icon_family(app_png)
    clean_family    = make_icon_family(os.path.join(new_icons, 'clean.gif'))
    history_family  = make_icon_family(os.path.join(new_icons, 'history.gif'))
    settings_family = make_icon_family(os.path.join(new_icons, 'settings.gif'))

    with open(output_r, 'w') as out:
        out.write("/* Automatically generated icon resources for AltanAI */\n\n")

        # Finder bundle boilerplate
        out.write("/* Application Finder Bundle Info */\n")
        out.write("data 'ALTN' (0) {\n\t$\"416C 7461 6E41 49\"\n};\n\n")
        out.write("resource 'FREF' (128) {\n\t'APPL', 0, \"\"\n};\n\n")
        out.write("resource 'BNDL' (128) {\n\t'ALTN', 0,\n\t{\n")
        out.write("\t\t'FREF',\n\t\t{\n\t\t\t0, 128\n\t\t},\n")
        out.write("\t\t'ICN#',\n\t\t{\n\t\t\t0, 128\n\t\t}\n\t}\n};\n\n")

        # ID 128 – Application icon (from Photoshop 7 PICT)
        write_icon_family(out, "App Icon", 128, app_family)
        print("Written App Icon (ID 128)")

        # ID 129 – Clear / Trash toolbar button (clean.gif)
        write_icon_family(out, "Clear Toolbar Icon", 129, clean_family)
        print("Written Clear Icon (ID 129)")

        # ID 130 – Settings toolbar button (settings.gif)
        write_icon_family(out, "Settings Toolbar Icon", 130, settings_family)
        print("Written Settings Icon (ID 130)")

        # ID 131 – History toolbar button (history.gif)
        write_icon_family(out, "History Toolbar Icon", 131, history_family)
        print("Written History Icon (ID 131)")

    print(f"\nDone! Written to {output_r}")

    # Clean up temp file if we used sips
    if app_png != os.path.join(new_icons, 'XMODEM Tool 2.gif'):
        try:
            os.unlink(app_png)
        except Exception:
            pass


if __name__ == '__main__':
    main()
