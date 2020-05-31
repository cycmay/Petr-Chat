#ifndef ROOMLINKLIST_H
#define ROOMLINKLIST_H

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <stdio.h>
#include <stdint.h>

#include "ClientLinkList.h"


//聊天室结构体
typedef struct _Room
{
	// name of room
	char roomName[20];
	// creator of the room
	Client * creator;
	// users in the room
	Client * usersList;
	struct _Room * next;			//指向下一个结点
}Room;

void InitRoomList(Room * room);


void AddRoom(Room * root, Room * room);


bool RemoveRoom(Room * root, char * roomName);

Room * FindRoomByName(Room * root, char* roomName);

int CountConRoom(Room * root);

// add user node in usersList
Client * AddClientInRoom(Room * root, Client * client);

// create new room
Room * createNewRoom(Client * creator, char * roomName);

extern Room * RoomList;
#endif
