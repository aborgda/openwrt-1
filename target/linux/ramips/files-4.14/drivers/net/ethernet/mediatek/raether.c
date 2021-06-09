#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/tcp.h>
#include <linux/phy.h>
#include <linux/of_mdio.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_net.h>

#include "raether.h"
#include "ra_mac.h"
#include "ra_eth.h"
#include "ra_phy.h"
#include "ra_ioctl.h"
#include "mii_mgr.h"
#if defined (CONFIG_RAETH_ESW_CONTROL)
#include "mtk_esw/ioctl.h"
#endif
#if defined (CONFIG_RAETH_ESW) || defined (CONFIG_MT7530_GSW)
#include "ra_esw_base.h"
#endif
#if defined (CONFIG_ETHTOOL)
#include "ra_ethtool.h"
#endif
#if defined (CONFIG_RALINK_MT7621)
#include "ra_eth_mt7621.h"
#endif

#if defined (CONFIG_RAETH_CHECKSUM_OFFLOAD) && !defined (RAETH_SDMA)
static int hw_offload_csg = 1;
#if defined (CONFIG_RAETH_SG_DMA_TX)
static int hw_offload_gso = 1;
#if defined (CONFIG_RAETH_TSO)
static int hw_offload_tso = 1;
#endif
#endif
#endif

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)

typedef struct {
	uint16_t MAGIC_TAG;
	uint32_t FOE_Entry:14;
#if defined (CONFIG_RALINK_MT7620)
	uint32_t CRSN:5;
	uint32_t SPORT:3;
	uint32_t ALG:10;
#elif defined (CONFIG_RALINK_MT7621)
	uint32_t CRSN:5;
	uint32_t SPORT:4;
	uint32_t ALG:9;
#else
	uint32_t FVLD:1;
	uint32_t ALG:1;
	uint32_t AI:8;
	uint32_t SP:3;
	uint32_t AIS:1;
	uint32_t RESV2:4;
#endif
}  __attribute__ ((packed)) PdmaRxDescInfo4;

#define FOE_MAGIC_GE		    0x7275
#define FOE_MAGIC_PPE_DWORD	    0x3fff7276UL	/* HNAT_V1: FVLD=0, HNAT_V2: FOE_Entry=0x3fff */
#define FOE_INFO_START_ADDR(skb)    (skb->head)
#define FOE_MAGIC_TAG(skb)	    ((PdmaRxDescInfo4 *)((skb)->head))->MAGIC_TAG
#define DO_FILL_FOE_DESC(skb,desc)  (*(uint32_t *)(FOE_INFO_START_ADDR(skb)+2) = (uint32_t)(desc))
#define IS_DPORT_PPE_VALID(skb)	    (*(uint32_t *)(FOE_INFO_START_ADDR(skb)) == FOE_MAGIC_PPE_DWORD)

#define FOE_4TB_SIZ		16384

enum DstPort {
	DP_RA0 = 11,
	DP_RA1 = 12,
	DP_RA2 = 13,
	DP_RA3 = 14,
	DP_RA4 = 15,
	DP_RA5 = 16,
	DP_RA6 = 17,
	DP_RA7 = 18,
	DP_RA8 = 19,
	DP_RA9 = 20,
	DP_RA10 = 21,
	DP_RA11 = 22,
	DP_RA12 = 23,
	DP_RA13 = 24,
	DP_RA14 = 25,
	DP_RA15 = 26,
	DP_WDS0 = 27,
	DP_WDS1 = 28,
	DP_WDS2 = 29,
	DP_WDS3 = 30,
	DP_APCLI0 = 31,
	DP_MESH0 = 32,
	DP_RAI0 = 33,
	DP_RAI1 = 34,
	DP_RAI2 = 35,
	DP_RAI3 = 36,
	DP_RAI4 = 37,
	DP_RAI5 = 38,
	DP_RAI6 = 39,
	DP_RAI7 = 40,
	DP_RAI8 = 41,
	DP_RAI9 = 42,
	DP_RAI10 = 43,
	DP_RAI11 = 44,
	DP_RAI12 = 45,
	DP_RAI13 = 46,
	DP_RAI14 = 47,
	DP_RAI15 = 48,
	DP_WDSI0 = 49,
	DP_WDSI1 = 50,
	DP_WDSI2 = 51,
	DP_WDSI3 = 52,
	DP_APCLII0 = 53,
	DP_MESHI0 = 54,
	MAX_WIFI_IF_NUM = 55,
	DP_GMAC1 = 60,
	DP_GMAC2 = 61,
	DP_NIC0 = 62,
	DP_NIC1 = 63,
	MAX_IF_NUM // MAX_IF_NUM = 64 entries (act_dp length 6bits)
};

/* state = unbind & dynamic */
struct ud_info_blk1 {
	uint32_t time_stamp:8;
	uint32_t pcnt:16;	/* packet count */
	uint32_t preb:1;
	uint32_t pkt_type:3;
	uint32_t state:2;
	uint32_t udp:1;
	uint32_t sta:1;		/* static entry */
};

/* state = bind & fin */
struct bf_info_blk1 {
	uint32_t time_stamp:15;
	uint32_t ka:1;		/* keep alive */
	uint32_t vlan_layer:3;
	uint32_t psn:1;		/* egress packet has PPPoE session */
#if defined (CONFIG_RALINK_MT7621)
	uint32_t vpm:2;		/* 0:ethertype remark, 1:0x8100, 2:0x88a8 */
#else
	uint32_t dvp:1;		/* inform switch of keeping VPRI */
	uint32_t drm:1;		/* inform switch of keeping DSCP(IPv4) or TC(IPv6) */
#endif
	uint32_t cah:1;		/* cacheable flag */
	uint32_t rmt:1;		/* remove tunnel ip header (6rd/dslite only) */
	uint32_t ttl:1;
	uint32_t pkt_type:3;
	uint32_t state:2;
	uint32_t udp:1;
	uint32_t sta:1;		/* static entry */
};

struct _info_blk2 {
#if defined (CONFIG_RALINK_MT7621)
	uint32_t qid:4;		/* QID in Qos Port */
	uint32_t fqos:1;	/* force to PSE QoS port */
	uint32_t dp:3;		/* force to PSE port x 0:PSE, 1:GSW, 2:GMAC, 4:PPE, 5:QDMA, 7:DROP */
	uint32_t mcast:1;	/* multicast this packet to CPU */
	uint32_t pcpl:1;	/* OSBN */
	uint32_t mlen:1;	/* 0:post 1:pre packet length in meter */
	uint32_t alen:1;	/* 0:post 1:pre packet length in accounting */
	uint32_t port_mg:6;	/* port meter group */
	uint32_t port_ag:6;	/* port account group */
	uint32_t dscp:8;	/* DSCP value */
#else
	uint32_t fpidx:4;	/* force port index */
	uint32_t fp:1;		/* force new user priority */
	uint32_t up:3;		/* new user priority */
	uint32_t fdq:4;		/* force DRAM queue (for CAR case) */
	uint32_t port_mg:6;	/* port meter group */
	uint32_t port_ag:6;	/* port account group */
	uint32_t dscp:8;	/* DSCP value */
#endif
};

