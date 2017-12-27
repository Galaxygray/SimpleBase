#include <fstream>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <io.h> 
#include <share.h> 
#include <errno.h>
using namespace std;
typedef char Status;
const int ALL_PAGE = -1;
const int BUFFER_SIZE = 40;
const int OPENFAILED = -2;
const int PAGE_USED = -3;
class BufferManager;
class FileHandle;

class FileManager {
public:
	FileManager();
	~FileManager();
	bool IsExists(const char* filename);
	Status CreateFile(const char* filename);
	Status DestroyFile(const char* filename);
	Status OpenFile(const char* filename, FileHandle& filehandle);
	Status CloseFile(FileHandle& filehandle);
	Status AllocateFile(char** buffer);
	Status DisposeBlock(char* buffer);
private:
	BufferManager *pBuffer;
};

class PageHandle {
	friend class FileHandle;
public:
	PageHandle();
	~PageHandle();
	PageHandle(const PageHandle &pagehandle);
	PageHandle&  operator =(const PageHandle &pagehandle);
	Status GetData(char*& ptr_data) const;
	Status GetPageNum(int &pagenum) const;
private:
	char* ptr_Data;
	int pagenum;
};

struct FileHeader {
	int firstfree;
	int numpages;
};

class FileHandle {
	friend class FileManager;
public:
	FileHandle();
	~FileHandle();
	FileHandle(const FileHandle &filehandle);
	FileHandle&  operator = (const FileHandle &filehandle);
	Status GetFirstPage(PageHandle &pagehandle) const;
	Status GetLastPage(PageHandle &pagehandle) const;
	Status GetNextPage(int curpagenum, PageHandle &pagehandle) const;
	Status GetPrePage(int curpagenum, PageHandle &pagehandle) const;
	Status GetThisPage(int curpagenum, PageHandle& pagehandle) const;
	Status AllocatePage(PageHandle& pagehandle);
	Status DisposePage(int pagenum) ;
	Status MarkDirty(int pagenum) const;
	Status UnpinPage(int pagenum) const;
	Status FlushToDisk() const;
	Status ForcePage(int pagenum) const;
private:
	Status IsValidPageNum(int pageNum) const;
	int fd;
	FileHeader fileheader;
	BufferManager *bfm;
	bool Isopen;
	bool bHdrChanged;
};


struct BufPageData {
	char* ptrData;
	bool dirty;
	int pincount;
	int pagenum;
	int prev;
	int next;
	int fd;
};

struct PageHeader {
	int nextfree;
};

struct HashEntry {
	HashEntry *next;   // next hash table element or NULL
	HashEntry *prev;   // prev hash table element or NULL
	int      pagenum; // page number
	int          slot;    // slot of this page in the buffer
};


class HashTable {
public:
	HashTable(int numBuckets);           // Constructor
	~HashTable();                         // Destructor
	Status  Find(int fd, int pagenum, int &slot);
	// Set slot to the hash table
	// entry for fd and pageNum
	Status  Insert(int fd, int pagenum, int slot);
	// Insert a hash table entry
	Status  Delete(int fd, int pagenum);  // Delete a hash table entry

private:
	int Hash(int fd, int pagenum) const
	{
		return (fd + pagenum) % numBuckets;
	}   // Hash function
	int numBuckets;                               // Number of hash table buckets
	HashEntry **hashTable;                     // Hash table
};

class BufferManager {
public:
	BufferManager(int numpages);
	~BufferManager();
	Status GetPage(int fd, int pagenum, char** buff, bool Ismulpin = true);
	Status AllocatePage(int fd, int pagenum, char** buff);
	Status MarkDirty(int fd, int pagenum);
	Status UnpinPage(int fd, int pagenum);
	Status FlushPages(int fd);
	Status FlushToDisk(int fd, int pagenum);
	Status ClearBuff();
	Status PrintBuff();
	Status ResizeBuff();
	Status GetBlockSize();
	Status AllocateBlock();
	Status DisposeBlock();
private:
	Status  InsertFree(int slot);
	Status  LinkHead(int slot);
	Status  Unlink(int slot);
	Status  InternalAlloc(int &slot);
	Status  ReadPage(int fd, int pageNum, char *dest);
	Status  WritePage(int fd, int pageNum, char *source);
	Status  InitPageData(int fd, int pageNum, int slot);

	BufPageData *bufTable;
	HashTable   hashTable;
	int            numPages;
	int            pageSize;
	int            first;
	int            last;
	int            free;
};







const int PAGE_SIZE = (1 << 12) - sizeof(PageHeader);
const int FILE_HEAD_SIZE = PAGE_SIZE + sizeof(PageHeader);
const int HASH_TABLE_SIZE = 20;
const int INVAILD_SLOT = -1;
const int INVAILD_PAGE = -1;
