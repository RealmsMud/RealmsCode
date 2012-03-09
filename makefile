LANG=C
# Moved compiler logic to compiler file, will commit a base of it and then add to svn ignore
# so we can modify the makefile and not worry about changing g++/icc
include compiler

CFLAGS := -g -Wall -I/usr/include/libxml2 -O0 $(COMPILER_CFLAGS) -std=c++0x -I.
CFLAGS += -I/usr/include/python3.2 


LIBS = -laspell -lxml2 -lz -lc -L./ -lpython3.2mu -lboost_python-py32 $(COMPILER_LIBS)

GENERAL_SOURCE := alphanum.cpp pythonHandler.cpp abjuration.cpp access.cpp action.cpp afflictions.cpp
GENERAL_SOURCE += alchemy.cpp alignment.cpp anchor.cpp area.cpp attack.cpp asynch.cpp timer.cpp bank.cpp
GENERAL_SOURCE += bans.cpp builder.cpp calendar.cpp carry.cpp catRef.cpp catRefInfo.cpp clans.cpp color.cpp
GENERAL_SOURCE += combat.cpp combatSystem.cpp command1.cpp command2.cpp command4.cpp command5.cpp container.cpp
GENERAL_SOURCE += commerce.cpp communication.cpp config.cpp conjuration.cpp cmd.cpp creatureStreams.cpp craft.cpp 
GENERAL_SOURCE += creature.cpp creature2.cpp creatures.cpp creatureAttr.cpp damage.cpp data.cpp delayedAction.cpp
GENERAL_SOURCE += demographics.cpp deityData.cpp die.cpp dice.cpp divination.cpp dm.cpp dmcrt.cpp dmobj.cpp dmply.cpp 
GENERAL_SOURCE += dmroom.cpp duel.cpp effects.cpp effectsAttr.cpp enchantment.cpp equipment.cpp errors.cpp evocation.cpp
GENERAL_SOURCE += exits.cpp factions.cpp files1.cpp files2.cpp files-xml-read.cpp fighters.cpp files-xml-save.cpp
GENERAL_SOURCE += fishing.cpp flags.cpp global.cpp gods.cpp group.cpp groups.cpp guilds.cpp healers.cpp healmagic.cpp 
GENERAL_SOURCE += hooks.cpp illusion.cpp io.cpp import.cpp levelGain.cpp location.cpp log.cpp logic.cpp login.cpp 
GENERAL_SOURCE += lottery.cpp magic.cpp magic1.cpp mccp.cpp md5.cpp memory.cpp misc.cpp money.cpp mxp.cpp msdp.cpp 
GENERAL_SOURCE += monsters.cpp monType.cpp mordorMain.cpp movement.cpp mudObject.cpp necromancy.cpp objIncrease.cpp 
GENERAL_SOURCE += object.cpp objects.cpp objectAttr.cpp oldQuest.cpp pets.cpp player.cpp player2.cpp 
GENERAL_SOURCE += players.cpp playerClass.cpp playerTitle.cpp post.cpp prefs.cpp property.cpp quests.cpp queue.cpp 
GENERAL_SOURCE += raceData.cpp range.cpp realms.cpp rogues.cpp room.cpp rooms.cpp sql.cpp security.cpp server.cpp   
GENERAL_SOURCE += ships.cpp singers.cpp size.cpp skills.cpp skillGain.cpp socket.cpp songs.cpp socials.cpp
GENERAL_SOURCE += specials.cpp special1.cpp spelling.cpp spells.cpp staff.cpp startlocs.cpp stats.cpp statistics.cpp
GENERAL_SOURCE += skillCommand.cpp steal.cpp swap.cpp talk.cpp threat.cpp serverTimer.cpp track.cpp translocation.cpp
GENERAL_SOURCE += transmutation.cpp traps.cpp undead.cpp unique.cpp update.cpp vprint.cpp wanderInfo.cpp warriors.cpp
GENERAL_SOURCE += watchers.cpp weaponless.cpp web.cpp xml.cpp 

REALMS_SOURCE  := main.cpp

ALL_SOURCE     := $(GENERAL_SOURCE) $(REALMS_SOURCE)

GENERAL_OBJ    := $(notdir $(GENERAL_SOURCE:%.cpp=%.o))
REALMS_OBJ	   := $(GENERAL_OBJ) $(notdir $(REALMS_SOURCE:%.cpp=%.o))

ALLOBJ         := $(REALMS_OBJ)

BINS := realms.exe list.exe convert.exe

default:
	@echo 'Using $(CC) as a compiler' 
	$(MAKE) all

DEPS := $(notdir $(ALL_SOURCE:%.cpp=%.d)) mud.h.d
-include $(DEPS)

all: realms.exe 
		
#all: $(BINS)

copy:
	@echo 'Copying binary to bin dir.'
	@cp -f realms.exe ../bin/
	
clean:
	@echo 'Removing all binaries, dependancies, and object files.'
	@rm -f $(ALLOBJ)
	@rm -f $(BINS)
	@rm -f $(DEPS)
	@rm -f *.pchi
	@rm -f *.gch
	@rm -f *.d
	@rm -f *~

mud.h.gch: mud.h
	@echo 'Precompiling mud.h'
	@g++ -x c++-header mud.h -o mud.h.gch $(CFLAGS) && \
	g++ -MM -MG -MT mud.h.gch $(CFLAGS) mud.h > mud.h.d

convert.exe: $(CONVERT_OBJ)
	@echo 'Building target: $@'
	@$(CC) -o $@ $(CFLAGS) $(CONVERT_OBJ) $(LIBS) 
	@echo 'Done making: $@'
	
list.exe: $(LIST_OBJ)
	@echo 'Building target: $@'
	@$(CC) -o $@ $(CFLAGS) $(LIST_OBJ) $(LIBS) 
	@echo 'Done making: $@'
	
realms.exe: $(PRE_COMPILED_HEADER) $(REALMS_OBJ)
	@echo 'Building target: $@'
	@$(CC) -o $@ $(CFLAGS) $(REALMS_OBJ) $(LIBS) 
	@echo 'Done making: $@'

threat.exe:  $(PRE_COMPILED_HEADER) $(THREAT_OBJ)
	@echo 'Building target: $@'
	@$(CC) -o $@ $(CFLAGS) $(THREAT_OBJ) $(LIBS)  
	@echo 'Done making: $@'

compiler:
	@cp compiler.default compiler
	
# General compilation commands
%.o : %.cpp
	@echo -e 'Compiling $<'
	@$(CC) $(CFLAGS) $(ICCFLAGS) -c $< -o$@ && \
	g++ -MM -MG $(CFLAGS) $< > $(@:%.o=%.d)
