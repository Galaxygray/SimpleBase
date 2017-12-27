#include "index.h"

IX_IndexHandle::IX_IndexHandle() {
	isOpenHandle = false;
	header_modified = false;
}

IX_IndexHandle::~IX_IndexHandle() {

}

Status IX_IndexHandle::InsertEntry(void* pData, const RID &rid) {
	if (isOpenHandle == false)
		return -1;
	IX_NodeHeader* rootheader;
	if (rootPH.GetData((char *&)rootheader))
		return -1;

	if (rootheader->num_keys == header.maxKeys_N) {
		int newrootpage;
		char* newrootdata;
		PageHandle newrootPH;
		if (CreateNewNode(newrootPH, newrootpage, newrootdata, false))
			return -1;
		IX_NodeHeader *newrootheader = (IX_NodeHeader *)newrootdata;
		newrootheader->isEmpty = false;
		newrootheader->firstSlotIndex = header.rootPage;
		int unused, unusedpage;
		if (SplitNode(newrootheader, rootheader, header.rootPage, -1, unused, unusedpage))
			return -1;
		if (pfh.MarkDirty(header.rootPage) || pfh.UnpinPage(header.rootPage))
			return -1;
		rootPH = newrootPH;
		header.rootPage = newrootpage;
		header_modified = true;
		IX_NodeHeader *useme;
		if (newrootPH.GetData((char *&)useme))
			return - 1;
		if (InsertIntoNonFullNode(useme, header.rootPage, pData, rid))
			return -1;
	}
	else {
		if (InsertIntoNonFullNode(rootheader, header.rootPage, pData, rid))
			return -1;
	}
	if (pfh.MarkDirty(header.rootPage) || pfh.UnpinPage(header.rootPage))
		return -1;
	return 0;
}

Status IX_IndexHandle::SplitNode(struct IX_NodeHeader *pHeader, struct IX_NodeHeader *oldHeader,
	int oldPage, int index, int & newKeyIndex, int &newPageNum) {
	bool isleaf = false;
	if (oldHeader->isLeafNode) {
		isleaf = true;
	}
	int numpage;
	IX_NodeHeader *newheader;
	PageHandle newPH;
	if (CreateNewNode(newPH, numpage, (char *&)newheader, isleaf))
		return -1;
	newPageNum = numpage;

	Node_Entry *pEntries = (Node_Entry *)((char *)pHeader + header.entryOffset_N);
	Node_Entry *oldEntries = (Node_Entry *)((char *)oldHeader + header.entryOffset_N);
	Node_Entry *newEntries = (Node_Entry *)((char *)newheader + header.entryOffset_N);
	char *pKeys = (char *)(&pHeader + header.entryOffset_N);
	char *newKeys = (char *)(&newheader + header.entryOffset_N);
	char *oldKeys = (char *)(&oldHeader + header.entryOffset_N);

	int preid = -1;
	int curid = oldHeader->firstSlotIndex;
	for (int i = 0; i < oldHeader->num_keys / 2; i++) {
		preid = curid;
		curid = oldEntries[i].nextSlot;
	}
	oldEntries[preid].nextSlot = -1;

	char* parentKey = oldKeys + curid * header.attr_length;
	if (!isleaf) {
		newheader->next = oldEntries[curid].page;
		newheader->isEmpty = false;
		preid = curid;
		curid = oldEntries[preid].nextSlot;
		oldEntries[preid].nextSlot = oldHeader->freeSlotIndex;
		oldHeader->freeSlotIndex = preid;
		oldHeader->num_keys--;
	}

	int preid2 = -1;
	int curid2 = newheader->freeSlotIndex;
	while (curid != -1) {
		newEntries[curid2].page = oldEntries[curid].page;
		newEntries[curid2].isValid = oldEntries[curid].isValid;
		newEntries[curid2].slot = oldEntries[curid].slot;
		memcpy(newKeys + curid2 * header.attr_length, oldKeys + curid * header.attr_length, header.attr_length);
		if (preid2 == -1) {
			newheader->freeSlotIndex = newEntries[curid].page;
			newEntries[curid2].nextSlot = newheader->firstSlotIndex;
			newheader->firstSlotIndex = curid2;
		}
		else {
			newheader->freeSlotIndex = newEntries[curid].page;
			newEntries[curid2].nextSlot = newEntries[preid2].nextSlot;
			newheader->firstSlotIndex = curid2;
		}
		preid2 = curid2;
		curid2 = newheader->freeSlotIndex;

		preid = curid;
		curid = oldEntries[preid].nextSlot;
		oldEntries[preid].nextSlot = oldHeader->freeSlotIndex;
		oldHeader->freeSlotIndex = preid;
		oldHeader->num_keys--;
		newheader->num_keys++;
	}

	int loc = pHeader->freeSlotIndex;
	memcpy(pKeys + loc * header.attr_length, parentKey, header.attr_length);
	newKeyIndex = loc;  
	pEntries[loc].page = numpage;
	pEntries[loc].isValid = OCCUPIED_NEW;

	if (index == -1) {
		pHeader->freeSlotIndex = pEntries[loc].nextSlot;
		pEntries[loc].nextSlot = pHeader->firstSlotIndex;
		pHeader->firstSlotIndex = loc;
	}
	else {
		pHeader->freeSlotIndex = pEntries[loc].nextSlot;
		pEntries[loc].nextSlot = pEntries[index].nextSlot;
		pEntries[index].nextSlot = loc;
	}
	pHeader->num_keys++;

	// if is leaf node, update the page pointers to the previous and next leaf node
	if (isleaf) {
		newheader->next = oldHeader->next;
		newheader->pre = oldPage;
		oldHeader->next = numpage;
		if (newheader->next != -1) {
			PageHandle nextPH;
			struct IX_NodeHeader *nextHeader;
			if (pfh.GetThisPage(newheader->next, nextPH) || (nextPH.GetData((char *&)nextHeader)))
				return -1;
			nextHeader->pre = numpage;
			if (pfh.MarkDirty(newheader->next) ||  pfh.UnpinPage(newheader->next))
				return -1;
		}
	}

	// Mark the new page as dirty, and unpin it
	if (pfh.MarkDirty(numpage) || pfh.UnpinPage(numpage)) {
		return -1;
	}
	return 0;
}

