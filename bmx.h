/*
 * Copyright (c) 2010  BMX protocol contributor(s):
 * Axel Neumann  <neumann at cgws dot de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */


#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/rtnetlink.h>

#include "cyassl/sha.h"
#include "cyassl/random.h"

/*
 * from other headers:
 * TODO: partly move this to system.h
 * dont touch this for compatibility reasons:
 */

#define CODE_VERSION 4             // to be incremented after each critical code change
#define BMX_BRANCH "BMX6"
#define BRANCH_VERSION "0.1-alpha" //put exactly one distinct word inside the string like "0.3-pre-alpha" or "0.3-rc1" or "0.3"
#define COMPATIBILITY_VERSION 13

/*
 * from iid.h:
 */
typedef uint16_t IID_T;

typedef struct neigh_node IID_NEIGH_T;

typedef struct dhash_node IID_NODE_T;



/*
 * from ip.h:
 */

#define GEN_ADDR_LEN 20
#define IP6_ADDR_LEN 16
#define IP4_ADDR_LEN 4
#define MAC_ADDR_LEN 6

//#define INET_ADDRSTRLEN INET_ADDRSTRLEN     // from in.h
//#define INET6_ADDRSTRLEN INET6_ADDRSTRLEN    // from in.h

#define IPX_STR_LEN INET6_ADDRSTRLEN


typedef uint32_t IP4_T;

typedef struct in6_addr IP6_T;

typedef IP6_T IPX_T;

typedef union {
	uint8_t   u8[GEN_ADDR_LEN];
	uint16_t u16[GEN_ADDR_LEN / sizeof(uint16_t)];
	uint32_t u32[GEN_ADDR_LEN / sizeof(uint32_t)];
	uint64_t u64[GEN_ADDR_LEN / sizeof(uint64_t)];
} ADDR_T;


typedef union {
	uint8_t   u8[MAC_ADDR_LEN];
	uint16_t u16[MAC_ADDR_LEN / sizeof(uint16_t)];
} MAC_T;





/*
 * from bmx.h:
 */
typedef uint32_t TIME_T;
#define TIME_MAX ((TIME_T)-1)

typedef uint32_t TIME_SEC_T;

typedef int8_t IDM_T; // smallest int which size does NOT matter






// to be used:
typedef uint64_t UMETRIC_T;

#define OGM_MANTISSA_BIT_SIZE  5
#define OGM_EXPONENT_BIT_SIZE  5
#define OGM_EXPONENT_OFFSET    OGM_MANTISSA_BIT_SIZE

#define OGM_EXPONENT_MAX       ((1<<OGM_EXPONENT_BIT_SIZE)-1)
#define OGM_MANTISSA_MASK      ((1<<OGM_MANTISSA_BIT_SIZE)-1)
#define OGM_EXPONENT_MASK      ((1<<OGM_EXPONENT_BIT_SIZE)-1)


#define OGM_MANTISSA_INVALID            0
#define OGM_MANTISSA_MIN__NOT_ROUTABLE  1
#define OGM_MANTISSA_ROUTABLE           2

#define FM8_EXPONENT_BIT_SIZE  OGM_EXPONENT_BIT_SIZE
#define FM8_MANTISSA_BIT_SIZE  (8-FM8_EXPONENT_BIT_SIZE)
#define FM8_MANTISSA_MASK      ((1<<FM8_MANTISSA_BIT_SIZE)-1)
#define FM8_MANTISSA_MIN       (1)

#define OGM_MANTISSA_MAX       (FM8_MANTISSA_MASK << (OGM_MANTISSA_BIT_SIZE - FM8_MANTISSA_BIT_SIZE))

#define UMETRIC_SHIFT_MAX          ((sizeof(UMETRIC_T)*8) - (OGM_EXPONENT_OFFSET+OGM_EXPONENT_MAX+1))
#define UMETRIC_MULTIPLY_MAX       (((UMETRIC_T)-1)>>(OGM_EXPONENT_OFFSET+OGM_EXPONENT_MAX+1))
#define UMETRIC_MASK               ((((UMETRIC_T) 1) << (OGM_EXPONENT_OFFSET+OGM_EXPONENT_MAX+1)) -1)

