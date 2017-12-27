#include "SystemManager.h"

SM_Manager::SM_Manager(RM_Manager &rm) : rmm(rm){
	printIndex = false;
	useQO = true;
	calcStats = false;
	printPageStats = true;
}

SM_Manager::~SM_Manager(){
	
}

int SM_Manager::compareEQ(void *value1, void *value2, AttrType type) {
	switch (type) {
	case INT: {
		if (*((int *)value1) == *((int *)value2)) return 0;
		else return 1;
		break;
	}
	case FLOAT: {
		if (*((float *)value1) == *((float *)value2)) return 0;
		else return 1;
		break;
	}
	case STRING: {
		return strcmp((char *)value1, (char *)value2);
		break;
	}
	}
}

int SM_Manager::compareLE(void *value1, void *value2, AttrType type) {
	switch (type) {
	case INT: {
		if (*((int *)value1) <= *((int *)value2)) return 0;
		else return 1;
		break;
	}
	case FLOAT: {
		if (*((float *)value1) <= *((float *)value2)) return 0;
		else return 1;
		break;
	}
	case STRING: {
		return strcmp((char *)value1, (char *)value2) > 0;
		break;
	}
	}
}

int SM_Manager::compareGE(void *value1, void *value2, AttrType type) {
	switch (type) {
	case INT: {
		if (*((int *)value1) >= *((int *)value2)) return 0;
		else return 1;
		break;
	}
	case FLOAT: {
		if (*((float *)value1) >= *((float *)value2)) return 0;
		else return 1;
		break;
	}
	case STRING: {
		return strcmp((char *)value1, (char *)value2) < 0;
		break;
	}
	}
}

int SM_Manager::compareLT(void *value1, void *value2, AttrType type) {
	switch (type) {
	case INT: {
		if (*((int *)value1) < *((int *)value2)) return 0;
		else return 1;
		break;
	}
	case FLOAT: {
	//	cout << *((float *)value1) << " " << *((float *)value2) << endl;
		if (*((float *)value1) < *((float *)value2)) return 0;
		else return 1;
		break;
	}
	case STRING: {
		return strcmp((char *)value1, (char *)value2) <= 0;
		break;
	}
	}
}

int SM_Manager::compareGT(void *value1, void *value2, AttrType type) {
	switch (type) {
	case INT: {
		if (*((int *)value1) > *((int *)value2)) return 0;
		else return 1;
		break;
	}
	case FLOAT: {
		if (*((float *)value1) > *((float *)value2)) return 0;
		else return 1;
		break;
	}
	case STRING: {
		return strcmp((char *)value1, (char *)value2) <= 0;
		break;
	}
	}
}



Status SM_Manager::CreateDb(const char *name) {
	if (_mkdir(name) < 0) {
		cout << "Cannot mkdir!\n" << endl;
		return -1;
	}
	if (_chmod(name, _S_IREAD | _S_IWRITE) < 0)
		return -1;
	if (_chdir(name) < 0)
		return -1;
	if (rmm.CreateFile("Reltions", sizeof(RelCatEntry)))
		return -1;
	if (rmm.CreateFile("Attrs", sizeof(AttrCatEntry)))
		return -1;
	_chdir("..");
	return 0;
}

Status SM_Manager::DestroyDb(const char *name) {
	if (_chdir(name) < 0)
		return -1;
	system("del *.*");
	system((string("rd -s -q ") + name).c_str());
	return 0;
}

Status SM_Manager::InitRelVec() {
	RM_FileScan fs;
	if (fs.OpenScan(relcatFH, STRING, 0, 0, NO_OP, 0))
		return -1;
	RM_Record record;
	RelVec.clear();
	RelCatEntry *reltion;
	while (true) {
		if (fs.GetNextRec(record))
			break;
		if (record.GetData((char *&)reltion))
			return -1;
		RelVec.push_back(*reltion);
	}
	return 0;
}

Status SM_Manager::InitAttrVec() {
	RM_FileScan fs;
	if (fs.OpenScan(attrcatFH, STRING, 0, 0, NO_OP, 0))
		return -1;
	RM_Record record;
	AttrVec.clear();
	AttrCatEntry *attr;
	while (true) {
		if (fs.GetNextRec(record))
			break;
		if (record.GetData((char *&)attr))
			return -1;
		AttrVec.push_back(*attr);
	}
	return 0;
}

