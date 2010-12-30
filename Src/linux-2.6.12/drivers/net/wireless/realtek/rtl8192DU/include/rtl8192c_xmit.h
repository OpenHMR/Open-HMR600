#ifndef _RTL8192C_XMIT_H_
#define _RTL8192C_XMIT_H_

#define HWXMIT_ENTRY	4

#define VO_QUEUE_INX	0
#define VI_QUEUE_INX	1
#define BE_QUEUE_INX	2
#define BK_QUEUE_INX	3
#define TS_QUEUE_INX	4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7

#define HW_QUEUE_ENTRY	8

#define TXDESC_SIZE 32
#define PACKET_OFFSET_SZ (8)
#define TXDESC_OFFSET (TXDESC_SIZE + PACKET_OFFSET_SZ)


#define tx_cmd tx_desc

//
//defined for TX DESC Operation
//

#define MAX_TID (15)

//OFFSET 0
#define OFFSET_SZ (0)
#define OFFSET_SHT (16)
#define OWN 	BIT(31)
#define FSG	BIT(27)
#define LSG	BIT(26)

//OFFSET 4
#define PKT_OFFSET_SZ (0)
#define QSEL_SHT (8)
#define HWPC BIT(31)

//OFFSET 8
#define BMC BIT(7)
#define BK BIT(30)
#define AGG_EN BIT(29)

//OFFSET 12
#define SEQ_SHT (16)

//OFFSET 16
#define TXBW BIT(18)

//OFFSET 20
#define DISFB BIT(15)

struct tx_desc{

	//DWORD 0
	unsigned int txdw0;

	unsigned int txdw1;

	unsigned int txdw2;

	unsigned int txdw3;

	unsigned int txdw4;

	unsigned int txdw5;

	unsigned int txdw6;

	unsigned int txdw7;	

};


union txdesc {
	struct tx_desc txdesc;
	unsigned int value[TXDESC_SIZE>>2];	
};

void cal_txdesc_chksum(struct tx_desc	*ptxdesc);
int update_txdesc(struct xmit_frame *pxmitframe, uint *ptxdesc, int sz);
void dump_xframe(_adapter *padapter, struct xmit_frame *pxmitframe);

int xmitframe_complete(_adapter *padapter, struct xmit_priv *pxmitpriv, struct xmit_buf *pxmitbuf);

struct xmit_frame *dequeue_one_xmitframe(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, struct tx_servq *ptxservq, _queue *pframe_queue);

void do_queue_select(_adapter *padapter, struct pkt_attrib *pattrib);
u32 get_ff_hwaddr(struct xmit_frame	*pxmitframe);

int pre_xmitframe(_adapter *padapter, struct xmit_frame *pxmitframe);
int xmitframe_enqueue(_adapter *padapter, struct xmit_frame *pxmitframe);
int xmitframe_direct(_adapter *padapter, struct xmit_frame *pxmitframe);

#endif

