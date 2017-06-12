/*-
 *   BSD LICENSE
 *
 *   Copyright (c) 2015 - 2016 CESNET
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of CESNET nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RTE_PMD_SZEDATA2_H_
#define RTE_PMD_SZEDATA2_H_

#include <stdbool.h>

#include <rte_byteorder.h>
#include <rte_io.h>

/* PCI Vendor ID */
#define PCI_VENDOR_ID_NETCOPE 0x1b26

/* PCI Device IDs */
#define PCI_DEVICE_ID_NETCOPE_COMBO80G 0xcb80
#define PCI_DEVICE_ID_NETCOPE_COMBO100G 0xc1c1
#define PCI_DEVICE_ID_NETCOPE_COMBO100G2 0xc2c1

/* number of PCI resource used by COMBO card */
#define PCI_RESOURCE_NUMBER 0

/* szedata2_packet header length == 4 bytes == 2B segment size + 2B hw size */
#define RTE_SZE2_PACKET_HEADER_SIZE 4

#define RTE_SZE2_MMIO_MAX 10

/*!
 * Round 'what' to the nearest larger (or equal) multiple of '8'
 * (szedata2 packet is aligned to 8 bytes)
 */
#define RTE_SZE2_ALIGN8(what) (((what) + ((8) - 1)) & (~((8) - 1)))

/*! main handle structure */
struct szedata {
	int fd;
	struct sze2_instance_info *info;
	uint32_t *write_size;
	void *space[RTE_SZE2_MMIO_MAX];
	struct szedata_lock lock[2][2];

	__u32 *rx_asize, *tx_asize;

	/* szedata_read_next variables - to keep context (ct) */

	/*
	 * rx
	 */
	/** initial sze lock ptr */
	const struct szedata_lock   *ct_rx_lck_orig;
	/** current sze lock ptr (initial or next) */
	const struct szedata_lock   *ct_rx_lck;
	/** remaining bytes (not read) within current lock */
	unsigned int                ct_rx_rem_bytes;
	/** current pointer to locked memory */
	unsigned char               *ct_rx_cur_ptr;
	/**
	 * allocated buffer to store RX packet if it was split
	 * into 2 buffers
	 */
	unsigned char               *ct_rx_buffer;
	/** registered function to provide filtering based on hwdata */
	int (*ct_rx_filter)(u_int16_t hwdata_len, u_char *hwdata);

	/*
	 * tx
	 */
	/**
	 * buffer for tx - packet is prepared here
	 * (in future for burst write)
	 */
	unsigned char               *ct_tx_buffer;
	/** initial sze TX lock ptrs - number according to TX interfaces */
	const struct szedata_lock   **ct_tx_lck_orig;
	/** current sze TX lock ptrs - number according to TX interfaces */
	const struct szedata_lock   **ct_tx_lck;
	/** already written bytes in both locks */
	unsigned int                *ct_tx_written_bytes;
	/** remaining bytes (not written) within current lock */
	unsigned int                *ct_tx_rem_bytes;
	/** current pointers to locked memory */
	unsigned char               **ct_tx_cur_ptr;
	/** NUMA node closest to PCIe device, or -1 */
	int                         numa_node;
};

static inline uint32_t
szedata2_read32(const volatile void *addr)
{
	return rte_le_to_cpu_32(rte_read32(addr));
}

static inline void
szedata2_write32(uint32_t value, volatile void *addr)
{
	rte_write32(rte_cpu_to_le_32(value), addr);
}

enum szedata2_link_speed {
	SZEDATA2_LINK_SPEED_DEFAULT = 0,
	SZEDATA2_LINK_SPEED_10G,
	SZEDATA2_LINK_SPEED_40G,
	SZEDATA2_LINK_SPEED_100G,
};

enum szedata2_mac_check_mode {
	SZEDATA2_MAC_CHMODE_PROMISC       = 0x0,
	SZEDATA2_MAC_CHMODE_ONLY_VALID    = 0x1,
	SZEDATA2_MAC_CHMODE_ALL_BROADCAST = 0x2,
	SZEDATA2_MAC_CHMODE_ALL_MULTICAST = 0x3,
};

/*
 * Maximum possible number of MAC addresses (limited by IBUF status
 * register value MAC_COUNT which has 5 bits).
 */
#define SZEDATA2_IBUF_MAX_MAC_COUNT 32

/*
 * Structure describes IBUF address space
 */