Status SM_Manager::OpenDb(const char *name) {
	if (_chdir(name) < 0) {
		cout << "Doesn't exist this directory" << endl;
		return -1;
	}
	if (rmm.OpenFile("Reltions", relcatFH))
		return -1;
	if (rmm.OpenFile("Attrs", attrcatFH))
		return -1;
	cout << "ok!" << endl;
	InitRelVec();
	InitAttrVec();
	return 0;
}

Status SM_Manager::CloseDb() {
	if (rmm.CloseFile(relcatFH))
		return -1;
	if (rmm.CloseFile(attrcatFH))
		return -1;
	AttrVec.clear();
	RelVec.clear();
	return 0;
}



//Status SM_Manager::PrintAll() {
//	for (auto v : RelVec) cout << v.relName << endl;
//	for (auto v : AttrVec) cout << v.attrName << endl;
//	return 0;
//}

Status SM_Manager::GetAttrsEntry(const char *name, AttrCatEntry *attrEntry, int &nattrs) {
	nattrs = 0;
	for (int i = 0; i < AttrVec.size(); i++) {
		if (strcmp(AttrVec[i].relName, name) == 0)
		{
			attrEntry[AttrVec[i].attrNum] = AttrVec[i];
			nattrs++;
		}
	}
	return 0;
}

Status SM_Manager::Print(const char *name) {
	int reltionPos = -1;
	for (int i = 0; i < RelVec.size(); i++) {
		if (strcmp(RelVec[i].relName, name) == 0) {
			reltionPos = i;
			break;
		}
	}
	if (reltionPos == -1) return -1;

	int nattrs = RelVec[reltionPos].attrCount;
	AttrCatEntry *attrEntry = new AttrCatEntry[nattrs];

	GetAttrsEntry(name, attrEntry, nattrs);
	Printer print(RelVec[reltionPos].attrCount, attrEntry);
	print.PrintHeader(attrEntry);
	RM_FileHandle rfh;
	if (rmm.OpenFile(name,rfh))
		return -1;
	RM_FileScan fs;
	if (fs.OpenScan(rfh, STRING, 0, 0, NO_OP, 0))
		return -1;
	RM_Record record;
	char *pData;
	while(true){
		if (fs.GetNextRec(record))
			break;
		record.GetData(pData);
		print.PrintData(attrEntry, pData);
	}

	delete attrEntry;
	return 0;
}

Status SM_Manager::GetAttrsFromCondtion(const char *name, AttrCatEntry *attrs, Condition *conditions, int nCondtions) {
	for (int i = 0; i < AttrVec.size(); i++) {
		if (strcmp(AttrVec[i].relName, name)) continue;
		for (int j = 0; j < nCondtions; j++) {
			if (strcmp(AttrVec[i].attrName, conditions[j].lhsAttr.attrName) == 0) {
				attrs[j] = AttrVec[i];
				break;
			}
		}
	}
	return 0;
}

Status SM_Manager::GetAttrFromRel(const char *name, AttrCatEntry &attr, RelAttr &relattr) {
	for (int i = 0; i < AttrVec.size(); i++) {
		if (strcmp(name, relattr.relName) == 0 && strcmp(AttrVec[i].attrName, relattr.attrName) == 0)
		{
			attr = AttrVec[i];
			break;
		}
	}
	return 0;
}

Status SM_Manager::GetAttrsFromRel( AttrCatEntry *attr,int nRelattrs,  RelAttr *relattr) {
	for (int i = 0; i < AttrVec.size(); i++) {
		for(int j = 0; j < nRelattrs; j ++)
		if (strcmp(AttrVec[i].relName, relattr[j].relName) == 0 && strcmp(AttrVec[i].attrName, relattr[j].attrName) == 0)
		{
		//	cout << AttrVec[i].attrType << " " << AttrVec[i].attrName << endl;
			attr[j] = AttrVec[i];
		//	cout << attr[j].attrType << " " << attr[j].attrName << endl;
			break;
		}
	}
}

