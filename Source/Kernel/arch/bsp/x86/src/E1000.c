/*******************************************************************************
 * @file E1000.c
 *
 * @see E1000.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/10/2026
 *
 * @version 1.0
 *
 * @brief Kernel's E1000 (Ethernet Controller) driver.
 *
 * @details Kernel's E1000 (Ethernet Controller) driver. Defines
 * the functions and definitions to interact with the E1000
 * controllers and their devices.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <IOCTL.h>
#include <Panic.h>
#include <errno.h>
#include <X64Cpu.h>
#include <IOAPIC.h>
#include <stdint.h>
#include <stddef.h>
#include <Memory.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <VirtualFS.h>
#include <Scheduler.h>
#include <PCIExpress.h>
#include <NetManager.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <KernelQueue.h>
#include <TimerManager.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <E1000.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "X86 E1000"

/** @brief Intel vendor ID. */
#define INTEL_VENDOR_ID 0x8086

/** @brief 82544E Copper Network Controllers */
#define E1000_DEV_ID_82544EI_COPPER 0x1008
/** @brief 82544E Fiber Network Controllers */
#define E1000_DEV_ID_82544EI_FIBER 0x1009
/** @brief 82544G Copper Network Controllers */
#define E1000_DEV_ID_82544GC_COPPER 0x100C
/** @brief 82544G LOM Network Controllers */
#define E1000_DEV_ID_82544GC_LOM 0x100D
/** @brief 82540EM Network Controllers  */
#define E1000_DEV_ID_82540EM 0x100E
/** @brief 82540EM LOM Network Controllers  */
#define E1000_DEV_ID_82540EM_LOM 0x1015
/** @brief 82540EP Network Controllers  */
#define E1000_DEV_ID_82540EP 0x1017
/** @brief 82540EP LOM Network Controllers  */
#define E1000_DEV_ID_82540EP_LOM 0x1016
/** @brief 82540EP Low Power Network Controllers  */
#define E1000_DEV_ID_82540EP_LP 0x101E
/** @brief 82545EM Copper Network Controllers */
#define E1000_DEV_ID_82545EM_COPPER 0x100F
/** @brief 82545EM Fiber Network Controllers */
#define E1000_DEV_ID_82545EM_FIBER 0x1011
/** @brief 82545GM Copper Network Controllers */
#define E1000_DEV_ID_82545GM_COPPER 0x1026
/** @brief 82545GM Fiber Network Controllers */
#define E1000_DEV_ID_82545GM_FIBER 0x1027
/** @brief 82545GM SerDes Network Controllers */
#define E1000_DEV_ID_82545GM_SERDES 0x1028
/** @brief 82546EB Copper Network Controllers */
#define E1000_DEV_ID_82546EB_COPPER 0x1010
/** @brief 82546EB Fiber Network Controllers */
#define E1000_DEV_ID_82546EB_FIBER 0x1012
/** @brief 82546EB Quad Copper Network Controllers */
#define E1000_DEV_ID_82546EB_QUAD_COP 0x101D
/** @brief 82546GB Copper Network Controllers */
#define E1000_DEV_ID_82546GB_COPPER 0x1079
/** @brief 82546GB Fiber Network Controllers */
#define E1000_DEV_ID_82546GB_FIBER 0x107A
/** @brief 82546GB SerDes Network Controllers */
#define E1000_DEV_ID_82546GB_SERDES 0x107B
/** @brief 82546GB PCIe Network Controllers */
#define E1000_DEV_ID_82546GB_PCIE 0x108A
/** @brief 82546GB Quad Copper Network Controllers */
#define E1000_DEV_ID_82546GB_QUAD_COP 0x1099
/** @brief 82541EI Network Controllers */
#define E1000_DEV_ID_82541EI 0x1013
/** @brief 82541EI Mobile Network Controllers */
#define E1000_DEV_ID_82541EI_MOBILE 0x1018
/** @brief 82541ER Network Controllers */
#define E1000_DEV_ID_82541ER 0x1078
/** @brief 82541GI Network Controllers */
#define E1000_DEV_ID_82541GI 0x1076
/** @brief 82541GI Mobile Network Controllers */
#define E1000_DEV_ID_82541GI_MOBILE 0x1077
/** @brief 82541PI Network Controllers */
#define E1000_DEV_ID_82541PI 0x107C
/** @brief 82547EI/GI Network Controllers */
#define E1000_DEV_ID_82547EIGI 0x1019
/** @brief 82547EI Mobile Network Controllers */
#define E1000_DEV_ID_82547EI_MOBILE 0x101A

/** @brief Device Control Register */
#define E1000_CTRL 0x00000
/** @brief Device Status Register */
#define E1000_STATUS 0x00008
/** @brief EEPROM/Flash Control Register */
#define E1000_EECD 0x00010
/** @brief EEPROM Read Register */
#define E1000_EERD 0x00014
/** @brief Control Extension Register */
#define E1000_CTRL_EXT 0x00018
/** @brief Flashing Protection Register */
#define E1000_FLA 0x0001C
/** @brief Media Independent Interface Control Register */
#define E1000_MDIC 0x00020
/** @brief Fiber Channel Receive Flow Control Asynchronous Time Register */
#define E1000_FCAL 0x00028
/** @brief Fiber Channel Transmit Flow Control Asynchronous Time Register */
#define E1000_FCAH 0x0002C
/** @brief Fiber Channel Type Register */
#define E1000_FCT 0x00030
/** @brief VET Packet Type Register */
#define E1000_VET 0x00038
/** @brief Flow Control Transmit Timer Value Register */
#define E1000_FCTTV 0x00170
/** @brief Transmit Configuration Word Register */
#define E1000_TXCW 0x00178
/** @brief Receive Configuration Word Register */
#define E1000_RXCW 0x00180
/** @brief LED Control Register */
#define E1000_LEDCTL 0x00E00

/* --- DMA Registers --- */
/** @brief Packet Buffer Allocation Register */
#define E1000_PBA 0x01000

/* --- Interrupt Registers --- */
/** @brief Interrupt Cause Read Register */
#define E1000_ICR 0x000C0
/** @brief Interrupt Throtling Register */
#define E1000_ITR 0x000C4
/** @brief Interrupt Cause Set Register */
#define E1000_ICS 0x000C8
/** @brief Interrupt Mask Set/Read Register */
#define E1000_IMS 0x000D0
/** @brief Interrupt Mask Clear Register */
#define E1000_IMC 0x000D8

/* --- Receive Registers --- */
/** @brief Receive Control Register */
#define E1000_RCTL 0x00100
/** @brief RX Flow Control High Watermark Register */
#define E1000_FCRTL 0x02160
/** @brief RX Flow Control Low Watermark Register */
#define E1000_FCRTH 0x02168
/** @brief Receive Descriptor Base Address Low Register */
#define E1000_RDBAL 0x02800
/** @brief Receive Descriptor Base Address High Register */
#define E1000_RDBAH 0x02804
/** @brief Receive Descriptor Length Register */
#define E1000_RDLEN 0x02808
/** @brief Receive Descriptor Head Register */
#define E1000_RDH 0x02810
/** @brief Receive Descriptor Tail Register */
#define E1000_RDT 0x02818
/** @brief Receive Delay Timer Register */
#define E1000_RDTR 0x02820
/** @brief Receive Interrupt Packet Delay Timer Register */
#define E1000_RADV 0x0282C
/** @brief Receive Small Packet Detect Interrupt Register */
#define E1000_RSRPD 0x02C00

/* --- Transmit Registers --- */
/** @brief Transmit Control Register */
#define E1000_TCTL 0x00400
/** @brief Transmit IPG Register */
#define E1000_TIPG 0x00410
/** @brief Adaptive IFS Throttle - AIT Register */
#define E1000_AIFS 0x00458
/** @brief Transmit Descriptor Base Address Low Register */
#define E1000_TDBAL 0x03800
/** @brief Transmit Descriptor Base Address High Register */
#define E1000_TDBAH 0x03804
/** @brief Transmit Descriptor Length Register */
#define E1000_TDLEN 0x03808
/** @brief Transmit Descriptor Head Register */
#define E1000_TDH 0x03810
/** @brief Transmit Descriptor Tail Register */
#define E1000_TDT 0x03818
/** @brief Transmit Interrupt Delay Value Register */
#define E1000_TIDV 0x03820

/* --- Transmit DMA Registers --- */
/** @brief Transmit DMA Control Register */
#define E1000_TXDMAC 0x03000
/** @brief Transmit Descriptor Control Register */
#define E1000_TXDCTL 0x03828
/** @brief Transmit Absolute Interrupt Delay Timer Register */
#define E1000_TADV 0x0382C
/** @brief TCP Segmentation Pad and Threshold  Register */
#define E1000_TSPMT 0x03830

/* --- Receive DMA Registers --- */
/** @brief Receive Descriptor Control Register */
#define E1000_RXDCTL 0x02828
/** @brief Receive Checksum Control  Register */
#define E1000_RXCSUM 0x05000

/* --- Address Filtering & Tables --- */
/** @brief Multicast Table Array (Spans 0x05200 - 0x053FC) */
#define E1000_MTA 0x05200
/** @brief Receive Address Low Array (Spans 0x05400 - 0x05478) */
#define E1000_RAL 0x05400
/** @brief Receive Address High Array (Spans 0x05404 - 0x0547C) */
#define E1000_RAH 0x05404
/** @brief VLAN Filter Table Array (Spans 0x05600 - 0x057FC) */
#define E1000_VFTA 0x05600

/* --- Management & Power Configuration Registers --- */
/** @brief Wakeup Control Register */
#define E1000_WUC 0x05800
/** @brief Wakeup Filter Control Register */
#define E1000_WUFC 0x05808
/** @brief Wakeup Status Register */
#define E1000_WUS 0x05810
/** @brief Host IPMI Receive IP Address Address */
#define E1000_IPAV 0x05838
/** @brief IPv4 Address Table Array */
#define E1000_IP4AT 0x05840
/** @brief IPv6 Address Table Array */
#define E1000_IP6AT 0x05880
/** @brief Wakeup Packet Length Register */
#define E1000_WUPL 0x05900
/** @brief Wakeup Packet Memory Array */
#define E1000_WUPM 0x05A00

/* --- Wake Up Flexible Pattern Filters --- */
/** @brief Flexible Filter Length Table Array (Spans 0x05F00 - 0x05F18) */
#define E1000_FFLT 0x05F00
/** @brief Flexible Filter Mask Table Array (Spans 0x09000 - 0x093F8) */
#define E1000_FFMT 0x09000
/** @brief Flexible Filter Value Table Array (Spans 0x09800 - 0x09BF8) */
#define E1000_FFVT 0x09800

/* --- Hardware Statistics Counters (0x04000 - 0x04124) --- */
/** @brief CRC Error Count Register */
#define E1000_CRCERRS 0x04000
/** @brief Alignment Error Count Register */
#define E1000_ALGNERRC 0x04004
/** @brief Symbol Error Count Register */
#define E1000_SYMERRC 0x04008
/** @brief RX Error Count Register */
#define E1000_RXERRC 0x0400C
/** @brief Missed Packets Count Register */
#define E1000_MPC 0x04010
/** @brief Single Collision Count Register */
#define E1000_SCC 0x04014
/** @brief Excessive Collisions Count Register */
#define E1000_ECOL 0x04018
/** @brief Multiple Collision Count Register */
#define E1000_MCC 0x0401C
/** @brief Late Collisions Count Register */
#define E1000_LCOL 0x04020
/** @brief Collision Count Register */
#define E1000_COLC 0x04028
/** @brief Defer Count Register */
#define E1000_DC 0x04030
/** @brief Transmit Underrun Count Register */
#define E1000_TNCRS 0x04034
/** @brief Sequence Error Count Register */
#define E1000_SEC 0x04038
/** @brief Receive Length Error Count Register */
#define E1000_RLEC 0x04040
/** @brief Receive Fragment Count Register */
#define E1000_XONRXC 0x04048
/** @brief Transmit Fragment Count Register */
#define E1000_XONTXC 0x0404C
/** @brief Receive Jabber Count Register */
#define E1000_XOFFRXC 0x04050
/** @brief Transmit Jabber Count Register */
#define E1000_XOFFTXC 0x04054
/** @brief FC Received Unsupported Count Register */
#define E1000_FCRUC 0x04058
/** @brief Receive Packets 64 Bytes Count Register */
#define E1000_PRC64 0x0405C
/** @brief Receive Packets 65-127 Bytes Count Register */
#define E1000_PRC127 0x04060
/** @brief Receive Packets 128-255 Bytes Count Register */
#define E1000_PRC255 0x04064
/** @brief Receive Packets 256-511 Bytes Count Register */
#define E1000_PRC511 0x04068
/** @brief Receive Packets 512-1023 Bytes Count Register */
#define E1000_PRC1023 0x0406C
/** @brief Receive Packets 1024-1522 Bytes Count Register */
#define E1000_PRC1522 0x04070
/** @brief Good Packets Received Count Register */
#define E1000_GPRC 0x04074
/** @brief Broadcast Packets Received Count Register */
#define E1000_BPRC 0x04078
/** @brief Multicast Packets Received Count Register */
#define E1000_MPRC 0x0407C
/** @brief Good Packets Transmitted Count Register */
#define E1000_GPTC 0x04080
/** @brief Good Octets Received Low Register */
#define E1000_GORCL 0x04088
/** @brief Good Octets Received High Register */
#define E1000_GORCH 0x0408C
/** @brief Good Octets Transmitted Low Register */
#define E1000_GOTCL 0x04090
/** @brief Good Octets Transmitted High Register */
#define E1000_GOTCH 0x04094
/** @brief Receive No Buffers Count Register */
#define E1000_RNBC 0x040A0
/** @brief Receive Undersize Count Register */
#define E1000_RUC 0x040A4
/** @brief Receive Fragment Count Register */
#define E1000_RFC 0x040A8
/** @brief Receive Jabber Count Register */
#define E1000_ROC 0x040AC
/** @brief Receive Total Packets Count Register */
#define E1000_RJC 0x040B0
/** @brief Management Packets Received Count Register */
#define E1000_MGTPRC 0x040B4
/** @brief Management Packets Dropped Count Register */
#define E1000_MGTPDC 0x040B8
/** @brief Management Packets Transmitted Count Register */
#define E1000_MGTPTC 0x040BC
/** @brief Total Octets Received Low Register */
#define E1000_TORL 0x040C0
/** @brief Total Octets Received High Register */
#define E1000_TORH 0x040C4
/** @brief Total Octets Transmitted Low Register */
#define E1000_TOTL 0x040C8
/** @brief Total Octets Transmitted High Register */
#define E1000_TOTH 0x040CC
/** @brief Total Packets Received Count Register */
#define E1000_TPR 0x040D0
/** @brief Total Packets Transmitted Count Register */
#define E1000_TPT 0x040D3
/** @brief Trans packets 64 Bytes Count Register */
#define E1000_PTC64 0x040D8
/** @brief Trans Packets 65-127 Bytes Count Register */
#define E1000_PTC127 0x040DC
/** @brief Trans Packets 128-255 Bytes Count Register */
#define E1000_PTC255 0x040E0
/** @brief Trans Packets 256-511 Bytes Count Register */
#define E1000_PTC511 0x040E4
/** @brief Trans Packets 512-1023 Bytes Count Register */
#define E1000_PTC1023 0x040E8
/** @brief Trans Packets 1024-1522 Bytes Count Register */
#define E1000_PTC1522 0x040EC
/** @brief Multicast Packets Transmitted Count Register */
#define E1000_MPTC 0x040F0
/** @brief Broadcast Packets Transmitted Count Register */
#define E1000_BPTC 0x040F4
/** @brief TSCO Packet Count Register */
#define E1000_TSCTC 0x040F8
/** @brief TSCO Octet Count Register */
#define E1000_TSCTFC 0x040FC

/* --- Diagnostic Registers --- */
/** @brief Receive Data FIFO Depth Register */
#define E1000_RDFH 0x02410
/** @brief Receive Data FIFO Tail Register */
#define E1000_RDFT 0x02418
/** @brief Receive Data FIFO Packet Length Register */
#define E1000_RDFHS 0x02420
/** @brief Receive Data FIFO Tail Saved Register */
#define E1000_RDFTS 0x02428
/** @brief Receive Data FIFO Packet Count Register */
#define E1000_RDFPC 0x02430
/** @brief Transmit Data FIFO Head Register */
#define E1000_TDFH 0x03410
/** @brief Transmit Data FIFO Tail Register */
#define E1000_TDFT 0x03418
/** @brief Transmit Data FIFO Packet Length Register */
#define E1000_TDFHS 0x03420
/** @brief Transmit Data FIFO Control Register */
#define E1000_TDFTS 0x03428
/** @brief Transmit Data FIFO Packet Count Register */
#define E1000_TDFPC 0x03430
/** @brief Packet Buffer Status Register */
#define E1000_PGM 0x10000

/* --- Standard IEEE 802.3 Clause 22 Registers (0 - 15) --- */
/** @brief PHY Control Register */
#define E1000_PHY_CTRL 0x00
/** @brief PHY Status Register*/
#define E1000_PHY_STATUS 0x01
/** @brief PHY Identifier Register 1  */
#define E1000_PHY_ID1 0x02
/** @brief PHY Identifier Register 2 */
#define E1000_PHY_ID2 0x03
/** @brief Auto-Negotiation Advertisement Register */
#define E1000_PHY_AUTONEG_ADV 0x04
/** @brief Auto-Negotiation Link Partner Ability Register */
#define E1000_PHY_LP_ABILITY 0x05
/** @brief Auto-Negotiation Expansion Register */
#define E1000_PHY_AUTONEG_EXP 0x06
/** @brief Auto-Negotiation Next Page Transmit Register */
#define E1000_PHY_NEXT_PAGE_TX 0x07
/** @brief Auto-Negotiation Link Partner Next Page Register */
#define E1000_PHY_LP_NEXT_PAGE 0x08
/** @brief 1000BASE-T Control Register */
#define E1000_PHY_1000T_CTRL 0x09
/** @brief 1000BASE-T Status Register */
#define E1000_PHY_1000T_STATUS 0x0A
/** @brief Extended Status Register */
#define E1000_PHY_EXT_STATUS 0x0F