struct szedata2_ibuf {
	/** Total Received Frames Counter low part */
	uint32_t trfcl; /**< 0x00 */
	/** Correct Frames Counter low part */
	uint32_t cfcl; /**< 0x04 */
	/** Discarded Frames Counter low part */
	uint32_t dfcl; /**< 0x08 */
	/** Counter of frames discarded due to buffer overflow low part */
	uint32_t bodfcl; /**< 0x0C */
	/** Total Received Frames Counter high part */
	uint32_t trfch; /**< 0x10 */
	/** Correct Frames Counter high part */
	uint32_t cfch; /**< 0x14 */
	/** Discarded Frames Counter high part */
	uint32_t dfch; /**< 0x18 */
	/** Counter of frames discarded due to buffer overflow high part */
	uint32_t bodfch; /**< 0x1C */
	/** IBUF enable register */
	uint32_t ibuf_en; /**< 0x20 */
	/** Error mask register */
	uint32_t err_mask; /**< 0x24 */
	/** IBUF status register */
	uint32_t ibuf_st; /**< 0x28 */
	/** IBUF command register */
	uint32_t ibuf_cmd; /**< 0x2C */
	/** Minimum frame length allowed */
	uint32_t mfla; /**< 0x30 */
	/** Frame MTU */
	uint32_t mtu; /**< 0x34 */
	/** MAC address check mode */
	uint32_t mac_chmode; /**< 0x38 */
	/** Octets Received OK Counter low part */
	uint32_t orocl; /**< 0x3C */
	/** Octets Received OK Counter high part */
	uint32_t oroch; /**< 0x40 */
	/** reserved */
	uint8_t reserved[60]; /**< 0x4C */
	/** IBUF memory for MAC addresses */
	uint32_t mac_mem[2 * SZEDATA2_IBUF_MAX_MAC_COUNT]; /**< 0x80 */
} __rte_packed;

/*
 * @return
 *     true if IBUF is enabled
 *     false if IBUF is disabled
 */
static inline bool
ibuf_is_enabled(const volatile struct szedata2_ibuf *ibuf)
{
	return ((szedata2_read32(&ibuf->ibuf_en) & 0x1) != 0) ? true : false;
}

/*
 * Enables IBUF.
 */
static inline void
ibuf_enable(volatile struct szedata2_ibuf *ibuf)
{
	szedata2_write32(szedata2_read32(&ibuf->ibuf_en) | 0x1, &ibuf->ibuf_en);
}

/*
 * Disables IBUF.
 */
static inline void
ibuf_disable(volatile struct szedata2_ibuf *ibuf)
{
	szedata2_write32(szedata2_read32(&ibuf->ibuf_en) & ~0x1,
			&ibuf->ibuf_en);
}

/*
 * @return
 *     true if ibuf link is up
 *     false if ibuf link is down
 */
static inline bool
ibuf_is_link_up(const volatile struct szedata2_ibuf *ibuf)
{
	return ((szedata2_read32(&ibuf->ibuf_st) & 0x80) != 0) ? true : false;
}

/*
 * @return
 *     MAC address check mode
 */
static inline enum szedata2_mac_check_mode
ibuf_mac_mode_read(const volatile struct szedata2_ibuf *ibuf)
{
	switch (szedata2_read32(&ibuf->mac_chmode) & 0x3) {
	case 0x0:
		return SZEDATA2_MAC_CHMODE_PROMISC;
	case 0x1:
		return SZEDATA2_MAC_CHMODE_ONLY_VALID;
	case 0x2:
		return SZEDATA2_MAC_CHMODE_ALL_BROADCAST;
	case 0x3:
		return SZEDATA2_MAC_CHMODE_ALL_MULTICAST;
	default:
		return SZEDATA2_MAC_CHMODE_PROMISC;
	}
}

/*
 * Writes "mode" in MAC address check mode register.
 */
static inline void
ibuf_mac_mode_write(volatile struct szedata2_ibuf *ibuf,
		enum szedata2_mac_check_mode mode)
{
	szedata2_write32((szedata2_read32(&ibuf->mac_chmode) & ~0x3) | mode,
			&ibuf->mac_chmode);
}

/*
 * Structure describes OBUF address space
 */
struct szedata2_obuf {
	/** Total Sent Frames Counter low part */
	uint32_t tsfcl; /**< 0x00 */
	/** Octets Sent Counter low part */
	uint32_t oscl; /**< 0x04 */
	/** Total Discarded Frames Counter low part */
	uint32_t tdfcl; /**< 0x08 */
	/** reserved */
	uint32_t reserved1; /**< 0x0C */
	/** Total Sent Frames Counter high part */
	uint32_t tsfch; /**< 0x10 */
	/** Octets Sent Counter high part */
	uint32_t osch; /**< 0x14 */
	/** Total Discarded Frames Counter high part */
	uint32_t tdfch; /**< 0x18 */
	/** reserved */
	uint32_t reserved2; /**< 0x1C */
	/** OBUF enable register */
	uint32_t obuf_en; /**< 0x20 */
	/** reserved */
	uint64_t reserved3; /**< 0x24 */
	/** OBUF control register */
	uint32_t ctrl; /**< 0x2C */
	/** OBUF status register */
	uint32_t obuf_st; /**< 0x30 */
} __rte_packed;

/*
 * @return
 *     true if OBUF is enabled
 *     false if OBUF is disabled
 */
static inline bool
obuf_is_enabled(const volatile struct szedata2_obuf *obuf)
{
	return ((szedata2_read32(&obuf->obuf_en) & 0x1) != 0) ? true : false;
}

/*
 * Enables OBUF.
 */
static inline void
obuf_enable(volatile struct szedata2_obuf *obuf)
{
	szedata2_write32(szedata2_read32(&obuf->obuf_en) | 0x1, &obuf->obuf_en);
}

/*
 * Disables OBUF.
 */
static inline void
obuf_disable(volatile struct szedata2_obuf *obuf)
{
	szedata2_write32(szedata2_read32(&obuf->obuf_en) & ~0x1,
			&obuf->obuf_en);
}

#endif
