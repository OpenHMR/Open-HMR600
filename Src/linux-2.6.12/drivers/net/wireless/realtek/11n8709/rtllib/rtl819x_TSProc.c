/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/
#include "rtllib.h"
#include <linux/etherdevice.h>
#include "rtl819x_TS.h"
extern void _setup_timer( struct timer_list*, void*, unsigned long);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#define list_for_each_entry_safe(pos, n, head, member) \
	for (pos = list_entry((head)->next, typeof(*pos), member), \
		n = list_entry(pos->member.next, typeof(*pos), member); \
		&pos->member != (head); \
		pos = n, n = list_entry(n->member.next, typeof(*n), member))
#endif
void TsSetupTimeOut(unsigned long data)
{
	// Not implement yet
	// This is used for WMMSA and ACM , that would send ADDTSReq frame.
}

void TsInactTimeout(unsigned long data)
{
	// Not implement yet
	// This is used for WMMSA and ACM.
	// This function would be call when TS is no Tx/Rx for some period of time.
}

//*************************************************************************************
//function:  I still not understand this function, so wait for further implementation
//   input:  unsigned long	 data		//acturally we send TX_TS_RECORD or RX_TS_RECORD to these timer
//  return:  NULL
//  notice:  
//***************************************************************************************/
#if 1
void RxPktPendingTimeout(unsigned long data)
{
	PRX_TS_RECORD	pRxTs = (PRX_TS_RECORD)data;
	struct rtllib_device *ieee = container_of(pRxTs, struct rtllib_device, RxTsRecord[pRxTs->num]);
	
	PRX_REORDER_ENTRY 	pReorderEntry = NULL;

	//u32 flags = 0;
	unsigned long flags = 0;
	struct rtllib_rxb *stats_IndicateArray[REORDER_WIN_SIZE];
	u8 index = 0;
	bool bPktInBuf = false;


	spin_lock_irqsave(&(ieee->reorder_spinlock), flags);
	//PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	RTLLIB_DEBUG(RTLLIB_DL_REORDER,"==================>%s()\n",__FUNCTION__);
	if(pRxTs->RxTimeoutIndicateSeq != 0xffff)
	{
		// Indicate the pending packets sequentially according to SeqNum until meet the gap.
		while(!list_empty(&pRxTs->RxPendingPktList))
		{
			pReorderEntry = (PRX_REORDER_ENTRY)list_entry(pRxTs->RxPendingPktList.prev,RX_REORDER_ENTRY,List);
			if(index == 0)
				pRxTs->RxIndicateSeq = pReorderEntry->SeqNum;

			if( SN_LESS(pReorderEntry->SeqNum, pRxTs->RxIndicateSeq) || 
				SN_EQUAL(pReorderEntry->SeqNum, pRxTs->RxIndicateSeq)	)
			{
				list_del_init(&pReorderEntry->List);
			
				if(SN_EQUAL(pReorderEntry->SeqNum, pRxTs->RxIndicateSeq))
					pRxTs->RxIndicateSeq = (pRxTs->RxIndicateSeq + 1) % 4096;

				RTLLIB_DEBUG(RTLLIB_DL_REORDER,"RxPktPendingTimeout(): IndicateSeq: %d\n", pReorderEntry->SeqNum);
				stats_IndicateArray[index] = pReorderEntry->prxb;
				index++;
				
				list_add_tail(&pReorderEntry->List, &ieee->RxReorder_Unused_List);
			}
			else
			{
				bPktInBuf = true;
				break;
			}
		}
	}

	if(index>0)
	{
		// Set RxTimeoutIndicateSeq to 0xffff to indicate no pending packets in buffer now.
		pRxTs->RxTimeoutIndicateSeq = 0xffff;
	
		// Indicate packets
		if(index > REORDER_WIN_SIZE){
			RTLLIB_DEBUG(RTLLIB_DL_ERR, "RxReorderIndicatePacket(): Rx Reorer buffer full!! \n");
			spin_unlock_irqrestore(&(ieee->reorder_spinlock), flags);
			return;
		}
		rtllib_indicate_packets(ieee, stats_IndicateArray, index);
		 bPktInBuf = false;

	}

	if(bPktInBuf && (pRxTs->RxTimeoutIndicateSeq==0xffff))
	{
		pRxTs->RxTimeoutIndicateSeq = pRxTs->RxIndicateSeq;
#if 0   
		if(timer_pending(&pTS->RxPktPendingTimer))
			del_timer_sync(&pTS->RxPktPendingTimer);
		pTS->RxPktPendingTimer.expires = jiffies + MSECS(pHTInfo->RxReorderPendingTime);
		add_timer(&pTS->RxPktPendingTimer);
#else
		mod_timer(&pRxTs->RxPktPendingTimer,  jiffies + MSECS(ieee->pHTInfo->RxReorderPendingTime));
#endif

#if 0		
		if(timer_pending(&pRxTs->RxPktPendingTimer))
			del_timer_sync(&pRxTs->RxPktPendingTimer);
		pRxTs->RxPktPendingTimer.expires = jiffies + ieee->pHTInfo->RxReorderPendingTime;
		add_timer(&pRxTs->RxPktPendingTimer);
#endif
	}
	spin_unlock_irqrestore(&(ieee->reorder_spinlock), flags);
	//PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
}
#endif