/* --- Intel/Vendor Specific Extended Registers (16 - 31) --- */
/** @brief PHY Specific Control Register*/
#define E1000_PHY_SPEC_CTRL 0x10
/** @brief PHY Specific Status Register*/
#define E1000_PHY_SPEC_STATUS 0x11
/** @brief Interrupt Enable Register */
#define E1000_PHY_INT_ENABLE 0x12
/** @brief Interrupt Status Register */
#define E1000_PHY_INT_STATUS 0x13
/** @brief Extended PHY Specific Control Register */
#define E1000_PHY_EXT_SPEC_CTRL 0x14
/** @brief Receive Error Counter Register */
#define E1000_PHY_RX_ERR_CNTR 0x15
/** @brief Global Status Register */
#define E1000_PHY_PGSTAT 0x17
/** @brief Manual LED Override Register */
#define E1000_PHY_LED_OVERRIDE 0x18
/** @brief Reserved Extended Core Engine Target */
#define E1000_PHY_RES_19 0x19
/** @brief Extended PHY Specific Control 2 Register */
#define E1000_PHY_EPSCON2 0x1A
/** @brief Extended PHY Specific Control 2 Register */
#define E1000_PHY_EXT_SPEC_CTRL2 0x1A
/** @brief Extended PHY Specific Status Register */
#define E1000_PHY_EPSSTAT 0x1B
/** @brief Register 30 Page Select  Register */
#define E1000_PHY_R30PS 0x1D
/** @brief Register 30 Access Window */
#define E1000_PHY_R30AW 0x1E
/** @brief Mode Control / Page Select Register */
#define E1000_PHY_PAGE_SELECT 0x1F

/* --- Device Control Register Bits --- */
/** @brief Full Duplex Enable */
#define E1000_CTRL_FD (1 << 0)
/** @brief Auto-Speed Detection Enable */
#define E1000_CTRL_ASDE (1 << 5)
/** @brief Speed Link Up */
#define E1000_CTRL_SLU (1 << 6)
/** @brief Speed Selection mask */
#define E1000_CTRL_SPD_SEL (3 << 8)
/** @brief Speed Selection 10 Mbps */
#define E1000_CTRL_SPD_SEL_10 (0 << 8)
/** @brief Speed Selection 100 Mbps */
#define E1000_CTRL_SPD_SEL_100 (1 << 8)
/** @brief Speed Selection 1000 Mbps */
#define E1000_CTRL_SPD_SEL_1000 (2 << 8)
/** @brief Force Speed Selection */
#define E1000_CTRL_FRCSPEED (1 << 11)
/** @brief Force Duplex Selection */
#define E1000_CTRL_FRCDPLX (1 << 12)
/** @brief Reset control bit */
#define E1000_CTRL_RST (1 << 26)
/** @brief E1000 CTRL Register RFCE (Receive Flow Control Enable) bit */
#define E1000_CTRL_RFCE (1 << 27)
/** @brief E1000 CTRL Register TFCE (Transmit Flow Control Enable) bit */
#define E1000_CTRL_TFCE (1 << 28)

/* --- Device PBA Register Bits --- */
/** @brief E1000 PBA (Packet Buffer Allocation) Register value mask */
#define E1000_PBA_RX_MASK 0x0000FFFF
/** @brief E1000 PBA (Packet Buffer Allocation) Register value shift */
#define E1000_PBA_RX_SHIFT 0

/* --- Receive Control Registers --- */
/** @brief Receive Control Register: Enable */
#define E1000_RCTL_EN (1 << 1)
/** @brief Receive Control Register: Store Bad Packets */
#define E1000_RCTL_SBP (1 << 2)
/** @brief Receive Control Register: Unicast Promiscuous Enable */
#define E1000_RCTL_UPE (1 << 3)
/** @brief Receive Control Register: Multicast Promiscuous Enable */
#define E1000_RCTL_MPE (1 << 4)
/** @brief Receive Control Register: Long Packet Enable */
#define E1000_RCTL_LPE (1 << 5)
/** @brief Receive Control Register: Loopback Mode - None */
#define E1000_RCTL_LBM_NONE (0 << 6)
/** @brief Receive Control Register: Loopback Mode - PHY */
#define E1000_RCTL_LBM_PHY (3 << 6)
/** @brief Receive Control Register: Receive Descriptor Minimum Threshold 2/8 */
#define E1000_RCTL_RDMTS_2 (0 << 8)
/** @brief Receive Control Register: Receive Descriptor Minimum Threshold 4/8 */
#define E1000_RCTL_RDMTS_4 (1 << 8)
/** @brief Receive Control Register: Receive Descriptor Minimum Threshold 8/8 */
#define E1000_RCTL_RDMTS_8 (2 << 8)
/** @brief Receive Control Register: Multicast Offset [47:36] */
#define E1000_RCTL_MO_36 (0 << 12)
/** @brief Receive Control Register: Multicast Offset [46:35] */
#define E1000_RCTL_MO_35 (1 << 12)
/** @brief Receive Control Register: Multicast Offset [45:34] */
#define E1000_RCTL_MO_34 (2 << 12)
/** @brief Receive Control Register: Multicast Offset [43:32] */
#define E1000_RCTL_MO_32 (3 << 12)
/** @brief Receive Control Register: Broadcast Accept Mode */
#define E1000_RCTL_BAM (1 << 15)
/** @brief Receive Control Register: Receive Buffer Size 2048 */
#define E1000_RCTL_BSIZE_2048 (0 << 16)
/** @brief Receive Control Register: Receive Buffer Size 1024 */
#define E1000_RCTL_BSIZE_1024 (1 << 16)
/** @brief Receive Control Register: Receive Buffer Size 512 */
#define E1000_RCTL_BSIZE_512 (2 << 16)
/** @brief Receive Control Register: Receive Buffer Size 256 */
#define E1000_RCTL_BSIZE_256 (3 << 16)
/** @brief Receive Control Register: Receive Buffer Size 16384 */
#define E1000_RCTL_BSIZE_16384 (1 << 16)
/** @brief Receive Control Register: Receive Buffer Size 8192 */
#define E1000_RCTL_BSIZE_8192 (2 << 16)
/** @brief Receive Control Register: Receive Buffer Size 4096 */
#define E1000_RCTL_BSIZE_4096 (3 << 16)
/** @brief Receive Control Register: VLAN Filter Enable */
#define E1000_RCTL_VFE (1 << 18)
/** @brief Receive Control Register: Canonical Form Indicator Enable */
#define E1000_RCTL_CFIEN (1 << 19)
/** @brief Receive Control Register: Canonical Form Indicator */
#define E1000_RCTL_CFI (1 << 20)
/** @brief Receive Control Register: Discard Pause Frames */
#define E1000_RCTL_DPF (1 << 22)
/** @brief Receive Control Register: Pass MAC Control Frames */
#define E1000_RCTL_PMCF (1 << 23)
/** @brief Receive Control Register: Buffer Size Extension */
#define E1000_RCTL_BSEX (1 << 25)
/** @brief Receive Control Register: Strip Ethernet CRC */
#define E1000_RCTL_SECRC (1 << 26)

/* --- Receive Descriptor Status Bits --- */
/** @brief Receive Descriptor Status: Descriptor Done */
#define E1000_RXD_STAT_DD (1 << 0)
/** @brief Receive Descriptor Status: End of Packet */
#define E1000_RXD_STAT_EOP (1 << 1)

/* --- Transmit Control Registers --- */
/** @brief Transmit Control Register: Enable */
#define E1000_TCTL_EN (1 << 1)
/** @brief Transmit Control Register: Pad Short Packets */
#define E1000_TCTL_PSP (1 << 3)
/** @brief Transmit Control Register: Collision Threshold */
#define E1000_TCTL_CT_SHIFT 4
/** @brief Transmit Control Register: Collision Distance */
#define E1000_TCTL_COLD_SHIFT 12
/** @brief Transmit Control Register: Software XOFF */
#define E1000_TCTL_SWXOFF (1 << 22)
/** @brief Transmit Control Register: Re-transmit on Late Collision */
#define E1000_TCTL_RTLC (1 << 24)
/** @brief Transmit Control Register: No Re-transmit on underrun */
#define E1000_TCTL_NRTLC (1 << 25)
/** @brief Transmit Control Register: Collision Threshold for Ethernet */
#define E1000_TCTL_CT_ETHER 0x10
/** @brief Transmit Control Register: Collision Distance for Full Duplex */
#define E1000_TCTL_COLD_FULL 0x40
/** @brief Transmit Control Register: Collision Distance for Half Duplex */
#define E1000_TCTL_COLD_HALF 0x200

/** @brief Transmit Descriptor Command: End of Packet */
#define E1000_TX_CMD_EOP ((1 << 0) << 24)
/** @brief Transmit Descriptor Command: Insert FCS */
#define E1000_TX_CMD_IFCS ((1 << 1) << 24)
/** @brief Transmit Descriptor Command: Insert Checksum */
#define E1000_TX_CMD_IC ((1 << 2) << 24)
/** @brief Transmit Descriptor Command: Report Status */
#define E1000_TX_CMD_RS ((1 << 3) << 24)
/** @brief Transmit Descriptor Command: Report Packet Sent */
#define E1000_TX_CMD_RPS ((1 << 4) << 24)
/** @brief Transmit Descriptor Command: Descriptor Extension */
#define E1000_TX_CMD_DEXT ((1 << 5) << 24)
/** @brief Transmit Descriptor Command: VLAN Packet Enable */
#define E1000_TX_CMD_VLE ((1 << 6) << 24)
/** @brief Transmit Descriptor Command: Interrupt Delay Enabl */
#define E1000_TX_CMD_IDE ((1 << 7) << 24)

/* --- Transmit Descriptor Status Bits --- */
/** @brief Transmit Descriptor Status: Descriptor Done */
#define E1000_TXD_STAT_DD (1 << 0)

/* --- Interrupt Control Registers --- */
/** @brief Interrupt Control Register: Transmit Descriptor Written Back */
#define E1000_ICR_TXDW (1 << 0)
/** @brief Interrupt Control Register: Transmit Queue Empty */
#define E1000_ICR_TXQE (1 << 1)
/** @brief Interrupt Control Register: Link Status Change */
#define E1000_ICR_LSC (1 << 2)
/** @brief Interrupt Control Register: Receive Sequence Error */
#define E1000_ICR_RXSEQ (1 << 3)
/** @brief Interrupt Control Register: Receive Descriptor Minimum Threshold */
#define E1000_ICR_RXDMT0 (1 << 4)
/** @brief Interrupt Control Register: Receive Overrun */
#define E1000_ICR_RXO (1 << 6)
/** @brief Interrupt Control Register: Receive Timer Interrupt */
#define E1000_ICR_RXT0 (1 << 7)
/** @brief Interrupt Control Register: MDI/O Access Complete */
#define E1000_ICR_MDAC (1 << 9)
/** @brief Interrupt Control Register: Receiving /C/ ordered sets */
#define E1000_ICR_RXCFG (1 << 10)
/** @brief Interrupt Control Register: PHY Interrupt */
#define E1000_ICR_PHYINT (1 << 12)
/** @brief Interrupt Control Register: General Purpose Interrupts */
#define E1000_ICR_GPI (2 << 13)
/** @brief Interrupt Control Register: Transmit Descriptor Low Threshold hit */
#define E1000_ICR_TXD_LOW (1 << 15)
/** @brief Interrupt Control Register: Small Receive Packet Detection */
#define E1000_ICR_SRPD (1 << 16)

/** @brief Interrupt Mask Set/Read Register: Transmit Descriptor Written Back */
#define E1000_IMS_TXDW (1 << 0)
/** @brief Interrupt Mask Set/Read Register: Transmit Queue Empty */
#define E1000_IMS_TXQE (1 << 1)
/** @brief Interrupt Mask Set/Read Register: Link Status Change */
#define E1000_IMS_LSC (1 << 2)
/** @brief Interrupt Mask Set/Read Register: Receive Sequence Error */
#define E1000_IMS_RXSEQ (1 << 3)
/**
 * @brief Interrupt Mask Set/Read Register: Receive Descriptor Minimum Threshold
 * hit
 */
#define E1000_IMS_RXDMT0 (1 << 4)
/** @brief Interrupt Mask Set/Read Register: Receive Overrun */
#define E1000_IMS_RXO (1 << 6)
/** @brief Interrupt Mask Set/Read Register: Receive Timer Interrupt */
#define E1000_IMS_RXT0 (1 << 7)
/** @brief Interrupt Mask Set/Read Register: MDI/O Access Complete  */
#define E1000_IMS_MDAC (1 << 9)
/** @brief Interrupt Mask Set/Read Register: Receiving /C/ ordered sets */
#define E1000_IMS_RXCFG (1 << 10)
/** @brief Interrupt Mask Set/Read Register: PHY Interrupt */
#define E1000_IMS_PHYINT (1 << 12)
/** @brief Interrupt Mask Set/Read Register: General Purpose Interrupts */
#define E1000_IMS_GPI (2 << 13)
/**
 * @brief Interrupt Mask Set/Read Register: Transmit Descriptor Low Threshold
 * hit
 */
#define E1000_IMS_TXD_LOW (1 << 15)
/** @brief Interrupt Mask Set/Read Register: Small Receive Packet Detection */
#define E1000_IMS_SRPD (1 << 16)

/** @brief EEPROM Read Status Flags: Done for Microwire */
#define E1000_EERD_MICROWIRE_DONE (1 << 1)
/** @brief EEPROM Read Status Flags: Address Shift for Microwire */
#define E1000_EERD_MICROWIRE_ADDR_SHIFT 2

/** @brief EEPROM Read Status Flags: Done for SPI */
#define E1000_EERD_SPI_DONE (1 << 4)
/** @brief EEPROM Read Status Flags: Address Shift for SPI */
#define E1000_EERD_SPI_ADDR_SHIFT 8

/** @brief EEPROM Read Status Flags: Data Shift */
#define E1000_EERD_DATA_SHIFT 16
/** @brief EEPROM Read Status Flags: Start */
#define E1000_EERD_START (1 << 0)

/* --- Control Extension Register Bits --- */
/** @brief Control Extension Register: Link Mode Mask */
#define E1000_CTRL_EXT_LINK_MODE_MASK   (3 << 22)
/** @brief Control Extension Register: Link Mode Direct */
#define E1000_CTRL_EXT_LINK_MODE_DIRECT (1 << 23)

/* --- Transmit Configuration Word Register Bits --- */
/** @brief Transmit Configuration Word Register: Full Duplex */
#define E1000_TXCW_FD  (1 << 5)
/** @brief Transmit Configuration Word Register: RX Pause */
#define E1000_TXCW_PAUSE (1 << 7)
/** @brief Transmit Configuration Word Register: Asymmetric Pause */
#define E1000_TXCW_ASYM_PD (1 << 8)
/** @brief Transmit Configuration Word Register: Link Up */
#define E1000_TXCW_TXC (1 << 30)
/** @brief Transmit Configuration Word Register: Auto-Negotiation Enable */
#define E1000_TXCW_ANE (1 << 31)

/* --- Status Register Bits --- */
/** @brief Status Register: Link Up */
#define E1000_STATUS_LU (1 << 1)

/* --- MDIO Register Values --- */
/** @brief MDIO Data Mask */
#define E1000_MDIO_DATA_MASK 0xFFFF
/** @brief MDIO Address Shift */
#define E1000_MDIO_ADDR_SHIFT 16
/** @brief MDIO PHY Shift */
#define E1000_MDIO_PHY_SHIFT 21
/** @brief MDIO Write Command */
#define E1000_MDIO_WRITE (1 << 26)
/** @brief MDIO Read Command */
#define E1000_MDIO_READ (1 << 27)
/** @brief MDIO Done (Ready) Flag */
#define E1000_MDIO_DONE (1 << 28)
/** @brief MDIO Error Flag */
#define E1000_MDIO_ERROR (1 << 30)

/* --- RX Checksum Offload --- */
/** @brief E1000 RX Checksum Offload for IPv4 */
#define E1000_RXCSUM_IPOFL (1 << 8)
/** @brief E1000 RX Checksum Offload for TCP/UDP */
#define E1000_RXCSUM_TUOFL (1 << 9)
/** @brief E1000 RX Checksum Offload for IPv6 */
#define E1000_RXCSUM_IPV6OFL (1 << 10)
/** @brief E1000 RX Checksum Clear Mask */
#define E1000_RXCSUM_CLEAR_MASK (E1000_RXCSUM_IPOFL | \
                                 E1000_RXCSUM_TUOFL | \
                                 E1000_RXCSUM_IPV6OFL)

/* --- Initialization Values --- */
/** @brief Default auto-negotiation mask */
#define E1000_AUTONEG_MASK 0x002F
/** @brief Default interrupt throttle value */
#define E1000_INT_THROTTLE_INIT 0
/** @brief Default RX interrupt delay value */
#define E1000_RX_INT_DELAY_INIT 0
/** @brief Default TX interrupt delay value */
#define E1000_TX_INT_DELAY_INIT 8
/** @brief Default absolute RX interrupt delay value */
#define E1000_RX_ABS_INT_DELAY_INIT 128
/** @brief Default absolute TX interrupt delay value */
#define E1000_TX_ABS_INT_DELAY_INIT 32
/** @brief Default RX checksum enable value */
#define E1000_ENABLE_XSUM_RX_INIT true
/** @brief Default use of jumbo frames */
#define E1000_USE_JUMBO_FRAMES_INIT false
/** @brief Default receive buffer count */
#define E1000_RX_BUFFER_COUNT 256
/** @brief Default transmit buffer count */
#define E1000_TX_BUFFER_COUNT 256
/** @brief Default unicast promiscuous mode */
#define E1000_ENABLE_UNICAST_PROMISCUOUS_INIT true
/** @brief Default multicast promiscuous mode */
#define E1000_ENABLE_MULTICAST_PROMISCUOUS_INIT true
/** @brief Default broadcast listen mode */
#define E1000_LISTEN_BROADCAST_INIT true

/* --- EEPROM Values --- */
/** @brief EEPROM Control Register 2 */
#define E1000_EEPROM_INIT_CTRL2 0x000F
/** @brief Flow Control Mask */
#define E1000_FLOW_CONTROL_MASK 0x3000
/** @brief Flow Control Transmit Mask */
#define E1000_FLOW_CONTROL_TX_MASK 0x2000
/** @brief Flow Control Receive Mask */
#define E1000_FLOW_CONTROL_RX_MASK 0x1000

/** @brief EEPROM Signature Word */
#define E1000_EEPROM_SIGNATURE_WORD 0x000A
/** @brief EEPROM Signature Mask */
#define E1000_EEPROM_SIGNATURE_MASK 0xC000
/** @brief EEPROM Signature Value */
#define E1000_EEPROM_SIGNATURE_VALUE 0x4000

/** @brief Reset timeout in nanoseconds */
#define E1000_RESET_TIMEOUT_NS 20000000ULL
/** @brief EEPROM read timeout in nanoseconds */
#define E1000_EEPROM_READ_TIMEOUT_NS 5000000ULL

