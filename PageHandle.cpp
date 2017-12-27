#include "File.h"

PageHandle::PageHandle() {
	pagenum = INVAILD_PAGE;
	ptr_Data = NULL;
}

PageHandle::~PageHandle() {

}

PageHandle::PageHandle(const PageHandle& pagehandle) {
	this->pagenum = pagehandle.pagenum;
	this->ptr_Data = pagehandle.ptr_Data;
}

PageHandle&  PageHandle::operator =(const PageHandle &pagehandle) {
	if (this != &pagehandle) {
		this->pagenum = pagehandle.pagenum;
		this->ptr_Data = pagehandle.ptr_Data;
	}
		return *this;
}

Status PageHandle::GetData(char *& pData) const {
	pData = ptr_Data;
	return 0;
}

Status PageHandle::GetPageNum(int &pagenum) const {
	pagenum = this->pagenum;
	return 0;
}
