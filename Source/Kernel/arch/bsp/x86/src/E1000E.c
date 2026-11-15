/*******************************************************************************
 * @file E1000E.c
 *
 * @see E1000E.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/10/2026
 *
 * @version 1.0
 *
 * @brief Kernel's E1000E (Ethernet Controller) driver.
 *
 * @details Kernel's E1000E (Ethernet Controller) driver. Defines
 * the functions and definitions to interact with the E1000E
 * controller and its device.
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
#include <KernelMutex.h>
#include <KernelError.h>
#include <KernelQueue.h>
#include <TimerManager.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <E1000E.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "X86 E1000E"

/** @brief Intel vendor ID. */
#define INTEL_VENDOR_ID 0x8086

/* ==========================================================================
 * Intel 82571 / 82572 Family PCIe Hardware
 * ========================================================================== */
/** @brief 82571EB Copper Network Controllers */
#define E1000E_DEV_ID_82571EB_COPPER         0x105E
/** @brief 82571EB Fiber Network Controllers */
#define E1000E_DEV_ID_82571EB_FIBER          0x105F
/** @brief 82571EB SerDes Network Controllers */
#define E1000E_DEV_ID_82571EB_SERDES         0x1060
/** @brief 82571EB Quad Copper Network Controllers */
#define E1000E_DEV_ID_82571EB_QUAD_COPPER    0x10A4
/** @brief 82571EB Quad Fiber Network Controllers */
#define E1000E_DEV_ID_82571EB_QUAD_FIBER     0x10A5
/** @brief 82571EB Quad Copper Low-Power Network Controllers */
#define E1000E_DEV_ID_82571EB_QUAD_COPPER_LP 0x10BC
/** @brief 82571PT Quad Copper Network Controllers */
#define E1000E_DEV_ID_82571PT_QUAD_COPPER    0x10D5
/** @brief 82571EB Dual SerDes Network Controllers */
#define E1000E_DEV_ID_82571EB_SERDES_DUAL    0x10D9
/** @brief 82571EB Quad SerDes Network Controllers */
#define E1000E_DEV_ID_82571EB_SERDES_QUAD    0x10DA
/** @brief 82572EI Copper Network Controllers */
#define E1000E_DEV_ID_82572EI_COPPER         0x107D
/** @brief 82572EI Fiber Network Controllers */
#define E1000E_DEV_ID_82572EI_FIBER          0x107E
/** @brief 82572EI SerDes Network Controllers */
#define E1000E_DEV_ID_82572EI_SERDES         0x107F
/** @brief 82572EI Generic Network Controllers */
#define E1000E_DEV_ID_82572EI                0x10B9

/* ==========================================================================
 * Intel 82573 / 82574 / 82583 Discrete PCIe Hardware
 * ========================================================================== */
/** @brief 82573E Network Controllers */
#define E1000E_DEV_ID_82573E                 0x108B
/** @brief 82573E Intel AMT Network Controllers */
#define E1000E_DEV_ID_82573E_IAMT            0x108C
/** @brief 82573L Network Controllers */
#define E1000E_DEV_ID_82573L                 0x109A
/** @brief 82574L Network Controllers */
#define E1000E_DEV_ID_82574L                 0x10D3
/** @brief 82574LA Network Controllers */
#define E1000E_DEV_ID_82574LA                0x10F6
/** @brief 82583V Network Controllers */
#define E1000E_DEV_ID_82583V                 0x150C

/* ==========================================================================
 * Intel 80003ES2LAN Corporate Server Family
 * ========================================================================== */
/** @brief 80003ES2LAN Dual-Port Copper Network Controllers */
#define E1000E_DEV_ID_80003ES2LAN_COPPER_DPT 0x1096
/** @brief 80003ES2LAN Dual-Port SerDes Network Controllers */
#define E1000E_DEV_ID_80003ES2LAN_SERDES_DPT 0x1098
/** @brief 80003ES2LAN Copper Network Controllers */
#define E1000E_DEV_ID_80003ES2LAN_COPPER     0x10BA
/** @brief 80003ES2LAN SerDes Network Controllers */
#define E1000E_DEV_ID_80003ES2LAN_SERDES     0x10BB

/* ==========================================================================
 * Intel ICH8 / ICH9 / ICH10 Integrated Platform LOM (LAN on Motherboard)
 * ========================================================================== */
/** @brief ICH8 IGP Network Controllers (Intel AMT) */
#define E1000E_DEV_ID_ICH8_IGP_AMT           0x1049
/** @brief ICH8 IGP Mobile Network Controllers (Intel AMT) */
#define E1000E_DEV_ID_ICH8_IGP_M_AMT         0x104A
/** @brief ICH8 IGP Corporate/Desktop Network Controllers */
#define E1000E_DEV_ID_ICH8_IGP_C             0x104B
/** @brief ICH8 IGP Mobile Network Controllers (No AMT) */
#define E1000E_DEV_ID_ICH8_IGP_M             0x104C
/** @brief ICH8 IGP Mobile Network Controllers (Consumer) */
#define E1000E_DEV_ID_ICH8_IGP_M_V           0x104D
/** @brief ICH8 82562V 10/100 Network Connection */
#define E1000E_DEV_ID_ICH8_IFE               0x10C4
/** @brief ICH8 82562V 10/100 Network Connection (G-Link) */
#define E1000E_DEV_ID_ICH8_IFE_GT            0x10C5
/** @brief ICH8 82567V-3 Network Connection */
#define E1000E_DEV_ID_ICH8_82567V_3          0x1501

/** @brief ICH9 IGP Corporate Network Controllers (Intel AMT) */
#define E1000E_DEV_ID_ICH9_IGP_AMT           0x10BD
/** @brief ICH9 IGP Mobile Network Controllers */
#define E1000E_DEV_ID_ICH9_IGP_M             0x10BF
/** @brief ICH9 IGP Corporate Desktop Network Controllers */
#define E1000E_DEV_ID_ICH9_IGP_C             0x10C0
/** @brief ICH9 82562V 10/100 Connection (G-Link) */
#define E1000E_DEV_ID_ICH9_IFE_G             0x10C2
/** @brief ICH9 82562V 10/100 Corporate Connection (GT-Link) */
#define E1000E_DEV_ID_ICH9_IFE_GT            0x10C3
/** @brief ICH9 IGP Mobile Network Controllers (Low Power) */
#define E1000E_DEV_ID_ICH9_IGP_M_V           0x10CB
/** @brief ICH9 IGP Consumer Desktop Network Controllers */
#define E1000E_DEV_ID_ICH9_BM                0x10E5
/** @brief ICH9 IGP Mobile Network Controllers (Intel AMT) */
#define E1000E_DEV_ID_ICH9_IGP_M_AMT         0x10F5
/** @brief ICH9 IGP Corporate Desktop Network Controllers (Alternate) */
#define E1000E_DEV_ID_ICH9_IGP_C_ALT         0x294C

/** @brief ICH10 Corporate Desktop Network Controllers (Intel AMT) */
#define E1000E_DEV_ID_ICH10_R_BM_LM          0x10CC
/** @brief ICH10 Consumer Desktop Network Controllers */
#define E1000E_DEV_ID_ICH10_R_BM_LF          0x10CD
/** @brief ICH10 Retail Desktop Network Controllers */
#define E1000E_DEV_ID_ICH10_R_BM_V           0x10CE
/** @brief ICH10 Corporate LOM Network Controllers */
#define E1000E_DEV_ID_ICH10_D_BM_LM          0x10DE
/** @brief ICH10 Consumer LOM Network Controllers */
#define E1000E_DEV_ID_ICH10_D_BM_LF          0x10DF
/** @brief ICH10 Retail LOM Network Controllers */
#define E1000E_DEV_ID_ICH10_D_BM_V           0x1525

/* ==========================================================================
 * Intel PCH / PCH2 (82577 / 82578 / 82579) Integrated Generation
 * ========================================================================== */
/** @brief PCH 82577LM Corporate Mobile Connections */
#define E1000E_DEV_ID_PCH_M_HW_LM            0x10EA
/** @brief PCH 82577V Consumer Mobile Connections */
#define E1000E_DEV_ID_PCH_M_HW_V             0x10EB
/** @brief PCH 82578DM Corporate Desktop Connections */
#define E1000E_DEV_ID_PCH_D_HW_LM            0x10EF
/** @brief PCH 82578V Consumer Desktop Connections */
#define E1000E_DEV_ID_PCH_D_HW_V             0x10F0
/** @brief PCH2 82579LM Corporate Network Connections */
#define E1000E_DEV_ID_PCH2_LV_LM             0x1502
/** @brief PCH2 82579V Consumer Network Connections */
#define E1000E_DEV_ID_PCH2_LV_V              0x1503

/* ==========================================================================
 * Intel I217 / I218 / I219 PCH Integrated Generations (8-Series to 800-Series)
 * ========================================================================== */
/** @brief Lynx Point I217-LM Corporate Network Connection */
#define E1000E_DEV_ID_PCH_LPT_I217_LM        0x153A
/** @brief Lynx Point I217-V Consumer Network Connection */
#define E1000E_DEV_ID_PCH_LPT_I217_V         0x153B

/** @brief Lynx Point-LP I218-V Consumer Network Connection */
#define E1000E_DEV_ID_PCH_LPTLP_I218_V       0x1559
/** @brief Lynx Point-LP I218-LM Corporate Network Connection */
#define E1000E_DEV_ID_PCH_LPTLP_I218_LM      0x155A
/** @brief Wildcat Point I218-LM2 Corporate Network Connection */
#define E1000E_DEV_ID_PCH_I218_LM2           0x15A0
/** @brief Wildcat Point I218-V2 Consumer Network Connection */
#define E1000E_DEV_ID_PCH_I218_V2            0x15A1
/** @brief Wildcat Point I218-LM3 Corporate Network Connection */
#define E1000E_DEV_ID_PCH_I218_LM3           0x15A2
/** @brief Wildcat Point I218-V3 Consumer Network Connection */
#define E1000E_DEV_ID_PCH_I218_V3            0x15A3

/** @brief Sunrise Point I219-LM (100-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_LM        0x156F
/** @brief Sunrise Point I219-V (100-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_V         0x1570
/** @brief Sunrise Point I219-LM2 (100/200-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_LM2       0x15B7
/** @brief Sunrise Point I219-V2 (100/200-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_V2        0x15B8
/** @brief Lewisburg I219-LM3 (C620 Server Chipset) */
#define E1000E_DEV_ID_PCH_LBG_I219_LM3       0x15B9
/** @brief Kaby Lake / Sunrise Point I219-V5 (200-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_V5        0x15D6
/** @brief Kaby Lake / Sunrise Point I219-LM4 (200-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_LM4       0x15D7
/** @brief Kaby Lake / Sunrise Point I219-V4 (200-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_V4        0x15D8
/** @brief Kaby Lake / Sunrise Point I219-LM5 (200-Series) */
#define E1000E_DEV_ID_PCH_SPT_I219_LM5       0x15E3

/** @brief Cannon Lake / Coffee Lake I219-LM7 (300-Series) */
#define E1000E_DEV_ID_PCH_CNP_I219_LM7       0x15BB
/** @brief Cannon Lake / Coffee Lake I219-V7 (300-Series) */
#define E1000E_DEV_ID_PCH_CNP_I219_V7        0x15BC
/** @brief Cannon Lake / Coffee Lake I219-LM6 (300-Series) */
#define E1000E_DEV_ID_PCH_CNP_I219_LM6       0x15BD
/** @brief Cannon Lake / Coffee Lake I219-V6 (300-Series) */
#define E1000E_DEV_ID_PCH_CNP_I219_V6        0x15BE

/** @brief Ice Lake I219-LM8 (400-Series Mobile) */
#define E1000E_DEV_ID_PCH_ICP_I219_LM8       0x15DF
/** @brief Ice Lake I219-V8 (400-Series Mobile) */
#define E1000E_DEV_ID_PCH_ICP_I219_V8        0x15E0
/** @brief Ice Lake I219-LM9 (400-Series Mobile) */
#define E1000E_DEV_ID_PCH_ICP_I219_LM9       0x15E1
/** @brief Ice Lake I219-V9 (400-Series Mobile) */
#define E1000E_DEV_ID_PCH_ICP_I219_V9        0x15E2

/** @brief Comet Lake I219-LM11 (400-Series Desktop) */
#define E1000E_DEV_ID_PCH_CMP_I219_LM11      0x0D4C
/** @brief Comet Lake I219-V11 (400-Series Desktop) */
#define E1000E_DEV_ID_PCH_CMP_I219_V11       0x0D4D
/** @brief Comet Lake I219-LM10 (400-Series Desktop) */
#define E1000E_DEV_ID_PCH_CMP_I219_LM10      0x0D4E
/** @brief Comet Lake I219-V10 (400-Series Desktop) */
#define E1000E_DEV_ID_PCH_CMP_I219_V10       0x0D4F

/** @brief Comet Lake I219-LM12 (400-Series Workstation) */
#define E1000E_DEV_ID_PCH_CMP_I219_LM12      0x0D53
/** @brief Comet Lake I219-V12 (400-Series Workstation) */
#define E1000E_DEV_ID_PCH_CMP_I219_V12       0x0D55

/** @brief Tiger Lake I219-LM15 (500-Series) */
#define E1000E_DEV_ID_PCH_TGP_I219_LM15      0x15F4
/** @brief Tiger Lake I219-V15 (500-Series) */
#define E1000E_DEV_ID_PCH_TGP_I219_V15       0x15F5
/** @brief Tiger Lake I219-LM14 (500-Series) */
#define E1000E_DEV_ID_PCH_TGP_I219_LM14      0x15F9
/** @brief Tiger Lake I219-V14 (500-Series) */
#define E1000E_DEV_ID_PCH_TGP_I219_V14       0x15FA
/** @brief Tiger Lake I219-LM13 (500-Series) */
#define E1000E_DEV_ID_PCH_TGP_I219_LM13      0x15FB
/** @brief Tiger Lake I219-V13 (500-Series) */
#define E1000E_DEV_ID_PCH_TGP_I219_V13       0x15FC

/** @brief Alder Lake I219-LM17 (600-Series) */
#define E1000E_DEV_ID_PCH_ADP_I219_LM17      0x1A1C
/** @brief Alder Lake I219-V17 (600-Series) */
#define E1000E_DEV_ID_PCH_ADP_I219_V17       0x1A1D
/** @brief Alder Lake I219-LM16 (600-Series) */
#define E1000E_DEV_ID_PCH_ADP_I219_LM16      0x1A1E
/** @brief Alder Lake I219-V16 (600-Series) */
#define E1000E_DEV_ID_PCH_ADP_I219_V16       0x1A1F

/** @brief Raptor Lake I219-LM19 (700-Series) */
#define E1000E_DEV_ID_PCH_RPL_I219_LM19      0x0DC5
/** @brief Raptor Lake I219-V19 (700-Series) */
#define E1000E_DEV_ID_PCH_RPL_I219_V19       0x0DC6
/** @brief Raptor Lake I219-LM18 (700-Series) */
#define E1000E_DEV_ID_PCH_RPL_I219_LM18      0x0DC7
/** @brief Raptor Lake I219-V18 (700-Series) */
#define E1000E_DEV_ID_PCH_RPL_I219_V18       0x0DC8

/** @brief Meteor Lake I219-LM20 (800-Series) */
#define E1000E_DEV_ID_PCH_MTP_I219_LM20      0x550A
/** @brief Meteor Lake I219-V20 (800-Series) */
#define E1000E_DEV_ID_PCH_MTP_I219_V20       0x550B
/** @brief Meteor Lake I219-LM21 (800-Series) */
#define E1000E_DEV_ID_PCH_MTP_I219_LM21      0x550C
/** @brief Meteor Lake I219-V21 (800-Series) */
#define E1000E_DEV_ID_PCH_MTP_I219_V21       0x550D
/** @brief Arrow Lake I219-LM22 (800-Series) */
#define E1000E_DEV_ID_PCH_ARL_I219_LM22      0x57A0
/** @brief Arrow Lake I219-V22 (800-Series) */
#define E1000E_DEV_ID_PCH_ARL_I219_V22       0x57A1

/* --- Registers: General --- */
/** @brief Device Control Register */
#define E1000E_CTRL 0x00000
/** @brief Device Status Register */
#define E1000E_STATUS 0x00008
/** @brief EEPROM/FLASH Control Register */
#define E1000E_EEC 0x00010
/** @brief EEPROM Read Register */
#define E1000E_EERD 0x00014
/** @brief Extended Device Control Register */
#define E1000E_CTRL_EXT 0x00018
/** @brief Flash Access Register */
#define E1000E_FLA 0x0001C
/** @brief MDI Control Register */
#define E1000E_MDIC 0x00020
/** @brief Flow Control Address Low */
#define E1000E_FCAL 0x00028
/** @brief Flow Control Address High */
#define E1000E_FCAH 0x0002C
/** @brief Flow Control Type */
#define E1000E_FCT 0x00030
/** @brief VLAN Ether Type */
#define E1000E_VET 0x00038
/** @brief Flow Control Transmit Timer Value */
#define E1000E_FCTTV 0x00170
/** @brief Flow Control Refresh Threshold Value */
#define E1000E_FCRTV 0x05F40
/** @brief LED Control */
#define E1000E_LEDCTL 0x00E00
/** @brief Extended Configuration Control */
#define E1000E_EXTCNF_CTRL 0x00F00
/** @brief Extended Configuration Size */
#define E1000E_EXTCNF_SIZE 0x00F08
/** @brief Packet Buffer Allocation */
#define E1000E_PBA 0x01000
/** @brief MNG EEPROM Control Register */
#define E1000E_EEMNGCTL 0x1010
/** @brief MNG EEPROM Read/Write data */
#define E1000E_EEMNGDATA 0x1014
/** @brief MNG Flash Control Register */
#define E1000E_FLMNGCTL 0x1018
/** @brief MNG FLASH Read data */
#define E1000E_FLMNGDATA 0x101C
/** @brief MNG FLASH Read Counter */
#define E1000E_FLMNGCNT 0x1020
/** @brief FLASH Timer Register */
#define E1000E_FLASHT 0x01028
/** @brief EEPROM Write Register */
#define E1000E_EEWR 0x0102C
/** @brief SW FLASH Burst Control Register */
#define E1000E_FLSWCTL 0x1030
/** @brief SW FLASH Burst Data Register */
#define E1000E_FLSWDATA 0x1034
/** @brief SW FLASH Burst Access Counter */
#define E1000E_FLSWCNT 0x1038
/** @brief FLASH Opcode Register */
#define E1000E_FLOP 0x0103C
/** @brief FLEEP Auto Load */
#define E1000E_FLOL 0x1050

/* --- Registers: PCIe --- */
/** @brief 3GIO Control Register */
#define E1000E_GCR 0x05B00
/** @brief Function-Tag register */
#define E1000E_FUNCTAG 0x05B08
/** @brief 3GIO Statistic Control Register #1 */
#define E1000E_GSCL_1 0x05B10
/** @brief 3GIO Statistic Control Registers #2 */
#define E1000E_GSCL_2 0x05B14
/** @brief 3GIO Statistic Control Register #3 */
#define E1000E_GSCL_3 0x05B18
/** @brief 3GIO Statistic Control Register #4 */
#define E1000E_GSCL_4 0x05B1C
/** @brief 3GIO Statistic Counter Registers #0 */
#define E1000E_GSCN_0 0x05B20
/** @brief 3GIO Statistic Counter Registers #1 */
#define E1000E_GSCN_1 0x05B24
/** @brief 3GIO Statistic Counter Registers #2 */
#define E1000E_GSCN_2 0x05B28
/** @brief 3GIO Statistic Counter Registers #3 */
#define E1000E_GSCN_3 0x05B2C
/** @brief Software Semaphore Register */
#define E1000E_SWSM 0x05B50
/** @brief 3GIO Control Register 2 */
#define E1000E_GCR2 0x05B64
/** @brief MSI—X PBA Clear  */
#define E1000E_PBACLR 0x5B68

/* --- Registers: Interrupt --- */
/** @brief Interrupt Cause Read Register */
#define E1000E_ICR 0x000C0
/** @brief Interrupt Throttling Register */
#define E1000E_ITR 0x000C4
/** @brief Extended Interrupt Throttle */
#define E1000E_EITR 0x000E8
/** @brief Interrupt Cause Set Register */
#define E1000E_ICS 0x000C8
/** @brief Interrupt Mask Set/Read Register */
#define E1000E_IMS 0x000D0
/** @brief Interrupt Mask Clear Register */
#define E1000E_IMC 0x000D8
/** @brief Interrupt Auto Clear */
#define E1000E_EIAC 0x000DC
/** @brief Interrupt Acknowledge Auto-Mask */
#define E1000E_IAM 0x000E0
/** @brief Interrupt Vector Allocation Registers */
#define E1000E_IVAR 0x000E4