Status SM_Manager::Update(const char *name, int nConditions, Condition *condtions, RelAttr &relattr, Value &value) {
	int reltionPos = -1;
	for (int i = 0; i < RelVec.size(); i++) {
		if (strcmp(RelVec[i].relName, name) == 0) {
			reltionPos = i;
			break;
		}
	}
	if (reltionPos == -1) return -1;

	int nattrs = RelVec[reltionPos].attrCount;
	AttrCatEntry *attrEntry = new AttrCatEntry[nConditions + 1];
	//memset(attrEntry, 0, (nConditions + 1) * sizeof(AttrCatEntry));
	GetAttrsFromCondtion(name, attrEntry, condtions, nConditions);
	GetAttrFromRel(name, *(attrEntry + nConditions), relattr);

	RM_FileHandle rfh;
	if (rmm.OpenFile(name, rfh))
		return -1;
	RM_FileScan fs;
	if (fs.OpenScan(rfh, STRING, 0, 0, NO_OP, 0))
		return -1;

	RM_Record record;
	RID rid;
	char *pData;
	while (true) {
		if (fs.GetNextRec(record))
			break;
		record.GetData(pData);
		bool flag = true;
		for (int i = 0; i < nConditions; i++) {
			if (Compare(pData + attrEntry[i].offset, condtions[i].rhsValue.data, attrEntry[i].attrType, condtions[i].op))
			{
				flag = false;
				break;
			}
		}

		if (flag) {
			cout << attrEntry[nConditions].offset << endl;
			if (attrEntry[nConditions].attrType == INT || attrEntry[nConditions].attrType == FLOAT) {
				memcpy(pData + (attrEntry[nConditions].offset), value.data, 4);
				//cout << *(int *)values[i].data << endl;
			}
			else {
				memset(pData + attrEntry[nConditions].offset, 0, attrEntry[nConditions].attrLength);
				strcpy(pData + (attrEntry[nConditions].offset), (char *)value.data);
				//cout << "123" << endl;
			}
			if (rfh.UpdateRec(record))
				return -1;
		}
	}
	if (rmm.CloseFile(rfh))
		return -1;

	delete attrEntry;
	return 0;
}

Status SM_Manager::Delete(const char *name, int nConditions, Condition *condtions) {
	int reltionPos = -1;
	for (int i = 0; i < RelVec.size(); i++) {
		if (strcmp(RelVec[i].relName, name) == 0) {
			reltionPos = i;
			break;
		}
	}
	if (reltionPos == -1) return -1;

	int nattrs = RelVec[reltionPos].attrCount;
	AttrCatEntry *attrEntry = new AttrCatEntry[nConditions];
	GetAttrsFromCondtion(name, attrEntry, condtions, nConditions);

	RM_FileHandle rfh;
	if (rmm.OpenFile(name, rfh))
		return -1;
	RM_FileScan fs;
	if (fs.OpenScan(rfh, STRING, 0, 0, NO_OP, 0))
		return -1;

	RM_Record record;
	RID rid;
	char *pData;
	while (true) {
		if (fs.GetNextRec(record))
			break;
		record.GetData(pData);
		bool flag = true;
		for (int i = 0; i < nConditions; i++) {
		//	cout << attrEntry[i].offset << " " << condtions->rhsValue.data << " " << attrEntry->attrType << endl;
			if (Compare(pData + attrEntry[i].offset, condtions[i].rhsValue.data, attrEntry[i].attrType, condtions[i].op))
			{
				flag = false;
				break;
			}
		}
		record.GetRid(rid);
	//	if (flag) cout << "oko" << endl;
		if (flag) if (rfh.DeleteRec(rid))
			return -1;
	}
	if (rmm.CloseFile(rfh))
		return -1;

	delete attrEntry;
	return 0;
}


