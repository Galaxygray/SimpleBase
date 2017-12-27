#include "Record.h"
#include <math.h>

RM_FileHandle::RM_FileHandle() {
	header_modified = false;
	openedFH = false;
}

RM_FileHandle::~RM_FileHandle(){

}

RM_FileHandle& RM_FileHandle::operator= (const RM_FileHandle &fileHandle) {
	if (this != &fileHandle) {
		this->openedFH = fileHandle.openedFH;
		this->header_modified = fileHandle.header_modified;
		this->pfh = fileHandle.pfh;
		memcpy(&this->header, &fileHandle.header, sizeof(struct RM_FileHeader));
	}
	return (*this);
}

Status RM_FileHandle::AllocateNewPage(PageHandle& pagehandle, int& pagenum) {
	if (pfh.AllocatePage(pagehandle))
		return -1;
	if (pagehandle.GetPageNum(pagenum))
		return -1;

	char* bitmap;
	RM_PageHeader* header;
	if (GetPageDataAndBitmap(pagehandle, bitmap, header))
		return -1;
	header->nextFreePage = this->header.firstFreePage;
	header->numRecords = 0;
	if (ResetBitmap(bitmap, this->header.numRecordsPerPage))
		return -1;
	this->header.numPages++;
	this->header.firstFreePage = pagenum;
	header_modified = true;
	return 0;
}

Status RM_FileHandle::GetPageDataAndBitmap(PageHandle& pagehandle, char*& bitmap, RM_PageHeader*& header) const{
	char* ptrData;
	if (pagehandle.GetData(ptrData))
		return -1;
	header = (RM_PageHeader*)ptrData;
	bitmap = ptrData + this->header.bitmapOffset;

	return 0;
}

Status RM_FileHandle::GetPageNumAndSlot(const RID& rid, int& pagenum, int& slot) const{
	if (rid.GetPageNum(pagenum))
		return -1;
	if (rid.GetSlotNum(slot))
		return -1;
	return 0;
}

Status RM_FileHandle::GetRec(const RID& rid, RM_Record& record) const{
	int pagenum, slot;
	if (rid.GetPageNum(pagenum) || rid.GetSlotNum(slot))
		return -1;
	PageHandle ph;
	if (pfh.GetThisPage(pagenum, ph))
		return -1;
	char* bitmap;
	RM_PageHeader* pageheader;
	if (GetPageDataAndBitmap(ph, bitmap, pageheader))
		return -1;
	bool recordexist = false;
	if (CheckBitSet(bitmap, pageheader->numRecords, slot, recordexist))
		return -1;
	if (record.SetRecord(rid, bitmap + slot * this->header.recordSize + header.bitmapSize, this->header.recordSize))
		return -1;
	if (pfh.UnpinPage(pagenum))
		return -1;
	return 0;
}

Status RM_FileHandle::InsertRec(const char* ptrData, RID& rid)
{
	if (ptrData == NULL)
		return -1;
	PageHandle ph;
	int pagenum, slot;
	if (header.firstFreePage == -1)
		AllocateNewPage(ph, pagenum);
	else {
		pfh.GetThisPage(header.firstFreePage, ph);
		pagenum = header.firstFreePage;
	}
	char* bitmap;
	RM_PageHeader* pageheader;
	if (GetPageDataAndBitmap(ph, bitmap, pageheader))
		return -1;
	if (GetFirstZeroBit(bitmap, header.numRecordsPerPage, slot))
		return -1;
	if (SetBit(bitmap, header.numRecordsPerPage,slot))
		return -1;
	memcpy(bitmap + slot*header.recordSize + header.bitmapSize,ptrData,  header.recordSize);
	pageheader->numRecords++;
	rid = RID(pagenum, slot);
	if (pageheader->numRecords == header.numRecordsPerPage)
	{
		header.firstFreePage = pageheader->nextFreePage;
		header_modified = true;
	}
	if (pfh.MarkDirty(pagenum) || pfh.UnpinPage(pagenum))
		return -1;
	return 0;
}

Status RM_FileHandle::DeleteRec(const RID& rid) {
	int pagenum, slot;
	if (rid.GetPageNum(pagenum) || rid.GetSlotNum(slot))
		return -1;
	PageHandle ph;
	if (pfh.GetThisPage(pagenum, ph))
		return -1;
	char* bitmap;
	RM_PageHeader* pageheader;
	if (GetPageDataAndBitmap(ph, bitmap, pageheader))
		return -1;
	bool recordexist = false;
	if (CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordexist))
		return -1;
	if (ResetBit(bitmap, header.numRecordsPerPage, slot))
		return -1;
	pageheader->numRecords--;
	if (pageheader->numRecords == header.numRecordsPerPage - 1)
	{
		pageheader->nextFreePage = header.firstFreePage;
		header.firstFreePage = pagenum;
		header_modified = true;
	}
	if (pfh.MarkDirty(pagenum) || pfh.UnpinPage(pagenum))
		return -1;
	return 0;
}