Status  IX_IndexHandle::InsertIntoNonFullNode(IX_NodeHeader *nHeader, int thisNodeNum, void *pData, const RID &rid) {
	struct Node_Entry *entries = (Node_Entry *)((char *)nHeader + header.entryOffset_N);
	char *Keys = (char *)nHeader + header.keysOffset_N;

	if (nHeader->isLeafNode) {
		int preindex = -1;
		bool isDup = false;
		if (FindNodeInsertIndex(nHeader, pData, preindex, isDup))
			return -1;
		int index = nHeader->freeSlotIndex;
		memcpy(Keys + header.attr_length * index, pData, header.attr_length);
		entries[index].isValid = 1;
		if (rid.GetPageNum(entries[index].page) || rid.GetSlotNum(entries[index].slot))
			return -1;
		nHeader->isEmpty = false;
		nHeader->num_keys++;
		nHeader->freeSlotIndex = entries[index].nextSlot;
		if (preindex == -1) {
			entries[index].nextSlot = nHeader->firstSlotIndex;
			nHeader->firstSlotIndex = index;
		}
		else {
			entries[index].nextSlot = entries[preindex].nextSlot;
			entries[preindex].nextSlot = index;
		}
	}
	else {
		int preindex = -1;
		bool isDup = false;
		if (FindNodeInsertIndex(nHeader, pData, preindex, isDup))
			return -1;
		int nextpage;
		if (preindex == -1) {
			nextpage = nHeader->next;
		}
		else {
			nextpage = entries[preindex].page;
		}
		PageHandle nextpageph;
		IX_NodeHeader *nextNodeHeader;
		int newkeyindex;
		int newpagenum;
		if (pfh.GetThisPage(nextpage, nextpageph) || nextpageph.GetData((char *&)nextNodeHeader))
			return -1;
		if (nextNodeHeader->num_keys == header.maxKeys_N) {
			if (SplitNode(nHeader, nextNodeHeader, nextpage, preindex, newkeyindex, newpagenum))
				return -1;
			char *key = Keys + newkeyindex * header.attr_length;

			int compared = comparator(pData, key, header.attr_length);
			if (compared >= 0) {
				int nextp = newpagenum;
				if (pfh.MarkDirty(nextp) || pfh.UnpinPage(nextp))
					return -1;
				if (pfh.GetThisPage(nextpage, nextpageph) || nextpageph.GetData((char *&)nextNodeHeader))
					return -1;
				nextpage = nextp;
			}
		}
		else {
			if (( InsertIntoNonFullNode(nextNodeHeader, nextpage, pData, rid)))
				return -1;
			if ( pfh.MarkDirty(nextpage) || pfh.UnpinPage(nextpage))
				return -1;
		}
	}
	return 0;
}





