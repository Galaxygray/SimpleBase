#include "base.h"
#include "SystemManager.h"
FileManager fm;
RM_Manager rm(fm);
SM_Manager sm(rm);

int main(int argc, char *argv[])
{
	char *dbname;
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " dbname \n";
		exit(1);
	}

	// Opens up the database folder    
	dbname = argv[1];
	cout << argv[1] << endl;
	if (sm.OpenDb(dbname)) {
		return (1);
	}

	cout << "opened!" << endl;
	RBparse(sm);

	// Closes the database folder
	if (sm.CloseDb()) {
		return (1);
	}


	cout << "Bye.\n";
}