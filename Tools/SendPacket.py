import socket
import struct
import time
import sys

def main():
    # --- CONFIGURATION ---
    INTERFACE = "tap0"         # Change to your target interface (e.g., eth0, wlan0, enp3s0)
    TARGET_PPS = 3000500          # Target packets per second (> 4000)
    TOTAL_FRAME_SIZE = 1500

    # 48-bit MAC Address helper conversion
    def mac_to_bytes(mac_str):
        return bytes(int(x, 16) for x in mac_str.split(':'))

    # Destination and Source details
    # REPLACE with your actual destination MAC and source interface MAC
    dst_mac = mac_to_bytes("08:00:27:0C:D4:17")  # Broadcast or specific target MAC
    src_mac = mac_to_bytes("4a:57:3a:44:75:96")  # Your interface MAC
    ether_type = b'\x88\xb5'                     # IEEE 802.1 Local Experimental EtherType
    header_length = len(dst_mac) + len(src_mac) + len(ether_type)

    payload_length = TOTAL_FRAME_SIZE - header_length
    payload = b'X' * payload_length

    # Assemble the raw binary Ethernet frame once outside the loop
    # Total length: 64 bytes (Minimum valid Ethernet frame)
    frame = dst_mac + src_mac + ether_type + payload

    print(f"[*] Initializing Raw Socket on interface: {INTERFACE}")
    print(f"[*] Target Speed: {TARGET_PPS} Packets Per Second")
    print("[*] Frame size: {0} bytes".format(len(frame)))

    # --- SOCKET SETUP ---
    try:
        # SOCK_RAW demands root privileges on Linux
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
        sock.bind((INTERFACE, 0))
        # Set socket to non-blocking to prevent the OS kernel from stalling the loop execution
        sock.setblocking(False)
    except PermissionError:
        print("[-] Error: This script requires root privileges to open raw sockets.")
        print("    Run again using: sudo python3 fast_sender.py")
        sys.exit(1)
    except Exception as e:
        print(f"[-] Failed to bind to interface {INTERFACE}: {e}")
        sys.exit(1)

    # --- TRANSMISSION LOOP ---
    print("[+] Transmission started. Press Ctrl+C to stop.")

    packet_count = 0
    start_time = time.perf_counter()
    report_interval = 1.0  # Report performance every 1 second
    next_report = start_time + report_interval

    # Time budget per packet to maintain the target PPS
    packet_interval = 1.0 / TARGET_PPS
    next_packet_time = time.perf_counter()

    try:
        while True:
            current_time = time.perf_counter()

            # Trigger transmission when the time slice for the next packet arrives
            if current_time >= next_packet_time:
                try:
                    sock.send(frame)
                    packet_count += 1
                except BlockingIOError:
                    # System buffer is temporarily full, skip to prevent crashing
                    pass

                # Increment schedule to the next precise interval window
                next_packet_time += packet_interval

            # Print accurate real-time metrics every second
            if current_time >= next_report:
                actual_elapsed = current_time - (next_report - report_interval)
                actual_pps = packet_count / actual_elapsed
                print(f" -> Sent: {packet_count} packets | Real-time Speed: {actual_pps:.2f} PPS")

                # Reset counters
                packet_count = 0
                next_report = current_time + report_interval

    except KeyboardInterrupt:
        print("\n[+] Transmission halted cleanly by user.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
