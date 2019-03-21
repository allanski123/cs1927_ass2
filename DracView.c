// DracView.c ... DracView ADT implementation

#include <stdlib.h>
#include <assert.h>
#include "Globals.h"
#include "Game.h"
#include "GameView.h"
#include "DracView.h"
#include <string.h>

#include <stdio.h>
// #include "Map.h" ... if you decide to use the Map ADT
     
struct dracView {
    GameView view;
    LocationID trail[TRAIL_SIZE];
    LocationID vampID;
    int trapCount[NUM_MAP_LOCATIONS];
};

void updateArray(LocationID *array, int i, int *arrayNum);
void addToTrail(DracView view, LocationID p);

// Creates a new DracView to summarise the current state of the game
DracView newDracView(char *pastPlays, PlayerMessage messages[])
{
    DracView dracView = malloc(sizeof(struct dracView));
    dracView->view = newGameView(pastPlays, messages);

    int i;
    for(i = 0; i < TRAIL_SIZE; i++){
        dracView->trail[i] = UNKNOWN_LOCATION;
    }

    dracView->vampID = NOWHERE;

    for(i=0; i < NUM_MAP_LOCATIONS; i++){
        dracView->trapCount[i] = 0;
    }

    // if pastPlays is empty - this means we can simply return dracview
    if(!strcmp(pastPlays,"\0")){
        return dracView;
    }

    // if not empty - we need to go through pastPlays
    // and backtrack to update the game state accordingly (trap numbers etc)
    int playHistLength = strlen(pastPlays);
    //int count=0;
    char playerLoc[3];
    LocationID p;
    for(i = 0; i < playHistLength; i += 8){
        playerLoc[0] = pastPlays[i+1];
        playerLoc[1] = pastPlays[i+2];
        playerLoc[2] = '\0';
        
        p = abbrevToID(playerLoc);
        // if dracula, else must be hunter
        // check the pastplay string and update trap numbers and vamp location
        // also update dracula's trail 
        if(pastPlays[i] == 'D'){
            // update location (if hide, doubleBack or Teleport)
            if(pastPlays[i+1] == 'H' && pastPlays[i+2] == 'I')
                p = dracView->trail[0];
            if(pastPlays[i+1] == 'D'){
                int index = (int)(pastPlays[i+2]-48); 
                if(index == 1) p = DOUBLE_BACK_1;
                if(index == 2) p = DOUBLE_BACK_2;
                if(index == 3) p = DOUBLE_BACK_3;
                if(index == 4) p = DOUBLE_BACK_4;
                if(index == 5) p = DOUBLE_BACK_5;
            }
            if(pastPlays[i+1] == 'T' && pastPlays[i+2] == 'P')
                p = CASTLE_DRACULA;

            // update traps
            if(pastPlays[i+3] == 'T')
                dracView->trapCount[p]++;
            if(pastPlays[i+4] == 'V')
                dracView->vampID = p;
            if(pastPlays[i+5] == 'M')
                dracView->trapCount[p]--;
            if(pastPlays[i+5] == 'V')
                dracView->vampID = NOWHERE;

            //printf("pushing now! !\n");
            //printf("Expected: \n%d, %s\n",p,idToName(p));
            addToTrail(dracView, p);

            //printf("TRAIL:");
         //   for(count = 0; count<TRAIL_SIZE; count++)
           //     printf("\n%d, %s\n",dracView->trail[count],idToName(dracView->trail[count]));

        } else {
            // can potentially run into 3 traps at max
            int j;
            for(j = 3; j < 6; j++){
                if(pastPlays[i+j] == 'T')
                    dracView->trapCount[p]--;
                else if(pastPlays[i+j] == 'V')
                    dracView->vampID = NOWHERE;
            }
        }
    }
    return dracView;
}
     
// Frees all memory previously allocated for the DracView toBeDeleted
void disposeDracView(DracView toBeDeleted)
{
    disposeGameView(toBeDeleted->view);
    free(toBeDeleted);
}

//// Functions to return simple information about the current state of the game

// Get the current round
Round giveMeTheRound(DracView currentView)
{
    return getRound(currentView->view);
}

// Get the current score
int giveMeTheScore(DracView currentView)
{
    return getScore(currentView->view);
}

// Get the current health points for a given player
int howHealthyIs(DracView currentView, PlayerID player)
{
    return getHealth(currentView->view, player);
}

// Get the current location id of a given player
LocationID whereIs(DracView currentView, PlayerID player)
{
    if(player == PLAYER_DRACULA)
        return currentView->trail[0];
    //printf("\nHE IS HERE %s\n",idToName(getLocation(currentView->view,player)));
    return getLocation(currentView->view, player);
}

// Get the most recent move of a given player
void lastMove(DracView currentView, PlayerID player,
                 LocationID *start, LocationID *end)
{
    LocationID playerTrail[TRAIL_SIZE];
    getHistory(currentView->view, player, playerTrail);
    *end = playerTrail[0];
    *start = playerTrail[1];
}