//***********************************************************************************************
//function:  Add BA timer function  
//   input:  unsigned long	 data		//acturally we send TX_TS_RECORD or RX_TS_RECORD to these timer
//  return:  NULL
//  notice:  
//**********************************************************************************************/
void TsAddBaProcess(unsigned long data)
{
	PTX_TS_RECORD	pTxTs = (PTX_TS_RECORD)data;
	u8 num = pTxTs->num;
	struct rtllib_device *ieee = container_of(pTxTs, struct rtllib_device, TxTsRecord[num]);

	TsInitAddBA(ieee, pTxTs, BA_POLICY_IMMEDIATE, false);
	RTLLIB_DEBUG(RTLLIB_DL_BA, "TsAddBaProcess(): ADDBA Req is started!! \n");
}


void ResetTsCommonInfo(PTS_COMMON_INFO	pTsCommonInfo)
{
	memset(pTsCommonInfo->Addr, 0, 6);
	memset(&pTsCommonInfo->TSpec, 0, sizeof(TSPEC_BODY));
	memset(&pTsCommonInfo->TClass, 0, sizeof(QOS_TCLAS)*TCLAS_NUM);
	pTsCommonInfo->TClasProc = 0;
	pTsCommonInfo->TClasNum = 0;
}

void ResetTxTsEntry(PTX_TS_RECORD pTS)
{
	ResetTsCommonInfo(&pTS->TsCommonInfo);
	pTS->TxCurSeq = 0;
	pTS->bAddBaReqInProgress = false;
	pTS->bAddBaReqDelayed = false;
	pTS->bUsingBa = false;
	pTS->bDisable_AddBa = false;
	ResetBaEntry(&pTS->TxAdmittedBARecord); //For BA Originator
	ResetBaEntry(&pTS->TxPendingBARecord);
}

