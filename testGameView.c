// testGameView.c ... test the GameView ADT

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "GameView.h"

//unit tests
static void testGetHistory(void);
static void testConnectedLocations(void);

int main()
{
    int i;
    GameView gv;

    //UNIT TESTS
    testGetHistory();
    testConnectedLocations();

    //TESTING FUNCTIONS TOGETHER
    printf("Test basic empty initialisation\n");
    PlayerMessage messages1[] = {};
    gv = newGameView("", messages1);
    assert(getCurrentPlayer(gv) == PLAYER_LORD_GODALMING);
    assert(getRound(gv) == 0);
    assert(getHealth(gv,PLAYER_DR_SEWARD) == GAME_START_HUNTER_LIFE_POINTS);
    assert(getHealth(gv,PLAYER_DRACULA) == GAME_START_BLOOD_POINTS);
    assert(getScore(gv) == GAME_START_SCORE);
    assert(getLocation(gv,PLAYER_LORD_GODALMING) == UNKNOWN_LOCATION);
    printf("passed\n");
    disposeGameView(gv);

    printf("Test for Dracula trail and basic functions\n");
    PlayerMessage messages2[] = {"Hello","Rubbish","Stuff","","Mwahahah"};
    gv = newGameView("GST.... SAO.... HZU.... MBB.... DC?....", messages2);
    assert(getCurrentPlayer(gv) == PLAYER_LORD_GODALMING);
    assert(getRound(gv) == 1);
    assert(getLocation(gv,PLAYER_LORD_GODALMING) == STRASBOURG);
    assert(getLocation(gv,PLAYER_DR_SEWARD) == ATLANTIC_OCEAN);
    assert(getLocation(gv,PLAYER_VAN_HELSING) == ZURICH);
    assert(getLocation(gv,PLAYER_MINA_HARKER) == BAY_OF_BISCAY);
    assert(getLocation(gv,PLAYER_DRACULA) == CITY_UNKNOWN);
    assert(getHealth(gv,PLAYER_DRACULA) == GAME_START_BLOOD_POINTS);
    printf("passed\n");
    disposeGameView(gv);

    printf("Test for encountering Dracula and hunter history\n");
    PlayerMessage messages3[] = {"Hello","Rubbish","Stuff","","Mwahahah","Aha!"};
    gv = newGameView("GST.... SAO.... HCD.... MAO.... DGE.... GGED...", messages3);
    assert(getLocation(gv,PLAYER_DRACULA) == GENEVA);
    assert(getHealth(gv,PLAYER_LORD_GODALMING) == 5);
    assert(getHealth(gv,PLAYER_DRACULA) == 30);
    assert(getLocation(gv,PLAYER_LORD_GODALMING) == GENEVA);
    LocationID history[TRAIL_SIZE];
    getHistory(gv,PLAYER_DRACULA,history);
    assert(history[0] == GENEVA);
    assert(history[1] == UNKNOWN_LOCATION);
    getHistory(gv,PLAYER_LORD_GODALMING,history);
    assert(history[0] == GENEVA);
    assert(history[1] == STRASBOURG);
    assert(history[2] == UNKNOWN_LOCATION);
    getHistory(gv,PLAYER_DR_SEWARD,history);
    assert(history[0] == ATLANTIC_OCEAN);
    assert(history[1] == UNKNOWN_LOCATION);
    printf("passed\n");        
    disposeGameView(gv);

    printf("Test for Dracula doubling back at sea, and losing blood points (Hunter View)\n");
    PlayerMessage messages4[] = {"Hello","Rubbish","Stuff","","Mwahahah","Aha!","","","","Back I go"};
    gv = newGameView("GGE.... SGE.... HGE.... MGE.... DS?.... "
                     "GST.... SST.... HST.... MST.... DD1....", messages4);
    assert(getLocation(gv,PLAYER_DRACULA) == DOUBLE_BACK_1);
    getHistory(gv,PLAYER_DRACULA,history);
    assert(history[0] == DOUBLE_BACK_1);
    assert(history[1] == SEA_UNKNOWN);
    assert(getHealth(gv,PLAYER_DRACULA) == GAME_START_BLOOD_POINTS - 4);
    assert(getCurrentPlayer(gv) == 0);
    printf("passed\n");
    disposeGameView(gv);

    printf("Test for Dracula doubling back at sea, and losing blood points (Drac View)\n");
    PlayerMessage messages5[] = {"Hello","Rubbish","Stuff","","Mwahahah","Aha!","","","","Back I go"};
    gv = newGameView("GGE.... SGE.... HGE.... MGE.... DEC.... "
                     "GST.... SST.... HST.... MST.... DD1....", messages5);
    assert(getLocation(gv,PLAYER_DRACULA) == DOUBLE_BACK_1);
    getHistory(gv,PLAYER_DRACULA,history);
    assert(history[0] == DOUBLE_BACK_1);
    assert(history[1] == ENGLISH_CHANNEL);
    assert(getHealth(gv,PLAYER_DRACULA) == GAME_START_BLOOD_POINTS - 4);
    assert(getCurrentPlayer(gv) == 0);
    printf("passed\n");
    disposeGameView(gv);

    printf("Test for connections\n");
    int size, seen[NUM_MAP_LOCATIONS], *edges;
    gv = newGameView("", messages1);    
    printf("Checking Galatz road connections\n");
    edges = connectedLocations(gv,&size,GALATZ,PLAYER_LORD_GODALMING,0,1,0,0);
    memset(seen, 0, NUM_MAP_LOCATIONS*sizeof(int));
    for (i = 0; i< size ; i++) seen[edges[i]] = 1;
    assert(size == 5); assert(seen[GALATZ]); assert(seen[CONSTANTA]);
    assert(seen[BUCHAREST]); assert(seen[KLAUSENBURG]); assert(seen[CASTLE_DRACULA]);
    free(edges);
    printf("Checking Ionian Sea sea connections\n");
    edges = connectedLocations(gv,&size,IONIAN_SEA,PLAYER_LORD_GODALMING,0,0,0,1);
    memset(seen, 0, NUM_MAP_LOCATIONS*sizeof(int));
    for (i = 0; i < size; i++) seen[edges[i]] = 1;
    assert(size == 7); assert(seen[IONIAN_SEA]); assert(seen[BLACK_SEA]);
    assert(seen[ADRIATIC_SEA]); assert(seen[TYRRHENIAN_SEA]);
    assert(seen[ATHENS]); assert(seen[VALONA]); assert(seen[SALONICA]);
    free(edges);
    printf("Checking Athens rail connections (none)\n");
    edges = connectedLocations(gv,&size,ATHENS,PLAYER_LORD_GODALMING,0,0,1,0);
    assert(size == 1);
    assert(edges[0] == ATHENS);
    free(edges);
    printf("passed\n");
    disposeGameView(gv);
    return 0;
}

