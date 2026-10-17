/*******************************************************************************
 * @file AHCI.c
 *
 * @see AHCI.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 09/10/2026
 *
 * @version 1.0
 *
 * @brief Kernel's AHCI (Advanced Host Controller Interface) driver.
 *
 * @details Kernel's AHCI (Advanced Host Controller Interface) driver. Defines
 * the functions and definitions to interact with the AHCI controller and its
 * devices.
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
#include <stdint.h>
#include <stddef.h>
#include <Memory.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <VirtualFS.h>
#include <Scheduler.h>
#include <PCIExpress.h>
#include <KernelHeap.h>
#include <DiskManager.h>
#include <KernelError.h>
#include <KernelQueue.h>
#include <TimerManager.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <AHCI.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "X86 AHCI"

/** @brief Handoff timeout value for graceful handoff */
#define AHCI_HANDOFF_TIMEOUT_NS 25000000ULL
/** @brief Handoff timeout value for forced handoff */
#define AHCI_HANDOFF_FORCE_TIMEOUT_NS 2000000000ULL
/** @brief AHCI reset timeout in nanoseconds. */
#define AHCI_RESET_TIMEOUT_NS 1000000000ULL

/** @brief AHCI global host control bit for enabling AHCI mode. */
#define AHCI_GLOBAL_HOST_CONTROL_AHCI_ENABLE 0x80000000
/** @brief AHCI global host control bit for enabling interrupts. */
#define AHCI_GLOBAL_HOST_CONTROL_AHCI_IE 0x00000002
/** @brief AHCI global host control bit for resetting the HBA. */
#define AHCI_GLOBAL_HOST_CONTROL_HBA_RESET 0x00000001
/** @brief BIOS handoff control status - BIOS owned */
#define AHCI_BIOS_HANDOFF_CONTROL_STATUS_BIOS_OWNED 0x01
/** @brief BIOS handoff control status - OS owned */
#define AHCI_BIOS_HANDOFF_CONTROL_STATUS_OS_OWNED 0x02
/** @brief BIOS handoff control status - BIOS busy */
#define AHCI_BIOS_HANDOFF_CONTROL_STATUS_BIOS_BUSY 0x04

/** @brief Offset of the first AHCI port from the base address. */
#define AHCI_PORT_OFFSET 0x100
/** @brief Size of each AHCI port's register space. */
#define AHCI_PORT_SIZE 0x80

/** @brief SATA status bits for device detection: present. */
#define AHCI_SATA_STATUS_DET_PRESENT 0x03
/** @brief SATA control bits for device detection: initialization. */
#define AHCI_SATA_CONTROL_DET_INIT 0x1
/** @brief SATA control bits for device detection: none. */
#define AHCI_SATA_CONTROL_DET_NONE 0x0
/** @brief SATA status bits for device detection shift. */
#define AHCI_SATA_STATUS_DET_SHIFT 0
/** @brief SATA status bits for interface power management: active. */
#define AHCI_SATA_STATUS_IPM_ACTIVE 0x01
/** @brief SATA status bits for interface power management shift. */
#define AHCI_SATA_STATUS_IPM_SHIFT 8
/** @brief SATA status bits for device detection mask. */
#define AHCI_SATA_STATUS_DET_MASK (0x0F << AHCI_SATA_STATUS_DET_SHIFT)
/** @brief SATA status bits for interface power management mask. */
#define AHCI_SATA_STATUS_IPM_MASK (0x0F << AHCI_SATA_STATUS_IPM_SHIFT)
/** @brief AHCI port reset delay in nanoseconds. */
#define AHCI_PORT_RESET_DELAY_NS     2000000ULL
/** @brief AHCI port reset timeout in nanoseconds. */
#define AHCI_PORT_RESET_TIMEOUT_NS 500000000ULL
/** @brief AHCI port stop timeout in nanoseconds. */
#define AHCI_PORT_STOP_TIMEOUT_NS  500000000ULL
/** @brief AHCI command timeout in nanoseconds. */
#define AHCI_COMMAND_TIMEOUT_NS 1000000000
/** @brief AHCI command and status start bit. */
#define AHCI_COMMAND_AND_STATUS_START 0x00000001
/** @brief AHCI command and status FIS receive enable bit. */
#define AHCI_COMMAND_AND_STATUS_FIS_RECEIVE_ENABLE 0x00000010
/** @brief AHCI command and status FIS receive running bit. */
#define AHCI_COMMAND_AND_STATUS_FIS_RECEIVE_RUNNING 0x00004000
/** @brief AHCI command and status command list running bit. */
#define AHCI_COMMAND_AND_STATUS_CMD_LIST_RUNNING 0x00008000

/** @brief Size of the command list. */
#define COMMAND_LIST_SIZE 1024
/** @brief Size of the FIS receive area. */
#define FIS_RECEIVE_AREA_SIZE 256
/** @brief Size of each command table. */
#define COMMAND_TABLE_SIZE 128
/** @brief Number of command tables. */
#define COMMAND_TABLE_COUNT 32
/** @brief Size of each PRDT. */
#define PRDT_SIZE 16
/** @brief Number of PRDTs. */
#define PRDT_COUNT 16

/** @brief SATA signature for SATA devices. */
#define AHCI_SIGNATURE_SATA 0x00000101
/** @brief SATA signature for SATAPI devices. */
#define AHCI_SIGNATURE_SATAPI 0xEB140101
/** @brief SATA signature for SEMB devices. */
#define AHCI_SIGNATURE_SEMB 0xC33C0101
/** @brief SATA signature for PM devices. */
#define AHCI_SIGNATURE_PM 0x96690101

/** @brief AHCI interrupt DHRE enable bits. */
#define AHCI_INTERRUPT_ENABLE_DHRE 0x00000001
/** @brief AHCI interrupt PSE enable bits. */
#define AHCI_INTERRUPT_ENABLE_PSE  0x00000002
/** @brief AHCI interrupt DSE enable bits. */
#define AHCI_INTERRUPT_ENABLE_DSE  0x00000004
/** @brief AHCI interrupt SBDE enable bits. */
#define AHCI_INTERRUPT_ENABLE_SBDE 0x00000008
/** @brief AHCI interrupt UFE enable bits. */
#define AHCI_INTERRUPT_ENABLE_UFE  0x00000010
/** @brief AHCI interrupt PCE enable bits. */
#define AHCI_INTERRUPT_ENABLE_PCE  0x00000020
/** @brief AHCI interrupt DMPE enable bits. */
#define AHCI_INTERRUPT_ENABLE_DMPE 0x00000040
/** @brief AHCI interrupt PRCE enable bits. */
#define AHCI_INTERRUPT_ENABLE_PRCE 0x00400000
/** @brief AHCI interrupt IPME enable bits. */
#define AHCI_INTERRUPT_ENABLE_IPME 0x00800000
/** @brief AHCI interrupt OFE enable bits. */
#define AHCI_INTERRUPT_ENABLE_OFE  0x01000000
/** @brief AHCI interrupt INFE enable bits. */
#define AHCI_INTERRUPT_ENABLE_INFE 0x04000000
/** @brief AHCI interrupt IFE enable bits. */
#define AHCI_INTERRUPT_ENABLE_IFE  0x08000000
/** @brief AHCI interrupt HBDE enable bits. */
#define AHCI_INTERRUPT_ENABLE_HBDE 0x10000000
/** @brief AHCI interrupt HBFE enable bits. */
#define AHCI_INTERRUPT_ENABLE_HBFE 0x20000000
/** @brief AHCI interrupt TFEE enable bits. */
#define AHCI_INTERRUPT_ENABLE_TFEE 0x40000000
/** @brief AHCI interrupt CPDE enable bits. */
#define AHCI_INTERRUPT_ENABLE_CPDE 0x80000000

/** @brief AHCI interrupt enable mask. */
#define AHCI_INTERRUPT_ENABLE_MASK (AHCI_INTERRUPT_ENABLE_DHRE | \
                                    AHCI_INTERRUPT_ENABLE_PSE  | \
                                    AHCI_INTERRUPT_ENABLE_DSE  | \
                                    AHCI_INTERRUPT_ENABLE_SBDE | \
                                    AHCI_INTERRUPT_ENABLE_UFE  | \
                                    AHCI_INTERRUPT_ENABLE_PCE  | \
                                    AHCI_INTERRUPT_ENABLE_DMPE | \
                                    AHCI_INTERRUPT_ENABLE_PRCE | \
                                    AHCI_INTERRUPT_ENABLE_IPME | \
                                    AHCI_INTERRUPT_ENABLE_OFE  | \
                                    AHCI_INTERRUPT_ENABLE_INFE | \
                                    AHCI_INTERRUPT_ENABLE_IFE  | \
                                    AHCI_INTERRUPT_ENABLE_HBDE | \
                                    AHCI_INTERRUPT_ENABLE_HBFE | \
                                    AHCI_INTERRUPT_ENABLE_TFEE | \
                                    AHCI_INTERRUPT_ENABLE_CPDE)

/** @brief AHCI interrupt status bits: Device to Host Register FIS Interrupt */
#define AHCI_INTERRUPT_STATUS_DHRS 0x00000001
/** @brief AHCI interrupt status bits: PIO Setup FIS Interrupt */
#define AHCI_INTERRUPT_STATUS_PSS  0x00000002
/** @brief AHCI interrupt status bits: DMA Setup FIS Interrupt */
#define AHCI_INTERRUPT_STATUS_DSS  0x00000004
/** @brief AHCI interrupt status bits: Set Device Bits Interrupt */
#define AHCI_INTERRUPT_STATUS_SDBS 0x00000008
/** @brief AHCI interrupt status bits: Unknown FIS Interrupt */
#define AHCI_INTERRUPT_STATUS_UFS  0x00000010
/** @brief AHCI interrupt status bits: Descriptor Processed */
#define AHCI_INTERRUPT_STATUS_DPS  0x00000020
/** @brief AHCI interrupt status bits: Port Change Status */
#define AHCI_INTERRUPT_STATUS_PCS  0x00000040
/** @brief AHCI interrupt status bits: Device Mechanical Presence Status */
#define AHCI_INTERRUPT_STATUS_DMPS 0x00000080
/** @brief AHCI interrupt status bits: PhyRdy Change Status */
#define AHCI_INTERRUPT_STATUS_PRCS 0x00400000
/** @brief AHCI interrupt status bits: Incorrect Port Multiplied Status */
#define AHCI_INTERRUPT_STATUS_IPMS 0x00800000
/** @brief AHCI interrupt status bits: Output FIS Error Status */
#define AHCI_INTERRUPT_STATUS_OFS  0x01000000
/** @brief AHCI interrupt status bits: Interface Non Fatal Error Status */
#define AHCI_INTERRUPT_STATUS_INFS 0x04000000
/** @brief AHCI interrupt status bits: Interface Fatal Error Status */
#define AHCI_INTERRUPT_STATUS_IFS  0x08000000
/** @brief AHCI interrupt status bits: Host Bus Data Error Status */
#define AHCI_INTERRUPT_STATUS_HBDS 0x10000000
/** @brief AHCI interrupt status bits: Host Bus Fatal Error Status */
#define AHCI_INTERRUPT_STATUS_HBFS 0x20000000
/** @brief AHCI interrupt status bits: Task File Error Status */
#define AHCI_INTERRUPT_STATUS_TFES 0x40000000
/** @brief AHCI interrupt status bits: Cold Presence Detect Status */
#define AHCI_INTERRUPT_STATUS_CPDS 0x80000000

/** @brief Mask for all error interrupt status bits. */
#define IS_ERROR_MASK (AHCI_INTERRUPT_STATUS_TFES | \
                       AHCI_INTERRUPT_STATUS_HBFS | \
                       AHCI_INTERRUPT_STATUS_HBDS | \
                       AHCI_INTERRUPT_STATUS_IFS  | \
                       AHCI_INTERRUPT_STATUS_INFS)

/** @brief Command header configuration: Read */
#define COMMAND_HEADER_CONFIG_READ 0
/** @brief Configuration for a write command header. */
#define COMMAND_HEADER_CONFIG_WRITE (1 << 6)
/** @brief Command header configuration: ATAPI */
#define COMMAND_HEADER_CONFIG_ATAPI 0x20
/** @brief Size of the identify data. */
#define CMD_IDENTIFY_DATA_SIZE 512
/** @brief FIS register: Host to Device */
#define FIS_REGISTER_H2D 0x27
/** @brief FIS register: Command */
#define FIS_COMMAND_FIS 0x8000
/** @brief FIS command: Identify Packet Device */
#define FIS_COMMAND_IDENTIFY_PACKET_DEVICE 0xA10000
/** @brief FIS command: Identify Device */
#define FIS_COMMAND_ATA_IDENTIFY 0xEC0000
/** @brief FIS command: Read DMA Extended */
#define FIS_COMMAND_ATA_READ_DMA_EXT 0x25
/** @brief FIS command: Write DMA Extended */
#define FIS_COMMAND_ATA_WRITE_DMA_EXT 0x35
/** @brief FIS command shift*/
#define FIS_COMMAND_SHIFT 16
/** @brief FIS device shift */
#define FIS_DEVICE_SHIFT 24

/** @brief FIS command device parameter: ATA Device LBA */
#define FIS_ATA_DEVICE_LBA 0x40

/** @brief Mask for busy task file status bits. */
#define TFD_STATUS_BSY 0x80
/** @brief Mask for data request task file status bits. */
#define TFD_STATUS_DRQ 0x08
/** @brief Mask for error task file status bits. */
#define TFD_STATUS_ERROR 0x0001
/** @brief Mask for all error task file data bits. */
#define TFD_ERROR 0xFF00

/** @brief ATA device type: SATA */
#define ATA_DEVICE_TYPE_SATA 0x00
/** @brief ATA device type: SATAPI */
#define ATA_DEVICE_TYPE_SATAPI 0x01
/** @brief Identify checksum valid */
#define IDENTIFY_CHECKSUM_VALID 0xA5

/** @brief Maximum size of a PRDT entry. */
#define PRDT_ENTRY_MAX_SIZE 0x1000
/** @brief IOC flag for a PRDT entry. */
#define PRDT_ENTRY_IOC (1 << 31)
/** @brief Shift for the PRDTL field in the command header. */
#define AHCI_COMMAND_HEADER_PRDTL_SHIFT 16
/** @brief Maximum number of AHCI ports. */
#define AHCI_MAX_PORTS 32

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Generic AHCI host control structure. */
typedef struct
{
  /** @brief Capabilities of the AHCI host */
  uint32_t capabilities;
  /** @brief Global host control register */
  uint32_t globalHostControl;
  /** @brief Interrupt status register */
  uint32_t interruptStatus;
  /** @brief Ports implemented register */
  uint32_t portsImplemented;
  /** @brief Version register */
  uint32_t version;
  /** @brief Command completion coalescing control register */
  uint32_t cccControl;
  /** @brief Command completion coalescing ports register */
  uint32_t cccPorts;
  /** @brief Enclosure management location register */
  uint32_t enclosureManagementLocation;
  /** @brief Enclosure management control register */
  uint32_t enclosureManagementControl;
  /** @brief Host capabilities extended register */
  uint32_t hostCapabilitiesExtended;
  /** @brief BIOS handoff control status register */
  uint32_t biosHandoffControlStatus;
  /** @brief Reserved space */
  uint8_t  reserved[0x74];
  /** @brief Vendor specific registers */
  uint8_t  vendorSpecific[0x60];
} __attribute__((packed)) S_GenericHostControl;

/** @brief Structure representing an AHCI port. */
typedef struct
{
  /** @brief Command list base address */
  uint32_t commandListBaseAddrLow;
  /** @brief Command list base address upper */
  uint32_t commandListBaseAddrHigh;
  /** @brief FIS base address low */
  uint32_t fisBaseAddrLow;
  /** @brief FIS base address upper */
  uint32_t fisBaseAddrHigh;
  /** @brief Interrupt status register */
  uint32_t interruptStatus;
  /** @brief Interrupt enable register */
  uint32_t interruptEnable;
  /** @brief Command and status register */
  uint32_t commandAndStatus;
  /** @brief Reserved space */
  uint32_t reserved0;
  /** @brief Task file data register */
  uint32_t taskFileData;
  /** @brief Signature register */
  uint32_t signature;
  /** @brief SATA status register */
  uint32_t sataStatus;
  /** @brief SATA control register */
  uint32_t sataControl;
  /** @brief SATA error register */
  uint32_t sataError;
  /** @brief SATA active register */
  uint32_t sataActive;
  /** @brief Command issue register */
  uint32_t commandIssue;
  /** @brief SATA notification register */
  uint32_t sataNotification;
  /** @brief FIS-based switching control register */
  uint32_t fisBasedSwitchingControl;
} __attribute__((packed)) S_AHCIPort;

/** @brief Structure representing an AHCI command table. */
typedef struct
{
  /** @brief Command header config */
  uint16_t config;
  /** @brief Physical region descriptor table length */
  uint16_t prdtl;
  /** @brief Physical region descriptor byte count */
  uint32_t prdbc;
  /** @brief Command table base address low */
  uint32_t ctbaLow;
  /** @brief Command table base address high */
  uint32_t ctbaHigh;
  /** @brief Reserved space */
  uint32_t reserved[4];
} __attribute__((packed)) S_AHCICommandHeader;

/** @brief Structure representing an AHCI physical region descriptor. */
typedef struct
{
  /** @brief Data base address low */
  uint32_t dbaLow;
  /** @brief Data base address high */
  uint32_t dbaHigh;
  /** @brief Reserved space */
  uint32_t reserved;
  /** @brief Data byte count and interrupt on completion */
  uint32_t dbcIoc;
} __attribute__((packed)) S_AHCIPhysicalRegionDescriptor;

/** @brief Structure representing an AHCI command table. */
typedef struct
{
  /** @brief Command FIS */
  uint8_t commandFIS[64];
  /** @brief ATAPI command */
  uint8_t atapiCommand[16];
  /** @brief Reserved space */
  uint8_t reserved[48];
  /** @brief Physical region descriptor table */
  S_AHCIPhysicalRegionDescriptor prdtEntry[PRDT_COUNT];
} __attribute__((packed)) S_ACPICommandTable;

/**
 * @brief Structure representing an AHCI FIS (Frame Information Structure) for
 * Register - Host to Device.
 */
typedef struct
{
  /** @brief FIS type */
  uint8_t fisType;
  /** @brief Port multiplier */
  uint8_t pmPort;
  /** @brief Command or control */
  uint8_t command;
  /** @brief Features low */
  uint8_t featureLow;

  /** @brief LBA (Logical Block Addressing) low */
  uint8_t lbaLow;
  /** @brief LBA (Logical Block Addressing) mid */
  uint8_t lbaMid;
  /** @brief LBA (Logical Block Addressing) high */
  uint8_t lbaHigh;
  /** @brief Device register */
  uint8_t device;

  /** @brief LBA (Logical Block Addressing) low extended */
  uint8_t lbaLowExp;
  /** @brief LBA (Logical Block Addressing) mid extended */
  uint8_t lbaMidExp;
  /** @brief LBA (Logical Block Addressing) high extended */
  uint8_t lbaHighExp;
  /** @brief Features high */
  uint8_t featureHigh;

  /** @brief Sector count low */
  uint8_t countLow;
  /** @brief Sector count high */
  uint8_t countHigh;
  /** @brief Isochronous command completion */
  uint8_t isoCommandCompletion;
  /** @brief Control register */
  uint8_t control;

  /** @brief Reserved space */
  uint32_t reserved1[4];
} __attribute__((packed)) S_FISRegisterH2D;

/** @brief ATA IDENTIFY DEVICE general configuration. */
typedef struct
{
  /** @brief Reserved bit. */
  uint16_t reserved0:           1;
  /** @brief Retired bit. */
  uint16_t retired0:            1;
  /** @brief Indicates an incomplete identify response. */
  uint16_t responseIncomplete:  1;
  /** @brief Retired bits. */
  uint16_t retired1:            3;
  /** @brief Obsolete bits. */
  uint16_t obsolete0:           2;
  /** @brief Retired bits. */
  uint16_t retired2:            7;
  /** @brief Indicates the device type. */
  uint16_t deviceType:          1;
} __attribute__((packed)) S_ATAIdentifyGeneralConfiguration;

/** @brief ATA IDENTIFY DEVICE trusted computing support. */
typedef struct
{
  /** @brief Indicates trusted computing feature support. */
  uint16_t featureSupported: 1;
  /** @brief Reserved bits. */
  uint16_t reserved:         15;
} __attribute__((packed)) S_ATAIdentifyTrustedComputing;

/** @brief ATA IDENTIFY DEVICE capabilities. */
typedef struct
{
  /** @brief Current long physical-sector alignment. */
  uint8_t currentLongPhysicalSectorAlignment: 2;
  /** @brief Reserved bits. */
  uint8_t reserved0:                          6;
  /** @brief Indicates DMA support. */
  uint8_t dmaSupported:                       1;
  /** @brief Indicates LBA support. */
  uint8_t lbaSupported:                       1;
  /** @brief Indicates that IORDY is disabled. */
  uint8_t iordyDisable:                       1;
  /** @brief Indicates IORDY support. */
  uint8_t iordySupported:                     1;
  /** @brief Reserved bit. */
  uint8_t reserved1:                          1;
  /** @brief Indicates standby timer support. */
  uint8_t standbyTimerSupport:                1;
  /** @brief Reserved bits. */
  uint8_t reserved2:                          2;
  /** @brief Standby timer value. */
  uint16_t standbyTimerValue:                  1;
  /** @brief Obsolete bits. */
  uint16_t obsolete0:                          1;
  /** @brief Reserved bits. */
  uint16_t reserved3:                         12;
  /** @brief Set to 1. */
  uint16_t setTo1:                             1;
  /** @brief Set to 0. */
  uint16_t setTo0:                             1;
} __attribute__((packed)) S_ATAIdentifyCapabilities;