/*
 * Foe Entry (80B: IPv6, 64B: IPv4)
 *
 *      IPV4 HNAPT:                  IPV4:
 *	+-----------------------+    +-----------------------+
 *	|  Information Block 1  |    |  Information Block 1  |
 *	+-----------------------+    +-----------------------+
 *	|         SIP(4B)       |    |        SIP(4B)        |
 *	+-----------------------+    +-----------------------+
 *	|         DIP(4B)       |    |        DIP(4B)        |
 *	+-----------+-----------+    +-----------------------+
 *	| SPORT(2B) | DPORT(2B) |    |         RESV          |
 *	+--------+--+-----------+    +-----------------------+
 *	| EG DSCP| Info Block 2 |    |  Information Block 2  |
 *	+--------+--------------+    +-----------------------+
 *	|      New SIP(4B)      |    |     New SIP (4B)      |
 *	+-----------------------+    +-----------------------+
 *	|      New DIP(4B)      |    |     New DIP (4B)      |
 *	+-----------+-----------+    +-----------+-----------+
 *	| New SPORT | New DPORT |    | New SPORT | New DPORT |
 *	+-----------+-----------+    +-----------+-----------+
 *	|         RESV          |    |         RESV          |
 *	+-----------------------+    +-----------------------+
 *	|         RESV          |    |         RESV          |
 *	+------+----------------+    +------+----------------+
 *	|Act_dp|      RESV      |    |Act_dp|      RESV      |
 *	+------+----+-----------+    +------+----+-----------+
 *	|   ETYPE   |   VLAN1   |    |   ETYPE   |   VLAN1   |
 *	+-----------+-----------+    +-----------+-----------+
 *	|       DMAC[47:16]     |    |       DMAC[47:16]     |
 *	+-----------+-----------+    +-----------+-----------+
 *	| DMAC[15:0]|   VLAN2   |    | DMAC[15:0]|   VLAN2   |
 *	+-----------+-----------+    +-----------+-----------+
 *	|       SMAC[47:16]     |    |       SMAC[47:16]     |
 *	+-----------+-----------+    +-----------+-----------+
 *	| SMAC[15:0]| PPPOE ID  |    | SMAC[15:0]| PPPOE ID  |
 *	+-----------+-----------+    +-----------+-----------+
 *
 */

struct _ipv4_hnapt {
	union {
		struct ud_info_blk1 udib1;
		struct bf_info_blk1 bfib1;
		uint32_t info_blk1;	// 1
	};

	uint32_t sip;			// 2
	uint32_t dip;			// 3
	uint16_t dport;
	uint16_t sport;			// 4

	union {
		struct _info_blk2 iblk2;
		uint32_t info_blk2;	// 5
	};

	uint32_t new_sip;		// 6
	uint32_t new_dip;		// 7

	uint16_t new_dport;
	uint16_t new_sport;		// 8

	uint32_t resv1;			// 9
	uint32_t resv2;			// 10
	uint32_t resv3:26;
	uint32_t act_dp:6;		// 11 (UDF)
	uint16_t vlan1;
	uint16_t etype;			// 12
	uint8_t dmac_hi[4];		// 13
	uint16_t vlan2;
	uint8_t dmac_lo[2];		// 14
	uint8_t smac_hi[4];		// 15
	uint16_t pppoe_id;
	uint8_t smac_lo[2];		// 16
};

struct _ipv4_dslite {
	union {
		struct ud_info_blk1 udib1;
		struct bf_info_blk1 bfib1;
		uint32_t info_blk1;	// 1
	};

	uint32_t sip;			// 2
	uint32_t dip;			// 3
	uint16_t dport;
	uint16_t sport;			// 4

	uint32_t tunnel_sipv6_0;	// 5
	uint32_t tunnel_sipv6_1;	// 6
	uint32_t tunnel_sipv6_2;	// 7
	uint32_t tunnel_sipv6_3;	// 8

	uint32_t tunnel_dipv6_0;	// 9
	uint32_t tunnel_dipv6_1;	// 10
	uint32_t tunnel_dipv6_2;	// 11
	uint32_t tunnel_dipv6_3;	// 12

	uint8_t flow_lbl[3];		/* in order to consist with Linux kernel (should be 20bits) */
	uint16_t priority:4;		/* in order to consist with Linux kernel (should be 8bits) */
	uint16_t resv:4;		// 13

	uint32_t hop_limit:8;
	uint32_t resv2:18;
	uint32_t act_dp:6;		// 14 (UDF)

	union {
		struct _info_blk2 iblk2;
		uint32_t info_blk2;	// 15
	};

	uint16_t vlan1;
	uint16_t etype;			// 16
	uint8_t dmac_hi[4];		// 17
	uint16_t vlan2;
	uint8_t dmac_lo[2];		// 18
	uint8_t smac_hi[4];		// 19
	uint16_t pppoe_id;
	uint8_t smac_lo[2];		// 20
};

struct _ipv6_3t_route {
	union {
		struct ud_info_blk1 udib1;
		struct bf_info_blk1 bfib1;
		uint32_t info_blk1;	// 1
	};

	uint32_t ipv6_sip0;		// 2
	uint32_t ipv6_sip1;		// 3
	uint32_t ipv6_sip2;		// 4
	uint32_t ipv6_sip3;		// 5

	uint32_t ipv6_dip0;		// 6
	uint32_t ipv6_dip1;		// 7
	uint32_t ipv6_dip2;		// 8
	uint32_t ipv6_dip3;		// 9

	uint32_t prot:8;
	uint32_t resv:24;		// 10

	uint32_t resv1;			// 11
	uint32_t resv2;			// 12
	uint32_t resv3;			// 13

	uint32_t resv4:26;
	uint32_t act_dp:6;		// 14 (UDF)

	union {
		struct _info_blk2 iblk2;
		uint32_t info_blk2;	// 15
	};

	uint16_t vlan1;
	uint16_t etype;			// 16
	uint8_t dmac_hi[4];		// 17
	uint16_t vlan2;
	uint8_t dmac_lo[2];		// 18
	uint8_t smac_hi[4];		// 19
	uint16_t pppoe_id;
	uint8_t smac_lo[2];		// 20
};

struct _ipv6_5t_route {
	union {
		struct ud_info_blk1 udib1;
		struct bf_info_blk1 bfib1;
		uint32_t info_blk1;	// 1
	};

	uint32_t ipv6_sip0;		// 2
	uint32_t ipv6_sip1;		// 3
	uint32_t ipv6_sip2;		// 4
	uint32_t ipv6_sip3;		// 5

	uint32_t ipv6_dip0;		// 6
	uint32_t ipv6_dip1;		// 7
	uint32_t ipv6_dip2;		// 8
	uint32_t ipv6_dip3;		// 9

	uint16_t dport;
	uint16_t sport;			// 10

	uint32_t resv1;			// 11
	uint32_t resv2;			// 12
	uint32_t resv3;			// 13

	uint32_t resv4:26;
	uint32_t act_dp:6;		// 14 (UDF)

	union {
		struct _info_blk2 iblk2;
		uint32_t info_blk2;	// 15
	};

	uint16_t vlan1;
	uint16_t etype;			// 16
	uint8_t dmac_hi[4];		// 17
	uint16_t vlan2;
	uint8_t dmac_lo[2];		// 18
	uint8_t smac_hi[4];		// 19
	uint16_t pppoe_id;
	uint8_t smac_lo[2];		// 20
};

struct _ipv6_6rd {
	union {
		struct ud_info_blk1 udib1;
		struct bf_info_blk1 bfib1;
		uint32_t info_blk1;	// 1
	};

	uint32_t ipv6_sip0;		// 2
	uint32_t ipv6_sip1;		// 3
	uint32_t ipv6_sip2;		// 4
	uint32_t ipv6_sip3;		// 5

	uint32_t ipv6_dip0;		// 6
	uint32_t ipv6_dip1;		// 7
	uint32_t ipv6_dip2;		// 8
	uint32_t ipv6_dip3;		// 9

	uint16_t dport;
	uint16_t sport;			// 10

	uint32_t tunnel_sipv4;		// 11
	uint32_t tunnel_dipv4;		// 12

	uint32_t hdr_chksum:16;
	uint32_t dscp:8;
	uint32_t ttl:8;			// 13

	uint32_t flag:3;
	uint32_t resv1:23;
	uint32_t act_dp:6;		// 14 (UDF)

	union {
		struct _info_blk2 iblk2;
		uint32_t info_blk2;	// 15
	};

	uint16_t vlan1;
	uint16_t etype;			// 16
	uint8_t dmac_hi[4];		// 17
	uint16_t vlan2;
	uint8_t dmac_lo[2];		// 18
	uint8_t smac_hi[4];		// 19
	uint16_t pppoe_id;
	uint8_t smac_lo[2];		// 20
};

struct FoeEntry {
	union {
		struct ud_info_blk1 udib1;
		struct bf_info_blk1 bfib1;	// common header
		struct _ipv4_hnapt ipv4_hnapt;	// nat & napt share same data structure
		struct _ipv4_dslite ipv4_dslite;
		struct _ipv6_3t_route ipv6_3t_route;
		struct _ipv6_5t_route ipv6_5t_route;
		struct _ipv6_6rd ipv6_6rd;
	};
};