/* --- Registers: Receive --- */
/** @brief Receive Control Register */
#define E1000E_RCTL 0x00100
/** @brief Packet Split Receive Control Register */
#define E1000E_PSRCTL 0x02170
/** @brief Flow Control Receive Threshold Low */
#define E1000E_FCRTL 0x02160
/** @brief Flow Control Receive Threshold High */
#define E1000E_FCRTH 0x02168
/** @brief Receive Descriptor Base Address Low queue 0 */
#define E1000E_RDBAL0 0x02800
/** @brief Receive Descriptor Base Address High queue 0 */
#define E1000E_RDBAH0 0x02804
/** @brief Receive Descriptor Length queue 0 */
#define E1000E_RDLEN0 0x02808
/** @brief Receive Descriptor Head queue 0 */
#define E1000E_RDH0 0x02810
/** @brief Receive Descriptor Tail queue 0 */
#define E1000E_RDT0 0x02818
/** @brief Rx Interrupt Delay Timer [Packet Timer] */
#define E1000E_RDTR 0x02820
/** @brief Receive Descriptor Control */
#define E1000E_RXDCTL 0x02828
/** @brief Receive Interrupt Absolute Delay Timer */
#define E1000E_RADV 0x0282C
/** @brief Receive Small Packet Detect Interrupt */
#define E1000E_RSRPD 0x02C00
/** @brief Receive ACK Interrupt Delay Register */
#define E1000E_RAID 0x02C08
/** @brief Receive Checksum Control */
#define E1000E_RXCSUM 0x05000
/** @brief Receive Filter Control Register */
#define E1000E_RFCTL 0x05008
/** @brief Management VLAN TAG Value 0 */
#define E1000E_MAVTV0 0x5010
/** @brief Management VLAN TAG Value 1 */
#define E1000E_MAVTV1 0x5014
/** @brief Management VLAN TAG Value 2 */
#define E1000E_MAVTV2 0x5018
/** @brief Management VLAN TAG Value 3 */
#define E1000E_MAVTV3 0x501C
/** @brief Multicast Table Array */
#define E1000E_MTA 0x05200
/** @brief Receive Address Low (0) */
#define E1000E_RAL_0 0x05400
/** @brief Receive Address High (0) */
#define E1000E_RAH_0 0x05404
/** @brief Receive Address Low (1) */
#define E1000E_RAL_1 0x05408
/** @brief Receive Address High (1) */
#define E1000E_RAH_1 0x0540C
/** @brief VLAN Filter Table Array */
#define E1000E_VFTA 0x05600
/** @brief Receive Address Low (15) */
#define E1000E_RAL_15 0x05478
/** @brief Receive Address High (15) */
#define E1000E_RAH_15 0x0547C
/** @brief Multiple Receive Queues Command register */
#define E1000E_MRQC 0x05818
/** @brief Redirection Table */
#define E1000E_RETA 0x05C00
/** @brief RSS Random Key Register */
#define E1000E_RSSRK 0x05C80

/* --- Registers: Transmit --- */
/** @brief Transmit Control Register */
#define E1000E_TCTL 0x00400
/** @brief Transmit IPG Register */
#define E1000E_TIPG 0x00410
/** @brief Adaptive IFS Throttle */
#define E1000E_AIT 0x00458
/** @brief Transmit Descriptor Base Address Low */
#define E1000E_TDBAL 0x03800
/** @brief Transmit Descriptor Base Address High */
#define E1000E_TDBAH 0x03804
/** @brief Transmit Descriptor Length */
#define E1000E_TDLEN 0x03808
/** @brief Transmit Descriptor Head */
#define E1000E_TDH 0x03810
/** @brief Transmit Descriptor Tail */
#define E1000E_TDT 0x03818
/** @brief Transmit Arbitration Count */
#define E1000E_TARC 0x03840
/** @brief Transmit Interrupt Delay Value */
#define E1000E_TIDV 0x03820
/** @brief Transmit Descriptor Control */
#define E1000E_TXDCTL 0x03828
/** @brief Transmit Absolute Interrupt Delay Value */
#define E1000E_TADV 0x0382C

/* --- Registers: Statistic --- */
/** @brief CRC Error Count */
#define E1000E_CRCERRS 0x04000
/** @brief Alignment Error Count */
#define E1000E_ALGNERRC 0x04004
/** @brief RX Error Count */
#define E1000E_RXERRC 0x0400C
/** @brief Missed Packets Count */
#define E1000E_MPC 0x04010
/** @brief Single Collision Count */
#define E1000E_SCC 0x04014
/** @brief Excessive Collisions Count */
#define E1000E_ECOL 0x04018
/** @brief Multiple Collision Count */
#define E1000E_MCC 0x0401C
/** @brief Late Collisions Count */
#define E1000E_LATECOL 0x04020
/** @brief Collision Count */
#define E1000E_COLC 0x04028
/** @brief Defer Count */
#define E1000E_DC 0x04030
/** @brief Transmit with No CRS */
#define E1000E_TNCRS 0x04034
/** @brief Carrier Extension Error Count */
#define E1000E_CEXTERR 0x0403C
/** @brief Receive Length Error Count */
#define E1000E_RLEC 0x04040
/** @brief XON Received Count */
#define E1000E_XONRXC 0x04048
/** @brief XON Transmitted Count */
#define E1000E_XONTXC 0x0404C
/** @brief XOFF Received Count */
#define E1000E_XOFFRXC 0x04050
/** @brief XOFF Transmitted Count */
#define E1000E_XOFFTXC 0x04054
/** @brief FC Received Unsupported Count */
#define E1000E_FCRUC 0x04058
/** @brief Packets Received [64 Bytes] Count */
#define E1000E_PRC64 0x0405C
/** @brief Packets Received [65-127 Bytes] Count */
#define E1000E_PRC127 0x04060
/** @brief Packets Received [128-255 Bytes] Count */
#define E1000E_PRC255 0x04064
/** @brief Packets Received [256-511 Bytes] Count */
#define E1000E_PRC511 0x04068
/** @brief Packets Received [512-1023 Bytes] Count */
#define E1000E_PRC1023 0x0406C
/** @brief Packets Received [1024 to Max Bytes] Count */
#define E1000E_PRC1522 0x04070
/** @brief Good Packets Received Count */
#define E1000E_GPRC 0x04074
/** @brief Broadcast Packets Received Count */
#define E1000E_BPRC 0x04078
/** @brief Multicast Packets Received Count */
#define E1000E_MPRC 0x0407C
/** @brief Good Packets Transmitted Count */
#define E1000E_GPTC 0x04080
/** @brief Good Octets Received Count Low */
#define E1000E_GORCL 0x04088
/** @brief Good Octets Received Count High */
#define E1000E_GORCH 0x0408C
/** @brief Good Octets Transmitted Count Low */
#define E1000E_GOTCL 0x04090
/** @brief Good Octets Transmitted Count High */
#define E1000E_GOTCH 0x04094
/** @brief Receive No Buffers Count */
#define E1000E_RNBC 0x040A0
/** @brief Receive Undersize Count */
#define E1000E_RUC 0x040A4
/** @brief Receive Fragment Count */
#define E1000E_RFC 0x040A8
/** @brief Receive Oversize Count */
#define E1000E_ROC 0x040AC
/** @brief Receive Jabber Count */
#define E1000E_RJC 0x040B0
/** @brief Management Packets Received Count */
#define E1000E_MNGPRC 0x040B4
/** @brief Management Packets Dropped Count */
#define E1000E_MPDC 0x040B8
/** @brief Management Packets Transmitted Count */
#define E1000E_MPTC 0x040BC
/** @brief Total Octets Received */
#define E1000E_TORL 0x040C0
/** @brief Total Octets Received */
#define E1000E_TORH 0x040C4
/** @brief Total Octets Transmitted */
#define E1000E_TOT 0x040C8
/** @brief Total Packets Received */
#define E1000E_TPR 0x040D0
/** @brief Total Packets Transmitted */
#define E1000E_TPT 0x040D4
/** @brief Packets Transmitted [64 Bytes] Count */
#define E1000E_PTC64 0x040D8
/** @brief Packets Transmitted [65-127 Bytes] Count */
#define E1000E_PTC127 0x040DC
/** @brief Packets Transmitted [128-255 Bytes] Count */
#define E1000E_PTC255 0x040E0
/** @brief Packets Transmitted [256-511 Bytes] Count */
#define E1000E_PTC511 0x040E4
/** @brief Packets Transmitted [512-1023 Bytes] Count */
#define E1000E_PTC1023 0x040E8
/** @brief Packets Transmitted [Greater than 1024 Bytes] Count */
#define E1000E_PTC1522 0x040EC
/** @brief Multicast Packets Transmitted Count */
#define E1000E_MPTC 0x040F0
/** @brief Broadcast Packets Transmitted Count */
#define E1000E_BPTC 0x040F4
/** @brief TCP Segmentation Context Transmitted Count */
#define E1000E_TSCTC 0x040F8
/** @brief TCP Segmentation Context Transmit Fail Count */
#define E1000E_TSCTFC 0x040FC
/** @brief Interrupt Assertion Count */
#define E1000E_IAC 0x04100

/* --- Registers: Management --- */
/** @brief Wake Up Control Register */
#define E1000E_WUC 0x05800
/** @brief Wake Up Filter Control Register */
#define E1000E_WUFC 0x05808
/** @brief Wake Up Status Register */
#define E1000E_WUS 0x05810
/** @brief Management Flex UDP/TCP Ports 0/1 */
#define E1000E_MFUTP01 0x05828
/** @brief Management Flex UDP/TCP Port 2/3 */
#define E1000E_MFUTP23 0x05830
/** @brief IP Address Valid */
#define E1000E_IPAV 0x5838
/** @brief IPv4 Address Table */
#define E1000E_IP4AT 0x05840
/** @brief Management Control Register */
#define E1000E_MANC 0x05820
/** @brief Management Control to Host Register */
#define E1000E_MANC2H 0x5860
/** @brief Manageability Filters Valid */
#define E1000E_MFVAL 0x5824
/** @brief Manageability Decision Filters */
#define E1000E_MDEF 0x5890
/** @brief IPv6 Address Table */
#define E1000E_IP6AT 0x05880
/** @brief Wake Up Packet Memory [128 Bytes] */
#define E1000E_WUPM 0x05A00
/** @brief Function Active and Power State to MNG */
#define E1000E_FACTPS 0x05B30
/** @brief Flexible Filter Length Table */
#define E1000E_FFLT 0x05F00
/** @brief Flexible Filter Mask Table */
#define E1000E_FFMT 0x09000
/** @brief Flexible TCO Filter Table */
#define E1000E_FTFT 0x09400
/** @brief Flexible Filter Value Table */
#define E1000E_FFVT 0x09800

/* --- Registers: Time --- */
/** @brief RX Time Sync Control Register */
#define E1000E_TSYNCRXCTL 0x0B620
/** @brief RX Timestamp High */
#define E1000E_RXSTMPH 0x0B628
/** @brief RX Timestamp Low */
#define E1000E_RXSTMPL 0x0B624
/** @brief RX Timestamp Attributes Low */
#define E1000E_RXSATRL 0x0B62C
/** @brief RX Timestamp Attributes High */
#define E1000E_RXSATRH 0x0B630
/** @brief RX Ethertype and Message Type Register */
#define E1000E_RXCFGL 0x0B634
/** @brief RX UDP Port */
#define E1000E_RXUDP 0x0B638
/** @brief TX Time Sync Control Register */
#define E1000E_TSYNCTXCTL 0x0B614
/** @brief TX Timestamp Value Low */
#define E1000E_TXSTMPL 0x0B618
/** @brief TX Timestamp Value High */
#define E1000E_TXSTMPH 0x0B61C
/** @brief System Time Register Low */
#define E1000E_SYSTIML 0x0B600
/** @brief System Time Register High */
#define E1000E_SYSTIMH 0x0B604
/** @brief Increment Attributes Register */
#define E1000E_TIMINCA 0x0B608
/** @brief Time Adjustment Offset Register Low */
#define E1000E_TIMADJL 0x0B60C
/** @brief Time Adjustment Offset Register High */
#define E1000E_TIMADJH 0x0B610

/* --- Registers: MSI-X --- */
/** @brief MSI-X Table Entry Lower Address */
#define E1000E_MSIXTADD 0x0000
/** @brief MSI-X Table Entry Upper Address */
#define E1000E_MSIXTUADD 0x0004
/** @brief MSI-X Table Entry Message */
#define E1000E_MSIXTMSG 0x0008
/** @brief MSI-X Table Entry Vector Control */
#define E1000E_MSIXTVCTRL 0x000C
/** @brief MSI-X PBA Bit Description */
#define E1000E_MSIXPBA 0x02000

/* --- Registers: Diagnostic --- */
/** @brief PHY OEM Bits Register */
#define E1000E_POEMB 0x00F10
/** @brief Receive Data FIFO Head Register */
#define E1000E_RDFH 0x02410
/** @brief Receive Data FIFO Tail Register */
#define E1000E_RDFT 0x02418
/** @brief Receive Data FIFO Head Saved Register */
#define E1000E_RDFHS 0x02420
/** @brief Receive Data FIFO Tail Saved Register */
#define E1000E_RDFTS 0x02428
/** @brief Receive Data FIFO Packet Count */
#define E1000E_RDFPC 0x02430
/** @brief Transmit Data FIFO Head Register */
#define E1000E_TDFH 0x03410
/** @brief Transmit Data FIFO Tail Register */
#define E1000E_TDFT 0x03418
/** @brief Transmit Data FIFO Head Saved Register */
#define E1000E_TDFHS 0x03420
/** @brief Transmit Data FIFO Tail Saved Register */
#define E1000E_TDFTS 0x03428
/** @brief Transmit Data FIFO Packet Count */
#define E1000E_TDFPC 0x03430
/** @brief Packet Buffer Memory */
#define E1000E_PBM 0x10000
/** @brief Packet Buffer Size */
#define E1000E_PBS 0x01008

/* --- Registers: Power Management --- */
/** @brief Energy Efficient Ethernet Register */
#define E1000E_EEER 0x0E30
/** @brief Internal PHY Configuration Register */
#define E1000E_IPCNFG 0x0E38

/* --- Software Semaphore Register Bits --- */
/** @brief Software EEPROM Semaphore Bit for the E1000E */
#define E1000E_SWSM_SWESMBI (1 << 1)

/* --- MDIO Phy Clause --- */
/** @brief MDIO Control Register */
#define E1000_PHY_MMD_CTRL 0x0D
/** @brief MDIO Data Register */
#define E1000_PHY_MMD_DATA 0x0E

/** @brief E1000E EEPROM Signature Retry Delay in nanoseconds */
#define E1000E_EEPROM_SIGNATURE_RETRY_DELAY_NS 20000000ULL
/** @brief E1000E EEPROM Signature Retry Count */
#define E1000E_EEPROM_SIGNATURE_RETRIES 5
/** @brief E1000E EEPROM Size in words */
#define E1000E_EEPROM_SIZE 64
/** @brief E1000E EEPROM Signature */
#define E1000E_EEPROM_SIGNATURE 0xBABA
/** @brief EEPROM read timeout in nanoseconds */
#define E1000E_EEPROM_READ_TIMEOUT_NS 5000000ULL
/** @brief E1000E Semaphore Lock Timeout in nanoseconds */
#define E1000E_SEMAPHORE_LOCK_TIMEOUT_NS 5000000ULL

/** @brief MDIO Write command */
#define E1000E_MDIO_WRITE (1 << 26)
/** @brief MDIO Read command */
#define E1000E_MDIO_READ (1 << 27)
/** @brief MDIO Done flag */
#define E1000E_MDIO_DONE (1 << 28)
/** @brief MDIO Error flag */
#define E1000E_MDIO_ERROR (1 << 30)
/** @brief MDIO Address shift */
#define E1000E_MDIO_ADDR_SHIFT 16
/** @brief MDIO PHY shift */
#define E1000E_MDIO_PHY_SHIFT 21
/** @brief MDIO Data mask */
#define E1000E_MDIO_DATA_MASK 0xFFFF
/** @brief MDIO Access timeout in nanoseconds */
#define E1000E_MDIO_ACCESS_TIMEOUT_NS 10000000ULL

/** @brief Reset timeout in nanoseconds */
#define E1000E_RESET_TIMEOUT_NS 20000000ULL

/* --- Receive Control Registers --- */
/** @brief Receive Control Register: Enable */
#define E1000E_RCTL_EN (1 << 1)
/** @brief Receive Control Register: Store Bad Packets */
#define E1000E_RCTL_SBP (1 << 2)
/** @brief Receive Control Register: Unicast Promiscuous Enable */
#define E1000E_RCTL_UPE (1 << 3)
/** @brief Receive Control Register: Multicast Promiscuous Enable */
#define E1000E_RCTL_MPE (1 << 4)
/** @brief Receive Control Register: Long Packet Enable */
#define E1000E_RCTL_LPE (1 << 5)
/** @brief Receive Control Register: Loopback Mode - None */
#define E1000E_RCTL_LBM_NONE (0 << 6)
/** @brief Receive Control Register: Loopback Mode - PHY */
#define E1000E_RCTL_LBM_PHY (0 << 6)
/** @brief Receive Control Register: Receive Descriptor Minimum Threshold 2/8 */
#define E1000E_RCTL_RDMTS_2 (0 << 8)
/** @brief Receive Control Register: Receive Descriptor Minimum Threshold 4/8 */
#define E1000E_RCTL_RDMTS_4 (1 << 8)
/** @brief Receive Control Register: Receive Descriptor Minimum Threshold 8/8 */
#define E1000E_RCTL_RDMTS_8 (2 << 8)
/** @brief Receive Control Register: Multicast Offset [47:36] */
#define E1000E_RCTL_MO_36 (0 << 12)
/** @brief Receive Control Register: Multicast Offset [46:35] */
#define E1000E_RCTL_MO_35 (1 << 12)
/** @brief Receive Control Register: Multicast Offset [45:34] */
#define E1000E_RCTL_MO_34 (2 << 12)
/** @brief Receive Control Register: Multicast Offset [43:32] */
#define E1000E_RCTL_MO_32 (3 << 12)
/** @brief Receive Control Register: Broadcast Accept Mode */
#define E1000E_RCTL_BAM (1 << 15)
/** @brief Receive Control Register: Receive Buffer Size 2048 */
#define E1000E_RCTL_BSIZE_2048 (0 << 16)
/** @brief Receive Control Register: Receive Buffer Size 1024 */
#define E1000E_RCTL_BSIZE_1024 (1 << 16)
/** @brief Receive Control Register: Receive Buffer Size 512 */
#define E1000E_RCTL_BSIZE_512 (2 << 16)
/** @brief Receive Control Register: Receive Buffer Size 256 */
#define E1000E_RCTL_BSIZE_256 (3 << 16)
/** @brief Receive Control Register: Receive Buffer Size 16384 */
#define E1000E_RCTL_BSIZE_16384 (1 << 16)
/** @brief Receive Control Register: Receive Buffer Size 8192 */
#define E1000E_RCTL_BSIZE_8192 (2 << 16)
/** @brief Receive Control Register: Receive Buffer Size 4096 */
#define E1000E_RCTL_BSIZE_4096 (3 << 16)
/** @brief Receive Control Register: VLAN Filter Enable */
#define E1000E_RCTL_VFE (1 << 18)
/** @brief Receive Control Register: Canonical Form Indicator Enable */
#define E1000E_RCTL_CFIEN (1 << 19)
/** @brief Receive Control Register: Canonical Form Indicator */
#define E1000E_RCTL_CFI (1 << 20)
/** @brief Receive Control Register: Discard Pause Frames */
#define E1000E_RCTL_DPF (1 << 22)
/** @brief Receive Control Register: Pass MAC Control Frames */
#define E1000E_RCTL_PMCF (1 << 23)
/** @brief Receive Control Register: Buffer Size Extension */
#define E1000E_RCTL_BSEX (1 << 25)
/** @brief Receive Control Register: Strip Ethernet CRC */
#define E1000E_RCTL_SECRC (1 << 26)

/* --- Initialization Values --- */
/** @brief Default auto-negotiation mask */
#define E1000E_AUTONEG_MASK 0x002F
/** @brief Default interrupt throttle value */
#define E1000E_INT_THROTTLE_INIT 4000
/** @brief Default RX interrupt delay value */
#define E1000E_RX_INT_DELAY_INIT 0
/** @brief Default TX interrupt delay value */
#define E1000E_TX_INT_DELAY_INIT 8
/** @brief Default absolute RX interrupt delay value */
#define E1000E_RX_ABS_INT_DELAY_INIT 128
/** @brief Default absolute TX interrupt delay value */
#define E1000E_TX_ABS_INT_DELAY_INIT 32
/** @brief Default RX checksum enable value */
#define E1000E_ENABLE_XSUM_RX_INIT true
/** @brief Default use of jumbo frames */
#define E1000E_USE_JUMBO_FRAMES_INIT false
/** @brief Default receive buffer count */
#define E1000E_RX_BUFFER_COUNT 256
/** @brief Default transmit buffer count */
#define E1000E_TX_BUFFER_COUNT 256
/** @brief Default unicast promiscuous mode */
#define E1000E_ENABLE_UNICAST_PROMISCUOUS_INIT true
/** @brief Default multicast promiscuous mode */
#define E1000E_ENABLE_MCAST_PROMISCUOUS_INIT true
/** @brief Default broadcast listen mode */
#define E1000E_LISTEN_BROADCAST_INIT true
/** @brief Default CRC stripping mode */
#define E1000E_CRC_STRIPPING_INIT false

/* --- Miscellaneous --- */
/** @brief Receive buffer size */
#define E1000E_RX_BUFFER_SIZE 2048
/** @brief Receive buffer alignment */
#define E1000E_RX_BUFFER_ALIGNMENT 128
/** @brief Transmit buffer size */
#define E1000E_TX_BUFFER_SIZE 2048
/** @brief Transmit buffer alignment */
#define E1000E_TX_BUFFER_ALIGNMENT 128