/** @brief ATA IDENTIFY DEVICE multisector settings. */
typedef struct
{
  /** @brief Current multisector setting. */
  uint8_t currentMultiSectorSetting;
  /** @brief Indicates a valid current multisector setting. */
  uint8_t multiSectorSettingValid:             1;
  /** @brief Reserved bits. */
  uint8_t reservedByte59:                      3;
  /** @brief Indicates sanitize feature support. */
  uint8_t sanitizeFeatureSupported:            1;
  /** @brief Indicates crypto-scramble extended command support. */
  uint8_t cryptoScrambleExtCommandSupported:   1;
  /** @brief Indicates overwrite extended command support. */
  uint8_t overwriteExtCommandSupported:        1;
  /** @brief Indicates block-erase extended command support. */
  uint8_t blockEraseExtCommandSupported:       1;
} __attribute__((packed)) S_ATAIdentifyMultiSectorSetting;

/** @brief ATA IDENTIFY DEVICE multiword DMA settings. */
typedef struct
{
  /** @brief Indicates that multiword DMA mode 0 is supported. */
  uint16_t multiwordDMAMode0Supported: 1;
  /** @brief Indicates that multiword DMA mode 1 is supported. */
  uint16_t multiwordDMAMode1Supported: 1;
  /** @brief Indicates that multiword DMA mode 2 is supported. */
  uint16_t multiwordDMAMode2Supported: 1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                  5;
  /** @brief Indicates that multiword DMA mode 0 is selected. */
  uint16_t multiwordDMAMode0Selected:  1;
  /** @brief Indicates that multiword DMA mode 1 is selected. */
  uint16_t multiwordDMAMode1Selected:  1;
  /** @brief Indicates that multiword DMA mode 2 is selected. */
  uint16_t multiwordDMAMode2Selected:  1;
  /** @brief Reserved bits. */
  uint16_t reserved1:                  5;
} __attribute__((packed)) S_ATAIdentifyMultiwordDMASettings;

/** @brief ATA IDENTIFY DEVICE additional supported features. */
typedef struct
{
  /** @brief Reserved bits. */
  uint16_t reserved0:                                 2;
  /** @brief Indicates nonvolatile write-cache support. */
  uint16_t nonVolatileWriteCache:                     1;
  /** @brief Indicates extended user-addressable sector support. */
  uint16_t extendedUserAddressableSectorsSupported:   1;
  /** @brief Indicates that all user data is encrypted. */
  uint16_t deviceEncryptsAllUserData:                 1;
  /** @brief Indicates read-zero-after-TRIM support. */
  uint16_t readZeroAfterTrimSupported:                1;
  /** @brief Indicates optional 28-bit command support. */
  uint16_t optional28BitCommandsSupported:            1;
  /** @brief Indicates IEEE 1667 support. */
  uint16_t ieee1667:                                  1;
  /** @brief Indicates DMA download-microcode support. */
  uint16_t downloadMicrocodeDMASupported:             1;
  /** @brief Obsolete bit. */
  uint16_t obsolete0:                                 1;
  /** @brief Indicates DMA write-buffer support. */
  uint16_t writeBufferDMASupported:                   1;
  /** @brief Indicates DMA read-buffer support. */
  uint16_t readBufferDMASupported:                    1;
  /** @brief Obsolete bit. */
  uint16_t obsolete1:                                 1;
  /** @brief Indicates LPSA ERC support. */
  uint16_t lpsaERCSupported:                          1;
  /** @brief Indicates deterministic read-after-TRIM support. */
  uint16_t deterministicReadAfterTrimSupported:       1;
  /** @brief Indicates CFA specification support. */
  uint16_t cFastSpecSupported:                        1;
} __attribute__((packed)) S_ATAIdentifyAdditionalSupported;

/** @brief ATA IDENTIFY DEVICE Serial ATA capabilities. */
typedef struct
{
  /** @brief Set to zero bit. */
  uint16_t setTo0:                    1;
  /** @brief Indicates SATA Gen1 support. */
  uint16_t sataGen1:                  1;
  /** @brief Indicates SATA Gen2 support. */
  uint16_t sataGen2:                  1;
  /** @brief Indicates SATA Gen3 support. */
  uint16_t sataGen3:                  1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                 4;
  /** @brief Indicates native command queuing support. */
  uint16_t ncq:                       1;
  /** @brief Indicates host-initiated power management support. */
  uint16_t hipm:                      1;
  /** @brief Indicates PHY events support. */
  uint16_t phyEvents:                 1;
  /** @brief Indicates NCQ unload support. */
  uint16_t ncqUnload:                 1;
  /** @brief Indicates NCQ priority support. */
  uint16_t ncqPriority:               1;
  /** @brief Indicates host automatic power-saving support. */
  uint16_t hostAutoPS:                1;
  /** @brief Indicates device automatic power-saving support. */
  uint16_t deviceAutoPS:              1;
  /** @brief Indicates read-log DMA support. */
  uint16_t readLogDMA:                1;
  /** @brief Set to zero bit. */
  uint16_t setTo01:                   1;
  /** @brief Current SATA link speed. */
  uint16_t currentSpeed:              3;
  /** @brief Indicates NCQ streaming support. */
  uint16_t ncqStreaming:              1;
  /** @brief Indicates NCQ queue-management support. */
  uint16_t ncqQueueManagement:        1;
  /** @brief Indicates NCQ receive/send support. */
  uint16_t ncqReceiveSend:            1;
  /** @brief Indicates DEVSLP-to-reduced-power-state support. */
  uint16_t devslpToReducedPowerState: 1;
  /** @brief Reserved bits. */
  uint16_t reserved1:                 8;
} __attribute__((packed)) S_ATAIdentifySerialAtaCapabilities;

/** @brief ATA IDENTIFY DEVICE Serial ATA supported features. */
typedef struct
{
  /** @brief Set to zero bit. */
  uint16_t setTo0:                    1;
  /** @brief Indicates non-zero data-buffer offset support. */
  uint16_t nonZeroOffsets:               1;
  /** @brief Indicates automatic DMA setup activation support. */
  uint16_t dmaSetupAutoActivate:         1;
  /** @brief Indicates device-initiated power management support. */
  uint16_t dipm:                         1;
  /** @brief Indicates in-order data delivery support. */
  uint16_t inOrderData:                  1;
  /** @brief Indicates hardware feature-control support. */
  uint16_t hardwareFeatureControl:       1;
  /** @brief Indicates software-settings preservation support. */
  uint16_t softwareSettingsPreservation: 1;
  /** @brief Indicates NCQ autosense support. */
  uint16_t ncqAutosense:                 1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                    8;
} __attribute__((packed)) S_ATAIdentifySerialAtaFeaturesSupported;

/** @brief ATA IDENTIFY DEVICE Serial ATA enabled features. */
typedef struct
{
  /** @brief Set to zero bit. */
  uint16_t setTo0:                       1;
  /** @brief Indicates non-zero data-buffer offsets are enabled. */
  uint16_t nonZeroOffsets:               1;
  /** @brief Indicates automatic DMA setup activation is enabled. */
  uint16_t dmaSetupAutoActivate:         1;
  /** @brief Indicates device-initiated power management is enabled. */
  uint16_t dipm:                         1;
  /** @brief Indicates in-order data delivery is enabled. */
  uint16_t inOrderData:                  1;
  /** @brief Indicates hardware feature control is enabled. */
  uint16_t hardwareFeatureControl:       1;
  /** @brief Indicates software-settings preservation is enabled. */
  uint16_t softwareSettingsPreservation: 1;
  /** @brief Indicates device automatic power saving is enabled. */
  uint16_t deviceAutoPS:                 1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                    8;
} __attribute__((packed)) S_ATAIdentifySerialAtaFeaturesEnabled;

/** @brief ATA IDENTIFY DEVICE command set support. */
typedef struct
{
  /** @brief Indicates that SMART commands are supported. */
  uint16_t smartCommandsSupported:           1;
  /** @brief Indicates that security mode is supported. */
  uint16_t securityModeSupported:            1;
  /** @brief Obsolete bit */
  uint16_t obsolete0:                        1;
  /** @brief Set to 1 */
  uint16_t setTo1:                           1;
  /** @brief Set to 0 */
  uint16_t setTo0:                           1;
  /** @brief Indicates that the volatile write cache is supported. */
  uint16_t volatileWriteCacheSupported:      1;
  /** @brief Indicates that the read look-ahead is supported. */
  uint16_t readLookaheadSupported:           1;
  /** @brief Obsolete bits */
  uint16_t obsolete1:                        2;
  /** @brief Set to 0 */
  uint16_t setTo01:                          1;
  /** @brief Obsolete bits */
  uint16_t obsolete2:                        2;
  /** @brief Indicates that the write buffer is supported. */
  uint16_t writeBufferSupported:             1;
  /** @brief Indicates that the read buffer is supported. */
  uint16_t readBufferSupported:              1;
  /** @brief Indicates that the NOP command is supported. */
  uint16_t nopSupported:                     1;
  /** @brief Obsolete bits */
  uint16_t obsolete3:                        1;
  /** @brief Indicates that the download microcode command is supported. */
  uint16_t downloadMicrocodeSupported:       1;
  /** @brief Obsolete bit */
  uint16_t obsolete4:                        1;
  /** @brief Reserved for CFA */
  uint16_t reservedForCFA:                   1;
  /** @brief Indicates that APM is supported. */
  uint16_t apmSupported:                     1;
  /** @brief Obsolete bit */
  uint16_t obsolete5:                        1;
  /** @brief Indicates that the PUIS is supported. */
  uint16_t puisSupported:                    1;
  /** @brief Indicates that the SET FEATURES command is supported. */
  uint16_t setFeatureSupported:              1;
  /** @brief Obsolete bits */
  uint16_t obsolete6:                        3;
  /** @brief Indicates that the 48-bit address feature is supported. */
  uint16_t address48BitSupported:            1;
  /** @brief Obsolete bit */
  uint16_t obsolete7:                        1;
  /** @brief Set to 1 */
  uint16_t setTo10:                          1;
  /** @brief Indicates that the flush cache extended command is supported. */
  uint16_t flushCacheExtSupported:           1;
  /** @brief Set to 1 */
  uint16_t setTo11:                          1;
  /** @brief Set to 0 */
  uint16_t setTo02:                          1;
  /** @brief Indicates that the SMART error log is supported. */
  uint16_t smartErrorLogSupported:           1;
  /** @brief Indicates that the SMART self-test is supported. */
  uint16_t smartSelfTestSupported:           1;
  /** @brief Reserved bit. */
  uint16_t reserved0:                        1;
  /** @brief Obsolete bit. */
  uint16_t obsolete8:                        1;
  /** @brief Indicates that the streaming feature is supported. */
  uint16_t streamingFeatureSupported:        1;
  /** @brief Indicates that the GPL feature is supported. */
  uint16_t gplFeatureSupported:              1;
  /** @brief Indicates that the write FUA is supported. */
  uint16_t writeFuaSupported:                1;
  /** @brief Obsolete bit. */
  uint16_t obsolete9:                        1;
  /** @brief Set to 1. */
  uint16_t setTo12:                          1;
  /** @brief Obsolete bits. */
  uint16_t obsolete10:                       4;
  /** @brief Indicates that the idle immediate with unload is supported. */
  uint16_t idleImmediateWithUnloadSupported: 1;
  /** @brief Set to 1. */
  uint16_t setTo13:                          1;
  /** @brief Set to 0. */
  uint16_t setTo03:                          1;
} __attribute__((packed)) S_ATAIdentifyCommandSetSupport;

/** @brief ATA IDENTIFY DEVICE command set active. */
typedef struct
{
  /** @brief Indicates that SMART commands are active. */
  uint16_t smartCommandsActive:         1;
  /** @brief Indicates that security mode is active. */
  uint16_t securityModeActive:            1;
  /** @brief Obsolete bit */
  uint16_t obsolete0:                     1;
  /** @brief Set to 1 */
  uint16_t setTo1:                        1;
  /** @brief Set to 0 */
  uint16_t setTo0:                        1;
  /** @brief Indicates that the volatile write cache is active. */
  uint16_t volatileWriteCacheActive:      1;
  /** @brief Indicates that the read look-ahead is active. */
  uint16_t readLookaheadActive:           1;
  /** @brief Obsolete bits */
  uint16_t obsolete1:                     2;
  /** @brief Set to 0 */
  uint16_t setTo01:                       1;
  /** @brief Obsolete bits */
  uint16_t obsolete2:                     2;
  /** @brief Indicates that the write buffer is active. */
  uint16_t writeBufferActive:             1;
  /** @brief Indicates that the read buffer is active. */
  uint16_t readBufferActive:              1;
  /** @brief Indicates that the NOP command is active. */
  uint16_t nopActive:                     1;
  /** @brief Obsolete bits */
  uint16_t obsolete3:                     1;
  /** @brief Indicates that the download microcode command is active. */
  uint16_t downloadMicrocodeActive:       1;
  /** @brief Obsolete bit */
  uint16_t obsolete4:                     1;
  /** @brief Reserved for CFA */
  uint16_t reservedForCFA:                1;
  /** @brief Indicates that APM is active. */
  uint16_t apmActive:                     1;
  /** @brief Obsolete bit */
  uint16_t obsolete5:                     1;
  /** @brief Indicates that the PUIS is active. */
  uint16_t puisActive:                    1;
  /** @brief Indicates that the SET FEATURES command is active. */
  uint16_t setFeatureActive:              1;
  /** @brief Obsolete bits */
  uint16_t obstacle6:                     3;
  /** @brief Indicates that the 48-bit address feature is active. */
  uint16_t address48BitActive:            1;
  /** @brief Obsolete bit */
  uint16_t obsolete7:                     1;
  /** @brief Set to 1 */
  uint16_t setTo10:                       1;
  /** @brief Indicates that the flush cache extended command is Active. */
  uint16_t flushCacheExtActive:           1;
  /** @brief Set to 1 */
  uint16_t setTo11:                       1;
  /** @brief Set to 0 */
  uint16_t setTo02:                       1;
  /** @brief Indicates that the SMART error log is Active. */
  uint16_t smartErrorLogActive:           1;
  /** @brief Indicates that the SMART self-test is Active. */
  uint16_t smartSelfTestActive:           1;
  /** @brief Reserved bit. */
  uint16_t reserved0:                     1;
  /** @brief Obsolete bit. */
  uint16_t obsolete8:                     1;
  /** @brief Indicates that the streaming feature is Active. */
  uint16_t streamingFeatureActive:        1;
  /** @brief Indicates that the GPL feature is Active. */
  uint16_t gplFeatureActive:              1;
  /** @brief Indicates that the write FUA is Active. */
  uint16_t writeFuaActive:                1;
  /** @brief Obsolete bit. */
  uint16_t obsolete9:                     1;
  /** @brief Set to 1. */
  uint16_t setTo12:                       1;
  /** @brief Obsolete bits. */
  uint16_t obsolete10:                    4;
  /** @brief Indicates that the idle immediate with unload is Active. */
  uint16_t idleImmediateWithUnloadActive: 1;
  /** @brief Set to 1. */
  uint16_t setTo13:                       1;
  /** @brief Set to 0. */
  uint16_t setTo03:                       1;
} __attribute__((packed)) S_ATAIdentifyCommandSetActive;

/** @brief ATA IDENTIFY DEVICE Ultra DMA support. */
typedef struct
{
  /** @brief Indicates that Ultra DMA mode 0 is supported. */
  uint8_t ultraDMAMode0Supported: 1;
  /** @brief Indicates that Ultra DMA mode 1 is supported. */
  uint8_t ultraDMAMode1Supported: 1;
  /** @brief Indicates that Ultra DMA mode 2 is supported. */
  uint8_t ultraDMAMode2Supported: 1;
  /** @brief Indicates that Ultra DMA mode 3 is supported. */
  uint8_t ultraDMAMode3Supported: 1;
  /** @brief Indicates that Ultra DMA mode 4 is supported. */
  uint8_t ultraDMAMode4Supported: 1;
  /** @brief Indicates that Ultra DMA mode 5 is supported. */
  uint8_t ultraDMAMode5Supported: 1;
  /** @brief Indicates that Ultra DMA mode 6 is supported. */
  uint8_t ultraDMAMode6Supported: 1;
  /** @brief Reserved bits. */
  uint8_t reserved0:              1;
} __attribute__((packed)) S_ATAIdentifyUltraDMASupport;

/** @brief ATA IDENTIFY DEVICE Ultra DMA active. */
typedef struct
{
  /** @brief Indicates that Ultra DMA mode 0 is active. */
  uint8_t ultraDMAMode0Active: 1;
  /** @brief Indicates that Ultra DMA mode 1 is active. */
  uint8_t ultraDMAMode1Active: 1;
  /** @brief Indicates that Ultra DMA mode 2 is active. */
  uint8_t ultraDMAMode2Active: 1;
  /** @brief Indicates that Ultra DMA mode 3 is active. */
  uint8_t ultraDMAMode3Active: 1;
  /** @brief Indicates that Ultra DMA mode 4 is active. */
  uint8_t ultraDMAMode4Active: 1;
  /** @brief Indicates that Ultra DMA mode 5 is active. */
  uint8_t ultraDMAMode5Active: 1;
  /** @brief Indicates that Ultra DMA mode 6 is active. */
  uint8_t ultraDMAMode6Active: 1;
  /** @brief Reserved bits. */
  uint8_t reserved0:           1;
} __attribute__((packed)) S_ATAIdentifyUltraDMAActive;

/** @brief ATA IDENTIFY DEVICE Hardware Reset Result. */
typedef struct
{
  /** @brief Set to 0 for PATA devices. */
  uint16_t setTo0ForPata:              1;
  /** @brief Device 0 device number detection method. */
  uint16_t dev0DevNumber:              2;
  /** @brief Device 0 passed diagnostics. */
  uint16_t dev0PassedDiag:             1;
  /** @brief Device 0 detected PDIAG. */
  uint16_t dev0DetectedPDIAG:          1;
  /** @brief Device 0 detected DASP. */
  uint16_t dev0DetectedDASP:           1;
  /** @brief Device 0 responded to Device 1 selection. */
  uint16_t dev0RespondToDev1Selection: 1;
  /** @brief Reserved bit. */
  uint16_t reserved0:                  1;
  /** @brief Set to 1. */
  uint16_t setTo1:                     1;
  /** @brief Device 1 device number detection method. */
  uint16_t dev1DevNumber:              2;
  /** @brief Device 1 detected PDIAG. */
  uint16_t dev1DetectedPDIAG:          1;
  /** @brief Reserved bit. */
  uint16_t reserved1:                  1;
  /** @brief CBLID is above the threshold. */
  uint16_t cblidAbove:                 1;
  /** @brief Set to 1 for PATA devices. */
  uint16_t setTo1ForPata:              1;
  /** @brief Set to 0. */
  uint16_t setTo0:                     1;
} __attribute__((packed)) S_ATAIdentifyHardwareResetResult;

/** @brief ATA IDENTIFY DEVICE physical and logical sector size. */
typedef struct
{
  /** @brief Logical sectors per physical sector. */
  uint16_t logicalSectorsPerPhysicalSector:         4;
  /** @brief Reserved bits. */
  uint16_t reserved0:                               8;
  /** @brief Indicates logical sectors longer than 256 words. */
  uint16_t sectorLongerThan256Words:         1;
  /** @brief Indicates multiple logical sectors per physical sector. */
  uint16_t multipleLogicalSectorsPerPhysicalSector: 1;
  /** @brief Set to 1. */
  uint16_t setTo1:                                  1;
  /** @brief Set to 0. */
  uint16_t setTo0:                                  1;
} __attribute__((packed)) S_ATAIdentifyPhysicalLogicalSectorSize;

/** @brief ATA IDENTIFY DEVICE command set extension supported. */
typedef struct
{
  /** @brief Obsolete bit. */
  uint16_t obsolete0:                       1;
  /** @brief Indicates that the write-read-verify command is supported. */
  uint16_t writeReadVerifySupported:        1;
  /** @brief Indicates that the write-uncorrectable command is supported. */
  uint16_t writeUncorrectableExtSupported:  1;
  /** @brief Indicates that the read-write-log DMA command is supported. */
  uint16_t readWriteLogDMAExtSupported:     1;
  /** @brief Indicates that the download microcode mode 3 is supported. */
  uint16_t downloadMicrocodeMode3Supported: 1;
  /** @brief Indicates that the free-fall control command is supported. */
  uint16_t freeFallControlSupported:        1;
  /** @brief Indicates that the sense data reporting command is supported. */
  uint16_t senseDataReportingSupported:     1;
  /** @brief Indicates that the EPC set command is supported. */
  uint16_t epcSetSupported:                 1;
  /** @brief Indicates that the accessible max address command is supported. */
  uint16_t accessibleMaxAddressSupported:   1;
  /** @brief Indicates that the DSN set command is supported. */
  uint16_t dsnSetSupported:                 1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                       4;
  /** @brief Set to 1. */
  uint16_t setTo1:                          1;
  /** @brief Set to 0. */
  uint16_t setTo0:                          1;
} __attribute__((packed)) S_ATAIdentifyCommandSetExtSupported;

