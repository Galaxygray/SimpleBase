#include "Record.h"

RM_Manager::RM_Manager(FileManager &pfm): pfm(pfm) {

}	

RM_Manager::~RM_Manager() {

}

Status RM_Manager::CreateFile(const char* filename, int recordSize) {
	if (filename == NULL)
		return -1;
	if (recordSize < 0 || recordSize > PAGE_SIZE)
		return -1;
	int numperpage = RM_FileHandle::CalcNumRecPerPage(recordSize);
	int bitmapSize = RM_FileHandle::NumBitsToCharSize(numperpage);
	int bitoffset = sizeof(struct RM_PageHeader);
	if ((PAGE_SIZE - bitmapSize - bitoffset) / recordSize <= 0)
		return -1;
	if (pfm.CreateFile(filename))
		return -1;
	PageHandle ph;
	FileHandle fh;
	if (pfm.OpenFile(filename, fh))
		return -1;
	int pagenum;
	if (fh.AllocatePage(ph) || ph.GetPageNum(pagenum))
		return -1;
	char* ptrData;
	if (ph.GetData(ptrData))
		return -1;
	RM_FileHeader* header = (RM_FileHeader*)ptrData;
	header->numRecordsPerPage = (PAGE_SIZE - bitmapSize - bitoffset) / recordSize;
	header->bitmapOffset = bitoffset;
	header->recordSize = recordSize;
	header->numPages = 1;
	header->firstFreePage = -1;
	header->bitmapSize = bitmapSize;
	if (fh.MarkDirty(pagenum) || fh.UnpinPage(pagenum) || pfm.CloseFile( fh))
		return - 1;
	return 0;
}

Status RM_Manager::DestroyFile(const char* filename) {
	if (filename == NULL)
		return -1;

	if (pfm.DestroyFile(filename))
		return -1;
	return 0;
}

Status RM_Manager::SetUpFH(RM_FileHandle& filehandle, FileHandle& fh, RM_FileHeader* header) {
	memcpy(&filehandle.header, header, sizeof(RM_FileHeader));
	filehandle.pfh = fh;
	filehandle.header_modified = false;
	filehandle.openedFH = true;
	return 0;
}

Status RM_Manager::OpenFile(const char* filename, RM_FileHandle& filehandle) {
	if (filename == NULL)
		return -1;
	if (filehandle.openedFH)
		return -1;
	FileHandle fh;
	if (pfm.OpenFile(filename, fh))
		return -1;
	PageHandle ph;
	int pagenum;
	if (fh.GetFirstPage(ph) || ph.GetPageNum(pagenum))
		return -1;
	char *ptrData;
	ph.GetData(ptrData);
	RM_FileHeader* header = (RM_FileHeader *)ptrData;
	SetUpFH(filehandle, fh, header);
	if (fh.UnpinPage(pagenum))
		return -1;
	return 0;
}

Status RM_Manager::CleanUpFH(RM_FileHandle& filehandle) {
	if (filehandle.openedFH == false)
		return -1;
	filehandle.openedFH = false;
	return 0;
}

Status RM_Manager::CloseFile(RM_FileHandle& filehandle) {
	PageHandle ph;
	int pagenum;
	char *pData;

	if (filehandle.header_modified) {
		if (filehandle.pfh.GetFirstPage(ph) || ph.GetPageNum(pagenum))
			return -1;
		if (ph.GetData(pData)) {
			if (filehandle.pfh.UnpinPage(pagenum))
				return -1;
			return -1;
		}
		memcpy(pData, &filehandle.header, sizeof(RM_FileHeader));
		if (filehandle.pfh.MarkDirty(pagenum) || filehandle.pfh.UnpinPage(pagenum))
			return -1;
	}

	if (pfm.CloseFile(filehandle.pfh))
		return -1;
	if (CleanUpFH(filehandle))
		return -1;
	return 0;
}