/** @brief Receive Address Hash Register Valid Bit */
#define E1000E_RAH_VALID (1 << 31)

/** @brief E1000E EXTCNF_CTRL Register Software Ownership Bit */
#define E1000E_EXTCNF_CTRL_SWOWN (1 << 5)

/* --- Transmit Control Registers --- */
/** @brief Transmit Control Register: Enable */
#define E1000E_TCTL_EN (1 << 1)
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
#define E1000_TCTL_UNORTX (1 << 25)
/** @brief Transmit Control Register: Collision Threshold for Ethernet */
#define E1000_TCTL_CT_ETHER 0x10
/** @brief Transmit Control Register: Collision Distance for Full Duplex */
#define E1000_TCTL_COLD_FULL 0x40
/** @brief Transmit Control Register: Collision Distance for Half Duplex */
#define E1000_TCTL_COLD_HALF 0x200

/* --- Device Control Register Bits --- */
/** @brief Full Duplex Enable */
#define E1000E_CTRL_FD (1 << 0)
/** @brief Auto-Speed Detection Enable */
#define E1000E_CTRL_ASDE (1 << 5)
/** @brief Speed Link Up */
#define E1000E_CTRL_SLU (1 << 6)
/** @brief Speed Selection mask */
#define E1000E_CTRL_SPD_SEL (3 << 8)
/** @brief Speed Selection 10 Mbps */
#define E1000E_CTRL_SPD_SEL_10 (0 << 8)
/** @brief Speed Selection 100 Mbps */
#define E1000E_CTRL_SPD_SEL_100 (1 << 8)
/** @brief Speed Selection 1000 Mbps */
#define E1000E_CTRL_SPD_SEL_1000 (2 << 8)
/** @brief Force Speed Selection */
#define E1000E_CTRL_FRCSPEED (1 << 11)
/** @brief Force Duplex Selection */
#define E1000E_CTRL_FRCDPLX (1 << 12)
/** @brief Reset control bit */
#define E1000E_CTRL_RST (1 << 26)
/** @brief E1000 CTRL Register RFCE (Receive Flow Control Enable) bit */
#define E1000E_CTRL_RFCE (1 << 27)
/** @brief E1000 CTRL Register TFCE (Transmit Flow Control Enable) bit */
#define E1000E_CTRL_TFCE (1 << 28)

/** @brief E1000E CTRL_EXT Register IAME (Interrupt Auto Mask Enable) bit */
#define E1000E_CTRL_EXT_IAME (1 << 27)
/** @brief E1000E CTRL_EXT Register Driver Load Bit */
#define E1000E_CTRL_EXT_DRV_LOAD (1 << 28)

/** @brief Interrupt Mask Set/Read Register: Transmit Descriptor Written Back */
#define E1000E_IMS_TXDW (1 << 0)
/** @brief Interrupt Mask Set/Read Register: Transmit Queue Empty */
#define E1000E_IMS_TXQE (1 << 1)
/** @brief Interrupt Mask Set/Read Register: Link Status Change */
#define E1000E_IMS_LSC (1 << 2)
/**
 * @brief Interrupt Mask Set/Read Register: Receive Descriptor Minimum Threshold
 * hit
 */
#define E1000E_IMS_RXDMT0 (1 << 4)
/** @brief Interrupt Mask Set/Read Register: Receive Overrun */
#define E1000E_IMS_RXO (1 << 6)
/** @brief Interrupt Mask Set/Read Register: Receive Timer Interrupt */
#define E1000E_IMS_RXT0 (1 << 7)
/** @brief Interrupt Mask Set/Read Register: MDI/O Access Complete  */
#define E1000E_IMS_MDAC (1 << 9)
/** @brief Interrupt Mask Set/Read Register: Receiving /C/ ordered sets */
#define E1000E_IMS_RXCFG (1 << 10)
/** @brief Interrupt Mask Set/Read Register: PHY Interrupt */
#define E1000E_IMS_PHYINT (1 << 12)
/** @brief Interrupt Mask Set/Read Register: General Purpose Interrupts */
#define E1000E_IMS_GPI (2 << 13)
/**
 * @brief Interrupt Mask Set/Read Register: Transmit Descriptor Low Threshold
 * hit
 */
#define E1000E_IMS_TXD_LOW (1 << 15)
/** @brief Interrupt Mask Set/Read Register: Small Receive Packet Detection */
#define E1000E_IMS_SRPD (1 << 16)

/** @brief Enabled interrupt mask */
#define E1000E_ENABLED_INTERRUPTS (E1000E_IMS_TXDW | \
                                   E1000E_IMS_LSC  | \
                                   E1000E_IMS_RXT0)

/* --- Device PBA Register Bits --- */
/** @brief E1000 PBA (Packet Buffer Allocation) Register value mask */
#define E1000E_PBA_RX_MASK 0x0000FFFF
/** @brief E1000 PBA (Packet Buffer Allocation) Register value shift */
#define E1000E_PBA_RX_SHIFT 0

/* --- Status Register Bits --- */
/** @brief Status Register: Link Up */
#define E1000E_STATUS_LU (1 << 1)
/** @brief Status Register: Speed */
#define E1000E_STATUS_SPEED_MASK (3 << 6)
/** @brief Status Register: Speed 1000 Mbps */
#define E1000E_STATUS_SPEED_1000A (2 << 6)
/** @brief Status Register: Speed 100 Mbps */
#define E1000E_STATUS_SPEED_1000B (3 << 6)
/** @brief Status Register: Speed 10 Mbps */
#define E1000E_STATUS_SPEED_100  (1 << 6)
/** @brief Status Register: Speed 10 Mbps */
#define E1000E_STATUS_SPEED_10   (0 << 6)
/** @brief Status Register: Full Duplex */
#define E1000E_STATUS_FD (1 << 0)

/* --- Auto-Negotiation Masks --- */
/** @brief Auto-Negotiation Mask: 1000BASE-T Full Duplex */
#define E1000E_AUTONEG_MASK_1000_FULL 0x20
/** @brief Auto-Negotiation Mask: 1000BASE-T Half Duplex */
#define E1000E_AUTONEG_MASK_1000_HALF 0x10
/** @brief Auto-Negotiation Mask: 100BASE-TX Full Duplex */
#define E1000E_AUTONEG_MASK_100_FULL 0x08
/** @brief Auto-Negotiation Mask: 100BASE-TX Half Duplex */
#define E1000E_AUTONEG_MASK_100_HALF 0x04
/** @brief Auto-Negotiation Mask: 10BASE-T Full Duplex */
#define E1000E_AUTONEG_MASK_10_FULL 0x02
/** @brief Auto-Negotiation Mask: 10BASE-T Half Duplex */
#define E1000E_AUTONEG_MASK_10_HALF 0x01
/** @brief Auto-Negotiation Mask: Off */
#define E1000E_AUTONEG_MASK_OFF 0x00

/* --- Miscellaneous --- */
/** @brief E1000 PBA (Packet Buffer Allocation) Register default value */
#define E1000E_PBA_RX_DEFAULT 0x00000030

/** @brief E1000 FCRTL Register XON Enable Bit */
#define E1000E_FCRTL_XONE (1 << 31)

/** @brief E1000E RXDCTL Register Packet Threshold Mask */
#define E1000E_RXDCTL_PTHRESH_MASK 0x0000003F
/** @brief E1000E RXDCTL Register Header Threshold Mask */
#define E1000E_RXDCTL_HTHRESH_MASK 0x00003F00
/** @brief E1000E RXDCTL Register Writeback Threshold Mask */
#define E1000E_RXDCTL_WTHRESH_MASK 0x003F0000

/** @brief E1000E RXDCTL Register Packet Threshold Default Value */
#define E1000E_RXDCTL_PTHRESH_DEFAULT 0x1F
/** @brief E1000E RXDCTL Register Header Threshold Default Value */
#define E1000E_RXDCTL_HTHRESH_DEFAULT 0x01
/** @brief E1000E RXDCTL Register Writeback Threshold Default Value */
#define E1000E_RXDCTL_WTHRESH_DEFAULT 0x00
/** @brief E1000E RXDCTL Register Granularity Bit */
#define E1000E_RXDCTL_GRAN (1 << 24)

/** @brief E1000E TXDCTL Register Packet Threshold Mask */
#define E1000E_TXDCTL_PTHRESH_MASK 0x0000003F
/** @brief E1000E TXDCTL Register Header Threshold Mask */
#define E1000E_TXDCTL_HTHRESH_MASK 0x00003F00
/** @brief E1000E TXDCTL Register Writeback Threshold Mask */
#define E1000E_TXDCTL_WTHRESH_MASK 0x003F0000
/** @brief E1000E TXDCTL Register Packet Threshold Default Value */
#define E1000E_TXDCTL_PTHRESH_DEFAULT 0x1F
/** @brief E1000E TXDCTL Register Header Threshold Default Value */
#define E1000E_TXDCTL_HTHRESH_DEFAULT 0x01
/** @brief E1000E TXDCTL Register Writeback Threshold Default Value */
#define E1000E_TXDCTL_WTHRESH_DEFAULT 0x00
/** @brief E1000E TXDCTL Register Granularity Bit */
#define E1000E_TXDCTL_GRAN (1 << 24)

/* --- EEPROM --- */
/** @brief EEPROM Read Status Flags: Address Shift */
#define E1000E_EERD_ADDR_SHIFT 2
/** @brief EEPROM Read Status Flags: Data Shift */
#define E1000E_EERD_DATA_SHIFT 16
/** @brief EEPROM Read Status Flags: Start */
#define E1000E_EERD_START (1 << 0)
/** @brief EEPROM Read Status Flags: Done */
#define E1000E_EERD_DONE (1 << 1)*/

/** @brief E1000E IP Control Register: EEE Enable for 100M */
#define E1000_IPCNFG_EEE_100M_AN (1 << 2)
/** @brief E1000E IP Control Register: EEE Enable for 1000M */
#define E1000_IPCNFG_EEE_1G_AN (1 << 3)  /* Enable 1000BASE-T EEE Auto-Neg */

#define E1000E_IPCNFG_EEE_100M_AN
#define E1000E_IPCNFG_EEE_1G_AN
#define E1000E_PHY_INIT_TIMEOUT_NS
#define E1000E_WUC_PME_EN

#define E1000E_WUC_APME
#define E1000E_WUC_PME_STATUS
#define E1000E_GCR_L1_ACT_WITHOUT_L0S_RX
#define E1000E_CTRL_EXT_DMA_DYN_CLK

#define E1000E_MULTICAST_TABLE_SIZE 128
#define E1000E_VLAN_FILTER_TABLE_SIZE 128

#define E1000E_RXCSUM_CLEAR_MASK
#define E1000E_RXCSUM_IPOFL
#define E1000E_RXCSUM_TUOFL
#define E1000E_RXCSUM_IPV6OFL

#define E1000E_STAT_UPDATE_INTERVAL_NS

#define E1000E_TCTL_PSP

#define E1000E_TRANSACTION_WAIT_NS
#define E1000E_TRANSACTION_WAIT_COUNT

#define E1000_TX_BUFFER_SIZE 2048

#define E1000E_TX_CMD_IDE
#define E1000E_TX_CMD_IFCS
#define E1000E_TX_CMD_EOP
#define E1000E_TX_CMD_RS

#define E1000E_ICR_RXT0
#define E1000E_ICR_LSC
#define E1000E_ICR_TXDW

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief The type of the E1000E media. */
typedef enum
{
  /** @brief Traditional PHY management/auto-negotiation */
  MEDIA_COPPER,
  /** @brief Fiber SerDes/optical link state machine */
  MEDIA_FIBER,
  /** @brief Internal Gigabit Serializer/Deserializer interface */
  MEDIA_SERDES
} E_E1000EMediaType;

/** @brief The type of the E1000E loopback mode. */
typedef enum
{
  /** @brief E1000E Loopback Mode: None */
  LOOPBACK_NONE,
  /** @brief E1000E Loopback Mode: PHY */
  LOOPBACK_PHY,
  /** @brief E1000E Loopback Mode: Internal SerDes */
  LOOPBACK_SERDES,
  /** @brief E1000E Loopback Mode: External SerDes */
  LOOPBACK_EXTERNAL_SERDES
} E_E1000ELoopbackMode;

/** @brief The type of the E1000E flow control. */
typedef enum
{
  /** @brief E1000E Flow Control: None */
  FLOW_CONTROL_NONE,
  /** @brief E1000E Flow Control: RX Pause */
  FLOW_CONTROL_RX_PAUSE,
  /** @brief E1000E Flow Control: TX Pause */
  FLOW_CONTROL_TX_PAUSE,
  /** @brief E1000E Flow Control: RX/TX Pause */
  FLOW_CONTROL_RX_TX_PAUSE
} E_E1000EFlowControl;

/** @brief The type of the E1000E semaphore. */
typedef enum
{
  /** @brief E1000E Semaphore: NVM */
  E1000E_SEM_NVM,
  /** @brief E1000E Semaphore: PHY */
  E1000E_SEM_PHY
} E_E1000ESemaphore;

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

/** @brief E1000E Buffer Descriptor Structure */
typedef struct
{
  /** @brief The physical address of the buffer. */
  uintptr_t physAddr;
  /** @brief The virtual address of the buffer. */
  void* virtAddr;
} S_E1000EBufferDescriptor;

/** @brief E1000E TX Buffer Metadata Structure */
typedef struct
{
  /** @brief The virtual address of the TX buffer. */
  uintptr_t virtAddr;
  /** @brief The length of the TX buffer. */
  size_t length;
} S_E1000ETXBufferMetadata;

/** @brief The E1000E driver structure. */
typedef struct S_E1000EDriver
{
  /** @brief The PCI device descriptor. */
  S_PCIDevice* pPCIDevice;
  /** @brief The PCI device identifier. */
  uint16_t deviceId;

  /** @brief The media type. */
  E_E1000EMediaType mediaType;

  /** @brief The base address of the device's BAR. */
  uintptr_t barAddress;
  /** @brief The size of the device's BAR. */
  size_t barSize;

  /** @brief The MAC address of the device. */
  uint64_t macAddress;
  /** @brief The PHY address of the device. */
  uint8_t phyAddress;

  /** @brief The RX descriptor ring address. */
  S_RXBufferDescriptor* pRXDescriptorRing;
  /** @brief The addresses of the RX descriptors in use. */
  S_E1000EBufferDescriptor* pRXDescriptorAddresses;
  /** @brief The RX descriptor memory size (ring + addresses). */
  size_t rxDescriptorMemorySize;
  /** @brief The index of the next RX descriptor to use. */
  uint32_t rxDescriptorIdx;
  /** @brief The list of RX fragments. */
  S_NetworkFragmentList* pRXFragmentList;

  /** @brief The TX descriptor ring address. */
  S_TXBufferDescriptor* pTXDescriptorRing;
  /** @brief The addresses of the TX descriptors in use. */
  S_E1000ETXBufferMetadata* pTXDescriptorMeta;
  /** @brief The TX descriptor memory size (ring + addresses). */
  size_t txDescriptorMemorySize;
  /** @brief The index of the next TX descriptor to use. */
  uint32_t txDescriptorIdx;
  /** @brief The index of the next TX descriptor to clean. */
  uint32_t txDescriptorCleanIdx;
  /** @brief The index of the next TX descriptor to watch. */
  uint32_t txDescriptorWatchIdx;
  /** @brief The ticket for the next TX operation. */
  uint32_t txTicket;
  /** @brief The service index for the next TX operation. */
  uint32_t txService;

  /** @brief Whether to use legacy interrupts. */
  bool useMSIXInterrupts;
  /** @brief The interrupt line used by the device. */
  int32_t interruptLine;

  /** @brief The network device descriptor. */
  S_NetworkDevice networkDevice;

  /** @brief The lock for the TX ring. */
  T_Spinlock txRingLock;

  /** @brief The lock for the device configuration. */
  S_KernelMutex configLock;

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
  E_E1000ELoopbackMode loopbackMode;
  /** @brief The flow control type. */
  E_E1000EFlowControl flowControl;
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
  T_U32Atomic txOperationRunning;
  /** @brief Whether the TX complete operation is running. */
  bool txCompleteOperationRunning;
  /** @brief Whether the link operation is running. */
  bool linkOperationRunning;
  /** @brief Whether the IOCTL operation is running. */
  T_U32Atomic ioctlOperationRunning;

  /** @brief Whether the link is up. */
  bool isLinkUp;
  /** @brief Whether the driver has been initialized. */
  bool isInitialized;

  /** @brief The network statistics for the device. */
  S_NetworkStatistics stats;
  /** @brief The lock for the network statistics. */
  T_Spinlock statsLock;

  /** @brief The task handle for the statistics operation. */
  T_NetManagerTask statTaskHandle;

  /** @brief The cached value of the Interrupt Cause Read register. */
  uint32_t cachedICR;
  /** @brief The core that will handle the interrupts for this driver. */
  uint32_t handlerCore;

  /** @brief The next E1000E driver in the linked list. */
  struct S_E1000EDriver* pNext;
} S_E1000EDriver;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the E1000E to ensure correctness of execution.
 *
 * @details Assert macro used by the E1000E to ensure correctness of execution.
 * Due to the critical nature of the E1000E, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define E1000E_ASSERT(COND, MSG, ERROR)              \
  {                                                 \
    if ((COND) == false)                            \
    {                                               \
      PANIC(ERROR, MODULE_NAME, MSG, false, false); \
    }                                               \
  }

/**
 * @brief Macro to access E1000E registers.
 *
 * @details Macro to access E1000E registers. It calculates the address of the
 * register based on the base address and the offset.
 *
 * @param[in] BASE The base address of the E1000E device.
 * @param[in] OFFSET The offset of the register to access.
 *
 * @return A pointer to the volatile uint32_t register.
 */
#define E1000E_REG(BASE, OFFSET) \
  ((volatile uint32_t *)((uintptr_t)(BASE) + (uintptr_t)(OFFSET)))

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Locks the specified E1000E semaphore.
 *
 * @details Locks the specified E1000E semaphore. This function will block until
 * the semaphore is acquired or a timeout occurs.
 *
 * @param[in] pDriver The E1000E driver instance.
 * @param[in] kSemaphore The semaphore to lock.
 *
 * @return The success state or the error code.
 */
static E_Return _SemaphoreLock(S_E1000EDriver*         pDriver,
                               const E_E1000ESemaphore kSemaphore);

/**
 * @brief Unlocks the specified E1000E semaphore.
 *
 * @details Unlocks the specified E1000E semaphore. This function will release
 * the semaphore and allow other operations to proceed.
 *
 * @param[in] pDriver The E1000E driver instance.
 * @param[in] kSemaphore The semaphore to unlock.
 */
static void _SemaphoreUnlock(S_E1000EDriver*         pDriver,
                             const E_E1000ESemaphore kSemaphore);

/**
 * @brief Validates the E1000E EEPROM.
 *
 * @details Validates the E1000E EEPROM by reading its contents and checking for
 * errors. This function is used to ensure that the EEPROM is functioning
 * correctly before proceeding with further operations.
 *
 * @param[in] kpDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _ValidateEEPROM(const S_E1000EDriver* kpDriver);

/**
 * @brief Reads a value from the E1000E EEPROM.
 *
 * @details Reads a value from the E1000E EEPROM at the specified address.
 *
 * @param[in] kpDriver The E1000E driver instance.
 * @param[in] kAddress The address to read from the EEPROM.
 * @param[out] pValue The value read from the EEPROM.
 *
 * @return The success state or the error code.
 */
static E_Return _ReadEEPROM(const S_E1000EDriver* kpDriver,
                            const uint16_t       kAddress,
                            uint16_t*            pValue);

/**
 * @brief Reads a value from the MDIO interface.
 *
 * @details Reads a value from the MDIO interface of the E1000E device.
 *
 * @param[in] kpDriver The E1000E driver instance.
 * @param[in] kRegAddress The address of the register to read.
 * @param[out] pValue The value read from the register.
 *
 * @return The success state or the error code.
 */
static E_Return _ReadMDIO(const S_E1000EDriver* kpDriver,
                          const uint8_t        kRegAddress,
                          uint16_t*            pValue);

/**
 * @brief Writes a value to the MDIO interface.
 *
 * @details Writes a value to the MDIO interface of the E1000E device.
 *
 * @param[in] kpDriver The E1000E driver instance.
 * @param[in] kRegAddress The address of the register to write.
 * @param[in] kValue The value to write to the register.
 *
 * @return The success state or the error code.
 */
static E_Return _WriteMDIO(const S_E1000EDriver* kpDriver,
                           const uint8_t        kRegAddress,
                           const uint16_t       kValue);
/**
 * @brief Attaches the E1000E driver to the system.
 *
 * @details Attaches the E1000E driver to the system. This function will use the
 * PCI device to initialize the E1000E hardware and retreive the E1000
 * parameters.
 *
 * @param[in] pDevice The PCI device node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(S_PCIDevice* pDevice);

/**
 * @brief Detaches the E1000E driver from the system.
 *
 * @details Detaches the E1000E driver from the system. This function will
 * release all resources allocated by the driver and remove it from the system.
 *
 * @param[in] pDriver The E1000E driver instance to detach.
 */
static void _Detach(S_E1000EDriver* pDriver);

/**
 * @brief Probes the E1000E device for its capabilities.
 *
 * @details Probes the E1000E device for its capabilities and initializes the
 * driver structure accordingly.
 *
 * @param[in] kDeviceId The device ID to probe.
 * @param[out] pDriver The driver structure to initialize.
 *
 * @return The success state or the error code.
 */
