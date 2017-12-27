#include "Record.h"

class IX_IndexHandle;

struct IX_IndexHeader {
	AttrType attr_type; // attribute type and length
	int attr_length;

	int entryOffset_N;  // the offset from the header of the beginning of

	int keysOffset_N;   // the offset from the header of the beginning of
						// the keys list in the nodes
	int maxKeys_N;      // Maximum number of entries in buckets and nodes

	int rootPage;   // Page number associated with the root page
};

class IX_Manager {
	static const char UNOCCUPIED = 'u';
public:
	IX_Manager(FileManager &pfm);
	~IX_Manager();

	// Create a new Index
	Status CreateIndex(const char *fileName, int indexNo,
		AttrType attrType, int attrLength);

	// Destroy and Index
	Status DestroyIndex(const char *fileName, int indexNo);

	// Open an Index
	Status OpenIndex(const char *fileName, int indexNo,
		IX_IndexHandle &indexHandle);

	// Close an Index
	Status CloseIndex(IX_IndexHandle &indexHandle);

private:
	// Checks that the index parameters given (attrtype and length) make
	// a valid index
	bool IsValidIndex(AttrType attrType, int attrLength);

	// Creates the index file name from the filename and index number, and
	// returns it as a string in indexname
	Status GetIndexFileName(const char *fileName, int indexNo, std::string &indexname);
	// Sets up the IndexHandle internal varables when opening an index
	Status SetUpIH(IX_IndexHandle &ih, FileHandle &fh, struct IX_IndexHeader *header);
	// Modifies th IndexHandle internal variables when closing an index
	Status CleanUpIH(IX_IndexHandle &indexHandle);

	FileManager &pfm; // The PF_Manager associated with this index.
};

struct IX_NodeHeader {
	bool isLeafNode;  // indicator for whether it is a leaf node
	bool isEmpty;     // Whether the node contains pointers or not
	int num_keys;     // number of valid keys the node holds

	int firstSlotIndex; // the pointer to the beginning of the linked list of
						// valid key/pointer slots
	int freeSlotIndex;  // the pointer to the beginning of free slots
	int next;
	int pre;
};

struct Node_Entry {
	char isValid;     // Whether the slot is valid, contains a duplicate
					  // value, or a single value
	int nextSlot;     // Pointer to the next slot in the node
	int page;     // Pointer to the page associated with this key
	int slot;     // Pointer to the slot associated with this entry
					  // (only valid for leaf nodes)
};

class IX_IndexHandle {
	friend class IX_Manager;
	friend class IX_IndexScan;
	static const int BEGINNING_OF_SLOTS = -2; // class constants
	static const int END_OF_SLOTS = -3;
	static const char UNOCCUPIED = 'u';
	static const char OCCUPIED_NEW = 'n';
	static const char OCCUPIED_DUP = 'r';
public:
	IX_IndexHandle();
	~IX_IndexHandle();

	// Insert a new index entry
	Status InsertEntry(void *pData, const RID &rid);

	// Delete a new index entry
	Status DeleteEntry(void *pData, const RID &rid);

	// Force index files to disk
	Status ForcePages();
	Status PrintIndex();
private:
	// Creates new node and bucket pages
	Status CreateNewNode(PageHandle &ph, int &page, char *& nData, bool isLeaf);
	Status CreateNewBucket(int &page);

	// Inserts a value into a non-full node, or a bucket
	Status InsertIntoNonFullNode(struct IX_NodeHeader *nHeader, int thisNodeNum, void *pData, const RID &rid);
	Status InsertIntoBucket(int page, const RID &rid);

	// Splits the node given itself, and the parent node
	Status SplitNode(struct IX_NodeHeader *pHeader, struct IX_NodeHeader *oldHeader, int oldPage, int index,
		int &newKeyIndex, int &newPageNum);

	// Find the index before the current one passed in
	Status FindPrevIndex(struct IX_NodeHeader *nHeader, int thisIndex, int &prevIndex);
	// Find the appropriate index to insert the value into
	Status FindNodeInsertIndex(struct IX_NodeHeader *nHeader,
		void* pData, int& index, bool& isDup, const RID& rid = RID(-1,-1));

