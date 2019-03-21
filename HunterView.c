// HunterView.c ... HunterView ADT implementation

#include <stdlib.h>
#include <assert.h>
#include "Globals.h"
#include "Game.h"
#include "GameView.h"
#include "HunterView.h"
#include "Map.h"
     
struct hunterView {
    GameView view;
    LocationID dracTrail[TRAIL_SIZE];
};

#define FIRST_ROUND 0     

// Creates a new HunterView to summarise the current state of the game
HunterView newHunterView(char *pastPlays, PlayerMessage messages[])
{
    assert(pastPlays != NULL);
    assert(messages != NULL);
    HunterView hunterView = malloc(sizeof(struct hunterView));
    hunterView->view = newGameView(pastPlays, messages);

    int i;
    for(i = 0; i < TRAIL_SIZE; i++){
        hunterView->dracTrail[i] = UNKNOWN_LOCATION;
    }

    return hunterView;
}
     
     
// Frees all memory previously allocated for the HunterView toBeDeleted
void disposeHunterView(HunterView toBeDeleted)
{
    assert(toBeDeleted != NULL);
    disposeGameView(toBeDeleted->view);
    free(toBeDeleted);
}


//// Functions to return simple information about the current state of the game

// Get the current round
Round giveMeTheRound(HunterView currentView)
{
    return getRound(currentView->view);
}

// Get the id of current player
PlayerID whoAmI(HunterView currentView)
{
    return getCurrentPlayer(currentView->view);
}

// Get the current score
int giveMeTheScore(HunterView currentView)
{
    return getScore(currentView->view);
}

// Get the current health points for a given player
int howHealthyIs(HunterView currentView, PlayerID player)
{
    return getHealth(currentView->view, player);
}

// Get the current location id of a given player
LocationID whereIs(HunterView currentView, PlayerID player)
{
    return getLocation(currentView->view, player);
}

//// Functions that return information about the history of the game

// Fills the trail array with the location ids of the last 6 turns
void giveMeTheTrail(HunterView currentView, PlayerID player,
                            LocationID trail[TRAIL_SIZE])
{
    getHistory(currentView->view, player, trail);
}

//// Functions that query the map to find information about connectivity

// What are my possible next moves (locations)
LocationID *whereCanIgo(HunterView currentView, int *numLocations,
                        int road, int rail, int sea)
{   
    LocationID *iCanGo;
    PlayerID player = getCurrentPlayer(currentView->view);
    LocationID from = getLocation(currentView->view, player);
    Round round = getRound(currentView->view);

    if(getRound(currentView->view) == FIRST_ROUND) {
        iCanGo = (LocationID *)(malloc(sizeof(LocationID)*NUM_MAP_LOCATIONS));
        *numLocations = NUM_MAP_LOCATIONS - 1;

        int i;
        for(i=0; i < NUM_MAP_LOCATIONS ;i++) {
            iCanGo[*numLocations] = i;
        }
    } else {
        iCanGo = connectedLocations(currentView->view, numLocations, from, player, round, road, rail, sea);
    }

    return iCanGo;
}

// What are the specified player's next possible moves
LocationID *whereCanTheyGo(HunterView currentView, int *numLocations,
                           PlayerID player, int road, int rail, int sea)
{
    LocationID *theyCanGo;
    LocationID from = getLocation(currentView->view, player);
    Round nextGo;

    // making sure to get the players up coming turn
    if(player > getCurrentPlayer(currentView->view)) {
        nextGo = getRound(currentView->view);
    } else {
        nextGo = getRound(currentView->view) + 1;
    }

    if(nextGo == FIRST_ROUND) {
        theyCanGo = (LocationID *)(malloc(sizeof(LocationID)*NUM_MAP_LOCATIONS));
        *numLocations = NUM_MAP_LOCATIONS - 1;

        int i;
        for(i=0; i < NUM_MAP_LOCATIONS ;i++) {
            theyCanGo[*numLocations] = i;
        }
    } else {
        if(player == PLAYER_DRACULA) {
            theyCanGo = connectedLocations(currentView->view, numLocations, from, player, nextGo, road, FALSE, sea);
        } else {
            theyCanGo = connectedLocations(currentView->view, numLocations, from, player, nextGo, road, rail, sea);
        }
    }

    return theyCanGo;
}