/** @brief ATA IDENTIFY DEVICE command set extension active. */
typedef struct
{
  /** @brief Obsolete bit. */
  uint16_t obsolete0:                    1;
  /** @brief Indicates that the write-read-verify command is Active. */
  uint16_t writeReadVerifyActive:        1;
  /** @brief Indicates that the write-uncorrectable command is Active. */
  uint16_t writeUncorrectableExtActive:  1;
  /** @brief Indicates that the read-write-log DMA command is Active. */
  uint16_t readWriteLogDMAExtActive:     1;
  /** @brief Indicates that the download microcode mode 3 is Active. */
  uint16_t downloadMicrocodeMode3Active: 1;
  /** @brief Indicates that the free-fall control command is Active. */
  uint16_t freeFallControlActive:        1;
  /** @brief Indicates that the sense data reporting command is Active. */
  uint16_t senseDataReportingActive:     1;
  /** @brief Indicates that the EPC set command is Active. */
  uint16_t epcSetActive:                 1;
  /** @brief Indicates that the accessible max address command is Active. */
  uint16_t accessibleMaxAddressActive:   1;
  /** @brief Indicates that the DSN set command is Active. */
  uint16_t dsnSetActive:                 1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                    4;
  /** @brief Set to 1. */
  uint16_t setTo1:                       1;
  /** @brief Set to 0. */
  uint16_t setTo0:                       1;
} __attribute__((packed)) S_ATAIdentifyCommandSetExtActive;

/** @brief ATA IDENTIFY DEVICE security status. */
typedef struct
{
  /** @brief Indicates ATA security support. */
  uint16_t securitySupported:              1;
  /** @brief Indicates ATA security is enabled. */
  uint16_t securityEnabled:                1;
  /** @brief Indicates the device is security locked. */
  uint16_t securityLocked:                 1;
  /** @brief Indicates the security state is frozen. */
  uint16_t securityFrozen:                 1;
  /** @brief Indicates the security count has expired. */
  uint16_t securityCountExpired:           1;
  /** @brief Indicates enhanced security erase support. */
  uint16_t enhancedSecurityEraseSupported: 1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                      2;
  /** @brief Security level. */
  uint16_t securityLevel:                  1;
  /** @brief Reserved bits. */
  uint16_t reserved1:                      7;
} __attribute__((packed)) S_ATAIdentifySecurityStatus;

/** @brief ATA IDENTIFY DEVICE CFA power mode 1 capabilities. */
typedef struct
{
  /** @brief Maximum current draw in milliamperes. */
  uint16_t maximumCurrentInMA:       12;
  /** @brief Indicates CFA power mode 1 is disabled. */
  uint16_t cfaPowerMode1Disabled:     1;
  /** @brief Indicates CFA power mode 1 is required. */
  uint16_t cfaPowerMode1Required:     1;
  /** @brief Reserved bit. */
  uint16_t reserved0:                 1;
  /** @brief Indicates word 160 support. */
  uint16_t word160Supported:          1;
} __attribute__((packed)) S_ATAIdentifyCfaPowerMode1;

/** @brief ATA IDENTIFY DEVICE data set management features. */
typedef struct
{
  /** @brief Indicates TRIM support. */
  uint16_t supportsTrim: 1;
  /** @brief Reserved bits. */
  uint16_t reserved0:    15;
} __attribute__((packed)) S_ATAIdentifyDataSetManagementFeature;

/** @brief ATA IDENTIFY DEVICE SCT command transport capabilities. */
typedef struct
{
  /** @brief Indicates SCT command transport support. */
  uint16_t supported:                     1;
  /** @brief Obsolete bit. */
  uint16_t obsolete:                      1;
  /** @brief Indicates WRITE SAME support. */
  uint16_t writeSameSupported:            1;
  /** @brief Indicates error recovery control support. */
  uint16_t errorRecoveryControlSupported: 1;
  /** @brief Indicates feature control support. */
  uint16_t featureControlSupported:       1;
  /** @brief Indicates data tables support. */
  uint16_t dataTablesSupported:           1;
  /** @brief Reserved bits. */
  uint16_t reserved0:                     6;
  /** @brief Vendor-specific SCT capabilities. */
  uint16_t vendorSpecific:                4;
} __attribute__((packed)) S_ATAIdentifySctCommandTransport;

/** @brief ATA IDENTIFY DEVICE logical-to-physical block alignment. */
typedef struct
{
  /** @brief Logical-sector alignment within a physical sector. */
  uint16_t alignmentOfLogicalWithinPhysical: 14;
  /** @brief Set to 1. */
  uint16_t setTo1:                           1;
  /** @brief Set to 0. */
  uint16_t setTo0:                           1;
} __attribute__((packed))S_ATAIdentifyBlockAlignment;

/** @brief ATA IDENTIFY DEVICE transport major version and type. */
typedef struct
{
  union
  {
    struct
    {
      /** @brief ATA8-APT version. */
      uint16_t ata8APTVersion :   1;
      /** @brief ATA/ATAPI-7 version. */
      uint16_t ataAtapi7Version : 1;
      /** @brief Reserved bits. */
      uint16_t reserved0:         6;
    } __attribute__((packed));

    struct
    {
      /** @brief ATA8-AST version. */
      uint16_t ata8ASTVersion :  1;
      /** @brief SATA 1.0 version. */
      uint16_t sata1aVersion :   1;
      /** @brief SATA 2.0 version. */
      uint16_t sata2Version :    1;
      /** @brief SATA 2.5 version. */
      uint16_t sata25Version :   1;
      /** @brief SATA 2.6 version. */
      uint16_t sata26Version :   1;
      /** @brief SATA 3.0 version. */
      uint16_t sata3Version :    1;
      /** @brief SATA 3.1 version. */
      uint16_t sata31Version:    1;
      /** @brief Reserved bits. */
      uint16_t reserved1:        1;
    } __attribute__((packed));
  } __attribute__((packed));
  /** @brief Transport type. */
  uint16_t transportTypeSate: 4;
} __attribute__((packed)) S_ATAIdentifyTransportMajorVersion;

/** @brief ATA IDENTIFY DEVICE data returned by an ATA device. */
typedef struct
{
  /** @brief General device configuration. */
  S_ATAIdentifyGeneralConfiguration generalConfiguration;
  /** @brief Obsolete word. */
  uint16_t obsolete0;
  /** @brief Device-specific configuration. */
  uint16_t specificConfiguration;
  /** @brief Obsolete word. */
  uint16_t obsolete1;
  /** @brief Retired words. */
  uint16_t retired0[2];
  /** @brief Obsolete word. */
  uint16_t obsolete2;
  /** @brief Reserved for CFA word. */
  uint16_t reservedCFA[2];
  /** @brief Retired word. */
  uint16_t retired1;
  /** @brief Device serial number. */
  uint16_t serialNumber[10];
  /** @brief Retired words. */
  uint16_t retired2[2];
  /** @brief Obsolete word. */
  uint16_t obsolete3;
  /** @brief Firmware revision. */
  uint8_t firmwareRevision[8];
  /** @brief Model number. */
  uint16_t modelNumber[20];
  /** @brief Maximum block transfer size. */
  uint8_t maximumBlockTransfer;
  /** @brief Vendor-specific byte. */
  uint8_t vendorUnique2;
  /** @brief Trusted computing support. */
  S_ATAIdentifyTrustedComputing trustedComputing;
  /** @brief Device capabilities. */
  S_ATAIdentifyCapabilities capabilities;
  /** @brief Obsolete words. */
  uint16_t obsolete4[2];
  /** @brief Validity of the current translation fields. */
  uint16_t translationFieldsValid:       3;
  /** @brief Reserved bits. */
  uint16_t reserved3:                    5;
  /** @brief Free-fall control sensitivity. */
  uint16_t freeFallControlSensitivity:   8;
  /** @brief Obsolete words. */
  uint16_t obsolete5[5];
  /** @brief Indicates multisector setting. */
  S_ATAIdentifyMultiSectorSetting multiSectorSetting;
  /** @brief Number of user-addressable sectors. */
  uint32_t numberOfLBA28Sectors;
  /** @brief Obsolete word. */
  uint16_t obsolete6;
  /** @brief Multiword DMA settings. */
  S_ATAIdentifyMultiwordDMASettings multiwordDMASettings;
  /** @brief Advanced PIO modes. */
  uint16_t advancedPIOMode3Supported: 1;
  /** @brief Advanced PIO modes. */
  uint16_t advancedPIOMode4Supported: 1;
  /** @brief Reserved bits. */
  uint16_t reserved4:                 14;
  /** @brief Minimum multiword DMA transfer cycle time. */
  uint16_t minimumMWXferCycleTime;
  /** @brief Recommended multiword DMA transfer cycle time. */
  uint16_t recommendedMWXferCycleTime;
  /** @brief Minimum PIO transfer cycle time. */
  uint16_t minimumPIOCycleTime;
  /** @brief Minimum PIO transfer cycle time with IORDY. */
  uint16_t minimumPIOCycleTimeIORDY;
  /** @brief Additional supported features. */
  S_ATAIdentifyAdditionalSupported additionalSupported;
  /** @brief Reserved words. */
  uint16_t reserved5[5];
  /** @brief Command queue depth. */
  uint16_t queueDepth: 5;
  /** @brief Reserved bits. */
  uint16_t reserved6:  11;
  /** @brief Serial ATA capabilities. */
  S_ATAIdentifySerialAtaCapabilities serialAtaCapabilities;
  /** @brief Serial ATA features supported by the device. */
  S_ATAIdentifySerialAtaFeaturesSupported serialAtaFeaturesSupported;
  /** @brief Serial ATA features enabled by the device. */
  S_ATAIdentifySerialAtaFeaturesEnabled serialAtaFeaturesEnabled;
  /** @brief Major ATA revision. */
  uint16_t majorRevision;
  /** @brief Minor ATA revision. */
  uint16_t minorRevision;
  /** @brief Supported ATA command-set bits. */
  S_ATAIdentifyCommandSetSupport commandSetSupport;
  /** @brief Enabled ATA command-set bits. */
  S_ATAIdentifyCommandSetActive commandSetActive;
  /** @brief Supported Ultra DMA modes. */
  S_ATAIdentifyUltraDMASupport ultraDMASupport;
  /** @brief Active Ultra DMA mode. */
  S_ATAIdentifyUltraDMAActive ultraDMAActive;
  /** @brief Normal security erase time in seconds. */
  uint16_t normalSecurityEraseUnitTimeRequired:     15;
  /** @brief Indicates extended normal erase time reporting. */
  uint16_t normalSecurityEraseUnitExtendedReported:  1;
  /** @brief Enhanced security erase time in seconds. */
  uint16_t enhancedSecurityEraseUnitTimeRequired:   15;
  /** @brief Indicates extended enhanced erase time reporting. */
  uint16_t enhancedSecurityEraseUnitExtendedReported: 1;
  /** @brief Current advanced power management level. */
  uint8_t currentAPMLevel;
  /** @brief Reserved byte. */
  uint8_t reserved7;
  /** @brief Master password identifier. */
  uint16_t masterPasswordID;
  /** @brief Hardware reset result. */
  S_ATAIdentifyHardwareResetResult hardwareResetResult;
  /** @brief Obsolete word. */
  uint16_t obsolete7;
  /** @brief Minimum streaming request size. */
  uint16_t streamMinRequestSize;
  /** @brief Streaming DMA transfer time. */
  uint16_t streamingTransferTimeDMA;
  /** @brief Streaming DMA/PIO access latency. */
  uint16_t streamingAccessLatencyDMAPIO;
  /** @brief Streaming performance granularity. */
  uint32_t streamingPerfGranularity;
  /** @brief Number of 48-bit LBA sectors. */
  uint64_t numberOfLBA48Sectors;
  /** @brief Streaming transfer time. */
  uint16_t streamingTransferTime;
  /** @brief Data set management capabilities. */
  uint16_t dsmCap;
  /** @brief Physical and logical sector size information. */
  S_ATAIdentifyPhysicalLogicalSectorSize physicalLogicalSectorSize;
  /** @brief Inter-seek delay. */
  uint16_t interSeekDelay;
  /** @brief World Wide Name. */
  uint16_t worldWideName[4];
  /** @brief Reserved words for the 128-bit World Wide Name. */
  uint16_t reservedForWorldWideName128[4];
  /** @brief Reserved word for the T13 technical report. */
  uint16_t reservedForTLCTechnicalReport;
  /** @brief Number of words per logical sector. */
  uint32_t wordsPerLogicalSector;
  /** @brief Extended ATA command sets supported by the device. */
  S_ATAIdentifyCommandSetExtSupported commandSetSupportExt;
  /** @brief Extended ATA command sets enabled by the device. */
  S_ATAIdentifyCommandSetExtActive commandSetActiveExt;
  /** @brief Reserved words for expanded command support. */
  uint16_t reservedForExpandedSupportAndActive[6];
  /** @brief Obsolete word. */
  uint16_t obsolete8;
  /** @brief ATA security status. */
  S_ATAIdentifySecurityStatus securityStatus;
  /** @brief Reserved vendor specific words. */
  uint16_t reservedVendor[31];
  /** @brief CFA power mode 1 capabilities. */
  S_ATAIdentifyCfaPowerMode1 cfaPowerMode1;
  /** @brief Reserved CFA words. */
  uint16_t reservedForCFAWord[7];
  /** @brief Nominal media form factor. */
  uint16_t nominalFormFactor: 4;
  /** @brief Reserved bits. */
  uint16_t reserved8:        12;
  /** @brief Data set management features. */
  S_ATAIdentifyDataSetManagementFeature dataSetManagementFeature;
  /** @brief Additional product identifier. */
  uint16_t additionalProductID[4];
  /** @brief Reserved words. */
  uint16_t reserved9[2];
  /** @brief Current media serial number. */
  uint16_t currentMediaSerialNumber[30];
  /** @brief SCT command transport capabilities. */
  S_ATAIdentifySctCommandTransport sctCommandTransport;
  /** @brief Reserved words. */
  uint16_t reserved10[2];
  /** @brief Logical-to-physical block alignment. */
  S_ATAIdentifyBlockAlignment blockAlignment;
  /** @brief Write-read-verify sector count for mode 3. */
  uint16_t writeReadVerifySectorCountMode3Only[2];
  /** @brief Write-read-verify sector count for mode 2. */
  uint16_t writeReadVerifySectorCountMode2Only[2];
  /** @brief Obsolete words. */
  uint16_t obsolete9[3];
  /** @brief Nominal media rotation rate. */
  uint16_t nominalMediaRotationRate;
  /** @brief Reserved word. */
  uint16_t reserved11;
  /** @brief Obsolete words. */
  uint16_t obsolete10;
  /** @brief Write-read-verify sector count for the current mode. */
  uint8_t writeReadVerifySectorCountMode;
  /** @brief Reserved byte. */
  uint8_t reserved12;
  /** @brief Reserved word. */
  uint16_t reserved13;
  /** @brief ATA transport major version. */
  S_ATAIdentifyTransportMajorVersion transportMajorVersion;
  /** @brief Transport minor version. */
  uint16_t transportMinorVersion;
  /** @brief Reserved words. */
  uint16_t reserved14[6];
  /** @brief Extended number of LBA28 sectors. */
  uint64_t extendedNumberOfLBA28Sectors;
  /** @brief Minimum blocks per download microcode mode 03. */
  uint16_t minBlocksPerDownloadMicrocodeMode03;
  /** @brief Maximum blocks per download microcode mode 03. */
  uint16_t maxBlocksPerDownloadMicrocodeMode03;
  /** @brief Reserved words. */
  uint16_t reserved15[19];
  /** @brief IDENTIFY DEVICE checksum validity indicator. */
  uint8_t checksumValidity;
  /** @brief IDENTIFY DEVICE checksum. */
  uint8_t checksum;
} __attribute__((packed)) S_ATAIdentify;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE general configuration.
 */
typedef struct
{
  /** @brief Packet Size: 00 = 12-byte command packet, 01 = 16-byte packet. */
  uint16_t packetSize              : 2;
  /** @brief Indicates if the device has an incomplete response. */
  uint16_t incompleteResponse      : 1;
  /** @brief Reserved bits. */
  uint16_t reserved0               : 2;
  /** @brief DRQ assertion acceleration timing style. */
  uint16_t drqResponse             : 2;
  /** @brief Obsolete bit. */
  uint16_t obsolete                : 1;
  /** @brief Command packet device type: 0x05 = CD-ROM/DVD/BD, 0x01 = Tape. */
  uint16_t commandPacketDeviceType : 5;
  /** @brief Reserved bit. */
  uint16_t reserved1               : 1;
  /** @brief Protocol type: Must be 0x02 for ATAPI protocol device profile. */
  uint16_t protocolType            : 2;
} __attribute__((packed)) S_ATAPIIdentifyGeneralConfiguration;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE core device capabilities (Words 49-50).
 */
typedef struct
{
  /** @brief Vendor unique parameters logging byte space. */
  uint8_t  vendorUnique;
  /** @brief Indicates if Direct Memory Access (DMA) is supported. */
  uint8_t  dmaSupported            : 1;
  /** @brief Indicates if Logical Block Addressing (LBA) is supported. */
  uint8_t  lbaSupported            : 1;
  /** @brief Indicates if the IORDY signal line can be disabled. */
  uint8_t  iordyDisable            : 1;
  /** @brief Indicates if the IORDY signal line tracking is supported. */
  uint8_t  iordySupported          : 1;
  /** @brief Reserved bit. */
  uint8_t  reserved0               : 1;
  /** @brief Indicates if the standby timer feature set is supported. */
  uint8_t  standbyTimerSupport     : 1;
  /** @brief Reserved bits. */
  uint8_t  reserved1               : 2;
  /** @brief Standby timer value capabilities marker. */
  uint16_t standbyTimerValue       : 1;
  /** @brief Obsolete capability tracking bit. */
  uint16_t obsolete0               : 1;
  /** @brief Reserved capabilities flags. */
  uint16_t reserved2               : 12;
  /** @brief Fixed integrity validation flag bit set to 1. */
  uint16_t setTo1                  : 1;
  /** @brief Fixed integrity validation flag bit set to 0. */
  uint16_t setTo0                  : 1;
} __attribute__((packed)) S_ATAPIIdentifyCapabilities;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE multiword DMA settings (Word 62).
 */
typedef struct
{
  /** @brief Ultra DMA mode 0 is supported. */
  uint16_t ultraDMAMode0Supported     : 1;
  /** @brief Ultra DMA mode 1 is supported. */
  uint16_t ultraDMAMode1Supported     : 1;
  /** @brief Ultra DMA mode 2 is supported. */
  uint16_t ultraDMAMode2Supported     : 1;
  /** @brief Ultra DMA mode 3 is supported. */
  uint16_t ultraDMAMode3Supported     : 1;
  /** @brief Ultra DMA mode 4 is supported. */
  uint16_t ultraDMAMode4Supported     : 1;
  /** @brief Ultra DMA mode 5 is supported. */
  uint16_t ultraDMAMode5Supported     : 1;
  /** @brief Ultra DMA mode 6 is supported. */
  uint16_t ultraDMAMode6Supported     : 1;
  /** @brief Multiword DMA mode 0 is supported. */
  uint16_t multiwordDMAMode0Supported : 1;
  /** @brief Multiword DMA mode 1 is supported. */
  uint16_t multiwordDMAMode1Supported : 1;
  /** @brief Multiword DMA mode 2 is supported. */
  uint16_t multiwordDMAMode2Supported : 1;
  /** @brief DMA is supported. */
  uint16_t dmaSupported               : 1;
  /** @brief Reserved bits. */
  uint16_t reserved0                  : 4;
  /** @brief DMA request required flag. */
  uint16_t dmadirRequired             : 1;
   /** @brief Ultra DMA mode 0 is supported. */
  uint16_t ultraDMAMode0BelowSupported     : 1;
  /** @brief Ultra DMA mode 1 is supported. */
  uint16_t ultraDMAMode1BelowSupported     : 1;
  /** @brief Ultra DMA mode 2 is supported. */
  uint16_t ultraDMAMode2BelowSupported     : 1;
  /** @brief Reserved bits. */
  uint16_t reserved1                  : 5;
  /** @brief Multiword DMA mode 0 is selected. */
  uint16_t multiwordDMAMode0Selected  : 1;
  /** @brief Multiword DMA mode 1 is selected. */
  uint16_t multiwordDMAMode1Selected  : 1;
  /** @brief Multiword DMA mode 2 is selected. */
  uint16_t multiwordDMAMode2Selected  : 1;
  /** @brief Reserved bits. */
  uint16_t reserved2                  : 5;
} __attribute__((packed)) S_ATAPIIdentifyMultiwordDMASettings;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE Serial ATA capabilities (Word 76-77).
 */
