#include "Record.h"

RID::RID() {
	page = slot = -1;
}

RID::RID(int pagenum, int slot) {
	this->page = pagenum;
	this->slot = slot;
}

RID::~RID(){

}

RID& RID::operator= (const RID &rid) {
	if (this != &rid) {
		this->page = rid.page;
		this->slot = rid.slot;
	}
	return *this;
}

bool RID::operator== (const RID &rid) const{
	return page == rid.page && slot == rid.slot;
}

Status RID::GetPageNum(int &pagenum) const {
	pagenum = this->page;
	return 0;
}

Status RID::GetSlotNum(int &slot) const {
	slot = this->slot;
	return 0;
}
