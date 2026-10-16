import struct

def create_partition_entry(bootable, start_lba, num_sectors, part_type=0x83):
    if num_sectors == 0:
        return b'\x00' * 16
    boot_flag = 0x80 if bootable else 0x00
    ch_start = b'\x00\x01\x01'
    ch_end = b'\xFE\xFF\xFF'
    return struct.pack('<B3sB3sII', boot_flag, ch_start, part_type, ch_end, start_lba, num_sectors)

def generate_mbr_image(filename="disk.img"):
    # Define layout dimensions (sectors)
    p0_len, p1_len, p3_len, trailing = 1024, 256, 2048, 2048
    lba_p0 = 1
    lba_p1 = lba_p0 + p0_len
    lba_p3 = lba_p1 + p1_len
    total_sectors = lba_p3 + p3_len + trailing

    # Initialize complete blank disk array
    disk = bytearray(total_sectors * 512)

    # Compile LBA 0 (MBR Sector)
    mbr = bytearray(512)
    mbr[446:462] = create_partition_entry(True,  lba_p0, p0_len)  # Partition 0
    mbr[462:478] = create_partition_entry(False, lba_p1, p1_len)  # Partition 1
    mbr[478:494] = create_partition_entry(False, 0, 0)            # Partition 2 (Empty)
    mbr[494:510] = create_partition_entry(False, lba_p3, p3_len)  # Partition 3
    mbr[510], mbr[511] = 0x55, 0xAA
    disk[0:512] = mbr

    # Write marker helper
    def write_marker(lba, text):
        offset = lba * 512
        b_text = text.encode('ascii')
        disk[offset:offset+len(b_text)] = b_text

    # Embed identifier strings at each partition's starting LBA
    write_marker(lba_p0, "PARTITION 0 | MBR")
    write_marker(lba_p1, "PARTITION 1 | MBR")
    write_marker(lba_p3, "PARTITION 3 | MBR")

    with open(filename, "wb") as f:
        f.write(disk)

    print(f"Generated MBR Image '{filename}' ({total_sectors} sectors). Markers injected.")

if __name__ == "__main__":
    generate_mbr_image()