Status IX_IndexHandle::FindNodeInsertIndex(IX_NodeHeader *nHeader, void *pData, int &preindex, bool &isDup, const RID& rid) {
	struct Node_Entry *entries = (Node_Entry *)((char *)nHeader + header.entryOffset_N);
	char *Keys = (char *)nHeader + header.keysOffset_N;
	int pagenum, slot;
	if (rid.GetPageNum(pagenum) || rid.GetSlotNum(slot))
		return -1;
	preindex = -1;
	int curindex = nHeader->firstSlotIndex;
	while (curindex != -1) {
		char* value = Keys + curindex * header.attr_length;
		int compared = comparator(pData, value, header.attr_length);
		if (compared == 0) {
			isDup = true;
			if ((~pagenum && compared == 0 && entries[curindex].page == pagenum && entries[curindex].slot == slot)) {
				preindex = curindex;
				break;
			}
		}
		if (compared < 0)
			break;
		preindex = curindex;
		curindex = entries[preindex].nextSlot;
	}
	return 0;
}


Status IX_IndexHandle::DeleteEntry(void* pData, const RID &rid) {
	IX_NodeHeader *rHeader;
	if (rootPH.GetData((char *&)rHeader))
		return -1;

	if (rHeader->isEmpty || (!rHeader->isLeafNode))
		return -1;
	if (rHeader->num_keys == 0 && rHeader->isLeafNode)
		return -1;

	bool toDelete = false;
	if (DeleteFromNode(rHeader, pData, rid, toDelete))
		return -1;
	int rootpage;
	if (rootPH.GetPageNum(rootpage) || pfh.MarkDirty(rootpage) || pfh.UnpinPage(rootpage))
		return -1;
	if (toDelete)
		rHeader->isLeafNode = true;
	return 0;
}

Status IX_IndexHandle::DeleteFromNode(IX_NodeHeader *nHeader, void *pData, const RID &rid, bool &toDelete) {
	toDelete = false;
	if (nHeader->isLeafNode) {
		if (DeleteFromLeaf(nHeader, pData, rid, toDelete))
			return -1;

	}
	else{
		int preindex, curindex;
		bool isDup = false;
		if (FindNodeInsertIndex(nHeader, pData, curindex, isDup))
			return -1;
		Node_Entry *entries = (Node_Entry *)((char * )nHeader + header.entryOffset_N);

		int nextnodepage;
		bool usedfirstpage = false;
		if (curindex == -1) {
			usedfirstpage = true;
			nextnodepage = nHeader->next;
			preindex = curindex;
		}
		else {
			if (FindPrevIndex(nHeader, curindex, preindex))
				return -1;
			nextnodepage = entries[curindex].page;
		}
		PageHandle nextNodePH;
		IX_NodeHeader *nextHeader;
		if (pfh.GetThisPage(nextnodepage, nextNodePH) || nextNodePH.GetData((char *&)nextHeader))
			return -1;
		bool toDeleteNext = false;
		if (DeleteFromNode(nextHeader, pData, rid, toDeleteNext))
			return -1;
		if (pfh.MarkDirty(nextnodepage) || pfh.UnpinPage(nextnodepage))
			return -1;

		if (toDeleteNext) {
			if (pfh.DisposePage(nextnodepage))
				return -1;
			if (usedfirstpage == false) {
				if (preindex == -1) {
					nHeader->firstSlotIndex = entries[curindex].nextSlot;
				}
				else
					entries[preindex].nextSlot = entries[curindex].nextSlot;
				entries[curindex].nextSlot = nHeader->freeSlotIndex;
				nHeader->freeSlotIndex = curindex;
			}
			else {
				int firstslot = nHeader->firstSlotIndex;
				nHeader->firstSlotIndex = entries[curindex].nextSlot;
				nHeader->next = entries[firstslot].page;
				entries[firstslot].nextSlot = nHeader->freeSlotIndex;
				nHeader->firstSlotIndex = firstslot;
			}
			if (nHeader->num_keys == 0) {
				nHeader->isEmpty = true;
				toDelete = true;
			}
			nHeader->num_keys--;
		}
	}
	return 0;
}

