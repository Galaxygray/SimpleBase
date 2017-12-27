#include "SystemManager.h"

Printer::Printer(int nattrs, AttrCatEntry *attrEntry) {
	for (int i = 0; i < nattrs; i++)
		Space.push_back(attrEntry[i].attrLength + 10);
}



Printer::~Printer() {
}

Status Printer::PrintData(AttrCatEntry *attrEntry, void *pData) {
	char buffer[200];
	int totsize = 0;
	for (int i = 0; i < Space.size(); i++) {
		int size = 0;
		if (attrEntry[i].attrType == INT) {
			size += snprintf(buffer + totsize, 200 - totsize, "%d", *((int *)((char *)pData + attrEntry[i].offset)));
			for (int j = size; j < Space[i]; j++) buffer[totsize + j] = ' ';
		}
		else if(attrEntry[i].attrType == FLOAT){
			size += snprintf(buffer + totsize, 200 - totsize, "%.4f", *((float *)((char *)pData + attrEntry[i].offset)));
			for (int j = size; j < Space[i]; j++) buffer[totsize + j] = ' ';
		}
		else {
			for(int j = 0; j < attrEntry[i].attrLength; j ++) size += snprintf(buffer + totsize + j, 200 - totsize, "%c", *((char *)pData + attrEntry[i].offset + j));
			for (int j = size; j < Space[i]; j++) buffer[totsize + j] = ' ';
		}
		totsize += Space[i];
	}
	for (int i = 0; i < totsize; i++) if (!buffer[i]) buffer[i] = ' ';
	buffer[totsize] = 0;
	puts(buffer);
	return 0;
}

Status Printer::PrintHeader(AttrCatEntry *attrEntry) {
	char buffer[200];
	int totsize = 0;
	for (int i = 0; i < Space.size(); i++) {
		int size = 0;
		size += snprintf(buffer + totsize, 200 - totsize, "%s", attrEntry[i].attrName);
		for (int j = size; j < Space[i]; j++) buffer[totsize + j] = ' ';
		totsize += Space[i];
	}
	buffer[totsize] = 0;
	puts(buffer);
	for (int i = 0; i < totsize; i++) putchar('-');
	puts("");
	return 0;
}