void ResetRxTsEntry(PRX_TS_RECORD pTS)
{
	ResetTsCommonInfo(&pTS->TsCommonInfo);
	pTS->RxIndicateSeq = 0xffff; // This indicate the RxIndicateSeq is not used now!!
	pTS->RxTimeoutIndicateSeq = 0xffff; // This indicate the RxTimeoutIndicateSeq is not used now!!
	ResetBaEntry(&pTS->RxAdmittedBARecord);	  // For BA Recepient
}
#ifdef _RTL8192_EXT_PATCH_
//YJ,add,090430
void ResetAdmitTRStream(struct rtllib_device *ieee, u8 *Addr)
{
	u8 	dir;
	bool				search_dir[4] = {0, 0, 0, 0};
	struct list_head*		psearch_list; //FIXME
	PTS_COMMON_INFO	pRet = NULL;
	PRX_TS_RECORD pRxTS = NULL;
	PTX_TS_RECORD pTxTS = NULL;

	if(ieee->iw_mode != IW_MODE_MESH) 
		return;

	//search_dir[DIR_BI_DIR]	= true;
	//search_dir[DIR_DIRECT]	= true;
	search_dir[DIR_DOWN] 	= true;
	psearch_list = &ieee->Rx_TS_Admit_List;
	for(dir = 0; dir <= DIR_BI_DIR; dir++)
	{
		if(search_dir[dir] ==false )
			continue;
		list_for_each_entry(pRet, psearch_list, List){
	//		RTLLIB_DEBUG(RTLLIB_DL_TS, "ADD:"MAC_FMT", TID:%d, dir:%d\n", MAC_ARG(pRet->Addr), pRet->TSpec.f.TSInfo.field.ucTSID, pRet->TSpec.f.TSInfo.field.ucDirection);
			if ((memcmp(pRet->Addr, Addr, 6) == 0) && (pRet->TSpec.f.TSInfo.field.ucDirection == dir))
			{
				pRxTS = (PRX_TS_RECORD)pRet;
				pRxTS->RxIndicateSeq = 0xffff;
				pRxTS->RxTimeoutIndicateSeq = 0xffff;
			}
					
		}	
	}
	search_dir[DIR_UP] 	= true;
	psearch_list = &ieee->Tx_TS_Admit_List;
	for(dir = 0; dir <= DIR_BI_DIR; dir++)
	{
		if(search_dir[dir] ==false )
			continue;
		list_for_each_entry(pRet, psearch_list, List){
	//		RTLLIB_DEBUG(RTLLIB_DL_TS, "ADD:"MAC_FMT", TID:%d, dir:%d\n", MAC_ARG(pRet->Addr), pRet->TSpec.f.TSInfo.field.ucTSID, pRet->TSpec.f.TSInfo.field.ucDirection);
			if ((memcmp(pRet->Addr, Addr, 6) == 0) && (pRet->TSpec.f.TSInfo.field.ucDirection == dir))
			{
				pTxTS = (PTX_TS_RECORD)pRet;
				pTxTS->TxCurSeq = 0xffff;
			}
					
		}	
	}

	return;
}
//YJ,add,090430,end
#endif

void TSInitialize(struct rtllib_device *ieee)
{
	PTX_TS_RECORD		pTxTS  = ieee->TxTsRecord;
	PRX_TS_RECORD		pRxTS  = ieee->RxTsRecord;
	PRX_REORDER_ENTRY	pRxReorderEntry = ieee->RxReorderEntry;
	u8				count = 0;
	RTLLIB_DEBUG(RTLLIB_DL_TS, "==========>%s()\n", __FUNCTION__);
	// Initialize Tx TS related info.
	INIT_LIST_HEAD(&ieee->Tx_TS_Admit_List);
	INIT_LIST_HEAD(&ieee->Tx_TS_Pending_List);
	INIT_LIST_HEAD(&ieee->Tx_TS_Unused_List);

	for(count = 0; count < TOTAL_TS_NUM; count++)
	{
		//
		pTxTS->num = count;
		// The timers for the operation of Traffic Stream and Block Ack.
		// DLS related timer will be add here in the future!!
		_setup_timer(&pTxTS->TsCommonInfo.SetupTimer,
			    TsSetupTimeOut,
			    (unsigned long) pTxTS);

		_setup_timer(&pTxTS->TsCommonInfo.InactTimer,
			    TsInactTimeout,
			    (unsigned long) pTxTS);

		_setup_timer(&pTxTS->TsAddBaTimer,
			    TsAddBaProcess,
			    (unsigned long) pTxTS);
		
		_setup_timer(&pTxTS->TxPendingBARecord.Timer,
			    BaSetupTimeOut, 
			    (unsigned long) pTxTS);
		_setup_timer(&pTxTS->TxAdmittedBARecord.Timer,
			    TxBaInactTimeout,
			    (unsigned long) pTxTS);
		
		ResetTxTsEntry(pTxTS);
		list_add_tail(&pTxTS->TsCommonInfo.List, 
				&ieee->Tx_TS_Unused_List); 
		pTxTS++;
	}

	// Initialize Rx TS related info.
	INIT_LIST_HEAD(&ieee->Rx_TS_Admit_List);
	INIT_LIST_HEAD(&ieee->Rx_TS_Pending_List);
	INIT_LIST_HEAD(&ieee->Rx_TS_Unused_List);
	for(count = 0; count < TOTAL_TS_NUM; count++)
	{
		pRxTS->num = count;
		INIT_LIST_HEAD(&pRxTS->RxPendingPktList);
		
		_setup_timer(&pRxTS->TsCommonInfo.SetupTimer,
			    TsSetupTimeOut,
			    (unsigned long) pRxTS);
	
		_setup_timer(&pRxTS->TsCommonInfo.InactTimer,
			    TsInactTimeout,
			    (unsigned long) pRxTS);

		_setup_timer(&pRxTS->RxAdmittedBARecord.Timer,
			    RxBaInactTimeout,
			    (unsigned long) pRxTS);
		
		_setup_timer(&pRxTS->RxPktPendingTimer,
			    RxPktPendingTimeout,
			    (unsigned long) pRxTS);
		
		ResetRxTsEntry(pRxTS);
		list_add_tail(&pRxTS->TsCommonInfo.List, &ieee->Rx_TS_Unused_List);
		pRxTS++;
	}
	// Initialize unused Rx Reorder List.
	INIT_LIST_HEAD(&ieee->RxReorder_Unused_List);
//#ifdef TO_DO_LIST
	for(count = 0; count < REORDER_ENTRY_NUM; count++)
	{
		list_add_tail( &pRxReorderEntry->List,&ieee->RxReorder_Unused_List);
		if(count == (REORDER_ENTRY_NUM-1))
			break;
		pRxReorderEntry = &ieee->RxReorderEntry[count+1];
	}
//#endif

}