static E_Return _ProbeDevice(const uint16_t kDeviceId, S_E1000EDriver* pDriver);

/**
 * @brief Loads the default device parameters into the driver structure.
 *
 * @details Loads the default device parameters into the driver structure. This
 * function is used to initialize the default parameters of the E1000E device
 * before probing for its capabilities.
 *
 * @param[out] pDriver The driver structure to initialize.
 */
static void _LoadDefaultDevice(S_E1000EDriver* pDriver);

/**
 * @brief Detects the E1000E device and initializes the driver structure.
 *
 * @details Detects the E1000E device and initializes the driver structure with
 * the device's capabilities and parameters.
 *
 * @param[in] pDevice The PCI device node to detect.
 * @param[out] pDriver The driver structure to initialize.
 *
 * @return The success state or the error code.
 */
static E_Return _DetectDevice(S_PCIDevice* pDevice, S_E1000EDriver* pDriver);

/**
 * @brief Resets the E1000E device.
 *
 * @details Resets the E1000E device to its default state.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _ResetDevice(S_E1000EDriver* pDriver);

/**
 * @brief Allocates the resources for the E1000E device.
 *
 * @details Allocates the resources for the E1000E device, such as memory and
 * buffer.
 *
 * @param[in] pDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _AllocateDeviceResources(S_E1000EDriver* pDriver);

/**
 * @brief Allocates the RX resources for the E1000E device.
 *
 * @details Allocates the RX resources for the E1000E device, such as memory
 * and buffer.
 *
 * @param[in] pDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _AllocateDeviceRXResources(S_E1000EDriver* pDriver);

/**
 * @brief Allocates the TX resources for the E1000E device.
 *
 * @details Allocates the TX resources for the E1000E device, such as memory
 * and buffer.
 *
 * @param[in] pDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _AllocateDeviceTXResources(S_E1000EDriver* pDriver);

/**
 * @brief Sets the MAC address of the E1000E device.
 *
 * @details Sets the MAC address of the E1000E device to the specified value.
 *
 * @param[in] pDriver The E1000E driver instance.
 * @param[in] kMACAddress The MAC address to set.
 */
static void _SetDeviceMAC(S_E1000EDriver* pDriver, const uint64_t kMACAddress);

/**
 * @brief Releases the resources of the E1000E device.
 *
 * @details Releases the resources of the E1000E device, such as memory and
 * buffers
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _ReleaseDeviceResources(S_E1000EDriver* pDriver);

/**
 * @brief Releases the RX resources of the E1000E device.
 *
 * @details Releases the RX resources of the E1000E device, such as memory and
 * buffers
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _ReleaseDeviceRXResources(S_E1000EDriver* pDriver);

/**
 * @brief Releases the TX resources of the E1000E device.
 *
 * @details Releases the TX resources of the E1000E device, such as memory and
 * buffers
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _ReleaseDeviceTXResources(S_E1000EDriver* pDriver);

/**
 * @brief Initializes the control flow of the E1000E device.
 *
 * @details Initializes the control flow of the E1000E device, for both RX and TX
 * paths, and sets up the necessary registers and configurations.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _InitializeControlFlow(S_E1000EDriver* pDriver);

/**
 * @brief Initializes the RX path of the E1000E device.
 *
 * @details Initializes the RX path of the E1000E device, such as setting up
 * the RX descriptor ring and configuring the device registers.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _InitializeDeviceRX(S_E1000EDriver* pDriver);

/**
 * @brief Initializes the TX path of the E1000E device.
 *
 * @details Initializes the TX path of the E1000E device, such as setting up
 * the TX descriptor ring and configuring the device registers.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _InitializeDeviceTX(S_E1000EDriver* pDriver);

/**
 * @brief Sets up the device interrupts for the E1000E device.
 *
 * @details Sets up the device interrupts for the E1000E device, including
 * configuring the interrupt mask and enabling the necessary interrupts.
 *
 * @param[in] pDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _SetupDeviceInterrupts(S_E1000EDriver* pDriver);

/**
 * @brief Sets the PHY to forced mode.
 *
 * @details Sets the PHY to forced mode, which means it will use a specific
 * speed and duplex setting.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _SetPhyForced(S_E1000EDriver* pDriver);

/**
 * @brief Sets the PHY to auto-negotiation mode.
 *
 * @details Sets the PHY to auto-negotiation mode, which means it will
 * automatically negotiate the speed and duplex settings with the link partner.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _SetPhyAutoNeg(S_E1000EDriver* pDriver);

/**
 * @brief Sets the PHY to non-copper mode.
 *
 * @details Sets the PHY to non-copper mode, which means it will use a
 * different interface type, such as fiber or SerDes.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _SetPhyNonCopper(S_E1000EDriver* pDriver);

/**
 * @brief Initializes the PHY of the E1000E device.
 *
 * @details Initializes the PHY of the E1000E device, including setting up the
 * necessary registers and configurations for proper operation.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _InitializePhy(S_E1000EDriver* pDriver);

/**
 * @brief Configures the power management of the E1000E device.
 *
 * @details Configures the power management of the E1000E device, enabling or
 * disabling active mode based on the provided parameter.
 *
 * @param[in] pDriver The E1000E driver instance.
 * @param[in] kEnableActiveMode Whether to enable active mode.
 */
static void _ConfigurePowerManagement(S_E1000EDriver* pDriver,
                                      const bool     kEnableActiveMode);

/**
 * @brief Sets up the multicast table for the E1000E device.
 *
 * @details Sets up the multicast table for the E1000E device, which is used to
 * filter incoming multicast packets based on the configured multicast
 * addresses.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _SetupMulticastTable(S_E1000EDriver* pDriver);

/**
 * @brief Sets up the VLAN for the E1000E device.
 *
 * @details Sets up the VLAN for the E1000E device, which is used to filter
 * incoming packets based on the configured VLAN ID.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _SetupVLAN(S_E1000EDriver* pDriver);

/**
 * @brief Brings the E1000E device link up after locking the configuration.
 *
 * @details Brings the E1000E device link up, which means it will start
 * transmitting and receiving packets. This function allocates the necessary
 * resources and initializes the device for operation.
 *
 * @param[in] pNetworkDevice The network device to set up.
 *
 * @return The success state or the error code.
 */
static E_Return _DevUplink(S_NetworkDevice* pNetworkDevice);

/**
 * @brief Brings the E1000E device link down after locking the configuration.
 *
 * @details Brings the E1000E device link down, which means it will stop
 * transmitting and receiving packets. This function releases the allocated
 * resources and deinitializes the device.
 *
 * @param[in] pNetworkDevice The network device to set down.
 *
 * @return The success state or the error code.
 */
static E_Return _DevDownlink(S_NetworkDevice* pNetworkDevice);

/**
 * @brief Brings the E1000E device link up.
 *
 * @details Brings the E1000E device link up, which means it will start
 * transmitting and receiving packets. This function allocates the necessary
 * resources and initializes the device for operation.
 *
 * @param[in] pDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _DevUplinkSafe(S_E1000EDriver* pDriver);

/**
 * @brief Brings the E1000E device link down.
 *
 * @details Brings the E1000E device link down, which means it will stop
 * transmitting and receiving packets. This function releases the allocated
 * resources and deinitializes the device.
 *
 * @param[in] pDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _DevDownlinkSafe(S_E1000EDriver* pDriver);

/**
 * @brief Waits for the completion of a transaction on the E1000E device.
 *
 * @details Waits for the completion of a transaction on the E1000E device,
 * checking the status of the device and ensuring that the operation has
 * completed successfully.
 *
 * @param[in] pDriver The E1000E driver instance.
 *
 * @return The success state or the error code.
 */
static E_Return _WaitForTransactionCompletion(S_E1000EDriver* pDriver);

/**
 * @brief Transmits a packet through the E1000E device.
 *
 * @details Transmits a packet through the E1000E device, using the provided
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
 * @brief Handles the IOCTL commands for the E1000E device.
 *
 * @details Handles the IOCTL commands for the E1000E device, allowing for
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
 * @brief Interrupt handler for the E1000E device.
 *
 * @details Interrupt handler for the E1000E device. This function is called
 * when an interrupt is generated by the device, and it handles the necessary
 * processing of the interrupt, including cleaning up completed transmissions
 * and processing received packets.
 *
 * @return The function returns true if the interrupted thread needs to be
 * scheduled, false otherwise.
 */
static bool _E1000EInterruptHandler(void);

/**
 * @brief Worker function to handle interrupts for the E1000E device.
 *
 * @details Worker function to handle interrupts for the E1000E device. This
 * function is called by the network worker thread and performs the necessary
 * processing of the interrupt in a separate context.
 *
 * @param[in] pDriverPtr Pointer to the E1000E driver instance.
 */
static void _HandleInterruptWorker(void* pDriverPtr);

/**
 * @brief Handles the link status change for the E1000E device.
 *
 * @details Handles the link status change for the E1000E device. This function
 * is called when the link status changes, and it updates the driver state
 * accordingly.
 *
 * @param[in] pDriverPtr Pointer to the E1000E driver instance.
 */
static void _HandleLinkStatusChange(void* pDriverPtr);

/**
 * @brief Cleans up completed transmissions for the E1000E device.
 *
 * @details Cleans up completed transmissions for the E1000E device. This
 * function checks the status of the TX descriptors and frees up any resources
 * associated with completed transmissions.
 *
 * @param[in] pDriver The E1000E driver instance.
 * @param[out] pIsMoreToClean Pointer to a boolean indicating if there are more
 * transmissions to clean.
 */
static void _CompleteTransmit(S_E1000EDriver* pDriver, bool* pIsMoreToClean);

/**
 * @brief Handles received packets for the E1000E device.
 *
 * @details Handles received packets for the E1000E device. This function
 * processes the RX descriptors and passes the received packets to the upper
 * network stack.
 *
 * @param[in] pDriver The E1000E driver instance.
 */
static void _HandleReceivedPacket(S_E1000EDriver* pDriver);

/**
 * @brief Updates the network statistics for the E1000E device.
 *
 * @details Updates the network statistics for the E1000E device. This function
 * reads the relevant registers and updates the statistics in the driver
 * structure.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 */
static void _UpdateStatistics(void* pDriver);

/**
 * @brief Handles the IOCTL command to get the network statistics.
 *
 * @details Handles the IOCTL command to get the network statistics for the
 * E1000E device. This function retrieves the current statistics and copies them
 * to the provided argument.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 * @param[in] pArg Pointer to the argument where the statistics will be copied.
 *
 * @return The size of the statistics structure is returned on success, a
 * negative error code is returned on failure.
 */
static ssize_t _IOControlGetStats(S_E1000EDriver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to reset the network statistics.
 *
 * @details Handles the IOCTL command to reset the network statistics for the
 * E1000E device. This function resets the statistics counters in the driver
 * structure.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlResetStats(S_E1000EDriver* pDriver);

/**
 * @brief Handles the IOCTL command to set the MAC address.
 *
 * @details Handles the IOCTL command to set the MAC address for the E1000
 * device. This function updates the MAC address in the driver structure and
 * configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 * @param[in] pArg Pointer to the argument containing the new MAC address.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlSetMACAddress(S_E1000EDriver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to get the MAC address.
 *
 * @details Handles the IOCTL command to get the MAC address for the E1000
 * device. This function retrieves the current MAC address from the driver
 * structure and copies it to the provided argument.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 * @param[in] pArg Pointer to the argument where the MAC address will be copied.
 *
 * @return The size of the MAC address structure is returned on success, a
 * negative error code is returned on failure.
 */
static ssize_t _IOControlGetMACAddress(S_E1000EDriver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to set the loopback mode.
 *
 * @details Handles the IOCTL command to set the loopback mode for the E1000
 * device. This function updates the loopback mode in the driver structure and
 * configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 * @param[in] pArg Pointer to the argument containing the new loopback mode.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlSetLoopbackMode(S_E1000EDriver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to get the link status.
 *
 * @details Handles the IOCTL command to get the link status for the E1000
 * device. This function retrieves the current link status from the driver
 * structure and copies it to the provided argument.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 * @param[in] pArg Pointer to the argument where the link status will be copied.
 *
 * @return The link status is returned on success (0 = down, 1 = up), a negative
 * error code is returned on failure.
 */
static ssize_t _IOControlGetLinkStatus(S_E1000EDriver* pDriver, void* pArg);

/**
 * @brief Handles the IOCTL command to set the auto-negotiation mode.
 *
 * @details Handles the IOCTL command to set the auto-negotiation mode for the
 * E1000E device. This function updates the auto-negotiation settings in the
 * driver structure and configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlSetAutoNeg(S_E1000EDriver* pDriver);

/**
 * @brief Handles the IOCTL command to force the speed and duplex settings.
 *
 * @details Handles the IOCTL command to force the speed and duplex settings
 * for the E1000E device. This function updates the speed and duplex settings in
 * the driver structure and configures the device accordingly.
 *
 * @param[in] pDriver Pointer to the E1000E driver instance.
 * @param[in] pArg Pointer to the argument containing the new speed and duplex
 * settings.
 *
 * @return Zero is returned on success, a negative error code is returned on
 * failure.
 */
static ssize_t _IOControlForceSpeedDuplex(S_E1000EDriver* pDriver, void* pArg);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief E1000E driver instance. */
static S_PCIDriver sX86E1000EDriver =
{
  .pName = "X86 E1000E Driver",
  .pDescription = "X86 Intel E1000E Driver for roOs",
  .pVersion = "1.0",
  .vendorID = INTEL_VENDOR_ID,
  .deviceID = 0xFFFF,
  .devClass = 0x02,
  .devSubClass = 0x00,
  .devProgIf = 0xFF,
  .pDriverAttach = _Attach
};
/** @brief E1000E driver linked list. */
static S_E1000EDriver* spDriverList = NULL;
/** @brief E1000E driver list lock. */
static T_Spinlock sDriverListLock = SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _SemaphoreLock(S_E1000EDriver*         pDriver,
                               const E_E1000ESemaphore kSemaphore)
{
  E_Return error;
  uint64_t timeout;
  uint32_t swSemaphore;
  uint32_t semaphoreValue;
  uint32_t semaphoreReg;

  switch (kSemaphore)
  {
    case E1000E_SEM_PHY:
      semaphoreValue = E1000E_EXTCNF_CTRL_SWOWN;
      semaphoreReg   = E1000E_EXTCNF_CTRL;
      error = NO_ERROR;
      break;
    case E1000E_SEM_NVM:
      semaphoreValue = E1000E_SWSM_SWESMBI;
      semaphoreReg   = E1000E_SWSM;
      error = NO_ERROR;
      break;
    default:
      KERNEL_ERROR("E1000E invalid semaphore lock request\n");
      error = ERR_INVALID_PARAMETER;
  }

  if (error == NO_ERROR)
  {
    timeout = TimeGetUptime() + E1000E_SEMAPHORE_LOCK_TIMEOUT_NS;
    do
    {
      swSemaphore = _MMIORead32(E1000E_REG(pDriver->barAddress, semaphoreReg));
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_SWSM),
                  swSemaphore | semaphoreValue);

      swSemaphore = _MMIORead32(E1000E_REG(pDriver->barAddress, semaphoreReg));
      if ((swSemaphore & semaphoreValue) != 0)
      {
        break;
      }
    } while (TimeGetUptime() < timeout);

    if (TimeGetUptime() >= timeout)
    {
      KERNEL_ERROR("E1000E semaphore lock timeout\n");
      error = ERR_EXCEEDED_LIMIT;
    }

    CPUPause();
  }

  return error;
}

static void _SemaphoreUnlock(S_E1000EDriver*         pDriver,
                             const E_E1000ESemaphore kSemaphore)
{
  uint32_t swSemaphore;
  uint32_t semaphoreValue;
  uint32_t semaphoreReg;

  switch (kSemaphore)
  {
    case E1000E_SEM_PHY:
      semaphoreValue = E1000E_EXTCNF_CTRL_SWOWN;
      semaphoreReg   = E1000E_EXTCNF_CTRL;
      error = NO_ERROR;
      break;
    case E1000E_SEM_NVM:
      semaphoreValue = E1000E_SWSM_SWESMBI;
      semaphoreReg   = E1000E_SWSM;
      error = NO_ERROR;
      break;
    default:
      KERNEL_ERROR("E1000E invalid semaphore unlock request\n");
      semaphoreValue = 0;
  }

  if (semaphoreValue != 0)
  {
    swSemaphore = _MMIORead32(E1000E_REG(pDriver->barAddress, semaphoreReg));
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, semaphoreReg),
                 swSemaphore & ~semaphoreValue);
  }
}

static E_Return _ValidateEEPROM(const S_E1000EDriver* kpDriver)
{
  E_Return error;
  uint16_t signature;
  uint32_t i;
  uint32_t retry;
  uint16_t checksum;



  for (retry = 0; retry < E1000E_EEPROM_SIGNATURE_RETRIES; ++retry)
  {
    checksum = 0;
    for (i = 0; i < E1000E_EEPROM_SIZE; ++i)
    {
      error = _ReadEEPROM(kpDriver, i, &signature);
      if (error == NO_ERROR)
      {
        checksum += signature;
        if (i == E1000E_EEPROM_SIZE - 1)
        {
          if (signature == E1000E_EEPROM_SIGNATURE)
          {
            KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                         MODULE_NAME,
                         "E1000E EEPROM signature validated successfully");
            error = NO_ERROR;
          }
          else
          {
            KERNEL_ERROR("E1000E EEPROM signature mismatch: expected 0x%04X, "
                         "got 0x%04X\n",
                         E1000E_EEPROM_SIGNATURE,
                         signature);
            error = ERR_INVALID_VALUE;
          }
        }
      }
      else
      {
        KERNEL_ERROR("E1000E EEPROM signature read failed with error code %d\n",
                      error);
        break;
      }
    }
    if (error == NO_ERROR)
    {
      break;
    }
    SleepNs(E1000E_EEPROM_SIGNATURE_RETRY_DELAY_NS);
  }

  return error;
}

static E_Return _ReadEEPROM(const S_E1000EDriver* kpDriver,
                            const uint16_t        kAddress,
                            uint16_t*             pValue)
{
  E_Return error;
  uint32_t regValue;
  uint16_t address;
  uint64_t timeout;

  error = _SemaphoreLock((S_E1000EDriver*)kpDriver, E1000E_SEM_NVM);
  if (error == NO_ERROR)
  {
    regValue = E1000E_EERD_START |
              ((address & 0x3FFF) << E1000E_EERD_ADDR_SHIFT);
    _MMIOWrite32(E1000E_REG(kpDriver->barAddress, E1000E_EERD), regValue);

    timeout = TimeGetUptime() + E1000E_EEPROM_READ_TIMEOUT_NS;
    do
    {
      regValue = _MMIORead32(E1000E_REG(kpDriver->barAddress, E1000E_EERD));
      if (TimeGetUptime() > timeout)
      {
        break;
      }
    } while ((regValue & E1000E_EERD_DONE) == 0);

    if ((regValue & E1000E_EERD_DONE) != 0)
    {
      /* Reset the EEPROM register */
      _MMIOWrite32(E1000E_REG(kpDriver->barAddress, E1000E_EERD), 0);

      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E EEPROM read for address %u returned value 0x%04X",
                  kAddress,
                  (uint16_t)(regValue >> E1000E_EERD_DATA_SHIFT));
      *pValue = (uint16_t)(regValue >> E1000E_EERD_DATA_SHIFT);
      error = NO_ERROR;
    }
    else
    {
      KERNEL_ERROR("E1000E EEPROM read timeout for address %u\n", kAddress);
      error = ERR_EXCEEDED_LIMIT;
    }
    _SemaphoreUnlock((S_E1000EDriver*)kpDriver, E1000E_SEM_NVM);
  }

  return error;
}

static E_Return _ReadMDIO(const S_E1000EDriver* kpDriver,
                          const uint8_t        kRegAddress,
                          uint16_t*            pValue)
{
  E_Return error;
  uint32_t regValue;
  uint64_t timeout;

  error = _SemaphoreLock((S_E1000EDriver*)kpDriver, E1000E_SEM_NVM);
  if (error == NO_ERROR)
  {
    error = _SemaphoreLock((S_E1000EDriver*)kpDriver, E1000E_SEM_PHY);
    if (error == NO_ERROR)
    {
      regValue = E1000E_MDIO_READ |
                ((kRegAddress & 0x1F) << E1000E_MDIO_ADDR_SHIFT) |
                ((kpDriver->phyAddress & 0x1F) << E1000E_MDIO_PHY_SHIFT);
      _MMIOWrite32(E1000E_REG(kpDriver->barAddress, E1000E_MDIC), regValue);

      timeout = TimeGetUptime() + E1000E_MDIO_ACCESS_TIMEOUT_NS;
      do
      {
        regValue = _MMIORead32(E1000E_REG(kpDriver->barAddress, E1000E_MDIC));
        if (TimeGetUptime() > timeout)
        {
          break;
        }
      } while ((regValue & E1000E_MDIO_DONE) == 0 &&
              (regValue & E1000E_MDIO_ERROR) == 0);

      if ((regValue & E1000E_MDIO_DONE) != 0 &&
          (regValue & E1000E_MDIO_ERROR) == 0)
      {
        KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000E MDIO read for address %u returned value 0x%04X",
                    kpDriver->phyAddress,
                    (uint16_t)(regValue & E1000E_MDIO_DATA_MASK));
        *pValue = (uint16_t)(regValue & E1000E_MDIO_DATA_MASK);
        error = NO_ERROR;
      }
      else
      {
        KERNEL_ERROR("E1000E MDIO read timeout for address %u\n",
                    kpDriver->phyAddress);
        error = ERR_EXCEEDED_LIMIT;
      }

      _SemaphoreUnlock((S_E1000EDriver*)kpDriver, E1000E_SEM_PHY);
    }

    _SemaphoreUnlock((S_E1000EDriver*)kpDriver, E1000E_SEM_NVM);
  }

  return error;
}

