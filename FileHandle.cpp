#include "File.h"

FileHandle::FileHandle(){
    Isopen = false;
	bfm = NULL;
}

FileHandle::~FileHandle(){

}

FileHandle::FileHandle(const FileHandle &fileHandle)
{
	// Just copy the data members since there is no memory allocation involved
	this->bfm = fileHandle.bfm;
	this->fileheader = fileHandle.fileheader;
	this->Isopen = fileHandle.Isopen;
	this->bHdrChanged = fileHandle.bHdrChanged;
	this->fd = fileHandle.fd;
}

FileHandle& FileHandle::operator= (const FileHandle &fileHandle)
{
	// Test for self-assignment
	if (this != &fileHandle) {

		// Just copy the members since there is no memory allocation involved
		this->bfm = fileHandle.bfm;
		this->fileheader = fileHandle.fileheader;
		this->Isopen = fileHandle.Isopen;
		this->bHdrChanged = fileHandle.bHdrChanged;
		this->fd = fileHandle.fd;
	}

	// Return a reference to this
	return (*this);
}

Status FileHandle::GetNextPage(int curpagenum, PageHandle &pageHandle) const {
	if (!Isopen)
		return -1;
	/*if (curpagenum != -1 || !IsValidPageNum(curpagenum))
		return -1;*/
	//cout << "Curpagenum: " << curpagenum << endl;
	for (curpagenum ++; curpagenum < fileheader.numpages; curpagenum++) {
		if (!GetThisPage(curpagenum, pageHandle))
			return 0;
	}
	return -1;
}

Status FileHandle::GetPrePage(int curpagenum, PageHandle &ph) const {
	return 0;
}


Status FileHandle::GetFirstPage(PageHandle &pagehandle) const{
	return GetNextPage(-1, pagehandle);
}


Status FileHandle::GetThisPage(int curpagenum, PageHandle& pagehandle) const {
	if (!Isopen)
		return -1;
	if (curpagenum < 0 || curpagenum >= fileheader.numpages) {
	//	cout << "GEtThisPage Erro" << endl;
		return -1;
	}
	char *pBuf;
	//cout << "GetPage " <<curpagenum << " " << fileheader.numpages <<  endl;
	if (bfm->GetPage(fd, curpagenum, &pBuf))
		return -1;
	//cout << "GetPage end" << endl;
	if (((PageHeader *)pBuf)->nextfree == PAGE_USED)
	{
		pagehandle.pagenum = curpagenum;
		pagehandle.ptr_Data = pBuf + sizeof(PageHeader);
	//	cout << "Success" << endl;
		return 0;
	}
	//cout << "Failed" << endl;
	if (UnpinPage(curpagenum))
		return -1;
	return -1;
}

Status FileHandle::FlushToDisk() const{
	if (bHdrChanged) {
		int numbytes = 0;
		if (_lseek(fd, 0, 0) < 0)
			return -1;
		if ((numbytes = _write(fd, &fileheader, sizeof(FileHeader))) != sizeof(FileHeader)) {
			return -1;
		}
	}
	if (bfm->FlushPages(fd))
		return -1;
	return 0;
}

Status FileHandle::ForcePage(int pagenum)const {
	if (bfm->FlushToDisk(fd, pagenum))
		return -1;
	return 0;
}

Status FileHandle::UnpinPage(int pagenum) const {
	if (bfm->UnpinPage(fd, pagenum))
		return -1;
	return 0;
}

Status FileHandle::IsValidPageNum(int pagenum) const{
	return Isopen && pagenum >= 0 && pagenum < fileheader.numpages;
}

Status FileHandle::AllocatePage(PageHandle &pagehandle) {
	if (!Isopen)
		return -1;
	int pagenum;
	char *pData;
	//cout << "Ok!" << endl;
	if (fileheader.firstfree != -1) {
		pagenum = fileheader.firstfree;
		if (bfm->GetPage(fd, pagenum, &pData))
			return -1;
		fileheader.firstfree = ((PageHeader *)pData)->nextfree;
	}
	else {
	//	cout << "Ok com here!" << endl;
		pagenum = fileheader.numpages;
		if (bfm->AllocatePage(fd, pagenum, &pData))
			return -1;
		fileheader.numpages++;
		
	}
	bHdrChanged = true;
	//cout << "ok end" << endl;
	PageHeader *pageheader = (PageHeader *)pData;
	pageheader->nextfree = PAGE_USED;
	memset(pData + sizeof(PageHeader), 0, PAGE_SIZE);

	if (bfm->MarkDirty(fd, pagenum))
		return -1;

	pagehandle.pagenum = pagenum;
	pagehandle.ptr_Data = pData + sizeof(PageHeader);
	return 0;
}

Status FileHandle::MarkDirty(int pagenum) const {
	if (bfm->MarkDirty(fd, pagenum))
		return -1;
	return 0;
}

Status FileHandle::DisposePage(int pagenum) {
	if (!Isopen)
		return -1;
	char *pbuffer;
	if (bfm->GetPage(fd, pagenum, &pbuffer))
		return -1;
	if (((PageHeader *)pbuffer)->nextfree != PAGE_USED)
	{
		if (UnpinPage(pagenum))
			return -1;
		return -1;
	}

	((PageHeader *)pbuffer)->nextfree = fileheader.firstfree;
	fileheader.firstfree = pagenum;
	bHdrChanged = true;

	if (MarkDirty(pagenum) || UnpinPage(pagenum))
		return -1;
	return 0;
}