#define UMETRIC_INVALID            ((((UMETRIC_T) 1) << OGM_EXPONENT_OFFSET) + OGM_MANTISSA_INVALID)
#define UMETRIC_MIN__NOT_ROUTABLE  ((((UMETRIC_T) 1) << OGM_EXPONENT_OFFSET) + OGM_MANTISSA_MIN__NOT_ROUTABLE)
#define UMETRIC_ROUTABLE           ((((UMETRIC_T) 1) << OGM_EXPONENT_OFFSET) + OGM_MANTISSA_ROUTABLE)
#define UMETRIC_FM8_MAX            ((((UMETRIC_T) 1) << (OGM_EXPONENT_OFFSET+OGM_EXPONENT_MAX)) + (((UMETRIC_T) FM8_MANTISSA_MASK) << ((OGM_EXPONENT_OFFSET+OGM_EXPONENT_MAX)-FM8_MANTISSA_BIT_SIZE)))
#define UMETRIC_FM8_MIN            ((((UMETRIC_T) 1) << OGM_EXPONENT_OFFSET) + (((UMETRIC_T) FM8_MANTISSA_MIN) << (OGM_EXPONENT_OFFSET-FM8_MANTISSA_BIT_SIZE)))
#define UMETRIC_MAX                UMETRIC_FM8_MAX
//#define UMETRIC_MAX       ((((UMETRIC_T) 1) << (OGM_EXPONENT_OFFSET+OGM_EXPONENT_MAX)) + (((UMETRIC_T) OGM_MANTISSA_MAX) << ((OGM_EXPONENT_OFFSET+OGM_EXPONENT_MAX)-OGM_MANTISSA_BIT_SIZE)))

// these fixes are used to improove (average) rounding errors in umetric_to_fmetric()
#define UMETRIC_TO_FMETRIC_INPUT_FIX (79)

//#define UMETRIC_MAX_SQRT           ((UMETRIC_T)358956)      // sqrt(UMETRIC_MAX)
//#define UMETRIC_MAX_HALF_SQRT      ((UMETRIC_T)253821)      // sqrt(UMETRIC_MAX/2)
//#define U64_MAX_QUARTER_SQRT       ((UMETRIC_T)2147493120)  // sqrt(U64_MAX/4)

struct float_u16 {

	union {
		struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint8_t mantissa_fm16;
			uint8_t exp_fm16;
#elif __BYTE_ORDER == __BIG_ENDIAN
			uint8_t exp_fm16;
			uint8_t mantissa_fm16;
#else
#error "Please fix <bits/endian.h>"
#endif
		} __attribute__((packed)) f;

		uint8_t u8[2];

		uint16_t u16;
	}val;
};

typedef struct float_u16 FMETRIC_U16_T;



struct float_u8 {
	union {

		struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			unsigned int mantissa_fmu8 : FM8_MANTISSA_BIT_SIZE;
			unsigned int exp_fmu8 : FM8_EXPONENT_BIT_SIZE;
#elif __BYTE_ORDER == __BIG_ENDIAN
			unsigned int exp_fmu8 : FM8_EXPONENT_BIT_SIZE;
			unsigned int mantissa_fmu8 : FM8_MANTISSA_BIT_SIZE;
#else
#error "Please fix <bits/endian.h>"
#endif
		} __attribute__((packed)) f;
		uint8_t u8;
	} val;
};

typedef struct float_u8 FMETRIC_U8_T;




#define MIN_TX_INTERVAL 35
#define MAX_TX_INTERVAL 10000  // < U16_MAX due to metricalgo->ogm_interval field
#define DEF_TX_INTERVAL 500
#define ARG_TX_INTERVAL "tx_interval"
extern int32_t my_tx_interval;

#define DEF_TX_DELAY ((2*my_tx_interval) + rand_num(my_tx_interval))

#define ARG_OGM_INTERVAL "ogm_interval"
#define DEF_OGM_INTERVAL 5000
#define MIN_OGM_INTERVAL 200
#define MAX_OGM_INTERVAL 60000 // 60000 = 1 minutes
extern int32_t my_ogm_interval;


#define MIN_OGM_PURGE_TO  (MAX_OGM_INTERVAL + MAX_TX_INTERVAL)
#define MAX_OGM_PURGE_TO  864000000 /*10 days*/
#define DEF_OGM_PURGE_TO  100000
#define ARG_OGM_PURGE_TO  "purge_timeout"
// extern int32_t purge_to;

#define DEF_DAD_TO 20000//(MAX_OGM_INTERVAL + MAX_TX_INTERVAL)
#define MIN_DAD_TO 100
#define MAX_DAD_TO 360000000
#define ARG_DAD_TO "dad_timeout"
extern int32_t dad_to;

#define DEF_DROP_ALL_FRAMES 0
#define MIN_DROP_ALL_FRAMES 0
#define MAX_DROP_ALL_FRAMES 1
#define ARG_DROP_ALL_FRAMES "drop_all_frames"

#define DEF_DROP_ALL_PACKETS 0
#define MIN_DROP_ALL_PACKETS 0
#define MAX_DROP_ALL_PACKETS 1
#define ARG_DROP_ALL_PACKETS "drop_all_packets"


#define MIN_DHASH_TO 300000 //300000
#define DHASH_TO_TOLERANCE_FK 10




/*
 * from msg.h:
 */


