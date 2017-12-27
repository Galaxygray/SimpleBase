#include "index.h"
#include <string>
#include <sstream>

int compare_string(void *value1, void* value2, int attrLength) {
	return strncmp((char *)value1, (char *)value2, attrLength);
}

int compare_int(void *value1, void* value2, int attrLength) {
	if ((*(int *)value1 < *(int *)value2))
		return -1;
	else if ((*(int *)value1 > *(int *)value2))
		return 1;
	else
		return 0;
}

int compare_float(void *value1, void* value2, int attrLength) {
	if ((*(float *)value1 < *(float *)value2))
		return -1;
	else if ((*(float *)value1 > *(float *)value2))
		return 1;
	else
		return 0;
}

IX_Manager::IX_Manager(FileManager& pfm):pfm(pfm) {
	
}

IX_Manager::~IX_Manager() {

}

Status IX_Manager::CreateIndex(const char *fileName,          // Create new index
	int        indexNo,
	AttrType   attrType,
	int        attrLength) {
	std::string indexname = std::string(fileName);
	std::stringstream convert;
	convert << indexNo;
	indexname += '.' + convert.str();
	if (pfm.CreateFile(indexname.c_str()))
		return -1;
	FileHandle fh;
	if (pfm.OpenFile(indexname.c_str(), fh))
		return -1;
	PageHandle ph;
	PageHandle root;
	int headerpage = 0;
	int rootpage = 0;
	int numkeys = IX_IndexHandle::CalcNumKeysNode(attrLength);
	if (fh.AllocatePage(ph) || ph.GetPageNum(headerpage) || fh.AllocatePage(root) || root.GetPageNum(rootpage))
		return -1;
	IX_IndexHeader *header;
	IX_NodeHeader *rootheader;
	Node_Entry* entries;
	if (ph.GetData((char *&)header) || root.GetData((char *&)rootheader))
		return -1;
	header->attr_length = attrLength;
	header->attr_type = attrType;
	header->maxKeys_N = numkeys;
	header->entryOffset_N = sizeof(IX_NodeHeader);
	header->keysOffset_N = header->entryOffset_N + numkeys * sizeof(Node_Entry);
	header->rootPage = rootpage;
	
	rootheader->isEmpty = true;
	rootheader->isLeafNode = true;
	rootheader->num_keys = 0;
	rootheader->pre = rootheader->next = INVAILD_PAGE;
	rootheader->firstSlotIndex = INVAILD_SLOT;
	rootheader->freeSlotIndex = 0;

	entries = (Node_Entry *)((char *)rootheader + header->entryOffset_N);
	for (int i = 0; i < header->maxKeys_N; i++) {
		entries[i].isValid = -1;
		entries[i].page = -1;
		entries[i].nextSlot = i + 1;
	}
	entries[header->maxKeys_N - 1].nextSlot = -1;
	if (fh.MarkDirty(headerpage) || fh.MarkDirty(rootpage) || fh.UnpinPage(headerpage) || fh.UnpinPage(rootpage))
		return -1;

	if (pfm.CloseFile(fh))
		return -1;

	return 0;
}

Status IX_Manager::DestroyIndex(const char *filename, int indexNo) {
	std::string indexname = std::string(filename);
	std::stringstream convert;
	convert << indexNo;
	indexname += '.' + convert.str();
	if (pfm.DestroyFile(indexname.c_str()))
		return -1;
	return 0;
}

Status IX_Manager::SetUpIH(IX_IndexHandle &ih, FileHandle &fh, IX_IndexHeader *header) {
	memcpy(&ih.header, header, sizeof(IX_IndexHeader));
	if (fh.GetThisPage(header->rootPage, ih.rootPH))
		return -1;
	if (ih.header.attr_type == INT) {
		ih.comparator = &compare_int;

	}
	else if (ih.header.attr_type == FLOAT) {
		ih.comparator = &compare_float;
	}
	else {
		ih.comparator = &compare_string;
	}

	ih.header_modified = false;
	ih.pfh = fh;
	ih.isOpenHandle = true;
	return 0;
}

Status IX_Manager::OpenIndex(const char* fileName, int indexNo, IX_IndexHandle& indexhandle) {
	std::string indexname = std::string(fileName);
	std::stringstream convert;
	convert << indexNo;
	indexname += '.' + convert.str();
	FileHandle fh;
	if (pfm.OpenFile(indexname.c_str(), fh))
		return -1;
	PageHandle ph;
	int pagenum;
	char* ptrData;
	if (fh.GetFirstPage(ph) || ph.GetData(ptrData) || ph.GetPageNum(pagenum))
		return -1;
	IX_IndexHeader* fileHeader = (IX_IndexHeader *) ptrData;
	SetUpIH(indexhandle, fh, fileHeader);
	if (fh.UnpinPage(pagenum))
		return -1;

	return 0;
}

Status IX_Manager::CloseIndex(IX_IndexHandle &indexhandle) {
	PageHandle ph;
	int pagenum;
	char *pData;

	int root = indexhandle.header.rootPage;
	if(indexhandle.header_modified){
		if(indexhandle.pfh.GetFirstPage(ph) || ph.GetPageNum(pagenum) || ph.GetData(pData))
			return -1;
		memcpy(pData, &indexhandle.header, sizeof(IX_IndexHeader));
		if(indexhandle.pfh.MarkDirty(pagenum) || indexhandle.pfh.UnpinPage(pagenum))
			return -1;	
	}
	if(pfm.CloseFile(indexhandle.pfh))
		return -1;
	return 0;
}

