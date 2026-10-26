/*******************************************************************************
 * @file NetManager.h
 *
 * @see NetManager.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 18/10/2026
 *
 * @version 1.0
 *
 * @brief Network manager.
 *
 * @details Network manager. The functions of this module allows
 * to abtract the use of the network devices and their drivers. The network
 * manager provides a simple interface to send and receive data through the
 * network devices.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __NET_NETMANAGER_H_
#define __NET_NETMANAGER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <VirtualFS.h>
#include <FastQueue.h>
#include <KernelError.h>
#include <KernelSemaphore.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Maximum length of a network interface name. */
#define NET_MGR_IFACE_NAME_MAX_LENGTH 32
/** @brief Maximum length of a network interface path. */
#define NET_MGR_MAX_FRAGMENT_COUNT 16

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Forward declaration of the network device structure. */
struct S_NetworkDevice;

/**
 * @brief Defines the up link operation for a network device.
 *
 * @details Defines the up link operation for a network device. This function
 * will be called when the network device is opened. It should perform any
 * necessary initialization or configuration to bring the device up and ready
 * for operation.
 *
 * @param[in, out] pDevice The device that is of type S_NetworkDevice.
 *
 * @return The function returns a non-negative value on success or a negative.
 */
typedef E_Return (*T_NetUpLink)(struct S_NetworkDevice* pDevice);

/**
 * @brief Defines the down link operation for a network device.
 *
 * @details Defines the down link operation for a network device. This function
 * will be called when the network device is closed. It should perform any
 * necessary cleanup or configuration to bring the device down and stop its
 * operation.
 *
 * @param[in, out] pDevice The device that is of type S_NetworkDevice.
 *
 * @return The function returns a non-negative value on success or a negative
 * value on error.
 */
typedef E_Return (*T_NetDownLink)(struct S_NetworkDevice* pDevice);

/**
 * @brief Defines the send operation for a network device.
 *
 * @details Defines the send operation for a network device. This function will
 * be called by the network manager when a send operation is requested on the
 * device. It should write data to the device from the provided buffer.
 *
 * @param[in, out] pDevice The device that is of type S_NetworkDevice.
 * @param[in] kpBuffer The buffer containing the bytes to write.
 * @param[in] kSize The number of bytes to write.
 * @param[in] kIsLastOfBurst A boolean indicating if this is the last packet of
 * a burst.
 * @param[in] kIsBlocking A boolean indicating if the operation will block in
 * the upper stages of the stack in case of no available resources.
 *
 * @return The function returns the number of bytes written or a negative
 * value on error.
 */
typedef ssize_t (*T_NetSend)(struct S_NetworkDevice* pDevice,
                             const void*             kpBuffer,
                             const size_t            kSize,
                             const bool              kIsLastOfBurst,
                             const bool              kIsBlocking);

/**
 * @brief Defines the control operation for a network device.
 *
 * @details Defines the control operation for a network device. This function
 * will be called by the network manager when a control operation is requested
 * on the device. It should perform the requested operation on the device.
 *
 * @param[in, out] pDevice The device that is of type S_NetworkDevice.
 * @param[in] kCommand The command to execute.
 * @param[in, out] pArg The argument for the command.
 *
 * @return The function returns a non-negative value on success or a negative
 * value on error.
 */
typedef ssize_t (*T_NetControl)(struct S_NetworkDevice* pDevice,
                                const uint32_t          kCommand,
                                void*                   pArg);

/** @brief A network packet buffer for internal driver use. */
typedef struct
{
  /** @brief The virtual address of the packet buffer. */
  uintptr_t virtAddr;
  /** @brief The physical address of the packet buffer. */
  uintptr_t physAddr;
} S_NetworkPacketBuffer;

typedef struct
{
  /** @brief The up link operation for the network device. */
  T_NetUpLink pUplink;
  /** @brief The down link operation for the network device. */
  T_NetDownLink pDownlink;
  /** @brief The send operation for the network device. */
  T_NetSend pSend;
  /** @brief The control operation for the network device. */
  T_NetControl pControl;
} S_NetworkOperations;