struct FoeEntry *PpeFoeBase = NULL;
dma_addr_t PpeFoeBasePhy = 0;
int (*ra_sw_nat_hook_rx)(struct sk_buff *skb) = NULL;
int (*ra_sw_nat_hook_tx)(struct sk_buff *skb, int gmac_no) = NULL;
int (*ra_sw_nat_hook_rs)(struct net_device *dev, int hold) = NULL;
int (*ra_sw_nat_hook_ec)(int engine_init) = NULL;
EXPORT_SYMBOL(ra_sw_nat_hook_rx);
EXPORT_SYMBOL(ra_sw_nat_hook_tx);
EXPORT_SYMBOL(ra_sw_nat_hook_rs);
EXPORT_SYMBOL(ra_sw_nat_hook_ec);
#endif

#if defined (CONFIG_RAETH_READ_MAC_FROM_MTD)
extern int ra_mtd_read_nm(char *name, loff_t from, size_t len, u_char *buf);
#endif

#if defined (CONFIG_VLAN_8021Q_DOUBLE_TAG)
extern int vlan_double_tag;
#endif

extern u32 ralink_asic_rev_id;

struct net_device *dev_raether = NULL;

//////////////////////////////////////////////////////////////

#if defined (CONFIG_RAETH_QDMA)
#include "raether_qdma.c"
#else
#include "raether_pdma.c"
#endif

//////////////////////////////////////////////////////////////

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
struct FoeEntry *
get_foe_table(dma_addr_t *dma_handle, uint32_t *FoeTblSize)
{
	if (dma_handle)
		*dma_handle = PpeFoeBasePhy;

	if (FoeTblSize)
		*FoeTblSize = FOE_4TB_SIZ;

	return PpeFoeBase;
}
EXPORT_SYMBOL(get_foe_table);

static void
foe_dma_table_free(void)
{
	/* free PPE FoE table */
	if (PpeFoeBase) {
		dma_free_coherent(NULL, FOE_4TB_SIZ * sizeof(struct FoeEntry), PpeFoeBase, PpeFoeBasePhy);
		PpeFoeBase = NULL;
		PpeFoeBasePhy = 0;
	}
}

static void
foe_dma_table_alloc(void)
{
	/* PPE FoE Table */
	PpeFoeBase = (struct FoeEntry *)dma_alloc_coherent(NULL, FOE_4TB_SIZ * sizeof(struct FoeEntry), &PpeFoeBasePhy, GFP_KERNEL);
}
#endif

#if defined (CONFIG_RAETH_HW_VLAN_TX) && !defined (RAETH_HW_VLAN4K)
static void
fill_hw_vlan_tx_map(END_DEVICE *ei_local)
{
	u32 i;

	/* init vlan_4k map table by index 15 */
	memset(ei_local->vlan_4k_map, 0x0F, sizeof(ei_local->vlan_4k_map));

	for (i = 0; i < 16; i++) {
		ei_local->vlan_id_map[i] = (u16)i;
		ei_local->vlan_4k_map[i] = (u8)i;
	}

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
	/* map VLAN TX for external offload (use slots 11..15) */
	i = 11;
#if defined (CONFIG_RA_HW_NAT_WIFI)
	ei_local->vlan_id_map[i] = (u16)DP_RA0;		// IDX: 11
	ei_local->vlan_4k_map[DP_RA0] = (u8)i;
	i++;
	ei_local->vlan_id_map[i] = (u16)DP_RA1;		// IDX: 12
	ei_local->vlan_4k_map[DP_RA1] = (u8)i;
	i++;
	ei_local->vlan_id_map[i] = (u16)DP_RAI0;	// IDX: 13
	ei_local->vlan_4k_map[DP_RAI0] = (u8)i;
	i++;
	ei_local->vlan_id_map[i] = (u16)DP_RAI1;	// IDX: 14
	ei_local->vlan_4k_map[DP_RAI1] = (u8)i;
	i++;
#endif
#if defined (CONFIG_RA_HW_NAT_PCI)
	ei_local->vlan_id_map[i] = (u16)DP_NIC0;	// IDX: 15
	ei_local->vlan_4k_map[DP_NIC0] = (u8)i;
#endif
#endif
}
#endif

static void
fill_dev_features(struct net_device *dev)
{
#if defined (RAETH_SDMA)

#if defined (CONFIG_RAETH_CHECKSUM_OFFLOAD)
	/* Can handle RX checksum */
	dev->hw_features |= NETIF_F_RXCSUM;
#endif

#else /* !RAETH_SDMA */

#if defined (CONFIG_RAETH_CHECKSUM_OFFLOAD)
	/* Can handle RX checksum */
	dev->hw_features |= NETIF_F_RXCSUM;
	if (hw_offload_csg) {
		/* Can generate TX checksum TCP/UDP over IPv4 */
		dev->hw_features |= NETIF_F_IP_CSUM;
	}
#if defined (CONFIG_RAETH_SG_DMA_TX)
	if (hw_offload_gso && hw_offload_csg) {
		dev->hw_features |= NETIF_F_SG;
#if defined (CONFIG_RAETH_TSO)
		if (hw_offload_tso) {
			/* Can TSOv4 */
			dev->hw_features |= NETIF_F_TSO;
#if defined (CONFIG_RAETH_TSOV6)
			/* Can TSOv6 + generate TX checksum TCP/UDP over IPv6 */
			dev->hw_features |= NETIF_F_TSO6;
			dev->hw_features |= NETIF_F_IPV6_CSUM; 
#endif
		}
#endif /* CONFIG_RAETH_TSO */
	}
#endif /* CONFIG_RAETH_SG_DMA_TX */
#endif /* CONFIG_RAETH_CHECKSUM_OFFLOAD */

#if defined (CONFIG_RAETH_HW_VLAN_RX)
	dev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
#endif

#if defined (CONFIG_RAETH_HW_VLAN_TX)
	dev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
#endif

#endif /* RAETH_SDMA */

	dev->features = dev->hw_features;
	dev->vlan_features = dev->features & ~(NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX);
}

static netdev_features_t
ei_fix_features(struct net_device *dev, netdev_features_t features)
{
#if defined (RAETH_SDMA)

#if defined (CONFIG_RAETH_CHECKSUM_OFFLOAD)
	features |= NETIF_F_RXCSUM;
#endif

#else /* !RAETH_SDMA */

#if defined (CONFIG_RAETH_HW_VLAN_RX)
#if defined (CONFIG_VLAN_8021Q_DOUBLE_TAG)
	if (vlan_double_tag)
		features &= ~(NETIF_F_HW_VLAN_CTAG_RX);
	else
#endif
		features |= NETIF_F_HW_VLAN_CTAG_RX;
#endif

#if defined (CONFIG_RAETH_HW_VLAN_TX)
#if defined (CONFIG_VLAN_8021Q_DOUBLE_TAG)
	if (vlan_double_tag)
		features &= ~(NETIF_F_HW_VLAN_CTAG_TX);
	else
#endif
		features |= NETIF_F_HW_VLAN_CTAG_TX;
#endif

#if defined (CONFIG_RAETH_CHECKSUM_OFFLOAD)
	features |= NETIF_F_RXCSUM;
	if (hw_offload_csg)
		features |= NETIF_F_IP_CSUM;
	else
		features &= ~NETIF_F_IP_CSUM;
#if defined (CONFIG_RAETH_SG_DMA_TX)
	if (hw_offload_gso && hw_offload_csg) {
		features |= NETIF_F_SG;
#if defined (CONFIG_RAETH_TSO)
		if (hw_offload_tso) {
			features |= NETIF_F_TSO;
#if defined (CONFIG_RAETH_TSOV6)
			features |= NETIF_F_TSO6;
			features |= NETIF_F_IPV6_CSUM;
#endif
		} else {
			features &= ~(NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_IPV6_CSUM);
		}
#endif /* CONFIG_RAETH_TSO */
	} else {
		features &= ~(NETIF_F_SG | NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_IPV6_CSUM);
	}
#endif /* CONFIG_RAETH_SG_DMA_TX */

#else /* !CONFIG_RAETH_CHECKSUM_OFFLOAD */
	features &= ~(NETIF_F_IP_CSUM | NETIF_F_RXCSUM);
#endif

#endif /* RAETH_SDMA */

	return features;
}

