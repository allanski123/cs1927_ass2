// GameView.c ... GameView ADT implementation

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Globals.h"
#include "Game.h"
#include "GameView.h"
#include "Map.h"

#define NUM_HUNTERS          4
#define MAX_ENCOUNTERS       4
#define PLAY_STRING_LENGTH   7

typedef struct Hunter {
    PlayerID ID;
    int health;
    int deaths;
    LocationID location;
} Hunter;

typedef struct Dracula {
    int health;
    LocationID location;
    LocationID trail[TRAIL_SIZE];
} Dracula;

struct gameView {
    Map map;
    int score;
    Round round;
    Hunter hunters[NUM_HUNTERS];
    Dracula dracula;
    char *playRecord;
    int curr; //current player
    //record traps and vampires in which locations...
};

//static functions
static connectionList getUniqueLocations(connectionList list, LocationID origin, PlayerID player);
static connectionList mergeConnectionLists(connectionList oldList, connectionList newList);
static int hospitalVisits(GameView currentView, PlayerID player);
static int numMaturedVampires(GameView currentView);
static int trapsEncountered(GameView currentView, PlayerID player);
static int timesDraculaEncountered(GameView currentView, PlayerID player);
static int turnsHunterRested(GameView currentView, PlayerID player);
static int turnsDracAtSea(GameView currentView);
static int turnsDracAtCastleDrac(GameView currentView);

// Creates a new GameView to summarise the current state of the game
GameView newGameView(char *pastPlays, PlayerMessage messages[])
{
    GameView gameView = malloc(sizeof(struct gameView));
    gameView->map = newMap();
    gameView->playRecord = malloc(sizeof(char)*strlen(pastPlays));
    strcpy(gameView->playRecord, pastPlays);
    gameView->round = getRound(gameView);
    gameView->curr = getCurrentPlayer(gameView);
    gameView->score = getScore(gameView);

    //Initialize hunters
    int i;
    for(i = 0; i < NUM_HUNTERS; i++)
    {
        gameView->hunters[i].ID = i;
        gameView->hunters[i].health = getHealth(gameView, i);
        gameView->hunters[i].deaths = hospitalVisits(gameView, i);
    }

    //Initialize Dracula
    gameView->dracula.health = getHealth(gameView, PLAYER_DRACULA);

    return gameView;
}


// Frees all memory previously allocated for the GameView toBeDeleted
void disposeGameView(GameView toBeDeleted)
{
    disposeMap(toBeDeleted->map);
    free(toBeDeleted);
}

//// Functions to return simple information about the current state of the game

// Get the current round
Round getRound(GameView currentView)
{
    Round numRounds;
    int playStringLength = (int)strlen(currentView->playRecord);
    numRounds = (playStringLength+1)/((PLAY_STRING_LENGTH+1)*NUM_PLAYERS);
    return numRounds;
}

// Get the id of current player - ie whose turn is it?
PlayerID getCurrentPlayer(GameView currentView)
{
    char lastPlayerChar;
    PlayerID lastPlayer=-1;
    PlayerID thisPlayer;
    char playerIdentifier[NUM_PLAYERS] = {'G', 'S', 'H', 'M', 'D'};
    lastPlayerChar = currentView->playRecord[strlen(currentView->playRecord)-PLAY_STRING_LENGTH];
    int index;
    for (index = 0; index < NUM_PLAYERS; index++){
        if (lastPlayerChar == playerIdentifier[index]){
            lastPlayer = index;
        }
    }
    if (lastPlayer == PLAYER_DRACULA || strlen(currentView->playRecord)==0){
        thisPlayer = PLAYER_LORD_GODALMING;
    } else {
        thisPlayer = lastPlayer + 1;
    }

    return thisPlayer;
}

// Get the current score
int getScore(GameView currentView)
{
    int score = GAME_START_SCORE;
    //minus 1 for each round completed
    score -= getRound(currentView);
    //minus 6 for each hospital visit
    PlayerID player;
    for (player=0; player<NUM_HUNTERS; player++){
        score -= 6*hospitalVisits(currentView, player);
    }
    //minus 13 for each vampire fallen off trail (matured)
    score -= 13*numMaturedVampires(currentView);
    return score;
}