// deprecated:
typedef uint16_t SQN_T;
#define SQN_MAX ((SQN_T)-1)
#define MAX_SQN_RANGE 8192 // the maxumim of all .._SQN_RANGE ranges, should never be more than SQN_MAX/4


// OGMs:
typedef uint16_t OGM_SQN_T;
#define OGM_SQN_BIT_SIZE (16)
#define OGM_SQN_MASK     ((1<<OGM_SQN_BIT_SIZE)-1)
#define OGM_SQN_MAX      OGM_SQN_MASK
#define OGM_SQN_STEP     1

#define MIN_OGM_SQN_RANGE 32
#define MAX_OGM_SQN_RANGE 8192 // changing this will cause compatibility trouble
#define DEF_OGM_SQN_RANGE MAX_OGM_SQN_RANGE
#define ARG_OGM_SQN_RANGE "ogm_validity_range"


typedef uint16_t OGM_MIX_T;
#define OGM_MIX_BIT_SIZE (sizeof (OGM_MIX_T) * 8)

#define OGM_IIDOFFST_BIT_SIZE (OGM_MIX_BIT_SIZE-(OGM_MANTISSA_BIT_SIZE+OGM_EXPONENT_BIT_SIZE))

#define OGM_IIDOFFST_MASK ((1<<OGM_IIDOFFST_BIT_SIZE)-1)

#define OGM_EXPONENT_BIT_POS (0)
#define OGM_MANTISSA_BIT_POS (0 + OGM_EXPONENT_BIT_SIZE)
#define OGM_IIDOFFST_BIT_POS (0 + OGM_MANTISSA_BIT_SIZE + OGM_EXPONENT_BIT_SIZE)





// aggregations of OGMs:
typedef uint8_t AGGREG_SQN_T;
#define AGGREG_SQN_BIT_SIZE (8)
#define AGGREG_SQN_MASK     ((1<<AGGREG_SQN_BIT_SIZE)-1)
#define AGGREG_SQN_MAX      AGGREG_SQN_MASK

#define AGGREG_SQN_CACHE_RANGE 64
#define AGGREG_SQN_CACHE_WARN  (AGGREG_SQN_CACHE_RANGE/2)
#define AGGREG_ARRAY_BYTE_SIZE (AGGREG_SQN_CACHE_RANGE/8)

typedef uint8_t OGM_DEST_T;
#define OGM_DEST_BIT_SIZE (8)
#define OGM_DEST_MASK     ((1<<OGM_DEST_BIT_SIZE)-1)
#define OGM_DEST_MAX      OGM_DEST_MASK

#define OGM_DEST_ARRAY_BIT_SIZE (1<<OGM_DEST_BIT_SIZE)

#define LOCALS_MAX (1<<OGM_DEST_BIT_SIZE) // because each local needs a bit to be indicated in the ogm.dest_field


typedef uint32_t PKT_SQN_T;
#define PKT_SQN_DAD_RANGE 1000
#define PKT_SQN_DAD_TOLERANCE 100
#define PKT_SQN_MAX ((PKT_SQN_T)-1)

typedef uint16_t DEVADV_SQN_T;
#define DEVADV_SQN_DISABLED 0 // dev-adv are not provided by this node!
#define DEVADV_SQN_DAD_RANGE 256
#define DEVADV_SQN_MAX ((DEVADV_SQN_T)-1)

typedef uint16_t LINKADV_SQN_T;
#define LINKADV_SQN_DAD_RANGE 256
#define LINKADV_SQN_MAX ((LINKADV_SQN_T)-1)


typedef uint8_t DEVADV_IDX_T;
//#define DEVADV_IDX_BIT_SIZE (8*sizeof(DEVADV_IDX_T))
#define DEVADV_IDX_INVALID 0
#define DEVADV_IDX_ALL 0
#define DEVADV_IDX_MIN 1
#define DEVADV_IDX_MAX ((DEVADV_IDX_T)-1)

typedef uint32_t LOCAL_ID_T;
#define LOCAL_ID_BIT_SIZE (8*sizeof(LOCAL_ID_T))
#define LOCAL_ID_INVALID 0
#define LOCAL_ID_MIN 1
#define LOCAL_ID_MAX ((LOCAL_ID_T)-1)
#define LOCAL_ID_ITERATIONS_MAX 256
#define LOCAL_ID_ITERATIONS_WARN (LOCAL_ID_ITERATIONS_MAX>>3)

extern LOCAL_ID_T my_local_id;




// hello and hello reply messages:
typedef uint16_t HELLO_SQN_T;

#define HELLO_SQN_BIT_SIZE (sizeof(HELLO_SQN_T)*8)
#define HELLO_SQN_MASK ((HELLO_SQN_T)-1)
#define HELLO_SQN_MAX       HELLO_SQN_MASK