typedef struct
{
  /** @brief Fixed tracking check flag bit set to zero. */
  uint16_t setTo0                     : 1;
  /** @brief SATA Gen1 signaling speed (1.5 Gbps) is supported. */
  uint16_t sataGen1                   : 1;
  /** @brief SATA Gen2 signaling speed (3.0 Gbps) is supported. */
  uint16_t sataGen2                   : 1;
  /** @brief SATA Gen3 signaling speed (6.0 Gbps) is supported. */
  uint16_t sataGen3                   : 1;
  /** @brief Reserved bits. */
  uint16_t reserved0                  : 4;
  /** @brief Native Command Queuing (NCQ) support indicator flag. */
  uint16_t ncq                        : 1;
  /** @brief Host-Initiated Power Management (HIPM) support flag. */
  uint16_t hipm                       : 1;
  /** @brief Hardware PHY event logging counters support flag. */
  uint16_t phyEvents                  : 1;
  /** @brief NCQ Unload feature command support flag. */
  uint16_t ncqUnload                  : 1;
  /** @brief NCQ Priority data queuing support flag. */
  uint16_t ncqPriority                : 1;
  /** @brief Host Automatic Power-Saving configuration support flag. */
  uint16_t hostAutoPS                 : 1;
  /** @brief Device Automatic Power-Saving configuration support flag. */
  uint16_t deviceAutoPS               : 1;
  /** @brief Read Log DMA feature execution support flag. */
  uint16_t readLogDMA                 : 1;
  /** @brief Set to 1 flag. */
  uint16_t setto01                    : 1;
  /** @brief Negotiated speed. */
  uint16_t negiotiatedSpeed           : 3;
  /** @brief Device Attention support flag. */
  uint16_t deviceAttentionSupport     : 1;
  /** @brief Host Environment Detection support flag. */
  uint16_t hostEnvDetectSupport       : 1;
  /** @brief Reserved bits. */
  uint16_t reserved1                  : 10;
} __attribute__((packed)) S_ATAPIIdentifySerialAtaCapabilities;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE Serial ATA features supported (Word 78).
 */
typedef struct
{
  /** @brief Fixed tracking check flag bit set to zero. */
  uint16_t setTo0                       : 1;
  /** @brief Non-zero data-buffer offset DMA configuration support flag. */
  uint16_t nonZeroOffsets               : 1;
  /** @brief Automatic DMA Setup FIS activation hardware support flag. */
  uint16_t dmaSetupAutoActivate         : 1;
  /** @brief Device-Initiated Power Management (DIPM) support flag. */
  uint16_t dipm                         : 1;
  /** @brief In-order data delivery data routing optimization support flag. */
  uint16_t inOrderData                  : 1;
  /** @brief Asynchronous notification support flag. */
  uint16_t asyncSupported               : 1;
  /** @brief Software-settings preservation handshake support flag. */
  uint16_t softwareSettingsPreservation : 1;
  /** @brief NCQ Autosense hardware feature support flag. */
  uint16_t ncqAutosense                 : 1;
  /** @brief Reserved bits. */
  uint16_t reserved0                    : 8;
} __attribute__((packed)) S_ATAPIIdentifySerialAtaFeaturesSupported;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE Serial ATA features enabled (Word 79).
 */
typedef struct
{
  /** @brief Fixed tracking check flag bit set to zero. */
  uint16_t setTo0                       : 1;
  /** @brief Non-zero data-buffer offset DMA operations are active. */
  uint16_t nonZeroOffsets               : 1;
  /** @brief Automatic DMA Setup FIS activation hardware is active. */
  uint16_t dmaSetupAutoActivate         : 1;
  /** @brief Device-Initiated Power Management (DIPM) features are active. */
  uint16_t dipm                         : 1;
  /** @brief In-order data delivery parameters are actively enforced. */
  uint16_t inOrderData                  : 1;
  /** @brief Asynchronous notification enabled. */
  uint16_t asyncEnabled                 : 1;
  /** @brief Software-settings preservation configurations are active. */
  uint16_t softwareSettingsPreservation : 1;
  /** @brief Device automatic power-saving state transformations are active. */
  uint16_t deviceAutoPS                 : 1;
  /** @brief Reserved bits. */
  uint16_t reserved0                    : 8;
} __attribute__((packed)) S_ATAPIIdentifySerialAtaFeaturesEnabled;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE command set support (Words 82-84).
 */
typedef struct
{
  /** @brief S.M.A.R.T. command extensions are supported. */
  uint16_t smartCommandsSupported      : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete0                   : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete1                   : 1;
  /** @brief The Power Management feature set supported. */
  uint16_t powerManagementSetSupported : 1;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo10                     : 1;
  /** @brief Volatile write-cache supported. */
  uint16_t volatileWriteCacheSupported : 1;
  /** @brief Read look-ahead caching acceleration features are supported. */
  uint16_t readLookaheadSupported      : 1;
  /** @brief Obsolete tracking configuration bits. */
  uint16_t obsolete2                   : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete3                   : 1;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo11                     : 1;
  /** @brief Fixed identification integrity validation bit set to 0. */
  uint16_t setTo01                     : 1;
  /** @brief Obsolete tracking configuration bits. */
  uint16_t obsolete4                   : 1;
  /** @brief Legacy WRITE BUFFER execution command is supported. */
  uint16_t writeBufferSupported       : 1;
  /** @brief Legacy READ BUFFER execution command is supported. */
  uint16_t readBufferSupported        : 1;
  /** @brief ATA non-operational NOP command is supported. */
  uint16_t nopSupported               : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete5                  : 1;
  /** @brief DOWNLOAD MICROCODE data execution feature set is supported. */
  uint16_t downloadMicrocodeSupported : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved0                  : 2;
  /** @brief Advanced Power Management (APM) optimization profiles supported. */
  uint16_t apmSupported               : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete6                  : 1;
  /** @brief Power-Up In Standby (PUIS) spin-up handshake is supported. */
  uint16_t puisSupported              : 1;
  /** @brief SET FEATURES hardware sub-register adjustment command supported. */
  uint16_t setFeatureSupported        : 1;
  /** @brief Obsolete tracking configuration bits. */
  uint16_t obsolete7                  : 3;
  /** @brief Advanced 48-bit address capacity command feature set supported. */
  uint16_t address48BitSupported      : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete8                  : 1;
  /** @brief FLUSH CACHE EXTENDED memory protection sync command supported. */
  uint16_t flushCacheExtSupported     : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved1                  : 1;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo12                    : 1;
  /** @brief Fixed identification integrity validation bit set to 0. */
  uint16_t setTo02                    : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved2                  : 5;
  /** @brief General Purpose Logging (GPL) tracking tables are supported. */
  uint16_t gplFeatureSupported        : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved3                  : 2;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo13                    : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved4                  : 5;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo14                    : 1;
  /** @brief Fixed identification integrity validation bit set to 0. */
  uint16_t setTo03                    : 1;
} __attribute__((packed)) S_ATAPIIdentifyCommandSetSupport;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE command set active (Words 85-87).
 */
typedef struct
{
  /** @brief S.M.A.R.T. command extensions are active. */
  uint16_t smartCommandsActive         : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete0                   : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete1                   : 1;
  /** @brief The Power Management feature set active. */
  uint16_t powerManagementSetActive    : 1;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo10                     : 1;
  /** @brief Volatile write-cache active. */
  uint16_t volatileWriteCacheActive    : 1;
  /** @brief Read look-ahead caching acceleration features are active. */
  uint16_t readLookaheadActive         : 1;
  /** @brief Obsolete tracking configuration bits. */
  uint16_t obsolete2                   : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete3                   : 1;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo11                     : 1;
  /** @brief Fixed identification integrity validation bit set to 0. */
  uint16_t setTo01                     : 1;
  /** @brief Obsolete tracking configuration bits. */
  uint16_t obsolete4                   : 1;
  /** @brief Legacy WRITE BUFFER execution command is active. */
  uint16_t writeBufferActive          : 1;
  /** @brief Legacy READ BUFFER execution command is active. */
  uint16_t readBufferActive           : 1;
  /** @brief ATA non-operational NOP command is active. */
  uint16_t nopActive                  : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete5                  : 1;
  /** @brief DOWNLOAD MICROCODE data execution feature set is active. */
  uint16_t downloadMicrocodeActive    : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved0                  : 2;
  /** @brief Advanced Power Management (APM) optimization profiles active. */
  uint16_t apmActive                  : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete6                  : 1;
  /** @brief Power-Up In Standby (PUIS) spin-up handshake is active. */
  uint16_t puisActive                 : 1;
  /** @brief SET FEATURES hardware sub-register adjustment command active. */
  uint16_t setFeatureActive           : 1;
  /** @brief Obsolete tracking configuration bits. */
  uint16_t obsolete7                  : 3;
  /** @brief Advanced 48-bit address capacity command feature set active. */
  uint16_t address48BitActive         : 1;
  /** @brief Obsolete tracking configuration bit. */
  uint16_t obsolete8                  : 1;
  /** @brief FLUSH CACHE EXTENDED memory protection sync command active. */
  uint16_t flushCacheExtActive        : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved1                  : 1;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo12                    : 1;
  /** @brief Fixed identification integrity validation bit set to 0. */
  uint16_t setTo02                    : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved2                  : 5;
  /** @brief General Purpose Logging (GPL) tracking tables are active. */
  uint16_t gplFeatureActive           : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved3                  : 2;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo13                    : 1;
  /** @brief Reserved expansion flags. */
  uint16_t reserved4                  : 5;
  /** @brief Fixed identification integrity validation bit set to 1. */
  uint16_t setTo14                    : 1;
  /** @brief Fixed identification integrity validation bit set to 0. */
  uint16_t setTo03                    : 1;
} __attribute__((packed)) S_ATAPIIdentifyCommandSetActive;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE Ultra DMA settings (Word 88).
 */
typedef struct
{
  /** @brief Ultra DMA mode 0 speed capability is supported. */
  uint8_t  ultraDMAMode0Supported     : 1;
  /** @brief Ultra DMA mode 1 speed capability is supported. */
  uint8_t  ultraDMAMode1Supported     : 1;
  /** @brief Ultra DMA mode 2 speed capability is supported. */
  uint8_t  ultraDMAMode2Supported     : 1;
  /** @brief Ultra DMA mode 3 speed capability is supported. */
  uint8_t  ultraDMAMode3Supported     : 1;
  /** @brief Ultra DMA mode 4 speed capability is supported. */
  uint8_t  ultraDMAMode4Supported     : 1;
  /** @brief Ultra DMA mode 5 speed capability is supported. */
  uint8_t  ultraDMAMode5Supported     : 1;
  /** @brief Ultra DMA mode 6 speed capability is supported. */
  uint8_t  ultraDMAMode6Supported     : 1;
  /** @brief Reserved tracking bit. */
  uint8_t  reserved0                  : 1;
  /** @brief Ultra DMA mode 0 is currently selected and active. */
  uint8_t  ultraDMAMode0Active        : 1;
  /** @brief Ultra DMA mode 1 is currently selected and active. */
  uint8_t  ultraDMAMode1Active        : 1;
  /** @brief Ultra DMA mode 2 is currently selected and active. */
  uint8_t  ultraDMAMode2Active        : 1;
  /** @brief Ultra DMA mode 3 is currently selected and active. */
  uint8_t  ultraDMAMode3Active        : 1;
  /** @brief Ultra DMA mode 4 is currently selected and active. */
  uint8_t  ultraDMAMode4Active        : 1;
  /** @brief Ultra DMA mode 5 is currently selected and active. */
  uint8_t  ultraDMAMode5Active        : 1;
  /** @brief Ultra DMA mode 6 is currently selected and active. */
  uint8_t  ultraDMAMode6Active        : 1;
  /** @brief Reserved tracking bit. */
  uint8_t  reserved1                  : 1;
} __attribute__((packed)) S_ATAPIIdentifyUltraDMASettings;

/** @brief ATA IDENTIFY DEVICE Hardware Reset Result. */
typedef struct
{
  /** @brief Set to 0 for PATA devices. */
  uint16_t setTo0ForPata:              1;
  /** @brief Device 0 device number detection method. */
  uint16_t dev0DevNumber:              2;
  /** @brief Device 0 passed diagnostics. */
  uint16_t dev0PassedDiag:             1;
  /** @brief Device 0 detected PDIAG. */
  uint16_t dev0DetectedPDIAG:          1;
  /** @brief Device 0 detected DASP. */
  uint16_t dev0DetectedDASP:           1;
  /** @brief Device 0 responded to Device 1 selection. */
  uint16_t dev0RespondToDev1Selection: 1;
  /** @brief Reserved bit. */
  uint16_t reserved0:                  1;
  /** @brief Set to 1. */
  uint16_t setTo1:                     1;
  /** @brief Device 1 device number detection method. */
  uint16_t dev1DevNumber:              2;
  /** @brief Device 1 detected PDIAG. */
  uint16_t dev1DetectedPDIAG:          1;
  /** @brief Reserved bit. */
  uint16_t reserved1:                  1;
  /** @brief CBLID is above the threshold. */
  uint16_t cblidAbove:                 1;
  /** @brief Set to 1 for PATA devices. */
  uint16_t setTo1ForPata:              1;
  /** @brief Set to 0. */
  uint16_t setTo0:                     1;
} __attribute__((packed)) S_ATAPIIdentifyHardwareResetResult;

/**
 * @brief ATAPI IDENTIFY PACKET DEVICE security status descriptor (Word 128).
 */
typedef struct
{
  /** @brief Indicates ATAPI sub-system encryption security is supported. */
  uint16_t securitySupported          : 1;
  /** @brief Indicates ATAPI sub-system encryption security is enabled. */
  uint16_t securityEnabled            : 1;
  /** @brief Indicates that the optical hardware is locked. */
  uint16_t securityLocked             : 1;
  /** @brief Indicates that the hardware security state is frozen. */
  uint16_t securityFrozen             : 1;
  /** @brief Indicates that the security execution countdown has expired. */
  uint16_t securityCountExpired       : 1;
  /** @brief Enhanced security erase command feature is supported. */
  uint16_t enhancedSecurityEraseSupported : 1;
  /** @brief Reserved bits. */
  uint16_t reserved0                  : 2;
  /** @brief Active security authorization tracking level flag. */
  uint16_t securityLevel              : 1;
  /** @brief Reserved bits. */
  uint16_t reserved1                  : 7;
} __attribute__((packed)) S_ATAPIIdentifySecurityStatus;

/**
 * @brief Structure representing ATAPI port identify data returned by an ATAPI
 * device.
 */
typedef struct
{
  /** @brief General device configuration bitmask properties (Word 0). */
  S_ATAPIIdentifyGeneralConfiguration generalConfiguration;
  /** @brief Reserved word (Word 1). */
  uint16_t reserved0;
  /** @brief Specific device internal configuration parameters (Word 2). */
  uint16_t specificConfiguration;
  /** @brief Reserved words (Word 3-9). */
  uint16_t reserved1[7];
  /** @brief Unique Device Serial Number String (Words 10-19). */
  uint16_t  serialNumber[10];
  /** @brief Reserved words (Words 20-22). */
  uint16_t reserved2[3];
  /** @brief Device Firmware Revision String (Words 23-26). */
  uint16_t firmwareRevision[4];
  /** @brief Device Model Number String (Words 27-46). */
  uint16_t modelNumber[20];
  /** @brief Reserved words (Words 47-48). */
  uint16_t reserved3[2];
  /** @brief Core hardware transport capabilities indicators (Words 49-50). */
  S_ATAPIIdentifyCapabilities capabilities;
  /** @brief Obsolete words (Words 51-52). */
  uint16_t obsolete0[2];
  /** @brief Validity masks for advanced extension parameter (Word 53). */
  uint16_t translationFieldsValid;
  /** @brief Reserved words (Words 54-61). */
  uint16_t reserved4[8];
  /** @brief DMA execution modes supported / currently active (Word 62-63). */
  S_ATAPIIdentifyMultiwordDMASettings multiwordDmaSettings;
  /** @brief Advanced PIO timing acceleration bitmask modes (Word 64). */
  uint16_t advancedPioModes;
  /** @brief Minimum Multiword DMA transfer cycle in nanoseconds (Word 65). */
  uint16_t minMultiwordDmaCycleTime;
  /** @brief Recommended Multiword DMA transfer cycle duration (Word 66). */
  uint16_t recommendedDmaCycleTime;
  /** @brief Minimum PIO cycle timing width constraint with/ flow (Word 67). */
  uint16_t minPioCycleTimeNoFlow;
  /** @brief Minimum PIO cycle loop size active IORDY flow (Word 68). */
  uint16_t minPioCycleTimeIordy;
  /** @brief Reserved words (Words 69-70). */
  uint16_t reserved5[2];
  /** @brief Obsolete words (Words 71-72). */
  uint16_t obsolete1[2];
  /** @brief Reserved words (Words 73-74). */
  uint16_t reserved6[2];
  /** @brief Obsolete word (Word 75). */
  uint16_t obsolete2;
  /** @brief Serial ATA (SATA) Specific Capabilities (Word 76). */
  S_ATAPIIdentifySerialAtaCapabilities serialAtaCapabilities;
  /** @brief Serial ATA features supported by the device (Word 78). */
  S_ATAPIIdentifySerialAtaFeaturesSupported serialAtaFeaturesSupported;
  /** @brief Serial ATA features currently active by the device (Word 79). */
  S_ATAPIIdentifySerialAtaFeaturesEnabled serialAtaFeaturesEnabled;
  /** @brief Major ATA standard revision profile mask (Word 80). */
  uint16_t majorRevision;
  /** @brief Minor ATA standard compliance sub-version tags (Word 81). */
  uint16_t minorRevision;
  /** @brief Supported ATA/ATAPI command-set bitmask fields (Words 82-84). */
  S_ATAPIIdentifyCommandSetSupport commandSetSupport;
  /** @brief Enabled ATA/ATAPI command-set bitmask fields (Words 85-87). */
  S_ATAPIIdentifyCommandSetActive commandSetEnabled;
  /** @brief Ultra DMA transfer timing modes supported/active (Word 88). */
  S_ATAPIIdentifyUltraDMASettings ultraDmaSettings;
  /** @brief Normal erase time in seconds (Word 89). */
  uint16_t normalEraseTimeSec;
  /** @brief Enhanced erase time in seconds (Word 90). */
  uint16_t enhancedEraseTimeSec;
  /** @brief Current APM level (Word 91). */
  uint16_t currentAPMLevel;
  /** @brief Master password identifier (Word 92). */
  uint16_t masterPasswordIdentifier;
  /** @brief Master/Slave hardware diagnostics result (Word 93). */
  S_ATAPIIdentifyHardwareResetResult hardwareResetResult;
  /** @brief Obsolete word (Word 94). */
  uint16_t obsolete3;
  /** @brief Reserved words (Words 95-107). */
  uint16_t reserved7[13];
  /** @brief World Wide Name (WWN) (Words 108-111). */
  uint16_t worldwideName[4];
  /** @brief Reserved words for structural storage array (Words 112-124). */
  uint16_t reserved8[13];
  /** @brief ATAPI Byte Count (Word 125). */
  uint16_t atapiByteCount;
  /** @brief Obsolete words (Words 126-128). */
  uint16_t obsolete4[3];
  /** @brief Reserved vendor unique configuration data words (Words 129-159). */
  uint16_t vendorSpecific[31];
  /** @brief Reserved words (Words 160-175). */
  uint16_t reserved9[16];
  /** @brief Reserved words (Words 176-221). */
  uint16_t reserved10[46];
  /** @brief Transport Major version number. (Word 222). */
  uint16_t transportMajorVersion;
  /** @brief Transport Minor version number. (Word 223). */
  uint16_t transportMinorVersion;
  /** @brief Reserved words (Words 224-254). */
  uint16_t reserved11[31];
  /** @brief Checksum validity indicator. */
  uint8_t checksumValidity;
  /** @brief Checksum. */
  uint8_t checksum;
} __attribute__((packed)) S_ATAPIIdentify;

/** @brief Enumerated type for AHCI port types. */
typedef enum
{
  /** @brief No port type */
  AHCI_PORT_TYPE_NONE = 0,
  /** @brief Unknown port type */
  AHCI_PORT_TYPE_UNKNOWN,
  /** @brief SATA port type */
  AHCI_PORT_TYPE_SATA,
  /** @brief SATAPI port type */
  AHCI_PORT_TYPE_SATAPI,
  /** @brief SEMB port type */
  AHCI_PORT_TYPE_SEMB,
  /** @brief PM port type */
  AHCI_PORT_TYPE_PM
} E_AHCIPortType;

/** @brief AHCI operation result. */
typedef enum
{
  /** @brief Operation successful. */
  AHCI_OP_SUCCESS = 0,
  /** @brief Operation failed. */
  AHCI_OP_ERROR   = 1,
  /** @brief Operation timed out. */
  AHCI_OP_TIMEOUT = 2,
  /** @brief Operation needs to be retried. */
  AHCI_OP_RETRY   = 3,
  /** @brief Operation aborted. */
  AHCI_OP_ABORT   = 4,
  /** @brief Operation is waiting for completion. */
  AHCI_OP_WAIT    = 5
} E_AHCIOpResult;

/** @brief Enumerated type for AHCI command types. */
typedef enum
{
  /** @brief AHCI command type for reading data. */
  AHCI_CMD_READ,
  /** @brief AHCI command type for writing data. */
  AHCI_CMD_WRITE,
  /** @brief AHCI command type for device identification. */
  AHCI_CMD_IDENTIFY,
  /** @brief AHCI command type for device reset. */
  AHCI_CMD_RESET,
  /** @brief AHCI command type for device flush. */
  AHCI_CMD_FLUSH,
  /** @brief AHCI command type for device standby. */
  AHCI_CMD_STANDBY,
  /** @brief AHCI command type for device sleep. */
  AHCI_CMD_SLEEP,
  /** @brief AHCI command type for device wakeup. */
  AHCI_CMD_WAKEUP
} E_AHCICommandType;