/* --- PHY Values --- */
/** @brief PHY Control Register: Full Duplex */
#define E1000_PHY_CTRL_FD (1 << 8)
/** @brief PHY Control Register: Speed Selection 10 Mbps */
#define E1000_PHY_CTRL_SPEED_10 0
/** @brief PHY Control Register: Speed Selection 100 Mbps */
#define E1000_PHY_CTRL_SPEED_100 (1 << 13)
/** @brief PHY Control Register: Speed Selection 1000 Mbps */
#define E1000_PHY_CTRL_SPEED_1000 (1 << 6)
/** @brief PHY Control Register: Speed Mask */
#define E1000_PHY_CTLR_SPEED_MASK ((1 << 6) | (1 << 13))
/** @brief PHY Control Register: Duplex Mask */
#define E1000_PHY_CTRL_DUPLEX_MASK (1 << 8)
/** @brief PHY Control Register: Auto-Negotiation Restart */
#define E1000_PHY_CTRL_AN_RESTART (1 << 9)
/** @brief PHY Control Register: Auto-Negotiation Enable */
#define E1000_PHY_CTRL_AN_ENABLE (1 << 12)
/** @brief PHY Control Register: Reset */
#define E1000_PHY_CTRL_RESET (1 << 15)

/** @brief PHY Auto-Negotiation Advertisement: 100BASE-TX Full Duplex */
#define E1000_PHY_ANA_100BASE_TX_FULL (1 << 8)
/** @brief PHY Auto-Negotiation Advertisement: 100BASE-TX Half Duplex */
#define E1000_PHY_ANA_100BASE_TX_HALF (1 << 7)
/** @brief PHY Auto-Negotiation Advertisement: 10BASE-T Full Duplex */
#define E1000_PHY_ANA_10BASE_T_FULL (1 << 6)
/** @brief PHY Auto-Negotiation Advertisement: 10BASE-T Half Duplex */
#define E1000_PHY_ANA_10BASE_T_HALF (1 << 5)
/** @brief PHY Auto-Negotiation Advertisement: 1000BASE-X Full Duplex */
#define E1000_PHY_ANA_1000BASE_X_FULL (1 << 6)
/** @brief PHY Auto-Negotiation Advertisement: 1000BASE-X Half Duplex */
#define E1000_PHY_ANA_1000BASE_X_HALF (1 << 5)

/** @brief PHY Auto-Negotiation Advertisement: 1000BASE-T Full Duplex */
#define E1000_PHY_1000T_1000BASE_T_FULL (1 << 9)
/** @brief PHY Auto-Negotiation Advertisement: 1000BASE-T Half Duplex */
#define E1000_PHY_1000T_1000BASE_T_HALF (1 << 8)

/* --- Auto-Negotiation Masks --- */
/** @brief Auto-Negotiation Mask: 1000BASE-T Full Duplex */
#define E1000_AUTONEG_MASK_1000_FULL 0x20
/** @brief Auto-Negotiation Mask: 1000BASE-T Half Duplex */
#define E1000_AUTONEG_MASK_1000_HALF 0x10
/** @brief Auto-Negotiation Mask: 100BASE-TX Full Duplex */
#define E1000_AUTONEG_MASK_100_FULL 0x08
/** @brief Auto-Negotiation Mask: 100BASE-TX Half Duplex */
#define E1000_AUTONEG_MASK_100_HALF 0x04
/** @brief Auto-Negotiation Mask: 10BASE-T Full Duplex */
#define E1000_AUTONEG_MASK_10_FULL 0x02
/** @brief Auto-Negotiation Mask: 10BASE-T Half Duplex */
#define E1000_AUTONEG_MASK_10_HALF 0x01
/** @brief Auto-Negotiation Mask: Off */
#define E1000_AUTONEG_MASK_OFF 0x00

/* --- Miscellaneous --- */
/** @brief E1000 PBA (Packet Buffer Allocation) Register default value */
#define E1000_PBA_RX_DEFAULT 0x00000030

/** @brief PHY Initialization Timeout in Nanoseconds */
#define E1000_PHY_INIT_TIMEOUT_NS 20000000ULL
/** @brief Maximum number of PHYs supported by the E1000 */
#define E1000_MDIO_MAX_PHY 32
/** @brief MDIO Access Timeout in Nanoseconds */
#define E1000_MDIO_ACCESS_TIMEOUT_NS 10000000ULL

/** @brief Receive buffer size */
#define E1000_RX_BUFFER_SIZE 2048
/** @brief Receive buffer alignment */
#define E1000_RX_BUFFER_ALIGNMENT 64
/** @brief Transmit buffer size */
#define E1000_TX_BUFFER_SIZE 2048
/** @brief Transmit buffer alignment */
#define E1000_TX_BUFFER_ALIGNMENT 64

/** @brief Enabled interrupt mask */
#define E1000_ENABLED_INTERRUPTS (E1000_IMS_TXDW | \
                                  E1000_IMS_LSC  | \
                                  E1000_IMS_RXT0)

/** @brief Maximum number of times to poll the device */
#define E1000_POLL_MAX_BUDGET 64
/** @brief Maximum number per batch to process TX descriptors */
#define E1000_TX_MAX_BUDGET 16
/** @brief Maximum number per batch to clean TX descriptors */
#define E1000_TX_CLEAN_BUDGET 32
/** @brief Number of times to wait for transactions to complete */
#define E1000_TRANSACTION_WAIT_COUNT 500
/** @brief Number of nanoseconds to wait for transactions to complete */
#define E1000_TRANSACTION_WAIT_NS 20000000
/** @brief Number of nanoseconds to wait for status updates */
#define E1000_STAT_UPDATE_INTERVAL_NS 2000000000

/** @brief Maximum number of multicast addresses supported */
#define E1000_MULTICAST_TABLE_SIZE 128
/** @brief Maximum number of VLAN filters supported */
#define E1000_VLAN_FILTER_TABLE_SIZE 128

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief The type of the E1000 EEPROM. */
typedef enum
{
  /** @brief Legacy 8-bit/6-bit bit-bang/EERD structure */
  EEPROM_MICROWIRE,
  /** @brief Modern 16-bit SPI structure */
  EEPROM_SPI
} E_E1000EEPROMType;

/** @brief The type of the E1000 media. */
typedef enum
{
  /** @brief Traditional PHY management/auto-negotiation */
  MEDIA_COPPER,
  /** @brief Fiber SerDes/optical link state machine */
  MEDIA_FIBER,
  /** @brief Internal Gigabit Serializer/Deserializer interface */
  MEDIA_SERDES
} E_E1000MediaType;

/** @brief The type of the E1000 loopback mode. */
typedef enum
{
  /** @brief E1000 Loopback Mode: None */
  LOOPBACK_NONE,
  /** @brief E1000 Loopback Mode: PHY */
  LOOPBACK_PHY,
  /** @brief E1000 Loopback Mode: Internal SerDes */
  LOOPBACK_SERDES,
  /** @brief E1000 Loopback Mode: External SerDes */
  LOOPBACK_EXTERNAL_SERDES
} E_E1000LoopbackMode;

/** @brief The type of the E1000 flow control. */
typedef enum
{
  /** @brief E1000 Flow Control: None */
  FLOW_CONTROL_NONE,
  /** @brief E1000 Flow Control: RX Pause */
  FLOW_CONTROL_RX_PAUSE,
  /** @brief E1000 Flow Control: TX Pause */
  FLOW_CONTROL_TX_PAUSE,
  /** @brief E1000 Flow Control: RX/TX Pause */
  FLOW_CONTROL_RX_TX_PAUSE
} E_E1000FlowControl;

/** @brief RX Packet Buffer Descriptor */
typedef struct
{
  /** @brief The address of the receive buffer. */
  uint64_t bufferAddress;
  /** @brief The length of the receive buffer. */
  uint16_t length;
  /** @brief The checksum of the received packet. */
  uint16_t checksum;
  /** @brief The status of the receive descriptor. */
  uint8_t status;
  /** @brief The errors of the receive descriptor. */
  uint8_t errors;
  /** @brief The special field of the receive descriptor. */
  uint16_t special;
} __attribute__((packed)) S_RXBufferDescriptor;

/** @brief TX Packet Buffer Descriptor */
typedef struct
{
  /** @brief The address of the transmit buffer. */
  uint64_t bufferAddress;
  /** @brief The length of the transmit buffer. */
  uint16_t length;
  /** @brief The checksum offset of the transmit buffer. */
  uint8_t cso;
  /** @brief The command of the transmit descriptor. */
  uint8_t cmd;
  /** @brief The status of the transmit descriptor. */
  uint8_t status;
  /** @brief The checksum start of the transmit descriptor. */
  uint8_t css;
  /** @brief The special field of the transmit descriptor. */
  uint16_t special;
} __attribute__((packed)) S_TXBufferDescriptor;

/** @brief E1000 Buffer Descriptor Structure */
typedef struct
{
  /** @brief The physical address of the buffer. */
  uintptr_t physAddr;
  /** @brief The virtual address of the buffer. */
  void *virtAddr;
} S_E1000BufferDescriptor;

/** @brief E1000 TX Buffer Metadata Structure */
typedef struct
{
  /** @brief The virtual address of the TX buffer. */
  uintptr_t virtAddr;
  /** @brief The length of the TX buffer. */
  size_t length;
} S_E1000TXBufferMetadata;

/** @brief The E1000 driver structure. */
typedef struct S_E1000Driver
{
  /** @brief The PCI device descriptor. */
  S_PCIDevice *pPCIDevice;
  /** @brief The PCI device identifier. */
  uint16_t deviceId;

  /** @brief The device EEPROM type. */
  E_E1000EEPROMType eepromType;
  /** @brief The media type. */
  E_E1000MediaType mediaType;

  /** @brief The base address of the device's BAR. */
  uintptr_t barAddress;
  /** @brief The size of the device's BAR. */
  size_t barSize;

  /** @brief The MAC address of the device. */
  uint64_t macAddress;
  /** @brief The PHY address of the device. */
  uint8_t phyAddress;

  /** @brief The RX descriptor ring address. */
  S_RXBufferDescriptor *rxDescriptorRing;
  /** @brief The addresses of the RX descriptors in use. */
  S_E1000BufferDescriptor *rxDescriptorAddresses;
  /** @brief The RX descriptor memory size (ring + addresses). */
  size_t rxDescriptorMemorySize;
  /** @brief The index of the next RX descriptor to use. */
  uint32_t rxDescriptorIdx;
  /** @brief The list of RX fragments. */
  S_NetworkFragmentList *rxFragmentList;

  /** @brief The TX descriptor ring address. */
  S_TXBufferDescriptor *txDescriptorRing;
  /** @brief The addresses of the TX descriptors in use. */
  S_E1000TXBufferMetadata *txDescriptorMeta;
  /** @brief The TX descriptor memory size (ring + addresses). */
  size_t txDescriptorMemorySize;
  /** @brief The index of the next TX descriptor to use. */
  uint32_t txDescriptorIdx;
  /** @brief The index of the next TX descriptor to clean. */
  uint32_t txDescriptorCleanIdx;
  /** @brief The index of the next TX descriptor to watch. */
  uint32_t txDescriptorWatchIdx;
  /** @brief The number of TX descriptors in use. */
  uint32_t txDescriptorInUse;

  /** @brief Whether to use legacy interrupts. */
  bool useLegacyInterrupts;
  /** @brief The interrupt line used by the device. */
  int32_t interruptLine;

  /** @brief The network device descriptor. */
  S_NetworkDevice networkDevice;

  /** @brief The lock for the TX ring. */
  S_KernelSpinlock txRingLock;
  /** @brief The lock for the device. */
  S_KernelSpinlock linkLock;

  /** @brief The number of RX buffers. */
  uint32_t rxBufferCount;
  /** @brief The number of TX buffers. */
  uint32_t txBufferCount;
  /** @brief The size of each buffer. */
  uint32_t bufferSize;
  /** @brief The auto-negotiation mask. */
  uint8_t autoNegMask;
  /** @brief The link speed. */
  E_NetworkLinkSpeed linkSpeed;
  /** @brief The duplex type. */
  E_NetworkDuplexType duplexType;
  /** @brief The loopback mode. */
  E_E1000LoopbackMode loopbackMode;
  /** @brief The flow control type. */
  E_E1000FlowControl flowControl;
  /** @brief The interrupt throttle value. */
  uint32_t intThrottle;
  /** @brief The RX interrupt delay value. */
  uint32_t rxIntDelay;
  /** @brief The TX interrupt delay value. */
  uint32_t txIntDelay;
  /** @brief The absolute RX interrupt delay value. */
  uint32_t rxAbsIntDelay;
  /** @brief The absolute TX interrupt delay value. */
  uint32_t txAbsIntDelay;
  /** @brief Whether to enable RX checksum offloading. */
  bool enableXSumRx;
  /** @brief Whether to enable jumbo frames. */
  bool useJumboFrames;
  /** @brief Whether to enable unicast promiscuous mode. */
  bool enableUnicastPromiscuous;
  /** @brief Whether to enable multicast promiscuous mode. */
  bool enableMulticastPromiscuous;
  /** @brief Whether to listen for broadcast packets. */
  bool listenBroadcast;

  /** @brief Whether the RX operation is running. */
  bool rxOperationRunning;
  /** @brief Whether the TX operation is running. */
  bool txOperationRunning;
  /** @brief Whether the TX complete operation is running. */
  bool txCompleteOperationRunning;
  /** @brief Whether the link operation is running. */
  bool linkOperationRunning;
  /** @brief Whether the IOCTL operation is running. */
  bool ioctlOperationRunning;

  /** @brief Whether the link is up. */
  bool isLinkUp;
  /** @brief Whether the driver has been initialized. */
  bool isInitialized;

  /** @brief The network statistics for the device. */
  S_NetworkStatistics stats;
  /** @brief The lock for the network statistics. */
  S_KernelSpinlock statsLock;

  /** @brief The task handle for the statistics operation. */
  T_NetManagerTask statTaskHandle;

  /** @brief The lock for the IOCTL operations. */
  S_KernelSpinlock ioLock;

  /** @brief The next E1000 driver in the linked list. */
  struct S_E1000Driver *pNext;
} S_E1000Driver;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the E1000 to ensure correctness of execution.
 *
 * @details Assert macro used by the E1000 to ensure correctness of execution.
 * Due to the critical nature of the E1000, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define E1000_ASSERT(COND, MSG, ERROR)              \
  {                                                 \
    if ((COND) == false)                            \
    {                                               \
      PANIC(ERROR, MODULE_NAME, MSG, false, false); \
    }                                               \
  }

/**
 * @brief Macro to access E1000 registers.
 *
 * @details Macro to access E1000 registers. It calculates the address of the
 * register based on the base address and the offset.
 *
 * @param[in] BASE The base address of the E1000 device.
 * @param[in] OFFSET The offset of the register to access.
 *
 * @return A pointer to the volatile uint32_t register.
 */
#define E1000_REG(BASE, OFFSET) \
  ((volatile uint32_t *)((uintptr_t)(BASE) + (uintptr_t)(OFFSET)))

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Validates the E1000 EEPROM.
 *
 * @details Validates the E1000 EEPROM by reading its contents and checking for
 * errors. This function is used to ensure that the EEPROM is functioning
 * correctly before proceeding with further operations.
 *
 * @param[in] kpDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _ValidateEEPROM(const S_E1000Driver* kpDriver);

/**
 * @brief Reads a value from the E1000 EEPROM.
 *
 * @details Reads a value from the E1000 EEPROM at the specified address.
 *
 * @param[in] kpDriver The E1000 driver instance.
 * @param[in] kAddress The address to read from the EEPROM.
 * @param[out] pValue The value read from the EEPROM.
 *
 * @return The success state or the error code.
 */
static E_Return _ReadEEPROM(const S_E1000Driver* kpDriver,
                            const uint16_t       kAddress,
                            uint16_t*            pValue);
/**
 * @brief Detects the MDIO interface of the E1000 device.
 *
 * @details Detects the MDIO interface of the E1000 device and initializes the
 * driver structure accordingly.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _DetectMDIO(S_E1000Driver* pDriver);

/**
 * @brief Reads a value from the MDIO interface.
 *
 * @details Reads a value from the MDIO interface of the E1000 device.
 *
 * @param[in] kpDriver The E1000 driver instance.
 * @param[in] kRegAddress The address of the register to read.
 * @param[out] pValue The value read from the register.
 *
 * @return The success state or the error code.
 */
static E_Return _ReadMDIO(const S_E1000Driver* kpDriver,
                          const uint8_t        kRegAddress,
                          uint16_t*            pValue);

/**
 * @brief Writes a value to the MDIO interface.
 *
 * @details Writes a value to the MDIO interface of the E1000 device.
 *
 * @param[in] kpDriver The E1000 driver instance.
 * @param[in] kRegAddress The address of the register to write.
 * @param[in] kValue The value to write to the register.
 *
 * @return The success state or the error code.
 */
static E_Return _WriteMDIO(const S_E1000Driver* kpDriver,
                           const uint8_t        kRegAddress,
                           const uint16_t       kValue);
/**
 * @brief Attaches the E1000 driver to the system.
 *
 * @details Attaches the E1000 driver to the system. This function will use the
 * PCI device to initialize the E1000 hardware and retreive the E1000
 * parameters.
 *
 * @param[in] pDevice The PCI device node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(S_PCIDevice *pDevice);

/**
 * @brief Detaches the E1000 driver from the system.
 *
 * @details Detaches the E1000 driver from the system. This function will
 * release all resources allocated by the driver and remove it from the system.
 *
 * @param[in] pDriver The E1000 driver instance to detach.
 */
static void _Detach(S_E1000Driver* pDriver);

/**
 * @brief Probes the E1000 device for its capabilities.
 *
 * @details Probes the E1000 device for its capabilities and initializes the
 * driver structure accordingly.
 *
 * @param[in] kDeviceId The device ID to probe.
 * @param[out] pDriver The driver structure to initialize.
 *
 * @return The success state or the error code.
 */
static E_Return _ProbeDevice(const uint16_t kDeviceId, S_E1000Driver* pDriver);

/**
 * @brief Loads the default device parameters into the driver structure.
 *
 * @details Loads the default device parameters into the driver structure. This
 * function is used to initialize the default parameters of the E1000 device
 * before probing for its capabilities.
 *
 * @param[out] pDriver The driver structure to initialize.
 */
static void _LoadDefaultDevice(S_E1000Driver* pDriver);

/**
 * @brief Detects the E1000 device and initializes the driver structure.
 *
 * @details Detects the E1000 device and initializes the driver structure with
 * the device's capabilities and parameters.
 *
 * @param[in] pDevice The PCI device node to detect.
 * @param[out] pDriver The driver structure to initialize.
 *
 * @return The success state or the error code.
 */
static E_Return _DetectDevice(S_PCIDevice* pDevice, S_E1000Driver* pDriver);