/** @brief A network device structure. */
typedef struct S_NetworkDevice
{
  /** @brief The name of the network device. */
  char pName[NET_MGR_IFACE_NAME_MAX_LENGTH];
  /** @brief The free buffers queue. */
  S_FastQueue* pFreeBufferQueue;
  /** @brief The network device driver data. */
  void* pDriverData;
  /** @brief The network device operations. */
  S_NetworkOperations operations;

  /** @brief The memory pool allocated for the network device. */
  void* pMemoryPool;
  /** @brief The size of the memory pool. */
  size_t memoryPoolSize;

  /** @brief The size of each buffer in the pool. */
  size_t bufferSize;

  /** @brief The buffer pool for the network device. */
  void* pBufferPool;
  /** @brief The size of the buffer pool. */
  size_t bufferPoolSize;
  /** @brief The number of buffers in the buffer pool. */
  size_t bufferCount;

  /** @brief The reserved buffer pool for the network device. */
  void* pReservedBufferPool;
  /** @brief The size of the reserved buffer pool. */
  size_t reservedBufferPoolSize;
  /** @brief The number of reserved buffers. */
  size_t reservedBufferCount;

  /** @brief The VFS driver for the network device. */
  T_VFSDriver fsDriver;
} S_NetworkDevice;

/** @brief A network fragment structure. */
typedef struct
{
  /** @brief The network packet buffer. */
  S_NetworkPacketBuffer buffer;
  /** @brief The size of the fragment. */
  uint32_t  size;
} S_NetworkFragment;

/** @brief A network fragment list structure. */
typedef struct
{
  /** @brief The list of network fragments. */
  S_NetworkFragment pFragments[NET_MGR_MAX_FRAGMENT_COUNT];
  /** @brief The number of fragments in the list. */
  uint8_t fragmentCount;
} S_NetworkFragmentList;

/**
 * @brief Kernel network worker task type.
 *
 * @details Kernel network worker task type. A kernel worker task is a function
 * that is executed in the background by a kernel worker thread. The task is
 * executed with the provided argument.
 *
 * @param[in] pArg The argument to pass to the task function.
 */
typedef void (*T_NetManagerTask)(void* pArg);

/** @brief Network statistics structure. */
typedef struct
{
  /** @brief The number of received packets. */
  uint64_t rxPackets;
  /** @brief The number of transmitted packets. */
  uint64_t txPackets;
  /** @brief The number of received bytes. */
  uint64_t rxBytes;
  /** @brief The number of transmitted bytes. */
  uint64_t txBytes;
  /** @brief The number of receive errors. */
  uint64_t rxErrors;
  /** @brief The number of transmit errors. */
  uint64_t txErrors;
  /** @brief The number of received dropped packets. */
  uint64_t rxDropped;
  /** @brief The number of transmitted dropped packets. */
  uint64_t txDropped;
  /** @brief The number of multicast packets received. */
  uint64_t multicast;
  /** @brief The number of collisions detected. */
  uint64_t collisions;
  /** @brief The number of receive length errors. */
  uint64_t rxLengthErrors;
  /** @brief The number of receive overflow errors. */
  uint64_t rxOverErrors;
  /** @brief The number of receive CRC errors. */
  uint64_t rxCrcErrors;
  /** @brief The number of receive frame errors. */
  uint64_t rxFrameErrors;
  /** @brief The number of receive FIFO errors. */
  uint64_t rxFifoErrors;
  /** @brief The number of receive missed errors. */
  uint64_t rxMissedErrors;
  /** @brief The number of transmit aborted errors. */
  uint64_t txAbortedErrors;
  /** @brief The number of transmit carrier errors. */
  uint64_t txCarrierErrors;
  /** @brief The number of transmit FIFO errors. */
  uint64_t txFifoErrors;
  /** @brief The number of transmit heartbeat errors. */
  uint64_t txHeartbeatErrors;
  /** @brief The number of transmit window errors. */
  uint64_t txWindowErrors;
  /** @brief The number of receive compressed packets. */
  uint64_t rxCompressed;
  /** @brief The number of transmit compressed packets. */
  uint64_t txCompressed;
  /** @brief The number of received packets with no handler. */
  uint64_t rxNoHandler;
} S_NetworkStatistics;