Status SM_Manager::Insert(const char *name, int nValues, Value *values) {
	int reltionPos = -1;
	for (int i = 0; i < RelVec.size(); i++) {
		if (strcmp(RelVec[i].relName, name) == 0 && RelVec[i].attrCount == nValues) {
			reltionPos = i;
			break;
		}
	}
	if (reltionPos == -1) return -1;

	int nattrs = RelVec[reltionPos].attrCount;
	AttrCatEntry *attrEntry = new AttrCatEntry[nattrs];
	
	GetAttrsEntry(name, attrEntry, nattrs);

	RM_FileHandle rfh;
	if (rmm.OpenFile(name, rfh))
		return -1;
	char *pData = new char[RelVec[reltionPos].tupleLength];
	memset(pData, 0, RelVec[reltionPos].tupleLength);
	for (int i = 0; i < nValues; i++) {
		if (attrEntry[i].attrType == INT || attrEntry[i].attrType == FLOAT) {
			memcpy(pData + (attrEntry[i].offset), values[i].data, 4);
		//	cout << *(int *)values[i].data << endl;
		}
		else {
			memcpy(pData + (attrEntry[i].offset), values[i].data, strlen((char *)values[i].data));
	//		cout << "123" << endl;
		}
	}
	RID rid;
	if (rfh.InsertRec(pData, rid))
		return -1;
	if (rmm.CloseFile(rfh))
		return -1;
	delete attrEntry;
	delete pData;
	return 0;
}


Status SM_Manager::CreateTable(const char *name, int attrCount, AttrInfo *attributes) {
	
	int totRecordSize = 0;
	AttrCatEntry attr;
	memset(&attr, 0, sizeof(attr));
	for (int i = 0; i < attrCount; i++) {
		memset(&attr, 0, sizeof(AttrCatEntry));
		attr.attrLength = attributes[i].attrLength;
		memcpy(attr.attrName, attributes[i].attrName, strlen(attributes[i].attrName));
		memcpy(attr.relName, name, strlen(name));
		attr.attrNum = attrCount;
		attr.attrNum = i;
		attr.offset = totRecordSize;
		attr.attrType = attributes[i].attrType;
		RID rid;
		AttrVec.push_back(attr);
		attrcatFH.InsertRec((char *)&attr, rid);
		totRecordSize += attributes[i].attrLength;
	}

	RelCatEntry reltion;
	memset(&reltion, 0, sizeof(reltion));
	reltion.attrCount = attrCount;
	reltion.numTuples = 0;
	memcpy(reltion.relName, name, strlen(name));
	reltion.tupleLength = totRecordSize;
	RID rid;
	RelVec.push_back(reltion);
	if (relcatFH.InsertRec((char *)&reltion, rid))
		return -1;
	if (relcatFH.ForcePages() || attrcatFH.ForcePages())
		return -1;
	if (rmm.CreateFile(name, totRecordSize))
		return -1;
	return 0;
}

Status SM_Manager::DropTable(const char *name) {
	RM_FileScan fs;
	if (fs.OpenScan(attrcatFH, INT, maxname, 0, NO_OP, 0))
		return -1;
	RM_Record rec;
	RID rid;
	AttrCatEntry *pattr;
	while (true) {
		if (fs.GetNextRec(rec))
			break;
		if (rec.GetData((char *&)pattr))
			return -1;
		if (strcmp(pattr->relName, name) == 0) {
			rec.GetRid(rid);
			attrcatFH.DeleteRec(rid);
		}
	}
	if (fs.CloseScan())
		return -1;
	if (fs.OpenScan(relcatFH, INT, maxname, 0, NO_OP, 0))
		return -1;
	RelCatEntry *reltion;
	while (true) {
		if (fs.GetNextRec(rec))
			break;
		if (rec.GetData((char *&)reltion))
			return -1;
		if (strcmp(reltion->relName, name) == 0) {
			rec.GetRid(rid);
			relcatFH.DeleteRec(rid);
		}
	}
	if (rmm.DestroyFile(name))
		return -1;
	return 0;
}