static void
show_dev_features(struct net_device *dev)
{
	if (dev->features & NETIF_F_IP_CSUM)
		printk("%s: HW IP/TCP/UDP checksum %s offload enabled\n", RAETH_DEV_NAME, "RX/TX");
	else if (dev->features & NETIF_F_RXCSUM)
		printk("%s: HW IP/TCP/UDP checksum %s offload enabled\n", RAETH_DEV_NAME, "RX");

	if (dev->features & NETIF_F_HW_VLAN_CTAG_RX)
		printk("%s: HW VLAN %s offload enabled\n", RAETH_DEV_NAME, "RX");

	if (dev->features & NETIF_F_HW_VLAN_CTAG_TX)
		printk("%s: HW VLAN %s offload enabled\n", RAETH_DEV_NAME, "TX");

	if (dev->features & NETIF_F_SG)
		printk("%s: HW Scatter/Gather TX offload enabled\n", RAETH_DEV_NAME);

	if (dev->features & NETIF_F_TSO)
		printk("%s: HW TCP segmentation offload (TSO) enabled\n", RAETH_DEV_NAME);
}

#if !defined (RAETH_HW_PADPKT)
static void
calc_dev_min_pkt_len(struct net_device *dev, END_DEVICE *ei_local)
{
#if defined (RAETH_SDMA)
	/* pad to 64 bytes (with VLAN tag) */
	ei_local->min_pkt_len = VLAN_ETH_ZLEN;
#else
#if defined (CONFIG_PSEUDO_SUPPORT)
	/* pad to 60 bytes (no or stripped VLAN tag) */
	ei_local->min_pkt_len = ETH_ZLEN;
#else
	if (dev->features & NETIF_F_HW_VLAN_CTAG_TX) {
		/* pad to 60 bytes (stripped VLAN tag) */
		ei_local->min_pkt_len = ETH_ZLEN;
	} else {
		/* pad to 64 bytes (with VLAN tag) */
		ei_local->min_pkt_len = VLAN_ETH_ZLEN;
	}
#endif
#endif
}
#endif

static void
fetch_stat_counters(END_DEVICE *ei_local)
{
#if defined (CONFIG_PSEUDO_SUPPORT)
	PSEUDO_ADAPTER *pPseudoAd = netdev_priv(ei_local->PseudoDev);
#endif

	spin_lock(&ei_local->stat_lock);
	fe_gdm1_fetch_mib(&ei_local->stat);
#if defined (CONFIG_PSEUDO_SUPPORT)
	fe_gdm2_fetch_mib(&pPseudoAd->stat);
#endif
	spin_unlock(&ei_local->stat_lock);
}

static void
stat_wq_handler(struct work_struct *work)
{
	END_DEVICE *ei_local = container_of(work, END_DEVICE, stat_wq);

	fetch_stat_counters(ei_local);

	if (ei_local->active)
		mod_timer(&ei_local->stat_timer, jiffies + (25 * HZ));
}

static void
stat_timer_handler(unsigned long ptr)
{
	struct net_device *dev = (struct net_device *)ptr;
	END_DEVICE *ei_local = netdev_priv(dev);

	if (ei_local->active)
		schedule_work(&ei_local->stat_wq);
}

static void
inc_rx_drop(END_DEVICE *ei_local, int gmac_no)
{
#if defined (CONFIG_PSEUDO_SUPPORT)
	PSEUDO_ADAPTER *pAd;

	if (gmac_no == PSE_PORT_GMAC2) {
		pAd = netdev_priv(ei_local->PseudoDev);
		pAd->stat.rx_dropped++;
	} else
#endif
	if (gmac_no == PSE_PORT_GMAC1)
		ei_local->stat.rx_dropped++;
}

static inline int
dma_recv(struct net_device* dev, END_DEVICE* ei_local, int work_todo)
{
	struct PDMA_rxdesc *rxd;
	struct sk_buff *new_skb, *rx_skb;
	int gmac_no = PSE_PORT_GMAC1;
	int work_done = 0;
	u32 rxd_dma_owner_idx;
	u32 rxd_info2, rxd_info4;
#if defined (CONFIG_RAETH_HW_VLAN_RX)
	u32 rxd_info3;
#endif
#if defined (CONFIG_RAETH_SPECIAL_TAG)
	struct vlan_ethhdr *veth;
#endif

	rxd_dma_owner_idx = le32_to_cpu(sysRegRead(DMA_RX_CALC_IDX0));

	while (work_done < work_todo) {
		rxd_dma_owner_idx = (rxd_dma_owner_idx + 1) % NUM_RX_DESC;
		rxd = &ei_local->rxd_ring[rxd_dma_owner_idx];
		
		rxd_info2 = ACCESS_ONCE(rxd->rxd_info2);
		
		if (!(rxd_info2 & RX2_DMA_DONE))
			break;
		
		/* copy RX desc to CPU */
#if defined (CONFIG_RAETH_HW_VLAN_RX)
		rxd_info3 = ACCESS_ONCE(rxd->rxd_info3);
#endif
		rxd_info4 = ACCESS_ONCE(rxd->rxd_info4);
		
		/* load completed skb pointer */
		rx_skb = ei_local->rxd_buff[rxd_dma_owner_idx];
		
#if defined (CONFIG_PSEUDO_SUPPORT)
		gmac_no = RX4_DMA_SP(rxd_info4);
#endif
		/* We have to check the free memory size is big enough
		 * before pass the packet to cpu */
		new_skb = __dev_alloc_skb(MAX_RX_LENGTH + NET_IP_ALIGN, GFP_ATOMIC);
		if (unlikely(new_skb == NULL)) {
#if defined (RAETH_PDMA_V2)
			ACCESS_ONCE(rxd->rxd_info2) = RX2_DMA_SDL0_SET(MAX_RX_LENGTH);
#else
			ACCESS_ONCE(rxd->rxd_info2) = RX2_DMA_LS0;
#endif
			wmb();
			
			/* move CPU pointer to next RXD */
			sysRegWrite(DMA_RX_CALC_IDX0, cpu_to_le32(rxd_dma_owner_idx));
			
			inc_rx_drop(ei_local, gmac_no);
#if !defined (CONFIG_RAETH_NAPI)
			/* mean need reschedule */
			work_done = work_todo;
#endif
#if defined (CONFIG_RAETH_DEBUG)
			if (net_ratelimit())
				printk(KERN_ERR "%s: Failed to alloc new RX skb! (GMAC: %d)\n", RAETH_DEV_NAME, gmac_no);
#endif
			break;
		}
#if !defined (RAETH_PDMA_V2)
		skb_reserve(new_skb, NET_IP_ALIGN);
#endif
		/* store new empty skb pointer */
		ei_local->rxd_buff[rxd_dma_owner_idx] = new_skb;
		
		/* map new skb to ring (unmap is not required on generic mips mm) */
#if defined (RAETH_PDMA_V2)
		ACCESS_ONCE(rxd->rxd_info1) = (u32)dma_map_single(NULL, new_skb->data, MAX_RX_LENGTH + NET_IP_ALIGN, DMA_FROM_DEVICE);
		ACCESS_ONCE(rxd->rxd_info2) = RX2_DMA_SDL0_SET(MAX_RX_LENGTH);
#else
		ACCESS_ONCE(rxd->rxd_info1) = (u32)dma_map_single(NULL, new_skb->data, MAX_RX_LENGTH, DMA_FROM_DEVICE);
		ACCESS_ONCE(rxd->rxd_info2) = RX2_DMA_LS0;
#endif
		wmb();
		
		/* move CPU pointer to next RXD */
		sysRegWrite(DMA_RX_CALC_IDX0, cpu_to_le32(rxd_dma_owner_idx));
		
		/* skb processing */
		rx_skb->len = RX2_DMA_SDL0_GET(rxd_info2);
#if defined (RAETH_PDMA_V2)
		rx_skb->data += NET_IP_ALIGN;
#endif
		rx_skb->tail = rx_skb->data + rx_skb->len;

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
		FOE_MAGIC_TAG(rx_skb) = FOE_MAGIC_GE;
		DO_FILL_FOE_DESC(rx_skb, (rxd_info4 & ~(RX4_DMA_ALG_SET)));
#endif

#if defined (CONFIG_RAETH_CHECKSUM_OFFLOAD)
		if (rxd_info4 & RX4_DMA_L4FVLD)
			rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
#endif

#if defined (CONFIG_RAETH_HW_VLAN_RX)
		if ((rxd_info2 & RX2_DMA_TAG) && rxd_info3) {
			vlan_insert_tag_hwaccel(rx_skb, __constant_htons(ETH_P_8021Q), RX3_DMA_VID(rxd_info3));
		}
#endif

#if defined (CONFIG_PSEUDO_SUPPORT)
		if (gmac_no == PSE_PORT_GMAC2)
			rx_skb->protocol = eth_type_trans(rx_skb, ei_local->PseudoDev);
		else
#endif
			rx_skb->protocol = eth_type_trans(rx_skb, dev);

#if defined (CONFIG_RAETH_SPECIAL_TAG)
#if defined (CONFIG_MT7530_GSW)
#define ESW_TAG_ID	0x00
#else
#define ESW_TAG_ID	0x81
#endif
		// port0: 0x8100 => 0x8100 0001
		// port1: 0x8101 => 0x8100 0002
		// port2: 0x8102 => 0x8100 0003
		// port3: 0x8103 => 0x8100 0004
		// port4: 0x8104 => 0x8100 0005
		// port5: 0x8105 => 0x8100 0006
		veth = vlan_eth_hdr(rx_skb);
		if ((veth->h_vlan_proto & 0xFF) == ESW_TAG_ID) {
			veth->h_vlan_TCI = htons((((veth->h_vlan_proto >> 8) & 0xF) + 1));
			veth->h_vlan_proto = __constant_htons(ETH_P_8021Q);
			rx_skb->protocol = veth->h_vlan_proto;
		}
#endif

/* ra_sw_nat_hook_rx return 1 --> continue
 * ra_sw_nat_hook_rx return 0 --> FWD & without netif_rx
 */
#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
		if((ra_sw_nat_hook_rx == NULL) ||
		   (ra_sw_nat_hook_rx != NULL && ra_sw_nat_hook_rx(rx_skb)))
#endif
		{
#if defined (CONFIG_RAETH_NAPI)
#if defined (CONFIG_RAETH_NAPI_GRO)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
			/* our 3.4 tree has already backported GRO changes > 3.7 */
			napi_gro_receive(&ei_local->napi, rx_skb);
#else
			if (rx_skb->ip_summed == CHECKSUM_UNNECESSARY)
				napi_gro_receive(&ei_local->napi, rx_skb);
			else
				netif_receive_skb(rx_skb);
#endif
#else
			netif_receive_skb(rx_skb);
#endif	/* CONFIG_RAETH_NAPI_GRO */
#else
			netif_rx(rx_skb);
#endif	/* CONFIG_RAETH_NAPI */
		}
		
		work_done++;
	}

	return work_done;
}

