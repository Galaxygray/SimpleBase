#include "File.h"

BufferManager::BufferManager(int numpages) : hashTable(HASH_TABLE_SIZE){
    this->numPages = numpages;
    pageSize = PAGE_SIZE + sizeof(PageHeader);
    bufTable = new BufPageData[numpages];

    for(int i = 0; i < numpages; i ++){
        if((bufTable[i].ptrData = new char[pageSize]) == NULL){
         //   printf("Allocate memeory for buffer failed!\n");
        }
        memset(bufTable[i].ptrData, 0 , pageSize);
        bufTable[i].prev = i - 1;
        bufTable[i].next = i + 1;
    }
    free = 0;
	bufTable[0].prev = bufTable[numPages - 1].next = first = last = INVAILD_SLOT;
}

BufferManager::~BufferManager(){
    for(int i = 0; i < this->numPages; i ++)
        delete[] bufTable[i].ptrData;
    delete[] bufTable;
}

Status BufferManager::GetPage(int fd, int pagenum, char** buffer, bool Ismulpin){
    int slot;
    if(hashTable.Find(fd, pagenum, slot))
    {
        if(InternalAlloc(slot)){
      //      printf("Cannot create page in buffer\n");
            return -1;
        }
		if (ReadPage(fd, pagenum, bufTable[slot].ptrData) || hashTable.Insert(fd, pagenum, slot) || InitPageData(fd, pagenum, slot)) {
			Unlink(slot);
			InsertFree(slot);
			return -1;
		}
		*buffer = bufTable[slot].ptrData;
        return 0;
    }

    if(!Ismulpin && bufTable[slot].pincount > 0){
    //    printf("Cannot get this got page!\n");
        return -1;
    }

    bufTable[slot].pincount ++;
    if(Unlink(slot) || LinkHead(slot))
         return -1;

    *buffer = bufTable[slot].ptrData;
    return 0;
}

Status BufferManager::AllocatePage(int fd, int numpage, char** buffer){
    int slot;
    if(!hashTable.Find(fd, numpage, slot))
        return -1;
	//cout << "InternalAlloc a Page!" << endl;
    if(InternalAlloc(slot))
        return -1;
    if(hashTable.Insert(fd, numpage, slot) || InitPageData(fd, numpage, slot)){
        Unlink(slot);
        InsertFree(slot);
        return -1;
    }

    *buffer = bufTable[slot].ptrData;
    return 0;
}

Status BufferManager::MarkDirty(int fd, int pagenum){
    int slot;
    if(hashTable.Find(fd, pagenum, slot))  
        return -1;
    
    bufTable[slot].dirty = true;

    if(Unlink(slot) || LinkHead(slot))
        return -1;
	return 0;
}

Status BufferManager::UnpinPage(int fd, int pagenum){
    int slot;
    if(hashTable.Find(fd, pagenum, slot))
        return -1;
    if(bufTable[slot].pincount == 0)
        return -1;

    bufTable[slot].pincount --;
    if(bufTable[slot].pincount == 0){
        Unlink(slot);
        LinkHead(slot);
    }

    return 0;
}

Status BufferManager::FlushPages(int fd){
    int slot = first;
    while(slot != INVAILD_SLOT){
        int next = bufTable[slot].next;
        if(!bufTable[slot].pincount){
			int t;
			if (hashTable.Find(fd, bufTable[slot].pagenum, t) || t != slot) {
				slot = next;
				continue;
			}
			if(bufTable[slot].dirty)
				WritePage(fd, bufTable[slot].pagenum, bufTable[slot].ptrData);
            bufTable[slot].dirty = false;
            hashTable.Delete(fd, bufTable[slot].pagenum);
            Unlink(slot);
            InsertFree(slot);
        }
        slot = next;
    }
	return 0;
}

Status BufferManager::FlushToDisk(int fd, int numpage){
    int slot;
    if(hashTable.Find(fd, numpage, slot))
        return -1;
    
    if(bufTable[slot].dirty){
        WritePage(fd, numpage, bufTable[slot].ptrData);
        bufTable[slot].dirty = false;
    }

    return 0;
}

Status BufferManager::ClearBuff(){
    int slot = first;
    while(slot != INVAILD_SLOT){
        int next = bufTable[slot].next;
        if(!bufTable[slot].pincount)
        if(hashTable.Delete(bufTable[slot].fd, bufTable[slot].pagenum) || Unlink(slot) || InsertFree(slot))
            return -1;
        slot = next;
    }
    return 0;
}

Status BufferManager::InsertFree(int slot){
    bufTable[slot].next = free;
    free = slot;
    return 0;
}

Status BufferManager::Unlink(int slot) {
	if (first == slot)
		first = bufTable[slot].next;
	if (last == slot)
		last = bufTable[slot].prev;
	if(bufTable[slot].prev != INVAILD_SLOT)
		bufTable[bufTable[slot].prev].next = bufTable[slot].next;
	if (bufTable[slot].next != INVAILD_SLOT)
		bufTable[bufTable[slot].next].prev = bufTable[slot].prev;

	bufTable[slot].prev = bufTable[slot].next = INVAILD_SLOT;
	return 0;
}

Status BufferManager::LinkHead(int slot){
    bufTable[slot].next = first;
    bufTable[slot].prev = -1;
    if(first != INVAILD_SLOT)
        bufTable[first].prev = slot;
    first = slot;
    if(last != INVAILD_SLOT)
        last = first;

    return 0;  
}

Status BufferManager::InternalAlloc(int& slot){
    if(free != INVAILD_SLOT){
        slot = free;
        free = bufTable[slot].next;
    }
    else{
        for(slot = last; slot != INVAILD_SLOT; slot = bufTable[slot].prev){
            if(bufTable[slot].pincount == 0){
                break;
            }
        }
        if(slot == INVAILD_SLOT)
            return -1;
        
        if(bufTable[slot].dirty){
            WritePage(bufTable[slot].fd, bufTable[slot].pagenum,bufTable[slot].ptrData);
            bufTable[slot].dirty = false;
        }
        hashTable.Delete(bufTable[slot].fd, bufTable[slot].pagenum);
        Unlink(slot);
    }
    if(LinkHead(slot))
        return -1;
    return 0;
}

Status BufferManager::ReadPage(int fd, int pagenum, char* dest){
    long offset = pagenum * pageSize + FILE_HEAD_SIZE;
	if (_lseek(fd, offset, 0) < 0)
		return -1;
	int numbytes = _read(fd, dest, pageSize);
	if (numbytes < 0 || numbytes != pageSize)
		return -1;
	return 0;
}

Status BufferManager::WritePage(int fd, int pagenum, char* source) {
	long offset = pagenum * pageSize + FILE_HEAD_SIZE;
	if (_lseek(fd, offset, 0) < 0)
		return -1;

	int numbytes = _write(fd, source, pageSize);
	if (numbytes < 0 || numbytes != pageSize)
		return -1;
	return 0;
}

Status BufferManager::InitPageData(int fd, int pagenum, int slot) {
	bufTable[slot].fd = fd;
	bufTable[slot].pagenum = pagenum;
	bufTable[slot].dirty = false;
	bufTable[slot].pincount = 1;
	return 0;
}

