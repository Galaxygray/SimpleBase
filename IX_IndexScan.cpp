#include "index.h"

bool iequal(void * value1, void * value2, AttrType attrtype, int attrLength) {
	switch (attrtype) {
	case FLOAT: return (*(float *)value1 == *(float*)value2);
	case INT: return (*(int *)value1 == *(int *)value2);
	default:
		return (strncmp((char *)value1, (char *)value2, attrLength) == 0);
	}
}

bool iless_than(void * value1, void * value2, AttrType attrtype, int attrLength) {
	switch (attrtype) {
	case FLOAT: return (*(float *)value1 < *(float*)value2);
	case INT: return (*(int *)value1 < *(int *)value2);
	default:
		return (strncmp((char *)value1, (char *)value2, attrLength) < 0);
	}
}

bool igreater_than(void * value1, void * value2, AttrType attrtype, int attrLength) {
	switch (attrtype) {
	case FLOAT: return (*(float *)value1 > *(float*)value2);
	case INT: return (*(int *)value1 > *(int *)value2);
	default:
		return (strncmp((char *)value1, (char *)value2, attrLength) > 0);
	}
}

bool iless_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength) {
	switch (attrtype) {
	case FLOAT: return (*(float *)value1 <= *(float*)value2);
	case INT: return (*(int *)value1 <= *(int *)value2);
	default:
		return (strncmp((char *)value1, (char *)value2, attrLength) <= 0);
	}
}

bool igreater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength) {
	switch (attrtype) {
	case FLOAT: return (*(float *)value1 >= *(float*)value2);
	case INT: return (*(int *)value1 >= *(int *)value2);
	default:
		return (strncmp((char *)value1, (char *)value2, attrLength) >= 0);
	}
}

bool inot_equal(void * value1, void * value2, AttrType attrtype, int attrLength) {
	switch (attrtype) {
	case FLOAT: return (*(float *)value1 != *(float*)value2);
	case INT: return (*(int *)value1 != *(int *)value2);
	default:
		return (strncmp((char *)value1, (char *)value2, attrLength) != 0);
	}
}


IX_IndexScan::IX_IndexScan() {
	openScan = false;  // Initialize all values
	value = NULL;
	initializedValue = false;
	hasBucketPinned = false;
	hasLeafPinned = false;
	scanEnded = true;
	scanStarted = false;
	endOfIndexReached = true;
	attrLength = 0;
	attrType = INT;

	foundFirstValue = false;
	foundLastValue = false;
	useFirstLeaf = false;
}

IX_IndexScan::~IX_IndexScan() {
	if (scanEnded == false && hasBucketPinned == true)
		indexHandle->pfh.UnpinPage(currBucketNum);
	if (scanEnded == false && hasLeafPinned == true && (currLeafNum != (indexHandle->header).rootPage))
		indexHandle->pfh.UnpinPage(currLeafNum);
	if (initializedValue == true) {
		free(value);
		initializedValue = false;
	}

}

Status IX_IndexScan::OpenScan(const IX_IndexHandle &indexhandle, CompOp op, void *value, ClientHint pinHint) {
	if (openScan || op == NE_OP)
		return -1;
	this->indexHandle = const_cast<IX_IndexHandle *>(&indexhandle);

	this->value = NULL;
	useFirstLeaf = true;

	this->compOp = op;
	switch (op) {
		case EQ_OP:	comparator = &iequal; useFirstLeaf = false; break;
		case LT_OP: comparator = &iless_than; break;
		case GT_OP: comparator = &igreater_than; useFirstLeaf = false; break;
		case LE_OP: comparator = &iless_than_or_eq_to; break;
		case GE_OP: comparator = &igreater_than_or_eq_to; useFirstLeaf = false; break;
		case NO_OP: comparator = NULL; break;
		default: return -1;
	}

	this->attrType = indexhandle.header.attr_type;
	this->attrLength = indexhandle.header.attr_length;
	if (op != NO_OP) {
		this->value = new char[attrLength];
		memcpy(this->value, value, attrLength);
		initializedValue = true;
	}

	openScan = true;
	scanEnded = false;
	hasLeafPinned = false;
	scanStarted = false;
	endOfIndexReached = false;
	foundFirstValue = false;
	foundLastValue = false;
	return 0;
}

Status IX_IndexScan::BeginScan(PageHandle &leafph, int &pagenum) {
	if (useFirstLeaf) {
		if (indexHandle->GetFirstLeafPage(leafph, pagenum))
			return -1;
		if (GetFirstEntryInLeaf(currLeafPH)) {
			scanEnded = true;
			return -1;
		}
		return 0;
	}
	else {
		if (indexHandle->FindRecordPage(leafph, pagenum, value))
			return -1;
		if (GetAppropriateEntryInLeaf(leafph)) {
			scanEnded = true;
			return -1;
		}
		return 0;
	}
}