Status IX_IndexHandle::FindPrevIndex(IX_NodeHeader *nHeader, int thisindex, int &preindex)
{
	Node_Entry *entries = (Node_Entry *)((char *)nHeader + header.entryOffset_N);
	int preid = -1;
	int curid = nHeader->firstSlotIndex;
	while (curid != thisindex) {
		preid = curid;
		curid = entries[preid].nextSlot;
	}
	preindex = preid;
	return 0;
}

Status IX_IndexHandle::DeleteFromLeaf(IX_NodeHeader *nHeader, void *pData, const RID &rid, bool &toDelete) {
	int preindex, curindex;
	bool isDup;
	if (FindNodeInsertIndex(nHeader, pData, curindex, isDup, rid)) {
		return -1;
	}
	if (!isDup)
		return -1;
	Node_Entry *entries = (Node_Entry *)((char *)nHeader + header.entryOffset_N);
	char *key = (char *)(nHeader + header.keysOffset_N);
	int pagenum, slot;
	if (rid.GetPageNum(pagenum) || rid.GetSlotNum(slot))
		return -1;
	if (entries[curindex].page != pagenum || entries[curindex].slot != slot)
		return -1;

	if (curindex == nHeader->firstSlotIndex) {
		preindex = curindex;
	}
	else {
		if (FindPrevIndex(nHeader, curindex, preindex)) {
			return -1;
		}
	}
	if (curindex == nHeader->firstSlotIndex)
		nHeader->firstSlotIndex = entries[curindex].nextSlot;
	else
		entries[preindex].nextSlot = entries[curindex].nextSlot;
	entries[curindex].nextSlot = nHeader->freeSlotIndex;
	nHeader->freeSlotIndex = entries[curindex].nextSlot;
	entries[curindex].isValid = false;
	nHeader->num_keys--;
	if (nHeader->num_keys == 0) {
		toDelete = true;
		int prevpage = nHeader->pre;
		int nextpage = nHeader->next;
		PageHandle ph;
		IX_NodeHeader *header;
		if (prevpage != -1) {
			if (pfh.GetThisPage(prevpage, ph) || ph.GetData((char *&)header))
				return -1;
			header->next = nextpage;
			if (pfh.MarkDirty(prevpage) || pfh.UnpinPage(prevpage))
				return -1;
		}
		if (nextpage != -1) {
			if (pfh.GetThisPage(nextpage, ph) || ph.GetData((char *&)header))
				return -1;
			header->pre = prevpage;
			if (pfh.MarkDirty(nextpage) || ph.GetData((char *&)header))
				return -1;
		}
	}
	return 0;
}