#define HELLO_SQN_TOLERANCE 4

#define MAX_HELLO_SQN_WINDOW 128
#define MIN_HELLO_SQN_WINDOW 1
#define DEF_HELLO_SQN_WINDOW 48
#define ARG_HELLO_SQN_WINDOW "link_window"
//extern int32_t my_link_window; // my link window size used to quantify the link qualities to direct neighbors
//#define RP_PURGE_ITERATIONS MAX_LINK_WINDOW


#define DEF_LINK_PURGE_TO  100000
#define MIN_LINK_PURGE_TO  (MAX_TX_INTERVAL*2)
#define MAX_LINK_PURGE_TO  864000000 /*10 days*/
#define ARG_LINK_PURGE_TO  "link_purge_timeout"




// descriptions 
typedef uint16_t DESC_SQN_T;
#define DESC_SQN_BIT_SIZE   (16)
#define DESC_SQN_MASK     ((1<<DESC_SQN_BIT_SIZE)-1)
#define DESC_SQN_MAX        DESC_SQN_MASK

#define DEF_DESCRIPTION_DAD_RANGE 8192


typedef uint8_t  FRAME_TYPE_T;

#define FRAME_ISSHORT_BIT_SIZE   (1)
#define FRAME_RELEVANCE_BIT_SIZE  (1)
#define FRAME_TYPE_BIT_SIZE    ((8*sizeof(FRAME_TYPE_T)) - FRAME_ISSHORT_BIT_SIZE - FRAME_RELEVANCE_BIT_SIZE)
#define FRAME_TYPE_MASK        MIN( (0x1F) /*some bits reserved*/, ((1<<FRAME_TYPE_BIT_SIZE)-1))
#define FRAME_TYPE_ARRSZ       (FRAME_TYPE_MASK+1)



#define HASH0_SHA1_LEN SHA_DIGEST_SIZE  // sha.h: 20 bytes

#define MAX_PACKET_SIZE 1600



struct packet_header // 12 bytes
{
	uint8_t    bmx_version;      //  8
	uint8_t    reserved;         //  8  reserved
	uint16_t   pkt_length; 	     // 16 the relevant data size in bytes (including the bmx_header)

	IID_T      transmitterIID;   // 16 IID of transmitter node

	LINKADV_SQN_T link_adv_sqn;     // 16 used for processing: link_adv, lq_adv, rp_adv, ogm_adv, ogm_ack

	PKT_SQN_T  pkt_sqn;          // 32
	LOCAL_ID_T local_id;         // 32
	
	DEVADV_IDX_T   dev_idx;          //  8

//	uint8_t    reserved_for_2byte_alignement;  //  8

} __attribute__((packed));






/*
 * from metrics.h
 */

typedef uint16_t ALGO_T;


struct host_metricalgo {

	FMETRIC_U16_T fmetric_u16_min;

	UMETRIC_T umetric_min;
	ALGO_T algo_type;
	uint16_t flags;
	uint8_t algo_rp_exp_numerator;
	uint8_t algo_rp_exp_divisor;
	uint8_t algo_tp_exp_numerator;
	uint8_t algo_tp_exp_divisor;


	uint8_t window_size;                // MUST be given as multiple of sqn_steps
        uint8_t lounge_size;                // MUST be given as multiple of sqn_steps e.g. 6
        uint8_t regression;             // e.g. 16
//        uint8_t fast_regression;             // e.g. 2
//        uint8_t fast_regression_impact;             // e.g. 8
	uint8_t hystere;
	uint8_t hop_penalty;
	uint8_t late_penalty;
};

struct lndev_probe_record {
	HELLO_SQN_T hello_sqn_max; // SQN which has been applied (if equals wa_pos) then wa_unscaled MUST NOT be set again!

	uint8_t hello_array[MAX_HELLO_SQN_WINDOW/8];
	uint32_t hello_sum;
	UMETRIC_T hello_umetric;
	TIME_T hello_time_max;
};


struct metric_record {
	SQN_T sqn_bit_mask;

        SQN_T clr; // SQN upto which waightedAverageVal has been purged
	SQN_T set; // SQN which has been applied (if equals wa_pos) then wa_unscaled MUST NOT be set again!

//	UMETRIC_T umetric;
//	UMETRIC_T umetric_fast;
	UMETRIC_T umetric;
//	UMETRIC_T umetric_prev;
};

#define ZERO_METRIC_RECORD {0, 0, 0, 0,0,0}




#include "avl.h"
#include "list.h"
#include "iid.h"
#include "control.h"
#include "allocate.h"



#define DESCRIPTION0_ID_RANDOM_T uint64_t
#define DESCRIPTION0_ID_RANDOM_LEN sizeof( DESCRIPTION0_ID_RANDOM_T )
#define DESCRIPTION0_ID_NAME_LEN 32