#if defined (FE_INT_ENABLE2) && (FE_INT_INIT2_VALUE != 0)
static void
dispatch_int_status2(END_DEVICE *ei_local)
{
	u32 reg_int_val;

	reg_int_val = sysRegRead(FE_INT_STATUS2);
	if (unlikely(!reg_int_val))
		return;

	sysRegWrite(FE_INT_STATUS2, reg_int_val);

	if (!ei_local->active)
		return;

#if defined (CONFIG_GE2_INTERNAL_GPHY_P0) || defined (CONFIG_GE2_RGMII_AN) || \
    defined (CONFIG_GE2_INTERNAL_GPHY_P4)
	if (reg_int_val & GE2_LINK_INT) {
		/* do not touch MDIO registers in hardirq/softirq context */
		ge2_int2_schedule_wq();
	}
#endif
}
#else
#define dispatch_int_status2(x)
#endif

#if defined (CONFIG_RAETH_NAPI)
static int
ei_napi_poll(struct napi_struct *napi, int budget)
{
	END_DEVICE *ei_local = netdev_priv(napi->dev);
	unsigned long flags;
	int work_done;

	/* process RX queue */
	work_done = dma_recv(napi->dev, ei_local, budget);

	/* cleanup TX queue */
	dma_xmit_clean(napi->dev, ei_local);

	if (work_done < budget) {
		dispatch_int_status2(ei_local);
		
#if defined (CONFIG_RAETH_NAPI_GRO)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
		/* our 3.4 tree has already backported GRO changes > 3.7 */
		napi_gro_flush(napi, false);
#else
		napi_gro_flush(napi);
#endif
#endif
		/* exit from NAPI poll mode, ack and enable TX/RX interrupts */
		local_irq_save(flags);
		napi_complete(napi);
#if (FE_INT_INIT_VALUE != 0)
		sysRegWrite(FE_INT_STATUS, FE_INT_INIT_VALUE);
#endif
#if defined (CONFIG_RAETH_QDMA)
		sysRegWrite(QFE_INT_STATUS, QFE_INT_INIT_VALUE);
#endif
		if (ei_local->active) {
#if defined (FE_INT_ENABLE2) && (FE_INT_INIT2_VALUE != 0)
			sysRegWrite(FE_INT_ENABLE2, FE_INT_INIT2_VALUE);
#endif
#if (FE_INT_INIT_VALUE != 0)
			sysRegWrite(FE_INT_ENABLE, FE_INT_INIT_VALUE);
#endif
#if defined (CONFIG_RAETH_QDMA)
			sysRegWrite(QFE_INT_ENABLE, QFE_INT_INIT_VALUE);
#endif
		}
		local_irq_restore(flags);
	}

	return work_done;
}

#else

static void
ei_receive(unsigned long ptr)
{
	struct net_device *dev = (struct net_device *)ptr;
	END_DEVICE *ei_local = netdev_priv(dev);
	int work_done;

	/* process RX queue */
	work_done = dma_recv(dev, ei_local, NUM_RX_MAX_PROCESS);

	if (work_done < NUM_RX_MAX_PROCESS) {
		unsigned long flags;
		
		/* ack and enable RX interrupts */
		local_irq_save(flags);
#if defined (CONFIG_RAETH_QDMA) && (QFE_INT_MASK_RX != 0)
		sysRegWrite(QFE_INT_STATUS, QFE_INT_MASK_RX);
#else
		sysRegWrite(FE_INT_STATUS, FE_INT_MASK_RX);
#endif
		if (ei_local->active)
#if defined (CONFIG_RAETH_QDMA) && (QFE_INT_MASK_RX != 0)
			sysRegWrite(QFE_INT_ENABLE, QFE_INT_INIT_VALUE);
#else
			sysRegWrite(FE_INT_ENABLE, FE_INT_INIT_VALUE);
#endif
		local_irq_restore(flags);
	} else {
		/* reschedule again */
		if (ei_local->active)
			tasklet_schedule(&ei_local->rx_tasklet);
	}
}

static void
ei_xmit_clean(unsigned long ptr)
{
	struct net_device *dev = (struct net_device *)ptr;
	END_DEVICE *ei_local = netdev_priv(dev);

	dma_xmit_clean(dev, ei_local);
}
#endif /* CONFIG_RAETH_NAPI */

/**
 * ei_interrupt - handle Frame Engine interrupt
 */
static irqreturn_t
ei_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *) dev_id;
	END_DEVICE *ei_local;
#if !defined (CONFIG_RAETH_NAPI)
	u32 __maybe_unused reg_int_pdma, reg_int_qdma;
#endif

	if (!dev)
		return IRQ_NONE;

	ei_local = netdev_priv(dev);

#if defined (CONFIG_RAETH_NAPI)
	if (napi_schedule_prep(&ei_local->napi)) {
		/* disable all interrupts */
#if (FE_INT_INIT_VALUE != 0)
		sysRegWrite(FE_INT_ENABLE, 0);
#endif
#if defined (FE_INT_ENABLE2) && (FE_INT_INIT2_VALUE != 0)
		sysRegWrite(FE_INT_ENABLE2, 0);
#endif
#if defined (CONFIG_RAETH_QDMA)
		sysRegWrite(QFE_INT_ENABLE, 0);
#endif
		/* enter to NAPI poll mode */
		__napi_schedule(&ei_local->napi);
	}

