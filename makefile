LANG=C
# Moved compiler logic to compiler file, will commit a base of it and then add to svn ignore
# so we can modify the makefile and not worry about changing g++/icc
include compiler

PROGRAM = realms.exe

CFLAGS := -g -Wall -I/usr/include/libxml2 $(COMPILER_CFLAGS) -std=c++11 -I.
CFLAGS += -I/usr/include/python3.2 


LIBS = -laspell -lxml2 -lz -lc -L./ -lpython3.2mu -lboost_python-py32 $(COMPILER_LIBS)

GENERAL_SOURCE := alphanum.cpp pythonHandler.cpp abjuration.cpp access.cpp action.cpp afflictions.cpp
GENERAL_SOURCE += alchemy.cpp alignment.cpp anchor.cpp area.cpp attack.cpp asynch.cpp timer.cpp bank.cpp
GENERAL_SOURCE += bans.cpp builder.cpp calendar.cpp carry.cpp catRef.cpp catRefInfo.cpp clans.cpp color.cpp
GENERAL_SOURCE += combat.cpp combatSystem.cpp command1.cpp command2.cpp command4.cpp command5.cpp container.cpp
GENERAL_SOURCE += commerce.cpp communication.cpp config.cpp conjuration.cpp cmd.cpp creatureStreams.cpp craft.cpp 
GENERAL_SOURCE += creature.cpp creature2.cpp creatures.cpp creatureAttr.cpp damage.cpp data.cpp delayedAction.cpp
GENERAL_SOURCE += demographics.cpp deityData.cpp die.cpp dice.cpp divination.cpp dm.cpp dmcrt.cpp dmobj.cpp dmply.cpp 
GENERAL_SOURCE += dmroom.cpp duel.cpp effects.cpp enchantment.cpp equipment.cpp errors.cpp evocation.cpp
GENERAL_SOURCE += exits.cpp factions.cpp files-xml-read.cpp fighters.cpp files-xml-save.cpp
GENERAL_SOURCE += fishing.cpp flags.cpp global.cpp gods.cpp group.cpp groups.cpp guilds.cpp healers.cpp healmagic.cpp 
GENERAL_SOURCE += help.cpp hooks.cpp illusion.cpp io.cpp import.cpp levelGain.cpp location.cpp log.cpp logic.cpp login.cpp 
GENERAL_SOURCE += lottery.cpp magic.cpp magic1.cpp mccp.cpp md5.cpp memory.cpp misc.cpp money.cpp mxp.cpp msdp.cpp 
GENERAL_SOURCE += monsters.cpp monType.cpp mordorMain.cpp movement.cpp mudObject.cpp necromancy.cpp objIncrease.cpp 
GENERAL_SOURCE += object.cpp objects.cpp objectAttr.cpp oldQuest.cpp pets.cpp player.cpp player2.cpp proxy.cpp
GENERAL_SOURCE += players.cpp playerClass.cpp playerTitle.cpp post.cpp prefs.cpp property.cpp quests.cpp queue.cpp 
GENERAL_SOURCE += raceData.cpp range.cpp realms.cpp rogues.cpp room.cpp rooms.cpp sql.cpp security.cpp server.cpp   
GENERAL_SOURCE += ships.cpp singers.cpp size.cpp skills.cpp skillGain.cpp socket.cpp songs.cpp socials.cpp
GENERAL_SOURCE += specials.cpp special1.cpp spelling.cpp spells.cpp staff.cpp startlocs.cpp stats.cpp statistics.cpp
GENERAL_SOURCE += skillCommand.cpp steal.cpp swap.cpp talk.cpp threat.cpp serverTimer.cpp track.cpp translocation.cpp
GENERAL_SOURCE += transmutation.cpp traps.cpp undead.cpp unique.cpp update.cpp vprint.cpp wanderInfo.cpp warriors.cpp
GENERAL_SOURCE += watchers.cpp weaponless.cpp web.cpp xml.cpp 

REALMS_SOURCE  := main.cpp

ALL_SOURCE     := $(GENERAL_SOURCE) $(REALMS_SOURCE)

DEPSDIR = .deps/$(PROGRAM)
OBJSDIR = .objs/$(PROGRAM)

GENERAL_OBJ    := $(GENERAL_SOURCE:%.cpp=$(OBJSDIR)/%.o)
REALMS_OBJ	   := $(GENERAL_OBJ) $(REALMS_SOURCE:%.cpp=$(OBJSDIR)/%.o)

ALLOBJ         := $(REALMS_OBJ)

DEPSPRECOMP = $(PRE_COMPILED_HEADER:%.h=$(DEPSDIR)/%.d)
OBJSPRECOMP = $(PRE_COMPILED_HEADER:%.h=%.h.gch)

BINS := realms.exe


default:
	@echo 'Using $(CC) as a compiler' 
	$(MAKE) all


DEPS = $(ALL_SOURCE:%.cpp=$(DEPSDIR)/%.d)

#Linkage
$(PROGRAM): $(DEPSPRECOMP) $(DEPS) $(OBJSPRECOMP) $(REALMS_OBJ)
	@echo 'Building target: $@'
	@$(CC) -o $@ $(CFLAGS) $(REALMS_OBJ) $(LIBS) 
	@echo 'Done making: $@'
	
#cleaning
.PHONY: clean
clean:
	@echo 'Removing all binaries, dependancies, and object files.'
	@rm -rf $(OBJSDIR)
	@rm -f $(BINS)
	@rm -rf $(DEPSDIR)
	@rm -f *.pchi
	@rm -f *.gch
	@rm -f *.d
	@rm -f *~
	
	
#creating the dependencies of the %.c files
$(DEPSDIR)/%.d: %.cpp
	@echo "Making dependencies for $<"
	@dirname $@ | xargs mkdir -p 2>/dev/null || echo "$@ already exists" >/dev/null
	@$(CC) -MM $(CFLAGS) $< 2>/dev/null | sed 's#.*:# $@ :#1' > $@
	
#creating the dependencies of the %.h files, for the precompiled header
$(DEPSDIR)/%.d: %.h
	@echo "Making dependencies for precompiled header $<"
	@dirname $@ | xargs mkdir -p 2>/dev/null || echo "$@ already exists" >/dev/null
	@$(CC) -MM $(CFLAGS) $< 2>/dev/null | sed 's#.*:# $@ :#1' > $@
	
#compiling the precompiled header
%.h.gch:%.h $(DEPSPRECOMP)
	@echo "Precompiling header $@..."
	@$(CC) $(CFLAGS) -o $@ -c $< || echo "error. Disabling precompiled header"
	@echo "...Done"
	
#compiling source files
$(OBJSDIR)/%.o: %.cpp $(DEPSDIR)/%.d $(OBJSPRECOMP)
	@echo -e 'Compiling $<'
	@dirname $@ | xargs mkdir -p 2>/dev/null || echo "$@ already exists" >/dev/null
	@$(CC) $(CFLAGS) $(ICCFLAGS) -c $< -o $@
	
all: $(PROGRAM)
		
#all: $(BINS)

copy:
	@echo 'Copying binary to bin dir.'
	@cp -f realms.exe ../bin/
	
compiler:
	@cp compiler.default compiler

#include the dependencies, (if we are not actually performing a mere <make clean>)
ifneq ($(strip $(MAKECMDGOALS)),clean)
ifneq ($(strip $(DEPSPRECOMP)),)
-include $(DEPSPRECOMP)
endif
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
endif
	