// Get the current health points for a given player
int getHealth(GameView currentView, PlayerID player)
{
    int health = 0;
    if (player == PLAYER_DRACULA){
        health += 40;
        //loses 10 when encountered a hunter
        PlayerID player;
        for (player = 0; player < NUM_HUNTERS; player++){
            health -= 10*timesDraculaEncountered(currentView, player);
        }
        //loses 2 each turn at sea
        health -= 2*turnsDracAtSea(currentView);
        //gains 10 if in Castle Dracula at end of turn and not yet defeated
        health += 10*turnsDracAtCastleDrac(currentView);

    } else {
        health += 9;
        //loses 2 when encounters a trap
        health -= 2*trapsEncountered(currentView, player);
        //loses 4 when encounters dracula
        health -= 4*timesDraculaEncountered(currentView, player);
        //gains 3 when rest in a city (in same city for 2 turns)
        health += 3*turnsHunterRested(currentView, player);
        //cannot exceed 9 points.
        if (health > 9){
            health = 9;
        }
    }
    return health;
}

// Get the current location id of a given player
LocationID getLocation(GameView currentView, PlayerID player)
{
    LocationID playerTrail[TRAIL_SIZE];
    getHistory(currentView, player, playerTrail);
    LocationID playerLocation = playerTrail[0];
    return playerLocation;
}

//// Functions that return information about the history of the game

// Fills the trail array with the location ids of the last 6 turns
void getHistory(GameView currentView, PlayerID player,
                LocationID trail[TRAIL_SIZE])
{
    //take in string of moves from currentView
    char playerIdentifier[NUM_PLAYERS] = {'G', 'S', 'H', 'M', 'D'};
    char thisPlayer = playerIdentifier[player];
    int trailCounter;

    //start all trail locations as unknown
    for (trailCounter = 0; trailCounter < TRAIL_SIZE; trailCounter ++){
        trail[trailCounter] = UNKNOWN_LOCATION;
    }
    trailCounter = 0;
    //find past 6 trail locations of character (backwards from end of string)
    //assuming a string from currentView called playRecord
    int thisIndex;
    int stringLength = (int) strlen(currentView->playRecord);
    LocationID location;
    char locationAbbrev[2];
    for (thisIndex = stringLength-PLAY_STRING_LENGTH; thisIndex >= 0 && trailCounter < TRAIL_SIZE; thisIndex -= (PLAY_STRING_LENGTH+1)){
        if (currentView->playRecord[thisIndex] == thisPlayer){
            locationAbbrev[0] = currentView->playRecord[thisIndex+1];
            locationAbbrev[1] = currentView->playRecord[thisIndex+2];
            location = abbrevToID(locationAbbrev);
            if (location != NOWHERE){
                trail[trailCounter] = location;
            } else {   //Special Cases for Dracula's Moves!
                if (strncmp(locationAbbrev, "C?", 2)==0){
                    trail[trailCounter] = CITY_UNKNOWN;
                } else if (strncmp(locationAbbrev, "S?", 2)==0){
                    trail[trailCounter] = SEA_UNKNOWN;
                } else if (strncmp(locationAbbrev, "HI", 2)==0){
                    trail[trailCounter] = HIDE;
                } else if (strncmp(locationAbbrev, "TP", 2)==0){
                    trail[trailCounter] = TELEPORT;
                } else if (locationAbbrev[0]== 'D' && locationAbbrev[1]>='0' && locationAbbrev[1]<('0' + TRAIL_SIZE)){
                    int doubleBacks[5] = {DOUBLE_BACK_1, DOUBLE_BACK_2, DOUBLE_BACK_3, DOUBLE_BACK_4, DOUBLE_BACK_5};
                    int doubleBackNum = locationAbbrev[1] - '1';
                    trail[trailCounter] = doubleBacks[doubleBackNum];
                }
            }
            trailCounter ++;
        }
    }

}

//// Functions that query the map to find information about connectivity

// Returns an array of LocationIDs for all directly connected locations