Status IX_IndexHandle::CreateNewNode(PageHandle &ph, int &page, char *&pData, bool isLeaf) {
	if (pfh.AllocatePage(ph) || ph.GetPageNum(page))
		return -1;
	if (ph.GetData(pData))
		return -1;
	IX_NodeHeader *nHeader = (IX_NodeHeader *)pData;

	nHeader->isLeafNode = isLeaf;
	nHeader->isEmpty = true;
	nHeader->num_keys = 0;
	nHeader->next = nHeader->pre = -1;
	nHeader->firstSlotIndex = -1;
	nHeader->freeSlotIndex = 0;

	Node_Entry *entreis = (Node_Entry *)((char *)nHeader + header.attr_length);
	for (int i = 0; i < header.maxKeys_N; i++) {
		entreis[i].isValid = -1;
		entreis[i].nextSlot = i + 1;
		entreis[i].page = -1;
	}
	entreis[header.maxKeys_N - 1].nextSlot = -1;
	return -1;
}

Status IX_IndexHandle::GetFirstLeafPage(PageHandle &ph, int &leafpage) {
	IX_NodeHeader *rHeader;
	if (rootPH.GetData((char *&)rHeader))
		return -1;
	
	if (rHeader->isLeafNode == true) {
		ph = rootPH;
		rootPH.GetPageNum(leafpage);
		return 0;
	}
	
	int nextpagenum = rHeader->next;
	PageHandle nextph;
	IX_NodeHeader *nextheader;
	if (nextpagenum == -1) {
		return -1;
	}
	if (pfh.GetThisPage(nextpagenum, nextph) || nextph.GetData((char *&)nextheader))
		return -1;
	while (nextheader->isLeafNode == false) {
		int prenum = nextpagenum;
		nextpagenum = nextheader->next;
		if (pfh.UnpinPage(prenum))
			return -1;
		if (pfh.GetThisPage(nextpagenum, nextph) || nextph.GetData((char *&)nextheader))
			return -1;
	}
	ph = nextph;
	leafpage = nextpagenum;
	return 0;
}


Status IX_IndexHandle::FindRecordPage(PageHandle &ph, int &pagenum, void *key) {
	IX_NodeHeader *nHeader;
	if (rootPH.GetData((char *&)nHeader))
		return -1;

	if (nHeader->isLeafNode == true) {
		pagenum = header.rootPage;
		ph = rootPH;
		return 0;
	}

	int index = -1;
	bool isDup = false;
	int nextPageNum;
	PageHandle nextPH;
	if (FindNodeInsertIndex((struct IX_NodeHeader *)nHeader, key, index, isDup))
		return -1;
	struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
	if (index == -1)
		nextPageNum = nHeader->next;
	else
		nextPageNum = entries[index].page;
	

	if ( pfh.GetThisPage(nextPageNum, nextPH) ||  nextPH.GetData((char *&)nHeader))
		return -1;

	while (nHeader->isLeafNode == false) {
		if ( FindNodeInsertIndex((struct IX_NodeHeader *)nHeader, key, index, isDup))
			return -1;

		entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
		int prevPage = nextPageNum;
		if (index == -1)
			nextPageNum = nHeader->next;
		else
			nextPageNum = entries[index].page;
		//char *keys = (char *)nHeader + header.keysOffset_N; 
		if (pfh.UnpinPage(prevPage))
			return 0;
		if (pfh.GetThisPage(nextPageNum, nextPH) ||  nextPH.GetData((char *&)nHeader))
			return 0;
	}
	pagenum = nextPageNum;
	ph = nextPH;
	return 0;
}


Status IX_IndexHandle::ForcePages() {
	if(pfh.FlushToDisk())
		return -1;
	return 0;
}


int IX_IndexHandle::CalcNumKeysNode(int attrLength) {
	return floor((PAGE_SIZE - sizeof(IX_NodeHeader)) * 1. / (attrLength + sizeof(Node_Entry)));
}


static int compare_string(void *value1, void* value2, int attrLength) {
	return strncmp((char *)value1, (char *)value2, attrLength);
}

static int compare_int(void *value1, void* value2, int attrLength) {
	if ((*(int *)value1 < *(int *)value2))
		return -1;
	else if ((*(int *)value1 > *(int *)value2))
		return 1;
	else
		return 0;
}

static int compare_float(void *value1, void* value2, int attrLength) {
	if ((*(float *)value1 < *(float *)value2))
		return -1;
	else if ((*(float *)value1 > *(float *)value2))
		return 1;
	else
		return 0;
}