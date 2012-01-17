#include "mud.h"
#include <sys/stat.h>
#include <sys/signal.h>
#include <unistd.h>
#include <errno.h>

#include "threat.h"

bool listing = false;

void setupMonster(int num, Monster *monster) {
	switch(num) {
		case 1:
			strcpy(monster->name, "red dragon");
			break;
		case 2:
			strcpy(monster->name, "Orwik, Master Sorcerer");
			break;
		case 3:
			strcpy(monster->name, "wimpy peon");
			break;
		default:
			strcpy(monster->name, "derp");
			break;
	}
	monster->validateId();
	monster->setLevel(1);
	monster->setType(MONSTER);
	monster->strength.setCur(100);
	monster->dexterity.setCur(100);
	monster->constitution.setCur(100);
	monster->intelligence.setCur(100);
	monster->piety.setCur(100);
	monster->hp.setMax(12);
	monster->hp.restore();
	monster->setArmor(25);
	monster->setDefenseSkill(5);
	monster->setWeaponSkill(5);
	monster->setExperience(10);
	monster->damage.setNumber(1);
	monster->damage.setSides(4);
	monster->damage.setPlus(1);
	monster->first_obj = 0;
	monster->first_fol = 0;
	monster->first_enm = 0;
	monster->first_tlk = 0;
	monster->parent_rom = 0;
	monster->area_room = 0;
	monster->following = 0;
}

int main(int argc, char *argv[]) {
    // We're using random, so seed it
    srand(getpid() + time(0));

	// Get our instance variables
	gConfig = Config::getInstance();
	gServer = Server::getInstance();

	Monster *monster1 = new Monster;
	setupMonster(1, monster1);
	ThreatTable table1(monster1);

	Monster *monster2 = new Monster;
	setupMonster(2, monster2);
	ThreatTable table2(monster2);

	Monster *monster3 = new Monster;
	setupMonster(3, monster3);

	ThreatTable table3(monster3);

	Monster *monster4 = new Monster;
    setupMonster(3, monster4);

    Monster *monster5 = new Monster;
    setupMonster(3, monster5);

    Monster *monster6 = new Monster;
    setupMonster(3, monster6);

    Monster *monster7 = new Monster;
    setupMonster(3, monster7);


	table1.adjustThreat(monster2, 15000);
	table1.adjustThreat(monster2, 15);
	table1.adjustThreat(monster2, 35);
	table1.adjustThreat(monster3, 35);
	table1.adjustThreat(monster3, 35);
	table1.adjustThreat(monster3, 35);
	table1.adjustThreat(monster3, -35);
	table1.adjustThreat(monster1, 500);

	table1.adjustThreat(monster5, 70);
	table1.adjustThreat(monster6, 70);
	table1.adjustThreat(monster7, 50);
    table1.adjustThreat(monster4, 70);
    table1.adjustThreat(monster6, 70);
    table1.adjustThreat(monster6, 70);


	table2.adjustThreat(monster1, 35);
	table2.adjustThreat(monster1, 55);
	table1.adjustThreat(monster2, -15000);

	table1.removeThreat(monster2);

	delete monster6;

	MudObject* target;;
	std::cout << std::endl << table1;
	target = table1.getTarget();
	if(target)
	    std::cout << "Target: " << target->getName() << std::endl;
	else
	    std::cout << "No Target" << std::endl;

	std::cout << std::endl << table2;
	target = table2.getTarget();
	if(target)
	        std::cout << "Target: " << target->getName() << std::endl;
	    else
	        std::cout << "No Target" << std::endl;

	target = table3.getTarget();
	if(target)
	        std::cout << "Target: " << target->getName() << std::endl;
	    else
	        std::cout << "No Target" << std::endl;


	delete monster1;
	delete monster2;
	delete monster3;
	return(0);
}