/**
 * @brief Resets the E1000 device.
 *
 * @details Resets the E1000 device to its default state.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _ResetDevice(S_E1000Driver* pDriver);

/**
 * @brief Allocates the resources for the E1000 device.
 *
 * @details Allocates the resources for the E1000 device, such as memory and
 * buffer.
 *
 * @param[in] pDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _AllocateDeviceResources(S_E1000Driver* pDriver);

/**
 * @brief Allocates the RX resources for the E1000 device.
 *
 * @details Allocates the RX resources for the E1000 device, such as memory
 * and buffer.
 *
 * @param[in] pDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _AllocateDeviceRXResources(S_E1000Driver* pDriver);

/**
 * @brief Allocates the TX resources for the E1000 device.
 *
 * @details Allocates the TX resources for the E1000 device, such as memory
 * and buffer.
 *
 * @param[in] pDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _AllocateDeviceTXResources(S_E1000Driver* pDriver);

/**
 * @brief Sets the MAC address of the E1000 device.
 *
 * @details Sets the MAC address of the E1000 device to the specified value.
 *
 * @param[in] pDriver The E1000 driver instance.
 * @param[in] kMACAddress The MAC address to set.
 */
static void _SetDeviceMAC(S_E1000Driver* pDriver, const uint64_t kMACAddress);

/**
 * @brief Releases the resources of the E1000 device.
 *
 * @details Releases the resources of the E1000 device, such as memory and
 * buffers
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _ReleaseDeviceResources(S_E1000Driver* pDriver);

/**
 * @brief Releases the RX resources of the E1000 device.
 *
 * @details Releases the RX resources of the E1000 device, such as memory and
 * buffers
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _ReleaseDeviceRXResources(S_E1000Driver* pDriver);

/**
 * @brief Releases the TX resources of the E1000 device.
 *
 * @details Releases the TX resources of the E1000 device, such as memory and
 * buffers
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _ReleaseDeviceTXResources(S_E1000Driver* pDriver);

/**
 * @brief Initializes the control flow of the E1000 device.
 *
 * @details Initializes the control flow of the E1000 device, for both RX and TX
 * paths, and sets up the necessary registers and configurations.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _InitializeControlFlow(S_E1000Driver* pDriver);

/**
 * @brief Initializes the RX path of the E1000 device.
 *
 * @details Initializes the RX path of the E1000 device, such as setting up
 * the RX descriptor ring and configuring the device registers.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _InitializeDeviceRX(S_E1000Driver* pDriver);

/**
 * @brief Initializes the TX path of the E1000 device.
 *
 * @details Initializes the TX path of the E1000 device, such as setting up
 * the TX descriptor ring and configuring the device registers.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _InitializeDeviceTX(S_E1000Driver* pDriver);

/**
 * @brief Sets up the device interrupts for the E1000 device.
 *
 * @details Sets up the device interrupts for the E1000 device, including
 * configuring the interrupt mask and enabling the necessary interrupts.
 *
 * @param[in] pDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _SetupDeviceInterrupts(S_E1000Driver* pDriver);

/**
 * @brief Sets the PHY to forced mode.
 *
 * @details Sets the PHY to forced mode, which means it will use a specific
 * speed and duplex setting.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _SetPhyForced(S_E1000Driver* pDriver);

/**
 * @brief Sets the PHY to auto-negotiation mode.
 *
 * @details Sets the PHY to auto-negotiation mode, which means it will
 * automatically negotiate the speed and duplex settings with the link partner.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _SetPhyAutoNeg(S_E1000Driver* pDriver);

/**
 * @brief Sets the PHY to non-copper mode.
 *
 * @details Sets the PHY to non-copper mode, which means it will use a
 * different interface type, such as fiber or SerDes.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _SetPhyNonCopper(S_E1000Driver* pDriver);

/**
 * @brief Initializes the PHY of the E1000 device.
 *
 * @details Initializes the PHY of the E1000 device, including setting up the
 * necessary registers and configurations for proper operation.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _InitializePhy(S_E1000Driver* pDriver);

/**
 * @brief Sets up the multicast table for the E1000 device.
 *
 * @details Sets up the multicast table for the E1000 device, which is used to
 * filter incoming multicast packets based on the configured multicast
 * addresses.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _SetupMulticastTable(S_E1000Driver* pDriver);

/**
 * @brief Sets up the VLAN for the E1000 device.
 *
 * @details Sets up the VLAN for the E1000 device, which is used to filter
 * incoming packets based on the configured VLAN ID.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _SetupVLAN(S_E1000Driver* pDriver);

/**
 * @brief Brings the E1000 device link up.
 *
 * @details Brings the E1000 device link up, which means it will start
 * transmitting and receiving packets. This function allocates the necessary
 * resources and initializes the device for operation.
 *
 * @param[in] pDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _DevUplink(S_NetworkDevice* pNetworkDevice);

/**
 * @brief Brings the E1000 device link down.
 *
 * @details Brings the E1000 device link down, which means it will stop
 * transmitting and receiving packets. This function releases the allocated
 * resources and deinitializes the device.
 *
 * @param[in] pDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _DevDownlink(S_NetworkDevice* pNetworkDevice);

/**
 * @brief Waits for the completion of a transaction on the E1000 device.
 *
 * @details Waits for the completion of a transaction on the E1000 device,
 * checking the status of the device and ensuring that the operation has
 * completed successfully.
 *
 * @param[in] pDriver The E1000 driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _WaitForTransactionCompletion(S_E1000Driver* pDriver);

/**
 * @brief Transmits a packet through the E1000 device.
 *
 * @details Transmits a packet through the E1000 device, using the provided
 * buffer and size. This function handles the necessary descriptor management
 * and ensures that the packet is transmitted correctly.
 *
 * @param[in] pDevice The network device instance.
 * @param[in] kpBuffer The buffer containing the packet data to transmit.
 * @param[in] kSize The size of the packet data in bytes.
 * @param[in] kIsLastOfBurst Whether this is the last packet of a burst.
 * @param[in] kIsBlocking Whether the upper stack level will block until the
 * transmission ready to be sent.
 *
 * @return The number of bytes transmitted or an error code.
 */
static ssize_t _TransmitPacket(struct S_NetworkDevice* pDevice,
                               const void*             kpBuffer,
                               const size_t            kSize,
                               const bool              kIsLastOfBurst,
                               const bool              kIsBlocking);

/**
 * @brief Handles the IOCTL commands for the E1000 device.
 *
 * @details Handles the IOCTL commands for the E1000 device, allowing for
 * configuration and control of the device through various commands.
 *
 * @param[in] pDevice The network device instance.
 * @param[in] kCommand The IOCTL command to execute.
 * @param[in] pArg The argument for the IOCTL command.
 *
 * @return The success state or the error code.
 */
static ssize_t _IOControl(struct S_NetworkDevice* pDevice,
                          const uint32_t          kCommand,
                          void*                   pArg);

/**
 * @brief Interrupt handler for the E1000 device.
 *
 * @details Interrupt handler for the E1000 device. This function is called
 * when an interrupt is generated by the device, and it handles the necessary
 * processing of the interrupt, including cleaning up completed transmissions
 * and processing received packets.
 *
 * @return The function returns true if the interrupted thread needs to be
 * scheduled, false otherwise.
 */
static bool _E1000InterruptHandler(void);

/**
 * @brief Worker function to handle interrupts for the E1000 device.
 *
 * @details Worker function to handle interrupts for the E1000 device. This
 * function is called by the network worker thread and performs the necessary
 * processing of the interrupt in a separate context.
 *
 * @param[in] pDriverPtr Pointer to the E1000 driver instance.
 */
static void _HandleInterruptWorker(void* pDriverPtr);

/**
 * @brief Handles the link status change for the E1000 device.
 *
 * @details Handles the link status change for the E1000 device. This function
 * is called when the link status changes, and it updates the driver state
 * accordingly.
 *
 * @param[in] pDriverPtr Pointer to the E1000 driver instance.
 */
static void _HandleLinkStatusChange(void* pDriverPtr);

/**
 * @brief Cleans up completed transmissions for the E1000 device.
 *
 * @details Cleans up completed transmissions for the E1000 device. This
 * function checks the status of the TX descriptors and frees up any resources
 * associated with completed transmissions.
 *
 * @param[in] pDriver The E1000 driver instance.
 * @param[out] pIsMoreToClean Pointer to a boolean indicating if there are more
 * transmissions to clean.
 */
static void _CompleteTransmit(S_E1000Driver* pDriver, bool* pIsMoreToClean);

/**
 * @brief Handles received packets for the E1000 device.
 *
 * @details Handles received packets for the E1000 device. This function
 * processes the RX descriptors and passes the received packets to the upper
 * network stack.
 *
 * @param[in] pDriver The E1000 driver instance.
 */
static void _HandleReceivedPacket(S_E1000Driver* pDriver);

/**
 * @brief Updates the network statistics for the E1000 device.
 *
 * @details Updates the network statistics for the E1000 device. This function
 * reads the relevant registers and updates the statistics in the driver
 * structure.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 */
static void _UpdateStatistics(void* pDriver);

/**
 * @brief Handles the IOCTL command to get the network statistics.
 *
 * @details Handles the IOCTL command to get the network statistics for the
 * E1000 device. This function retrieves the current statistics and copies them
 * to the provided argument.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 * @param[in] pArg Pointer to the argument where the statistics will be copied.
 *
 * @return The size of the statistics structure is returned on success, a
 * negative error code is returned on failure.
 */
static ssize_t _IOControlGetStats(S_E1000Driver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to reset the network statistics.
 *
 * @details Handles the IOCTL command to reset the network statistics for the
 * E1000 device. This function resets the statistics counters in the driver
 * structure.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlResetStats(S_E1000Driver* pDriver);

/**
 * @brief Handles the IOCTL command to set the MAC address.
 *
 * @details Handles the IOCTL command to set the MAC address for the E1000
 * device. This function updates the MAC address in the driver structure and
 * configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 * @param[in] pArg Pointer to the argument containing the new MAC address.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlSetMACAddress(S_E1000Driver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to get the MAC address.
 *
 * @details Handles the IOCTL command to get the MAC address for the E1000
 * device. This function retrieves the current MAC address from the driver
 * structure and copies it to the provided argument.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 * @param[in] pArg Pointer to the argument where the MAC address will be copied.
 *
 * @return The size of the MAC address structure is returned on success, a
 * negative error code is returned on failure.
 */
static ssize_t _IOControlGetMACAddress(S_E1000Driver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to set the loopback mode.
 *
 * @details Handles the IOCTL command to set the loopback mode for the E1000
 * device. This function updates the loopback mode in the driver structure and
 * configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 * @param[in] pArg Pointer to the argument containing the new loopback mode.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlSetLoopbackMode(S_E1000Driver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to get the link status.
 *
 * @details Handles the IOCTL command to get the link status for the E1000
 * device. This function retrieves the current link status from the driver
 * structure and copies it to the provided argument.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 * @param[in] pArg Pointer to the argument where the link status will be copied.
 *
 * @return The link status is returned on success (0 = down, 1 = up), a negative
 * error code is returned on failure.
 */
static ssize_t _IOControlGetLinkStatus(S_E1000Driver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to set the auto-negotiation mode.
 *
 * @details Handles the IOCTL command to set the auto-negotiation mode for the
 * E1000 device. This function updates the auto-negotiation settings in the
 * driver structure and configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlSetAutoNeg(S_E1000Driver* pDriver);

/**
 * @brief Handles the IOCTL command to force the speed and duplex settings.
 *
 * @details Handles the IOCTL command to force the speed and duplex settings
 * for the E1000 device. This function updates the speed and duplex settings in
 * the driver structure and configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000 driver instance.
 * @param[in] pArg Pointer to the argument containing the new speed and duplex
 * settings.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlForceSpeedDuplex(S_E1000Driver* pDriver, void* pArg);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief E1000 driver instance. */
static S_PCIDriver sX86E1000Driver =
{
  .pName = "X86 E1000 Driver",
  .pDescription = "X86 Intel E1000 Driver for roOs",
  .pVersion = "1.0",
  .vendorID = INTEL_VENDOR_ID,
  .deviceID = 0xFFFF,
  .devClass = 0x02,
  .devSubClass = 0x00,
  .devProgIf = 0xFF,
  .pDriverAttach = _Attach
};
/** @brief E1000 driver linked list. */
static S_E1000Driver* spDriverList = NULL;
/** @brief E1000 driver list lock. */
static S_KernelSpinlock sDriverListLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _ValidateEEPROM(const S_E1000Driver* kpDriver)
{
  E_Return error;
  uint16_t signature;

  error = _ReadEEPROM(kpDriver, E1000_EEPROM_SIGNATURE_WORD, &signature);

  if (error == NO_ERROR)
  {
    signature = signature & E1000_EEPROM_SIGNATURE_MASK;
    if (signature == E1000_EEPROM_SIGNATURE_VALUE)
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000 EEPROM signature validated successfully with value "
                   "0x%04X",
                   signature);
      error = NO_ERROR;
    }
    else
    {
      KERNEL_ERROR("E1000 EEPROM signature validation failed with value "
                   "0x%04X\n",
                   signature);
      error = ERR_INVALID_VALUE;
    }
  }

  return error;
}

static E_Return _ReadEEPROM(const S_E1000Driver* kpDriver,
                            const uint16_t       kAddress,
                            uint16_t*            pValue)
{
  E_Return error;
  uint32_t opDoneMask;
  uint32_t regValue;
  uint16_t address;
  uint64_t timeout;

  if (kpDriver->eepromType == EEPROM_MICROWIRE)
  {
    opDoneMask = E1000_EERD_MICROWIRE_DONE;
    address = kAddress << E1000_EERD_MICROWIRE_ADDR_SHIFT;
  }
  else
  {
    opDoneMask = E1000_EERD_SPI_DONE;
    address = kAddress << E1000_EERD_SPI_ADDR_SHIFT;
  }

  regValue = E1000_EERD_START | address;
  _MMIOWrite32(E1000_REG(kpDriver->barAddress, E1000_EERD), regValue);

  timeout = TimeGetUptime() + E1000_EEPROM_READ_TIMEOUT_NS;
  do
  {
    regValue = _MMIORead32(E1000_REG(kpDriver->barAddress, E1000_EERD));
    if (TimeGetUptime() > timeout)
    {
      break;
    }
  } while ((regValue & opDoneMask) == 0);

  if ((regValue & opDoneMask) != 0)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 EEPROM read for address %u returned value 0x%04X",
                 kAddress,
                 (uint16_t)(regValue >> E1000_EERD_DATA_SHIFT));
    *pValue = (uint16_t)(regValue >> E1000_EERD_DATA_SHIFT);
    error = NO_ERROR;
  }
  else
  {
    KERNEL_ERROR("E1000 EEPROM read timeout for address %u\n", kAddress);
    error = ERR_EXCEEDED_LIMIT;
  }

  return error;
}

static void _DetectMDIO(S_E1000Driver* pDriver)
{
  uint16_t phyId1;
  uint16_t i;
  E_Return error;

  for (i = 0; i < E1000_MDIO_MAX_PHY; ++i)
  {
    pDriver->phyAddress = i;
    error = _ReadMDIO(pDriver, 0, &phyId1);
    if (error == NO_ERROR)
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000 PHY detected at address %u ",
                   i);
      break;
    }
  }
  if (i == E1000_MDIO_MAX_PHY)
  {
    pDriver->phyAddress = 1;
    KERNEL_ERROR("E1000 PHY not detected on any address\n");
  }
}

static E_Return _ReadMDIO(const S_E1000Driver* kpDriver,
                          const uint8_t        kRegAddress,
                          uint16_t*            pValue)
{
  E_Return error;
  uint32_t regValue;
  uint64_t timeout;

  regValue = E1000_MDIO_READ |
             ((kRegAddress & 0x1F) << E1000_MDIO_ADDR_SHIFT) |
             ((kpDriver->phyAddress & 0x1F) << E1000_MDIO_PHY_SHIFT);
  _MMIOWrite32(E1000_REG(kpDriver->barAddress, E1000_MDIC), regValue);

  timeout = TimeGetUptime() + E1000_MDIO_ACCESS_TIMEOUT_NS;
  do
  {
    regValue = _MMIORead32(E1000_REG(kpDriver->barAddress, E1000_MDIC));
    if (TimeGetUptime() > timeout)
    {
      break;
    }
  } while ((regValue & E1000_MDIO_DONE) == 0 &&
           (regValue & E1000_MDIO_ERROR) == 0);

  if ((regValue & E1000_MDIO_DONE) != 0 && (regValue & E1000_MDIO_ERROR) == 0)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 MDIO read for address %u returned value 0x%04X",
                 kpDriver->phyAddress,
                 (uint16_t)(regValue & E1000_MDIO_DATA_MASK));
    *pValue = (uint16_t)(regValue & E1000_MDIO_DATA_MASK);
    error = NO_ERROR;
  }
  else
  {
    KERNEL_ERROR("E1000 MDIO read timeout for address %u\n",
                 kpDriver->phyAddress);
    error = ERR_EXCEEDED_LIMIT;
  }

  return error;
}

static E_Return _WriteMDIO(const S_E1000Driver* kpDriver,
                           const uint8_t        kRegAddress,
                           const uint16_t       kValue)
{
  E_Return error;
  uint32_t regValue;
  uint64_t timeout;

  regValue = kValue |
             E1000_MDIO_WRITE |
             (kRegAddress << E1000_MDIO_ADDR_SHIFT) |
             (kpDriver->phyAddress << E1000_MDIO_PHY_SHIFT);
  _MMIOWrite32(E1000_REG(kpDriver->barAddress, E1000_MDIC), regValue);

  timeout = TimeGetUptime() + E1000_MDIO_ACCESS_TIMEOUT_NS;
  do
  {
    regValue = _MMIORead32(E1000_REG(kpDriver->barAddress, E1000_MDIC));
    if (TimeGetUptime() > timeout)
    {
      break;
    }
  } while ((regValue & E1000_MDIO_DONE) == 0);

  if ((regValue & E1000_MDIO_DONE) != 0)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 MDIO write for address %u",
                 kpDriver->phyAddress);
    error = NO_ERROR;
  }
  else
  {
    KERNEL_ERROR("E1000 MDIO write timeout for address %u\n",
                 kpDriver->phyAddress);
    error = ERR_EXCEEDED_LIMIT;
  }

  return error;
}