void AdmitTS(struct rtllib_device *ieee, PTS_COMMON_INFO pTsCommonInfo, u32 InactTime)
{
	del_timer_sync(&pTsCommonInfo->SetupTimer);
	del_timer_sync(&pTsCommonInfo->InactTimer);

	if(InactTime!=0)
		mod_timer(&pTsCommonInfo->InactTimer, jiffies + MSECS(InactTime));
}


PTS_COMMON_INFO SearchAdmitTRStream(struct rtllib_device *ieee, u8*	Addr, u8 TID, TR_SELECT	TxRxSelect)
{
	//DIRECTION_VALUE 	dir;
	u8 	dir;
	bool				search_dir[4] = {0, 0, 0, 0};
	struct list_head*		psearch_list; //FIXME
	PTS_COMMON_INFO	pRet = NULL;
	if(ieee->iw_mode == IW_MODE_MASTER) //ap mode
	{
		if(TxRxSelect == TX_DIR)
		{
			search_dir[DIR_DOWN] = true;
			search_dir[DIR_BI_DIR]= true;
		}
		else
		{
			search_dir[DIR_UP] 	= true;
			search_dir[DIR_BI_DIR]= true;
		}
	}
	else if(ieee->iw_mode == IW_MODE_ADHOC)
	{
		if(TxRxSelect == TX_DIR)
			search_dir[DIR_UP] 	= true;
		else
			search_dir[DIR_DOWN] = true;
	}
	else
	{
		if(TxRxSelect == TX_DIR)
		{
			search_dir[DIR_UP] 	= true;
			search_dir[DIR_BI_DIR]= true;
			search_dir[DIR_DIRECT]= true;
		}
		else
		{
			search_dir[DIR_DOWN] = true;
			search_dir[DIR_BI_DIR]= true;
			search_dir[DIR_DIRECT]= true;
		}
	}

	if(TxRxSelect == TX_DIR)
		psearch_list = &ieee->Tx_TS_Admit_List;
	else
		psearch_list = &ieee->Rx_TS_Admit_List;
	
	//for(dir = DIR_UP; dir <= DIR_BI_DIR; dir++)
	for(dir = 0; dir <= DIR_BI_DIR; dir++)
	{
		if(search_dir[dir] ==false )
			continue;
		list_for_each_entry(pRet, psearch_list, List){
	//		RTLLIB_DEBUG(RTLLIB_DL_TS, "ADD:"MAC_FMT", TID:%d, dir:%d\n", MAC_ARG(pRet->Addr), pRet->TSpec.f.TSInfo.field.ucTSID, pRet->TSpec.f.TSInfo.field.ucDirection);
			if (memcmp(pRet->Addr, Addr, 6) == 0)
				if (pRet->TSpec.f.TSInfo.field.ucTSID == TID)
					if(pRet->TSpec.f.TSInfo.field.ucDirection == dir)
					{
	//					printk("Bingo! got it\n");
						break;
					}
					
		}	
		if(&pRet->List  != psearch_list)
			break;
	}

	if(&pRet->List  != psearch_list){
		return pRet ;
	}
	else 
		return NULL;
}