/** @brief Enumerated type for AHCI operations. */
typedef enum
{
  /** @brief AHCI operation for reading data. */
  AHCI_OP_READ,
  /** @brief AHCI operation for writing data. */
  AHCI_OP_WRITE,
  /** @brief AHCI operation for device identification. */
  AHCI_OP_IDENTIFY,
  /** @brief AHCI operation for device reset. */
  AHCI_OP_RESET,
  /** @brief AHCI operation for device flush. */
  AHCI_OP_FLUSH,
  /** @brief AHCI operation for device standby. */
  AHCI_OP_STANDBY,
  /** @brief AHCI operation for device sleep. */
  AHCI_OP_IDLE,
  /** @brief AHCI operation for device wakeup. */
  AHCI_OP_WAKEUP,
  /** @brief AHCI operation for device trim. */
  AHCI_OP_TRIM,
  /** @brief AHCI operation for device S.M.A.R.T. */
  AHCI_OP_SMART,
  /** @brief AHCI operation for device security lock. */
  AHCI_OP_SECURITY_SET_PASSWORD,
  /** @brief AHCI operation for device security unlock. */
  AHCI_OP_SECURITY_UNLOCK,
  /** @brief AHCI operation for device security erase prepare. */
  AHCI_OP_SECURITY_ERASE_PREPARE,
  /** @brief AHCI operation for device security erase unit. */
  AHCI_OP_SECURITY_ERASE_UNIT,
  /** @brief AHCI operation for checking power mode. */
  AHCI_OP_CHECK_POWER_MODE
} E_AHCIOperation;

/** @brief Structure representing SATA device information. */
typedef struct
{
  /** @brief Indicates if the device supports 48-bit LBA. */
  bool isLBA48Supported;
  /** @brief Indicates if the device supports DMA. */
  bool isDMASupported;
  /** @brief Total number of sectors on the device. */
  uint64_t totalSectors;
   /** @brief Size of each sector in bytes. */
  uint16_t sectorSize;
  /** @brief Model number of the device. */
  char modelNumber[41];
  /** @brief Serial number of the device. */
  char serialNumber[21];
} S_SATADevInformation;

/** @brief Structure representing SATAPI device information. */
typedef struct
{
  /** @brief Indicates if the device supports 16-byte SCSI packet. */
  bool isSCSI16Supported;
  /** @brief Indicates if the device supports DMA. */
  bool isDMASupported;
  /** @brief Indicates if the device supports Native Command Queuing. */
  bool isNativeCommandQueuingSupported;
  /** @brief Indicates if the device supports IORDY. */
  bool isIORDYSupported;
  /** @brief Indicates if the negociated speed. */
  uint8_t negociatedSpeed;
  /** @brief Model number of the device. */
  char modelNumber[41];
  /** @brief Serial number of the device. */
  char serialNumber[21];
} S_SATAPIDevInformation;

/** @brief Structure representing an AHCI command. */
typedef struct
{
  /** @brief Pointer to the async result header. */
  E_AHCIOpResult* pResult;
  /** @brief Pointer to the thread that issued the command. */
  S_KernelThread* pThread;
  /** @brief AHCI command type. */
  E_AHCICommandType type;
} S_AHCICommand;

/** @brief Structure representing an AHCI port controller. */
typedef struct S_AHCIPortControler
{
  /** @brief Pointer to the AHCI port registers */
  S_AHCIPort* pPort;

  /** @brief Lock for the port */
  S_KernelSpinlock lock;

  /** @brief Lock for the port's retry waiting queue */
  S_KernelSpinlock retryQueueLock;
  /** @brief Retry waiting threads queue */
  S_KernelQueue *pRetryWaitingList;

  /** @brief Command waiting operation table */
  S_AHCICommand commandWaitingTable[COMMAND_TABLE_COUNT];
  /** @brief Issues command tables */
  uint32_t issuedCommandTable;

  /** @brief VFS handle for the port */
  T_VFSDriver vfsHandle;

  /** @brief Pointer to the AHCI port memory */
  void* pMem;
  /** @brief Size of the AHCI port memory */
  uint32_t memorySize;
  /** @brief Physical address of the port's scratchpad memory */
  uintptr_t scratchpadPhysAddr;
  /** @brief Virtual address of the port's scratchpad memory */
  uintptr_t scratchpadVirtAddr;
  /** @brief The type of the AHCI port */
  E_AHCIPortType type;
  /** @brief Port's device information*/
  union
  {
    /** @brief SATA device information */
    S_SATADevInformation satadInfo;
    /** @brief SATAPI device information */
    S_SATAPIDevInformation satapidInfo;
  } devInfo;
} S_AHCIPortControler;

/** @brief x86 AHCI driver controller. */
typedef struct S_AHCIControler
{
  /** @brief Driver's lock */
  S_KernelSpinlock lock;
  /** @brief Pointer to the generic AHCI BAR 5 */
  void* pBARBaseAddr;
  /** @brief Pointer to the PCI device */
  S_PCIDevice* pDevice;
  /** @brief Pointer to the generic S_AHCIPortControler structure */
  S_AHCIPortControler ports[AHCI_MAX_PORTS];
  /** @brief Pointer to the next driver control structure */
  struct S_AHCIControler* pNext;
} S_AHCIControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the AHCI to ensure correctness of execution.
 *
 * @details Assert macro used by the AHCI to ensure correctness of execution.
 * Due to the critical nature of the AHCI, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define AHCI_ASSERT(COND, MSG, ERROR) {                   \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Performs the BIOS handoff for the AHCI controller.
 *
 * @details This function checks if the BIOS owns the AHCI controller and, if
 * so, requests ownership from the BIOS. It waits for the BIOS to release
 * control of the controller, and if the BIOS does not release control within a
 * specified timeout, it forces the handoff by clearing the BIOS ownership bit
 * and setting the OS ownership bit. This ensures that the operating system can
 * take control of the AHCI controller for proper operation.
 *
 * @param[in] pHostControl Pointer to the generic host control structure of the
 * AHCI controller.
 */
static void _PerformBIOSHandoff(S_GenericHostControl* pHostControl);

/**
 * @brief This function starts the command engine for a given AHCI port.
 *
 * @details The command engine is responsible for processing commands sent to
 * the AHCI port. This function sets the appropriate bits in the command and
 * status register of the port to enable the command engine and the FIS receive
 * functionality. It ensures that the port is ready to accept and process
 * commands.
 *
 * @param[in] pPort Pointer to the AHCI port structure.
 */
static void _StartAHCIPortCommandEngine(S_AHCIPort* pPort);

/**
 * @brief This function stops the command engine for a given AHCI port.
 *
 * @details The command engine is responsible for processing commands sent to
 * the AHCI port. This function clears the appropriate bits in the command and
 * status register of the port to disable the command engine and the FIS receive
 * functionality. It waits for the command engine to stop, checking the status
 * register until the command list and FIS receive bits are cleared or a timeout
 * occurs. If the command engine does not stop within the specified timeout, an
 * error is returned.
 *
 * @param[in] pPort Pointer to the AHCI port structure.
 *
 * @return The function returns a value indicating success or failure of
 * stopping the command engine.
 */
static E_Return _StopAHCIPortCommandEngine(S_AHCIPort* pPort);

/**
 * @brief Enables interrupts for a given AHCI port.
 *
 * @details This function sets the appropriate bits in the interrupt enable
 * register of the specified AHCI port to enable interrupts. This allows the
 * port to generate interrupt signals to the host controller when certain events
 * occur, such as command completion or error conditions. Enabling interrupts is
 * essential for efficient communication between the AHCI port and the operating
 * system, allowing the system to respond to events without constant polling.
 *
 * @param[in] pPort Pointer to the AHCI port structure for which interrupts
 * should be enabled.
 */
static void _EnableAHCIPortInterrupts(S_AHCIPort* pPort);

/**
 * @brief Allocates resources for a given AHCI port.
 *
 * @details This function allocates the necessary resources for the specified
 * AHCI port, such as memory buffers and command lists.
 *
 * @param[in] pDrvCtrl Pointer to the AHCI controller structure.
 * @param[in] kPortIndex The index of the port for which to allocate resources.
 *
 * @return The success state or the error code.
 */
static E_Return _AllocateAHCIPortResources(S_AHCIControler* pDrvCtrl,
                                           const uint8_t    kPortIndex);

/**
 * @brief Initializes the AHCI ports for a given controller.
 *
 * @details This function initializes all the AHCI ports associated with the
 * specified controller. It sets up the necessary configurations and prepares
 * the ports for operation.
 *
 * @param[in] pDrvCtrl Pointer to the AHCI controller structure.
 */
static void _InitAHCIPorts(S_AHCIControler* pDrvCtrl);

/**
 * @brief Identifies the AHCI ports for a given controller.
 *
 * @details This function identifies the types and capabilities of all the
 * AHCI ports associated with the specified controller. It retrieves information
 * about each port, such as whether it is a SATA or SATAPI port, and gathers
 * relevant device information.
 *
 * @param[in] pDrvCtrl Pointer to the AHCI controller structure.
 */
static void _IdentifyAHCIPorts(S_AHCIControler* pDrvCtrl);

/**
 * @brief Initializes the AHCI controller.
 *
 * @details This function initializes the AHCI controller by performing
 * necessary configurations and setups. It prepares the controller for operation
 * and ensures that it is ready to manage the connected AHCI ports.
 *
 * @param[in] pHostControl Pointer to the generic host control structure of the
 * AHCI controller.
 *
 * @return The success state or the error code.
 */
static E_Return _InitAHCIController(S_GenericHostControl* pHostControl);

/**
 * @brief Initializes the AHCI interrupts for a given PCI device.
 *
 * @details This function sets up the interrupt handling for the AHCI controller
 * associated with the specified PCI device. It configures the necessary
 * interrupt vectors and enables the interrupt mechanism to allow the controller
 * to signal events to the operating system.
 *
 * @param[in] pDevice Pointer to the PCI device structure representing the AHCI
 * controller.
 * @param[in] pHostControl Pointer to the generic host control structure of the
 * AHCI controller.
 *
 * @return The success state or the error code.
 */
static E_Return _InitAHCIInterrupts(S_PCIDevice*          pDevice,
                                    S_GenericHostControl* pHostControl);

/**
 * @brief Clears the error status for a given AHCI port.
 *
 * @details This function clears any error conditions that may have occurred on
 * the specified AHCI port. It resets the error status registers and ensures
 * that the port is in a clean state, ready to process new commands without
 * being affected by previous errors.
 *
 * @param[in] pPort Pointer to the AHCI port structure for which to clear
 * errors.
 */
static void _ClearAHCIPortError(S_AHCIPort* pPort);

/**
 * @brief Checks for error conditions on a given AHCI port.
 *
 * @details This function checks the status registers of the specified AHCI port
 * to determine if any error conditions are present. It evaluates the error bits
 * and returns a boolean value indicating whether an error has occurred.
 *
 * @param[in] pPort Pointer to the AHCI port structure to check for errors.
 *
 * @return Returns true if an error condition is detected, false otherwise.
 */
static bool _CheckAHCIPortError(S_AHCIPort* pPort);

/**
 * @brief Parses the SATAPI IDENTIFY data for a given AHCI port.
 *
 * @details This function retrieves and interprets the IDENTIFY data from a
 * SATAPI device connected to the specified AHCI port. It extracts relevant
 * information such as device capabilities, supported features, and other
 * characteristics, populating the provided port controller structure with
 * this information.
 *
 * @param[in] kVirtAddr The virtual address of the IDENTIFY data buffer.
 * @param[in] pPortCtrl Pointer to the AHCI port controller structure to be
 * populated with the parsed information.
 */
static void _ParseSATAPIPortIdentify(const uintptr_t      kVirtAddr,
                                     S_AHCIPortControler* pPortCtrl);

/**
 * @brief Parses the SATA IDENTIFY data for a given AHCI port.
 *
 * @details This function retrieves and interprets the IDENTIFY data from a
 * SATA device connected to the specified AHCI port. It extracts relevant
 * information such as device capabilities, supported features, and other
 * characteristics, populating the provided port controller structure with
 * this information.
 *
 * @param[in] kVirtAddr The virtual address of the IDENTIFY data buffer.
 * @param[in] pPortCtrl Pointer to the AHCI port controller structure to be
 * populated with the parsed information.
 */
static void _ParseSATAPortIdentify(const uintptr_t      kVirtAddr,
                                   S_AHCIPortControler* pPortCtrl);
/**
 * @brief Attaches the AHCI driver to the system.
 *
 * @details Attaches the AHCI driver to the system. This function will use the
 * PCI device to initialize the AHCI hardware and retreive the AHCI parameters.
 *
 * @param[in] pDevice The PCI device node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(S_PCIDevice* pDevice);

/**
 * @brief AHCI interrupt handler.
 *
 * @details This function is the interrupt handler for the AHCI driver. It will
 * handle the interrupts generated by the AHCI hardware and process them
 * accordingly.
 *
 * @return Returns true if the interrupt requests scheduling, false otherwise.
 */
static bool _AHCIInterruptHandler(void);

/**
 * @brief This function handles the interrupt for a specific AHCI port.
 *
 * @details This function is called when an interrupt is generated for a
 * specific AHCI port. It processes the interrupt by checking the status of the
 * port, handling any errors, and completing any pending commands.
 * The function ensures that the port is in a stable state after handling the
 * interrupt and prepares it for further operations.
 *
 * @param[in] pDrvCtrl Pointer to the AHCI controller structure that manages the
 * ports.
 * @param[in] kPortIndex Port index of the AHCI port for which the interrupt is
 * being handled.
 */
static void _AHCIHandlePortInterrupt(S_AHCIControler* pDrvCtrl,
                                     const uint8_t    kPortIndex);

/**
 * @brief This function handles the interrupt for a specific SATA port.
 *
 * @details This function is called when an interrupt is generated for a
 * specific SATA port. It processes the interrupt by checking the status of the
 * port, handling any errors, and completing any pending commands. The function
 * ensures that the port is in a stable state after handling the interrupt and
 * prepares it for further operations.
 *
 * @param[in] pPortCtrl Pointer to the AHCI port controller structure that
 * contains the state and configuration of the AHCI port.
 */
static void _AHCIHandleSATAPortInterrupt(S_AHCIPortControler* pPortCtrl);

/**
 * @brief This function waits for an AHCI operation to be retried.
 *
 * @details This function is used to wait for an AHCI operation to be retried.
 * It will block the current thread until the operation can be retried.
 * The function checks the status of the AHCI port and waits for the appropriate
 * conditions to be met before allowing the operation to proceed. This is
 * typically used in scenarios where an operation has failed or timed out,
 * and a retry is necessary to ensure successful completion.
 *
 * @param[in] pPortCtrl The AHCI port controller structure that contains the
 * state and configuration of the AHCI port.
 * @param[in] pCurThread The current kernel thread that is waiting for the
 * operation to be retried.
 */
static void _WaitForAHCIRetry(S_AHCIPortControler* pPortCtrl,
                              S_KernelThread*      pCurThread);

/**
 * @brief Reads data from a SATA port.
 *
 * @details This function reads data from a SATA port.
 * It uses the AHCI port controller to communicate with the SATA device.
 *
 * @param[in] pPortCtrl The AHCI port controller structure that contains the
 * state and configuration of the AHCI port.
 * @param[in] kBufPhys The physical address of the buffer to read into.
 * @param[in] kSectorStart The starting sector to read from.
 * @param[in] kSectorCount The number of sectors to read.
 * @param[out] pAsyncResult The result of the asynchronous operation.
 *
 * @return The success state or the error code.
 */
static E_AHCIOpResult _ReadSATAPort(S_AHCIPortControler* pPortCtrl,
                                    const uintptr_t      kBufPhys[PRDT_COUNT],
                                    const uint64_t       kSectorStart,
                                    const size_t         kSectorCount,
                                    E_AHCIOpResult*      pAsyncResult);

/**
 * @brief Writes data to a SATA port.
 *
 * @details This function writes data to a SATA port.
 * It uses the AHCI port controller to communicate with the SATA device.
 *
 * @param[in] pPortCtrl The AHCI port controller structure that contains the
 * state and configuration of the AHCI port.
 * @param[in] kBufPhys The physical address of the buffer to write from.
 * @param[in] kSectorStart The starting sector to write to.
 * @param[in] kSectorCount The number of sectors to write.
 * @param[out] pAsyncResult The result of the asynchronous operation.
 *
 * @return The success state or the error code.
 */
static E_AHCIOpResult _WriteSATAPort(S_AHCIPortControler* pPortCtrl,
                                    const uintptr_t       kBufPhys[PRDT_COUNT],
                                    const uint64_t        kSectorStart,
                                    const size_t          kSectorCount,
                                    E_AHCIOpResult*       pAsyncResult);

/**
 * @brief Reads data from an AHCI port.
 *
 * @details This function reads data from an AHCI port.
 * It uses the AHCI port controller to communicate with the AHCI device.
 *
 * @param[in] pPortCtrl The AHCI port controller structure that contains the
 * state and configuration of the AHCI port.
 * @param[in] kBufPhys The physical address of the buffer to read into.
 * @param[in] kSectorStart The starting sector to read from.
 * @param[in] kSectorCount The number of sectors to read.
 * @param[out] pAsyncResult The result of the asynchronous operation.
 *
 * @return The success state or the error code.
 */
static E_AHCIOpResult _ReadAHCIPort(S_AHCIPortControler* pPortCtrl,
                                    const uintptr_t      kBufPhys[PRDT_COUNT],
                                    const uint64_t       kSectorStart,
                                    const size_t         kSectorCount,
                                    E_AHCIOpResult*      pAsyncResult);

/**
 * @brief Writes data to an AHCI port.
 *
 * @details This function writes data to an AHCI port.
 * It uses the AHCI port controller to communicate with the AHCI device.
 *
 * @param[in] pPortCtrl The AHCI port controller structure that contains the
 * state and configuration of the AHCI port.
 * @param[in] kBufPhys The physical address of the buffer to write from.
 * @param[in] kSectorStart The starting sector to write to.
 * @param[in] kSectorCount The number of sectors to write.
 * @param[out] pAsyncResult The result of the asynchronous operation.
 *
 * @return The success state or the error code.
 */
static E_AHCIOpResult _WriteAHCIPort(S_AHCIPortControler* pPortCtrl,
                                     const uintptr_t      kBufPhys[PRDT_COUNT],
                                     const uint64_t       kSectorStart,
                                     const size_t         kSectorCount,
                                     E_AHCIOpResult*      pAsyncResult);
/**
 * @brief Performs an AHCI operation on a given port controller.
 *
 * @details This function performs the specified AHCI operation (read, write,
 * identify, etc.) on the given port controller. It prepares the necessary
 * parameters and buffers, issues the command to the hardware, and waits for
 * completion or error conditions. The function handles retries and error
 * checking as needed.
 *
 * @param[in] pPortCtrl Pointer to the AHCI port controller structure.
 * @param[in] pBuffers Array of physical addresses for data buffers.
 * @param[in] kSectorStart The starting sector for the operation.
 * @param[in] kSectorCount The number of sectors to operate on.
 * @param[in] kOperation The type of AHCI operation to perform.
 *
 * @return The result of the operation, indicating success, error, timeout,
 * retry, or abort conditions.
 */
static ssize_t _AHCIPerformOperation(S_AHCIPortControler*  pPortCtrl,
                                     uintptr_t             pBuffers[PRDT_COUNT],
                                     const uint64_t        kSectorStart,
                                     const size_t          kSectorCount,
                                     const E_AHCIOperation kOperation);
/**
 * @brief Checks and populates the parameters for an AHCI operation.
 *
 * @details This function validates the input parameters for an AHCI operation
 * and populates the necessary data structures. It ensures that the parameters
 * are within valid ranges and that the required resources are available.
 *
 * @param[in] pDriverData Pointer to the driver data structure.
 * @param[in] kpBuffer Pointer to the data buffer.
 * @param[in] kSectorStart The starting sector for the operation.
 * @param[in] kSectorCount The number of sectors to operate on.
 * @param[out] pBuffers Array of physical addresses for data buffers.
 *
 * @return true if the parameters are valid and populated, false otherwise.
 */
static bool _CheckAndPopulateParameters(void*          pDriverData,
                                         const void*    kpBuffer,
                                         const uint64_t kSectorStart,
                                         const size_t   kSectorCount,
                                         uintptr_t      pBuffers[PRDT_COUNT]);

/**
 * @brief Reads data from the AHCI device.
 *
 * @details This function reads data from the AHCI device. It takes the driver
 * data, a buffer to store the read data, the starting sector, and the number of
 * sectors to read. It returns the number of bytes read or an error code.
 *
 * @param[in] pDriverData Pointer to the driver data structure.
 * @param[out] pBuffer Pointer to the buffer where the read data will be stored.
 * @param[in] kSectorStart The starting sector for the read operation.
 * @param[in] kSectorCount The number of sectors to read.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _AHCIRead(void*          pDriverData,
                         void*          pBuffer,
                         const uint64_t kSectorStart,
                         const size_t   kSectorCount);

/**
 * @brief Writes data to the AHCI device.
 *
 * @details This function writes data to the AHCI device. It takes the driver
 * data, a buffer containing the data to write, the starting sector, and the
 * number of sectors to write. It returns the number of bytes written or an
 * error code.
 *
 * @param[in] pDriverData Pointer to the driver data structure.
 * @param[in] kpBuffer Pointer to the buffer containing the data to write.
 * @param[in] kSectorStart The starting sector for the write operation.
 * @param[in] kSectorCount The number of sectors to write.
 *
 * @return The number of bytes written or an error code.
 */
static ssize_t _AHCIWrite(void*          pDriverData,
                          const void*    kpBuffer,
                          const uint64_t kSectorStart,
                          const size_t   kSectorCount);

/**
 * @brief Performs an IOCTL operation on the AHCI device.
 *
 * @details This function performs an IOCTL operation on the AHCI device. It
 * takes the driver data, a command, and a pointer to the arguments for the
 * command. It returns the result of the IOCTL operation or an error code.
 *
 * @param[in] pDriverData Pointer to the driver data structure.
 * @param[in] kCommand The IOCTL command to perform.
 * @param[in] pArgs Pointer to the arguments for the IOCTL command.
 *
 * @return The result of the IOCTL operation or an error code.
 */
static ssize_t _AHCIIOCTL(void*          pDriverData,
                          const uint32_t kCommand,
                          void*          pArgs);

/**
 * @brief Retrieves the sector size of the AHCI device.
 *
 * @details This function retrieves the sector size of the AHCI device. It takes
 * the driver data and returns the sector size in bytes or an error code.
 *
 * @param[in] pDriverData Pointer to the driver data structure.
 *
 * @return The sector size in bytes or an error code.
 */
static ssize_t _AHCIGetSectorSize(void* pDriverData);

/**
 * @brief Retrieves the total number of sectors of the AHCI device.
 *
 * @details This function retrieves the total number of sectors of the AHCI
 * device. It takes the driver data and returns the total number of sectors or
 * an error code.
 *
 * @param[in] pDriverData Pointer to the driver data structure.
 *
 * @return The total number of sectors or an error code.
 */
static ssize_t _AHCIGetTotalSectors(void* pDriverData);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief AHCI driver instance. */
static S_PCIDriver sX86AHCIDriver =
{
  .pName            = "X86 AHCI Driver",
  .pDescription     = "X86 Advanced Host Controller Interface Driver for roOs",
  .pVersion         = "1.0",
  .vendorID         = 0xFFFF,
  .deviceID         = 0xFFFF,
  .devClass         = 0x01,
  .devSubClass      = 0x06,
  .devProgIf        = 0x01,
  .pDriverAttach    = _Attach
};

/** @brief AHCI driver control structure */
static S_AHCIControler* spDrvCtrl = NULL;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _PerformBIOSHandoff(S_GenericHostControl* pHostControl)
{
  uint32_t status;
  uint64_t timeout;

  /* Perform BIOS handoff */
  status = _MMIORead32(&pHostControl->biosHandoffControlStatus);
  if (status & AHCI_BIOS_HANDOFF_CONTROL_STATUS_BIOS_OWNED)
  {
    status |= AHCI_BIOS_HANDOFF_CONTROL_STATUS_OS_OWNED;
    _MMIOWrite32(&pHostControl->biosHandoffControlStatus, status);

    timeout = TimeGetUptime() + AHCI_HANDOFF_TIMEOUT_NS;
    do
    {
      /* Wait for the BIOS to release control */
      if (TimeGetUptime() > timeout)
      {
        break;
      }
      status = _MMIORead32(&pHostControl->biosHandoffControlStatus);
    } while ((status & AHCI_BIOS_HANDOFF_CONTROL_STATUS_BIOS_OWNED) != 0);

    /* Check for timeout */
    if((status & AHCI_BIOS_HANDOFF_CONTROL_STATUS_BIOS_OWNED) != 0)
    {
      /* Force clean */
      status &= ~AHCI_BIOS_HANDOFF_CONTROL_STATUS_BIOS_OWNED;
      status &= ~AHCI_BIOS_HANDOFF_CONTROL_STATUS_BIOS_BUSY;
      status |= AHCI_BIOS_HANDOFF_CONTROL_STATUS_OS_OWNED;

      timeout = TimeGetUptime() + AHCI_HANDOFF_FORCE_TIMEOUT_NS;
      while (TimeGetUptime() < timeout)
      {
      }
    }
  }
}

static void _StartAHCIPortCommandEngine(S_AHCIPort* pPort)
{
  uint32_t status;

  /* Start the command engine */
  status = _MMIORead32(&pPort->commandAndStatus);
  status |= AHCI_COMMAND_AND_STATUS_START |
            AHCI_COMMAND_AND_STATUS_FIS_RECEIVE_ENABLE;
  _MMIOWrite32(&pPort->commandAndStatus, status);
}

static E_Return _StopAHCIPortCommandEngine(S_AHCIPort* pPort)
{
  uint32_t status;
  uint64_t timeout;
  E_Return err;

  /* Stop the command engine */
  status = _MMIORead32(&pPort->commandAndStatus);
  status &= ~(AHCI_COMMAND_AND_STATUS_START |
              AHCI_COMMAND_AND_STATUS_FIS_RECEIVE_ENABLE);
  _MMIOWrite32(&pPort->commandAndStatus, status);

  /* Wait for the command engine to stop */
  timeout = TimeGetUptime() + AHCI_PORT_STOP_TIMEOUT_NS;
  while (_MMIORead32(&pPort->commandAndStatus) &
         (AHCI_COMMAND_AND_STATUS_CMD_LIST_RUNNING |
          AHCI_COMMAND_AND_STATUS_FIS_RECEIVE_RUNNING))
  {
    /* Wait for the command engine to stop */
    if (TimeGetUptime() > timeout)
    {
      break;
    }
  }
  if (_MMIORead32(&pPort->commandAndStatus) &
      (AHCI_COMMAND_AND_STATUS_CMD_LIST_RUNNING |
       AHCI_COMMAND_AND_STATUS_FIS_RECEIVE_RUNNING))
  {
    /* Reset the port */
    status = _MMIORead32(&pPort->sataControl);
    status = (status & ~0xF) | AHCI_SATA_CONTROL_DET_INIT;
    _MMIOWrite32(&pPort->sataControl, status);

    /* Wait for at least 1 ms */
    timeout = TimeGetUptime() + AHCI_PORT_RESET_DELAY_NS;
    while (TimeGetUptime() < timeout)
    {
    }

    /* Clear the DET field */
    status = _MMIORead32(&pPort->sataControl);
    status = (status & ~0xF) | AHCI_SATA_CONTROL_DET_NONE;
    _MMIOWrite32(&pPort->sataControl, status);

    /* Wait for reset to complete */
    timeout = TimeGetUptime() + AHCI_PORT_RESET_TIMEOUT_NS;
    while ((_MMIORead32(&pPort->sataStatus) & AHCI_SATA_STATUS_DET_MASK) !=
           (AHCI_SATA_STATUS_DET_PRESENT << AHCI_SATA_STATUS_DET_SHIFT))
    {
      /* Wait for the command engine to stop */
      if (TimeGetUptime() > timeout)
      {
        break;
      }
    }

    /* Check if the reset was successful */
    if ((_MMIORead32(&pPort->sataStatus) & AHCI_SATA_STATUS_DET_MASK) !=
         (AHCI_SATA_STATUS_DET_PRESENT << AHCI_SATA_STATUS_DET_SHIFT))
    {
      err = ERR_INVALID_VALUE;
    }
    else
    {
      err = NO_ERROR;
    }
  }
  else
  {
    err = NO_ERROR;
  }

  return err;
}

static void _EnableAHCIPortInterrupts(S_AHCIPort* pPort)
{
  uint32_t interruptStatus;

  interruptStatus = _MMIORead32(&pPort->interruptStatus);
  /* Clear any pending interrupts */
  _MMIOWrite32(&pPort->interruptStatus, interruptStatus);

  /* Enable interrupts for the port */
  _MMIOWrite32(&pPort->interruptEnable, AHCI_INTERRUPT_ENABLE_MASK);
}


static E_Return _AllocateAHCIPortResources(S_AHCIControler* pDrvCtrl,
                                          const uint8_t     kPortIndex)
{
  E_Return             err;
  uintptr_t            cmdListPhysAddr;
  size_t               pageCount;
  size_t               allocSize;
  uint8_t*             pMem;
  S_AHCICommandHeader* pCmdHeader;
  S_AHCIPortControler* pPortCtrl;
  size_t               i;
  S_AHCIPort*          pPort;
  uintptr_t            scrachpadPhysAddr;
  uintptr_t            scrachpadVirtAddr;

  pPortCtrl = &pDrvCtrl->ports[kPortIndex];
  pPort     = pPortCtrl->pPort;

  /* Allocate the control structures */
  KERNEL_SPINLOCK_INIT(pPortCtrl->lock);
  KERNEL_SPINLOCK_INIT(pPortCtrl->retryQueueLock);
  memset(pPortCtrl->commandWaitingTable, 0,
         sizeof(pPortCtrl->commandWaitingTable));
  pPortCtrl->issuedCommandTable = 0;
  pPortCtrl->pRetryWaitingList = KQueueCreate(NULL);

  /* Allocate the port's scratchpad memory */
  scrachpadPhysAddr = MemoryAllocFrames(1);
  if (scrachpadPhysAddr != 0)
  {
    scrachpadVirtAddr = (uintptr_t)MemoryKernelMap((void*)scrachpadPhysAddr,
                                                   KERNEL_PAGE_SIZE,
                                                   MEMMGR_MAP_KERNEL |
                                                   MEMMGR_MAP_RW |
                                                   MEMMGR_MAP_CACHE_DISABLED,
                                                   &err);
    if (err != NO_ERROR)
    {
      MemoryReleaseFrame(scrachpadPhysAddr, 1);
    }
  }
  else
  {
    err = ERR_NO_MEMORY;
  }

  if (err == NO_ERROR)
  {
    pDrvCtrl->ports[kPortIndex].scratchpadPhysAddr = scrachpadPhysAddr;
    pDrvCtrl->ports[kPortIndex].scratchpadVirtAddr = scrachpadVirtAddr;

    /* Allocate the command list */
    allocSize = (COMMAND_LIST_SIZE +
                FIS_RECEIVE_AREA_SIZE +
                COMMAND_TABLE_COUNT *
                (COMMAND_TABLE_SIZE + PRDT_SIZE * PRDT_COUNT));
    if (allocSize % KERNEL_PAGE_SIZE == 0)
    {
      pageCount = allocSize / KERNEL_PAGE_SIZE;
    }
    else
    {
      pageCount = (allocSize / KERNEL_PAGE_SIZE) + 1;
    }
    cmdListPhysAddr = MemoryAllocFrames(pageCount);
    if (cmdListPhysAddr != 0)
    {
      pMem = MemoryKernelMap((void*)cmdListPhysAddr,
                            pageCount * KERNEL_PAGE_SIZE,
                            MEMMGR_MAP_KERNEL |
                            MEMMGR_MAP_RW |
                            MEMMGR_MAP_CACHE_DISABLED,
                            &err);
    }
    else
    {
      err = ERR_NO_MEMORY;
    }

    /* Perform the setup */
    if (err == NO_ERROR)
    {
      pDrvCtrl->ports[kPortIndex].pMem = pMem;
      pDrvCtrl->ports[kPortIndex].memorySize = pageCount * KERNEL_PAGE_SIZE;

      memset(pMem, 0, pageCount * KERNEL_PAGE_SIZE);

      /* Reserve the allocated frame start to the command list */
      _MMIOWrite32(&pPort->commandListBaseAddrLow,
                  (uint32_t)(cmdListPhysAddr & 0xFFFFFFFF));
      _MMIOWrite32(&pPort->commandListBaseAddrHigh,
                  (uint32_t)((cmdListPhysAddr >> 32) & 0xFFFFFFFF));

      /* Increment the memory while keeping the alignement */
      cmdListPhysAddr += COMMAND_LIST_SIZE;

      _MMIOWrite32(&pPort->fisBaseAddrLow,
                  (uint32_t)(cmdListPhysAddr & 0xFFFFFFFF));
      _MMIOWrite32(&pPort->fisBaseAddrHigh,
                  (uint32_t)((cmdListPhysAddr >> 32) & 0xFFFFFFFF));

      /* Increment the memory while keeping the alignement */
      cmdListPhysAddr += FIS_RECEIVE_AREA_SIZE;

      /* Allocate the command tables */
      pCmdHeader = (S_AHCICommandHeader*)pMem;
      for (i = 0; i < COMMAND_TABLE_COUNT; ++i)
      {
        _MMIOWrite32(&pCmdHeader[i].prdtl,
                     PRDT_COUNT << AHCI_COMMAND_HEADER_PRDTL_SHIFT);
        _MMIOWrite32(&pCmdHeader[i].ctbaLow,
                    (uint32_t)(cmdListPhysAddr & 0xFFFFFFFF));
        _MMIOWrite32(&pCmdHeader[i].ctbaHigh,
                    (uint32_t)((cmdListPhysAddr >> 32) & 0xFFFFFFFF));

        cmdListPhysAddr += sizeof(S_ACPICommandTable);
      }
    }
  }

  return err;
}

static void _InitAHCIPorts(S_AHCIControler* pDrvCtrl)
{
  E_Return              err;
  uint8_t               portIndex;
  uint32_t              portImplemented;
  S_GenericHostControl* pHostControl;
  S_AHCIPort*           pPort;
  uint32_t              status;
  uint32_t              devDetect;
  uint32_t              ipmState;

  pHostControl = (S_GenericHostControl*)pDrvCtrl->pBARBaseAddr;
  portImplemented = _MMIORead32(&pHostControl->portsImplemented);

  for (portIndex = 0; portIndex < AHCI_MAX_PORTS; ++portIndex)
  {
    if (portImplemented & (1 << portIndex))
    {
      pPort = (S_AHCIPort*)((uintptr_t)pHostControl + AHCI_PORT_OFFSET +
                            (portIndex * AHCI_PORT_SIZE));
      /* Stop the command engine */
      err = _StopAHCIPortCommandEngine(pPort);
      if (err == NO_ERROR)
      {
        /* Allocate memory for the port */
        pDrvCtrl->ports[portIndex].pPort = pPort;
        err = _AllocateAHCIPortResources(pDrvCtrl, portIndex);
        if (err == NO_ERROR)
        {
          /* Clear errors */
          status = _MMIORead32(&pPort->sataError);
          _MMIOWrite32(&pPort->sataError, status);

          /* Start the command engine */
          _StartAHCIPortCommandEngine(pPort);

          /* Setup interrupts */
          _EnableAHCIPortInterrupts(pPort);

          status    = _MMIORead32(&pPort->sataStatus);
          devDetect = (status & AHCI_SATA_STATUS_DET_MASK) >>
                      AHCI_SATA_STATUS_DET_SHIFT;
          ipmState  = (status & AHCI_SATA_STATUS_IPM_MASK) >>
                      AHCI_SATA_STATUS_IPM_SHIFT;

          if (devDetect == AHCI_SATA_STATUS_DET_PRESENT &&
              ipmState == AHCI_SATA_STATUS_IPM_ACTIVE)
          {
            status = _MMIORead32(&pPort->signature);
            switch (status)
            {
              case AHCI_SIGNATURE_SATA:
                pDrvCtrl->ports[portIndex].type = AHCI_PORT_TYPE_SATA;
                KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                            MODULE_NAME,
                            "Port %d: SATA device detected.",
                            portIndex);
                break;
              case AHCI_SIGNATURE_SATAPI:
                pDrvCtrl->ports[portIndex].type = AHCI_PORT_TYPE_SATAPI;
                KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                            MODULE_NAME,
                            "Port %d: SATAPI device detected.",
                            portIndex);
                break;
              case AHCI_SIGNATURE_SEMB:
                pDrvCtrl->ports[portIndex].type = AHCI_PORT_TYPE_SEMB;
                KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                            MODULE_NAME,
                            "Port %d: SEMB device detected.",
                            portIndex);
                break;
              case AHCI_SIGNATURE_PM:
                pDrvCtrl->ports[portIndex].type = AHCI_PORT_TYPE_PM;
                KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                            MODULE_NAME,
                            "Port %d: PM device detected.",
                            portIndex);
                break;
              default:
                pDrvCtrl->ports[portIndex].type = AHCI_PORT_TYPE_UNKNOWN;
                KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                            MODULE_NAME,
                            "Port %d: Unknown device detected. Signature: 0x%08X",
                            portIndex,
                            status);
                break;
            }
          }
        }
      }
    }
  }
}

