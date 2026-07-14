import os
import struct

from PIL import Image


# Real Mac OS system 4-bit CLUT (color table, resource ID 127, entries 0-15).
# These are the exact RGB values the system uses when displaying 4-bit icon data.
MAC_4BIT_COLORS = [
    (255, 255, 255),  # 0:  White
    (255, 204, 0),    # 1:  Yellow
    (255, 102, 0),    # 2:  Orange
    (221, 0, 0),      # 3:  Red
    (255, 0, 153),    # 4:  Pink
    (102, 0, 153),    # 5:  Purple
    (0, 0, 221),      # 6:  Blue
    (0, 170, 255),    # 7:  Cyan
    (0, 221, 0),      # 8:  Green
    (0, 102, 0),      # 9:  Dark Green
    (102, 51, 0),     # 10: Brown
    (153, 102, 51),   # 11: Tan
    (204, 204, 204),  # 12: Light Gray
    (136, 136, 136),  # 13: Medium Gray
    (68, 68, 68),     # 14: Dark Gray
    (0, 0, 0),        # 15: Black
]


def build_mac_system_8bit_clut():
    """Build the real Mac OS system 8-bit color lookup table (CLUT resource 127).

    Layout:
      indices  0-15:  same 16 colors as the 4-bit palette
      indices 16-231: 6x6x6 RGB cube  (216 entries, 6 levels per channel)
      indices 232-255: 24-step gray ramp
    """
    colors = [(0, 0, 0)] * 256

    # Indices 0-15: the 16 system colors
    for i, rgb in enumerate(MAC_4BIT_COLORS):
        colors[i] = rgb

    # Indices 16-231: 6x6x6 RGB cube
    levels = [0, 51, 102, 153, 204, 255]
    cube_idx = 0
    for r in levels:
        for g in levels:
            for b in levels:
                colors[16 + cube_idx] = (r, g, b)
                cube_idx += 1

    # Indices 232-255: 24-step gray ramp (dark to light)
    for i in range(24):
        gray = i * 255 // 23
        colors[232 + i] = (gray, gray, gray)

    return colors


MAC_8BIT_COLORS = build_mac_system_8bit_clut()