void MakeTSEntry(
		PTS_COMMON_INFO	pTsCommonInfo,
		u8*		Addr,
		PTSPEC_BODY	pTSPEC,
		PQOS_TCLAS	pTCLAS,
		u8		TCLAS_Num,
		u8		TCLAS_Proc
	)
{
	u8	count;

	if(pTsCommonInfo == NULL)
		return;
	
	memcpy(pTsCommonInfo->Addr, Addr, 6);

	if(pTSPEC != NULL)
		memcpy((u8*)(&(pTsCommonInfo->TSpec)), (u8*)pTSPEC, sizeof(TSPEC_BODY));

	for(count = 0; count < TCLAS_Num; count++)
		memcpy((u8*)(&(pTsCommonInfo->TClass[count])), (u8*)pTCLAS, sizeof(QOS_TCLAS));

	pTsCommonInfo->TClasProc = TCLAS_Proc;
	pTsCommonInfo->TClasNum = TCLAS_Num;
}

#ifdef _RTL8192_EXT_PATCH_
void dump_ts_list(struct list_head * ts_list)
{
	PTS_COMMON_INFO	pRet = NULL;
	u8 i=0;
	list_for_each_entry(pRet, ts_list, List){
		printk("i=%d ADD:"MAC_FMT", TID:%d, dir:%d\n",i,MAC_ARG(pRet->Addr), pRet->TSpec.f.TSInfo.field.ucTSID, pRet->TSpec.f.TSInfo.field.ucDirection);
		i++;
 	}
	
}
#endif