static E_Return _Attach(S_PCIDevice* pDevice)
{
  E_Return       err;
  S_E1000Driver* pE1000Driver;
  uint8_t        function;

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Probing E1000 driver to device %04X\\%04X",
               pDevice->pFunction->vendorID,
               pDevice->pFunction->deviceID);

  /* Chech the vendor ID and device ID */
  if (pDevice->pFunction->vendorID == INTEL_VENDOR_ID)
  {
    pE1000Driver = KMalloc(sizeof(S_E1000Driver), KMALLOC_FREE_POOL);
    memset(pE1000Driver, 0, sizeof(S_E1000Driver));
    KERNEL_SPINLOCK_INIT(pE1000Driver->txRingLock);
    KERNEL_SPINLOCK_INIT(pE1000Driver->linkLock);
    KERNEL_SPINLOCK_INIT(pE1000Driver->statsLock);
    KERNEL_SPINLOCK_INIT(pE1000Driver->ioLock);

    pE1000Driver->pPCIDevice = pDevice;

    err = _ProbeDevice(pDevice->pFunction->deviceID, pE1000Driver);
    if (err == NO_ERROR)
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "Attaching E1000 driver to device %04X",
                   pDevice->pFunction->deviceID);

      err = _DetectDevice(pDevice, pE1000Driver);
      if (err == NO_ERROR)
      {
        if (pDevice->isMultiFunction == true)
        {
          function = pDevice->function;
        }
        else
        {
          function = 0xFF;
        }

        err = _SetupDeviceInterrupts(pE1000Driver);
        if (err == NO_ERROR)
        {
           err = NetManagerRegisterDevice(_DevUplink,
                                          _DevDownlink,
                                          _TransmitPacket,
                                          _IOControl,
                                          pDevice->bus,
                                          pDevice->slot,
                                          function,
                                          pE1000Driver,
                                          &pE1000Driver->networkDevice);
          if (err == NO_ERROR)
          {
            KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                        MODULE_NAME,
                        "E1000 driver registered interface %s",
                        pE1000Driver->networkDevice.pName);

            KERNEL_LOCK(sDriverListLock);
            pE1000Driver->pNext = spDriverList;
            spDriverList = pE1000Driver;
            KERNEL_UNLOCK(sDriverListLock);
          }
          else
          {
            KERNEL_ERROR("Failed to register E1000 driver with manager\n");
            _Detach(pE1000Driver);
          }
        }
        else
        {
          KERNEL_ERROR("Failed to setup E1000 driver interrupts\n");
          _Detach(pE1000Driver);
        }
      }
      else
      {
        KERNEL_ERROR("Failed to detect E1000 device %04X\\%04X\n",
                     pDevice->pFunction->vendorID,
                     pDevice->pFunction->deviceID);
        _Detach(pE1000Driver);
      }
    }
    else
    {
      KERNEL_ERROR("Unknown device E1000 device %04X\\%04X\n",
                   pDevice->pFunction->vendorID,
                   pDevice->pFunction->deviceID);
      KFree(pE1000Driver);
    }
  }
  else
  {
    KERNEL_ERROR("Unsupported vendor ID %04X\\%04X for E1000 driver\n",
                 pDevice->pFunction->vendorID,
                 pDevice->pFunction->deviceID);
    err = ERR_NOT_SUPPORTED;
  }

  return err;
}

static void _Detach(S_E1000Driver* pDriver)
{
  E_Return       retCode;
  S_E1000Driver* pPrevDriver;

  if (pDriver->interruptLine > 0)
  {
    if (pDriver->useLegacyInterrupts == true)
    {
      PCISetupInterruptMSI(pDriver->pPCIDevice, 0, false);
    }
    else
    {
      IOAPICDisableIRQ(pDriver->pPCIDevice->irqLine);
    }
    FreeInterruptLine(pDriver->interruptLine);
    pDriver->interruptLine = 0;
  }

  /* Unmap the BAR */
  if (pDriver->barAddress != 0)
  {
    retCode = MemoryKernelUnmap((void *)pDriver->barAddress, pDriver->barSize);
    E1000_ASSERT(retCode == NO_ERROR,
                 "Failed to unmap BAR for device",
                 retCode);
  }

  /* Remove from the list */
  KERNEL_LOCK(sDriverListLock);
  if (spDriverList == pDriver)
  {
    spDriverList = pDriver->pNext;
  }
  else
  {
    pPrevDriver = spDriverList;
    while (pPrevDriver != NULL && pPrevDriver->pNext != pDriver)
    {
      pPrevDriver = pPrevDriver->pNext;
    }
    if (pPrevDriver != NULL)
    {
      pPrevDriver->pNext = pDriver->pNext;
    }
  }
  KERNEL_UNLOCK(sDriverListLock);

  KFree(pDriver);
}

static E_Return _ProbeDevice(const uint16_t kDeviceId, S_E1000Driver* pDriver)
{
  E_Return retCode;

  retCode = NO_ERROR;

  pDriver->deviceId = kDeviceId;

  switch (kDeviceId)
  {
    case E1000_DEV_ID_82544EI_COPPER:
    case E1000_DEV_ID_82544GC_COPPER:
    case E1000_DEV_ID_82544GC_LOM:
    case E1000_DEV_ID_82540EM:
    case E1000_DEV_ID_82540EM_LOM:
    case E1000_DEV_ID_82540EP:
    case E1000_DEV_ID_82540EP_LOM:
    case E1000_DEV_ID_82540EP_LP:
    case E1000_DEV_ID_82545EM_COPPER:
    case E1000_DEV_ID_82545GM_COPPER:
    case E1000_DEV_ID_82546EB_COPPER:
    case E1000_DEV_ID_82546EB_QUAD_COP:
    case E1000_DEV_ID_82546GB_COPPER:
    case E1000_DEV_ID_82546GB_PCIE:
    case E1000_DEV_ID_82546GB_QUAD_COP:
      pDriver->eepromType = EEPROM_SPI;
      pDriver->mediaType = MEDIA_COPPER;
      break;

    case E1000_DEV_ID_82545EM_FIBER:
    case E1000_DEV_ID_82545GM_FIBER:
    case E1000_DEV_ID_82546EB_FIBER:
    case E1000_DEV_ID_82546GB_FIBER:
      pDriver->eepromType = EEPROM_SPI;
      pDriver->mediaType = MEDIA_FIBER;
      break;

    case E1000_DEV_ID_82545GM_SERDES:
    case E1000_DEV_ID_82546GB_SERDES:
      pDriver->eepromType = EEPROM_SPI;
      pDriver->mediaType = MEDIA_SERDES;
      break;

    case E1000_DEV_ID_82541EI:
    case E1000_DEV_ID_82541EI_MOBILE:
    case E1000_DEV_ID_82541ER:
    case E1000_DEV_ID_82541GI:
    case E1000_DEV_ID_82541GI_MOBILE:
    case E1000_DEV_ID_82541PI:
    case E1000_DEV_ID_82547EIGI:
    case E1000_DEV_ID_82547EI_MOBILE:

      pDriver->eepromType = EEPROM_MICROWIRE;
      pDriver->mediaType = MEDIA_COPPER;
      break;

    default:
      retCode = ERR_NOT_SUPPORTED;
      break;
  }

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X probing result: %s",
               kDeviceId,
               (retCode == NO_ERROR) ? "Supported" : "Not Supported");
  if (retCode == NO_ERROR)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X detected with EEPROM type %s and media "
                 "type %s",
                 kDeviceId,
                 (pDriver->eepromType == EEPROM_SPI) ? "SPI" : "MICROWIRE",
                 (pDriver->mediaType == MEDIA_COPPER) ? "Copper" :
                 (pDriver->mediaType == MEDIA_FIBER) ? "Fiber" : "SerDes");
  }


  return retCode;
}

static void _LoadDefaultDevice(S_E1000Driver* pDriver)
{
  E_Return retCode;
  uint16_t eepromValue;
  uint64_t macAddress;

  /* Initialize the device's settings */
  pDriver->rxBufferCount              = E1000_RX_BUFFER_COUNT;
  pDriver->txBufferCount              = E1000_TX_BUFFER_COUNT;
  pDriver->bufferSize                 = E1000_RX_BUFFER_SIZE;
  pDriver->autoNegMask                = E1000_AUTONEG_MASK;
  pDriver->linkSpeed                  = LINK_SPEED_AUTONEG;
  pDriver->duplexType                 = DUPLEX_AUTONEG;
  pDriver->intThrottle                = E1000_INT_THROTTLE_INIT;
  pDriver->rxIntDelay                 = E1000_RX_INT_DELAY_INIT;
  pDriver->txIntDelay                 = E1000_TX_INT_DELAY_INIT;
  pDriver->rxAbsIntDelay              = E1000_RX_ABS_INT_DELAY_INIT;
  pDriver->txAbsIntDelay              = E1000_TX_ABS_INT_DELAY_INIT;
  pDriver->enableXSumRx               = E1000_ENABLE_XSUM_RX_INIT;
  pDriver->useJumboFrames             = E1000_USE_JUMBO_FRAMES_INIT;
  pDriver->loopbackMode               = LOOPBACK_NONE;
  pDriver->enableUnicastPromiscuous   = E1000_ENABLE_UNICAST_PROMISCUOUS_INIT;
  pDriver->enableMulticastPromiscuous = E1000_ENABLE_MULTICAST_PROMISCUOUS_INIT;
  pDriver->listenBroadcast            = E1000_LISTEN_BROADCAST_INIT;

  retCode = _ReadEEPROM(pDriver, E1000_EEPROM_INIT_CTRL2, &eepromValue);
  if (retCode == NO_ERROR)
  {
    if ((eepromValue & E1000_FLOW_CONTROL_MASK) == 0)
    {
      pDriver->flowControl = FLOW_CONTROL_NONE;
    }
    else if ((eepromValue & E1000_FLOW_CONTROL_MASK) ==
             E1000_FLOW_CONTROL_TX_MASK)
    {
      pDriver->flowControl = FLOW_CONTROL_TX_PAUSE;
    }
    else if ((eepromValue & E1000_FLOW_CONTROL_MASK) ==
             E1000_FLOW_CONTROL_RX_MASK)
    {
      pDriver->flowControl = FLOW_CONTROL_RX_PAUSE;
    }
    else
    {
      pDriver->flowControl = FLOW_CONTROL_RX_TX_PAUSE;
    }
  }
  else
  {
    pDriver->flowControl = FLOW_CONTROL_NONE;
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to read flow control "
                 "settings from EEPROM\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  /* Read the MAC address from EEPROM */
  eepromValue = 0;
  retCode = _ReadEEPROM(pDriver, 0, &eepromValue);
  macAddress = eepromValue;
  retCode |= _ReadEEPROM(pDriver, 1, &eepromValue);
  macAddress |= ((uint64_t)eepromValue << 16);
  retCode |= _ReadEEPROM(pDriver, 2, &eepromValue);
  macAddress |= ((uint64_t)eepromValue << 32);

  if (retCode == NO_ERROR)
  {
    pDriver->macAddress = macAddress;
  }
  else
  {
    pDriver->macAddress = 0;
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to read MAC address from"
                 " EEPROM\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  /* Get the MDIO PHY address */
  _DetectMDIO(pDriver);
}

static E_Return _DetectDevice(S_PCIDevice* pDevice, S_E1000Driver* pDriver)
{
  E_Return retCode;

  /* Get the BAR and map it */
  pDriver->barAddress = PCIGetBARAddress(pDevice->pFunction, 0);
  pDriver->barSize    = PCIGetBARSize(pDevice->pFunction, 0);

  if (pDriver->barAddress != 0 || pDriver->barSize != 0)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X with BAR at 0x%p of size %llu",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function,
                 pDriver->barAddress,
                 pDriver->barSize);

    pDriver->barSize = ALIGN_UP(pDriver->barSize, KERNEL_PAGE_SIZE);
    pDriver->barAddress = (uintptr_t)MemoryKernelMap((void*)pDriver->barAddress,
                                                     pDriver->barSize,
                                                     MEMMGR_MAP_CACHE_DISABLED |
                                                     MEMMGR_MAP_KERNEL |
                                                     MEMMGR_MAP_RW,
                                                     &retCode);

    if (retCode == NO_ERROR)
    {
      /* Validate the EEPROM */
      retCode = _ValidateEEPROM(pDriver);
      if (retCode == NO_ERROR)
      {
        /* Load the default configuration */
        _LoadDefaultDevice(pDriver);

        KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "E1000 device %04X:%04X:%04X MAC:"
                     " %02X:%02X:%02X:%02X:%02X:%02X",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function,
                     (uint8_t)(pDriver->macAddress & 0xFF),
                     (uint8_t)((pDriver->macAddress >> 8) & 0xFF),
                     (uint8_t)((pDriver->macAddress >> 16) & 0xFF),
                     (uint8_t)((pDriver->macAddress >> 24) & 0xFF),
                     (uint8_t)((pDriver->macAddress >> 32) & 0xFF),
                     (uint8_t)((pDriver->macAddress >> 40) & 0xFF));
      }
      else
      {
        retCode = MemoryKernelUnmap((void *)pDriver->barAddress,
                                    pDriver->barSize);
        E1000_ASSERT(retCode == NO_ERROR,
                     "Failed to unmap BAR for device",
                     retCode);

        pDriver->barAddress = 0;
        pDriver->barSize = 0;
        KERNEL_ERROR("Failed to validate EEPROM for device %04X:%04X:%04X\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
      }
    }
    else
    {
      pDriver->barAddress = 0;
      pDriver->barSize = 0;
      KERNEL_ERROR("Failed to map BAR for device %04X:%04X:%04X\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    pDriver->barAddress = 0;
    pDriver->barSize = 0;
    KERNEL_ERROR("Failed to detect BAR for device %04X:%04X:%04X\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }

  return retCode;
}

static void _ResetDevice(S_E1000Driver* pDriver)
{
  uint32_t regValue;

  /* Clear interrupts */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMC), 0xFFFFFFFF);

  /* Disable RX and TX */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RCTL), 0);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TCTL), E1000_TCTL_PSP);

  /* Clear RX and TX descriptors */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDH), 0);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDT), 0);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDH), 0);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDT), 0);

  /* Wait for the disable to take effect */
  SleepNs(E1000_RESET_TIMEOUT_NS, NULL);

  /* Reset the controller */
  regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL));
  regValue |= E1000_CTRL_RST;
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL), regValue);

  /* Wait for the reset to complete */
  SleepNs(E1000_RESET_TIMEOUT_NS, NULL);

  /* Clear interrupts and pending events */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMC), 0xFFFFFFFF);
  _MMIORead32(E1000_REG(pDriver->barAddress, E1000_ICR));
}

static E_Return _AllocateDeviceResources(S_E1000Driver* pDriver)
{
  E_Return retCode;

  /* Set the device's MAC address */
  _SetDeviceMAC(pDriver, pDriver->macAddress);

  /* Allocate the device RX buffers */
  retCode = _AllocateDeviceRXResources(pDriver);
  if (retCode == NO_ERROR)
  {
    retCode = _AllocateDeviceTXResources(pDriver);
    if (retCode != NO_ERROR)
    {
      _ReleaseDeviceRXResources(pDriver);
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to allocate TX "
                   "resources\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to allocate RX "
                 "resources\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  return retCode;
}

static E_Return _AllocateDeviceRXResources(S_E1000Driver* pDriver)
{
  E_Return retCode;
  E_Return internalRetCode;
  size_t   toAllocate;

  toAllocate = ALIGN_UP(pDriver->rxBufferCount *
                        (sizeof(S_RXBufferDescriptor) +
                         sizeof(S_E1000BufferDescriptor)),
                        KERNEL_PAGE_SIZE);

  /* Allocate the RX buffer descriptor ring */
  pDriver->rxDescriptorIdx  = 0;
  pDriver->rxFragmentList   = NULL;
  pDriver->rxDescriptorRing = MemoryKernelAllocate(toAllocate,
                                                   MEMMGR_MAP_KERNEL |
                                                   MEMMGR_MAP_RW     |
                                                   MEMMGR_MAP_CACHE_DISABLED,
                                                   &retCode);

  if (retCode == NO_ERROR)
  {
    /* Allocate the RX buffer descriptor addresses */
    pDriver->rxDescriptorAddresses = (void *)
      ((uintptr_t)pDriver->rxDescriptorRing +
       (pDriver->rxBufferCount * sizeof(S_RXBufferDescriptor)));

    /* Allocate the RX descriptor ring size */
    pDriver->rxDescriptorMemorySize = toAllocate;
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X RX ring created",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);

    /* Allocate the device RX buffers */
    retCode = NetManagerCreateBufferPool(pDriver->bufferSize,
                                         pDriver->rxBufferCount,
                                         E1000_RX_BUFFER_ALIGNMENT,
                                         &pDriver->networkDevice);
    if (retCode == NO_ERROR)
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000 device %04X:%04X:%04X RX buffer pool created",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
    else
    {
      internalRetCode = MemoryKernelFree(pDriver->rxDescriptorRing, toAllocate);
      E1000_ASSERT(internalRetCode == NO_ERROR,
                   "Failed to free RX descriptor ring",
                   internalRetCode);
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to create RX pool\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to create RX ring\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  return retCode;
}

static E_Return _AllocateDeviceTXResources(S_E1000Driver* pDriver)
{
  E_Return retCode;
  size_t   toAllocate;

  toAllocate = ALIGN_UP(pDriver->txBufferCount *
                        (sizeof(S_TXBufferDescriptor) +
                         sizeof(S_E1000TXBufferMetadata)),
                        KERNEL_PAGE_SIZE);

  /* Allocate the TX buffer descriptor ring */
  pDriver->txDescriptorIdx      = 0;
  pDriver->txDescriptorCleanIdx = 0;
  pDriver->txDescriptorWatchIdx = 0;
  pDriver->txDescriptorInUse    = 0;

  pDriver->txDescriptorRing = MemoryKernelAllocate(toAllocate,
                                                   MEMMGR_MAP_KERNEL |
                                                   MEMMGR_MAP_RW     |
                                                   MEMMGR_MAP_CACHE_DISABLED,
                                                   &retCode);
  if (retCode == NO_ERROR)
  {
    /* ALlocate the TX buffer descriptor addresses */
    pDriver->txDescriptorMeta = (void *)
      ((uintptr_t)pDriver->txDescriptorRing +
       (pDriver->txBufferCount * sizeof(S_TXBufferDescriptor)));
    pDriver->txDescriptorMemorySize = toAllocate;

    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X TX ring created",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to create TX ring\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
  return retCode;
}

static void _SetDeviceMAC(S_E1000Driver* pDriver, const uint64_t kMACAddress)
{
  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X setting MAC address to "
               "%02X:%02X:%02X:%02X:%02X:%02X",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function,
               (uint8_t)(kMACAddress & 0xFF),
               (uint8_t)((kMACAddress >> 8) & 0xFF),
               (uint8_t)((kMACAddress >> 16) & 0xFF),
               (uint8_t)((kMACAddress >> 24) & 0xFF),
               (uint8_t)((kMACAddress >> 32) & 0xFF),
               (uint8_t)((kMACAddress >> 40) & 0xFF));

  /* Write the MAC address to the device */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RAL),
               kMACAddress & 0xFFFFFFFF);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RAH),
               (kMACAddress >> 32) & 0x0000FFFF);
}