static void _IdentifyAHCIPorts(S_AHCIControler* pDrvCtrl)
{
  uint8_t              i;
  uint32_t             j;
  int8_t               slotPort;
  uint32_t             actSlots;
  uint32_t             cmdIssue;
  uint32_t             status;
  S_AHCICommandHeader* pCmdHeader;
  S_AHCIPortControler* pPortCtrl;
  S_ACPICommandTable*  pCmdTable;
  S_AHCIPort*          pPort;
  S_FISRegisterH2D*    pCmdFIS;
  uint64_t             timeout;
  uint32_t             cmdValue;
  bool                 complete;
  bool                 error;
  E_Return             err;

  for (i = 0; i < AHCI_MAX_PORTS; ++i)
  {
    pPortCtrl = &pDrvCtrl->ports[i];
    pPort     = pPortCtrl->pPort;

    if (pPortCtrl->type != AHCI_PORT_TYPE_NONE)
    {
      /* Clear interrupts */
      _MMIOWrite32(&pPort->interruptEnable, 0);
      status = _MMIORead32(&pPort->interruptStatus);
      _MMIOWrite32(&pPort->interruptStatus, status);

      if (pPortCtrl->type == AHCI_PORT_TYPE_PM ||
          pPortCtrl->type == AHCI_PORT_TYPE_SEMB)
      {
        KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Port %d: Unsupported device type.",
                     i);
        continue;
      }


      KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "IDENTIFY Port %d: Type: %d",
                   i,
                   pPortCtrl->type);

      /* Get a free slot */
      slotPort = -1;
      actSlots = _MMIORead32(&pPort->sataActive);
      cmdIssue = _MMIORead32(&pPort->commandAndStatus);
      actSlots |= cmdIssue;
      for (j = 0; j < COMMAND_TABLE_COUNT; ++j)
      {
        if ((actSlots & (1 << j)) == 0)
        {
          slotPort = j;
          break;
        }
      }

      /* If no free slot is available, mark the port as unknown */
      if (slotPort == -1)
      {
        pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
        continue;
      }

      /* Setup the command header */
      pCmdHeader = (S_AHCICommandHeader*)pPortCtrl->pMem;
      cmdValue = sizeof(S_FISRegisterH2D) / sizeof(uint32_t) |
                 COMMAND_HEADER_CONFIG_READ;
      if (pPortCtrl->type == AHCI_PORT_TYPE_SATAPI)
      {
        cmdValue |= COMMAND_HEADER_CONFIG_ATAPI;
      }
      _MMIOWrite32(&pCmdHeader[slotPort].config,
                   cmdValue | (1 << AHCI_COMMAND_HEADER_PRDTL_SHIFT));

      /* Setup the command table */
      pCmdTable = (S_ACPICommandTable*)((uintptr_t)pCmdHeader +
                                        COMMAND_LIST_SIZE +
                                        FIS_RECEIVE_AREA_SIZE +
                                        slotPort *
                                        sizeof(S_ACPICommandTable));
      for (j = 0; j < sizeof(S_ACPICommandTable) / sizeof(uint32_t); ++j)
      {
        _MMIOWrite32(((uint32_t*)pCmdTable) + j, 0);
      }

      /* Setup the PRDT */
      _MMIOWrite32(&pCmdTable->prdtEntry[0].dbaLow,
        (uint32_t)(pPortCtrl->scratchpadPhysAddr & 0xFFFFFFFF));
      _MMIOWrite32(&pCmdTable->prdtEntry[0].dbaHigh,
        (uint32_t)((pPortCtrl->scratchpadPhysAddr >> 32) & 0xFFFFFFFF));
      _MMIOWrite32(&pCmdTable->prdtEntry[0].dbcIoc,
                   CMD_IDENTIFY_DATA_SIZE - 1);

      /* Setup the command FIS */
      pCmdFIS = (S_FISRegisterH2D*)&pCmdTable->commandFIS;
      cmdValue = FIS_REGISTER_H2D | FIS_COMMAND_FIS;
      if (pPortCtrl->type == AHCI_PORT_TYPE_SATAPI)
      {
        cmdValue |= FIS_COMMAND_IDENTIFY_PACKET_DEVICE;
      }
      else
      {
        cmdValue |= FIS_COMMAND_ATA_IDENTIFY;
      }
      _MMIOWrite32(&pCmdFIS->fisType, cmdValue);

      if(pPortCtrl->type == AHCI_PORT_TYPE_SATAPI)
      {
        _MMIOWrite32(&pCmdFIS->lbaLow, 0xA0000000);
        _MMIOWrite32(&pCmdFIS->lbaLowExp, 0);
        _MMIOWrite32(&pCmdFIS->countLow, 0x00000000);
      }
      else
      {
        _MMIOWrite32(&pCmdFIS->lbaLow, 0xA0000000);
        _MMIOWrite32(&pCmdFIS->lbaLowExp, 0);
        _MMIOWrite32(&pCmdFIS->countLow, 0x20000000);
      }
      _MMIOWrite32(&pCmdFIS->reserved1, 0);

      /* Wait for slot to be available */
      timeout = TimeGetUptime() + AHCI_COMMAND_TIMEOUT_NS;
      status = _MMIORead32(&pPort->taskFileData);
      while ((_MMIORead32(&pPort->taskFileData) &
             (TFD_STATUS_BSY)) != 0)
      {
        /* Wait for the command engine to stop */
        if (TimeGetUptime() > timeout)
        {
          break;
        }
      }
      if ((_MMIORead32(&pPort->taskFileData) &
          (TFD_STATUS_BSY)) != 0)
      {
        KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Port %d: Command timeout.",
                     i);
        pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
        continue;
      }

      /* Issue the command */
      _ClearAHCIPortError(pPort);
      _MMIOWrite32(&pPort->commandIssue, 1 << slotPort);

      /* Poll for completion */
      complete = false;
      timeout = TimeGetUptime() + AHCI_COMMAND_TIMEOUT_NS;
      while (true)
      {
        status = _MMIORead32(&pPort->commandIssue);
        if ((status & (1 << slotPort)) == 0)
        {
          complete = true;
          break;
        }
        error = _CheckAHCIPortError(pPort);
        if (error == true)
        {
          break;
        }
        if (TimeGetUptime() > timeout)
        {
          KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                       MODULE_NAME,
                       "Port %d: Command timeout.",
                       i);
          break;
        }
      }

      error = _CheckAHCIPortError(pPort);
      if (error == true)
      {
        _MMIOWrite32(&pPort->interruptStatus, status);
        KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                      MODULE_NAME,
                      "Port %d: Task file error.",
                      i);
        complete = false;
      }

      /* Clear interrupts */
      status = _MMIORead32(&pPort->interruptStatus);
      _MMIOWrite32(&pPort->interruptStatus, status);

      if (complete == true)
      {
        if (pPortCtrl->type == AHCI_PORT_TYPE_SATAPI)
        {
          _ParseSATAPIPortIdentify(pPortCtrl->scratchpadVirtAddr, pPortCtrl);
          KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                       MODULE_NAME,
                       "Port %d: SATAPI device identified."
                       "\n\tModel: %s"
                       "\n\tSerial: %s",
                       i,
                       pPortCtrl->devInfo.satapidInfo.modelNumber,
                       pPortCtrl->devInfo.satapidInfo.serialNumber);
          /* Enable interrupts for this port */
          _MMIOWrite32(&pPort->interruptEnable, AHCI_INTERRUPT_ENABLE_MASK);
        }
        else
        {
          _ParseSATAPortIdentify(pPortCtrl->scratchpadVirtAddr, pPortCtrl);
          KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                       MODULE_NAME,
                       "Port %d: SATA device identified."
                       "\n\tModel: %s"
                       "\n\tSerial: %s"
                       "\n\tTotal Sectors: %llu"
                       "\n\tSector Size: %u",
                       i,
                       pPortCtrl->devInfo.satadInfo.modelNumber,
                       pPortCtrl->devInfo.satadInfo.serialNumber,
                       pPortCtrl->devInfo.satadInfo.totalSectors,
                       pPortCtrl->devInfo.satadInfo.sectorSize);
          /* Enable interrupts for this port */
          _MMIOWrite32(&pPort->interruptEnable, AHCI_INTERRUPT_ENABLE_MASK);
          err = DiskManagerAddDevice(DISK_TYPE_AHCI,
                                     _AHCIRead,
                                     _AHCIWrite,
                                     _AHCIIOCTL,
                                     _AHCIGetSectorSize,
                                     _AHCIGetTotalSectors,
                                     pPortCtrl,
                                     &pPortCtrl->vfsHandle);
          if (err != NO_ERROR)
          {
            KERNEL_ERROR("Port %d: Failed to add SATAPI device to disk"
                          " manager. Error: %d\n",
                          i,
                          err);
          }
        }

      }
      else
      {
        pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
      }
    }
  }
}