bool GetTs(
	struct rtllib_device*	ieee,
	PTS_COMMON_INFO			*ppTS,
	u8*				Addr,
	u8				TID,
	TR_SELECT			TxRxSelect,  //Rx:1, Tx:0
	bool				bAddNewTs
	)
{
	u8	UP = 0;
	//
	// We do not build any TS for Broadcast or Multicast stream.
	// So reject these kinds of search here.
	//
	if(is_broadcast_ether_addr(Addr) || is_multicast_ether_addr(Addr))
	{
		RTLLIB_DEBUG(RTLLIB_DL_ERR, "ERR! get TS for Broadcast or Multicast\n");
		return false;
	}
#if 0
	if(ieee->pStaQos->CurrentQosMode == QOS_DISABLE)
	{	UP = 0; } //only use one TS
	else if(ieee->pStaQos->CurrentQosMode & QOS_WMM)
	{
#else
	if (ieee->current_network.qos_data.supported == 0)
		UP = 0;
	else
	{
#endif
		// In WMM case: we use 4 TID only
		if (!IsACValid(TID))
		{
			RTLLIB_DEBUG(RTLLIB_DL_ERR, "ERR! in %s(), TID(%d) is not valid\n", __FUNCTION__, TID);
			return false;
		}

		switch(TID)
		{
		case 0:
		case 3:
			UP = 0;
			break;

		case 1:
		case 2:
			UP = 2;
			break;
			
		case 4:
		case 5:
			UP = 5;
			break;
			
		case 6:
		case 7:
			UP = 7;
			break;
		}
	}

	*ppTS = SearchAdmitTRStream(
			ieee, 
			Addr,
			UP, 
			TxRxSelect);
	if(*ppTS != NULL)
	{
		return true;
	}
	else
	{
		if(bAddNewTs == false)
		{
			RTLLIB_DEBUG(RTLLIB_DL_TS, "add new TS failed(tid:%d)\n", UP);
			return false;
		}
		else
		{
			// 
			// Create a new Traffic stream for current Tx/Rx
			// This is for EDCA and WMM to add a new TS.
			// For HCCA or WMMSA, TS cannot be addmit without negotiation.
			//
			TSPEC_BODY	TSpec;
			PQOS_TSINFO		pTSInfo = &TSpec.f.TSInfo;
			struct list_head*	pUnusedList = 
								(TxRxSelect == TX_DIR)?
								(&ieee->Tx_TS_Unused_List):
								(&ieee->Rx_TS_Unused_List);
								
			struct list_head*	pAddmitList = 
								(TxRxSelect == TX_DIR)?
								(&ieee->Tx_TS_Admit_List):
								(&ieee->Rx_TS_Admit_List);

			DIRECTION_VALUE		Dir =		(ieee->iw_mode == IW_MODE_MASTER)? 
								((TxRxSelect==TX_DIR)?DIR_DOWN:DIR_UP):
								((TxRxSelect==TX_DIR)?DIR_UP:DIR_DOWN);
			RTLLIB_DEBUG(RTLLIB_DL_TS, "to add Ts\n");
			if(!list_empty(pUnusedList))
			{
				(*ppTS) = list_entry(pUnusedList->next, TS_COMMON_INFO, List);
				list_del_init(&(*ppTS)->List);	
				if(TxRxSelect==TX_DIR)
				{
					PTX_TS_RECORD tmp = container_of(*ppTS, TX_TS_RECORD, TsCommonInfo);
					ResetTxTsEntry(tmp);
				}
				else{
					PRX_TS_RECORD tmp = container_of(*ppTS, RX_TS_RECORD, TsCommonInfo);	
					ResetRxTsEntry(tmp);
				}
				
				RTLLIB_DEBUG(RTLLIB_DL_TS, "to init current TS, UP:%d, Dir:%d, addr:"MAC_FMT"\n", UP, Dir, MAC_ARG(Addr));
				// Prepare TS Info releated field
				pTSInfo->field.ucTrafficType = 0;			// Traffic type: WMM is reserved in this field
				pTSInfo->field.ucTSID = UP;			// TSID
				pTSInfo->field.ucDirection = Dir;			// Direction: if there is DirectLink, this need additional consideration.
				pTSInfo->field.ucAccessPolicy = 1;		// Access policy
				pTSInfo->field.ucAggregation = 0; 		// Aggregation
				pTSInfo->field.ucPSB = 0; 				// Aggregation
				pTSInfo->field.ucUP = UP;				// User priority
				pTSInfo->field.ucTSInfoAckPolicy = 0;		// Ack policy
				pTSInfo->field.ucSchedule = 0;			// Schedule
					
				MakeTSEntry(*ppTS, Addr, &TSpec, NULL, 0, 0);
				AdmitTS(ieee, *ppTS, 0);
				list_add_tail(&((*ppTS)->List), pAddmitList);
				// if there is DirectLink, we need to do additional operation here!!

				return true;
			}
			else
			{
				RTLLIB_DEBUG(RTLLIB_DL_ERR, "ERR!!in function %s() There is not enough dir=%d(0=up down=1) TS record to be used!!", __FUNCTION__,Dir);
				return false;
			}		
		}		
	}
}

void RemoveTsEntry(
	struct rtllib_device*	ieee,
	PTS_COMMON_INFO			pTs,
	TR_SELECT			TxRxSelect
	)
{
	//u32 flags = 0;
	unsigned long flags = 0;
	del_timer_sync(&pTs->SetupTimer);
	del_timer_sync(&pTs->InactTimer);
	TsInitDelBA(ieee, pTs, TxRxSelect);

	if(TxRxSelect == RX_DIR)
	{
//#ifdef TO_DO_LIST
		PRX_REORDER_ENTRY	pRxReorderEntry;
		PRX_TS_RECORD 		pRxTS = (PRX_TS_RECORD)pTs;
		if(timer_pending(&pRxTS->RxPktPendingTimer))	
			del_timer_sync(&pRxTS->RxPktPendingTimer);

                while(!list_empty(&pRxTS->RxPendingPktList))
                {
                //      PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
                        spin_lock_irqsave(&(ieee->reorder_spinlock), flags);
                        //pRxReorderEntry = list_entry(&pRxTS->RxPendingPktList.prev,RX_REORDER_ENTRY,List);
			pRxReorderEntry = (PRX_REORDER_ENTRY)list_entry(pRxTS->RxPendingPktList.prev,RX_REORDER_ENTRY,List);
                        list_del_init(&pRxReorderEntry->List);
                        {
                                int i = 0;
                                struct rtllib_rxb * prxb = pRxReorderEntry->prxb;
				if (unlikely(!prxb))
				{
					spin_unlock_irqrestore(&(ieee->reorder_spinlock), flags);
					return;
				}
                                for(i =0; i < prxb->nr_subframes; i++) {
                                        dev_kfree_skb(prxb->subframes[i]);
                                }
                                kfree(prxb);
                                prxb = NULL;
                        }
                        list_add_tail(&pRxReorderEntry->List,&ieee->RxReorder_Unused_List);
                        //PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
                        spin_unlock_irqrestore(&(ieee->reorder_spinlock), flags);
                }

//#endif
	}
	else
	{
		PTX_TS_RECORD pTxTS = (PTX_TS_RECORD)pTs;
		del_timer_sync(&pTxTS->TsAddBaTimer);
	}
}

void RemovePeerTS(struct rtllib_device* ieee, u8* Addr)
{
	PTS_COMMON_INFO	pTS, pTmpTS;
	printk("===========>RemovePeerTS,"MAC_FMT"\n", MAC_ARG(Addr));
#if 1
	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Tx_TS_Pending_List, List)
	{
		if (memcmp(pTS->Addr, Addr, 6) == 0)
		{
			RemoveTsEntry(ieee, pTS, TX_DIR);
			list_del_init(&pTS->List);
			list_add_tail(&pTS->List, &ieee->Tx_TS_Unused_List);
		}
	}
	
	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Tx_TS_Admit_List, List)
	{
		if (memcmp(pTS->Addr, Addr, 6) == 0)
		{
			printk("====>remove Tx_TS_admin_list\n");
			RemoveTsEntry(ieee, pTS, TX_DIR);
			list_del_init(&pTS->List);
			list_add_tail(&pTS->List, &ieee->Tx_TS_Unused_List);
		}
	}
	
	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Rx_TS_Pending_List, List)
	{
		if (memcmp(pTS->Addr, Addr, 6) == 0)
		{
			RemoveTsEntry(ieee, pTS, RX_DIR);
			list_del_init(&pTS->List);
			list_add_tail(&pTS->List, &ieee->Rx_TS_Unused_List);
		}
	}

	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Rx_TS_Admit_List, List)
	{
		if (memcmp(pTS->Addr, Addr, 6) == 0)
		{
			RemoveTsEntry(ieee, pTS, RX_DIR);
			list_del_init(&pTS->List);
			list_add_tail(&pTS->List, &ieee->Rx_TS_Unused_List);
		}
	}