/** @brief Network link speed. */
typedef enum
{
  /** @brief Link Speed: Auto-Negotiation */
  LINK_SPEED_AUTONEG,
  /** @brief Link Speed: 10 Mbps */
  LINK_SPEED_10MBPS,
  /** @brief Link Speed: 100 Mbps */
  LINK_SPEED_100MBPS,
  /** @brief Link Speed: 1 Gbps */
  LINK_SPEED_1GBPS,
  /** @brief Link Speed: 2.5 Gbps */
  LINK_SPEED_2_5GBPS,
  /** @brief Link Speed: 5 Gbps */
  LINK_SPEED_5GBPS,
  /** @brief Link Speed: 10 Gbps */
  LINK_SPEED_10GBPS
} E_NetworkLinkSpeed;

/** @brief Network duplex type. */
typedef enum
{
  /** @brief Network Duplex Type: Auto-Negotiation */
  DUPLEX_AUTONEG,
  /** @brief Network Duplex Type: Half Duplex */
  DUPLEX_HALF,
  /** @brief Network Duplex Type: Full Duplex */
  DUPLEX_FULL
} E_NetworkDuplexType;

/** @brief Network link configuration structure. */
typedef struct
{
  /** @brief The link speed of the network device. */
  E_NetworkLinkSpeed linkSpeed;
  /** @brief The duplex type of the network device. */
  E_NetworkDuplexType duplexType;
} S_NetworkLinkConfig;

/** @brief Type for a network manager task. */
typedef void* T_NetManagerTaskHandle;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * @brief Initialize the network manager.
 *
 * @details This function initializes the network manager and prepares it for
 * use. This will create the knetd kernel worker thread and initialize the
 * network manager data structures. This function should be called during the
 * kernel initialization process.
 */
void NetManagerInit(void);

/**
 * @brief Create a buffer pool for a network device.
 *
 * @details This function creates a buffer pool for the specified network
 * device. The buffer pool is used to manage the memory buffers for the network
 * device. This will create a reserved buffer pool for the device network
 * manager. The reserved buffer pool is used to manage the memory buffers for
 * the network device.
 *
 * @param[in] kBufferSize The size of each buffer in the pool.
 * @param[in] kBufferCount The number of buffers in the pool.
 * @param[in] kAlignment The alignment of each buffer in the pool.
 * @param[in, out] pDevice The network device for which to create the buffer
 * pool.
 *
 * @return The result of the operation.
 */
E_Return NetManagerCreateBufferPool(const size_t     kBufferSize,
                                    const size_t     kBufferCount,
                                    const uint32_t   kAlignment,
                                    S_NetworkDevice* pDevice);

/**
 * @brief Release a buffer pool for a network device.
 *
 * @details This function releases the buffer pool for the specified network
 * device.
 *
 * @param[in, out] pDevice The network device for which to destroy the buffer
 * pool.
 *
 * @return The result of the operation.
 */
E_Return NetManagerReleaseBufferPool(S_NetworkDevice* pDevice);

/**
 * @brief Register a network device with the network manager.
 *
 * @details This function registers a network device with the network manager.
 * The network manager will manage the device and provide an interface to
 * access it.
 *
 * @param[in] pUplink The up link operation for the network device.
 * @param[in] pDownlink The down link operation for the network device.
 * @param[in] pSend The send operation for the network device.
 * @param[in] pCtrl The control operation for the network device.
 * @param[in] kBus The bus number of the PCI device.
 * @param[in] kDevice The device number of the PCI device.
 * @param[in] kFunction The function number of the PCI device.
 * @param[in] pDriverData The driver data for the network device.
 * @param[out] pDevice Pointer to the network device structure to be filled
 * by the function.
 *
 * @return The result of the operation.
 */