	// Delete from an internal node, a leaf, and a bucket
	Status DeleteFromNode(struct IX_NodeHeader *nHeader, void *pData, const RID &rid, bool &toDelete);
	Status DeleteFromBucket(struct IX_BucketHeader *bHeader, const RID &rid,
		bool &deletePage, RID &lastRID, int &nextPage);
	Status DeleteFromLeaf(struct IX_NodeHeader *nHeader, void *pData, const RID &rid, bool &toDelete);

	// checks if the values given in the header (offsets, sizes, etc) make
	// a valid header
	bool isValidIndexHeader() const;

	// Given an attribute length, calculates the max number of entries
	// for the bucket and the nodes
	static int CalcNumKeysNode(int attrLength);
	static int CalcNumKeysBucket(int attrLength);

	// Returns the first leaf page in leafPH, and its page number in
	// leafPage
	Status GetFirstLeafPage(PageHandle &leafPH, int &leafPage);
	Status FindRecordPage(PageHandle &leafPH, int &leafPage, void * key);

	// Private variables
	bool isOpenHandle;     // Indicator for whether the indexHandle is being used
	FileHandle pfh;     // The PF_FileHandle associated with this index
	bool header_modified;  // Indicator for whether the header has been modified
	PageHandle rootPH;  // The PF_PageHandle associated with the root node
	struct IX_IndexHeader header; // The header for this index

								  // The comparator used to compare keys in this index
	int(*comparator) (void *, void *, int);
	bool(*printer) (void *, int);

};

class IX_IndexScan {
public:
	IX_IndexScan();                                 // Constructor
	~IX_IndexScan();                                 // Destructor
	Status OpenScan(const IX_IndexHandle &indexHandle, // Initialize index scan
		CompOp      compOp,
		void        *value,
		ClientHint  pinHint = NO_HINT);
	Status GetNextEntry(RID &rid);                         // Get next matching entry
	Status CloseScan();                                 // Terminate index scan

private:
	Status BeginScan(PageHandle &leafPH, int &pageNum);
	// Sets up the scan private variables to the first entry within the given leaf
	Status GetFirstEntryInLeaf(PageHandle &leafPH);
	// Sets up the scan private variables to the appropriate entry within the given leaf
	Status GetAppropriateEntryInLeaf(PageHandle &leafPH);
	// Sets up the scan private variables to the first entry within this bucket
	Status GetFirstBucketEntry(int nextBucket, PageHandle &bucketPH);
	// Sets up the scan private variables to the next entry in the index
	Status FindNextValue();

	// Sets the RID
	Status SetRID(bool setCurrent);


	bool openScan;              // Indicator for whether the scan is being used
	IX_IndexHandle *indexHandle;// Pointer to the indexHandle that modifies the
								// file that the scan will try to traverse

								// The comparison to determine whether a record satisfies given scan conditions
	bool(*comparator) (void *, void*, AttrType, int);
	int attrLength;     // Comparison type, and information about the value
	void *value;        // to compare to
	AttrType attrType;
	CompOp compOp;

	bool scanEnded;     // Indicators for whether the scan has started or 
	bool scanStarted;   // ended

	PageHandle currLeafPH;   // Currently pinned Leaf and Bucket PageHandles

	char *currKey;              // the keys of the current record, and the following
	char *nextKey;              // two records after that
	char *nextNextKey;
	struct IX_NodeHeader *leafHeader;     // the scan's current leaf and bucket header
	struct Node_Entry *leafEntries;
	char * leafKeys;
	int leafSlot;              
	int bucketSlot;
	int currLeafNum;       
	int currBucketNum;
	int nextBucketNum;

	RID currRID;   
	RID nextRID;

	bool hasBucketPinned; 
	bool hasLeafPinned;
	bool initializedValue; 
	bool endOfIndexReached; 
	bool foundFirstValue;
	bool foundLastValue;
	bool useFirstLeaf;
};