#else /* !CONFIG_RAETH_NAPI */

#if (FE_INT_INIT_VALUE != 0)
	reg_int_pdma = sysRegRead(FE_INT_STATUS);
	if (reg_int_pdma)
		sysRegWrite(FE_INT_STATUS, reg_int_pdma);

#if !defined (CONFIG_RAETH_QDMA)
	if (reg_int_pdma & FE_INT_MASK_TX)
		tasklet_schedule(&ei_local->tx_tasklet);
#endif
#if (FE_INT_MASK_RX != 0)
	if (reg_int_pdma & FE_INT_MASK_RX) {
		u32 reg_int_mask = sysRegRead(FE_INT_ENABLE);
		if (reg_int_mask & FE_INT_MASK_RX) {
			/* disable PDMA RX interrupt */
			sysRegWrite(FE_INT_ENABLE, reg_int_mask & ~(FE_INT_MASK_RX));
			tasklet_hi_schedule(&ei_local->rx_tasklet);
		}
	}
#endif
#endif

#if defined (CONFIG_RAETH_QDMA)
	reg_int_qdma = sysRegRead(QFE_INT_STATUS);
	if (reg_int_qdma)
		sysRegWrite(QFE_INT_STATUS, reg_int_qdma);

	if (reg_int_qdma & QFE_INT_MASK_TX)
		tasklet_schedule(&ei_local->tx_tasklet);

#if (QFE_INT_MASK_RX != 0)
	if (reg_int_qdma & QFE_INT_MASK_RX) {
		u32 reg_int_mask = sysRegRead(QFE_INT_ENABLE);
		if (reg_int_mask & QFE_INT_MASK_RX) {
			/* disable QDMA RX interrupt */
			sysRegWrite(QFE_INT_ENABLE, reg_int_mask & ~(QFE_INT_MASK_RX));
			tasklet_hi_schedule(&ei_local->rx_tasklet);
		}
	}
#endif
#endif

	dispatch_int_status2(ei_local);
#endif /* CONFIG_RAETH_NAPI */

	return IRQ_HANDLED;
}

static int
ei_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	return raeth_ioctl(ifr, cmd);
}

static int
ei_change_mtu(struct net_device *dev, int new_mtu)
{
	if (new_mtu < 68 || new_mtu > MAX_RX_LENGTH)
		return -EINVAL;

#if !defined (CONFIG_RAETH_JUMBOFRAME)
	if (new_mtu > ETH_DATA_LEN)
		return -EINVAL;
#endif

	dev->mtu = new_mtu;

	return 0;
}

#if defined (CONFIG_PSEUDO_SUPPORT)
static struct rtnl_link_stats64 *
VirtualIF_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	PSEUDO_ADAPTER *pPseudoAd = netdev_priv(dev);
	END_DEVICE *ei_local = netdev_priv(pPseudoAd->RaethDev);

	spin_lock(&ei_local->stat_lock);
	fe_gdm2_fetch_mib(&pPseudoAd->stat);
	memcpy(stats, &pPseudoAd->stat, sizeof(struct rtnl_link_stats64));
	spin_unlock(&ei_local->stat_lock);

	return stats;
}

#if defined (CONFIG_RAETH_ESW_CONTROL)
#if defined (CONFIG_GE2_INTERNAL_GPHY_P0) || defined (CONFIG_GE2_RGMII_AN) || \
    defined (CONFIG_GE2_INTERNAL_GPHY_P4)
int
VirtualIF_get_bytes(port_bytes_t *pb)
{
	struct net_device *dev = dev_raether;
	END_DEVICE *ei_local;
	PSEUDO_ADAPTER *pPseudoAd;
	struct rtnl_link_stats64 *stat;

	if (!dev)
		return -1;

	ei_local = netdev_priv(dev);
	pPseudoAd = netdev_priv(ei_local->PseudoDev);
	stat = &pPseudoAd->stat;

	spin_lock(&ei_local->stat_lock);
	fe_gdm2_fetch_mib(stat);
	pb->RX = stat->rx_bytes;
	pb->TX = stat->tx_bytes;
	spin_unlock(&ei_local->stat_lock);

	return 0;
}
#endif
#endif

static int
VirtualIF_open(struct net_device *dev)
{
	netdev_update_features(dev);

	fe_gdm2_set_mac(dev->dev_addr);

#if defined (CONFIG_RAETH_BQL)
	netdev_reset_queue(dev);
#endif
	netif_start_queue(dev);

	printk("%s: ===> VirtualIF_open\n", dev->name);
	return 0;
}

static int
VirtualIF_close(struct net_device *dev)
{
	netif_tx_disable(dev);

	printk("%s: ===> VirtualIF_close\n", dev->name);
	return 0;
}

static int
VirtualIF_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	END_DEVICE *ei_local;
	PSEUDO_ADAPTER *pPseudoAd = netdev_priv(dev);

	if (!netif_running(pPseudoAd->RaethDev)) {
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	ei_local = netdev_priv(pPseudoAd->RaethDev);

	return dma_xmit(skb, dev, ei_local, PSE_PORT_GMAC2);
}

static const struct net_device_ops VirtualIF_netdev_ops = {
	.ndo_open		= VirtualIF_open,
	.ndo_stop		= VirtualIF_close,
	.ndo_start_xmit		= VirtualIF_start_xmit,
	.ndo_get_stats64	= VirtualIF_get_stats64,
	.ndo_do_ioctl		= ei_ioctl,
	.ndo_change_mtu		= ei_change_mtu,
	.ndo_fix_features	= ei_fix_features,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static int
VirtualIF_init(struct net_device *dev_parent)
{
	struct net_device *dev;
	PSEUDO_ADAPTER *pPseudoAd;
	END_DEVICE *ei_local;
	u8 *mac_addr;

	int i = 0;
	struct sockaddr addr;

	dev = alloc_etherdev(sizeof(PSEUDO_ADAPTER));
	if (!dev)
		return -ENOMEM;

	pPseudoAd = netdev_priv(dev);
	pPseudoAd->RaethDev = dev_parent;

	ether_setup(dev);
	strcpy(dev->name, DEV2_NAME);
	eth_hw_addr_inherit(dev, dev_parent);
	mac_addr = dev->dev_addr;
	if(mac_addr[6] < 0xFF )
		mac_addr[6]+=1;
	else
		mac_addr[6]-=1;

	dev->netdev_ops = &VirtualIF_netdev_ops;
#if defined (CONFIG_ETHTOOL)
	/* init mii structure */
	pPseudoAd->mii_info.dev = dev;
	pPseudoAd->mii_info.mdio_read = mdio_virt_read;
	pPseudoAd->mii_info.mdio_write = mdio_virt_write;
	pPseudoAd->mii_info.phy_id_mask = 0x1f;
	pPseudoAd->mii_info.reg_num_mask = 0x1f;
	pPseudoAd->mii_info.phy_id = 0x1e;
	pPseudoAd->mii_info.supports_gmii = mii_check_gmii_support(&pPseudoAd->mii_info);
	dev->ethtool_ops = &ra_virt_ethtool_ops;
#endif

	fill_dev_features(dev);

	/* Register this device */
	if (register_netdevice(dev) != 0) {
		free_netdev(dev);
		return -ENXIO;
	}

	ei_local = netdev_priv(dev_parent);
	ei_local->PseudoDev = dev;

	return 0;
}
#endif /* CONFIG_PSEUDO_SUPPORT */

static void
ei_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	END_DEVICE *ei_local = netdev_priv(dev);

	spin_lock(&ei_local->stat_lock);
	fe_gdm1_fetch_mib(&ei_local->stat);
	memcpy(stats, &ei_local->stat, sizeof(struct rtnl_link_stats64));
	spin_unlock(&ei_local->stat_lock);
}

/**
 * ei_init - pick up ethernet port at boot time
 * @dev: network device to probe
 *
 * This routine probe the ethernet port at boot time.
 */
