#include "index.h"
using namespace std;
FileManager fm;
IX_Manager im(fm);
IX_IndexHandle ih;
IX_IndexScan is;
int main(void) {
	/*if (im.CreateIndex("Student", 0, STRING, 10)) {
		return -1;
	}*/
	if (im.OpenIndex("Student", 0, ih))
		return -1;
	//RID rid(1,0);
	//char str[10];
	//memset(str, 0, sizeof(str));
	//memcpy(str, "15281155", 8);
	//if (ih.InsertEntry(str, rid))
	//	return -1;
	/*char str[10];
	memset(str, 0, sizeof(str));
	memcpy(str, "15281155", 8);
	if (ih.DeleteEntry(str, RID(1,0)))
		return -1;*/
	if (is.OpenScan(ih, NO_OP, 0))
		return -1;
	RID rid;
	int pagenum, slot;
	while (true) {
		if (is.GetNextEntry(rid))
			break;
		if (rid.GetPageNum(pagenum) || rid.GetSlotNum(slot))
			return -1;
		cout << pagenum << " " << slot << endl;
	}
	if (im.CloseIndex(ih))
		return -1;
	return 0;
}