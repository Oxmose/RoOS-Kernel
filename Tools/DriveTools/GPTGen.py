import struct
import zlib
import uuid

def create_pmbr():
    pmbr = bytearray(512)
    pmbr[446:462] = struct.pack('<B3sB3sII', 0x00, b'\x00\x02\x00', 0xEE, b'\xFF\xFF\xFF', 1, 0xFFFFFFFF)
    pmbr[510], pmbr[511] = 0x55, 0xAA
    return pmbr

def create_gpt_entry(type_guid, unique_guid, first_lba, last_lba, name=""):
    if last_lba == 0:
        return b'\x00' * 128
    t_guid = uuid.UUID(type_guid).bytes_le
    u_guid = uuid.UUID(unique_guid).bytes_le
    name_bytes = name.encode('utf-16-le').ljust(72, b'\x00')[:72]
    return struct.pack('<16s16sQQQ72s', t_guid, u_guid, first_lba, last_lba, 0, name_bytes)

def generate_gpt_image(filename="disk_gpt.img"):
    DATA_TYPE_GUID = "0fc63daf-8483-4772-8e79-3d69d8477de4"

    # Calculate partition ranges (starting at LBA 34)
    p0, p1, p3, p4, p5, p6, trail = 1024, 256, 2048, 2048, 256, 512, 2048

    s_p0 = 34;   e_p0 = s_p0 + p0 - 1
    s_p1 = e_p0 + 1; e_p1 = s_p1 + p1 - 1
    # P2 left empty
    s_p3 = e_p1 + 1; e_p3 = s_p3 + p3 - 1
    s_p4 = e_p3 + 1; e_p4 = s_p4 + p4 - 1
    s_p5 = e_p4 + 1; e_p5 = s_p5 + p5 - 1
    s_p6 = e_p5 + 1; e_p6 = s_p6 + p6 - 1
    total_sectors = s_p6 + p6 + trail

    disk = bytearray(total_sectors * 512)

    # 1. Build Partition Array Table (LBAs 2 to 33)
    entries = bytearray()
    entries += create_gpt_entry(DATA_TYPE_GUID, str(uuid.uuid4()), s_p0, e_p0, "Part0")
    entries += create_gpt_entry(DATA_TYPE_GUID, str(uuid.uuid4()), s_p1, e_p1, "Part1")
    entries += create_gpt_entry(DATA_TYPE_GUID, "00000000-0000-0000-0000-000000000000", 0, 0, "") # P2 Empty
    entries += create_gpt_entry(DATA_TYPE_GUID, str(uuid.uuid4()), s_p3, e_p3, "Part3")
    entries += create_gpt_entry(DATA_TYPE_GUID, str(uuid.uuid4()), s_p4, e_p4, "Part4")
    entries += create_gpt_entry(DATA_TYPE_GUID, str(uuid.uuid4()), s_p5, e_p5, "Part5")
    entries += create_gpt_entry(DATA_TYPE_GUID, str(uuid.uuid4()), s_p6, e_p6, "Part6")
    entries = entries.ljust(128 * 128, b'\x00')
    crc_array = zlib.crc32(entries) & 0xFFFFFFFF
    disk[1024:1024+len(entries)] = entries  # Write into array slot (LBA 2)

    # 2. Build GPT Header (LBA 1)
    gpt_header = bytearray(struct.pack('<8s4sII4xQQQQ16sQIII',
        b'EFI PART', b'\x00\x00\x01\x00', 92, 0, 1, total_sectors - 1, 34, total_sectors - 1,
        uuid.uuid4().bytes_le, 2, 128, 128, crc_array
    ))
    crc_header = zlib.crc32(gpt_header) & 0xFFFFFFFF
    struct.pack_into('<I', gpt_header, 16, crc_header)
    disk[512:512+92] = gpt_header

    # 3. Add Legacy Protective MBR (LBA 0)
    disk[0:512] = create_pmbr()

    # Write marker helper
    def write_marker(lba, text):
        offset = lba * 512
        b_text = text.encode('ascii')
        disk[offset:offset+len(b_text)] = b_text

    # Embed identity tags into the first sector of each valid partition mapping
    write_marker(s_p0, "PARTITION 0 | GPT")
    write_marker(s_p1, "PARTITION 1 | GPT")
    write_marker(s_p3, "PARTITION 3 | GPT")
    write_marker(s_p4, "PARTITION 4 | GPT")
    write_marker(s_p5, "PARTITION 5 | GPT")
    write_marker(s_p6, "PARTITION 6 | GPT")

    with open(filename, "wb") as f:
        f.write(disk)

    print(f"Generated GPT Image '{filename}' ({total_sectors} sectors). Markers injected.")

if __name__ == "__main__":
    generate_gpt_image()