static int __init
ei_init(struct net_device *dev)
{
	END_DEVICE *ei_local = netdev_priv(dev);
	const char *mac_addr;

	ei_local->active = 0;
#if !defined (RAETH_HW_PADPKT)
	ei_local->min_pkt_len = ETH_ZLEN;
#endif

	spin_lock_init(&ei_local->stat_lock);
	spin_lock_init(&ei_local->page_lock);

	INIT_WORK(&ei_local->stat_wq, stat_wq_handler);

	setup_timer(&ei_local->stat_timer, stat_timer_handler, (unsigned long)dev);

#if defined (CONFIG_RAETH_NAPI)
	netif_napi_add(dev, &ei_local->napi, ei_napi_poll, NAPI_WEIGHT);
#else
	tasklet_init(&ei_local->rx_tasklet, ei_receive, (unsigned long)dev);
	tasklet_init(&ei_local->tx_tasklet, ei_xmit_clean, (unsigned long)dev);
#endif

	mac_addr = of_get_mac_address(ei_local->dev->of_node);
	if (mac_addr)
		ether_addr_copy(dev->dev_addr, mac_addr);

	/* If the mac address is invalid, use random mac address  */
	if (!is_valid_ether_addr(dev->dev_addr)) {
		eth_hw_addr_random(dev);
		dev_err(ei_local->dev, "generated random MAC address %pM\n",
			dev->dev_addr);
	}

	if (fe_dma_ring_alloc(ei_local) != 0) {
		printk(KERN_WARNING "%s: ring_alloc FAILED!\n", RAETH_DEV_NAME);
		return -ENOMEM;
	}

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
	foe_dma_table_alloc();
#endif

#if defined (CONFIG_PSEUDO_SUPPORT)
	/* Register virtual net device (eth3) for the driver */
	return VirtualIF_init(dev);
#else
	return 0;
#endif
}

static void
ei_uninit(struct net_device *dev)
{
	END_DEVICE *ei_local = netdev_priv(dev);

#if defined (CONFIG_PSEUDO_SUPPORT)
	if (ei_local->PseudoDev) {
		unregister_netdevice(ei_local->PseudoDev);
		free_netdev(ei_local->PseudoDev);
		ei_local->PseudoDev = NULL;
	}
#endif

#if defined (CONFIG_RAETH_NAPI)
	netif_napi_del(&ei_local->napi);
#endif

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
	/* down PPE engine before free ring */
	if (ra_sw_nat_hook_ec != NULL)
		ra_sw_nat_hook_ec(0);

	foe_dma_table_free();
#endif

	fe_dma_clear_addr();
	fe_dma_ring_free(ei_local);
}

/**
 * ei_open - Open/Initialize the ethernet port.
 * @dev: network device to initialize
 *
 * This routine goes all-out, setting everything
 * up a new at each open, even though many of these registers should only need to be set once at boot.
 */
static int
ei_open(struct net_device *dev)
{
	int err;
	END_DEVICE *ei_local;

	if (!dev) {
		printk(KERN_EMERG "%s: ei_open passed a non-existent device!\n", dev->name);
		return -ENXIO;
	}

	ei_local = netdev_priv(dev);

	netdev_update_features(dev);
	show_dev_features(dev);
#if !defined (RAETH_HW_PADPKT)
	calc_dev_min_pkt_len(dev, ei_local);
#endif

	spin_lock_bh(&ei_local->page_lock);

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
	/* down PPE engine before FE reset */
	if (ra_sw_nat_hook_ec != NULL)
		ra_sw_nat_hook_ec(0);
#endif

	fe_eth_reset();
	fe_dma_init(ei_local);
	fe_esw_init();
	fe_cdm_init(dev);
	fe_gdm_init(dev);
	fe_pse_init();

#if defined (CONFIG_RAETH_HW_VLAN_TX) && !defined (RAETH_HW_VLAN4K)
	fe_cdm_update_vlan_tx(ei_local->vlan_id_map);
#endif

	fe_gdm1_set_mac(dev->dev_addr);
#if defined (CONFIG_PSEUDO_SUPPORT)
	fe_gdm2_set_mac(ei_local->PseudoDev->dev_addr);
#endif

#if defined (CONFIG_RAETH_ESW_CONTROL)
	esw_ioctl_init_post();
#endif

#if defined (CONFIG_RA_HW_NAT) || defined (CONFIG_RA_HW_NAT_MODULE)
	/* up PPE engine after FE init */
	if (ra_sw_nat_hook_ec != NULL)
		ra_sw_nat_hook_ec(1);
#endif

	spin_unlock_bh(&ei_local->page_lock);

	/* request interrupt line for FE */
	err = request_irq(dev->irq, ei_interrupt, 0, dev->name, dev);
	if (err)
		return err;

#if defined (CONFIG_RAETH_ESW) || defined (CONFIG_MT7530_GSW)
	/* prepare switch for INT handling */
	esw_irq_init();
#if defined (CONFIG_RAETH_ESW) || defined (CONFIG_RALINK_MT7621)
	/* request interrupt line for MT7620 ESW or MT7621 GSW */
	err = request_irq((8 + 17), esw_interrupt, 0, "ralink_esw", dev);
#elif defined (CONFIG_MT7530_INT_GPIO)
	// todo, needed capture GPIO interrupt for external MT7530
#endif
#endif

#if defined (CONFIG_RAETH_BQL)
	netdev_reset_queue(dev);
#endif

#if defined (CONFIG_RAETH_NAPI)
	napi_enable(&ei_local->napi);
#endif

	/* allow processing */
	ei_local->active = 1;

#if defined (CONFIG_RAETH_ESW) || defined (CONFIG_MT7530_GSW)
	esw_irq_enable();
#endif
	fe_irq_enable();
	fe_dma_start();

	netif_start_queue(dev);

	/* counters overflow after ~34s for 1Gbps speed, use 25s period for safe */
	mod_timer(&ei_local->stat_timer, jiffies + (25 * HZ));

	return 0;
}

/**
 * ei_close - shut down network device
 * @dev: network device to clear
 *
 * This routine shut down network device.
 */
static int
ei_close(struct net_device *dev)
{
	END_DEVICE *ei_local = netdev_priv(dev);

	/* block processing */
	ei_local->active = 0;

#if defined (CONFIG_PSEUDO_SUPPORT)
	netif_tx_disable(ei_local->PseudoDev);
#endif
	netif_tx_disable(dev);

	fe_dma_stop();
	fe_irq_disable();

#if defined (CONFIG_RAETH_NAPI)
	napi_disable(&ei_local->napi);
#else
	tasklet_kill(&ei_local->rx_tasklet);
	tasklet_kill(&ei_local->tx_tasklet);
#endif

	del_timer_sync(&ei_local->stat_timer);
	cancel_work_sync(&ei_local->stat_wq);
#if defined (CONFIG_RAETH_ESW) || defined (CONFIG_MT7530_GSW)
	esw_irq_cancel_wq();
#endif
#if defined (CONFIG_GE2_INTERNAL_GPHY_P0) || defined (CONFIG_GE2_RGMII_AN) || \
    defined (CONFIG_GE2_INTERNAL_GPHY_P4)
	ge2_int2_cancel_wq();
#endif

	free_irq(dev->irq, dev);
#if defined (CONFIG_RAETH_ESW) || (defined (CONFIG_RALINK_MT7621) && defined (CONFIG_MT7530_GSW))
	free_irq((8 + 17), dev);
#elif defined (CONFIG_MT7530_INT_GPIO)
	// todo, needed capture GPIO interrupt for external MT7530
#endif

	fe_dma_uninit(ei_local);

	/* fetch pending FE counters */
	fetch_stat_counters(ei_local);

	return 0;
}