static void _ReleaseDeviceResources(S_E1000Driver* pDriver)
{
  _ReleaseDeviceRXResources(pDriver);
  _ReleaseDeviceTXResources(pDriver);
}

static void _ReleaseDeviceRXResources(S_E1000Driver* pDriver)
{
  E_Return retCode;

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Releasing RX resources for E1000 device %04X:%04X:%04X",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  /* Free the RX buffer descriptor ring */
  retCode = MemoryKernelFree(pDriver->rxDescriptorRing,
                             pDriver->rxDescriptorMemorySize);
  E1000_ASSERT(retCode == NO_ERROR,
               "Failed to free RX descriptor ring",
               retCode);

  /* Release the device RX buffers */
  retCode = NetManagerReleaseBufferPool(&pDriver->networkDevice);
  E1000_ASSERT(retCode == NO_ERROR,
               "Failed to release RX buffer pool",
               retCode);
}

static void _ReleaseDeviceTXResources(S_E1000Driver* pDriver)
{
  E_Return retCode;

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Releasing TX resources for E1000 device %04X:%04X:%04X",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  /* Free the TX buffer descriptor ring */
  retCode = MemoryKernelFree(pDriver->txDescriptorRing,
                             pDriver->txDescriptorMemorySize);
  E1000_ASSERT(retCode == NO_ERROR,
               "Failed to free TX descriptor ring",
               retCode);
}

static void _InitializeControlFlow(S_E1000Driver* pDriver)
{
  uint32_t regValue;
  uint32_t rxPBA;
  uint32_t lowWatermark;
  uint32_t highWatermark;

  if (pDriver->flowControl != FLOW_CONTROL_NONE)
  {
    /* Set flow mac address (as per IEEE 802.3x) */
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_FCAL), 0x00C28001);
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_FCAH), 0x00000100);

    /* Set the flow control timer (as per IEEE 802.3x) */
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_FCT), 0x00008808);
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_FCTTV), 0x0000FFFF);

    /* Set the low and high watermarks for the flow control */
    regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_PBA));
    rxPBA = (regValue & E1000_PBA_RX_MASK) >> E1000_PBA_RX_SHIFT;
    if (rxPBA == 0 || rxPBA == 0xFFFF)
    {
      rxPBA = E1000_PBA_RX_DEFAULT;
    }
    rxPBA *= 1024;
    lowWatermark  = ((rxPBA * 50) / 100) & 0x0000FFF8;
    highWatermark = ((rxPBA * 80) / 100) & 0x0000FFF8;

    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_FCRTH), highWatermark);
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_FCRTL), lowWatermark);

    /* Set the flow control bits in the control register */
    regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL));
    if (pDriver->flowControl == FLOW_CONTROL_RX_PAUSE)
    {
      regValue |= E1000_CTRL_RFCE;
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X flow control RX enabled",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    else if (pDriver->flowControl == FLOW_CONTROL_TX_PAUSE)
    {
      regValue |= E1000_CTRL_TFCE;
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X flow control TX enabled",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    else if (pDriver->flowControl == FLOW_CONTROL_RX_TX_PAUSE)
    {
      regValue |= E1000_CTRL_TFCE | E1000_CTRL_RFCE;
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X flow control RX/TX enabled",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL), regValue);
  }
  else
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X flow control disabled",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
}

static void _InitializeDeviceRX(S_E1000Driver* pDriver)
{
  S_RXBufferDescriptor*    pRXDescRing;
  S_E1000BufferDescriptor* pRXDescAddresses;
  uintptr_t                bufferAddress;
  uintptr_t                physAddr;
  uint32_t                 configReg;
  size_t                   i;

  /* Initialize the RX descriptor ring */
  bufferAddress    = (uintptr_t)pDriver->networkDevice.pBufferPool;
  pRXDescRing      = pDriver->rxDescriptorRing;
  pRXDescAddresses = pDriver->rxDescriptorAddresses;
  for (i = 0; i < pDriver->rxBufferCount; ++i)
  {
    pRXDescAddresses[i].virtAddr = (void *)(bufferAddress +
                                            i * pDriver->bufferSize);
    physAddr = (uintptr_t)MemoryMgrGetPhysAddr(
        (uintptr_t)pRXDescAddresses[i].virtAddr,
        NULL,
        NULL);
    pRXDescAddresses[i].physAddr = physAddr;
    _MMIOWrite64(&pRXDescRing[i].bufferAddress, physAddr);
    _MMIOWrite64(&pRXDescRing[i].length, pDriver->bufferSize);
  }

  /* Initialize the RX descriptor ring status */
  physAddr = (uintptr_t)MemoryMgrGetPhysAddr((uintptr_t)pRXDescRing,
                                             NULL,
                                             NULL);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDBAL),
               physAddr & 0xFFFFFFFF);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDBAH),
               (physAddr >> 32) & 0xFFFFFFFF);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDLEN),
               pDriver->rxBufferCount * sizeof(S_RXBufferDescriptor));
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDH), 0);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDT),
               pDriver->rxBufferCount - 1);

  configReg = E1000_RCTL_EN | E1000_RCTL_RDMTS_8 | E1000_RCTL_MO_36;
  if (pDriver->enableUnicastPromiscuous == true)
  {
    configReg |= E1000_RCTL_UPE;
  }
  if (pDriver->enableMulticastPromiscuous == true)
  {
    configReg |= E1000_RCTL_MPE;
  }
  if (pDriver->useJumboFrames == true)
  {
    configReg |= E1000_RCTL_LPE;
  }
  if (pDriver->loopbackMode != LOOPBACK_NONE)
  {
    configReg |= E1000_RCTL_LBM_PHY;
  }
  else
  {
    configReg |= E1000_RCTL_LBM_NONE;
  }
  if (pDriver->listenBroadcast == true)
  {
    configReg |= E1000_RCTL_BAM;
  }

  if (pDriver->bufferSize == 2048)
  {
    configReg |= E1000_RCTL_BSIZE_2048;
  }
  else if (pDriver->bufferSize == 1024)
  {
    configReg |= E1000_RCTL_BSIZE_1024;
  }
  else if (pDriver->bufferSize == 512)
  {
    configReg |= E1000_RCTL_BSIZE_512;
  }
  else if (pDriver->bufferSize == 256)
  {
    configReg |= E1000_RCTL_BSIZE_256;
  }
  else if (pDriver->bufferSize == 4096)
  {
    configReg |= E1000_RCTL_BSIZE_4096 | E1000_RCTL_BSEX;
  }
  else if (pDriver->bufferSize == 8192)
  {
    configReg |= E1000_RCTL_BSIZE_8192 | E1000_RCTL_BSEX;
  }
  else if (pDriver->bufferSize == 16384)
  {
    configReg |= E1000_RCTL_BSIZE_16384 | E1000_RCTL_BSEX;
  }
  else
  {
    configReg |= E1000_RCTL_BSIZE_2048;
    KERNEL_ERROR("E1000 device %04X:%04X:%04X invalid RX buffer size %u, "
                 "using default size\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function,
                 pDriver->bufferSize);
  }

  /* Write configuration to the device */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RCTL), configReg);

  /* Initialize the interrupt throttle */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDTR),
                pDriver->rxIntDelay & 0x0000FFFF);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RADV),
                pDriver->rxAbsIntDelay & 0x0000FFFF);

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X RX initialized",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static void _InitializeDeviceTX(S_E1000Driver* pDriver)
{
  uintptr_t physAddr;

  /* Initialize the TX descriptor ring status */
  physAddr = (uintptr_t)MemoryMgrGetPhysAddr(
      (uintptr_t)pDriver->txDescriptorRing,
      NULL,
      NULL);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDBAL),
               physAddr & 0xFFFFFFFF);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDBAH),
               (physAddr >> 32) & 0xFFFFFFFF);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDLEN),
               pDriver->txBufferCount * sizeof(S_TXBufferDescriptor));
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDH), 0);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDT), 0);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TCTL),
               E1000_TCTL_EN |
               E1000_TCTL_PSP |
               E1000_TCTL_CT_ETHER |
               E1000_TCTL_COLD_FULL |
               E1000_TCTL_RTLC);

  /* Initialize the interrupt throttle */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TIDV),
                pDriver->txIntDelay & 0x0000FFFF);
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TADV),
                pDriver->txAbsIntDelay & 0x0000FFFF);

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X TX initialized",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static E_Return _SetupDeviceInterrupts(S_E1000Driver* pDriver)
{
  E_Return err;
  S_PCIDevice *pDevice;

  pDevice = pDriver->pPCIDevice;

  if (pDriver->interruptLine <= 0)
  {
    pDriver->interruptLine = AllocateInterruptLine();
  }

  if (pDriver->interruptLine > 0)
  {
    /* Enable MSI for the device */
    err = PrepareDeviceMSIInterrupt(pDevice);
    if (err == NO_ERROR)
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000 device %04X:%04X:%04X using MSI interrupts",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      pDriver->useLegacyInterrupts = false;

      /* Setup MSI */
      err = PCISetupInterruptMSI(pDevice, pDriver->interruptLine, true);
      if (err != NO_ERROR)
      {
        KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to setup MSI\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
      }
    }
    else
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000 device %04X:%04X:%04X using legacy interrupts",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      pDriver->useLegacyInterrupts = true;

      /* We do not have the MSI capability, fall back to legacy interrupts */
      err = PrepareDeviceLegacyInterrupt(pDevice);
      if (err == NO_ERROR)
      {
        /* Setup the IRQ in the IOAPIC*/
        IOAPICSetupIRQ(pDevice->irqLine,
                       pDriver->interruptLine,
                       CPUGetLAPICId(),
                       IOAPIC_TRIGGER_MODE_LEVEL,
                       IOAPIC_POLARITY_HIGH);
      }
      else
      {
        KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to prepare legacy interrupts\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
      }
    }

    if (err == NO_ERROR)
    {
      /* Register the interrupt handler */
      err = InterruptRegister(pDriver->interruptLine, _E1000InterruptHandler);
      if (err == NO_ERROR)
      {
        /* Mask the interrupt in the device */
        _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMC), 0xFFFFFFFF);

        /* Setup the interrupt throttle */
        _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_ITR),
                     pDriver->intThrottle);

        InterruptSetEOI(pDriver->interruptLine);
      }
      else
      {
        if (pDriver->useLegacyInterrupts == true)
        {
          PCISetupInterruptMSI(pDevice, 0, false);
        }
        else
        {
          IOAPICDisableIRQ(pDevice->irqLine);
        }
        FreeInterruptLine(pDriver->interruptLine);
        pDriver->interruptLine = 0;
        KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to register handler\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
      }
    }
    else
    {
      FreeInterruptLine(pDriver->interruptLine);
      pDriver->interruptLine = 0;
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to setup interrupts\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to allocate interrupt line\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    err = ERR_EXCEEDED_LIMIT;
  }

  return err;
}

static void _SetPhyForced(S_E1000Driver* pDriver)
{
  uint32_t regValue;
  uint16_t phyValue;
  E_Return retCode;

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000 device %04X:%04X:%04X auto-negotiation disabled",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function);

  /* Setup the MAC forced speed */
  regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL));
  regValue |= E1000_CTRL_FRCDPLX | E1000_CTRL_FRCSPEED;
  regValue &= ~E1000_CTRL_ASDE;

  if (pDriver->duplexType == DUPLEX_FULL ||
      pDriver->linkSpeed == LINK_SPEED_1GBPS)
  {
    phyValue = E1000_PHY_CTRL_FD;
    regValue |= E1000_CTRL_FD;
  }
  else
  {
    phyValue = 0;
    regValue &= ~E1000_CTRL_FD;
  }

  regValue &= ~E1000_CTRL_SPD_SEL;
  if (pDriver->linkSpeed == LINK_SPEED_10MBPS)
  {
    phyValue = E1000_PHY_CTRL_SPEED_10;
    regValue |= E1000_CTRL_SPD_SEL_10;
  }
  else if (pDriver->linkSpeed == LINK_SPEED_100MBPS)
  {
    phyValue |= E1000_PHY_CTRL_SPEED_100;
    regValue |= E1000_CTRL_SPD_SEL_100;
  }
  else if (pDriver->linkSpeed == LINK_SPEED_1GBPS)
  {
    phyValue |= E1000_PHY_CTRL_SPEED_1000;
    regValue |= E1000_CTRL_SPD_SEL_1000;
  }
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL), regValue);

  /* Set the PHY */
  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000_PHY_CTRL, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue &= ~(E1000_PHY_CTLR_SPEED_MASK |
                  E1000_PHY_CTRL_DUPLEX_MASK |
                  E1000_PHY_CTRL_AN_ENABLE);
    regValue |= phyValue | E1000_PHY_CTRL_RESET;
    retCode = _WriteMDIO(pDriver, E1000_PHY_CTRL, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
    else
    {
      /* Wait for the PHY to be ready */
      do
      {
        retCode = _ReadMDIO(pDriver, E1000_PHY_CTRL, (uint16_t*)&regValue);
      } while (retCode == NO_ERROR && (regValue & E1000_PHY_CTRL_RESET) != 0);
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000 device %04X:%04X:%04X PHY set to speed %u, duplex "
                    "%s",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function,
                    pDriver->linkSpeed,
                    (pDriver->duplexType == DUPLEX_FULL) ?
                    "full" : "half");
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }
}

static void _SetPhyAutoNeg(S_E1000Driver* pDriver)
{
  uint32_t regValue;
  E_Return retCode;

  if (pDriver->autoNegMask != E1000_AUTONEG_MASK_OFF &&
      (pDriver->linkSpeed != LINK_SPEED_AUTONEG ||
        pDriver->duplexType != DUPLEX_AUTONEG))
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X auto-negotiation settings"
                  " conflict, using auto-negotiation\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }

  /* Setup the MAC auto-negotiation speed */
  regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL));
  regValue &= ~(E1000_CTRL_FRCSPEED | E1000_CTRL_FRCDPLX | E1000_CTRL_FD);
  regValue |= E1000_CTRL_ASDE;
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL), regValue);

  /* Set the PHY */
  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000_PHY_AUTONEG_ADV, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue &= ~(E1000_PHY_ANA_1000BASE_X_FULL |
                  E1000_PHY_ANA_1000BASE_X_HALF |
                  E1000_PHY_ANA_100BASE_TX_FULL |
                  E1000_PHY_ANA_100BASE_TX_HALF |
                  E1000_PHY_ANA_10BASE_T_FULL |
                  E1000_PHY_ANA_10BASE_T_HALF);

    if (pDriver->deviceId == E1000_DEV_ID_82544EI_COPPER ||
        pDriver->deviceId == E1000_DEV_ID_82544GC_COPPER ||
        pDriver->deviceId == E1000_DEV_ID_82544GC_LOM)
    {
      regValue |= E1000_PHY_ANA_1000BASE_X_FULL |
                  E1000_PHY_ANA_1000BASE_X_HALF;
    }
    else
    {
      if (pDriver->autoNegMask & E1000_AUTONEG_MASK_100_FULL)
      {
        regValue |= E1000_PHY_ANA_100BASE_TX_FULL;
      }
      if (pDriver->autoNegMask & E1000_AUTONEG_MASK_100_HALF)
      {
        regValue |= E1000_PHY_ANA_100BASE_TX_HALF;
      }
      if (pDriver->autoNegMask & E1000_AUTONEG_MASK_10_FULL)
      {
        regValue |= E1000_PHY_ANA_10BASE_T_FULL;
      }
      if (pDriver->autoNegMask & E1000_AUTONEG_MASK_10_HALF)
      {
        regValue |= E1000_PHY_ANA_10BASE_T_HALF;
      }
    }

    retCode = _WriteMDIO(pDriver, E1000_PHY_AUTONEG_ADV, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }

  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000_PHY_1000T_CTRL, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue &= ~(E1000_PHY_1000T_1000BASE_T_FULL |
                  E1000_PHY_1000T_1000BASE_T_HALF);

    if (pDriver->deviceId == E1000_DEV_ID_82544EI_COPPER ||
        pDriver->deviceId == E1000_DEV_ID_82544GC_COPPER ||
        pDriver->deviceId == E1000_DEV_ID_82544GC_LOM)
    {
      regValue |= E1000_PHY_1000T_1000BASE_T_FULL |
                  E1000_PHY_1000T_1000BASE_T_HALF;
    }
    else
    {
      if (pDriver->autoNegMask & E1000_AUTONEG_MASK_1000_FULL)
      {
        regValue |= E1000_PHY_1000T_1000BASE_T_FULL;
      }
      if (pDriver->autoNegMask & E1000_AUTONEG_MASK_1000_HALF)
      {
        regValue |= E1000_PHY_1000T_1000BASE_T_HALF;
      }
    }
    retCode = _WriteMDIO(pDriver, E1000_PHY_1000T_CTRL, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }

  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000_PHY_CTRL, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue |= E1000_PHY_CTRL_AN_ENABLE | E1000_PHY_CTRL_AN_RESTART;
    retCode = _WriteMDIO(pDriver, E1000_PHY_CTRL, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
    else
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000 device %04X:%04X:%04X PHY set to auto-negotiation",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }
}

static void _SetPhyNonCopper(S_E1000Driver* pDriver)
{
  uint32_t regValue;

  /* Setup the MAC for non-copper media */
  regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL));
  regValue &= ~(E1000_CTRL_FRCSPEED | E1000_CTRL_FRCDPLX | E1000_CTRL_SPD_SEL);
  regValue |= E1000_CTRL_ASDE | E1000_CTRL_FD;
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL), regValue);

  /* Set the transmit word */
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TXCW), 0);

  regValue = E1000_TXCW_FD | E1000_TXCW_ANE | E1000_TXCW_TXC;

  /* Advertise flow control capabilities based on flow control settings */
  if (pDriver->flowControl == FLOW_CONTROL_RX_PAUSE ||
      pDriver->flowControl == FLOW_CONTROL_RX_TX_PAUSE) {
      regValue |= E1000_TXCW_PAUSE;
  }
  if (pDriver->flowControl == FLOW_CONTROL_TX_PAUSE ||
      pDriver->flowControl == FLOW_CONTROL_RX_TX_PAUSE) {
      regValue |= E1000_TXCW_ASYM_PD;
  }

  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TXCW), regValue);

  /* For SerDes, we must inform the MAC to bypass optical */
  regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL_EXT));
  regValue &= ~E1000_CTRL_EXT_LINK_MODE_MASK;
  regValue |= E1000_CTRL_EXT_LINK_MODE_DIRECT;
  _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL_EXT), regValue);

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000 device %04X:%04X:%04X PHY set to non-copper media",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function);
}