Status RM_FileHandle::UpdateRec(const RM_Record& record) {
	RID rid;
	if (record.GetRid(rid))
		return -1;
	int pagenum, slot;
	if (GetPageNumAndSlot(rid, pagenum, slot))
		return -1;
	PageHandle ph;
	if (pfh.GetThisPage(pagenum, ph))
		return -1;
	RM_PageHeader* pageheader;
	char *bitmap;
	if (GetPageDataAndBitmap(ph, bitmap, pageheader))
		return -1;
	bool recordexist = false;
	if (CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordexist))
		return -1;
	char* ptrData;
	if (record.GetData(ptrData))
		return -1;
	memcpy(bitmap + slot*header.recordSize + header.bitmapSize, ptrData, header.recordSize);
	if (pfh.MarkDirty(pagenum) || pfh.UnpinPage(pagenum))
		return -1;
	return 0;
}

Status RM_FileHandle::GetNextRecord(int page, int slot, RM_Record &rec, PageHandle &ph, bool nextPage) {
	char *bitmap;
	RM_PageHeader *pageheader;
	int nextRec;
	int nextRecPage = page;
	int nextRecSlot;
	if (nextPage) {
		while (true) {
			if (pfh.GetNextPage(page, ph) == -1)
				return -1;
			//cout << "NextPage" << " " << page << endl;
			if (ph.GetPageNum(page))
				return -1;
		//	cout << "Page" << " " << endl;
			if (GetPageDataAndBitmap(ph, bitmap, pageheader))
				return -1;
			//cout << "Getbitmap" << endl;
			if (GetNextOneBit(bitmap, header.numRecordsPerPage, 0, nextRec) != INVAILD_SLOT)
				break;
			//cout << "GetNextOnebit" << endl;
			if (pfh.UnpinPage(page))
				return -1;
		}
		nextRecPage = page;
	}
	else {
		if (GetPageDataAndBitmap(ph, bitmap, pageheader))
			return -1;
		if (GetNextOneBit(bitmap, header.numRecordsPerPage, slot + 1, nextRec) == INVAILD_SLOT) {
			cout << "Error GoT!" << endl;
			for (int i = 0; i < header.numRecordsPerPage; i++) cout << (int)bitmap[i];
			puts("");
			return -1;
		}
	}
	nextRecSlot = nextRec;
	RID rid(nextRecPage, nextRec);
	//cout << nextRecPage << " " << nextRec << endl;
	if ((rec.SetRecord(rid, bitmap + (header.bitmapSize) + (nextRecSlot)*(header.recordSize),
		header.recordSize)))
		return -1;
//	cout << "okok" << endl;
	return 0;
}


Status RM_FileHandle::ResetBitmap(char* bitmap, int size){
	int sz = NumBitsToCharSize(size);
	for (int i = 0; i < sz; i++)
		bitmap[i] = 0;
	return 0;
}

Status RM_FileHandle::SetBit(char* bitmap, int size, int pos) {
	int sz = pos / 8;
	bitmap[sz] |= 1 << (pos - sz * 8);
	return 0;
}

Status RM_FileHandle::ResetBit(char* bitmap, int size, int pos) {
	int sz = pos / 8;
	bitmap[sz] &= ~(1 << (pos - sz * 8));
	return 0;
}

Status RM_FileHandle::CheckBitSet(char* bitmap, int size, int pos, bool& set) const{
	int sz = pos / 8;
	if ((bitmap[sz] & (1 << (pos - sz * 8))))
		set = true;
	else
		set = false;
	return 0;
}

Status RM_FileHandle::GetFirstZeroBit(char* bitmap, int size, int& slot) {
	int sz = size / 8;
	slot = 0;
	while (slot < sz && !~bitmap[slot]) slot++;
	for(int i = 0; i < 8; i ++)
		if ((bitmap[slot] & (1 << i)) == 0)
		{
			slot = slot * 8 + i;
			return 0;
		}
	return -1;
}

Status RM_FileHandle::GetNextOneBit(char* bitmap, int size, int st, int& slot) {
	int sz = NumBitsToCharSize(size);
	slot = st / 8;
//	cout << "Statrt" << endl;
	while (slot < sz && bitmap[slot] == 0) slot++;
	if (slot >= sz) return -1;
	//cout << "slot: " << slot << endl;
	for (int i = (slot * 8 >= st ? 0 : st % 8); i < 8; i++)
	if ((bitmap[slot] & (1 << i)) != 0)
	{
			slot = slot * 8 + i;
			//cout << slot << endl;
			return 0;
	}
	return -1;
}

int RM_FileHandle::NumBitsToCharSize(int size) {
	return (int)round(size / 8);
}

int RM_FileHandle::CalcNumRecPerPage(int recsize) {
	return (int)(PAGE_SIZE / (recsize + 1. / 8));
}


Status RM_FileHandle::PrintHeaderInfo() const{
//	cout << header.bitmapOffset << endl;
	//cout << header.bitmapSize << endl;
	cout << "Header: " << header.firstFreePage << endl;
	return 0;
}

int RM_FileHandle::getRecordSize() {
	return header.recordSize;
}

bool RM_FileHandle::isValidFileHeader() const {
	if (header.numPages > 0 && header.recordSize > 0 && header.numRecordsPerPage > 0)
		return 1;
	return 0;
}

Status RM_FileHandle::ForcePages(int pagenum) {
	if (pagenum == -1)
	{
		if (pfh.FlushToDisk())
			return -1;
	}
	else
		if(pfh.ForcePage(pagenum))
		return -1;
	return 0;
}