struct description_id {
	char    name[DESCRIPTION0_ID_NAME_LEN];
	union {
		uint8_t u8[DESCRIPTION0_ID_RANDOM_LEN];
		uint16_t u16[DESCRIPTION0_ID_RANDOM_LEN / sizeof(uint16_t)];
		uint32_t u32[DESCRIPTION0_ID_RANDOM_LEN / sizeof(uint32_t)];
		uint64_t u64[DESCRIPTION0_ID_RANDOM_LEN / sizeof( uint64_t)];
	} rand;
} __attribute__((packed));

struct description_hash {
	union {
		uint8_t u8[HASH0_SHA1_LEN];
		uint32_t u32[HASH0_SHA1_LEN/sizeof(uint32_t)];
	} h;
};





#define BMX_ENV_LIB_PATH "BMX6_LIB_PATH"
#define BMX_DEF_LIB_PATH "/usr/lib"
// e.g. sudo BMX_LIB_PATH="$(pwd)/lib" ./bmx6 -d3 eth0:bmx
#define BMX_ENV_DEBUG "BMX6_DEBUG"


#define DEF_TTL 50                /* Time To Live of OGM broadcast messages */
#define MAX_TTL 63
#define MIN_TTL 1
#define ARG_TTL "ttl"
extern int32_t my_ttl;



#define ARG_HELP		"help"
#define ARG_VERBOSE_HELP	"verbose_help"
#define ARG_EXP			"exp_help"
#define ARG_VERBOSE_EXP		"verbose_exp_help"

#define ARG_VERSION		"version"

#define ARG_TEST		"test"
#define ARG_SHOW_PARAMETER 	"parameters"



#define ARG_ORIGINATORS "originators"
#define ARG_STATUS "status"
#define ARG_LINKS "links"
#define ARG_LOCALS "locals"
#define ARG_ROUTES "routes"
#define ARG_INTERFACES "interfaces"

#define ARG_THROW "throw"




#define MAX_DBG_STR_SIZE 1500
#define OUT_SEQNO_OFFSET 1

enum NoYes {
	NO,
	YES
};

enum ADGSN {
	ADD,
	DEL,
	GET,
	SET,
	NOP
};


#define SUCCESS 0
#define FAILURE -1


#define MAX_SELECT_TIMEOUT_MS 400 /* MUST be smaller than (1000/2) to fit into max tv_usec */
#define CRITICAL_PURGE_TIME_DRIFT 5


#define MAX( a, b ) ( (a>b) ? (a) : (b) )
#define MIN( a, b ) ( (a<b) ? (a) : (b) )

#define U64_MAX ((uint64_t)(-1))
#define U32_MAX ((uint32_t)(-1))
#define I32_MAX ((U32_MAX>>1))
#define U16_MAX ((uint16_t)(-1))
#define I16_MAX ((U16_MAX>>1))
#define U8_MAX  ((uint8_t)(-1))
#define I8_MAX  ((U8_MAX>>1))


#define U32_LT( a, b )  ( ((uint32_t)( (a) - (b) ) ) >  I32_MAX )
#define U32_LE( a, b )  ( ((uint32_t)( (b) - (a) ) ) <= I32_MAX )
#define U32_GT( a, b )  ( ((uint32_t)( (b) - (a) ) ) >  I32_MAX )
#define U32_GE( a, b )  ( ((uint32_t)( (a) - (b) ) ) <= I32_MAX )

#define UXX_LT( mask, a, b )  ( ((mask)&( (a) - (b) ) ) >  (((mask)&U32_MAX)>>1) )
#define UXX_LE( mask, a, b )  ( ((mask)&( (b) - (a) ) ) <= (((mask)&U32_MAX)>>1) )
#define UXX_GT( mask, a, b )  ( ((mask)&( (b) - (a) ) ) >  (((mask)&U32_MAX)>>1) )
#define UXX_GE( mask, a, b )  ( ((mask)&( (a) - (b) ) ) <= (((mask)&U32_MAX)>>1) )

#define MAX_UXX( mask, a, b ) ( (UXX_GT(mask,a,b)) ? (a) : (b) )
#define MIN_UXX( mask, a, b ) ( (UXX_LT(mask,a,b)) ? (a) : (b) )


#define UXX_GET_MAX(mask, a, b ) ( (UXX_GT( (mask), (a), (b) )) ? (a) : (b) )




#define WARNING_PERIOD 20000

#define MAX_PATH_SIZE 300
#define MAX_ARG_SIZE 200


extern TIME_T bmx_time;
extern TIME_SEC_T bmx_time_sec;

extern IDM_T initializing;
extern IDM_T terminating;
extern IDM_T cleaning_up;


extern uint32_t s_curr_avg_cpu_load;