static void testGetHistory(void){
    //Unit Test getHistory Function
    GameView gv;
    int character;
    printf("Testing getHistory Function\n");
    PlayerMessage messagesA[] = {"Hello","Rubbish","Stuff","","Mwahahah","Aha!","","","","Back I go"};
    gv = newGameView("GGE.... SGE.... HGE.... MGE.... DEC.... "
                             "GST.... SST.... HST.... MLO.... DLO....", messagesA);

    char *players[NUM_PLAYERS] = {"Godalming", "Seward", "Van Helsing", "Mina", "Dracula"};

    for (character=PLAYER_LORD_GODALMING; character<=PLAYER_DRACULA; character++){
        printf("   %s history: ", players[character]);
        LocationID charTrail[TRAIL_SIZE];
        getHistory(gv, character, charTrail);
        int i;
        for (i=0; i<TRAIL_SIZE; i++){
            if (charTrail[i] == -1){
                printf("unknown, ");
            } else {
                printf("%s, ", idToName(charTrail[i]));
            }
        }
        printf("\n");
    }
    disposeGameView(gv);
    printf("Passed test 1\n");

    gv = newGameView("GMN.... SPL.... HAM.... MPA.... DGA.V.. "
                             "GLV.... SLO.... HNS.... MST.... DHIT... "
                             "GIR.... SPL.... HAO.... MZU.... DCDT... "
                             "GSW.... SLO.... HNS.... MFR.... DKLT... "
                             "GLV.... SPL.... HAO.... MZU.... DBCT... "
                             "GSW.... SLO.... HNS.... MMR....", messagesA);

    for (character=PLAYER_LORD_GODALMING; character<=PLAYER_DRACULA; character++){
        printf("   %s history: ", players[character]);
        LocationID charTrail[TRAIL_SIZE];
        getHistory(gv, character, charTrail);
        int i;
        for (i=0; i<TRAIL_SIZE; i++){
            if (charTrail[i] == -1){
                printf("unknown, ");
            } else {
                printf("%s, ", idToName(charTrail[i]));
            }
        }
        printf("\n");
    }
    disposeGameView(gv);

    printf("Passed test 2\n");

    //need to manage "hide" and "double back"....
}

static void testConnectedLocations(void){
    printf("Testing connectedLocations Function\n");
    GameView gv;
    int numLocations = 0;
    char *players[NUM_PLAYERS] = {"Godalming", "Seward", "Van Helsing", "Mina", "Dracula"};
    //initialise empty gameview
    PlayerMessage messagesB[] = {};
    gv = newGameView("", messagesB);

    //LocationID from;
    PlayerID player;
    // connectedLocations(GameView currentView, int *numLocations, LocationID from,
    //                          PlayerID player, Round round, int road, int rail, int sea);
    for (player=PLAYER_LORD_GODALMING; player<=PLAYER_DRACULA; player++){
        printf("   connections from Paris for %s: ", players[player]);
        LocationID *connectionArray = malloc(sizeof(LocationID)* MAX_MAP_LOCATION);
        connectionArray = connectedLocations(gv, &numLocations, PARIS, player, getRound(gv), TRUE, TRUE, TRUE);
        //LocationID connections[numLocations];
        int i;
        for (i=0; i<numLocations; i++){
            printf("%s, ", idToName(connectionArray[i]));
        }
        printf("\n");
    }
    printf("Passed test 1\n");

};