static int
ei_start_xmit(struct sk_buff* skb, struct net_device *dev)
{
	END_DEVICE *ei_local = netdev_priv(dev);

	return dma_xmit(skb, dev, ei_local, PSE_PORT_GMAC1);
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void ei_fake_poll(struct net_device *dev)
{
//	request_irq(dev->irq, ei_interrupt, IRQF_DISABLED, dev->name, dev);
	disable_irq(dev->irq);
	ei_interrupt(dev->irq, dev);
	enable_irq(dev->irq);
//#if defined (CONFIG_RAETH_NAPI)
//	ei_napi_poll(0, 16);
//#else
//	ei_receive((unsigned long)dev);
//#endif
}
#endif

static const struct net_device_ops ei_netdev_ops = {
	.ndo_init		= ei_init,
	.ndo_uninit		= ei_uninit,
	.ndo_open		= ei_open,
	.ndo_stop		= ei_close,
	.ndo_start_xmit		= ei_start_xmit,
	.ndo_get_stats64	= ei_get_stats64,
	.ndo_do_ioctl		= ei_ioctl,
	.ndo_change_mtu		= ei_change_mtu,
	.ndo_fix_features	= ei_fix_features,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= ei_fake_poll
#endif
};

static int fe_mdio_reset(struct mii_bus *bus)
{
	/* TODO */
	return 0;
}

int fe_mdio_init(END_DEVICE *priv, struct platform_device *pdev)
{
	struct device_node *mii_np;
	int err;

	mii_np = of_get_child_by_name(pdev->dev.of_node, "mdio-bus");
	if (!mii_np) {
		printk("no mdio-bus child node found");
		return -ENODEV;
	}

	if (!of_device_is_available(mii_np)) {
		err = 0;
		goto err_put_node;
	}

	priv->mii_bus = mdiobus_alloc();
	if (!priv->mii_bus) {
		err = -ENOMEM;
		goto err_put_node;
	}

	priv->mii_bus->name = "mdio";
	priv->mii_bus->read = mt7620_mdio_read;
	priv->mii_bus->write = mt7620_mdio_write;
	priv->mii_bus->reset = fe_mdio_reset;
	priv->mii_bus->priv = NULL;
	priv->mii_bus->parent = &pdev->dev;

	snprintf(priv->mii_bus->id, MII_BUS_ID_SIZE, "%s", mii_np->name);
	err = of_mdiobus_register(priv->mii_bus, mii_np);
	if (err)
		goto err_free_bus;

	return 0;

err_free_bus:
	kfree(priv->mii_bus);
err_put_node:
	of_node_put(mii_np);
	priv->mii_bus = NULL;
	return err;
}

void fe_mdio_cleanup(END_DEVICE *priv)
{
	if (!priv->mii_bus)
		return;

	mdiobus_unregister(priv->mii_bus);
	of_node_put(priv->mii_bus->dev.of_node);
	kfree(priv->mii_bus);
}

/**
 * raeth_init - Module Init code
 *
 * Called by kernel to register net_device
 *
 */

u32 ralink_asic_rev_id;

static int raeth_init(struct platform_device *pdev)
{
	int ret;
	struct net_device *dev;
	END_DEVICE *ei_local;

	ralink_asic_rev_id = (*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x0c));

#if defined (CONFIG_RALINK_MT7620)
	/* MT7620 has Frame Engine bugs (probably in CDM unit).
	   ECO_ID < 5:
	   1. TX Csum_Gen raise abnormal TX flood and FE hungs
	   2. TSO stuck and FE hungs
	*/
	if ((ralink_asic_rev_id & 0xf) < 5) {
#if defined (CONFIG_RAETH_CHECKSUM_OFFLOAD)
		hw_offload_csg = 0;
#if defined (CONFIG_RAETH_SG_DMA_TX)
		hw_offload_gso = 0;
#if defined (CONFIG_RAETH_TSO)
		hw_offload_tso = 0;
#endif
#endif
#endif
	}
#endif

	dev = alloc_etherdev(sizeof(END_DEVICE));
	if (!dev)
		return -ENOMEM;

	ei_local = netdev_priv(dev);

	ether_setup(dev);
	strcpy(dev->name, DEV_NAME);

	ei_local->dev = &pdev->dev;

	dev->irq = 5;
	dev->base_addr = RALINK_FRAME_ENGINE_BASE;
	dev->watchdog_timeo = 5*HZ;
	dev->netdev_ops = &ei_netdev_ops;
#if defined (CONFIG_ETHTOOL)
	/* init mii structure */
	ei_local->mii_info.dev = dev;
	ei_local->mii_info.mdio_read = mdio_read;
	ei_local->mii_info.mdio_write = mdio_write;
	ei_local->mii_info.phy_id_mask = 0x1f;
	ei_local->mii_info.reg_num_mask = 0x1f;
	ei_local->mii_info.phy_id = 1;
	ei_local->mii_info.supports_gmii = mii_check_gmii_support(&ei_local->mii_info);
	dev->ethtool_ops = &ra_ethtool_ops;
#endif

#if defined (CONFIG_RAETH_HW_VLAN_TX) && !defined (RAETH_HW_VLAN4K)
	fill_hw_vlan_tx_map(ei_local);
#endif
	fill_dev_features(dev);

	/* Register net device (eth2) for the driver */
	ret = register_netdev(dev);
	if (ret != 0) {
		free_netdev(dev);
		printk(KERN_WARNING " " __FILE__ ": No ethernet port found.\n");
		return ret;
	}

#if defined (CONFIG_RALINK_MT7620)
	fe_mdio_init(ei_local, pdev);
#endif

	dev_raether = dev;

	debug_proc_init();

	early_phy_init();

#if defined (CONFIG_RAETH_ESW_CONTROL)
	esw_ioctl_init();
#elif defined (CONFIG_RAETH_DHCP_TOUCH)
	esw_dhcpc_init();
#endif

	printk("Ralink APSoC Ethernet Driver %s (%s)\n", RAETH_VERSION, RAETH_DEV_NAME);
#if defined (CONFIG_RAETH_QDMA)
#if defined (CONFIG_RAETH_QDMATX_QDMARX)
	printk("%s: QDMA RX ring %d, QDMA TX pool %d. Max packet size %d\n", RAETH_DEV_NAME, NUM_RX_DESC, NUM_TX_DESC, MAX_RX_LENGTH);
#else
	printk("%s: PDMA RX ring %d, QDMA TX pool %d. Max packet size %d\n", RAETH_DEV_NAME, NUM_RX_DESC, NUM_TX_DESC, MAX_RX_LENGTH);
#endif
#else
	printk("%s: PDMA RX ring %d, PDMA TX ring %d. Max packet size %d\n", RAETH_DEV_NAME, NUM_RX_DESC, NUM_TX_DESC, MAX_RX_LENGTH);
#endif
#if defined (CONFIG_RAETH_NAPI)
#if defined (CONFIG_RAETH_NAPI_GRO)
	printk("%s: NAPI & GRO support, weight %d\n", RAETH_DEV_NAME, NAPI_WEIGHT);
#else
	printk("%s: NAPI support, weight %d\n", RAETH_DEV_NAME, NAPI_WEIGHT);
#endif
#endif
#if defined (CONFIG_RAETH_BQL)
	printk("%s: Byte Queue Limits (BQL) support\n", RAETH_DEV_NAME);
#endif

	return 0;
}

/**
 * raeth_uninit - Module Exit code
 *
 * Cmd 'rmmod' will invode the routine to exit the module
 *
 */

static int raeth_exit(struct platform_device *pdev)
{
	END_DEVICE *ei_local;
	struct net_device *dev = dev_raether;
	if (!dev)
		return -ENODEV;

	ei_local = netdev_priv(dev);

#if defined (CONFIG_RALINK_MT7620)
	fe_mdio_cleanup(ei_local);
#endif

#if defined (CONFIG_RAETH_ESW_CONTROL)
	esw_ioctl_uninit();
#endif

	unregister_netdev(dev);
	free_netdev(dev);

	debug_proc_exit();

	dev_raether = NULL;

	return 0;
}

const struct of_device_id of_fe_match[] = {
	{ .compatible = "mediatek,mt7620-eth" },
	{ .compatible = "ralink,rt5350-eth" },
	{},
};

static struct platform_driver fe_driver = {
	.probe = raeth_init,
	.remove = raeth_exit,
	.driver = {
		.name = "mtk_soc_eth",
		.owner = THIS_MODULE,
		.of_match_table = of_fe_match,
	},
};

module_platform_driver(fe_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ralink APSoC Ethernet Driver");