extern IDM_T my_description_changed;

extern struct orig_node self;


/**
 * The most important data structures
 */

enum {
	FIELD_TYPE_UINT,
	FIELD_TYPE_HEX,
	FIELD_TYPE_STRING_SIZE,
	FIELD_TYPE_STRING_CHAR,
	FIELD_TYPE_STRING_BINARY,
	FIELD_TYPE_STRPTR_CHAR,
	FIELD_TYPE_IP4,
	FIELD_TYPE_IPX,
	FIELD_TYPE_IPX4,
	FIELD_TYPE_IPX6,
	FIELD_TYPE_MAC,

	FIELD_TYPE_END
};

#define FIELD_STANDARD_SIZES {-1,-1,-1,-8,-8,(8*sizeof(void*)),32,128,128,128,48}
// negative values mean size must be multiple of negativ value, positive values mean absolute bit sizes

enum {
        FIELD_RELEVANCE_LOW,
        FIELD_RELEVANCE_MEDI,
        FIELD_RELEVANCE_HIGH
};

struct field_format {
	uint16_t field_type;
        int32_t field_pos; // -1 means relative to previous 
	uint32_t field_bits;
	uint8_t field_host_order;
        uint8_t field_relevance;
	const char * field_name;
};

#define FIELD_FORMAT_END {FIELD_TYPE_END, 0, 0, 0, FIELD_RELEVANCE_LOW, NULL}
#define FIELD_STR_VALUE(name) #name
#define FIELD_FORMAT_INIT(f_type, f_struct_name, f_struct_field, f_host_order, f_relevance) { \
.field_type = f_type, \
.field_pos = (((unsigned long)&(((struct f_struct_name*) NULL)->f_struct_field))*8), \
.field_bits = (sizeof( (((struct f_struct_name *) NULL)->f_struct_field) ) * 8), \
.field_host_order = f_host_order, \
.field_relevance = f_relevance, \
.field_name = FIELD_STR_VALUE(f_struct_field) \
}

struct field_iterator {
        const struct field_format *format;
//        char * msg_name;
        uint8_t *data;
        uint32_t data_size;
        uint32_t min_msg_size;
//        uint8_t fixed_msg_size;

        uint32_t field;
        uint32_t field_bits;
        uint32_t var_bits;
        uint32_t field_bit_pos;
        uint32_t msg_bit_pos;

};

struct status_handl {
        uint16_t min_msg_size;
        char status_name[16];
        char *code_category;
        uint8_t *data;

	int32_t (*frame_creator) (struct status_handl *status_handl);

	const struct field_format *format;
};

extern struct avl_tree status_tree;


uint32_t fields_dbg(struct ctrl_node *cn, uint16_t relevance, uint16_t data_size, uint8_t *data,
                    uint16_t min_msg_size,  const struct field_format *format);

int16_t field_format_get_items(const struct field_format *format);



struct task_node {
	struct list_node list;
	TIME_T expire;
	void (* task) (void *fpara); // pointer to the function to be executed
	void *data; //NULL or pointer to data to be given to function. Data will be freed after functio is called.
};

struct tx_task_content {
	struct dev_node *dev; // the outgoing interface to be used for transmitting
	struct link_node *link;
	uint32_t u32;
	uint16_t u16;
	IID_T myIID4x;
	IID_T neighIID4x;
	uint16_t type;
};

struct tx_task_node {
	struct list_node list;

	struct tx_task_content task;
	uint16_t frame_msgs_length; 
	int16_t  tx_iterations;
	TIME_T considered_ts;
	TIME_T send_ts;
};



extern struct avl_tree local_tree;

struct local_node {

	LOCAL_ID_T local_id;
	struct avl_tree link_tree;
	struct link_dev_node *best_rp_lndev;
	struct link_dev_node *best_tp_lndev;
	struct link_dev_node *best_lndev;
	struct neigh_node *neigh; // to be set when confirmed, use carefully

	PKT_SQN_T packet_sqn;
	TIME_T packet_time;
	LINKADV_SQN_T packet_link_sqn_ref; //indicating the maximum existing link_adv_sqn

	// the latest received link_adv:
	LINKADV_SQN_T link_adv_sqn;
	TIME_T link_adv_time;
	uint16_t link_adv_msgs;
	int16_t link_adv_msg_for_me;
	int16_t link_adv_msg_for_him;
	struct msg_link_adv *link_adv;
	DEVADV_SQN_T link_adv_dev_sqn_ref;

	// the latest received dev_adv:
	DEVADV_SQN_T dev_adv_sqn;
	uint16_t dev_adv_msgs;
	struct msg_dev_adv *dev_adv;

	// the latest received rp_adv:
	TIME_T rp_adv_time;
	IDM_T rp_ogm_request_rcvd;
	int32_t orig_routes;
};