LocationID *connectedLocations(GameView currentView, int *numLocations,
                               LocationID from, PlayerID player, Round round,
                               int road, int rail, int sea)
{
    //find max rail connections allowed
    int maxRailConnections = 0;

    if (player != PLAYER_DRACULA){
        maxRailConnections = (player + getRound(currentView)) % 4;
    }

    connectionList roadConnections;
    connectionList seaConnections;
    connectionList railConnections;
    //find connections
    if (road == TRUE) {
        roadConnections = getConnections(currentView->map, from, ROAD);
    } else {
        roadConnections.numConnections = 0;
        roadConnections.connections = NULL;
    }
    if (sea == TRUE) {
        seaConnections = getConnections(currentView->map, from, BOAT);
    } else {
        seaConnections.numConnections = 0;
        seaConnections.connections = NULL;
    }
    if (rail == TRUE && maxRailConnections > 0){
        railConnections = getConnections(currentView->map, from, RAIL);
        if (maxRailConnections > 1){
            //add more rails!!
            int railIndex;
            int numRailConnections = railConnections.numConnections;
            for (railIndex=0; railIndex<numRailConnections; railIndex++){
                LocationID fromRail = railConnections.connections[railIndex];
                connectionList newRailConnections = getConnections(currentView->map, fromRail, RAIL);
                railConnections = mergeConnectionLists(railConnections, newRailConnections);
            }
        }
        if (maxRailConnections > 2){
            //add more rails!!
            int railIndex;
            int numRailConnections = railConnections.numConnections;
            for (railIndex=0; railIndex<numRailConnections; railIndex++){
                LocationID fromRail = railConnections.connections[railIndex];
                connectionList newRailConnections = getConnections(currentView->map, fromRail, RAIL);
                railConnections = mergeConnectionLists(railConnections, newRailConnections);
            }
        }
    }  else {
        railConnections.numConnections = 0;
        railConnections.connections = NULL;
    }

    //reduce this list to unique locations :D need to remove Paris from this list...
    railConnections = getUniqueLocations(railConnections, from, player);

    //get unique locations array, remove any travel back to same, update numConnections...
    connectionList uniqueLocations = mergeConnectionLists(roadConnections, seaConnections);
    uniqueLocations = mergeConnectionLists(uniqueLocations, railConnections);
    uniqueLocations = getUniqueLocations(uniqueLocations, from, player);

    int totalConnections = uniqueLocations.numConnections;
    *numLocations = totalConnections;

    LocationID *locations = malloc(sizeof(LocationID)*totalConnections);
    //add all connection details to array.
    int index = 0;
    for (index=0; index < totalConnections; index++){
        locations[index] = uniqueLocations.connections[index];

    }
    return locations;
}

static connectionList getUniqueLocations(connectionList list, LocationID origin, PlayerID player){
    int numUnique = 0;
    int index, location, arrayCount;
    //bit array, TRUE if location connected, 0 if not.
    int uniqueArray[NUM_MAP_LOCATIONS] = {0};
    for (index=0; index<list.numConnections; index++){
        uniqueArray[list.connections[index]] = TRUE;
    }
    //include destination 'from' in array
    uniqueArray[origin] = TRUE;
    //remove hospital for Dracula
    if (player == PLAYER_DRACULA){
        uniqueArray[ST_JOSEPH_AND_ST_MARYS] = FALSE;
    }
    //count unique connections
    for (location=0; location<NUM_MAP_LOCATIONS; location++){
        if (uniqueArray[location] == TRUE){
            numUnique ++;
        }
    }

    //return in connectionList
    connectionList uniqueConnectionList;
    uniqueConnectionList.numConnections = numUnique;
    uniqueConnectionList.connections = malloc(sizeof(LocationID)*numUnique);
    arrayCount = 0;
    for (location=0; location<NUM_MAP_LOCATIONS; location++){
        if (uniqueArray[location] == TRUE){
            uniqueConnectionList.connections[arrayCount] = location;
            arrayCount ++;
        }
    }
    return uniqueConnectionList;
}

static connectionList mergeConnectionLists(connectionList oldList, connectionList newList){
    connectionList newConnectionList;

    newConnectionList.numConnections = oldList.numConnections + newList.numConnections;
    newConnectionList.connections = malloc(sizeof(LocationID)*newConnectionList.numConnections);

    int index=0, oldIndex, newIndex;
    for (oldIndex=0; oldIndex < oldList.numConnections; oldIndex++){
        newConnectionList.connections[index] = oldList.connections[oldIndex];
        index ++;
    }
    for (newIndex=0; newIndex < newList.numConnections; newIndex++){
        newConnectionList.connections[index] = newList.connections[newIndex];
        index ++;
    }

    return newConnectionList;
}

static int hospitalVisits(GameView currentView, PlayerID player){
    int hospitalStays = 0;
    int index;
    char locationAbbrev[2];
    for (index = (int)player * (PLAY_STRING_LENGTH+1);
         index < strlen(currentView->playRecord); index += (PLAY_STRING_LENGTH+1)*NUM_PLAYERS){
        locationAbbrev[0] = currentView->playRecord[index+1];
        locationAbbrev[1] = currentView->playRecord[index+2];
        LocationID location = abbrevToID(locationAbbrev);
        if (location == ST_JOSEPH_AND_ST_MARYS){
            hospitalStays ++;
        }
    }
    return hospitalStays;
}