static E_Return _WriteMDIO(const S_E1000EDriver* kpDriver,
                           const uint8_t        kRegAddress,
                           const uint16_t       kValue)
{
  E_Return error;
  uint32_t regValue;
  uint64_t timeout;

  error = _SemaphoreLock((S_E1000EDriver*)kpDriver, E1000E_SEM_NVM);
  if (error == NO_ERROR)
  {
    error = _SemaphoreLock((S_E1000EDriver*)kpDriver, E1000E_SEM_PHY);
    if (error == NO_ERROR)
    {
      regValue = kValue |
                E1000E_MDIO_WRITE |
                (kRegAddress << E1000E_MDIO_ADDR_SHIFT) |
                (kpDriver->phyAddress << E1000E_MDIO_PHY_SHIFT);
      _MMIOWrite32(E1000E_REG(kpDriver->barAddress, E1000E_MDIC), regValue);
      timeout = TimeGetUptime() + E1000E_MDIO_ACCESS_TIMEOUT_NS;
      do
      {
        regValue = _MMIORead32(E1000E_REG(kpDriver->barAddress, E1000E_MDIC));
        if (TimeGetUptime() > timeout)
        {
          break;
        }
      } while ((regValue & E1000E_MDIO_DONE) == 0);

      if ((regValue & E1000E_MDIO_DONE) != 0)
      {
        KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000E MDIO write for address %u",
                    kpDriver->phyAddress);
        error = NO_ERROR;
      }
      else
      {
        KERNEL_ERROR("E1000E MDIO write timeout for address %u\n",
                    kpDriver->phyAddress);
        error = ERR_EXCEEDED_LIMIT;
      }

      _SemaphoreUnlock((S_E1000EDriver*)kpDriver, E1000E_SEM_PHY);
    }

    _SemaphoreUnlock((S_E1000EDriver*)kpDriver, E1000E_SEM_NVM);
  }

  return error;
}

static E_Return _Attach(S_PCIDevice* pDevice)
{
  E_Return        err;
  S_E1000EDriver* pE1000EDriver;
  uint8_t         function;
  uint32_t        intState;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Probing E1000E driver to device %04X\\%04X",
               pDevice->pFunction->vendorID,
               pDevice->pFunction->deviceID);

  /* Chech the vendor ID and device ID */
  if (pDevice->pFunction->vendorID == INTEL_VENDOR_ID)
  {
    pE1000EDriver = KMalloc(sizeof(S_E1000EDriver), KMALLOC_FREE_POOL);
    memset(pE1000EDriver, 0, sizeof(S_E1000EDriver));
    SPINLOCK_INIT(pE1000EDriver->txRingLock);
    SPINLOCK_INIT(pE1000EDriver->statsLock);

    err = KernelMutexInit(&pE1000EDriver->configLock, KMUTEX_FLAG_QUEUING_PRIO);
    if (err == NO_ERROR)
    {
      pE1000EDriver->pPCIDevice = pDevice;

      err = _ProbeDevice(pDevice->pFunction->deviceID, pE1000EDriver);
      if (err == NO_ERROR)
      {
        KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "Attaching E1000E driver to device %04X",
                    pDevice->pFunction->deviceID);

        err = _DetectDevice(pDevice, pE1000EDriver);
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

          err = _SetupDeviceInterrupts(pE1000EDriver);
          if (err == NO_ERROR)
          {
            err = NetManagerRegisterDevice(_DevUplink,
                                            _DevDownlink,
                                            _TransmitPacket,
                                            _IOControl,
                                            _HandleInterruptWorker,
                                            pDevice->bus,
                                            pDevice->slot,
                                            function,
                                            pE1000EDriver,
                                            &pE1000EDriver->networkDevice,
                                            pE1000EDriver->handlerCore);
            if (err == NO_ERROR)
            {
              KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                          MODULE_NAME,
                          "E1000E driver registered interface %s",
                          pE1000EDriver->networkDevice.pName);
              KERNEL_ENTER_CRITICAL_LOCAL(intState);
              SpinlockAcquire(&sDriverListLock);

              pE1000EDriver->pNext = spDriverList;
              spDriverList = pE1000EDriver;

              SpinlockRelease(&sDriverListLock);
              KERNEL_EXIT_CRITICAL_LOCAL(intState);
            }
            else
            {
              KERNEL_ERROR("Failed to register E1000E driver with manager\n");
              _Detach(pE1000EDriver);
            }
          }
          else
          {
            KERNEL_ERROR("Failed to setup E1000E driver interrupts\n");
            _Detach(pE1000EDriver);
          }
        }
        else
        {
          KERNEL_ERROR("Failed to detect E1000E device %04X\\%04X\n",
                      pDevice->pFunction->vendorID,
                      pDevice->pFunction->deviceID);
          _Detach(pE1000EDriver);
        }
      }
      else
      {
        KERNEL_ERROR("Unknown device E1000E device %04X\\%04X\n",
                    pDevice->pFunction->vendorID,
                    pDevice->pFunction->deviceID);
        KFree(pE1000EDriver);
      }
    }
    else
    {
      KERNEL_ERROR("Failed to initialize E1000E driver mutex\n");
      KFree(pE1000EDriver);
    }
  }
  else
  {
    KERNEL_ERROR("Unsupported vendor ID %04X\\%04X for E1000E driver\n",
                 pDevice->pFunction->vendorID,
                 pDevice->pFunction->deviceID);
    err = ERR_NOT_SUPPORTED;
  }

  return err;
}

static void _Detach(S_E1000EDriver* pDriver)
{
  E_Return       retCode;
  uint32_t       intState;
  S_E1000EDriver* pPrevDriver;

  if (pDriver->interruptLine > 0)
  {
    if (pDriver->useMSIXInterrupts == true)
    {
      PCISetupInterruptMSIX(pDriver->pPCIDevice, 0, 0, false);
    }
    else
    {
      PCISetupInterruptMSI(pDriver->pPCIDevice, 0, 0, false);
    }
    FreeInterruptLine(pDriver->interruptLine);
    pDriver->interruptLine = 0;
  }

  /* Unmap the BAR */
  if (pDriver->barAddress != 0)
  {
    retCode = MemoryKernelUnmap((void *)pDriver->barAddress, pDriver->barSize);
    E1000E_ASSERT(retCode == NO_ERROR,
                 "Failed to unmap BAR for device",
                 retCode);
  }

  /* Remove from the list */
  KERNEL_ENTER_CRITICAL_LOCAL(intState);
  SpinlockAcquire(&sDriverListLock);
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
  SpinlockRelease(&sDriverListLock);
  KERNEL_EXIT_CRITICAL_LOCAL(intState);

  KFree(pDriver);
}

static E_Return _ProbeDevice(const uint16_t kDeviceId, S_E1000EDriver* pDriver)
{
  E_Return retCode;

  retCode = NO_ERROR;

  pDriver->deviceId = kDeviceId;
  switch (kDeviceId)
  {
    case E1000E_DEV_ID_82571EB_COPPER:
    case E1000E_DEV_ID_82571EB_QUAD_COPPER:
    case E1000E_DEV_ID_82571PT_QUAD_COPPER:
    case E1000E_DEV_ID_82571EB_QUAD_COPPER_LP:
    case E1000E_DEV_ID_82572EI_COPPER:
    case E1000E_DEV_ID_82572EI:
    case E1000E_DEV_ID_82573E:
    case E1000E_DEV_ID_82573E_IAMT:
    case E1000E_DEV_ID_82573L:
    case E1000E_DEV_ID_82574L:
    case E1000E_DEV_ID_82574LA:
    case E1000E_DEV_ID_82583V:
    case E1000E_DEV_ID_80003ES2LAN_COPPER_DPT:
    case E1000E_DEV_ID_80003ES2LAN_COPPER:
    case E1000E_DEV_ID_ICH8_IGP_AMT:
    case E1000E_DEV_ID_ICH8_IGP_C:
    case E1000E_DEV_ID_ICH8_IGP_M_AMT:
    case E1000E_DEV_ID_ICH8_IGP_M:
    case E1000E_DEV_ID_ICH8_IGP_M_V:
    case E1000E_DEV_ID_ICH8_IFE:
    case E1000E_DEV_ID_ICH8_IFE_GT:
    case E1000E_DEV_ID_ICH8_82567V_3:
    case E1000E_DEV_ID_ICH9_IGP_AMT:
    case E1000E_DEV_ID_ICH9_IGP_C:
    case E1000E_DEV_ID_ICH9_IGP_M_AMT:
    case E1000E_DEV_ID_ICH9_IGP_M:
    case E1000E_DEV_ID_ICH9_IGP_M_V:
    case E1000E_DEV_ID_ICH9_IGP_C_ALT:
    case E1000E_DEV_ID_ICH9_IFE_GT:
    case E1000E_DEV_ID_ICH9_IFE_G:
    case E1000E_DEV_ID_ICH9_BM:
    case E1000E_DEV_ID_ICH10_R_BM_LM:
    case E1000E_DEV_ID_ICH10_R_BM_LF:
    case E1000E_DEV_ID_ICH10_R_BM_V:
    case E1000E_DEV_ID_ICH10_D_BM_LM:
    case E1000E_DEV_ID_ICH10_D_BM_LF:
    case E1000E_DEV_ID_ICH10_D_BM_V:
    case E1000E_DEV_ID_PCH_M_HW_LM:
    case E1000E_DEV_ID_PCH_M_HW_V:
    case E1000E_DEV_ID_PCH_D_HW_LM:
    case E1000E_DEV_ID_PCH_D_HW_V:
    case E1000E_DEV_ID_PCH2_LV_LM:
    case E1000E_DEV_ID_PCH2_LV_V:
    case E1000E_DEV_ID_PCH2_LV_LM:
    case E1000E_DEV_ID_PCH2_LV_V:
    case E1000E_DEV_ID_PCH_LPT_I217_LM:
    case E1000E_DEV_ID_PCH_LPT_I217_V:
    case E1000E_DEV_ID_PCH_LPTLP_I218_V:
    case E1000E_DEV_ID_PCH_LPTLP_I218_LM:
    case E1000E_DEV_ID_PCH_I218_LM2:
    case E1000E_DEV_ID_PCH_I218_V2:
    case E1000E_DEV_ID_PCH_I218_LM3:
    case E1000E_DEV_ID_PCH_I218_V3:
    case E1000E_DEV_ID_PCH_SPT_I219_LM:
    case E1000E_DEV_ID_PCH_SPT_I219_V:
    case E1000E_DEV_ID_PCH_SPT_I219_LM2:
    case E1000E_DEV_ID_PCH_SPT_I219_V2:
    case E1000E_DEV_ID_PCH_LBG_I219_LM3:
    case E1000E_DEV_ID_PCH_SPT_I219_V5:
    case E1000E_DEV_ID_PCH_SPT_I219_LM4:
    case E1000E_DEV_ID_PCH_SPT_I219_V4:
    case E1000E_DEV_ID_PCH_SPT_I219_LM5:
    case E1000E_DEV_ID_PCH_CNP_I219_LM7:
    case E1000E_DEV_ID_PCH_CNP_I219_V7:
    case E1000E_DEV_ID_PCH_CNP_I219_LM6:
    case E1000E_DEV_ID_PCH_CNP_I219_V6:
    case E1000E_DEV_ID_PCH_ICP_I219_LM8:
    case E1000E_DEV_ID_PCH_ICP_I219_V8:
    case E1000E_DEV_ID_PCH_ICP_I219_LM9:
    case E1000E_DEV_ID_PCH_ICP_I219_V9:
    case E1000E_DEV_ID_PCH_CMP_I219_LM11:
    case E1000E_DEV_ID_PCH_CMP_I219_V11:
    case E1000E_DEV_ID_PCH_CMP_I219_LM10:
    case E1000E_DEV_ID_PCH_CMP_I219_V10:
    case E1000E_DEV_ID_PCH_CMP_I219_LM12:
    case E1000E_DEV_ID_PCH_CMP_I219_V12:
    case E1000E_DEV_ID_PCH_TGP_I219_LM15:
    case E1000E_DEV_ID_PCH_TGP_I219_V15:
    case E1000E_DEV_ID_PCH_CMP_I219_LM10:
    case E1000E_DEV_ID_PCH_TGP_I219_LM14:
    case E1000E_DEV_ID_PCH_TGP_I219_V14:
    case E1000E_DEV_ID_PCH_TGP_I219_LM13:
    case E1000E_DEV_ID_PCH_TGP_I219_V13:
    case E1000E_DEV_ID_PCH_ADP_I219_LM17:
    case E1000E_DEV_ID_PCH_ADP_I219_V17:
    case E1000E_DEV_ID_PCH_ADP_I219_LM16:
    case E1000E_DEV_ID_PCH_ADP_I219_V16:
    case E1000E_DEV_ID_PCH_RPL_I219_LM19:
    case E1000E_DEV_ID_PCH_RPL_I219_V19:
    case E1000E_DEV_ID_PCH_RPL_I219_LM18:
    case E1000E_DEV_ID_PCH_RPL_I219_V18:
    case E1000E_DEV_ID_PCH_MTP_I219_LM20:
    case E1000E_DEV_ID_PCH_MTP_I219_V20:
    case E1000E_DEV_ID_PCH_MTP_I219_LM21:
    case E1000E_DEV_ID_PCH_MTP_I219_V21:
    case E1000E_DEV_ID_PCH_ARL_I219_LM22:
    case E1000E_DEV_ID_PCH_ARL_I219_V22:
      pDriver->mediaType = MEDIA_COPPER;
      break;

    case E1000E_DEV_ID_82571EB_FIBER:
    case E1000E_DEV_ID_82571EB_QUAD_FIBER:
    case E1000E_DEV_ID_82572EI_FIBER:
      pDriver->mediaType = MEDIA_FIBER;
      break;

    case E1000E_DEV_ID_82571EB_SERDES:
    case E1000E_DEV_ID_82571EB_SERDES_DUAL:
    case E1000E_DEV_ID_82571EB_SERDES_QUAD:
    case E1000E_DEV_ID_82572EI_SERDES:
    case E1000E_DEV_ID_80003ES2LAN_SERDES_DPT:
    case E1000E_DEV_ID_80003ES2LAN_SERDES:
      pDriver->mediaType = MEDIA_SERDES;
      break;

    default:
      retCode = ERR_NOT_SUPPORTED;
      break;
  }

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X probing result: %s",
               kDeviceId,
               (retCode == NO_ERROR) ? "Supported" : "Not Supported");
  if (retCode == NO_ERROR)
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X detected with media type %s",
                 kDeviceId,
                 (pDriver->mediaType == MEDIA_COPPER) ? "Copper" :
                 (pDriver->mediaType == MEDIA_FIBER) ? "Fiber" : "SerDes");
  }

  return retCode;
}

static void _LoadDefaultDevice(S_E1000EDriver* pDriver)
{
  E_Return retCode;
  uint16_t eepromValue;
  uint64_t macAddress;
  uint32_t registerVal;

  /* Initialize the device's settings */
  pDriver->rxBufferCount              = E1000E_RX_BUFFER_COUNT;
  pDriver->txBufferCount              = E1000E_TX_BUFFER_COUNT;
  pDriver->bufferSize                 = E1000E_RX_BUFFER_SIZE;
  pDriver->autoNegMask                = E1000E_AUTONEG_MASK;
  pDriver->linkSpeed                  = LINK_SPEED_AUTONEG;
  pDriver->duplexType                 = DUPLEX_AUTONEG;
  pDriver->intThrottle                = E1000E_INT_THROTTLE_INIT;
  pDriver->rxIntDelay                 = E1000E_RX_INT_DELAY_INIT;
  pDriver->txIntDelay                 = E1000E_TX_INT_DELAY_INIT;
  pDriver->rxAbsIntDelay              = E1000E_RX_ABS_INT_DELAY_INIT;
  pDriver->txAbsIntDelay              = E1000E_TX_ABS_INT_DELAY_INIT;
  pDriver->enableXSumRx               = E1000E_ENABLE_XSUM_RX_INIT;
  pDriver->useJumboFrames             = E1000E_USE_JUMBO_FRAMES_INIT;
  pDriver->loopbackMode               = LOOPBACK_NONE;
  pDriver->enableUnicastPromiscuous   = E1000E_ENABLE_UNICAST_PROMISCUOUS_INIT;
  pDriver->enableMulticastPromiscuous = E1000E_ENABLE_MCAST_PROMISCUOUS_INIT;
  pDriver->listenBroadcast            = E1000E_LISTEN_BROADCAST_INIT;
  pDriver->crcStripping               = E1000E_CRC_STRIPPING_INIT;
  pDriver->flowControl                = FLOW_CONTROL_RX_TX_PAUSE;

  /* Read the MAC address from registers */
  registerVal = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_RAL_0));
  macAddress = registerVal;
  registerVal = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_RAH_0));
  macAddress |= ((uint64_t)(registerVal & 0xFFFF) << 32);
  if (registerVal & E1000E_RAH_VALID)
  {
    pDriver->macAddress = macAddress;
  }
  else
  {
    pDriver->macAddress = 0;
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to read MAC address from"
                 " registers\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  /* Get the MDIO PHY address */
  pDriver->phyAddress = 1;
}

static E_Return _DetectDevice(S_PCIDevice* pDevice, S_E1000EDriver* pDriver)
{
  E_Return retCode;

  /* Get the BAR and map it */
  pDriver->barAddress = PCIGetBARAddress(pDevice->pFunction, 0);
  pDriver->barSize    = PCIGetBARSize(pDevice->pFunction, 0);

  if (pDriver->barAddress != 0 || pDriver->barSize != 0)
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X with BAR at 0x%p of size %llu",
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
      /* Validate the EEPROM, this is not critical on E1000E */
      retCode = _ValidateEEPROM(pDriver);
      if (retCode != NO_ERROR)
      {
        KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to validate EEPROM\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
      }
      /* Load the default configuration */
      _LoadDefaultDevice(pDriver);

      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000E device %04X:%04X:%04X MAC:"
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

static void _ResetDevice(S_E1000EDriver* pDriver)
{
  uint32_t regValue;

  /* Clear interrupts */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMC), 0x01FFFFFF);

  /* Disable RX and TX */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RCTL), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TCTL), E1000E_TCTL_PSP);

  /* Clear RX and TX descriptors */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDH0), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDT0), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDH0 + 0x100), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDT0 + 0x100), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDH), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDT), 0);

  /* Wait for the disable to take effect */
  SleepNs(E1000E_RESET_TIMEOUT_NS, NULL);

  /* Reset the controller */
  regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL));
  regValue |= E1000E_CTRL_RST;
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL), regValue);

  /* Wait for the reset to complete */
  SleepNs(E1000E_RESET_TIMEOUT_NS, NULL);

  /* Clear interrupts and pending events */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMC), 0x01FFFFFF);
  _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_ICR));
}

static E_Return _AllocateDeviceResources(S_E1000EDriver* pDriver)
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
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to allocate TX "
                   "resources\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to allocate RX "
                 "resources\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  return retCode;
}

static E_Return _AllocateDeviceRXResources(S_E1000EDriver* pDriver)
{
  E_Return retCode;
  E_Return internalRetCode;
  size_t   toAllocate;

  toAllocate = ALIGN_UP(pDriver->rxBufferCount *
                        (sizeof(S_RXBufferDescriptor) +
                         sizeof(S_E1000EBufferDescriptor)),
                        KERNEL_PAGE_SIZE);

  /* Allocate the RX buffer descriptor ring */
  pDriver->rxDescriptorIdx   = 0;
  pDriver->pRXFragmentList   = NULL;
  pDriver->pRXDescriptorRing = MemoryKernelAllocate(toAllocate,
                                                    MEMMGR_MAP_KERNEL |
                                                    MEMMGR_MAP_RW     |
                                                    MEMMGR_MAP_CACHE_DISABLED,
                                                    &retCode);

  if (retCode == NO_ERROR)
  {
    /* Allocate the RX buffer descriptor addresses */
    pDriver->pRXDescriptorAddresses = (void *)
      ((uintptr_t)pDriver->pRXDescriptorRing +
       (pDriver->rxBufferCount * sizeof(S_RXBufferDescriptor)));

    /* Allocate the RX descriptor ring size */
    pDriver->rxDescriptorMemorySize = toAllocate;
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X RX ring created",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);

    /* Allocate the device RX buffers */
    retCode = NetManagerCreateBufferPool(pDriver->bufferSize,
                                         pDriver->rxBufferCount,
                                         E1000E_RX_BUFFER_ALIGNMENT,
                                         &pDriver->networkDevice);
    if (retCode == NO_ERROR)
    {
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000E device %04X:%04X:%04X RX buffer pool created",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
    else
    {
      internalRetCode = MemoryKernelFree(pDriver->pRXDescriptorRing, toAllocate);
      E1000E_ASSERT(internalRetCode == NO_ERROR,
                   "Failed to free RX descriptor ring",
                   internalRetCode);
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to create RX pool\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to create RX ring\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  return retCode;
}

static E_Return _AllocateDeviceTXResources(S_E1000EDriver* pDriver)
{
  E_Return retCode;
  size_t   toAllocate;

  toAllocate = ALIGN_UP(pDriver->txBufferCount *
                        (sizeof(S_TXBufferDescriptor) +
                         sizeof(S_E1000ETXBufferMetadata)),
                        KERNEL_PAGE_SIZE);

  /* Allocate the TX buffer descriptor ring */
  pDriver->txDescriptorIdx      = 0;
  pDriver->txDescriptorCleanIdx = 0;
  pDriver->txDescriptorWatchIdx = 0;
  pDriver->txService            = 0;
  pDriver->txTicket             = 0;

  pDriver->pTXDescriptorRing = MemoryKernelAllocate(toAllocate,
                                                   MEMMGR_MAP_KERNEL |
                                                   MEMMGR_MAP_RW     |
                                                   MEMMGR_MAP_CACHE_DISABLED,
                                                   &retCode);
  if (retCode == NO_ERROR)
  {
    /* ALlocate the TX buffer descriptor addresses */
    pDriver->pTXDescriptorMeta = (void *)
      ((uintptr_t)pDriver->pTXDescriptorRing +
       (pDriver->txBufferCount * sizeof(S_TXBufferDescriptor)));
    pDriver->txDescriptorMemorySize = toAllocate;

    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X TX ring created",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to create TX ring\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
  return retCode;
}

static void _SetDeviceMAC(S_E1000EDriver* pDriver, const uint64_t kMACAddress)
{
  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X setting MAC address to "
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
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RAL_0),
               kMACAddress & 0xFFFFFFFF);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RAH_0),
               (kMACAddress >> 32) & 0x0000FFFF);
}