static E_Return _InitAHCIController(S_GenericHostControl* pHostControl)
{
  uint32_t globalHostControl;
  uint64_t timeout;
  E_Return err;

  /* Enable AHCI mode */
  globalHostControl = _MMIORead32(&pHostControl->globalHostControl);
  globalHostControl |= AHCI_GLOBAL_HOST_CONTROL_AHCI_ENABLE;
  _MMIOWrite32(&pHostControl->globalHostControl, globalHostControl);

  /* Perform handoff */
  _PerformBIOSHandoff(pHostControl);

  /* Clear interrupts */
  _MMIOWrite32(&pHostControl->interruptStatus, 0xFFFFFFFF);

  /* Reset the AHCI controller */
  _MMIOWrite32(&pHostControl->globalHostControl,
                AHCI_GLOBAL_HOST_CONTROL_HBA_RESET);
  while (_MMIORead32(&pHostControl->globalHostControl) &
          AHCI_GLOBAL_HOST_CONTROL_HBA_RESET)
  {
    /* Wait for the reset to complete */
    timeout = TimeGetUptime() + AHCI_RESET_TIMEOUT_NS;
    if (TimeGetUptime() > timeout)
    {
      break;
    }
  }

  /* Enable AHCI mode */
  globalHostControl = _MMIORead32(&pHostControl->globalHostControl);
  globalHostControl |= AHCI_GLOBAL_HOST_CONTROL_AHCI_ENABLE;
  _MMIOWrite32(&pHostControl->globalHostControl, globalHostControl);

  /* Check reset and continue */
  if ((_MMIORead32(&pHostControl->globalHostControl) &
      AHCI_GLOBAL_HOST_CONTROL_HBA_RESET) == 0)
  {
    err = NO_ERROR;
  }
  else
  {
    err = ERR_INVALID_VALUE;
  }

  return err;
}

static E_Return _InitAHCIInterrupts(S_PCIDevice*          pDevice,
                                    S_GenericHostControl* pHostControl)
{
  E_Return err;
  uint32_t globalHostControl;

  /* Try to setup MSI-X */
  err = PCISetupInterruptMSIX(pDevice, pDevice->interruptLine, 0, true);

  if (err == ERR_NOT_SUPPORTED)
  {
    /* Try to setup MSI */
    err = PCISetupInterruptMSI(pDevice, pDevice->interruptLine, true);
  }

  if (err == NO_ERROR)
  {
    /* Register the interrupt handler */
    err = InterruptRegister(pDevice->interruptLine,
                            _AHCIInterruptHandler,
                            false);
    if (err == NO_ERROR)
    {
      /* Set the interrupt enable in the controller */
      globalHostControl = _MMIORead32(&pHostControl->globalHostControl);
      globalHostControl |= AHCI_GLOBAL_HOST_CONTROL_AHCI_IE;
      _MMIOWrite32(&pHostControl->globalHostControl, globalHostControl);

      InterruptSetEOI(pDevice->interruptLine);
    }
  }

  return err;
}

static void _ClearAHCIPortError(S_AHCIPort* pPort)
{
  uint32_t status;

  status = _MMIORead32(&pPort->sataError);
  _MMIOWrite32(&pPort->sataError, status);
  status = _MMIORead32(&pPort->interruptStatus);
  _MMIOWrite32(&pPort->interruptStatus, status);
  status = _MMIORead32(&pPort->taskFileData);
  _MMIOWrite32(&pPort->taskFileData, status);
}

static bool _CheckAHCIPortError(S_AHCIPort* pPort)
{
  uint32_t status;
  bool     error;

  error = false;

  /* Read and check SERR */
  status = _MMIORead32(&pPort->sataError);
  if ((status & 0x0000FFFF) == 0)
  {
    /* Read and check IS */
    status = _MMIORead32(&pPort->interruptStatus);
    if ((status & IS_ERROR_MASK) == 0)
    {
      /* Read and check TFD */
      status = _MMIORead32(&pPort->taskFileData);
      if ((status & (TFD_ERROR | TFD_STATUS_ERROR)) != 0)
      {
        _MMIOWrite32(&pPort->taskFileData, status);
        error = true;
      }
    }
    else
    {
      /* Clear the interrupt status */
      _MMIOWrite32(&pPort->interruptStatus, status);
      error = true;
    }
  }
  else
  {
    /* Clear the error */
    _MMIOWrite32(&pPort->sataError, status);
    error = true;
  }

  return error;
}

static void _ParseSATAPIPortIdentify(const uintptr_t      kVirtAddr,
                                     S_AHCIPortControler* pPortCtrl)
{
  S_ATAPIIdentify*        pIdentify;
  S_SATAPIDevInformation* pDevInfo;
  uint32_t                i;
  uint8_t                 checksum;
  uint16_t                charWord;
  uint32_t                charIndex;

  pIdentify = (S_ATAPIIdentify*)kVirtAddr;
  pDevInfo = &pPortCtrl->devInfo.satapidInfo;

  /* Check the CRC */
  if (pIdentify->checksumValidity == IDENTIFY_CHECKSUM_VALID)
  {
    checksum = 0;
    for (i = 0; i < CMD_IDENTIFY_DATA_SIZE; ++i)
    {
      checksum += ((uint8_t*)pIdentify)[i];
    }
  }
  else
  {
    checksum = 0;
  }

  if (checksum == 0)
  {
    /* Check if the device is a SATA device */
    if (pIdentify->generalConfiguration.commandPacketDeviceType ==
        ATA_DEVICE_TYPE_SATAPI)
    {
      /* Parse the SATAPI device information */
      if (pIdentify->generalConfiguration.packetSize == 0)
      {
        pDevInfo->isSCSI16Supported = false;
      }
      else
      {
        pDevInfo->isSCSI16Supported = true;
      }
      if (pIdentify->capabilities.dmaSupported)
      {
        pDevInfo->isDMASupported = true;
      }
      else
      {
        pDevInfo->isDMASupported = false;
      }
      if (pIdentify->serialAtaCapabilities.sataGen3)
      {
        pDevInfo->negociatedSpeed = 3;
      }
      else if (pIdentify->serialAtaCapabilities.sataGen2)
      {
        pDevInfo->negociatedSpeed = 2;
      }
      else if (pIdentify->serialAtaCapabilities.sataGen1)
      {
        pDevInfo->negociatedSpeed = 1;
      }

      if (pIdentify->capabilities.iordySupported)
      {
        pDevInfo->isIORDYSupported = true;
      }
      else
      {
        pDevInfo->isIORDYSupported = false;
      }

      if (pIdentify->serialAtaCapabilities.ncq)
      {
        pDevInfo->isNativeCommandQueuingSupported = true;
      }
      else
      {
        pDevInfo->isNativeCommandQueuingSupported = false;
      }

      /* Parse model and serial number */
      charIndex = 0;
      for (i = 0; i < 20; ++i)
      {
        charWord = pIdentify->modelNumber[i];
        pDevInfo->modelNumber[charIndex++] = (char)(charWord >> 8);
        pDevInfo->modelNumber[charIndex++] = (char)(charWord & 0xFF);
      }
      pDevInfo->modelNumber[charIndex] = '\0';
      charIndex = 0;
      for (i = 0; i < 10; ++i)
      {
        charWord = pIdentify->serialNumber[i];
        pDevInfo->serialNumber[charIndex++] = (char)(charWord >> 8);
        pDevInfo->serialNumber[charIndex++] = (char)(charWord & 0xFF);
      }
      pDevInfo->serialNumber[charIndex] = '\0';
    }
    else
    {
      pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
    }
  }
  else
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "Port %d: Identify data checksum invalid: 0x%x.",
                  pPortCtrl - spDrvCtrl->ports,
                  checksum);
    pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
  }
}

static void _ParseSATAPortIdentify(const uintptr_t      kVirtAddr,
                                   S_AHCIPortControler* pPortCtrl)
{
  S_ATAIdentify*        pIdentify;
  S_SATADevInformation* pDevInfo;
  uint32_t              i;
  uint8_t               checksum;
  uint16_t              charWord;
  uint32_t              charIndex;

  pIdentify = (S_ATAIdentify*)kVirtAddr;
  pDevInfo = &pPortCtrl->devInfo.satadInfo;

  /* Check the CRC */
  if (pIdentify->checksumValidity == IDENTIFY_CHECKSUM_VALID)
  {
    checksum = 0;
    for (i = 0; i < CMD_IDENTIFY_DATA_SIZE; ++i)
    {
      checksum += ((uint8_t*)pIdentify)[i];
    }
  }
  else
  {
    checksum = 0;
  }

  if (checksum == 0)
  {
    /* Check if the device is a SATA device */
    if (pIdentify->generalConfiguration.deviceType == ATA_DEVICE_TYPE_SATA)
    {
      if (pIdentify->physicalLogicalSectorSize.setTo0 == 0 &&
          pIdentify->physicalLogicalSectorSize.setTo1 == 0)
      {
        if (pIdentify->physicalLogicalSectorSize.sectorLongerThan256Words == 0)
        {
          pDevInfo->sectorSize = 512;
        }
        else
        {
          pDevInfo->sectorSize = pIdentify->wordsPerLogicalSector *
                                 sizeof(uint16_t);
        }
      }
      else
      {
        pDevInfo->sectorSize = 512;
      }
      /* Check if the device supports LBA48 */
      if (pIdentify->commandSetSupport.address48BitSupported)
      {
        pDevInfo->isLBA48Supported = true;
        pDevInfo->totalSectors = pIdentify->numberOfLBA48Sectors;
      }
      else
      {
        /* We do not support LBA28 */
        pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
        pDevInfo->isLBA48Supported = false;
        pDevInfo->totalSectors = pIdentify->numberOfLBA28Sectors;
      }

      if (pIdentify->capabilities.dmaSupported)
      {
        pDevInfo->isDMASupported = true;
      }
      else
      {
        pDevInfo->isDMASupported = false;
      }

      /* Parse model and serial number */
      charIndex = 0;
      for (i = 0; i < 20; ++i)
      {
        charWord = pIdentify->modelNumber[i];
        pDevInfo->modelNumber[charIndex++] = (char)(charWord >> 8);
        pDevInfo->modelNumber[charIndex++] = (char)(charWord & 0xFF);
      }
      pDevInfo->modelNumber[charIndex] = '\0';
      charIndex = 0;
      for (i = 0; i < 10; ++i)
      {
        charWord = pIdentify->serialNumber[i];
        pDevInfo->serialNumber[charIndex++] = (char)(charWord >> 8);
        pDevInfo->serialNumber[charIndex++] = (char)(charWord & 0xFF);
      }
      pDevInfo->serialNumber[charIndex] = '\0';
    }
    else
    {
      pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
    }
  }
  else
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                  MODULE_NAME,
                  "Port %d: Identify data checksum invalid: 0x%x.",
                  pPortCtrl - spDrvCtrl->ports,
                  checksum);
    pPortCtrl->type = AHCI_PORT_TYPE_UNKNOWN;
  }
}

static E_Return _Attach(S_PCIDevice* pDevice)
{
  E_Return              err;
  E_Return              internalError;
  uintptr_t             barAddr;
  size_t                barSize;
  S_GenericHostControl* pHostControl;
  S_AHCIControler*      pDrvCtrl;

  /* Get the BAR for the AHCI controller */
  barAddr = PCIGetBARAddress(pDevice->pFunction, 5);
  barSize = PCIGetBARSize(pDevice->pFunction, 5);

  if (barAddr != 0 && barSize != 0)
  {
    /* Map the memory region */
    pHostControl = MemoryKernelMap((void*)barAddr,
                                   barSize,
                                   MEMMGR_MAP_HARDWARE |
                                   MEMMGR_MAP_RW |
                                   MEMMGR_MAP_KERNEL,
                                   &err);
    if (err == NO_ERROR)
    {
      err = _InitAHCIController(pHostControl);
      KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "AHCI controller initialized | Status: %x",
                   err);

      if (err == NO_ERROR)
      {
        err = _InitAHCIInterrupts(pDevice, pHostControl);
        KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "AHCI controller interrupts initialized | Status: %x",
                     err);
        if (err == NO_ERROR)
        {
          pDrvCtrl = (S_AHCIControler*)KMalloc(sizeof(S_AHCIControler),
          KMALLOC_FREE_POOL);

          /* Initialize the driver control structure */
          memset(pDrvCtrl, 0, sizeof(S_AHCIControler));
          KERNEL_SPINLOCK_INIT(pDrvCtrl->lock);
          pDrvCtrl->pBARBaseAddr = pHostControl;
          pDrvCtrl->pDevice = pDevice;
          pDevice->pDriver->pDriverData = pDrvCtrl;

          /* Initialize the ports */
          _InitAHCIPorts(pDrvCtrl);
          /* Identify the AHCI ports */
          _IdentifyAHCIPorts(pDrvCtrl);

          pDrvCtrl->pNext = spDrvCtrl;
          spDrvCtrl = pDrvCtrl;
        }
      }

      if (err != NO_ERROR)
      {
        internalError = MemoryKernelUnmap(pHostControl, barSize);
        AHCI_ASSERT(internalError == NO_ERROR,
                    "Failed to unmap AHCI BAR.",
                    internalError);
      }
    }
  }
  else
  {
    err = ERR_INVALID_PARAMETER;
  }

  err = NO_ERROR;

  return err;
}

static bool _AHCIInterruptHandler(void)
{
  S_AHCIControler*      pDrvCtrl;
  S_KernelThread*       pCurrentThread;
  S_GenericHostControl* pHostControl;
  uint32_t              status;
  uint8_t               portIndex;

  /* Get the current thread */
  pCurrentThread = GetCurrentThread();
  status         = CPUGetContextInterruptNumber(pCurrentThread);

  /* Find which controller generated the interrupt */
  pDrvCtrl = spDrvCtrl;
  while (pDrvCtrl != NULL)
  {
    if (pDrvCtrl->pDevice->interruptLine == status)
    {
      /* Set EOI */
      InterruptSetEOI(pDrvCtrl->pDevice->interruptLine);
      break;
    }
    pDrvCtrl = pDrvCtrl->pNext;
  }

  if (pDrvCtrl != NULL)
  {
    /* Get the gorst host control structure */
    pHostControl = (S_GenericHostControl*)pDrvCtrl->pBARBaseAddr;
    status = _MMIORead32(&pHostControl->interruptStatus);
    if (status != 0)
    {
      /* Search for the ports that generated the interrupt */
      for (portIndex = 0; portIndex < AHCI_MAX_PORTS; ++portIndex)
      {
        if (status & (1 << portIndex))
        {
          KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                       MODULE_NAME,
                       "Port %d: Interrupt received.",
                       portIndex);
          if (pDrvCtrl->ports[portIndex].type != AHCI_PORT_TYPE_NONE)
          {
            _AHCIHandlePortInterrupt(pDrvCtrl, portIndex);
          }
          /* Clear the interrupt status */
          _MMIOWrite32(&pHostControl->interruptStatus, (1 << portIndex));
        }
      }
    }
  }

  return false;
}

static void _AHCIHandlePortInterrupt(S_AHCIControler* pDrvCtrl,
                                     const uint8_t    kPortIndex)
{
  S_AHCIPortControler* pPortCtrl;
  uint32_t             status;

  pPortCtrl = &pDrvCtrl->ports[kPortIndex];

  if (pPortCtrl->type == AHCI_PORT_TYPE_SATA)
  {
    _AHCIHandleSATAPortInterrupt(pPortCtrl);
  }
  else
  {
    /* Lock the port control structure */
    KERNEL_LOCK(pPortCtrl->lock);

    /* Clear the interrupt status for the port */
    status = _MMIORead32(&pPortCtrl->pPort->interruptStatus);
    _MMIOWrite32(&pPortCtrl->pPort->interruptStatus, status);

    /* Unlock the port control structure */
    KERNEL_UNLOCK(pPortCtrl->lock);
  }
}

static void _AHCIHandleSATAPortInterrupt(S_AHCIPortControler* pPortCtrl)
{
  S_AHCIPort*     pPort;
  uint8_t         i;
  uint32_t        interruptStatus;
  uint32_t        registerValue;
  uint32_t        commandIssue;
  S_AHCICommand*  pCommand;
  E_AHCIOpResult  result;

  /* Lock the port control structure */
  KERNEL_LOCK(pPortCtrl->lock);

  pPort = pPortCtrl->pPort;
  interruptStatus = _MMIORead32(&pPort->interruptStatus);

  /* Check the interrupt type */
  result = AHCI_OP_ERROR;
  if (interruptStatus & AHCI_INTERRUPT_STATUS_DHRS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DHRS Interrupt received.");
    result = AHCI_OP_SUCCESS;
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_PSS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "PSS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_DSS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DSS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_SDBS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "SDBS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_UFS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "UFS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_DPS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DPS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_PCS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "PCS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_DMPS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DMPS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_PRCS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "PRCS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_IPMS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IPMS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_OFS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "OFS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_INFS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "INFS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_IFS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IFS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_HBDS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "HBDS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_HBFS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "HBFS Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_TFES)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "TFES Interrupt received.");
    /* TODO Handle */
  }
  if (interruptStatus & AHCI_INTERRUPT_STATUS_CPDS)
  {
    KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "CPDS Interrupt received.");
    /* TODO Handle */
  }

  /* Find the command that caused the interrupt */
  registerValue = _MMIORead32(&pPort->commandIssue);
  commandIssue  = pPortCtrl->issuedCommandTable & ~registerValue;

  for (i = 0; i < COMMAND_TABLE_COUNT; ++i)
  {
    if (commandIssue & (1 << i))
    {
      /* Command completed */
      KERNEL_DEBUG(AHCI_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "Port %d: Command in slot %d completed.",
                    pPortCtrl - spDrvCtrl->ports,
                    i);

      /* Get the command and update the result */
      pCommand = &pPortCtrl->commandWaitingTable[i];
      *(pCommand->pResult) = result;

      /* Make the slot available*/
      pPortCtrl->issuedCommandTable &= ~(1 << i);

      /* Wake up the thread waiting for this command */
      SetThreadToReady(pCommand->pThread);
    }
  }

  /* Clear the interrupt status */
  _MMIOWrite32(&pPort->interruptStatus, interruptStatus);

  /* Unlock the port control structure */
  KERNEL_UNLOCK(pPortCtrl->lock);
}

