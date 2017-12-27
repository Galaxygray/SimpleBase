#include "File.h"

typedef char Status;

FileManager::FileManager() {
	pBuffer = new BufferManager(BUFFER_SIZE);
}

FileManager::~FileManager() {
	delete pBuffer;
}

Status FileManager::CreateFile(const char* filename){
	int fd;
	if (filename == NULL)
		return -1;
	if ( (fd = _sopen( filename, O_BINARY | O_CREAT | O_EXCL | O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE)) < 0)
		return -1;
	
    char* buff = new char[FILE_HEAD_SIZE];
    memset(buff, 0 , FILE_HEAD_SIZE);
    FileHeader* fileheader = (FileHeader *)(buff);
    fileheader->firstfree = -1;
    fileheader->numpages = 0;
    int numbytes = -1;
    if((numbytes = _write(fd, buff, FILE_HEAD_SIZE)) != FILE_HEAD_SIZE){
		_close(fd);
        if(numbytes < 0){
            printf("System write Error!\n");
            return -1;
        }
        else{
            printf("write FileHeader to file failed!\n");
            return -1;
        }
    }

	_close(fd);
    return 0;
}

Status FileManager::DestroyFile(const char* filename){
	int st;
	if ((st = remove(filename)) < 0) {
		cout << st << endl;
		return -1;
	}
    return 0;
}

Status FileManager::OpenFile(const char* filename, FileHandle& filehandle){
	if (filehandle.Isopen)
		return -1;
	if ((filehandle.fd = _open(filename, O_BINARY| O_RDWR)) < 0) {
		cout << "Opened Failed!\n" << endl;
		return -1;
	}
	filehandle.bfm = pBuffer;
    int numbytes = _read(filehandle.fd, (char*)(&filehandle.fileheader), sizeof(FileHeader));
    if(numbytes != sizeof(FileHeader)){
		cout << numbytes << endl;
        printf("FileHeader Read Error!\n");
        return -1;
    }

    filehandle.Isopen = true;
     return 0;
}

Status FileManager::CloseFile(FileHandle& filehandle){
    if(!filehandle.Isopen){
        printf("File has closed!\n");
        return -1;
    }
    if(filehandle.FlushToDisk()){
        return -1;
    }
	_close(filehandle.fd);
    return 0;
}