E_Return NetManagerRegisterDevice(T_NetUpLink      pUplink,
                                  T_NetDownLink    pDownlink,
                                  T_NetSend        pSend,
                                  T_NetControl     pCtrl,
                                  const uint8_t    kBus,
                                  const uint8_t    kDevice,
                                  const uint8_t    kFunction,
                                  void*            pDriverData,
                                  S_NetworkDevice* pDevice);

/**
 * @brief Allocate a fragment list for a network packet.
 *
 * @details This function allocates a fragment list for a network packet.
 *
 * @return A pointer to the allocated fragment list, or NULL if allocation
 * fails.
 */
S_NetworkFragmentList* NetManagerAllocateFragmentList(void);

/**
 * @brief Process a received network packet.
 *
 * @details This function processes a received network packet by handling its
 * fragments.
 *
 * @param[in] pFragmentList The fragment list representing the received packet.
 * @param[in] pDevice The network device that received the packet.
 */
void NetManagerProcessReceivedPacket(S_NetworkFragmentList* pFragmentList,
                                     S_NetworkDevice*       pDevice);

/**
 * @brief Free a fragment list for a network packet.
 *
 * @details This function frees a fragment list for a network packet.
 *
 * @param[in] pFragmentList The fragment list to be freed.
 * @param[in] pDevice The network device associated with the fragment list.
 */
void NetManagerFreeFragmentList(S_NetworkFragmentList* pFragmentList,
                                S_NetworkDevice*       pDevice);

/**
 * @brief Submits a task to the kernel network worker manager.
 *
 * @details Submits a task to the kernel worker manager. The task will be
 * executed in the background by one of the kernel worker threads.
 *
 * @param[in] task The task to submit.
 * @param[in] pArg The argument to pass to the task function.
 * @param[in] pCompletionSemaphore The semaphore to signal when the task is
 * complete. Can be NULL if no completion signal is needed.
 *
 */
void NetManagerSubmitTask(T_NetManagerTask   task,
                          void*              pArg,
                          S_KernelSemaphore* pCompletionSemaphore);

/**
 * @brief Release a transmit buffer back to the network stack.
 *
 * @details This function releases a transmit buffer back to the network stack.
 * It is used to return a buffer that was previously allocated for transmission
 * back to the network stack's buffer pool.
 *
 * @param[in] pBufferAddress The address of the buffer to be released.
 * @param[in] kBufferSize The size of the buffer to be released.
 */
void NetManagerReleaseTransmitBuffer(void*        pBufferAddress,
                                     const size_t kBufferSize);

/**
 * @brief Add a periodic task to the kernel network worker manager.
 *
 * @details Adds a periodic task to the kernel worker manager. The task will be
 * executed in the background by one of the kernel worker threads at the
 * specified period.
 *
 * @param[in] task The task to add.
 * @param[in] pArg The argument to pass to the task function.
 * @param[in] kPeriodNs The period in nanoseconds at which the task should be
 * executed.
 *
 * @return A handle to the added task, which can be used to remove it later.
 */
T_NetManagerTaskHandle NetManagerAddPeriodicTask(T_NetManagerTask task,
                                                 void*            pArg,
                                                 const uint32_t   kPeriodNs);

/**
 * @brief Remove a periodic task from the kernel network worker manager.
 *
 * @details Removes a periodic task from the kernel worker manager. The task
 * will no longer be executed in the background by the kernel worker threads.
 *
 * @param[in] taskHandle The handle of the task to remove, as returned by
 * NetManagerAddPeriodicTask.
 */
void NetManagerRemovePeriodicTask(T_NetManagerTaskHandle taskHandle);

#endif /* #ifndef __NET_NETMANAGER_H_ */

/************************************ EOF *************************************/