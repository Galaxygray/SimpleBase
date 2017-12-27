//#include "SystemManager.h"
//FileManager control;
//FileHandle fh;
//RM_Manager rm(control);
//RM_FileHandle rmf;
//RM_FileScan FS;
//SM_Manager sm(rm);
//
//int main(void) {
//	//if (sm.CreateDb("Test"))
//	//	return -1;
//	if (sm.OpenDb("Test"))
//		return -1;
//	/*char tablename[] = "Student";
//	AttrInfo *stduentattrs = new AttrInfo[4];
//	stduentattrs[0].attrName = "Id";
//	stduentattrs[0].attrLength = 10;
//	stduentattrs[0].attrType = STRING;
//	stduentattrs[1].attrName = "Name";
//	stduentattrs[1].attrLength = 6;
//	stduentattrs[1].attrType = STRING;
//	stduentattrs[2].attrName = "Class";
//	stduentattrs[2].attrLength = 10;
//	stduentattrs[2].attrType = STRING;
//	stduentattrs[3].attrName = "Grade";
//	stduentattrs[3].attrLength = 4;
//	stduentattrs[3].attrType = INT;
//	if (sm.CreateTable(tablename, 4, stduentattrs))
//		return -1;*/
//	/*int nvalues = 4;
//	Value *values = new Value[4];
//	values[0].type = STRING;
//	values[0].data = "15281100";
//	values[1].type = STRING;
//	values[1].data = "ÇØÓð";
//	values[2].type = STRING;
//	values[2].data = "1500";
//	values[3].type = INT;
//	int v = 0;
//	values[3].data = &v;
//	if (sm.Insert("Student", nvalues, values))
//		return -1;*/
//	//char tablename[] = "Student";
//	//Value v;
//	//int x = 100;
//	//v.type = INT;
//	//v.data = &x;
//	//Condition condition;
//	//condition.lhsAttr.attrName = "Id";
//	//condition.lhsAttr.relName = "Student";
//	//condition.rhsValue.data = "15281155";
//	//condition.rhsValue.type = STRING;
//	//RelAttr relattr;
//	//relattr.attrName = "Grade";
//	//relattr.relName = "Student";
//	///*if (sm.Update(tablename, 1, &condition, relattr, v))
//	//	return -1;*/
//	//if (sm.Delete(tablename, 1, &condition))
//	//	return -1;
//	//if (sm.Print(tablename))
//	//	return -1;
//	//if (sm.DropTable("Student"));
//	/*Condition *conditons = new Condition[1];
//	conditons[0].lhsAttr.attrName = "Id";
//	conditons[0].lhsAttr.relName = "books";
//	conditons[0].op = EQ_OP;
//	int v = -137954629;
//	conditons[0].rhsValue.data = &v;
//	conditons[0].rhsValue.type = INT;
//	if (sm.Delete("books", 1, conditons))
//		return -1;*/
//	//sm.PrintAll();
//		if (sm.Print("books"))
//			return -1;
//	if (sm.CloseDb())
//		return -1;
//
//
//}