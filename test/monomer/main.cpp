#include "monomer.h"
#include "gamcs/CSOSAgent.h"
#ifdef _MYSQL_FOUND_
#include "gamcs/Mysql.h"
#endif
#include "gamcs/CDotViewer.h"
#include "gamcs/PrintViewer.h"
#include "gamcs/Sqlite.h"

int main(void)
{
	CSOSAgent ma(1, 0.9, 0.01);
//#ifdef _MYSQL_FOUND_
//	Mysql mysql("localhost", "root", "huangk", "Monomer");
//	ma.loadMemoryFromStorage(&mysql);
//#endif
	Sqlite sqlite("./test.db");
	ma.loadMemoryFromStorage(&sqlite);

	Monomer mono;
	mono.connectAgent(&ma);
	mono.loop();

	//ma.deleteState(9);
	PrintViewer dotv;
	dotv.attachStorage(&ma);
	dotv.view();

	ma.dumpMemoryToStorage(&sqlite);
//#ifdef _MYSQL_FOUND_
//	ma.dumpMemoryToStorage(&mysql);
//#endif
}