def macbinary_resource_fork(filepath):
    if not os.path.exists(filepath):
        return None
    with open(filepath, 'rb') as f:
        data = f.read()
    if len(data) < 128:
        return None

    data_len = struct.unpack('>I', data[83:87])[0]
    rsrc_len = struct.unpack('>I', data[87:91])[0]
    rsrc_offset = 128 + ((data_len + 127) // 128) * 128
    if rsrc_len <= 0 or rsrc_offset + rsrc_len > len(data):
        return None
    return data[rsrc_offset:rsrc_offset + rsrc_len]

def parse_rsrc_data(data):
    if len(data) < 16:
        return None

    data_offset, map_offset, data_len, map_len = struct.unpack('>IIII', data[:16])
    map_data = data[map_offset:map_offset+map_len]
    if len(map_data) < 28:
        return None

    type_offset, name_offset = struct.unpack('>HH', map_data[24:28])
    num_types_m1, = struct.unpack('>h', map_data[type_offset:type_offset+2])
    num_types = num_types_m1 + 1

    resources = {}
    curr = type_offset + 2
    for i in range(num_types):
        rsrc_type, num_items_m1, offset_to_reference_list = struct.unpack('>4sHH', map_data[curr:curr+8])
        rsrc_type_str = rsrc_type.decode('mac_roman', errors='replace')
        num_items = num_items_m1 + 1

        resources[rsrc_type_str] = {}
        ref_curr = type_offset + offset_to_reference_list
        for j in range(num_items):
            entry = map_data[ref_curr:ref_curr+12]
            rsrc_id, name_offset_in_list, attr = struct.unpack('>hHB', entry[:5])
            data_off_bytes = entry[5:8]
            actual_data_offset, = struct.unpack('>I', b'\x00' + data_off_bytes)

            actual_data_len, = struct.unpack('>I', data[data_offset+actual_data_offset : data_offset+actual_data_offset+4])
            rsrc_data = data[data_offset+actual_data_offset+4 : data_offset+actual_data_offset+4+actual_data_len]
            resources[rsrc_type_str][rsrc_id] = rsrc_data

            ref_curr += 12
        curr += 8

    return resources

def parse_rsrc_fork(filepath):
    if not os.path.exists(filepath):
        return None
    with open(filepath, 'rb') as f:
        data = f.read()
    return parse_rsrc_data(data)

def parse_macbinary_rsrc(filepath):
    data = macbinary_resource_fork(filepath)
    if data is None:
        return None
    return parse_rsrc_data(data)

def format_hex_rez(data):
    # Formats binary data as Rez hex stream, e.g. $"XXXX XXXX ..."
    hex_str = data.hex().upper()
    lines = []
    for i in range(0, len(hex_str), 32):
        chunk = hex_str[i:i+32]
        formatted_chunk = ' '.join(chunk[j:j+4] for j in range(0, len(chunk), 4))
        lines.append(f'\t$"{formatted_chunk}"')
    return '\n'.join(lines)

def rgb_distance(a, b):
    dr = a[0] - b[0]
    dg = a[1] - b[1]
    db = a[2] - b[2]
    rmean = (a[0] + b[0]) / 2
    return (2 + rmean / 256) * dr * dr + 4 * dg * dg + (2 + (255 - rmean) / 256) * db * db

def nearest_index(rgb, palette):
    return min(range(len(palette)), key=lambda i: rgb_distance(rgb, palette[i]))

def image_rgb_pixels(path, size):
    image = Image.open(path).convert('RGB')
    if image.size != size:
        image = image.resize(size, Image.Resampling.NEAREST)
    if hasattr(image, 'get_flattened_data'):
        return list(image.get_flattened_data())
    return list(image.getdata())

def pack_1bit(rows):
    data = bytearray()
    for row in rows:
        for x in range(0, len(row), 8):
            value = 0
            for bit in row[x:x + 8]:
                value = (value << 1) | (1 if bit else 0)
            data.append(value)
    return bytes(data)

def make_bw_icon(pixels, width, height):
    rows = []
    mask_rows = []
    for y in range(height):
        row = []
        mask_row = []
        for x in range(width):
            rgb = pixels[y * width + x]
            luminance = (rgb[0] * 299 + rgb[1] * 587 + rgb[2] * 114) // 1000
            row.append(luminance < 150)
            mask_row.append(True)
        rows.append(row)
        mask_rows.append(mask_row)
    return pack_1bit(rows) + pack_1bit(mask_rows)

def make_8bit_icon(pixels):
    return bytes(nearest_index(rgb, MAC_8BIT_COLORS) for rgb in pixels)

def make_4bit_icon(pixels):
    indexes = [nearest_index(rgb, MAC_4BIT_COLORS) for rgb in pixels]
    data = bytearray()
    for i in range(0, len(indexes), 2):
        data.append((indexes[i] << 4) | indexes[i + 1])
    return bytes(data)

def make_tiff_icon_family(path):
    large = image_rgb_pixels(path, (32, 32))
    small = image_rgb_pixels(path, (16, 16))
    return {
        'ICN#': make_bw_icon(large, 32, 32),
        'ics#': make_bw_icon(small, 16, 16),
        'icl4': make_4bit_icon(large),
        'ics4': make_4bit_icon(small),
        'icl8': make_8bit_icon(large),
        'ics8': make_8bit_icon(small),
    }

def make_icns_data(parts):
    icon_family_len = 8 + sum(8 + len(part_data) for _, part_data in parts)
    icns_data = b'icns' + struct.pack('>I', icon_family_len)
    for part_type, part_data in parts:
        icns_data += part_type.encode('mac_roman') + struct.pack('>I', 8 + len(part_data)) + part_data
    return icns_data

def parse_icns_parts(data):
    if len(data) < 8 or data[:4] != b'icns':
        return []

    parts = []
    offset = 8
    total_len, = struct.unpack('>I', data[4:8])
    total_len = min(total_len, len(data))
    while offset + 8 <= total_len:
        part_type = data[offset:offset + 4].decode('mac_roman', errors='replace')
        part_len, = struct.unpack('>I', data[offset + 4:offset + 8])
        if part_len < 8 or offset + part_len > total_len:
            break
        parts.append((part_type, data[offset + 8:offset + part_len]))
        offset += part_len
    return parts

def icon_family_from_resource_file(path):
    rsrcs = parse_rsrc_fork(path)
    if not rsrcs or 'icns' not in rsrcs:
        return None

    resource_id = -16455
    if resource_id not in rsrcs['icns']:
        resource_id = sorted(rsrcs['icns'])[0]
    icns_data = rsrcs['icns'][resource_id]

    return {
        'icns': icns_data,
        'parts': dict(parse_icns_parts(icns_data)),
    }

def write_data_resource(out, resource_type, resource_id, label, data):
    out.write(f"/* {label} */\n")
    out.write(f"data '{resource_type}' ({resource_id}, purgeable) {{\n")
    out.write(format_hex_rez(data))
    out.write("\n};\n\n")

def write_icon_family(out, label, resource_id, family):
    for resource_type in ('ICN#', 'ics#', 'icl4', 'ics4', 'icl8', 'ics8'):
        write_data_resource(
            out,
            resource_type,
            resource_id,
            f"{label} {resource_type}",
            family[resource_type]
        )

    icns_parts = [(resource_type, family[resource_type])
                  for resource_type in ('ICN#', 'ics#', 'icl4', 'ics4', 'icl8', 'ics8')]
    write_data_resource(out, 'icns', resource_id, f"{label} icns family", make_icns_data(icns_parts))

def write_resource_icon_family(out, label, resource_id, family, family_type='icns'):
    for resource_type in ('ICN#', 'ics#', 'icl4', 'ics4', 'icl8', 'ics8'):
        if resource_type in family['parts']:
            write_data_resource(
                out,
                resource_type,
                resource_id,
                f"{label} {resource_type}",
                family['parts'][resource_type]
            )
    write_data_resource(out, family_type, resource_id, f"{label} family", family['icns'])

def main():
    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    extracted_dir = "/Users/altan/Documents/ambitious/macOS9-AI-Code-Studio/new-icons/extracted/new-icons"
    output_r = os.path.join(base, 'rsrc', 'icons.r')

    # Load families from resource forks
    app_family = icon_family_from_resource_file(os.path.join(extracted_dir, 'app-icon.rsr/..namedfork/rsrc'))
    clear_family = icon_family_from_resource_file(os.path.join(extracted_dir, 'clean.rsr/..namedfork/rsrc'))
    settings_family = icon_family_from_resource_file(os.path.join(extracted_dir, 'settings.rsr/..namedfork/rsrc'))
    history_family = icon_family_from_resource_file(os.path.join(extracted_dir, 'history.rsr/..namedfork/rsrc'))
    sidebar_family = icon_family_from_resource_file(os.path.join(extracted_dir, 'sidebar.rsr/..namedfork/rsrc'))

    with open(output_r, 'w') as out:
        out.write("/* Automatically generated icon resources for AltanAI */\n\n")

        # Write ALTN signature, BNDL, and FREF for app finder icon
        out.write("/* Application Finder Bundle Info */\n")
        out.write("data 'ALTN' (0) {\n\t$\"416C 7461 6E41 49\"\n};\n\n")
        out.write("resource 'FREF' (128) {\n\t'APPL', 0, \"\"\n};\n\n")
        out.write("resource 'BNDL' (128) {\n")
        out.write("\t'ALTN', 0,\n")
        out.write("\t{\n")
        out.write("\t\t'FREF',\n")
        out.write("\t\t{\n")
        out.write("\t\t\t0, 128\n")
        out.write("\t\t},\n")
        out.write("\t\t'ICN#',\n")
        out.write("\t\t{\n")
        out.write("\t\t\t0, 128\n")
        out.write("\t\t}\n")
        out.write("\t}\n")
        out.write("};\n\n")
        
        # 1. App Icon (ID 128) - standard 'icns' type
        if app_family:
            write_resource_icon_family(out, "App Icon", 128, app_family, 'icns')
            print("Extracted App Icon (ID 128)")
        else:
            print("ERROR: Failed to load app-icon.rsr")
            
        # 2. Clear Icon (ID 129) - custom 'CLER' type to prevent collision
        if clear_family:
            write_resource_icon_family(out, "Clear Toolbar Icon", 129, clear_family, 'CLER')
            print("Extracted Clear Icon (ID 129)")
        else:
            print("ERROR: Failed to load clean.rsr")
            
        # 3. Settings Icon (ID 130) - custom 'SETT' type to prevent collision
        if settings_family:
            write_resource_icon_family(out, "Settings Toolbar Icon", 130, settings_family, 'SETT')
            print("Extracted Settings Icon (ID 130)")
        else:
            print("ERROR: Failed to load settings.rsr")
            
        # 4. History Icon (ID 131) - custom 'HIST' type to prevent collision
        if history_family:
            write_resource_icon_family(out, "History Toolbar Icon", 131, history_family, 'HIST')
            print("Extracted History Icon (ID 131)")
        else:
            print("ERROR: Failed to load history.rsr")

        # 5. Sidebar Icon (ID 132) - custom 'SIDE' type
        if sidebar_family:
            write_resource_icon_family(out, "Sidebar Toolbar Icon", 132, sidebar_family, 'SIDE')
            print("Extracted Sidebar Icon (ID 132)")

if __name__ == '__main__':
    main()