Status IX_IndexScan::GetNextEntry(RID &rid) {
	if (scanEnded)
		return -1;
	if (foundLastValue)
		return -1;
	bool notFound = true;
	while (notFound) {
		if (openScan && !scanEnded && !scanStarted) {
			if (BeginScan(currLeafPH, currLeafNum))
				return -1;
			currKey = nextNextKey;
			scanStarted = true;
			SetRID(true);
			if (FindNextValue())
				endOfIndexReached = true;
		}
		else {
			currKey = nextKey;
			currRID = nextRID;
		}
		SetRID(false);
		nextKey = nextNextKey;
		if (FindNextValue())
			endOfIndexReached = true;

		int thispagenum;
		if (currRID.GetPageNum(thispagenum))
			return -1;
		if (thispagenum == -1)
		{
			scanEnded = true;
			return -1;
		}
		
		if (compOp == NO_OP) {
			rid = currRID;
			notFound = false;
			foundFirstValue = true;
		}
		else if (comparator((void *)currKey, value, attrType, attrLength)) {
			rid = currRID;
			notFound = false;
			foundFirstValue = true;
		}
		else if (foundFirstValue)
			foundLastValue = true;

	}
	int slot;
	currRID.GetSlotNum(slot);
	return  0;
}

Status IX_IndexScan::SetRID(bool setcurrent) {
	if (endOfIndexReached && !setcurrent)
	{
		nextRID = RID(-1, -1);
		return 0;
	}

	if (setcurrent) {
		if (hasLeafPinned) {
			currRID = RID(leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
		}
	}
	else {
		if (hasLeafPinned)
			nextRID = RID(leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
	}

	return 0;
}


Status IX_IndexScan::FindNextValue() {
	int prevLeafSlot = leafSlot;
	leafSlot = leafEntries[leafSlot].nextSlot;
	if (leafSlot != -1 && leafEntries[leafSlot].isValid) {
		nextNextKey = leafKeys + leafSlot * attrLength;
		return 0;
	}

	int nextleafpage = leafHeader->next;
	if (currLeafNum != indexHandle->header.rootPage) {
		if (indexHandle->pfh.UnpinPage(currLeafNum))
			return -1;
	}
	hasLeafPinned = false;

	if (nextleafpage = -1) {
		currLeafNum = nextleafpage;
		if (indexHandle->pfh.GetThisPage(currLeafNum, currLeafPH))
			return -1;
		if (GetFirstEntryInLeaf(currLeafPH))
			return -1;
		return 0;
	}

	return -1;
}

Status IX_IndexScan::GetFirstEntryInLeaf(PageHandle &leafph) {
	hasLeafPinned = true;
	if (leafph.GetData((char *&)leafHeader))
		return -1;
	if (leafHeader->num_keys == 0)
		return -1;

	leafEntries = (Node_Entry *)((char *)leafHeader + indexHandle->header.entryOffset_N);
	leafKeys = (char *)leafHeader + indexHandle->header.keysOffset_N;

	leafSlot = leafHeader->firstSlotIndex;
	if (leafSlot != -1)
		nextNextKey = leafKeys + leafSlot * attrLength;
	else
		return -1;
	return 0;
}

Status IX_IndexScan::GetAppropriateEntryInLeaf(PageHandle &leafph) {
	hasLeafPinned = true;
	if (leafph.GetData((char *&)leafHeader))
		return -1;

	if (leafHeader->num_keys == 0)
		return -1;

	leafEntries = (Node_Entry *)((char *)leafHeader + indexHandle->header.entryOffset_N);
	leafKeys = (char *)leafHeader + indexHandle->header.keysOffset_N;

	int index = 0;
	bool isDup = false;
	if (indexHandle->FindNodeInsertIndex(leafHeader, value, index, isDup))
		return -1;

	leafSlot = index;
	if (leafSlot != -1)
		nextNextKey = leafKeys + leafSlot * attrLength;
	else
		return -1;
	return 0;
}


Status IX_IndexScan::CloseScan() {
	if (openScan == false)
		return -1;
	if (scanEnded == false && hasLeafPinned == true && (currLeafNum != (indexHandle->header).rootPage))
		indexHandle->pfh.UnpinPage(currLeafNum);
	if (initializedValue == true) {
		free(value);
		initializedValue = false;
	}
	openScan = false;
	scanStarted = false;

	return 0;
}