static void _InitializePhy(S_E1000Driver* pDriver)
{
  if (pDriver->mediaType == MEDIA_COPPER)
  {
    if (pDriver->autoNegMask == E1000_AUTONEG_MASK_OFF &&
        pDriver->linkSpeed != LINK_SPEED_AUTONEG &&
        pDriver->duplexType != DUPLEX_AUTONEG)
    {
      _SetPhyForced(pDriver);
    }
    else
    {
      _SetPhyAutoNeg(pDriver);
    }
  }
  else
  {
    _SetPhyNonCopper(pDriver);
  }

  /* Wait for the PHY to complete auto-negotiation */
  SleepNs(E1000_PHY_INIT_TIMEOUT_NS, NULL);
}

static void _SetupMulticastTable(S_E1000Driver* pDriver)
{
  size_t i;

  /* Clear the multicast table */
  for (i = 0; i < E1000_MULTICAST_TABLE_SIZE; ++i)
  {
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_MTA + i * 4), 0);
  }

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X multicast table initialized",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static void _SetupVLAN(S_E1000Driver* pDriver)
{
  size_t i;

  /* Disable VLAN filtering  */
  for (i = 0; i < E1000_VLAN_FILTER_TABLE_SIZE; ++i)
  {
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_VFTA + (i * 4)), 0);
  }

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X VLAN filtering disabled",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static E_Return _DevUplink(S_NetworkDevice* pNetworkDevice)
{
  E_Return       retCode;
  uint32_t       regValue;
  S_E1000Driver* pDriver;

  pDriver = (S_E1000Driver *)pNetworkDevice->pDriverData;

  KERNEL_LOCK(pDriver->linkLock);

  if (pDriver->isInitialized == false)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X device uplink requested",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);

    retCode = _AllocateDeviceResources(pDriver);
    if (retCode == NO_ERROR)
    {
      /* Reset the controller */
      _ResetDevice(pDriver);

      /* Initialize the device's RXChecksum settings */
      regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_RXCSUM));
      regValue &= E1000_RXCSUM_CLEAR_MASK;
      if (pDriver->enableXSumRx == true)
      {
        regValue |= E1000_RXCSUM_IPOFL |
                    E1000_RXCSUM_TUOFL |
                    E1000_RXCSUM_IPV6OFL |
                    0x10;
        _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RXCSUM), regValue);
      }

      /* Initialize the device's control flow */
      _InitializeControlFlow(pDriver);

      /* Initialize the device's RX and TX */
      _InitializeDeviceRX(pDriver);
      _InitializeDeviceTX(pDriver);

      /* Setup the multicas table */
      _SetupMulticastTable(pDriver);

      /* Setup VLAN */
      _SetupVLAN(pDriver);

      pDriver->isInitialized = true;
      CPUMemoryFenceRelease();

      /* Initialize the device's PHY */
      regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL));
      regValue |= E1000_CTRL_SLU | E1000_CTRL_ASDE;
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL), regValue);

      _InitializePhy(pDriver);

      /* Start the periodic stats task */
      pDriver->statTaskHandle = NetManagerAddPeriodicTask(_UpdateStatistics,
                                                pDriver,
                                                E1000_STAT_UPDATE_INTERVAL_NS);

      /* Enable all interrupts */
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMS),
                   E1000_ENABLED_INTERRUPTS);

      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000 device %04X:%04X:%04X device uplink completed",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
    else
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to allocate resources\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X device already initialized",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = NO_ERROR;
  }

  KERNEL_UNLOCK(pDriver->linkLock);


  return retCode;
}

static E_Return _DevDownlink(S_NetworkDevice* pNetworkDevice)
{
  E_Return       retCode;
  S_E1000Driver* pDriver;
  uint32_t       regValue;

  pDriver = (S_E1000Driver *)pNetworkDevice->pDriverData;

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X device downlink requested",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  KERNEL_LOCK(pDriver->linkLock);
  if (pDriver->isInitialized == true)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X device downlink in progress",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);

    pDriver->isInitialized = false;

    /* Disable all interrupts */
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMC), 0xFFFFFFFF);

    KERNEL_UNLOCK(pDriver->linkLock);

    /* Wait until all pending transactions are completed */
    retCode = _WaitForTransactionCompletion(pDriver);
    if (retCode == NO_ERROR)
    {
      /* Release the device's resources */
      _ReleaseDeviceResources(pDriver);

      /* Shut down the device */
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMC), 0xFFFFFFFF);

      /* Disable RX and TX */
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RCTL), 0);
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TCTL), E1000_TCTL_PSP);

      /* Clear RX and TX descriptors */
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDH), 0);
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDT), 0);
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDH), 0);
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDT), 0);

      /* Wait for the disable to take effect */
      SleepNs(E1000_RESET_TIMEOUT_NS, NULL);

      regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_CTRL));
      regValue &= ~(E1000_CTRL_ASDE | E1000_CTRL_SLU);
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_CTRL), regValue);

      NetManagerRemovePeriodicTask(pDriver->statTaskHandle);

      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X device downlink completed",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    else
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to wait for transaction"
                   " completion\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }

  }
  else
  {
    KERNEL_UNLOCK(pDriver->linkLock);
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X device already downlink",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = NO_ERROR;
  }

  return retCode;
}

static E_Return _WaitForTransactionCompletion(S_E1000Driver* pDriver)
{
  E_Return retCode;
  size_t   waitCount;
  bool     isNotComplete;

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X waiting for pending transactions to"
               " complete",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  waitCount = 0;
  while ((pDriver->rxOperationRunning == true ||
          pDriver->txOperationRunning == true ||
          pDriver->txCompleteOperationRunning == true ||
          pDriver->linkOperationRunning == true ||
          pDriver->ioctlOperationRunning == true) &&
          waitCount < E1000_TRANSACTION_WAIT_COUNT)
  {
    SleepNs(E1000_TRANSACTION_WAIT_NS, NULL);
    ++waitCount;
  }

  if (waitCount < E1000_TRANSACTION_WAIT_COUNT)
  {
    do
    {
      SleepNs(E1000_TRANSACTION_WAIT_NS, NULL);
      /* Clear the pending transmit */
      _CompleteTransmit(pDriver, &isNotComplete);
    } while (isNotComplete == true && waitCount < E1000_TRANSACTION_WAIT_COUNT);

    if (isNotComplete == true)
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to complete pending "
                   "transactions\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      retCode = ERR_EXCEEDED_LIMIT;
    }
    else
    {
      retCode = NO_ERROR;
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to complete pending "
                 "transactions\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = ERR_EXCEEDED_LIMIT;
  }

  return retCode;
}

static ssize_t _TransmitPacket(struct S_NetworkDevice* pDevice,
                               const void*             kpBuffer,
                               const size_t            kSize,
                               const bool              kIsLastOfBurst,
                               const bool              kIsBlocking)
{
  S_E1000Driver* pDriver;
  ssize_t        sentBytes;
  uintptr_t      physAddr;
  uint32_t       tailIdx;
  uint32_t       packetSize;
  uint64_t       regValue;
  uint32_t       requiredPackets;
  uint32_t       availableDescriptors;

  pDriver = (S_E1000Driver *)pDevice->pDriverData;

  pDriver->txOperationRunning = true;
  CPUMemoryFenceRelease();

  KERNEL_LOCK(pDriver->linkLock);
  if (pDriver->isInitialized == true)
  {
    KERNEL_LOCK(pDriver->txRingLock);
    sentBytes = 0;
    tailIdx = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_TDT));
    if (pDriver->txDescriptorIdx >= pDriver->txDescriptorCleanIdx)
    {
      availableDescriptors = (pDriver->txBufferCount - 1) -
                             (pDriver->txDescriptorIdx -
                              pDriver->txDescriptorCleanIdx);
    }
    else
    {
      availableDescriptors = (pDriver->txDescriptorCleanIdx -
                              pDriver->txDescriptorIdx) -
                              1;
    }
    requiredPackets = (kSize + E1000_TX_BUFFER_SIZE - 1) / E1000_TX_BUFFER_SIZE;

    if (requiredPackets <= availableDescriptors)
    {
      /* Check if we have enough space in the transmit ring */
      while ((size_t)sentBytes < kSize)
      {
        /* Get the buffer address */
        physAddr = (uintptr_t)MemoryMgrGetPhysAddr(
            (uintptr_t)((uint8_t *)kpBuffer + sentBytes),
            NULL,
            NULL);

        /* Get the packet size and setup driver */
        packetSize = MIN(kSize - sentBytes, E1000_TX_BUFFER_SIZE);

        /* Send the packet */
        _MMIOWrite64(&pDriver->txDescriptorRing[tailIdx].bufferAddress,
                     physAddr);

        /* Save the metadata for the last packet only */
        sentBytes += packetSize;
        regValue = packetSize | E1000_TX_CMD_IDE;
        if ((size_t)sentBytes == kSize)
        {
          regValue |= E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS;

          pDriver->txDescriptorMeta[tailIdx].virtAddr = (uintptr_t)kpBuffer;
          pDriver->txDescriptorMeta[tailIdx].length = kSize;
        }
        else
        {
          pDriver->txDescriptorMeta[tailIdx].virtAddr = (uintptr_t)NULL;
          pDriver->txDescriptorMeta[tailIdx].length = 0;
        }

        /* Manage RS to indicate clean on last packet */
        ++pDriver->txDescriptorInUse;
        if (pDriver->txDescriptorInUse == E1000_TX_MAX_BUDGET ||
            (kIsLastOfBurst == true && (size_t)sentBytes == kSize))
        {
          regValue |= E1000_TX_CMD_RS;
          pDriver->txDescriptorWatchIdx = tailIdx;
          pDriver->txDescriptorInUse = 0;
        }

        _MMIOWrite64(&pDriver->txDescriptorRing[tailIdx].length, regValue);

        /* Update the transmit descriptor tail */
        tailIdx = (tailIdx + 1) % pDriver->txBufferCount;
        pDriver->txDescriptorIdx = tailIdx;
      }

      CPUMemoryFenceAcquire();
      /* Update the transmit descriptor tail */
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_TDT), tailIdx);
    }
    else
    {
      if (kIsBlocking == false)
      {
        ++pDriver->stats.txDropped;
      }
      sentBytes = -EBUSY;
    }

    KERNEL_UNLOCK(pDriver->txRingLock);
  }
  else
  {
    sentBytes = -ENODEV;
  }
  KERNEL_UNLOCK(pDriver->linkLock);

  CPUMemoryFenceAcquire();
  pDriver->txOperationRunning = false;

  return sentBytes;
}

static ssize_t _IOControl(struct S_NetworkDevice* pDevice,
                          const uint32_t          kCommand,
                          void*                   pArg)
{
  S_E1000Driver* pDriver;
  ssize_t        retCode;

  pDriver = (S_E1000Driver *)pDevice->pDriverData;

  pDriver->ioctlOperationRunning = true;
  CPUMemoryFenceRelease();

  KERNEL_LOCK(pDriver->linkLock);
  if (pDriver->isInitialized == true)
  {
    KERNEL_UNLOCK(pDriver->linkLock);

    switch (kCommand)
    {
      case VFS_IOCTL_NET_IFACE_UP:
        retCode = _DevUplink(pDevice);
        break;
      case VFS_IOCTL_NET_IFACE_DOWN:
        retCode = _DevDownlink(pDevice);
        break;
      case VFS_IOCTL_NET_GET_STATS:
        retCode = _IOControlGetStats(pDriver, pArg);
        break;
      case VFS_IOCTL_NET_RESET_STATS:
        retCode = _IOControlResetStats(pDriver);
        break;
      case VFS_IOCTL_NET_SET_MAC_ADDRESS:
        retCode = _IOControlSetMACAddress(pDriver, pArg);
        break;
      case VFS_IOCTL_NET_GET_MAC_ADDRESS:
        retCode = _IOControlGetMACAddress(pDriver, pArg);
        break;
      case VFS_IOCTL_NET_SET_LOOPBACK_MODE:
        retCode = _IOControlSetLoopbackMode(pDriver, pArg);
        break;
      case VFS_IOCTL_NET_GET_LINK_STATUS:
        retCode = _IOControlGetLinkStatus(pDriver, pArg);
        break;
      case VFS_IOCTL_NET_SET_AUTONEG:
        retCode = _IOControlSetAutoNeg(pDriver);
        break;
      case VFS_IOCTL_NET_FORCE_SPD_DPX:
        retCode = _IOControlForceSpeedDuplex(pDriver, pArg);
        break;
      default:
        KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl command 0x%08X not "
                     "supported\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function,
                     kCommand);
        retCode = -EINVAL;
        break;
    }
  }
  else
  {
    KERNEL_UNLOCK(pDriver->linkLock);
    KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl requested on uninitialized"
                 " device\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -ENODEV;
  }

  CPUMemoryFenceAcquire();
  pDriver->ioctlOperationRunning = false;

  return retCode;
}

static bool _E1000InterruptHandler(void)
{
  S_KernelThread* pCurrentThread;
  uint32_t        intId;
  uint32_t        status;
  S_E1000Driver*  pDriver;

  /* Get the current thread */
  pCurrentThread = GetCurrentThread();
  intId = CPUGetContextInterruptNumber(pCurrentThread);

  /* Find the E1000 driver associated with this interrupt */
  KERNEL_LOCK(sDriverListLock);
  pDriver = spDriverList;
  while (pDriver != NULL)
  {
    if (pDriver->interruptLine == (int32_t)intId)
    {
      break;
    }
    pDriver = pDriver->pNext;
  }
  KERNEL_UNLOCK(sDriverListLock);

  if (pDriver != NULL)
  {
    /* Mask the interrupt in the device */
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMC), 0xFFFFFFFF);

    status = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_ICR));
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 %04X:%04X:%04X interrupt status: 0x%08X",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function,
                 status);

    if ((status & E1000_ICR_LSC) == E1000_ICR_LSC)
    {
      NetManagerSubmitTask(_HandleLinkStatusChange, pDriver, NULL);
    }
    if ((status & E1000_ICR_RXT0) == E1000_ICR_RXT0 ||
        (status & E1000_ICR_TXDW) == E1000_ICR_TXDW)
    {
      NetManagerSubmitTask(_HandleInterruptWorker, pDriver, NULL);
    }
  }

  /* Acknowledge the interrupt in the IOAPIC */
  InterruptSetEOI(intId);

  return false;
}

static void _HandleInterruptWorker(void* pDriverPtr)
{
  S_E1000Driver* pDriver;

  pDriver = (S_E1000Driver *)pDriverPtr;

  KERNEL_LOCK(pDriver->linkLock);
  if (pDriver->isInitialized == true)
  {
    KERNEL_UNLOCK(pDriver->linkLock);

    /* Handle transmit Complete */
    _CompleteTransmit(pDriver, NULL);

    /* Handle the received packet */
    _HandleReceivedPacket(pDriver);

    /* Re-enable the interrupt in the device */
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMS),
                 E1000_ENABLED_INTERRUPTS);
  }
  else
  {
    KERNEL_UNLOCK(pDriver->linkLock);
    KERNEL_ERROR("E1000 device %04X:%04X:%04X interrupt requested on "
                 "uninitialized device\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
}

static void _HandleLinkStatusChange(void* pDriverPtr)
{
  uint32_t       status;
  S_E1000Driver* pDriver;

  pDriver = (S_E1000Driver *)pDriverPtr;

  pDriver->linkOperationRunning = true;
  CPUMemoryFenceRelease();

  /* Read the link status */
  status = _MMIORead32(E1000_REG(pDriver->barAddress, E1000_STATUS));
  if (status & E1000_STATUS_LU)
  {
    pDriver->isLinkUp = true;
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X link is up",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
  else
  {
    pDriver->isLinkUp = false;
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000 device %04X:%04X:%04X link is down",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  /* Enable the interrupt in the device */
  KERNEL_LOCK(pDriver->linkLock);
  if (pDriver->isInitialized == true)
  {
    /* Re-enable the interrupt in the device */
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_IMS),
                 E1000_ENABLED_INTERRUPTS);
  }
  KERNEL_UNLOCK(pDriver->linkLock);

  CPUMemoryFenceAcquire();
  pDriver->linkOperationRunning = false;
}

static void _CompleteTransmit(S_E1000Driver* pDriver, bool* pIsMoreToClean)
{
  uint32_t                status;
  uint32_t                watchIdx;
  uint32_t                cleanIdx;
  uint32_t                nextIdx;
  S_E1000TXBufferMetadata freeBuffers[E1000_TX_CLEAN_BUDGET];
  uint32_t                freeCount;
  S_KernelThread*         pCurThread;

  pDriver->txCompleteOperationRunning = true;
  CPUMemoryFenceRelease();

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
              MODULE_NAME,
              "E1000 device %04X:%04X:%04X transmit complete",
              pDriver->pPCIDevice->bus,
              pDriver->pPCIDevice->slot,
              pDriver->pPCIDevice->function);

  do
  {
    freeCount = 0;

    /* Check if there is anything to clean */
    KERNEL_LOCK(pDriver->txRingLock);
    nextIdx  = pDriver->txDescriptorIdx;
    watchIdx = pDriver->txDescriptorWatchIdx;
    cleanIdx = pDriver->txDescriptorCleanIdx;

    if (cleanIdx != nextIdx)
    {
      /* Check if the next to watch is done processing */
      status = _MMIORead32(&pDriver->txDescriptorRing[watchIdx].status);
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X checking descriptor %u status: 0x%08X",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function,
                  watchIdx,
                  status);
      if ((status & E1000_TXD_STAT_DD) == E1000_TXD_STAT_DD)
      {
        while (cleanIdx != nextIdx && freeCount < E1000_TX_CLEAN_BUDGET)
        {
          KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                      MODULE_NAME,
                      "E1000 device %04X:%04X:%04X cleaning descriptor %u",
                      pDriver->pPCIDevice->bus,
                      pDriver->pPCIDevice->slot,
                      pDriver->pPCIDevice->function,
                      cleanIdx);
          /* Clean the descriptor */
          _MMIOWrite32(&pDriver->txDescriptorRing[cleanIdx].status, 0);

          /* Release the buffer */
          if (pDriver->txDescriptorMeta[cleanIdx].length != 0)
          {
            KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                        MODULE_NAME,
                        "E1000 device %04X:%04X:%04X unring buffer at 0x%p of size %u",
                        pDriver->pPCIDevice->bus,
                        pDriver->pPCIDevice->slot,
                        pDriver->pPCIDevice->function,
                        pDriver->txDescriptorMeta[cleanIdx].virtAddr,
                        pDriver->txDescriptorMeta[cleanIdx].length);
            freeBuffers[freeCount].virtAddr =
                pDriver->txDescriptorMeta[cleanIdx].virtAddr;
            freeBuffers[freeCount].length =
                pDriver->txDescriptorMeta[cleanIdx].length;
            ++freeCount;
          }

          if (cleanIdx == watchIdx)
          {
            cleanIdx = (cleanIdx + 1) % pDriver->txBufferCount;
            break;
          }
          else
          {
            cleanIdx = (cleanIdx + 1) % pDriver->txBufferCount;
          }
        }
      }
      pDriver->txDescriptorCleanIdx = cleanIdx;
    }
    KERNEL_UNLOCK(pDriver->txRingLock);

    /* Release the buffers */
    for (nextIdx = 0; nextIdx < freeCount; ++nextIdx)
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X releasing buffer at 0x%p of size %u",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function,
                  freeBuffers[nextIdx].virtAddr,
                  freeBuffers[nextIdx].length);
      NetManagerReleaseTransmitBuffer((void *)freeBuffers[nextIdx].virtAddr,
                                      freeBuffers[nextIdx].length);
    }
    /* If we haven't cleaned enough buffers, respin */
    if (freeCount == E1000_TX_CLEAN_BUDGET)
    {
      pCurThread = GetCurrentThread();
      CPUSaveContextAndSchedule(pCurThread->pVCpu);
    }

  } while (freeCount == E1000_TX_CLEAN_BUDGET);

  if (pIsMoreToClean != NULL)
  {
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000 device %04X:%04X:%04X transmit complete, more to clean: %s %x %x",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function,
                (pDriver->txDescriptorCleanIdx !=
                 pDriver->txDescriptorIdx) ? "true" : "false",
                pDriver->txDescriptorCleanIdx,
                pDriver->txDescriptorIdx);
    *pIsMoreToClean = (pDriver->txDescriptorCleanIdx !=
                       pDriver->txDescriptorIdx);
  }

  CPUMemoryFenceAcquire();
  pDriver->txCompleteOperationRunning = false;
}