static void _ReleaseDeviceResources(S_E1000EDriver* pDriver)
{
  _ReleaseDeviceRXResources(pDriver);
  _ReleaseDeviceTXResources(pDriver);
}

static void _ReleaseDeviceRXResources(S_E1000EDriver* pDriver)
{
  E_Return retCode;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Releasing RX resources for E1000E device %04X:%04X:%04X",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  /* Free the RX buffer descriptor ring */
  retCode = MemoryKernelFree(pDriver->pRXDescriptorRing,
                             pDriver->rxDescriptorMemorySize);
  E1000E_ASSERT(retCode == NO_ERROR,
               "Failed to free RX descriptor ring",
               retCode);

  /* Release the device RX buffers */
  retCode = NetManagerReleaseBufferPool(&pDriver->networkDevice);
  E1000E_ASSERT(retCode == NO_ERROR,
               "Failed to release RX buffer pool",
               retCode);
}

static void _ReleaseDeviceTXResources(S_E1000EDriver* pDriver)
{
  E_Return retCode;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Releasing TX resources for E1000E device %04X:%04X:%04X",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  /* Free the TX buffer descriptor ring */
  retCode = MemoryKernelFree(pDriver->pTXDescriptorRing,
                             pDriver->txDescriptorMemorySize);
  E1000E_ASSERT(retCode == NO_ERROR,
               "Failed to free TX descriptor ring",
               retCode);
}

static void _InitializeControlFlow(S_E1000EDriver* pDriver)
{
  uint32_t regValue;
  uint32_t rxPBA;
  uint32_t lowWatermark;
  uint32_t highWatermark;

  if (pDriver->flowControl != FLOW_CONTROL_NONE)
  {
    /* Set flow mac address (as per IEEE 802.3x) */
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_FCAL), 0x00C28001);
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_FCAH), 0x00000100);

    /* Set the flow control timer (as per IEEE 802.3x) */
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_FCT), 0x00008808);
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_FCTTV), 0x0000FFFF);

    /* Set the low and high watermarks for the flow control */
    regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_PBA));
    rxPBA = (regValue & E1000E_PBA_RX_MASK) >> E1000E_PBA_RX_SHIFT;
    if (rxPBA == 0 || rxPBA == 0xFFFF)
    {
      rxPBA = E1000E_PBA_RX_DEFAULT;
    }
    rxPBA *= 1024;
    lowWatermark  = ((rxPBA * 50) / 100) & 0x0000FFF8;
    highWatermark = ((rxPBA * 80) / 100) & 0x0000FFF8;

    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_FCRTH), highWatermark);
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_FCRTL),
                 lowWatermark | E1000E_FCRTL_XONE);

    /* Set the flow control bits in the control register */
    regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL));
    if (pDriver->flowControl == FLOW_CONTROL_RX_PAUSE)
    {
      regValue |= E1000E_CTRL_RFCE;
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X flow control RX enabled",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    else if (pDriver->flowControl == FLOW_CONTROL_TX_PAUSE)
    {
      regValue |= E1000E_CTRL_TFCE;
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X flow control TX enabled",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    else if (pDriver->flowControl == FLOW_CONTROL_RX_TX_PAUSE)
    {
      regValue |= E1000E_CTRL_TFCE | E1000E_CTRL_RFCE;
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X flow control RX/TX enabled",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL), regValue);
  }
  else
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X flow control disabled",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
}

static void _InitializeDeviceRX(S_E1000EDriver* pDriver)
{
  S_RXBufferDescriptor*     pRXDescRing;
  S_E1000EBufferDescriptor* pRXDescAddresses;
  uintptr_t                 bufferAddress;
  uintptr_t                 physAddr;
  uint32_t                  configReg;
  size_t                    i;

  /* Initialize the RX descriptor ring */
  bufferAddress    = (uintptr_t)pDriver->networkDevice.pBufferPool;
  pRXDescRing      = pDriver->pRXDescriptorRing;
  pRXDescAddresses = pDriver->pRXDescriptorAddresses;
  for (i = 0; i < pDriver->rxBufferCount; ++i)
  {
    pRXDescAddresses[i].virtAddr = (void *)(bufferAddress +
                                            i * pDriver->bufferSize);
    physAddr = (uintptr_t)MemoryMgrGetPhysAddr(
        (uintptr_t)pRXDescAddresses[i].virtAddr,
        NULL,
        NULL);
    pRXDescAddresses[i].physAddr = physAddr;
    pRXDescRing[i].bufferAddress = physAddr;
    pRXDescRing[i].length = pDriver->bufferSize;
  }

  /* Initialize the RX descriptor ring status */
  physAddr = (uintptr_t)MemoryMgrGetPhysAddr((uintptr_t)pRXDescRing,
                                             NULL,
                                             NULL);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDBAL0),
               physAddr & 0xFFFFFFFF);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDBAH0),
               (physAddr >> 32) & 0xFFFFFFFF);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDLEN0),
               pDriver->rxBufferCount * sizeof(S_RXBufferDescriptor));
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDH0), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDT0),
               pDriver->rxBufferCount - 1);

  /* Initialize the RX descriptor control register */
  regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_RXDCTL));
  regValue &= ~(E1000E_RXDCTL_PTHRESH_MASK |
                E1000E_RXDCTL_HTHRESH_MASK |
                E1000E_RXDCTL_WTHRESH_MASK);
  regValue |= E1000E_RXDCTL_PTHRESH_DEFAULT |
              E1000E_RXDCTL_HTHRESH_DEFAULT |
              E1000E_RXDCTL_WTHRESH_DEFAULT |
              E1000E_RXDCTL_GRAN;
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RXDCTL), regValue);

  configReg = E1000E_RCTL_EN | E1000E_RCTL_RDMTS_8 | E1000E_RCTL_MO_36;
  if (pDriver->enableUnicastPromiscuous == true)
  {
    configReg |= E1000E_RCTL_UPE;
  }
  if (pDriver->enableMulticastPromiscuous == true)
  {
    configReg |= E1000E_RCTL_MPE;
  }
  if (pDriver->useJumboFrames == true)
  {
    configReg |= E1000E_RCTL_LPE;
  }
  if (pDriver->loopbackMode != LOOPBACK_NONE)
  {
    configReg |= E1000E_RCTL_LBM_PHY;
  }
  else
  {
    configReg |= E1000E_RCTL_LBM_NONE;
  }
  if (pDriver->listenBroadcast == true)
  {
    configReg |= E1000E_RCTL_BAM;
  }

  if (pDriver->bufferSize == 2048)
  {
    configReg |= E1000E_RCTL_BSIZE_2048;
  }
  else if (pDriver->bufferSize == 1024)
  {
    configReg |= E1000E_RCTL_BSIZE_1024;
  }
  else if (pDriver->bufferSize == 512)
  {
    configReg |= E1000E_RCTL_BSIZE_512;
  }
  else if (pDriver->bufferSize == 256)
  {
    configReg |= E1000E_RCTL_BSIZE_256;
  }
  else if (pDriver->bufferSize == 4096)
  {
    configReg |= E1000E_RCTL_BSIZE_4096 | E1000E_RCTL_BSEX;
  }
  else if (pDriver->bufferSize == 8192)
  {
    configReg |= E1000E_RCTL_BSIZE_8192 | E1000E_RCTL_BSEX;
  }
  else if (pDriver->bufferSize == 16384)
  {
    configReg |= E1000E_RCTL_BSIZE_16384 | E1000E_RCTL_BSEX;
  }
  else
  {
    configReg |= E1000E_RCTL_BSIZE_2048;
    KERNEL_ERROR("E1000E device %04X:%04X:%04X invalid RX buffer size %u, "
                 "using default size\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function,
                 pDriver->bufferSize);
  }

  /* Write configuration to the device */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RCTL), configReg);

  /* Initialize the interrupt throttle */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDTR),
                pDriver->rxIntDelay & 0x0000FFFF);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RADV),
                pDriver->rxAbsIntDelay & 0x0000FFFF);

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X RX initialized",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static void _InitializeDeviceTX(S_E1000EDriver* pDriver)
{
  uintptr_t physAddr;
  uint32_t  regValue;

  /* Initialize the TX descriptor ring status */
  physAddr = (uintptr_t)MemoryMgrGetPhysAddr(
      (uintptr_t)pDriver->pTXDescriptorRing,
      NULL,
      NULL);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDBAL),
               physAddr & 0xFFFFFFFF);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDBAH),
               (physAddr >> 32) & 0xFFFFFFFF);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDLEN),
               pDriver->txBufferCount * sizeof(S_TXBufferDescriptor));
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDH), 0);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDT), 0);

  regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_TXDCTL));
  regValue &= ~(E1000E_TXDCTL_PTHRESH_MASK |
                E1000E_TXDCTL_HTHRESH_MASK |
                E1000E_TXDCTL_WTHRESH_MASK);
  regValue |= E1000E_TXDCTL_PTHRESH_DEFAULT |
              E1000E_TXDCTL_HTHRESH_DEFAULT |
              E1000E_TXDCTL_WTHRESH_DEFAULT |
              E1000E_TXDCTL_GRAN;
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TXDCTL), regValue);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TCTL),
               E1000E_TCTL_EN |
               E1000E_TCTL_PSP |
               E1000E_TCTL_CT_ETHER |
               E1000E_TCTL_COLD_FULL |
               E1000E_TCTL_RTLC);

  /* Initialize the interrupt throttle */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TIDV),
                pDriver->txIntDelay & 0x0000FFFF);
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TADV),
                pDriver->txAbsIntDelay & 0x0000FFFF);

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X TX initialized",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static E_Return _SetupDeviceInterrupts(S_E1000EDriver* pDriver)
{
  E_Return     err;
  S_PCIDevice* pDevice;
  uint32_t     ctrlExt;

  pDevice = pDriver->pPCIDevice;

  if (pDriver->interruptLine <= 0)
  {
    pDriver->interruptLine = AllocateInterruptLine();
    pDriver->handlerCore = GetBalancedInterruptHandlerCore();
  }

  if (pDriver->interruptLine > 0)
  {
    /* Enable MSI for the device */
    err = PrepareDeviceMSIInterrupt(pDevice);
    if (err == NO_ERROR)
    {
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000E device %04X:%04X:%04X using MSI-X interrupts",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      pDriver->useMSIXInterrupts = true;

      /* Setup MSIX */
      err = PCISetupInterruptMSIX(pDevice,
                                  pDriver->interruptLine,
                                  0,
                                  pDriver->handlerCore,
                                  true);
      if (err != NO_ERROR)
      {
        KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000E device %04X:%04X:%04X using MSI interrupts",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
        pDriver->useMSIXInterrupts = false;

        /* We do not have the MSI-X capability, fall back to MSI interrupts */
        err = PCISetupInterruptMSI(pDevice,
                                   pDriver->interruptLine,
                                   pDriver->handlerCore,
                                   true);
        if (err != NO_ERROR)
        {
          KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to setup MSI-X\n",
                      pDriver->pPCIDevice->bus,
                      pDriver->pPCIDevice->slot,
                      pDriver->pPCIDevice->function);
          }
      }
    }

    if (err == NO_ERROR)
    {
      /* Register the interrupt handler */
      err = InterruptRegister(pDriver->interruptLine, _E1000EInterruptHandler);
      if (err == NO_ERROR)
      {
        /* Mask the interrupt in the device */
        _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMC), 0x01FFFFFF);

        /* Set interrupt auto mask */
        ctrlExt = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT));
        ctrlExt |= E1000E_CTRL_EXT_IAME;
        _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT), ctrlExt);
        _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IAM),
                     E1000E_ENABLED_INTERRUPTS);

        /* Setup the interrupt throttle */
        _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_ITR),
                     pDriver->intThrottle);

        InterruptSetEOI(pDriver->interruptLine);

        KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "E1000E device %04X:%04X:%04X interrupt handler registered",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
      }
      else
      {
        if (pDriver->useMSIXInterrupts == true)
        {
          PCISetupInterruptMSIX(pDevice, 0, 0, 0, false);
        }
        else
        {
          PCISetupInterruptMSI(pDevice, 0, 0, false);
        }
        FreeInterruptLine(pDriver->interruptLine);
        pDriver->interruptLine = 0;
        KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to register handler\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function);
      }
    }
    else
    {
      FreeInterruptLine(pDriver->interruptLine);
      pDriver->interruptLine = 0;
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to setup interrupts\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to allocate interrupt line\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    err = ERR_EXCEEDED_LIMIT;
  }

  return err;
}

static void _SetPhyForced(S_E1000EDriver* pDriver)
{
  uint32_t regValue;
  uint16_t phyValue;
  E_Return retCode;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000E device %04X:%04X:%04X auto-negotiation disabled",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function);

  /* Setup the MAC forced speed */
  regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL));
  regValue |= E1000E_CTRL_FRCDPLX | E1000E_CTRL_FRCSPEED;
  regValue &= ~E1000E_CTRL_ASDE;

  if (pDriver->duplexType == DUPLEX_FUL)
  {
    phyValue = E1000E_PHY_CTRL_FD;
    regValue |= E1000E_CTRL_FD;
  }
  else
  {
    phyValue = 0;
    regValue &= ~E1000E_CTRL_FD;
  }

  regValue &= ~E1000E_CTRL_SPD_SEL;
  if (pDriver->linkSpeed == LINK_SPEED_10MBPS)
  {
    phyValue = E1000E_PHY_CTRL_SPEED_10;
    regValue |= E1000E_CTRL_SPD_SEL_10;
  }
  else if (pDriver->linkSpeed == LINK_SPEED_100MBPS)
  {
    phyValue |= E1000E_PHY_CTRL_SPEED_100;
    regValue |= E1000E_CTRL_SPD_SEL_100;
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X invalid link speed %u, "
                 "using default speed\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function,
                 pDriver->linkSpeed);
  }
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL), regValue);

  /* Set the PHY */
  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000E_PHY_CTRL, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue &= ~(E1000E_PHY_CTLR_SPEED_MASK |
                  E1000E_PHY_CTRL_DUPLEX_MASK |
                  E1000E_PHY_CTRL_AN_ENABLE);
    regValue |= phyValue | E1000E_PHY_CTRL_RESET;
    retCode = _WriteMDIO(pDriver, E1000E_PHY_CTRL, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
    else
    {
      /* Wait for the PHY to be ready */
      do
      {
        retCode = _ReadMDIO(pDriver, E1000E_PHY_CTRL, (uint16_t*)&regValue);
      } while (retCode == NO_ERROR && (regValue & E1000E_PHY_CTRL_RESET) != 0);
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000E device %04X:%04X:%04X PHY set to speed %u, duplex "
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
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }
}

