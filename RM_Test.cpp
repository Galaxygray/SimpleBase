//#include "Record.h"
//#include <iostream>
//FileManager control;
//FileHandle fh;
//RM_Manager rm(control);
//RM_FileHandle rmf;
//RM_FileScan FS;
//using namespace std;
//string str;
//int main(void) {
//	/*if (rm.CreateFile("TestRM.txt", 12))
//		return -1;*/
//
//	if (rm.OpenFile("TestRM.txt", rmf))
//		return -1;
//	/*rmf.PrintHeaderInfo();
//	str = "123456789012123123";
//	int pa, sl;
//	for (int i = 0; i < 10; i++) {
//		RID rid;
//		if (rmf.InsertRec(str.c_str(), rid))
//			return -1;
//	
//		rid.GetPageNum(pa);
//		rid.GetSlotNum(sl);
//		cout << pa << " " << sl << endl;
//	}*/
//	/*rmf.DeleteRec(RID(2,1));
//	RID rid;
//	rmf.InsertRec(str.c_str(), rid);
//	rid.GetPageNum(pa);
//	rid.GetSlotNum(sl);
//	cout << pa << " " << sl << endl;*/
//	/*RM_Record record;
//	record.SetRecord(RID(1, 2), "999999999999", 12);
//	rmf.UpdateRec(record);*/
//	rmf.PrintHeaderInfo();
//	if (FS.OpenScan(rmf, STRING, 12, 0, NO_OP, 0))
//		return -1;
//	int ok;
//	RM_Record record;
//	RID rid;
//	int x, y;
//	while (cin >> ok && ok) {
//		if (FS.GetNextRec(record)) {
//			cout << "EOF" << endl;
//			break;
//		}
//		char *pData;
//		if (record.GetData(pData))
//		{
//			cout << "GetDataError" << endl;
//			break;
//		}
//		for (int i = 0; i < 12; i++) putchar(*(pData + i));
//		puts("");
//		record.GetRid(rid);
//		rid.GetPageNum(x);
//		rid.GetSlotNum(y);
//		cout << x << " " << y << endl;
//	} 
//
//	if (rm.CloseFile(rmf))
//		return -1;
//	return 0;
//}
//
