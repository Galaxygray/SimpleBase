#include "Record.h"
#include <direct.h>
#include <map>
#include <set>
#include <vector>
const int maxname = 20;

typedef struct RelCatEntry {
	char relName[maxname + 1];
	int tupleLength;
	int attrCount;
	int numTuples;
	bool statsInitialized;
} RelCatEntry;

typedef struct AttrCatEntry {
	char relName[maxname + 1];
	char attrName[maxname + 1];
	int offset;
	AttrType attrType;
	int attrLength;
	int attrNum;
	int numDistinct;
} AttrCatEntry;


class SM_Manager {
public:
	SM_Manager(RM_Manager &rmm);
	~SM_Manager();                             // Destructor
	Status CreateDb(const char *dbnaem);
	Status DestroyDb(const char *dbname);
	Status OpenDb(const char *dbName);           // Open the database
	Status CloseDb();                             // close the database
	Status PrintAll();
	Status Insert(const char *name, int nValues, Value *values);
	Status GetAttrsEntry(const char *name, AttrCatEntry *attrsOffset, int &nAttrs);
	Status InitRelVec();
	Status InitAttrVec();
	Status Delete(const char *name, int nConditions, Condition *condtions);
	Status GetAttrsFromCondtion(const char *name, AttrCatEntry *attrs, Condition *conditions, int nCondtions);
	Status GetAttrFromRel(const char *name, AttrCatEntry &attr, RelAttr &relattr);
	Status GetAttrsFromRel(AttrCatEntry *attr,int nRelattrs,  RelAttr *relattr);
	Status Update(const char *name, int nCondtions, Condition *condtions, RelAttr &relattr, Value &value);
	Status Select(int nRelAttrs, RelAttr *relAttrs, int nReltions, const char *reltions, int nConditions, Condition *conditions);
	Status dfs(int d,RelCatEntry *reltions, int nConditions, Condition *conditions, int nattrs, void **ptrData, AttrCatEntry *attrEntries, int nRelAttrs, RelAttr *relAttrs);
	int compareEQ(void *value1, void *value2, AttrType type);
	int compareLE(void *value1, void *value2, AttrType type);
	int compareGE(void *value1, void *value2, AttrType type);
	int compareLT(void *value1, void *value2, AttrType type);
	int compareGT(void *value1, void *value2, AttrType type);
	int Compare(void *value1, void *value2, AttrType type, CompOp compOp) {
		switch (compOp) {
		case EQ_OP :
			return compareEQ(value1, value2, type);
		case NE_OP :
			return !compareEQ(value1, value2, type);
		case LE_OP :
			return compareLE(value1, value2, type);
		case GE_OP :
			return compareGE(value1, value2, type);
		case LT_OP :
			return compareLT(value1, value2, type);
		case GT_OP :
			return compareGT(value1, value2, type);
		}
		return 0;
	}

	Status CreateTable(const char *relName,           // create relation relName
		int        attrCount,          //   number of attributes
		
		AttrInfo   *attributes);       //   attribute data
	Status CreateIndex(const char *relName,           // create an index for
		const char *attrName);         //   relName.attrName
	Status DropTable(const char *relName);          // destroy a relation

	Status DropIndex(const char *relName,           // destroy index on
		const char *attrName);         //   relName.attrName
	Status Load(const char *relName,           // load relName from
		const char *fileName);         //   fileName
	Status Help();                             // Print relations in db
	Status Help(const char *relName);          // print schema of relName

	Status Print(const char *relName);          // print relName contents

	Status Set(const char *paramName,         // set parameter to
		const char *value);            //   value

private:
	// Returns true if given attribute has valid/matching type and length
	//bool isValidAttrType(AttrInfo attribute);

	//// Inserts an entry about specified relName relation into relcat
	//Status InsertRelCat(const char *relName, int attrCount, int recSize);

	//// Insert an entry about specified attribute into attrcat
	//Status InsertAttrCat(const char *relName, AttrInfo attr, int offset, int attrNum);
	//// Retrieve the record and data associated with a relation entry. Return
	//// error if one doesnt' exist
	//Status GetRelEntry(const char *relName, RM_Record &relRec, RelCatEntry *&entry);

	//// Finds the entry associated with a particular attribute
	//Status FindAttr(const char *relName, const char *attrName, RM_Record &attrRec, AttrCatEntry *&entry);

	//// Sets up print for DataAttrInfo from a file, printing relcat and printing attrcat
	//Status SetUpPrint(RelCatEntry* rEntry, DataAttrInfo *attributes);
	//Status SetUpRelCatAttributes(DataAttrInfo *attributes);
	//Status SetUpAttrCatAttributes(DataAttrInfo *attributes);

	//// Prepares the Attr array, which helps with loading
	//Status PrepareAttr(RelCatEntry *rEntry, Attr* attributes);

	//// Given a RelCatEntry, it populates aEntry with information about all its attribute.
	//// While doing so, it also updates the attribute-to-relation mapping
	//Status GetAttrForRel(RelCatEntry *relEntry, AttrCatEntry *aEntry, std::map<std::string, std::set<std::string> > &attrToRel);
	//// Given a list of relations, it retrieves all the relCatEntries associated with them placing
	//// them in the list specified by relEntries. It also returns the total # of attributes in all the
	//// relations combined, and populates the mapping from relation name to index number in relEntries
	//Status GetAllRels(RelCatEntry *relEntries, int nRelations, const char * const relations[],
	//	int &attrCount, std::map<std::string, int> &relToInt);

	//// Opens a file and loads it
	//Status OpenAndLoadFile(RM_FileHandle &relFH, const char *fileName, Attr* attributes,
	//	int attrCount, int recLength, int &loadedRecs);
	//// Cleans up the Attr array after loading
	//Status CleanUpAttr(Attr* attributes, int attrCount);
	//float ConvertStrToFloat(char *string);
	//Status PrintStats(const char *relName);
	//Status CalcStats(const char *relName);

	//Status PrintPageStats();
	//Status ResetPageStats();

	RM_Manager &rmm;

	RM_FileHandle relcatFH;
	RM_FileHandle attrcatFH;
	std::vector<RelCatEntry> RelVec;
	std::vector<AttrCatEntry> AttrVec;
	bool printIndex; 

	bool useQO;
	bool calcStats;
	bool printPageStats;
};



class Printer {
	friend class SM_Manager;
	Printer(int nattrs, AttrCatEntry *attrEntery);
	~Printer();
	Status PrintData(AttrCatEntry *attrEntry, void *pData);
	Status PrintHeader(AttrCatEntry *attrEntry);
	std::vector<int> Space;
};