Status SM_Manager::Select(int nRelAttrs, RelAttr *relAttrs, int nReltions, const char *name, int nConditions, Condition *conditions) {
	int reltionPos = -1;
	for (int i = 0; i < RelVec.size(); i++) {
		if (strcmp(RelVec[i].relName, name) == 0) {
			reltionPos = i;
			break;
		}
	}
	if (reltionPos == -1) return -1;

	int nattrs = RelVec[reltionPos].attrCount;
	AttrCatEntry *attrEntry = new AttrCatEntry[nConditions];
	GetAttrsFromCondtion(name, attrEntry, conditions, nConditions);
//	for (int i = 0; i < nConditions; i++)
//		cout << conditions[i]. << " " << attrEntry[i].attrType << endl;
	RM_FileHandle rfh;
	if (rmm.OpenFile(name, rfh))
		return -1;
	RM_FileScan fs;
	if (fs.OpenScan(rfh, STRING, 0, 0, NO_OP, 0))
		return -1;

	RM_Record record;
	RID rid;
	char *pData;
	AttrCatEntry *relAttrsEntry;
	int cnt = 0;
	if (relAttrs[0].relName != NULL) {
		relAttrsEntry = new AttrCatEntry[nRelAttrs];
		GetAttrsFromRel(relAttrsEntry, nRelAttrs, relAttrs);
		cnt = nRelAttrs;
	}
	else
	{
		relAttrsEntry = new AttrCatEntry[RelVec[reltionPos].attrCount];
		for (int i = 0; i < AttrVec.size(); i++) {
			if (strcmp(AttrVec[i].relName, name) == 0)
				relAttrsEntry[cnt++] = AttrVec[i];
		}
	}
	Printer print(cnt, relAttrsEntry);
	print.PrintHeader(relAttrsEntry);
	while (true) {
		if (fs.GetNextRec(record))
			break;
		record.GetData(pData);
		bool flag = true;
		for (int i = 0; i < nConditions; i++) {
		//	cout << attrEntry[i].attrName<< " "  << conditions[i].op << " " << attrEntry[i].attrType << endl;
		//	if(attrEntry[i].attrType == STRING) cout << (pData + attrEntry[i].offset) << " " << (char *)conditions[i]->rhsValue.data << endl;
			if (Compare(pData + attrEntry[i].offset, conditions[i].rhsValue.data, attrEntry[i].attrType, conditions[i].op))
			{
				flag = false;
				break;
			}
		}
		record.GetRid(rid);
		if (flag) {
			print.PrintData(relAttrsEntry, pData);
		}
	}
	if (rmm.CloseFile(rfh))
		return -1;

	delete attrEntry;
	delete relAttrsEntry;
	return 0;
}










//Status SM_Manager::Select(int nRelAttrs, RelAttr *relAttrs, int nReltions, const char **reltions, int nConditions, Condition *conditions) {
//	RelCatEntry *relEntries = new RelCatEntry[nReltions];
//	int maxAttrs = 0;
//	for (int i = 0; i < RelVec.size(); i++) {
//		for (int j = 0; j < nReltions; j++)
//		{
//			if (strcmp(reltions[j], RelVec[i].relName) == 0)
//			{
//				relEntries[j] = RelVec[i];
//				break;
//			}
//		}
//	}
//
//	for (int i = 0; i < nReltions; i++) maxAttrs += relEntries[i].attrCount;
//	AttrCatEntry *attrEntries = new AttrCatEntry[maxAttrs];
//	void **ptrData = new void *[maxAttrs];
//	for (int i = 0; i < AttrVec.size(); i++) {
//		for (int j = 0; j < nConditions; j++) {
//			if(strcmp(AttrVec[i].relName, conditions[j].lhsAttr.relName) == 0 && strcmp(AttrVec[i].attrName, conditions[j].lhsAttr.attrName) == 0)
//		}
//	}
//	
//
//
//
//	dfs(nReltions, relEntries ,nConditions, conditions,0,  ptrData, attrEntries, nRelAttrs, relAttrs);
//	
//	return 0;
//}
//
//Status SM_Manager::dfs(int d, RelCatEntry *reltions,  int nConditions, Condition *conditions, int nattrs, void **ptrData, AttrCatEntry *attrEntries, int nRelAttrs, RelAttr *relAttrs) {
//	for (int i = 0; i < nConditions; i++) {
//		if (strcmp(conditions[i].lhsAttr.relName, reltions[i].relName) == 0 || (conditions[i].bRhsIsAttr && strcmp(conditions[i].rhsAttr.relName, reltions[i].relName) == 0))
//		{
//			attrEntries[nattrs] = ;
//		}
//	}
//
//	return 0;
//}