extern struct avl_tree link_tree;

struct link_node_key {
	DEVADV_IDX_T dev_idx;
	LOCAL_ID_T local_id;
};

struct link_node {

	struct link_node_key key;

	IPX_T link_ip;

	TIME_T pkt_time_max;
	TIME_T hello_time_max;

	HELLO_SQN_T hello_sqn_max;

	struct local_node *local; // set immediately
	
	struct list_head lndev_list; // list with one link_node_dev element per link
};


struct link_dev_key {
	struct link_node *link;
	struct dev_node *dev;
};

struct router_node {

//	struct link_dev_key key_2BRemoved;

	struct local_node *local_key;

	struct metric_record mr;
	OGM_SQN_T ogm_sqn_last;
	UMETRIC_T ogm_umetric_last;
	
	UMETRIC_T path_metric_best; //TODO removed
	struct link_dev_node *path_lndev_best;
};


extern struct avl_tree link_dev_tree;

struct link_dev_node {
	struct list_node list;
	struct link_dev_key key;

	UMETRIC_T tx_probe_umetric;
	UMETRIC_T timeaware_tx_probe;
	struct lndev_probe_record rx_probe_record;
	UMETRIC_T timeaware_rx_probe;

	struct list_head tx_task_lists[FRAME_TYPE_ARRSZ]; // scheduled frames and messages
	int16_t link_adv_msg;
	TIME_T pkt_time_max;
};



extern struct avl_tree neigh_tree;

struct neigh_node {

	struct neigh_node *nnkey;
	struct dhash_node *dhn; // confirmed dhash

	struct local_node *local; // to be set when confirmed, use carefully

	// filled in by ???:

	IID_T neighIID4me;

	struct iid_repos neighIID4x_repos;

//	AGGREG_SQN_T ogm_aggregation_rcvd_set;
	AGGREG_SQN_T ogm_aggregation_cleard_max;
	uint8_t ogm_aggregations_not_acked[AGGREG_ARRAY_BYTE_SIZE];
	uint8_t ogm_aggregations_rcvd[AGGREG_ARRAY_BYTE_SIZE];
};





extern struct avl_tree orig_tree;
extern struct avl_tree blocked_tree;

struct orig_node {
	// filled in by validate_new_link_desc0():

	struct description_id id;

	struct dhash_node *dhn;
	struct description *desc;

	TIME_T updated_timestamp; // last time this on's desc was succesfully updated

	DESC_SQN_T descSqn;

	OGM_SQN_T ogmSqn_rangeMin;
	OGM_SQN_T ogmSqn_rangeSize;



	// filled in by process_desc0_tlvs()->
	IPX_T primary_ip;
	char primary_ip_str[IPX_STR_LEN];
	uint8_t blocked;


	struct host_metricalgo *path_metricalgo;

	// calculated by update_path_metric()

	OGM_SQN_T ogmSqn_maxRcvd;

	OGM_SQN_T ogmSqn_next;
	UMETRIC_T ogmMetric_next;

	OGM_SQN_T ogmSqn_send;
//	UMETRIC_T ogmMetric_send;

	UMETRIC_T *metricSqnMaxArr;          // TODO: remove

	struct avl_tree rt_tree;

	struct router_node * best_rt_local;  // TODO: remove
	struct router_node *curr_rt_local;   // the currently used local neighbor for routing
	struct link_dev_node *curr_rt_lndev; // the configured route in the kernel!


	//size of plugin data is defined during intialization and depends on registered PLUGIN_DATA_ORIG hooks
	void *plugin_data[];

};




extern struct avl_tree dhash_tree;
extern struct avl_tree dhash_invalid_tree;

struct dhash_node {

	struct description_hash dhash;

	TIME_T referred_by_me_timestamp; // last time this dhn was referred

	struct neigh_node *neigh;

	IID_T myIID4orig;


	struct orig_node *on;
};



extern struct avl_tree blacklisted_tree;

struct black_node {

	struct description_hash dhash;
};





/* list element to store all the disabled tunnel rule netmasks */
struct throw_node
{
	struct list_node list;
	uint32_t addr;
	uint8_t  netmask;
};


struct ogm_aggreg_node {

	struct list_node list;

	struct msg_ogm_adv *ogm_advs;

	uint8_t ogm_dest_field[(OGM_DEST_ARRAY_BIT_SIZE / 8)];
//	int16_t ogm_dest_bit_max;
	int16_t ogm_dest_bytes;

	uint16_t aggregated_msgs;

	AGGREG_SQN_T    sqn;
	uint8_t  tx_attempt;
};

struct packet_buff {

	struct packet_buff_info {
		//filled by wait4Event()
		struct sockaddr_storage addr;
		struct timeval tv_stamp;
		struct dev_node *iif;
		int total_length;
		uint8_t unicast;

