#include "File.h"
#include "base.h"

class RID {
public:
	RID();                                         // Default constructor
	RID(int pageNum, int slotNum);
	~RID();                                        // Destructor
	RID& operator= (const RID &rid);               // Copies RID
	bool operator== (const RID &rid) const;

	Status GetPageNum(int &pageNum) const;         // Return page number
	Status GetSlotNum(int &slotNum) const;         // Return slot number

	Status isValidRID() const; // checks if it is a valid RID

private:
	int page;
	int slot;
};

class RM_Record {
	friend class RM_FileHandle;
public:
	RM_Record() {

	}
	~RM_Record() {

	}
	RM_Record& operator= (const RM_Record &record) {
		if (this != &record) {
			this->data = record.data;
			this->rid = record.rid;
			this->size = record.size;
		}
		return *this;
	}

	Status SetRecord(const RID& rid, char *pData, int size) {
		this->rid = rid;
		this->data = pData;
		this->size = size;
		return 0;
	}

	Status GetData(char *&pData) const {
		pData = this->data;
		return 0;
	}

	Status GetRid(RID &rid) const {
		rid = this->rid;
		return 0;
	}

	Status GetSize(int &size) const {
		size = this->size;
		return 0;
	}

private:
	RID rid;        
	char * data;    
					
	int size;       
};

struct RM_FileHeader {
	int recordSize;           
	int numRecordsPerPage;    
	int numPages;             
	int firstFreePage;
	int bitmapOffset;        			  
	int bitmapSize;           
};

struct RM_PageHeader {
	int nextFreePage;
	int numRecords;
};

class RM_FileHandle {
	friend class RM_Manager;
	friend class RM_FileScan;
public:
	RM_FileHandle();
	~RM_FileHandle();
	RM_FileHandle& operator= (const RM_FileHandle &fileHandle);

	
	Status GetRec(const RID &rid, RM_Record &rec) const;

	Status InsertRec(const char *pData, RID &rid);
	Status PrintHeaderInfo() const;
	Status DeleteRec(const RID &rid);
	Status UpdateRec(const RM_Record &rec);
	Status ForcePages(int pageNum = -1);

private:
	
	static int NumBitsToCharSize(int size);

	
	static int CalcNumRecPerPage(int recSize);

	
	bool isValidFileHeader() const;
	int getRecordSize();

	Status GetNextRecord(int page, int slot, RM_Record &rec, PageHandle &ph, bool nextPage);

	
	Status AllocateNewPage(PageHandle &ph, int &page);

	bool isValidFH() const;

	Status GetPageDataAndBitmap(PageHandle &ph, char *&bitmap, struct RM_PageHeader *&pageheader) const;

	Status GetPageNumAndSlot(const RID &rid, int &page, int &slot) const;

	Status ResetBitmap(char * bitmap, int size);
	Status SetBit(char * bitmap, int size, int bitnum);
	Status ResetBit(char * bitmap, int size, int bitnum);
	Status CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const;
	Status GetFirstZeroBit(char *bitmap, int size, int &location);
	Status GetNextOneBit(char *bitmap, int size, int start, int &location);

	bool openedFH;
	struct RM_FileHeader header;
	FileHandle pfh;
	bool header_modified;
};

class RM_Manager {
public:
	RM_Manager(FileManager &pfm);
	~RM_Manager();

	Status CreateFile(const char *fileName, int recordSize);
	Status DestroyFile(const char *fileName);
	Status OpenFile(const char *fileName, RM_FileHandle &fileHandle);

	Status CloseFile(RM_FileHandle &fileHandle);
private:
	// helper method for open scan which sets up private variables of
	// RM_FileHandle. 
	Status SetUpFH(RM_FileHandle& fileHandle, FileHandle &fh, struct RM_FileHeader* header);
	// helper method for close scan which sets up private variables of
	// RM_FileHandle
	Status CleanUpFH(RM_FileHandle &fileHandle);

	FileManager &pfm;
};

class RM_FileScan {
public:
	RM_FileScan();
	~RM_FileScan();

	Status OpenScan(const RM_FileHandle &fileHandle,
		AttrType   attrType,
		int        attrLength,
		int        attrOffset,
		CompOp     compOp,
		void       *value,
		ClientHint pinHint = NO_HINT); 
	Status GetNextRec(RM_Record &rec);
	Status CloseScan();

private:
	
	Status GetNumRecOnPage(PageHandle& ph, int &numRecords);

	bool openScan; 

				 
	RM_FileHandle* fileHandle;
	bool(*comparator) (void *, void *, AttrType, int);
	int attrOffset;
	int attrLength;
	void *value;
	AttrType attrType;
	CompOp compOp;

	
	bool scanEnded;

	
	int scanPage;
	int scanSlot;
	PageHandle currentPH;
	
	int numRecOnPage;
	int numSeenOnPage;
	bool useNextPage;
	bool hasPagePinned;
	bool initializedValue;
};