static int numMaturedVampires(GameView currentView){
    // [player, loc, loc, trap, immature, trap/vampire matures, ., space]
    //mature vamp if 'V' in 5 spots after Dracula's ID.
    int numMatured = 0;
    int index;
    for (index = PLAYER_DRACULA*(PLAY_STRING_LENGTH+1);
         index < strlen(currentView->playRecord); index += ((PLAY_STRING_LENGTH+1)*NUM_PLAYERS)){
        if (currentView->playRecord[index+5]=='V'){
            numMatured ++;
        }
    }
    return numMatured;
}

static int trapsEncountered (GameView currentView, PlayerID player){
    // [player, loc, loc, encounter, encounter, encounter, encounter, space]
    int numTraps = 0;
    int index;
    for (index = (int)player * (PLAY_STRING_LENGTH+1);
         index < strlen(currentView->playRecord); index += ((PLAY_STRING_LENGTH+1)*NUM_PLAYERS)){
        int offset;
        for (offset = 0; offset<MAX_ENCOUNTERS; offset ++){
            if (currentView->playRecord[index + 3 + offset] == 'T') {
                numTraps ++;
            }
        }
    }
    return numTraps;
}

static int timesDraculaEncountered (GameView currentView, PlayerID player){
    // [player, loc, loc, encounter, encounter, encounter, encounter, space]
    int numDracula = 0;
    int index;
    for (index = (int)player * (PLAY_STRING_LENGTH+1);
         index < strlen(currentView->playRecord); index += ((PLAY_STRING_LENGTH+1)*NUM_PLAYERS)){
        int offset;
        for (offset = 0; offset<MAX_ENCOUNTERS; offset ++){
            if (currentView->playRecord[index + 3 + offset] == 'D') {
                numDracula ++;
            }
        }
    }
    return numDracula;
}

static int turnsHunterRested(GameView currentView, PlayerID player){
    int timesRested = 0;
    int index = (int)player * (PLAY_STRING_LENGTH+1);
    char locationAbbrev[2];
    LocationID lastLocation;
    //get the first location
    locationAbbrev[0] = currentView->playRecord[index+1];
    locationAbbrev[1] = currentView->playRecord[index+2];
    lastLocation = abbrevToID(locationAbbrev);
    //get the next location and compare
    for (index = (index + (PLAY_STRING_LENGTH+1)*NUM_PLAYERS);
         index < strlen(currentView->playRecord); index += (PLAY_STRING_LENGTH+1)*NUM_PLAYERS){
        locationAbbrev[0] = currentView->playRecord[index+1];
        locationAbbrev[1] = currentView->playRecord[index+2];
        LocationID location = abbrevToID(locationAbbrev);
        if (location == lastLocation){
            timesRested ++;
        }
        lastLocation = location;
    }
    return timesRested;
}

static int turnsDracAtSea(GameView currentView){
    int turnsAtSea = 0;
    int index;
    char locationAbbrev[2];
    for (index = PLAYER_DRACULA * (PLAY_STRING_LENGTH+1);
         index < strlen(currentView->playRecord); index += (PLAY_STRING_LENGTH+1)*NUM_PLAYERS){
        locationAbbrev[0] = currentView->playRecord[index+1];
        locationAbbrev[1] = currentView->playRecord[index+2];
        LocationID location = abbrevToID(locationAbbrev);
        //if doubled back
        if (locationAbbrev[0]== 'D' && locationAbbrev[1]>='0' && locationAbbrev[1]<('0' + TRAIL_SIZE)){
            int movesBack = locationAbbrev[1] - '0';
            int moveIndex = index - (PLAY_STRING_LENGTH+1)*NUM_PLAYERS*movesBack;
            locationAbbrev[0] = currentView->playRecord[moveIndex+1];
            locationAbbrev[1] = currentView->playRecord[moveIndex+2];
            location = abbrevToID(locationAbbrev);
        }
        //if turn was at sea
        if (location != NOWHERE && idToType(location) == SEA){
            turnsAtSea ++;
        } else if (strncmp(locationAbbrev, "S?", 2) == 0){
            turnsAtSea ++;
        }
    }
    return turnsAtSea;
}

static int turnsDracAtCastleDrac(GameView currentView){
    int castleDracStays = 0;
    int index;
    char locationAbbrev[2];
    for (index = PLAYER_DRACULA * (PLAY_STRING_LENGTH+1);
         index < strlen(currentView->playRecord); index += (PLAY_STRING_LENGTH+1)*NUM_PLAYERS){
        locationAbbrev[0] = currentView->playRecord[index+1];
        locationAbbrev[1] = currentView->playRecord[index+2];
        LocationID location = abbrevToID(locationAbbrev);
        if (location == CASTLE_DRACULA){
            castleDracStays ++;
        }
    }
    return castleDracStays;
}