static void _SetPhyAutoNeg(S_E1000EDriver* pDriver)
{
  uint32_t regValue;
  E_Return retCode;

  if (pDriver->autoNegMask != E1000E_AUTONEG_MASK_OFF &&
      (pDriver->linkSpeed != LINK_SPEED_AUTONEG ||
        pDriver->duplexType != DUPLEX_AUTONEG))
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X auto-negotiation settings"
                  " conflict, using auto-negotiation\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }

  /* Setup the MAC auto-negotiation speed */
  regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL));
  regValue &= ~(E1000E_CTRL_FRCSPEED | E1000E_CTRL_FRCDPLX | E1000E_CTRL_FD);
  regValue |= E1000E_CTRL_ASDE;
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL), regValue);

  /* Set the PHY */
  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000E_PHY_AUTONEG_ADV, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue &= ~(E1000E_PHY_ANA_1000BASE_X_FULL |
                  E1000E_PHY_ANA_1000BASE_X_HALF |
                  E1000E_PHY_ANA_100BASE_TX_FULL |
                  E1000E_PHY_ANA_100BASE_TX_HALF |
                  E1000E_PHY_ANA_10BASE_T_FULL |
                  E1000E_PHY_ANA_10BASE_T_HALF);

    if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_1000_FULL)
    {
      regValue |= E1000E_PHY_ANA_1000BASE_X_FULL;
    }
    if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_1000_HALF)
    {
      regValue |= E1000E_PHY_ANA_1000BASE_X_HALF`;
    }
    if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_100_FULL)
    {
      regValue |= E1000E_PHY_ANA_100BASE_TX_FULL;
    }
    if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_100_HALF)
    {
      regValue |= E1000E_PHY_ANA_100BASE_TX_HALF;
    }
    if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_10_FULL)
    {
      regValue |= E1000E_PHY_ANA_10BASE_T_FULL;
    }
    if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_10_HALF)
    {
      regValue |= E1000E_PHY_ANA_10BASE_T_HALF;
    }

    retCode = _WriteMDIO(pDriver, E1000E_PHY_AUTONEG_ADV, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }

  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000E_PHY_1000T_CTRL, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue &= ~(E1000E_PHY_1000T_1000BASE_T_FULL |
                  E1000E_PHY_1000T_1000BASE_T_HALF);

    if (pDriver->deviceId == E1000E_DEV_ID_82544EI_COPPER ||
        pDriver->deviceId == E1000E_DEV_ID_82544GC_COPPER ||
        pDriver->deviceId == E1000E_DEV_ID_82544GC_LOM)
    {
      regValue |= E1000E_PHY_1000T_1000BASE_T_FULL |
                  E1000E_PHY_1000T_1000BASE_T_HALF;
    }
    else
    {
      if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_1000_FULL)
      {
        regValue |= E1000E_PHY_1000T_1000BASE_T_FULL;
      }
      if (pDriver->autoNegMask & E1000E_AUTONEG_MASK_1000_HALF)
      {
        regValue |= E1000E_PHY_1000T_1000BASE_T_HALF;
      }
    }
    retCode = _WriteMDIO(pDriver, E1000E_PHY_1000T_CTRL, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }

  regValue = 0;
  retCode  = _ReadMDIO(pDriver, E1000E_PHY_CTRL, (uint16_t*)&regValue);
  if (retCode == NO_ERROR)
  {
    regValue |= E1000E_PHY_CTRL_AN_ENABLE | E1000E_PHY_CTRL_AN_RESTART;
    retCode = _WriteMDIO(pDriver, E1000E_PHY_CTRL, regValue);
    if (retCode != NO_ERROR)
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to set PHY\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
    else
    {
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000E device %04X:%04X:%04X PHY set to auto-negotiation",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to read PHY\n",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
  }
}

static void _SetPhyNonCopper(S_E1000EDriver* pDriver)
{
  uint32_t regValue;

  /* Setup the MAC for non-copper media */
  regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL));
  regValue &= ~(E1000E_CTRL_FRCSPEED | E1000E_CTRL_FRCDPLX | E1000E_CTRL_SPD_SEL);
  regValue |= E1000E_CTRL_ASDE | E1000E_CTRL_FD;
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL), regValue);

  /* Set the transmit word */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TXCW), 0);

  regValue = E1000E_TXCW_FD | E1000E_TXCW_ANE | E1000E_TXCW_TXC;

  /* Advertise flow control capabilities based on flow control settings */
  if (pDriver->flowControl == FLOW_CONTROL_RX_PAUSE ||
      pDriver->flowControl == FLOW_CONTROL_RX_TX_PAUSE)
  {
    regValue |= E1000E_TXCW_PAUSE;
  }
  if (pDriver->flowControl == FLOW_CONTROL_TX_PAUSE ||
      pDriver->flowControl == FLOW_CONTROL_RX_TX_PAUSE)
  {
    regValue |= E1000E_TXCW_ASYM_PD;
  }

  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TXCW), regValue);

  /* For SerDes, we must inform the MAC to bypass optical */
  regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT));
  regValue &= ~E1000E_CTRL_EXT_LINK_MODE_MASK;
  regValue |= E1000E_CTRL_EXT_LINK_MODE_DIRECT;
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT), regValue);

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000E device %04X:%04X:%04X PHY set to non-copper media",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function);
}

static void _InitializePhy(S_E1000EDriver* pDriver)
{
  uint32_t ipcnfg;

  if (pDriver->mediaType == MEDIA_COPPER)
  {
    if (pDriver->autoNegMask == E1000E_AUTONEG_MASK_OFF &&
        pDriver->linkSpeed != LINK_SPEED_AUTONEG &&
        pDriver->duplexType != DUPLEX_AUTONEG &&
        pDriver->linkSpeed != LINK_SPEED_1GBPS)
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

  /* Only applies to newer E1000E PHYs (82579, I217, I218, I219).
   * Using standard Clause 45 over Clause 22 MMD access registers (Reg 13 & 14):
   * Device 7 (Auto-Neg), Register 0x003C (EEE Advertisement) -> Write 0x0000
   */
  switch (pDriver->deviceId)
  {
    case E1000E_DEV_ID_PCH2_LV_LM:
    case E1000E_DEV_ID_PCH2_LV_V:
    case E1000E_DEV_ID_PCH_LPT_I217_LM:
    case E1000E_DEV_ID_PCH_LPT_I217_V:
    case E1000E_DEV_ID_PCH_LPTLP_I218_V:
    case E1000E_DEV_ID_PCH_LPTLP_I218_LM:
    case E1000E_DEV_ID_PCH_I218_LM2:
    case E1000E_DEV_ID_PCH_I218_V2:
    case E1000E_DEV_ID_PCH_I218_LM3:
    case E1000E_DEV_ID_PCH_I218_V3:
    case E1000E_DEV_ID_PCH_SPT_I219_LM:
    case E1000E_DEV_ID_PCH_SPT_I219_V:
    case E1000E_DEV_ID_PCH_SPT_I219_LM2:
    case E1000E_DEV_ID_PCH_SPT_I219_V2:
    case E1000E_DEV_ID_PCH_LBG_I219_LM3:
    case E1000E_DEV_ID_PCH_SPT_I219_V5:
    case E1000E_DEV_ID_PCH_SPT_I219_LM4:
    case E1000E_DEV_ID_PCH_SPT_I219_V4:
    case E1000E_DEV_ID_PCH_SPT_I219_LM5:
    case E1000E_DEV_ID_PCH_CNP_I219_LM7:
    case E1000E_DEV_ID_PCH_CNP_I219_V7:
    case E1000E_DEV_ID_PCH_CNP_I219_LM6:
    case E1000E_DEV_ID_PCH_CNP_I219_V6:
    case E1000E_DEV_ID_PCH_ICP_I219_LM8:
    case E1000E_DEV_ID_PCH_ICP_I219_V8:
    case E1000E_DEV_ID_PCH_ICP_I219_LM9:
    case E1000E_DEV_ID_PCH_ICP_I219_V9:
    case E1000E_DEV_ID_PCH_CMP_I219_LM11:
    case E1000E_DEV_ID_PCH_CMP_I219_V11:
    case E1000E_DEV_ID_PCH_CMP_I219_LM10:
    case E1000E_DEV_ID_PCH_CMP_I219_V10:
    case E1000E_DEV_ID_PCH_CMP_I219_LM12:
    case E1000E_DEV_ID_PCH_CMP_I219_V12:
    case E1000E_DEV_ID_PCH_TGP_I219_LM15:
    case E1000E_DEV_ID_PCH_TGP_I219_V15:
    case E1000E_DEV_ID_PCH_CMP_I219_LM10:
    case E1000E_DEV_ID_PCH_TGP_I219_LM14:
    case E1000E_DEV_ID_PCH_TGP_I219_V14:
    case E1000E_DEV_ID_PCH_TGP_I219_LM13:
    case E1000E_DEV_ID_PCH_TGP_I219_V13:
    case E1000E_DEV_ID_PCH_ADP_I219_LM17:
    case E1000E_DEV_ID_PCH_ADP_I219_V17:
    case E1000E_DEV_ID_PCH_ADP_I219_LM16:
    case E1000E_DEV_ID_PCH_ADP_I219_V16:
    case E1000E_DEV_ID_PCH_RPL_I219_LM19:
    case E1000E_DEV_ID_PCH_RPL_I219_V19:
    case E1000E_DEV_ID_PCH_RPL_I219_LM18:
    case E1000E_DEV_ID_PCH_RPL_I219_V18:
    case E1000E_DEV_ID_PCH_MTP_I219_LM20:
    case E1000E_DEV_ID_PCH_MTP_I219_V20:
    case E1000E_DEV_ID_PCH_MTP_I219_LM21:
    case E1000E_DEV_ID_PCH_MTP_I219_V21:
    case E1000E_DEV_ID_PCH_ARL_I219_LM22:
    case E1000E_DEV_ID_PCH_ARL_I219_V22:
      /* Disable EEE */
      ipcnfg = _MMIORead32(E1000_REG(pDriver->barAddress, E1000E_IPCNFG));
      ipcnfg &= ~(E1000E_IPCNFG_EEE_100M_AN | E1000E_IPCNFG_EEE_1G_AN);
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_IPCNFG), ipcnfg);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_EEER), 0);

      /* Disable EEE advertisement on Phy */
      if (_WriteMDIO(pDriver, E1000_PHY_MMD_CTRL, 0x0007) == NO_ERROR)
      {
        _WriteMDIO(pDriver, E1000_PHY_MMD_DATA, 0x003C);
        _WriteMDIO(pDriver, E1000_PHY_MMD_CTRL, 0x4007);
        _WriteMDIO(pDriver, E1000_PHY_MMD_DATA, 0x0000);
      }
      break;
    default:
      break;
  }
  /* Wait for the PHY to complete auto-negotiation */
  SleepNs(E1000E_PHY_INIT_TIMEOUT_NS, NULL);
}

static void _ConfigurePowerManagement(S_E1000EDriver* pDriver,
                                      const bool     kEnableActiveMode)
{
  uint32_t     regValue;
  uint8_t      pcieCapOffset;
  uint16_t     regCtrl;
  S_PCIDevice* pDevice;

  if (kEnableActiveMode == true)
  {
    /* Clear any pending Wakeup Status flags (WUS) and disable WoL */
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_WUFC), 0);
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_WUS), 0xFFFFFFFF);
    regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000E_WUC));
    regValue &= ~(E1000E_WUC_PME_EN | E1000E_WUC_APME);
    regValue |= E1000E_WUC_PME_STATUS;
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_WUC), regValue);

    /* Prevent premature L1 ASPM transitions during RX bursts */
    regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000E_GCR));
    regValue |= E1000E_GCR_L1_ACT_WITHOUT_L0S_RX;
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_GCR), regValue);

    /* Enable Dynamic Clock Gating while keeping Relaxed Ordering intact */
    regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000E_CTRL_EXT));
    regValue |= E1000E_CTRL_EXT_DMA_DYN_CLK;
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_CTRL_EXT), regValue);

    pDevice = pDriver->pPCIDevice;

    /* Force PCI Power State to D0 (Full Power) */
    pcieCapOffset = PCIFindCapability(pDevice, PCI_CAPABILITY_ID_POWERMGNT);
    if (pcieCapOffset != 0)
    {
      regCtrl = PCIReadConfig16(pDevice,
                                pcieCapOffset + PCI_PM_CTRL_STATUS_OFFSET);
      if ((regCtrl & PCI_PM_CTRL_STATE_MASK) != PCI_PM_CTRL_STATE_D0)
      {
        regCtrl &= ~PCI_PM_CTRL_STATE_MASK;
        regCtrl |= PCI_PM_CTRL_STATE_D0;
        PCIWriteConfig16(pDevice,
                         pcieCapOffset + PCI_PM_CTRL_STATUS_OFFSET, regCtrl);

        /* Wait 10ms for D3 -> D0 transition */
        SleepNs(10000000ULL, NULL);
      }
    }

    /* Disable L0s and L1 ASPM in the PCIe Link Control Register */
    pcieCapOffset = PCIFindCapability(pDevice, PCI_CAPABILITY_ID_PCIE);
    if (pcieCapOffset != 0)
    {
      regCtrl = PCIReadConfig16(pDevice,
                                pcieCapOffset + PCI_PCIE_LINK_CTRL_OFFSET);
      regCtrl &= ~PCI_PCIE_LCTL_ASPM_MASK;
      PCIWriteConfig16(pDevice,
                       pcieCapOffset + PCI_PCIE_LINK_CTRL_OFFSET,
                       regCtrl);
    }

    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X active power state configured",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
  else
  {
    /* Prepare device for low-power idle / downlink state */
    regValue = _MMIORead32(E1000_REG(pDriver->barAddress, E1000E_CTRL_EXT));
    regValue &= ~E1000E_CTRL_EXT_DMA_DYN_CLK;
    _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_CTRL_EXT), regValue);

    KERNEL_DEBUG(E1000_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X power management set to idle",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
}

static void _SetupMulticastTable(S_E1000EDriver* pDriver)
{
  size_t i;

  /* Clear the multicast table */
  for (i = 0; i < E1000E_MULTICAST_TABLE_SIZE; ++i)
  {
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_MTA + i * 4), 0);
  }

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X multicast table initialized",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static void _SetupVLAN(S_E1000EDriver* pDriver)
{
  size_t i;

  /* Disable VLAN filtering  */
  for (i = 0; i < E1000E_VLAN_FILTER_TABLE_SIZE; ++i)
  {
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_VFTA + (i * 4)), 0);
  }

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X VLAN filtering disabled",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);
}

static E_Return _DevUplink(S_NetworkDevice* pNetworkDevice)
{
  E_Return       retCode;
  E_Return       internalErr;
  S_E1000EDriver* pDriver;

  pDriver = (S_E1000EDriver *)pNetworkDevice->pDriverData;

  internalErr = KernelMutexLock(&pDriver->configLock);
  E1000E_ASSERT(internalErr == NO_ERROR,
               "E1000E device failed to lock config mutex\n",
               internalErr);

  retCode = _DevUplinkSafe(pDriver);

  internalErr = KernelMutexUnlock(&pDriver->configLock);
  E1000E_ASSERT(internalErr == NO_ERROR,
               "E1000E device failed to unlock config mutex\n",
               internalErr);

  return retCode;
}

static E_Return _DevDownlink(S_NetworkDevice* pNetworkDevice)
{
  E_Return       retCode;
  E_Return       internalErr;
  S_E1000EDriver* pDriver;

  pDriver = (S_E1000EDriver *)pNetworkDevice->pDriverData;

  internalErr = KernelMutexLock(&pDriver->configLock);
  E1000E_ASSERT(internalErr == NO_ERROR,
               "E1000E device failed to lock config mutex\n",
               internalErr);

  retCode = _DevDownlinkSafe(pDriver);

  internalErr = KernelMutexUnlock(&pDriver->configLock);
  E1000E_ASSERT(internalErr == NO_ERROR,
               "E1000E device failed to unlock config mutex\n",
               internalErr);

  return retCode;
}

static E_Return _DevUplinkSafe(S_E1000EDriver* pDriver)
{
  E_Return retCode;
  uint32_t regValue;

  if (pDriver->isInitialized == false)
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X device uplink requested",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);

    retCode = _AllocateDeviceResources(pDriver);
    if (retCode == NO_ERROR)
    {
      /* Reset the controller */
      _ResetDevice(pDriver);

      /* Notify the devices that the driver is loaded */
      regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT));
      regValue |= E1000E_CTRL_EXT_DRV_LOAD;
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT), regValue);

      /* Setup power management */
      _ConfigurePowerManagement(pDriver, true);

      /* Initialize the device's RXChecksum settings */
      regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_RXCSUM));
      regValue &= E1000E_RXCSUM_CLEAR_MASK;
      if (pDriver->enableXSumRx == true)
      {
        regValue |= E1000E_RXCSUM_IPOFL |
                    E1000E_RXCSUM_TUOFL |
                    E1000E_RXCSUM_IPV6OFL |
                    0x10;
        _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RXCSUM), regValue);
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

      /* Initialize the device's PHY */
      regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL));
      regValue |= E1000E_CTRL_SLU | E1000E_CTRL_ASDE;
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL), regValue);

      _InitializePhy(pDriver);

      /* Start the periodic stats task */
      pDriver->statTaskHandle = NetManagerAddPeriodicTask(_UpdateStatistics,
                                                pDriver,
                                                E1000E_STAT_UPDATE_INTERVAL_NS,
                                                pDriver->handlerCore);

      /* Enable all interrupts */
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMS),
                   E1000E_ENABLED_INTERRUPTS);

      pDriver->isInitialized = true;

      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "E1000E device %04X:%04X:%04X device uplink completed",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
    else
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to allocate resources\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X device already initialized",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = NO_ERROR;
  }

  return retCode;
}

static E_Return _DevDownlinkSafe(S_E1000EDriver* pDriver)
{
  E_Return retCode;
  uint32_t regValue;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X device downlink requested",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  if (pDriver->isInitialized == true)
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X device downlink in progress",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);

    pDriver->isInitialized = false;

    NetManagerRemovePeriodicTask(pDriver->statTaskHandle);

    /* Disable all interrupts */
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMC), 0x01FFFFFF);

    /* Wait until all pending transactions are completed */
    retCode = _WaitForTransactionCompletion(pDriver);
    if (retCode == NO_ERROR)
    {
      /* Release the device's resources */
      _ReleaseDeviceResources(pDriver);

      /* Shut down the device */
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMC), 0x01FFFFFF);

      /* Disable RX and TX */
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RCTL), 0);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TCTL),
                   E1000E_TCTL_PSP);

      /* Clear RX and TX descriptors */
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDH0), 0);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDT0), 0);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDH0 + 0x100), 0);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDT0 + 0x100), 0);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDH), 0);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_TDT), 0);

      /* Wait for the disable to take effect */
      SleepNs(E1000E_RESET_TIMEOUT_NS, NULL);

      /* Setup power management */
      _ConfigurePowerManagement(pDriver, false);

      /* Disable driver */
      regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT));
      regValue &= ~E1000E_CTRL_EXT_DRV_LOAD;
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL_EXT), regValue);

      regValue = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_CTRL));
      regValue &= ~(E1000E_CTRL_ASDE | E1000E_CTRL_SLU);
      _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_CTRL), regValue);

      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X device downlink completed",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);
    }
    else
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to wait for transaction"
                   " completion\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
    }
  }
  else
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X device already downlink",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = NO_ERROR;
  }

  return retCode;
}

static E_Return _WaitForTransactionCompletion(S_E1000EDriver* pDriver)
{
  E_Return retCode;
  size_t   waitCount;
  bool     isNotComplete;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X waiting for pending transactions to"
               " complete",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  SleepNs(E1000E_TRANSACTION_WAIT_NS, NULL);

  waitCount = 0;
  while ((pDriver->rxOperationRunning == true ||
          pDriver->txOperationRunning > 0 ||
          pDriver->txCompleteOperationRunning == true ||
          pDriver->linkOperationRunning == true ||
          pDriver->ioctlOperationRunning > 0) &&
          waitCount < E1000E_TRANSACTION_WAIT_COUNT)
  {
    SleepNs(E1000E_TRANSACTION_WAIT_NS, NULL);
    ++waitCount;
  }

  if (waitCount < E1000E_TRANSACTION_WAIT_COUNT)
  {
    do
    {
      SleepNs(E1000E_TRANSACTION_WAIT_NS, NULL);
      /* Clear the pending transmit */
      _CompleteTransmit(pDriver, &isNotComplete);
    } while (isNotComplete == true && waitCount < E1000E_TRANSACTION_WAIT_COUNT);

    if (isNotComplete == true)
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to complete pending "
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
    KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to complete pending "
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
  S_E1000EDriver* pDriver;
  ssize_t         sentBytes;
  uintptr_t       physAddr;
  uint32_t        tailIdx;
  uint16_t        packetSize;
  uint8_t         regValue;
  uint32_t        requiredPackets;
  uint32_t        availableDescriptors;
  uint32_t        transmitTicket;

  pDriver = (S_E1000EDriver *)pDevice->pDriverData;

  AtomicIncrement32(&pDriver->txOperationRunning);

  if (pDriver->isInitialized == true)
  {
    SpinlockAcquire(&pDriver->txRingLock);
    sentBytes = 0;
    tailIdx = pDriver->txDescriptorIdx;
    if (tailIdx >= pDriver->txDescriptorCleanIdx)
    {
      availableDescriptors = (pDriver->txBufferCount - 1) -
                             (tailIdx - pDriver->txDescriptorCleanIdx);
    }
    else
    {
      availableDescriptors = pDriver->txDescriptorCleanIdx - tailIdx - 1;
    }
    requiredPackets = (kSize + E1000_TX_BUFFER_SIZE - 1) / E1000_TX_BUFFER_SIZE;

    if (requiredPackets <= availableDescriptors)
    {
      /* Update the transmit descriptor tail */
      pDriver->txDescriptorIdx = (tailIdx + requiredPackets) %
                                 pDriver->txBufferCount;
      /* Manage to indicate clean on last packet */
      if (kIsLastOfBurst == true)
      {
        if (pDriver->txDescriptorIdx == 0)
        {
          pDriver->txDescriptorWatchIdx = pDriver->txBufferCount - 1;
        }
        else
        {
          pDriver->txDescriptorWatchIdx = pDriver->txDescriptorIdx - 1;
        }
      }

      /* Get the transmit ticket for this burst */
      transmitTicket = pDriver->txTicket++;

      SpinlockRelease(&pDriver->txRingLock);

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
        pDriver->pTXDescriptorRing[tailIdx].bufferAddress = physAddr;

        /* Save the metadata for the last packet only */
        sentBytes += packetSize;
        regValue = E1000E_TX_CMD_IDE;
        if ((size_t)sentBytes == kSize)
        {
          regValue |= E1000E_TX_CMD_EOP | E1000E_TX_CMD_IFCS;

          pDriver->pTXDescriptorMeta[tailIdx].virtAddr = (uintptr_t)kpBuffer;
          pDriver->pTXDescriptorMeta[tailIdx].length = kSize;
        }
        else
        {
          pDriver->pTXDescriptorMeta[tailIdx].virtAddr = (uintptr_t)NULL;
          pDriver->pTXDescriptorMeta[tailIdx].length = 0;
        }

        /* Manage RS to indicate clean on last packet */
        if (kIsLastOfBurst == true && (size_t)sentBytes == kSize)
        {
          regValue |= E1000E_TX_CMD_RS;
        }

        pDriver->pTXDescriptorRing[tailIdx].length = packetSize;
        pDriver->pTXDescriptorRing[tailIdx].cmd    = regValue;
        pDriver->pTXDescriptorRing[tailIdx].status = 0;

        /* Update the transmit descriptor tail */
        tailIdx = (tailIdx + 1) % pDriver->txBufferCount;
      }

      /* Wait for the transmit service to complete */
      while (pDriver->txService != transmitTicket)
      {
        CPUPause();
      }

      /* Update the transmit descriptor tail */
      _MMIOWrite32(E1000_REG(pDriver->barAddress, E1000E_TDT), tailIdx);

      /* Increment the transmit service */
      ++pDriver->txService;
    }
    else
    {
      if (kIsBlocking == false)
      {
        ++pDriver->stats.txDropped;
      }

      SpinlockRelease(&pDriver->txRingLock);

      sentBytes = -EBUSY;
    }
  }
  else
  {
    sentBytes = -ENETDOWN;
  }

  AtomicDecrement32(&pDriver->txOperationRunning);

  return sentBytes;
}

static ssize_t _IOControl(struct S_NetworkDevice* pDevice,
                          const uint32_t          kCommand,
                          void*                   pArg)
{
  S_E1000EDriver* pDriver;
  ssize_t        retCode;

  pDriver = (S_E1000EDriver *)pDevice->pDriverData;

  if (pDriver->isInitialized == true)
  {
    AtomicIncrement32(&pDriver->ioctlOperationRunning);

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
        KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl command 0x%08X not "
                     "supported\n",
                     pDriver->pPCIDevice->bus,
                     pDriver->pPCIDevice->slot,
                     pDriver->pPCIDevice->function,
                     kCommand);
        retCode = -EINVAL;
        break;
    }

    AtomicDecrement32(&pDriver->ioctlOperationRunning);
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl requested on uninitialized"
                 " device\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -ENODEV;
  }

  return retCode;
}

static bool _E1000EInterruptHandler(void)
{
  S_KernelThread* pCurrentThread;
  uint32_t        intId;
  uint32_t        status;
  S_E1000EDriver*  pDriver;

  /* Get the current thread */
  pCurrentThread = GetCurrentThread();
  intId = CPUGetContextInterruptNumber(pCurrentThread);

  /* Find the E1000E driver associated with this interrupt */
  SpinlockAcquireISR(&sDriverListLock);
  pDriver = spDriverList;
  while (pDriver != NULL)
  {
    if (pDriver->interruptLine == (int32_t)intId)
    {
      break;
    }
    pDriver = pDriver->pNext;
  }
  SpinlockReleaseISR(&sDriverListLock);


  if (pDriver != NULL)
  {
    status = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_ICR));
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E %04X:%04X:%04X interrupt status: 0x%08X",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function,
                 status);
    if ((status & (E1000E_ICR_RXT0 | E1000E_ICR_LSC | E1000E_ICR_TXDW)) != 0)
    {
      pDriver->cachedICR = status;
      NetManagerSubmitTask(&pDriver->networkDevice);
    }
  }

  /* Acknowledge the interrupt in the IOAPIC */
  InterruptSetEOI(intId);

  return true;
}

static void _HandleInterruptWorker(void* pDriverPtr)
{
  S_E1000EDriver* pDriver;
  uint32_t       icrStatus;

  pDriver = (S_E1000EDriver *)pDriverPtr;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X handling interrupt",
               pDriver->pPCIDevice->bus,
               pDriver->pPCIDevice->slot,
               pDriver->pPCIDevice->function);

  icrStatus = pDriver->cachedICR;

  if (icrStatus & E1000E_ICR_TXDW)
  {
    /* Handle transmit Complete */
    _CompleteTransmit(pDriver, NULL);
  }

  if (icrStatus & E1000E_ICR_RXT0)
  {
    /* Handle the received packet */
    _HandleReceivedPacket(pDriver);
  }

  if (icrStatus & E1000E_ICR_LSC)
  {
    _HandleLinkStatusChange(pDriver);
  }

  pDriver->cachedICR = 0;

  /* Re-enable the interrupt in the device */
  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMS),
               E1000E_ENABLED_INTERRUPTS);
}

static void _HandleLinkStatusChange(void* pDriverPtr)
{
  uint32_t        status;
  S_E1000EDriver* pDriver;

  pDriver = (S_E1000EDriver *)pDriverPtr;

  pDriver->linkOperationRunning = true;

  /* Read the link status */
  status = _MMIORead32(E1000E_REG(pDriver->barAddress, E1000E_STATUS));
  if (status & E1000E_STATUS_LU)
  {

    pDriver->isLinkUp = true;

    if ((status & E1000E_STATUS_SPEED_MASK) == E1000E_STATUS_SPEED_1000A)
    {
      pDriver->linkSpeed = LINK_SPEED_1GBPS
    }
    if ((status & E1000E_STATUS_SPEED_MASK) == E1000E_STATUS_SPEED_1000B)
    {
      pDriver->linkSpeed = LINK_SPEED_1GBPS
    }
    else if ((status & E1000E_STATUS_SPEED_MASK) == E1000E_STATUS_SPEED_100)
    {
      pDriver->linkSpeed = LINK_SPEED_100MBPS;
    }
    else if ((status & E1000E_STATUS_SPEED_MASK) == E1000E_STATUS_SPEED_10)
    {
      pDriver->linkSpeed = LINK_SPEED_10MBPS;
    }
    else
    {
      pDriver->linkSpeed = LINK_SPEED_10MBPS;
    }

    if (status & E1000E_STATUS_FD)
    {
      pDriver->duplexType = DUPLEX_FULL;
    }
    else
    {
      pDriver->duplexType = DUPLEX_HALF;
    }

    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X link is up",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }
  else
  {
    pDriver->isLinkUp = false;
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "E1000E device %04X:%04X:%04X link is down",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
  }

  /* Enable the interrupt in the device */
  if (pDriver->isInitialized == true)
  {
    /* Re-enable the interrupt in the device */
    _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_IMS),
                 E1000E_ENABLED_INTERRUPTS);
  }

  pDriver->linkOperationRunning = false;
}

static void _CompleteTransmit(S_E1000EDriver* pDriver, bool* pIsMoreToClean)
{
  uint32_t                 watchIdx;
  uint32_t                 cleanIdx;
  uint32_t                 nextIdx;
  S_E1000ETXBufferMetadata freeBuffers[E1000E_TX_CLEAN_BUDGET];
  uint32_t                 freeCount;
  S_KernelThread*          pCurThread;

  pDriver->txCompleteOperationRunning = true;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
              MODULE_NAME,
              "E1000E device %04X:%04X:%04X transmit complete",
              pDriver->pPCIDevice->bus,
              pDriver->pPCIDevice->slot,
              pDriver->pPCIDevice->function);

  do
  {
    freeCount = 0;

    /* Check if there is anything to clean */
    SpinlockAcquire(&pDriver->txRingLock);
    nextIdx  = pDriver->txDescriptorIdx;
    watchIdx = pDriver->txDescriptorWatchIdx;
    cleanIdx = pDriver->txDescriptorCleanIdx;

    if (cleanIdx != nextIdx)
    {
      /* Check if the next to watch is done processing */
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X checking descriptor %u status: 0x%08X",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function,
                  watchIdx,
                  pDriver->pTXDescriptorRing[watchIdx].status);
      if ((pDriver->pTXDescriptorRing[watchIdx].status & E1000E_TXD_STAT_DD) == E1000E_TXD_STAT_DD)
      {
        while (cleanIdx != nextIdx && freeCount < E1000E_TX_CLEAN_BUDGET)
        {
          KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                      MODULE_NAME,
                      "E1000E device %04X:%04X:%04X cleaning descriptor %u",
                      pDriver->pPCIDevice->bus,
                      pDriver->pPCIDevice->slot,
                      pDriver->pPCIDevice->function,
                      cleanIdx);
          /* Clean the descriptor */
          pDriver->pTXDescriptorRing[cleanIdx].status = 0;

          /* Release the buffer */
          if (pDriver->pTXDescriptorMeta[cleanIdx].length != 0)
          {
            KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                        MODULE_NAME,
                        "E1000E device %04X:%04X:%04X unring buffer at 0x%p of size %u",
                        pDriver->pPCIDevice->bus,
                        pDriver->pPCIDevice->slot,
                        pDriver->pPCIDevice->function,
                        pDriver->pTXDescriptorMeta[cleanIdx].virtAddr,
                        pDriver->pTXDescriptorMeta[cleanIdx].length);
            freeBuffers[freeCount].virtAddr =
                pDriver->pTXDescriptorMeta[cleanIdx].virtAddr;
            freeBuffers[freeCount].length =
                pDriver->pTXDescriptorMeta[cleanIdx].length;
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
    SpinlockRelease(&pDriver->txRingLock);

    /* Release the buffers */
    for (nextIdx = 0; nextIdx < freeCount; ++nextIdx)
    {
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X releasing buffer at 0x%p of size %u",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function,
                  freeBuffers[nextIdx].virtAddr,
                  freeBuffers[nextIdx].length);
      NetManagerReleaseTransmitBuffer((void *)freeBuffers[nextIdx].virtAddr,
                                      freeBuffers[nextIdx].length);
    }
    /* If we haven't cleaned enough buffers, respin */
    if (freeCount == E1000E_TX_CLEAN_BUDGET)
    {
      pCurThread = GetCurrentThread();
      CPUSaveContextAndSchedule(pCurThread->pVCpu);
    }

  } while (freeCount == E1000E_TX_CLEAN_BUDGET);

  if (pIsMoreToClean != NULL)
  {
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000E device %04X:%04X:%04X transmit complete, more to clean: %s %x %x",
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

  pDriver->txCompleteOperationRunning = false;
}

static void _HandleReceivedPacket(S_E1000EDriver* pDriver)
{
  uint32_t               pktIdx;
  uint16_t               pktSize;
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
  pRXDescRing = pDriver->pRXDescriptorRing;
  pFragmentList = pDriver->pRXFragmentList;

  pDriver->rxOperationRunning = true;

  do
  {
    /* Go through all descriptors to check the reception status */
    pollBudget = 0;
    while ((pRXDescRing[pktIdx].status & E1000E_RXD_STAT_DD) != 0 &&
          pollBudget < E1000E_POLL_MAX_BUDGET)
    {
      /* Process the received packet */
      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X processing received packet",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function);

      /* Get the packet size and buffer addresses */
      pktSize = pRXDescRing[pktIdx].length;
      physAddr = pDriver->pRXDescriptorAddresses[pktIdx].physAddr;
      virtAddr = (uintptr_t)pDriver->pRXDescriptorAddresses[pktIdx].virtAddr;

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

          pDriver->pRXFragmentList = pFragmentList;
        }
        else
        {
          KERNEL_ERROR("E1000E device %04X:%04X:%04X failed to allocate "
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
        if ((pRXDescRing[pktIdx].status & E1000E_RXD_STAT_EOP) == E1000E_RXD_STAT_EOP ||
            pFragmentList->fragmentCount >= NET_MGR_MAX_FRAGMENT_COUNT)
        {
          /* Process the complete packet */
          NetManagerProcessReceivedPacket(pFragmentList,
                                          &pDriver->networkDevice);
          pDriver->pRXFragmentList = NULL;
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
        pDriver->pRXDescriptorAddresses[pktIdx].virtAddr = (void *)
            packetBuffer.virtAddr;
        pDriver->pRXDescriptorAddresses[pktIdx].physAddr =
            packetBuffer.physAddr;
        pRXDescRing[pktIdx].bufferAddress = packetBuffer.physAddr;
        pRXDescRing[pktIdx].length = pDriver->bufferSize;
        pRXDescRing[pktIdx].status = 0;
      }
      else
      {
        if (pDriver->pRXFragmentList != NULL)
        {
          /* Drop the fragment list */
          NetManagerProcessReceivedPacket(pDriver->pRXFragmentList,
                                          &pDriver->networkDevice);
          pDriver->pRXFragmentList = NULL;
          pFragmentList = NULL;
        }

        /* Backpressure: drop the packet */
        ++pDriver->stats.rxDropped;
        pRXDescRing[pktIdx].length = pDriver->bufferSize;
        pRXDescRing[pktIdx].status = 0;
        KERNEL_ERROR("E1000E device %04X:%04X:%04X no free buffer available,"
                    " dropping packet\n",
                    pDriver->pPCIDevice->bus,
                    pDriver->pPCIDevice->slot,
                    pDriver->pPCIDevice->function);
      }

      /* Move to the next descriptor */
      pktIdx = (pktIdx + 1) % pDriver->rxBufferCount;
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

    /* Update the receive descriptor index */
    pDriver->rxDescriptorIdx = pktIdx;
    if (pollBudget == E1000E_POLL_MAX_BUDGET)
    {
      /* We still have packets to read, simply yield */
      pCurThread = GetCurrentThread();
      CPUSaveContextAndSchedule(pCurThread->pVCpu);
    }
  } while (pollBudget == E1000E_POLL_MAX_BUDGET);

  _MMIOWrite32(E1000E_REG(pDriver->barAddress, E1000E_RDT0), tail & 0xFFFF);

  pDriver->rxOperationRunning = false;
}

static void _UpdateStatistics(void* pDriver)
{
  uint32_t       regLow;
  uint32_t       regHigh;
  uintptr_t      bar;
  S_E1000EDriver* pDev;

  pDev = (S_E1000EDriver*)pDriver;
  bar = (pDev->barAddress);

  SpinlockAcquire(&pDev->statsLock);

  /* General statistics */
  pDev->stats.rxPackets += _MMIORead32(E1000E_REG(bar, E1000E_GPRC));
  pDev->stats.txPackets += _MMIORead32(E1000E_REG(bar, E1000E_GPTC));
  regLow  = _MMIORead32(E1000E_REG(bar, E1000E_GORCL));
  regHigh = _MMIORead32(E1000E_REG(bar, E1000E_GORCH));
  pDev->stats.rxBytes   += regLow | ((uint64_t)regHigh << 32);
  regLow  = _MMIORead32(E1000E_REG(bar, E1000E_GOTCL));
  regHigh = _MMIORead32(E1000E_REG(bar, E1000E_GOTCH));
  pDev->stats.txBytes   += regLow | ((uint64_t)regHigh << 32);

  pDev->stats.multicast  += _MMIORead32(E1000E_REG(bar, E1000E_MPRC));
  pDev->stats.collisions += _MMIORead32(E1000E_REG(bar, E1000E_COLC));

  /* RX Errors */
  pDev->stats.rxLengthErrors += _MMIORead32(E1000E_REG(bar, E1000E_RLEC));
  pDev->stats.rxOverErrors   += _MMIORead32(E1000E_REG(bar, E1000E_RNBC));
  pDev->stats.rxCrcErrors    += _MMIORead32(E1000E_REG(bar, E1000E_CRCERRS));
  pDev->stats.rxFrameErrors  += _MMIORead32(E1000E_REG(bar, E1000E_ALGNERRC));
  pDev->stats.rxFifoErrors   += _MMIORead32(E1000E_REG(bar, E1000E_MPC));
  pDev->stats.rxMissedErrors = pDev->stats.rxFifoErrors;
  pDev->stats.rxErrors = pDev->stats.rxLengthErrors +
                            pDev->stats.rxOverErrors +
                            pDev->stats.rxCrcErrors +
                            pDev->stats.rxFrameErrors +
                            pDev->stats.rxFifoErrors;

  /* TX Errors */
  pDev->stats.txAbortedErrors   += _MMIORead32(E1000E_REG(bar, E1000E_ECOL));
  pDev->stats.txCarrierErrors   += _MMIORead32(E1000E_REG(bar, E1000E_TNCRS));
  pDev->stats.txFifoErrors      += _MMIORead32(E1000E_REG(bar, E1000E_LCOL));
  pDev->stats.txWindowErrors    += pDev->stats.txFifoErrors;
  pDev->stats.txErrors = pDev->stats.txAbortedErrors +
                            pDev->stats.txCarrierErrors +
                            pDev->stats.txFifoErrors;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "E1000E device %04X:%04X:%04X updated statistics: "
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

  SpinlockRelease(&pDev->statsLock);
}

static ssize_t _IOControlGetStats(S_E1000EDriver* pDriver, void* pArg)
{
  ssize_t  retCode;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(S_NetworkStatistics),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RW) == true)
  {
    _UpdateStatistics(pDriver);

    SpinlockAcquire(&pDriver->statsLock);

    memcpy(pArg, &pDriver->stats, sizeof(S_NetworkStatistics));

    SpinlockRelease(&pDriver->statsLock);

    retCode = sizeof(S_NetworkStatistics);
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl get stats called with "
                 "invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlResetStats(S_E1000EDriver* pDriver)
{
  SpinlockAcquire(&pDriver->statsLock);

  memset(&pDriver->stats, 0, sizeof(S_NetworkStatistics));

  SpinlockRelease(&pDriver->statsLock);

  return 0;
}

static ssize_t _IOControlSetMACAddress(S_E1000EDriver* pDriver, void* pArg)
{
  ssize_t  retCode;
  uint64_t macAddress;
  E_Return internalErr;

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
      KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl set MAC address called"
                   " with invalid multicast MAC address\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      retCode = -EINVAL;
    }
    else if (macAddress == 0 || macAddress == 0x0000FFFFFFFFFFFF)
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl set MAC address called"
                   " with invalid MAC address\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      retCode = -EINVAL;
    }
    else
    {

      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "E1000E device %04X:%04X:%04X ioctl set MAC address to "
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

      internalErr = KernelMutexLock(&pDriver->configLock);
      E1000E_ASSERT(internalErr == NO_ERROR,
                   "E1000E device failed to lock config mutex\n",
                   internalErr);

      pDriver->macAddress = macAddress;
      _SetDeviceMAC(pDriver, macAddress);

      internalErr = KernelMutexUnlock(&pDriver->configLock);
      E1000E_ASSERT(internalErr == NO_ERROR,
                   "E1000E device failed to unlock config mutex\n",
                   internalErr);

      retCode = 0;
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device NULL ioctl set MAC address called with NULL "
                 "driver\n");
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlGetMACAddress(S_E1000EDriver* pDriver, void* pArg)
{
  ssize_t   retCode;
  uint64_t* pMACAddress;
  E_Return  internalErr;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(uint64_t),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RW) == true)
  {
    internalErr = KernelMutexLock(&pDriver->configLock);
    E1000E_ASSERT(internalErr == NO_ERROR,
                 "E1000E device failed to lock config mutex\n",
                 internalErr);


    pMACAddress  = (uint64_t*)pArg;
    *pMACAddress = pDriver->macAddress;

    internalErr = KernelMutexUnlock(&pDriver->configLock);
    E1000E_ASSERT(internalErr == NO_ERROR,
                 "E1000E device failed to unlock config mutex\n",
                 internalErr);

    retCode = sizeof(pDriver->macAddress);
  }
  else
  {
    KERNEL_ERROR("E1000E MAC address called with NULL args\n");
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlSetLoopbackMode(S_E1000EDriver* pDriver, void* pArg)
{
  ssize_t  retCode;
  E_Return internalErr;
  bool     isLoopbackEnabled;

  /* Validate parameters */
  if (pArg != NULL &&
      MemoryStringIsMapped(pArg,
                           sizeof(bool),
                           MEMMGR_MAP_USER |
                           MEMMGR_MAP_KERNEL |
                           MEMMGR_MAP_RO) == true)
  {
    isLoopbackEnabled = *(bool*)pArg;
    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000E device %04X:%04X:%04X ioctl set loopback mode to %s",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function,
                isLoopbackEnabled ? "enabled" : "disabled");

    internalErr = KernelMutexLock(&pDriver->configLock);
    E1000E_ASSERT(internalErr == NO_ERROR,
                 "E1000E device failed to lock config mutex\n",
                 internalErr);

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
      /* Reinitialize the network device */
      _DevDownlinkSafe(pDriver);
      _DevUplinkSafe(pDriver);
    }

    internalErr = KernelMutexUnlock(&pDriver->configLock);
    E1000E_ASSERT(internalErr == NO_ERROR,
                 "E1000E device failed to unlock config mutex\n",
                 internalErr);
    retCode = 0;
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl set loopback mode called"
                 " with invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlGetLinkStatus(S_E1000EDriver* pDriver, void* pArg)
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
    if (pDriver->isInitialized == true)
    {
      *(bool*)pArg = pDriver->isLinkUp;
    }
    else
    {
      *(bool*)pArg = false;
    }

    KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000E device %04X:%04X:%04X ioctl get link status: %s",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function,
                pDriver->isLinkUp ? "up" : "down");
    retCode = 0;
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl get link status called"
                 " with invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}

static ssize_t _IOControlSetAutoNeg(S_E1000EDriver* pDriver)
{
  ssize_t  retCode;
  E_Return internalErr;

  internalErr = KernelMutexLock(&pDriver->configLock);
  E1000E_ASSERT(internalErr == NO_ERROR,
                "E1000E device failed to lock config mutex\n",
                internalErr);

  /* Set the auto-negotiation parameters */
  pDriver->autoNegMask = E1000E_AUTONEG_MASK;
  pDriver->linkSpeed   = LINK_SPEED_AUTONEG;
  pDriver->duplexType  = DUPLEX_AUTONEG;

  KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                MODULE_NAME,
                "E1000E device %04X:%04X:%04X ioctl set auto-negotiation",
                pDriver->pPCIDevice->bus,
                pDriver->pPCIDevice->slot,
                pDriver->pPCIDevice->function);

  if (pDriver->isInitialized == true)
  {


    /* Reinitialize the network device */
    _DevDownlinkSafe(pDriver);
    _DevUplinkSafe(pDriver);
    retCode = 0;
  }

  internalErr = KernelMutexUnlock(&pDriver->configLock);
  E1000E_ASSERT(internalErr == NO_ERROR,
               "E1000E device failed to unlock config mutex\n",
               internalErr);

  return retCode;
}

static ssize_t _IOControlForceSpeedDuplex(S_E1000EDriver* pDriver, void* pArg)
{
  ssize_t              retCode;
  S_NetworkLinkConfig* pConfig;
  bool                 valid;
  E_Return             internalErr;

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
      internalErr = KernelMutexLock(&pDriver->configLock);
      E1000E_ASSERT(internalErr == NO_ERROR,
                   "E1000E device failed to lock config mutex\n",
                   internalErr);

      pDriver->autoNegMask = 0;
      pDriver->linkSpeed   = pConfig->linkSpeed;
      pDriver->duplexType  = pConfig->duplexType;

      KERNEL_DEBUG(E1000E_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "E1000E device %04X:%04X:%04X ioctl force speed/duplex to "
                    "%s/%s",
                  pDriver->pPCIDevice->bus,
                  pDriver->pPCIDevice->slot,
                  pDriver->pPCIDevice->function,
                  (pConfig->linkSpeed == LINK_SPEED_10MBPS) ? "10Mbps" :
                  (pConfig->linkSpeed == LINK_SPEED_100MBPS) ? "100Mbps" :
                  (pConfig->linkSpeed == LINK_SPEED_1GBPS) ? "1Gbps" : "N/A",
                  (pConfig->duplexType == DUPLEX_HALF) ? "Half" :
                  (pConfig->duplexType == DUPLEX_FULL) ? "Full" : "N/A");

      if (pDriver->isInitialized == true)
      {
        /* Reinitialize the network device */
        _DevDownlinkSafe(pDriver);
        _DevUplinkSafe(pDriver);
      }

      retCode = 0;

      internalErr = KernelMutexUnlock(&pDriver->configLock);
      E1000E_ASSERT(internalErr == NO_ERROR,
                   "E1000E device failed to unlock config mutex\n",
                   internalErr);
    }
    else
    {
      KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl force speed/duplex "
                   "called with invalid parameters\n",
                   pDriver->pPCIDevice->bus,
                   pDriver->pPCIDevice->slot,
                   pDriver->pPCIDevice->function);
      retCode = -EINVAL;
    }
  }
  else
  {
    KERNEL_ERROR("E1000E device %04X:%04X:%04X ioctl force speed/duplex called"
                 " with invalid args\n",
                 pDriver->pPCIDevice->bus,
                 pDriver->pPCIDevice->slot,
                 pDriver->pPCIDevice->function);
    retCode = -EINVAL;
  }

  return retCode;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_PCI(sX86E1000EDriver);

/************************************ EOF *************************************/