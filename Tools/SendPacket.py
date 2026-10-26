from scapy.all import Ether, sendp

sendJubo = False

for i in range(512):
    stringVal = "Hello OSdev Kernel!" + str(i)
    if sendJubo:
        stringVal = "Jubo is the best!" + str(i)
        padding = "A" * (2100 - len(stringVal))
    else:
        padding = "A" * (200 - len(stringVal))
    payload = bytes(stringVal + padding, 'utf-8')
    # Build a raw Ethernet frame
    # Replace destination and source MACs with your hobby kernel's MAC and host TAP MAC
    #frame = Ether(dst="52:54:00:12:34:56", src="4a:57:3a:44:75:96", type=0x0800) / payload
    frame = Ether(dst="08:00:27:0C:D4:17", src="4a:57:3a:44:75:96", type=0x0800) / payload

    # Send on the host TAP interface
    sendp(frame, iface="tap0")