#endif
}

void RemoveAllTS(struct rtllib_device* ieee)
{
	PTS_COMMON_INFO pTS, pTmpTS;
#if 1	
	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Tx_TS_Pending_List, List)
	{
		RemoveTsEntry(ieee, pTS, TX_DIR);
		list_del_init(&pTS->List);
		list_add_tail(&pTS->List, &ieee->Tx_TS_Unused_List);	
	}

	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Tx_TS_Admit_List, List)
	{
		RemoveTsEntry(ieee, pTS, TX_DIR);
		list_del_init(&pTS->List);
		list_add_tail(&pTS->List, &ieee->Tx_TS_Unused_List);
	}
	
	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Rx_TS_Pending_List, List)
	{
		RemoveTsEntry(ieee, pTS, RX_DIR);
		list_del_init(&pTS->List);
		list_add_tail(&pTS->List, &ieee->Rx_TS_Unused_List);
	}

	list_for_each_entry_safe(pTS, pTmpTS, &ieee->Rx_TS_Admit_List, List)
	{
		RemoveTsEntry(ieee, pTS, RX_DIR);
		list_del_init(&pTS->List);
		list_add_tail(&pTS->List, &ieee->Rx_TS_Unused_List);
	}
#endif
}

void TsStartAddBaProcess(struct rtllib_device* ieee, PTX_TS_RECORD	pTxTS)
{
	if(pTxTS->bAddBaReqInProgress == false)
	{
		pTxTS->bAddBaReqInProgress = true;
#if 1
		if(pTxTS->bAddBaReqDelayed)
		{
			RTLLIB_DEBUG(RTLLIB_DL_BA, "TsStartAddBaProcess(): Delayed Start ADDBA after 60 sec!!\n");
			mod_timer(&pTxTS->TsAddBaTimer, jiffies + MSECS(TS_ADDBA_DELAY));
		}
		else
		{
			RTLLIB_DEBUG(RTLLIB_DL_BA,"TsStartAddBaProcess(): Immediately Start ADDBA now!!\n");
			mod_timer(&pTxTS->TsAddBaTimer, jiffies+10); //set 10 ticks
		}
#endif
	}
	else
		RTLLIB_DEBUG(RTLLIB_DL_ERR, "%s()==>BA timer is already added\n", __FUNCTION__);
}

#ifndef BUILT_IN_RTLLIB
EXPORT_SYMBOL_RSL(RemovePeerTS);
#ifdef _RTL8192_EXT_PATCH_
EXPORT_SYMBOL_RSL(ResetAdmitTRStream);
#endif
#endif