// Find out what minions are placed at the specified location
void whatsThere(DracView currentView, LocationID where,
                         int *numTraps, int *numVamps)
{
    *numTraps = 0;
    *numVamps = 0;

    // if traps are set in valid places 
    if(where != SEA && where != NOWHERE){
        *numTraps = currentView->trapCount[where];
        if(where == currentView->vampID){
            *numVamps = 1;
        }
    }
}

//// Functions that return information about the history of the game

// Fills the trail array with the location ids of the last 6 turns
void giveMeTheTrail(DracView currentView, PlayerID player,
                            LocationID trail[TRAIL_SIZE])
{
    getHistory(currentView->view, player, trail);
    
    int i;
    if(trail == NULL){
        for(i = 0; i < TRAIL_SIZE; i++){
            trail[i] = currentView->trail[i];
        }
    }
}

//// Functions that query the map to find information about connectivity

// What are my (Dracula's) possible next moves (locations)
LocationID *whereCanIgo(DracView currentView, int *numLocations, int road, int sea)
{
    LocationID *possibleMoves;
    LocationID whereAmI = whereIs(currentView, PLAYER_DRACULA);
    Round round = giveMeTheRound(currentView);
    // dracula does not move by rail !
    int rail = 0;

    // all possible moves (including those in the trail - will remove them soon!)
    possibleMoves = connectedLocations(currentView->view, numLocations, 
                        whereAmI, PLAYER_DRACULA, round, road, rail, sea);
    // store the locationID's of the last 6 turns in trail array
    getHistory(currentView->view, PLAYER_DRACULA, currentView->trail);

    int alreadyHid = 0;
    int alreadyDoubleB = 0;
    int i,j;
    int highestIndexInTrail = -1;
    // check whether drac has already double backed or already hid (in trail history)
    for(i = 0; i < TRAIL_SIZE; i++){
        if(currentView->trail[i] >= DOUBLE_BACK_1 && currentView->trail[i] <= DOUBLE_BACK_5){
            alreadyDoubleB = 1;
        }
        if(currentView->trail[i] == HIDE){
            alreadyHid = 1;
        }
        if(currentView->trail[i] != -1){
            highestIndexInTrail ++;
        }
    }

    // will remove illegal double backs and hides from trail
    // will remove any location that is in Dracula's trail
    for(i = 0; i < *numLocations; i++){

        // if the current location is on SEA - dracula cannot hide - so remove from array
        // or if he has already hid and the current location is in possible moves array - remove it
        if (possibleMoves[i] == HIDE && (alreadyHid || idToType(whereAmI) == SEA) ){
            updateArray(possibleMoves, i, numLocations);
        }

        // if the location is in both arrays (pos and trail), remove it
        for(j = 0; j < TRAIL_SIZE; j++) {
            if (possibleMoves[i] == currentView->trail[j]) {
                updateArray(possibleMoves, i, numLocations);
            }
        }

        //if already doubled back or can't double back that far, remove it
        if (possibleMoves[i] >= DOUBLE_BACK_1 && possibleMoves[i] <= DOUBLE_BACK_5){
            int doubleBackIndex = possibleMoves[i] - DOUBLE_BACK_1 + 1;
            if (alreadyDoubleB){
                updateArray(possibleMoves, i, numLocations);
            } else if (doubleBackIndex > highestIndexInTrail) {
                updateArray(possibleMoves, i, numLocations);
            }
        }

    }

    /*for(i=0; i<*numLocations; i++){
        printf("\n%s\n",idToName(possibleMoves[i]));
    }*/
    return possibleMoves;
}

// What are the specified player's next possible moves
LocationID *whereCanTheyGo(DracView currentView, int *numLocations,
                           PlayerID player, int road, int rail, int sea)
{
    LocationID whereAmI = whereIs(currentView,player);
    Round round = giveMeTheRound(currentView)+1;
    LocationID *possibleMoves = connectedLocations(currentView->view, 
        numLocations, whereAmI, player, round, road, rail,sea);

    if(player == PLAYER_DRACULA){
        possibleMoves = whereCanIgo(currentView, numLocations, road, sea);
    }

    return possibleMoves;
}

void updateArray(LocationID *array, int i, int *arrayNum)
{

    // remove location from array
    array[i] = NOWHERE;

    int count;
    for(count = i; count < (*arrayNum) - 1; count++){
        array[count] = array[count + 1];
    }
    // chop off last array index (because we have shifted all indexes across)
    // and we now have 1 less index in the array
    (*arrayNum)--;
}

void addToTrail(DracView view, LocationID p)
{
    int i;
    // starting from the end of array - shift each index one across
    for(i = TRAIL_SIZE-1; i >= 1; i--){
        view->trail[i] = view->trail[i-1];
    }
    // after exit - set the current location 
    view->trail[0] = p;
}