		//filled in by rx_packet()
		uint32_t rx_counter;
		IID_T transmittersIID;
		PKT_SQN_T pkt_sqn;
		LINKADV_SQN_T link_sqn;

		struct link_node_key link_key;

		IPX_T llip;
		char llip_str[INET6_ADDRSTRLEN];
		struct dev_node *oif;
		struct link_dev_node *lndev;
		struct link_node *link;

//		struct neigh_node *described_neigh; // might be updated again process_dhash_description_neighIID4x()
	} i;

	union {
		struct packet_header header;
		unsigned char data[MAX_PACKET_SIZE + 1];
	} packet;

};



#define timercpy(d, a) (d)->tv_sec = (a)->tv_sec; (d)->tv_usec = (a)->tv_usec;



enum {
	CLEANUP_SUCCESS,
	CLEANUP_FAILURE,
	CLEANUP_MY_SIGSEV,
	CLEANUP_RETURN
};


/***********************************************************
 Data Infrastructure
 ************************************************************/
IDM_T equal_link_key( struct link_dev_key *a, struct link_dev_key *b );

void blacklist_neighbor(struct packet_buff *pb);

IDM_T blacklisted_neighbor(struct packet_buff *pb, struct description_hash *dhash);

struct neigh_node *is_described_neigh( struct link_node *link, IID_T transmittersIID4x );

void purge_link_route_orig_nodes(struct dev_node *only_dev, IDM_T only_expired);
void free_orig_node(struct orig_node *on);
void init_orig_node(struct orig_node *on, struct description_id *id);

void purge_local_node(struct local_node *local);

IDM_T update_local_neigh(struct packet_buff *pb, struct dhash_node *dhn);
void update_neigh_dhash(struct orig_node *on, struct description_hash *dhash);

LOCAL_ID_T new_local_id(struct dev_node *dev);

void rx_packet( struct packet_buff *pb );


/***********************************************************
 Runtime Infrastructure
************************************************************/


/*
 * ASSERTION / PARANOIA ERROR CODES:
 * Negative numbers are used as SIGSEV error codes !
 * Currently used numbers are: -500000 -500001 ... -501242
 */

#ifdef NO_ASSERTIONS
#define paranoia( ... )
#define assertion( ... )
#define ASSERTION( ... )
#define EXITERROR( ... )
#define CHECK_INTEGRITY( ... )

#else//NO_ASSERTIONS

#define paranoia( code , problem ) do { if ( (problem) ) { cleanup_all( code ); } }while(0)
#define assertion( code , condition ) do { if ( !(condition) ) { cleanup_all( code ); } }while(0)

#ifdef EXTREME_PARANOIA
#define ASSERTION( code , condition ) do { if ( !(condition) ) { cleanup_all( code ); } }while(0)
#define CHECK_INTEGRITY( ) checkIntegrity()
#else
#define CHECK_INTEGRITY( )
#define ASSERTION( code , condition )
#endif

#ifdef EXIT_ON_ERROR
#define EXITERROR( code , condition )                                                                                  \
  do {                                                                                                                 \
      if ( !(condition) ) {                                                                                            \
         dbgf(DBGL_SYS, DBGT_ERR, "This is paranoid! Disable EXIT_ON_ERROR to not exit due to minor or others' misbehavior");              \
           cleanup_all( code );                                                                                        \
      }                                                                                                                \
  }while(0)
#else
#define EXITERROR( code , condition )
#endif

#endif//NO_ASSERTIONS


#ifndef PROFILING
#define STATIC_FUNC static
#define STATIC_INLINE_FUNC static inline
#else
#define STATIC_FUNC
#define STATIC_INLINE_FUNC
#endif

#ifdef STATIC_VARIABLES
#define STATIC_VAR static
#else
#define STATIC_VAR
#endif


#ifndef NO_TRACE_FUNCTION_CALLS

#define FUNCTION_CALL_BUFFER_SIZE 64

//extern char* function_call_buffer_name_array[FUNCTION_CALL_BUFFER_SIZE];
//extern TIME_T function_call_buffer_time_array[FUNCTION_CALL_BUFFER_SIZE];
//extern uint8_t function_call_buffer_pos;

void trace_function_call(const char *);

#define TRACE_FUNCTION_CALL trace_function_call ( __FUNCTION__ )


#else

#define TRACE_FUNCTION_CALL

#endif


void wait_sec_msec( TIME_SEC_T sec, TIME_T msec );

void cleanup_all( int32_t status );

void upd_time( struct timeval *precise_tv );

char *get_human_uptime( uint32_t reference );



/***********************************************************
 Configuration data and handlers
************************************************************/


IDM_T validate_name( char* name );
IDM_T validate_param(int32_t probe, int32_t min, int32_t max, char *name);