static void _WaitForAHCIRetry(S_AHCIPortControler* pPortCtrl,
                              S_KernelThread*      pCurThread)
{
  /* Add the port to the retry queue */
  KERNEL_LOCK(pPortCtrl->retryQueueLock);
  SetCurrentThreadToWaitingWithPriority(pPortCtrl->pRetryWaitingList,
                                        pCurThread->priority,
                                        false);
  KERNEL_UNLOCK(pPortCtrl->retryQueueLock);
}

static E_AHCIOpResult _ReadSATAPort(S_AHCIPortControler* pPortCtrl,
                                    const uintptr_t      kBufPhys[PRDT_COUNT],
                                    const uint64_t       kSectorStart,
                                    const size_t         kSectorCount,
                                    E_AHCIOpResult*      pAsyncResult)
{
  S_AHCIPort*          pPort;
  S_AHCICommandHeader* pCmdHeader;
  S_ACPICommandTable*  pCmdTable;
  S_FISRegisterH2D*    pCmdFIS;
  uint8_t              slotPort;
  uint32_t             actSlots;
  uint32_t             cmdIssue;
  uint32_t             cmdValue;
  E_AHCIOpResult       retCode;
  uint16_t             prdtCount;
  uint32_t             i;
  size_t               totalBytes;

  pPort = pPortCtrl->pPort;

  /* Check if the drive is ready */
  slotPort = COMMAND_TABLE_COUNT;
  cmdValue = _MMIORead32(&pPort->taskFileData);
  if ((cmdValue & (TFD_STATUS_BSY | TFD_STATUS_DRQ)) == 0)
  {
    /* Find a free slot in the command list */;
    actSlots = _MMIORead32(&pPort->sataActive);
    cmdIssue = _MMIORead32(&pPort->commandAndStatus);
    actSlots |= cmdIssue;
    for (slotPort = 0; slotPort < COMMAND_TABLE_COUNT; ++slotPort)
    {
      if ((actSlots & (1 << slotPort)) == 0 &&
          (pPortCtrl->issuedCommandTable & (1 << slotPort)) == 0)
      {
        break;
      }
    }
  }

  /* If no free slot is available, ask for retry */
  if (slotPort != COMMAND_TABLE_COUNT)
  {
    totalBytes = kSectorCount * pPortCtrl->devInfo.satadInfo.sectorSize;
    /* Calculate how many PRDT entries are needed */
    prdtCount = totalBytes / PRDT_ENTRY_MAX_SIZE;
    if (totalBytes % PRDT_ENTRY_MAX_SIZE != 0)
    {
      prdtCount++;
    }

    /* Setup the command header */
    pCmdHeader = (S_AHCICommandHeader*)pPortCtrl->pMem;
    cmdValue = sizeof(S_FISRegisterH2D) / sizeof(uint32_t) |
                COMMAND_HEADER_CONFIG_READ;
    _MMIOWrite32(&pCmdHeader[slotPort].config,
                 cmdValue | (prdtCount << AHCI_COMMAND_HEADER_PRDTL_SHIFT));
    _MMIOWrite32(&pCmdHeader[slotPort].prdbc, 0);

    /* Setup the command table */
    pCmdTable = (S_ACPICommandTable*)((uintptr_t)pCmdHeader +
                                      COMMAND_LIST_SIZE +
                                      FIS_RECEIVE_AREA_SIZE +
                                      slotPort *
                                      sizeof(S_ACPICommandTable));
    for (i = 0; i < sizeof(S_ACPICommandTable) / sizeof(uint32_t); ++i)
    {
      _MMIOWrite32(((uint32_t*)pCmdTable) + i, 0);
    }

    /* Setup the full PRDTs */
    for (i = 0; i < (uint32_t)(prdtCount - 1); ++i)
    {
      _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaLow,
        (uint32_t)(kBufPhys[i] & 0xFFFFFFFF));
      _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaHigh,
        (uint32_t)((kBufPhys[i] >> 32) & 0xFFFFFFFF));
      _MMIOWrite32(&pCmdTable->prdtEntry[i].dbcIoc, PRDT_ENTRY_MAX_SIZE - 1);

      totalBytes -= PRDT_ENTRY_MAX_SIZE;
    }

    /* Setup the last PRDT */
    _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaLow,
      (uint32_t)(kBufPhys[i] & 0xFFFFFFFF));
    _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaHigh,
      (uint32_t)((kBufPhys[i] >> 32) & 0xFFFFFFFF));
    _MMIOWrite32(&pCmdTable->prdtEntry[i].dbcIoc,
                 (totalBytes - 1) | PRDT_ENTRY_IOC);

    /* Setup the command FIS */
    pCmdFIS = (S_FISRegisterH2D*)&pCmdTable->commandFIS;
    cmdValue = FIS_REGISTER_H2D |
               FIS_COMMAND_FIS  |
               (FIS_COMMAND_ATA_READ_DMA_EXT << FIS_COMMAND_SHIFT);
    _MMIOWrite32(&pCmdFIS->fisType, cmdValue);
    cmdValue = (kSectorStart & 0x00FFFFFF) |
               (FIS_ATA_DEVICE_LBA << FIS_DEVICE_SHIFT);
    _MMIOWrite32(&pCmdFIS->lbaLow, cmdValue);
    cmdValue = (kSectorStart >> 24) & 0x00FFFFFF;
    _MMIOWrite32(&pCmdFIS->lbaLowExp, cmdValue);
    cmdValue = kSectorCount & 0x0000FFFF;
    _MMIOWrite32(&pCmdFIS->countLow, cmdValue);
    _MMIOWrite32(&pCmdFIS->reserved1, 0);

    /* Add the port to the waiting table queue */
    pPortCtrl->issuedCommandTable |= (1 << slotPort);
    pPortCtrl->commandWaitingTable[slotPort].pResult = pAsyncResult;
    pPortCtrl->commandWaitingTable[slotPort].pThread = GetCurrentThread();
    pPortCtrl->commandWaitingTable[slotPort].type = AHCI_CMD_READ;

    /* Issue the command */
    _ClearAHCIPortError(pPort);
    _MMIOWrite32(&pPort->commandIssue, 1 << slotPort);

    retCode = AHCI_OP_WAIT;
  }
  else
  {
    retCode = AHCI_OP_RETRY;
  }

  return retCode;
}

static E_AHCIOpResult _WriteSATAPort(S_AHCIPortControler* pPortCtrl,
                                     const uintptr_t      kBufPhys[PRDT_COUNT],
                                     const uint64_t       kSectorStart,
                                     const size_t         kSectorCount,
                                     E_AHCIOpResult*      pAsyncResult)
{
  S_AHCIPort*          pPort;
  S_AHCICommandHeader* pCmdHeader;
  S_ACPICommandTable*  pCmdTable;
  S_FISRegisterH2D*    pCmdFIS;
  uint8_t              slotPort;
  uint32_t             actSlots;
  uint32_t             cmdIssue;
  uint32_t             cmdValue;
  E_AHCIOpResult       retCode;
  uint16_t             prdtCount;
  uint32_t             i;
  size_t               totalBytes;

  pPort = pPortCtrl->pPort;

  /* Check if the drive is ready */
  slotPort = COMMAND_TABLE_COUNT;
  cmdValue = _MMIORead32(&pPort->taskFileData);
  if ((cmdValue & (TFD_STATUS_BSY | TFD_STATUS_DRQ)) == 0)
  {
    /* Find a free slot in the command list */;
    actSlots = _MMIORead32(&pPort->sataActive);
    cmdIssue = _MMIORead32(&pPort->commandAndStatus);
    actSlots |= cmdIssue;
    for (slotPort = 0; slotPort < COMMAND_TABLE_COUNT; ++slotPort)
    {
      if ((actSlots & (1 << slotPort)) == 0 &&
          (pPortCtrl->issuedCommandTable & (1 << slotPort)) == 0)
      {
        break;
      }
    }
  }

  /* If no free slot is available, ask for retry */
  if (slotPort != COMMAND_TABLE_COUNT)
  {
    totalBytes = kSectorCount * pPortCtrl->devInfo.satadInfo.sectorSize;
    /* Calculate how many PRDT entries are needed */
    prdtCount = totalBytes / PRDT_ENTRY_MAX_SIZE;
    if (totalBytes % PRDT_ENTRY_MAX_SIZE != 0)
    {
      prdtCount++;
    }

    /* Setup the command header */
    pCmdHeader = (S_AHCICommandHeader*)pPortCtrl->pMem;
    cmdValue = sizeof(S_FISRegisterH2D) / sizeof(uint32_t) |
               COMMAND_HEADER_CONFIG_WRITE;
    _MMIOWrite32(&pCmdHeader[slotPort].config,
                 cmdValue | (prdtCount << AHCI_COMMAND_HEADER_PRDTL_SHIFT));
    _MMIOWrite32(&pCmdHeader[slotPort].prdbc, 0);

    /* Setup the command table */
    pCmdTable = (S_ACPICommandTable*)((uintptr_t)pCmdHeader +
                                      COMMAND_LIST_SIZE +
                                      FIS_RECEIVE_AREA_SIZE +
                                      slotPort *
                                      sizeof(S_ACPICommandTable));
    for (i = 0; i < sizeof(S_ACPICommandTable) / sizeof(uint32_t); ++i)
    {
      _MMIOWrite32(((uint32_t*)pCmdTable) + i, 0);
    }

    /* Setup the full PRDTs */
    for (i = 0; i < (uint32_t)(prdtCount - 1); ++i)
    {
      _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaLow,
        (uint32_t)(kBufPhys[i] & 0xFFFFFFFF));
      _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaHigh,
        (uint32_t)((kBufPhys[i] >> 32) & 0xFFFFFFFF));
      _MMIOWrite32(&pCmdTable->prdtEntry[i].dbcIoc, PRDT_ENTRY_MAX_SIZE - 1);

      totalBytes -= PRDT_ENTRY_MAX_SIZE;
    }

    /* Setup the last PRDT */
    _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaLow,
      (uint32_t)(kBufPhys[i] & 0xFFFFFFFF));
    _MMIOWrite32(&pCmdTable->prdtEntry[i].dbaHigh,
      (uint32_t)((kBufPhys[i] >> 32) & 0xFFFFFFFF));
    _MMIOWrite32(&pCmdTable->prdtEntry[i].dbcIoc,
                 (totalBytes - 1) | PRDT_ENTRY_IOC);

    /* Setup the command FIS */
    pCmdFIS = (S_FISRegisterH2D*)&pCmdTable->commandFIS;
    cmdValue = FIS_REGISTER_H2D |
               FIS_COMMAND_FIS  |
               FIS_COMMAND_ATA_WRITE_DMA_EXT;
    _MMIOWrite32(&pCmdFIS->fisType, cmdValue);
    cmdValue = (kSectorStart & 0x00FFFFFF) |
               (FIS_ATA_DEVICE_LBA << FIS_DEVICE_SHIFT);
    _MMIOWrite32(&pCmdFIS->lbaLow, cmdValue);
    cmdValue = (kSectorStart >> 24) & 0x00FFFFFF;
    _MMIOWrite32(&pCmdFIS->lbaLowExp, cmdValue);
    cmdValue = kSectorCount & 0x0000FFFF;
    _MMIOWrite32(&pCmdFIS->countLow, cmdValue);
    _MMIOWrite32(&pCmdFIS->reserved1, 0);

    /* Add the port to the waiting table queue */
    pPortCtrl->issuedCommandTable |= (1 << slotPort);
    pPortCtrl->commandWaitingTable[slotPort].pResult = pAsyncResult;
    pPortCtrl->commandWaitingTable[slotPort].pThread = GetCurrentThread();
    pPortCtrl->commandWaitingTable[slotPort].type = AHCI_CMD_WRITE;

    /* Issue the command */
    _ClearAHCIPortError(pPort);
    _MMIOWrite32(&pPort->commandIssue, 1 << slotPort);

    retCode = AHCI_OP_WAIT;
  }
  else
  {
    retCode = AHCI_OP_RETRY;
  }

  return retCode;
}

static E_AHCIOpResult _ReadAHCIPort(S_AHCIPortControler* pPortCtrl,
                                    const uintptr_t      kBufPhys[PRDT_COUNT],
                                    const uint64_t       kSectorStart,
                                    const size_t         kSectorCount,
                                    E_AHCIOpResult*      pAsyncResult)
{
  E_AHCIOpResult result;

  /* Check the port type */
  if (pPortCtrl->type == AHCI_PORT_TYPE_SATA)
  {
    result = _ReadSATAPort(pPortCtrl,
                           kBufPhys,
                           kSectorStart,
                           kSectorCount,
                           pAsyncResult);
  }
  else
  {
    result = AHCI_OP_ERROR;
  }

  return result;
}

static E_AHCIOpResult _WriteAHCIPort(S_AHCIPortControler* pPortCtrl,
                                     const uintptr_t      kBufPhys[PRDT_COUNT],
                                     const uint64_t       kSectorStart,
                                     const size_t         kSectorCount,
                                     E_AHCIOpResult*      pAsyncResult)
{
  E_AHCIOpResult result;

  /* Check the port type */
  if (pPortCtrl->type == AHCI_PORT_TYPE_SATA)
  {
    result = _WriteSATAPort(pPortCtrl,
                            kBufPhys,
                            kSectorStart,
                            kSectorCount,
                            pAsyncResult);
  }
  else
  {
    result = AHCI_OP_ERROR;
  }

  return result;
}

static ssize_t _AHCIPerformOperation(S_AHCIPortControler*  pPortCtrl,
                                     uintptr_t             pBuffers[PRDT_COUNT],
                                     const uint64_t        kSectorStart,
                                     const size_t          kSectorCount,
                                     const E_AHCIOperation kOperation)
{
  ssize_t          operationSize;
  E_AHCIOpResult   result;
  S_KernelThread*  pCurThread;
  E_AHCIOpResult   asyncResult;
  uint32_t         intState;

  /* Get the physical address of the buffer */
  pCurThread = GetCurrentThread();
  do
  {
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    /* Lock the port */
    KERNEL_LOCK(pPortCtrl->lock);

    if (kOperation == AHCI_OP_READ)
    {
      result = _ReadAHCIPort(pPortCtrl,
                              pBuffers,
                              kSectorStart,
                              kSectorCount,
                              &asyncResult);
    }
    else if (kOperation == AHCI_OP_WRITE)
    {
      result = _WriteAHCIPort(pPortCtrl,
                              pBuffers,
                              kSectorStart,
                              kSectorCount,
                              &asyncResult);
    }
    else
    {
      result = AHCI_OP_ERROR;
    }

    /* Check the result */
    if (result == AHCI_OP_WAIT)
    {
      /* Set the current thread to waiting */
      SetCurrentThreadToWaiting(NULL, false);

      /* Unlock the port */
      KERNEL_UNLOCK(pPortCtrl->lock);

      /* Schedule */
      CPUSaveContextAndSchedule(pCurThread->pVCpu);

      /* Get the aync result on return */
      result = asyncResult;
    }
    else if (result == AHCI_OP_RETRY)
    {
      /* Set the current thread to waiting */
      _WaitForAHCIRetry(pPortCtrl, pCurThread);

      /* Unlock the port */
      KERNEL_UNLOCK(pPortCtrl->lock);

      /* Schedule */
      CPUSaveContextAndSchedule(pCurThread->pVCpu);
    }
    else
    {
      /* Unlock the port */
      KERNEL_UNLOCK(pPortCtrl->lock);
    }

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
  } while (result == AHCI_OP_RETRY);

  if (result == AHCI_OP_SUCCESS)
  {
    operationSize = kSectorCount;
  }
  else
  {
    operationSize = -1;
  }

  return operationSize;
}

static bool _CheckAndPopulateParameters(void*          pDriverData,
                                        const void*    kpBuffer,
                                        const uint64_t kSectorStart,
                                        const size_t   kSectorCount,
                                        uintptr_t      pBuffers[PRDT_COUNT])
{
  bool                 isValid;
  S_AHCIPortControler* pPortCtrl;
  size_t               prdtCount;
  size_t               i;
  S_KernelProcess*     pCurrentProcess;
  size_t               endSector;
  size_t               totalBytes;

  isValid  = true;
  pPortCtrl = (S_AHCIPortControler*)pDriverData;

  if (pPortCtrl->type == AHCI_PORT_TYPE_SATA)
  {
    /* Check that pBuffer is aligned on 4K boundary */
    if (((uintptr_t)kpBuffer & (PRDT_ENTRY_MAX_SIZE - 1)) == 0)
    {
      endSector = kSectorStart + kSectorCount - 1;
      /* Check that the sector are within the valid range */
      if (endSector < kSectorStart ||
          endSector >= pPortCtrl->devInfo.satadInfo.totalSectors)
      {
        isValid = false;
      }

      totalBytes = kSectorCount * pPortCtrl->devInfo.satadInfo.sectorSize;

      /* Check that the amount of sectors can be read with the PRDT size*/
      if (totalBytes > PRDT_ENTRY_MAX_SIZE * PRDT_COUNT)
      {
        isValid = false;
      }

      prdtCount = totalBytes / PRDT_ENTRY_MAX_SIZE;
      if (totalBytes % PRDT_ENTRY_MAX_SIZE != 0)
      {
        prdtCount++;
      }
      if (prdtCount > PRDT_COUNT)
      {
        isValid = false;
      }
    }
    else
    {
      isValid = false;
    }
  }
  else
  {
    isValid = false;
  }

  if (isValid == true)
  {
    pCurrentProcess = GetCurrentProcess();
    /* Populate the buffer array with the addresses of the buffers */
    for (i = 0; i < prdtCount; ++i)
    {
      pBuffers[i] = MemoryMgrGetPhysAddr((uintptr_t)kpBuffer +
                                          i * PRDT_ENTRY_MAX_SIZE,
                                         pCurrentProcess,
                                         NULL);
      if (pBuffers[i] == 0)
      {
        isValid = false;
        break;
      }
    }
  }

  return isValid;
}

static ssize_t _AHCIRead(void*          pDriverData,
                         void*          pBuffer,
                         const uint64_t kSectorStart,
                         const size_t   kSectorCount)
{
  ssize_t   sectorsRead;
  bool      isValid;
  uintptr_t pBuffers[PRDT_COUNT];

  if (kSectorCount != 0)
  {
    /* Check the input parameters */
    isValid = _CheckAndPopulateParameters(pDriverData,
                                          pBuffer,
                                          kSectorStart,
                                          kSectorCount,
                                          pBuffers);
    if (isValid == true)
    {
      sectorsRead = _AHCIPerformOperation(pDriverData,
                                          pBuffers,
                                          kSectorStart,
                                          kSectorCount,
                                          AHCI_OP_READ);
    }
    else
    {
      sectorsRead = -1;
    }
  }
  else
  {
    sectorsRead = 0;
  }

  return sectorsRead;
}

static ssize_t _AHCIWrite(void*          pDriverData,
                          const void*    kpBuffer,
                          const uint64_t kSectorStart,
                          const size_t   kSectorCount)
{
  ssize_t   sectorsWrite;
  bool      isValid;
  uintptr_t pBuffers[PRDT_COUNT];

  if (kSectorCount != 0)
  {
    /* Check the input parameters */
    isValid = _CheckAndPopulateParameters(pDriverData,
                                          kpBuffer,
                                          kSectorStart,
                                          kSectorCount,
                                          pBuffers);
    if (isValid == true)
    {
      sectorsWrite = _AHCIPerformOperation(pDriverData,
                                          pBuffers,
                                          kSectorStart,
                                          kSectorCount,
                                          AHCI_OP_WRITE);
    }
    else
    {
      sectorsWrite = -1;
    }
  }
  else
  {
    sectorsWrite = 0;
  }

  return sectorsWrite;
}

static ssize_t _AHCIIOCTL(void*          pDriverData,
                          const uint32_t kCommand,
                          void*          pArgs)
{
  ssize_t result;

  (void)pArgs;

  switch (kCommand)
  {
    case VFS_IOCTL_DEV_GET_SECTOR_SIZE:
      result = _AHCIGetSectorSize(pDriverData);
      break;
    case VFS_IOCTL_DEV_GET_SECTOR_COUNT:
      result = _AHCIGetTotalSectors(pDriverData);
      break;
    default:
      result = -1;
      break;
  }

  return result;
}

static ssize_t _AHCIGetSectorSize(void* pDriverData)
{
  S_AHCIPortControler* pPortCtrl;
  ssize_t              sectorSize;

  pPortCtrl = (S_AHCIPortControler*)pDriverData;

  if (pPortCtrl->type == AHCI_PORT_TYPE_SATA)
  {
    sectorSize = pPortCtrl->devInfo.satadInfo.sectorSize;
  }
  else
  {
    sectorSize = -1;
  }

  return sectorSize;
}

static ssize_t _AHCIGetTotalSectors(void* pDriverData)
{
  S_AHCIPortControler* pPortCtrl;
  ssize_t              totalSectors;

  pPortCtrl = (S_AHCIPortControler*)pDriverData;

  if (pPortCtrl->type == AHCI_PORT_TYPE_SATA)
  {
    totalSectors = pPortCtrl->devInfo.satadInfo.totalSectors;
  }
  else
  {
    totalSectors = -1;
  }

  return totalSectors;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_PCI(sX86AHCIDriver);

/************************************ EOF *************************************/