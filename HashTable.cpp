#include "File.h"

HashTable::HashTable(int numBuckets){
    this->numBuckets = numBuckets;
    hashTable = new HashEntry* [numBuckets];
    for(int i = 0; i < numBuckets; i ++)
        hashTable[i] = NULL;
}

HashTable::~HashTable(){
    for(int i = 0; i < numBuckets; i ++){
        HashEntry* ptr = hashTable[i], *Next;
        while(ptr != NULL){
            Next = ptr->next;
            delete ptr;
            ptr = Next;
        }
    }
    delete[] hashTable;
}

Status HashTable::Find(int fd, int pagenum, int& slot){
    int bucket = Hash(fd, pagenum);
	//cout << "HashTable st!" << endl;
    for(HashEntry* ptr = hashTable[bucket]; ptr != NULL; ptr = ptr->next){
        if(ptr->pagenum == pagenum){
            slot = ptr->slot;
            return 0;
        }
    }

   // printf("Cann't find this pagenum!\n");
    return -1;
}

Status HashTable::Insert(int fd, int pagenum, int slot){
	int bucket = Hash(fd, pagenum);
    HashEntry* ptr;
    for(ptr = hashTable[bucket]; ptr != NULL; ptr = ptr->next){
        if(ptr->pagenum == pagenum){
            printf("This page has existed!\n");
            return -1;
        }
    }
    if((ptr = new HashEntry) == NULL){
        printf("Allocate Meomory to new HashEntry failed!\n");
        return -1;
    }

    ptr->pagenum = pagenum;
    ptr->slot = slot;
    ptr->next = hashTable[bucket];
    ptr->prev = NULL;
    if(hashTable[bucket] != NULL)
        hashTable[bucket]->prev = ptr;
    hashTable[bucket] = ptr;
    return 0;
}

Status HashTable::Delete(int fd, int pagenum){
    int bucket = Hash(fd, pagenum);
    HashEntry* ptr;
    for(ptr = hashTable[bucket]; ptr != NULL; ptr = ptr->next){
        if(ptr->pagenum == pagenum){
            break;
        }
    }
    if(ptr == NULL){
        printf("Cannot't find this page!\n");
        return -1;
    }
    if(ptr == hashTable[bucket])
        hashTable[bucket] = ptr->next;
    if(ptr->prev != NULL)
        ptr->prev->next = ptr->next;
    if(ptr->next != NULL)
        ptr->next->prev = ptr->prev;
    delete ptr;
    return 0;
}