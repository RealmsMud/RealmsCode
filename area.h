/*
 * area.h
 *	 Header file for area code.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/

 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef AREA_H
#define AREA_H

#define RADIAN 57.2957795

// max vision is defined because of the output functions - if it's any higher
// than this, some of the area grid gets truncated
#define MAX_VISION		18
// we don't let too many AreaTrack objects hang around
#define MAX_AREA_TRACK	100

//#include <list>
//#include <vector>
#include <map>
// forward declaration
class Area;
class AreaData;
class AreaRoom;
class Player;



class MapMarker {
public:
	MapMarker();
	MapMarker& operator=(const MapMarker& m);
	bool	operator==(const MapMarker& m) const;
	bool	operator!=(const MapMarker& m) const;

	void	save(xmlNodePtr curNode) const;
	void	load(xmlNodePtr curNode);
	void	load(bstring str);
	void	reset();
	bstring str(bool color=false) const;
	bstring direction(const MapMarker *mapmarker) const;
	bstring distance(const MapMarker *mapmarker) const;

	bstring filename() const;

	short	getArea() const;
	short	getX() const;
	short	getY() const;
	short	getZ() const;

	void	setArea(short n);
	void	setX(short n);
	void	setY(short n);
	void	setZ(short n);

	void	set(short _area, short _x, short _y, short _z);
	void	add(short _x, short _y, short _z);

protected:
	short	area;
	short	x;
	short	y;
	short	z;
};



class AreaTrack {
public:
	AreaTrack();

	MapMarker	mapmarker;
	Track		track;

	int getDuration() const;
	void setDuration(int dur);

protected:
	int		duration;
};


class AreaZone {
public:
	AreaZone();
	~AreaZone();

	bstring getFishing() const;
	bool 	inside(const Area *area, const MapMarker *mapmarker) const;
	bool 	inRestrict(char tile, const char *list) const;
	void	load(xmlNodePtr curNode);
	void	save(xmlNodePtr curNode) const;
	bool	swap(Swap s);

	bool flagIsSet(int flag) const;


	bstring name;			// for staff identification
	bstring display;		// displayed to player in room description

	char	terRestrict[10];
	char	mapRestrict[10];

	WanderInfo wander;		// Random monster info
	CatRef	unique;			// does this zone lead to a unique room

	char	flags[16];

	MapMarker	min;
	MapMarker	max;
	std::map<int, MapMarker*> coords;

protected:
	bstring fishing;
};

class TileInfo {
public:
	TileInfo();

	void	load(xmlNodePtr curNode);
	void	save(xmlNodePtr curNode) const;
	char	getStyle(const Player* player=0) const;

	char	getId() const;
	bstring getName() const;
	bstring getDescription() const;
	short	getCost() const;
	float	getVision() const;
	char	getDisplay() const;
	short	getTrackDur() const;
	bool	flagIsSet(int flag) const;

	bool	isWater() const;
	bool	isRoad() const;
	short	getFly() const;
	bool	spawnHerbs(BaseRoom* room) const;
	bstring getFishing() const;

	WanderInfo wander;		// Random monster info
	std::map<Season,char> season;
protected:
	char	id;
	bstring name;
	bstring description;
	bstring fishing;
	short	cost;
	float	vision;
	char	style;
	char	display;
	short	trackDur;		// duration of tracks in game minutes
	char	flags[16];

	bool	water;
	bool	road;
	short	fly;

	std::list<CatRef> herbs; // searchable disposable herbs
};



class AreaData {
public:
	AreaData();

	char get(short x, short y, short z) const;
	std::vector< std::vector< std::vector<char> > > data;

	void setArea(Area* a);
	void setTerrain(bool t);
protected:
	Area	*area;
	bool	isTerrain;
};


class Area {
protected:
	//void	move(ctag *fp, AreaRoom* room);

public:
	Area();
	~Area();


	CatRef	getUnique(const MapMarker *mapmarker) const;
	bool	move(Player* player, MapMarker *mapmarker);
	void	remove(AreaRoom* room);
	AreaRoom *getRoom(const MapMarker *mapmarker);
	AreaRoom *loadRoom(const Creature* creature, const MapMarker* mapmarker, bool recycle, bool p=false);
	void	checkCycle(MapMarker *mapmarker) const;
	short	checkCycle(short vector, short critical) const;
	bool	canPass(const Creature* creature, const MapMarker *mapmarker, bool adjust=false) const;
	bool	isRoad(short x, short y, short z, bool adjust=false) const;
	bool	isWater(short x, short y, short z, bool adjust=false) const;
	TileInfo *getTile(char grid, char seasonFlags, Season season=NO_SEASON, bool checkSeason=true) const;
	char	getTerrain(const Player* player, const MapMarker *mapmarker, short y, short x, short z, bool terOnly) const;
	char	getSeasonFlags(const MapMarker *mapmarker, short y=0, short x=0, short z=0) const;
	float	getLosPower(const Player* player, int xVision, int yVision) const;
	void	getGridText(char grid[][80], int height, const MapMarker *mapmarker, int maxWidth) const;
	bstring showGrid(const Player* player, const MapMarker *mapmarker, bool compass) const;
	bool	outOfBounds(short x, short y, short z) const;
	void	adjustCoords(short* x, short* y, short* z) const;
	void	cleanUpRooms();

	Track*	getTrack(MapMarker* mapmarker) const;
	void	addTrack(AreaTrack *aTrack);
	int		getTrackDuration(const MapMarker* mapmarker) const;
	void	updateTrack(int t);
	bool	swap(Swap s);

	void	load(xmlNodePtr curNode);
	void	loadZones(xmlNodePtr curNode);
	void	loadTerrain(int minDepth);
	void	loadRooms();
	void	loadTiles(xmlNodePtr curNode, bool ter);
	void	save(xmlNodePtr curNode, bool saveRooms) const;
	void	checkFileSize(int& size, const char* filename) const;

	bool	isSunlight(const MapMarker* mapmarker) const;
	bool	flagIsSet(int flag, const MapMarker* mapmarker) const;

	// line of sight functions
	void	losCloser(int *x, int *y, int me_x, int me_y, int i) const;
	float	lineOfSight(float *grid, const Player* player, int width, int *y, int *x, int me_y, int me_x, int *i, const MapMarker *mapmarker) const;
	void	makeLosGrid(float *grid, const Player* player, int height, int width, const MapMarker *mapmarker) const;

public:
	short	id;
	bstring name;

	// stuff we will need
	//	terrain grid
	//	data for each terrain spot
	//	zone-specific data

	// if the point goes over this coord, it is cycled to the other
	// side of the world. set to 0 if no auto-cycling
	short		critical_x;
	short		critical_y;
	short		critical_z;

	// the point 0,0,0 will change if we ever modify the dimensions of the
	// map. thus, a special map marker '*' marks 0,0,0 for us. we are free to edit
	// map dimensions and keep the same coordinates for all rooms
	short		zero_offset_x;
	short		zero_offset_y;
	short		zero_offset_z;

	short		width;
	short		height;
	short		depth;

	char		dataFile[20];

	char		defaultTerrain;
	AreaData	aTerrain;
	AreaData	aMap;
	AreaData	aSeason;

	// how much flying helps vision
	short		flightPower;

	std::map<bstring, AreaRoom*> rooms;
	std::list<AreaZone*> zones;
	std::list<AreaTrack*> tracks;

	std::map<char, TileInfo*> ter_tiles;
	std::map<char, TileInfo*> map_tiles;
protected:
	int minDepth;
};


#endif
