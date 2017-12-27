#include "Record.h"

RM_FileScan::RM_FileScan() {
	openScan = false;
	value = NULL;
	initializedValue = false;
	hasPagePinned = false;
	scanEnded = true;
}

RM_FileScan::~RM_FileScan() {
	if (scanEnded == false && hasPagePinned == true && openScan == true) {
		fileHandle->pfh.UnpinPage(scanPage);
	}
	if (initializedValue == true) { // free any memory not freed
		free(value);
		initializedValue = false;
	}
}

Status RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,
	AttrType   attrType,
	int        attrLength,
	int        attrOffset,
	CompOp     compOp,
	void       *value,
	ClientHint pinHint) {
	if (openScan)
		return -1;
	if (fileHandle.isValidFileHeader())
		this->fileHandle = const_cast<RM_FileHandle *>(&fileHandle);

	this->value = value;
	openScan = true;
	scanEnded = false;

	this->compOp = compOp;
	numRecOnPage = 0;
	numSeenOnPage = 0;
	useNextPage = true;
	scanPage = 0;
	scanSlot = -1;
	hasPagePinned = false;
	return 0;
}

Status  RM_FileScan::GetNumRecOnPage(PageHandle &ph, int &num) {
	char *pData;
	if (ph.GetData(pData))
		return -1;
	RM_PageHeader *pageheader = (RM_PageHeader *)pData;
	num = pageheader->numRecords;
	return 0;
}


Status RM_FileScan::GetNextRec(RM_Record &record) {
	if (!openScan)
		return -1;
	if (scanEnded)
		return -1;

	hasPagePinned = true;
	//cout << scanPage << " " << scanSlot << endl;
	while (true) {
		RM_Record temprec;
		if (fileHandle->GetNextRecord(scanPage, scanSlot, temprec, currentPH, useNextPage)) {
			hasPagePinned = false;
			scanEnded = true;
			return -1;
		}
		hasPagePinned = true;

		if (useNextPage) {
			GetNumRecOnPage(currentPH, numRecOnPage);
			useNextPage = false;
			numSeenOnPage = 0;
			if (numRecOnPage == 1)
				currentPH.GetPageNum(scanPage);
		}
		numSeenOnPage++;

		if (numSeenOnPage == numRecOnPage) {
			useNextPage = true;
			if (fileHandle->pfh.UnpinPage(scanPage))
				return -1;

			hasPagePinned = false;
		}

		RID rid;
		temprec.GetRid(rid);
		rid.GetPageNum(scanPage);
		rid.GetSlotNum(scanSlot);
		char *pData;
		if (temprec.GetData(pData))
			return -1;
	//	cout << "12312" << endl;
		if (compOp != NO_OP) {
	//		cout << "1213" << endl;
			bool ok = (*comparator)(pData + attrOffset, this->value, attrType, attrLength);
			if (ok) {
				record = temprec;
				break;
			}
		}
		else {
		//	cout << "231 " << endl;
			record = temprec;
			break;
		}
	}
	return 0;
}

Status RM_FileScan::CloseScan() {
	openScan = false;
	return 0;
}