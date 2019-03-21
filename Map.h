// Map.h ... interface to Map data type

#ifndef MAP_H
#define MAP_H

#include "Places.h"

typedef struct edge{
    LocationID  start;
    LocationID  end;
    TransportID type;
} Edge;

// graph representation is hidden 
typedef struct MapRep *Map;

typedef struct connectionList{
    LocationID *connections;
    int numConnections;
} connectionList;

// operations on graphs 
Map  newMap();  
void disposeMap(Map g); 
void showMap(Map g); 
int  numV(Map g);
int  numE(Map g, TransportID type);

//finds all connections of a specified type, from a specified location
connectionList getConnections(Map g, LocationID locationFrom, TransportID type);

#endif