static void _HandleReceivedPacket(S_E1000Driver* pDriver)
{
  uint32_t               pktIdx;
  uint16_t               pktSize;
  uint32_t               status;
  S_RXBufferDescriptor*  pRXDescRing;
  S_NetworkPacketBuffer  packetBuffer;
  bool                   isFreeBufferAvailable;
  uint32_t               tail;
  S_NetworkFragmentList* pFragmentList;
  uint32_t               fragmentIdx;
  uintptr_t              physAddr;
  uintptr_t              virtAddr;
  uint32_t               pollBudget;
  S_KernelThread*        pCurThread;

  pktIdx = pDriver->rxDescriptorIdx;
  pRXDescRing = pDriver->rxDescriptorRing;
  pFragmentList = pDriver->rxFragmentList;

  pDriver->rxOperationRunning = true;
  CPUMemoryFenceRelease();

  do
  {
    /* Go through all descriptors to check the reception status */
    status = _MMIORead32(&pRXDescRing[pktIdx].status) & 0xFF;
    pollBudget = 0;
    while ((status & E1000_RXD_STAT_DD) != 0 &&
          pollBudget < E1000_POLL_MAX_BUDGET)
    {
      /* Process the received packet */
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X processing received packet",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);

      /* Get the packet size and buffer addresses */
      pktSize = _MMIORead32(&pRXDescRing[pktIdx].length) & 0xFFFF;
      physAddr = pDriver->rxDescriptorAddresses[pktIdx].physAddr;
      virtAddr = (uintptr_t)pDriver->rxDescriptorAddresses[pktIdx].virtAddr;

      /* Compose the fragment list */
      if (pFragmentList == NULL)
      {
        pFragmentList = NetManagerAllocateFragmentList();
        if (pFragmentList != NULL)
        {
          pFragmentList->pFragments[0].buffer.virtAddr = virtAddr;
          pFragmentList->pFragments[0].buffer.physAddr = physAddr;
          pFragmentList->pFragments[0].size = pktSize;
          pFragmentList->fragmentCount = 1;

          pDriver->rxFragmentList = pFragmentList;
        }
        else
        {
          KERNEL_ERROR("E1000 device %04X:%04X:%04X failed to allocate "
                      "fragment list\n",
                      pDriver->pPCIDevice->bus,
                      pDriver->pPCIDevice->slot,
                      pDriver->pPCIDevice->function);
        }
      }
      else
      {
        fragmentIdx = pFragmentList->fragmentCount;
        pFragmentList->pFragments[fragmentIdx].buffer.virtAddr = virtAddr;
        pFragmentList->pFragments[fragmentIdx].buffer.physAddr = physAddr;
        pFragmentList->pFragments[fragmentIdx].size = pktSize;
        ++pFragmentList->fragmentCount;
      }

      /* If the fragment list is complete, process it, otherwise drop the packet
      */
      if (pFragmentList != NULL)
      {
        /* Get a free packet buffer */
        isFreeBufferAvailable = FQueuePop(pDriver->networkDevice.pFreeBufferQueue,
                                          &packetBuffer);

        /* Check if the packet is complete */
        if ((status & E1000_RXD_STAT_EOP) == E1000_RXD_STAT_EOP ||
            pFragmentList->fragmentCount >= NET_MGR_MAX_FRAGMENT_COUNT)
        {
          /* Process the complete packet */
          NetManagerProcessReceivedPacket(pFragmentList,
                                          &pDriver->networkDevice);
          pDriver->rxFragmentList = NULL;
          pFragmentList = NULL;
        }
      }
      else
      {
        isFreeBufferAvailable = false;
      }

      if (isFreeBufferAvailable == true)
      {
        /* Replace the received packet buffer */
        pDriver->rxDescriptorAddresses[pktIdx].virtAddr = (void *)
            packetBuffer.virtAddr;
        pDriver->rxDescriptorAddresses[pktIdx].physAddr =
            packetBuffer.physAddr;
        _MMIOWrite64(&pRXDescRing[pktIdx].bufferAddress, packetBuffer.physAddr);
        _MMIOWrite64(&pRXDescRing[pktIdx].length, pDriver->bufferSize);
      }
      else
      {
        if (pDriver->rxFragmentList != NULL)
        {
          /* Drop the fragment list */
          NetManagerProcessReceivedPacket(pDriver->rxFragmentList,
                                          &pDriver->networkDevice);
          pDriver->rxFragmentList = NULL;
          pFragmentList = NULL;
        }

        /* Backpressure: drop the packet */
        ++pDriver->stats.rxDropped;
        _MMIOWrite64(&pRXDescRing[pktIdx].length, pDriver->bufferSize);
        KERNEL_ERROR("E1000 device %04X:%04X:%04X no free buffer available,"
                    " dropping packet\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
      }

      /* Move to the next descriptor */
      pktIdx = (pktIdx + 1) % pDriver->rxBufferCount;
      status = _MMIORead32(&pRXDescRing[pktIdx].status) & 0xFF;

      ++pollBudget;
    }

    /* Update the tail index */
    if (pktIdx == 0)
    {
      tail = pDriver->rxBufferCount - 1;
    }
    else
    {
      tail = pktIdx - 1;
    }
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000_RDT), tail & 0xFFFF);

    /* Update the receive descriptor index */
    pDriver->rxDescriptorIdx = pktIdx;

    if (pollBudget == E1000_POLL_MAX_BUDGET)
    {
      /* We still have packets to read, simply yield */
      pCurThread = GetCurrentThread();
      CPUSaveContextAndSchedule(pCurThread->pVCpu);
    }
  } while (pollBudget == E1000_POLL_MAX_BUDGET);

  CPUMemoryFenceAcquire();
  pDriver->rxOperationRunning = false;
}

static void _UpdateStatistics(void* pDriver)
{
  uint32_t       regLow;
  uint32_t       regHigh;
  uintptr_t      bar;
  S_E1000Driver* pDev;

  pDev = (S_E1000Driver*)pDriver;
  bar = (pDev->barAddress);

  KERNEL_LOCK(pDev->statsLock);

  /* General statistics */
  pDev->stats.rxPackets += _MMIORead32(E1000_REG(bar, E1000_GPRC));
  pDev->stats.txPackets += _MMIORead32(E1000_REG(bar, E1000_GPTC));
  regLow  = _MMIORead32(E1000_REG(bar, E1000_GORCL));
  regHigh = _MMIORead32(E1000_REG(bar, E1000_GORCH));
  pDev->stats.rxBytes   += regLow | ((uint64_t)regHigh << 32);
  regLow  = _MMIORead32(E1000_REG(bar, E1000_GOTCL));
  regHigh = _MMIORead32(E1000_REG(bar, E1000_GOTCH));
  pDev->stats.txBytes   += regLow | ((uint64_t)regHigh << 32);

  pDev->stats.multicast  += _MMIORead32(E1000_REG(bar, E1000_MPRC));
  pDev->stats.collisions += _MMIORead32(E1000_REG(bar, E1000_COLC));

  /* RX Errors */
  pDev->stats.rxLengthErrors += _MMIORead32(E1000_REG(bar, E1000_RLEC));
  pDev->stats.rxOverErrors   += _MMIORead32(E1000_REG(bar, E1000_RNBC));
  pDev->stats.rxCrcErrors    += _MMIORead32(E1000_REG(bar, E1000_CRCERRS));
  pDev->stats.rxFrameErrors  += _MMIORead32(E1000_REG(bar, E1000_ALGNERRC));
  pDev->stats.rxFifoErrors   += _MMIORead32(E1000_REG(bar, E1000_MPC));
  pDev->stats.rxMissedErrors = pDev->stats.rxFifoErrors;
  pDev->stats.rxErrors = pDev->stats.rxLengthErrors +
                            pDev->stats.rxOverErrors +
                            pDev->stats.rxCrcErrors +
                            pDev->stats.rxFrameErrors +
                            pDev->stats.rxFifoErrors;

  /* TX Errors */
  pDev->stats.txAbortedErrors   += _MMIORead32(E1000_REG(bar, E1000_ECOL));
  pDev->stats.txCarrierErrors   += _MMIORead32(E1000_REG(bar, E1000_TNCRS));
  pDev->stats.txFifoErrors      += _MMIORead32(E1000_REG(bar, E1000_LCOL));
  pDev->stats.txWindowErrors    += pDev->stats.txFifoErrors;
  pDev->stats.txErrors = pDev->stats.txAbortedErrors +
                            pDev->stats.txCarrierErrors +
                            pDev->stats.txFifoErrors;

  KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000 device %04X:%04X:%04X updated statistics: "
               "rxPackets=%llu, txPackets=%llu, rxBytes=%llu, txBytes=%llu, "
               "rxErrors=%llu, txErrors=%llu",
               pDev->pPCIDevice->bus,
               pDev->pPCIDevice->slot,
               pDev->pPCIDevice->function,
               pDev->stats.rxPackets,
               pDev->stats.txPackets,
               pDev->stats.rxBytes,
               pDev->stats.txBytes,
               pDev->stats.rxErrors,
               pDev->stats.txErrors);

  KERNEL_UNLOCK(pDev->statsLock);
}

static ssize_t _IOControlGetStats(S_E1000Driver* pDriver, void* pArg)
{
  ssize_t retCode;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(S_NetworkStatistics),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RW) == true)
  {
    _UpdateStatistics(pDriver);
    KERNEL_LOCK(pDriver->statsLock);
    memcpy(pArg, &pDriver->stats, sizeof(S_NetworkStatistics));
    KERNEL_UNLOCK(pDriver->statsLock);
    retCode = sizeof(S_NetworkStatistics);
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl get stats called with "
                 "invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlResetStats(S_E1000Driver* pDriver)
{
  KERNEL_LOCK(pDriver->statsLock);
  memset(&pDriver->stats, 0, sizeof(S_NetworkStatistics));
  KERNEL_UNLOCK(pDriver->statsLock);

  return 0;
}

static ssize_t _IOControlSetMACAddress(S_E1000Driver* pDriver, void* pArg)
{
  ssize_t  retCode;
  uint64_t macAddress;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(uint64_t),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RW) == true)
  {
    /* Validate the MAC address */
    macAddress = (*(uint64_t*)pArg) & 0x0000FFFFFFFFFFFF;
    if ((macAddress & 0x1ULL) != 0)
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl set MAC address called"
                   " with invalid multicast MAC address\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      retCode = -EINVAL;
    }
    else if (macAddress == 0 || macAddress == 0x0000FFFFFFFFFFFF)
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl set MAC address called"
                   " with invalid MAC address\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      retCode = -EINVAL;
    }
    else
    {
      KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000 device %04X:%04X:%04X ioctl set MAC address to "
                  "%02X:%02X:%02X:%02X:%02X:%02X",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function,
                  (uint8_t)(macAddress & 0xFF),
                  (uint8_t)((macAddress >> 8) & 0xFF),
                  (uint8_t)((macAddress >> 16) & 0xFF),
                  (uint8_t)((macAddress >> 24) & 0xFF),
                  (uint8_t)((macAddress >> 32) & 0xFF),
                  (uint8_t)((macAddress >> 40) & 0xFF));
      pDriver->macAddress = macAddress;
      _SetDeviceMAC(pDriver, macAddress);
      retCode = 0;
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device NULL ioctl set MAC address called with NULL "
                 "driver\n");
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlGetMACAddress(S_E1000Driver* pDriver, void* pArg)
{
  ssize_t   retCode;
  uint64_t* pMACAddress;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(uint64_t),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RW) == true)
  {
    pMACAddress  = (uint64_t*)pArg;
    *pMACAddress = pDriver->macAddress;
    retCode      = sizeof(pDriver->macAddress);
  }
  else
  {
    KERNEL_ERROR("E1000 MAC address called with NULL args\n");
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlSetLoopbackMode(S_E1000Driver* pDriver, void* pArg)
{
  ssize_t retCode;
  bool    isLoopbackEnabled;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(bool),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RO) == true)
  {
    isLoopbackEnabled = *(bool*)pArg;
    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000 device %04X:%04X:%04X ioctl set loopback mode to %s",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function,
                isLoopbackEnabled ? "enabled" : "disabled");

    KERNEL_LOCK(pDriver->ioLock);
    KERNEL_LOCK(pDriver->linkLock);
    if (isLoopbackEnabled == true)
    {
      pDriver->loopbackMode = LOOPBACK_PHY;
    }
    else
    {
      pDriver->loopbackMode = LOOPBACK_NONE;
    }
    if (pDriver->isInitialized == true)
    {
      KERNEL_UNLOCK(pDriver->linkLock);

      /* Reinitialize the network device */
      _DevDownlink(&pDriver->networkDevice);
      _DevUplink(&pDriver->networkDevice);
    }
    else
    {
      KERNEL_UNLOCK(pDriver->linkLock);
    }
    KERNEL_UNLOCK(pDriver->ioLock);

    retCode = 0;
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl set loopback mode called"
                 " with invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlGetLinkStatus(S_E1000Driver* pDriver, void* pArg)
{
  ssize_t retCode;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(bool),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RW) == true)
  {
    KERNEL_LOCK(pDriver->linkLock);
    if (pDriver->isInitialized == true)
    {
      *(bool*)pArg = pDriver->isLinkUp;
    }
    else
    {
      *(bool*)pArg = false;
    }
    KERNEL_UNLOCK(pDriver->linkLock);

    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000 device %04X:%04X:%04X ioctl get link status: %s",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function,
                pDriver->isLinkUp ? "up" : "down");
    retCode = 0;
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl get link status called"
                 " with invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlSetAutoNeg(S_E1000Driver* pDriver)
{
  ssize_t retCode;

  KERNEL_LOCK(pDriver->linkLock);
  if (pDriver->isInitialized == true)
  {
    KERNEL_UNLOCK(pDriver->linkLock);

    /* Set the auto-negotiation parameters */
    pDriver->autoNegMask = E1000_AUTONEG_MASK;
    pDriver->linkSpeed   = LINK_SPEED_AUTONEG;
    pDriver->duplexType  = DUPLEX_AUTONEG;

    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000 device %04X:%04X:%04X ioctl set auto-negotiation",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function);

    /* Reinitialize the network device */
    _DevDownlink(&pDriver->networkDevice);
    _DevUplink(&pDriver->networkDevice);
    retCode = 0;
  }
  else
  {
    KERNEL_UNLOCK(pDriver->linkLock);
    KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl set auto-negotiation called"
                 " on uninitialized device\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -ENODEV;
  }
  KERNEL_UNLOCK(pDriver->ioLock);

  return retCode;
}

static ssize_t _IOControlForceSpeedDuplex(S_E1000Driver* pDriver, void* pArg)
{
  ssize_t              retCode;
  S_NetworkLinkConfig* pConfig;
  bool                 valid;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(S_NetworkLinkConfig),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RW) == true)
  {
    /* Gather parameters */
    pConfig = (S_NetworkLinkConfig*)pArg;
    valid = false;

    if (pConfig->linkSpeed == LINK_SPEED_10MBPS ||
        pConfig->linkSpeed == LINK_SPEED_100MBPS ||
        pConfig->linkSpeed == LINK_SPEED_1GBPS)
    {
      if (pConfig->duplexType == DUPLEX_HALF ||
          pConfig->duplexType == DUPLEX_FULL)
      {
        valid = true;
      }
    }

    if (valid == true)
    {
      KERNEL_UNLOCK(pDriver->ioLock);
      KERNEL_LOCK(pDriver->linkLock);
      if (pDriver->isInitialized == true)
      {
        KERNEL_UNLOCK(pDriver->linkLock);

        pDriver->autoNegMask = 0;
        pDriver->linkSpeed   = pConfig->linkSpeed;
        pDriver->duplexType  = pConfig->duplexType;

        KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000 device %04X:%04X:%04X ioctl force speed/duplex to "
                    "%s/%s",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function,
                  (pConfig->linkSpeed == LINK_SPEED_10MBPS) ? "10Mbps" :
                  (pConfig->linkSpeed == LINK_SPEED_100MBPS) ? "100Mbps" :
                  (pConfig->linkSpeed == LINK_SPEED_1GBPS) ? "1Gbps" : "N/A",
                  (pConfig->duplexType == DUPLEX_HALF) ? "Half" :
                  (pConfig->duplexType == DUPLEX_FULL) ? "Full" : "N/A");

        /* Reinitialize the network device */
        _DevDownlink(&pDriver->networkDevice);
        _DevUplink(&pDriver->networkDevice);
        retCode = 0;
      }
      else
      {
        KERNEL_UNLOCK(pDriver->linkLock);
        KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl force speed/duplex "
                     "called on uninitialized device\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
        retCode = -ENODEV;
      }
      KERNEL_UNLOCK(pDriver->ioLock);
    }
    else
    {
      KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl force speed/duplex "
                   "called with invalid parameters\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      retCode = -EINVAL;
    }
  }
  else
  {
    KERNEL_ERROR("E1000 device %04X:%04X:%04X ioctl force speed/duplex called"
                 " with invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}
/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_PCI(sX86E1000Driver);

/************************************ EOF *************************************/