#include "RoomLinkList.h"
#include "ClientLinkList.h"

void InitRoomList(Room * room)
{
	room->next= NULL;
}


void AddRoom(Room * root, Room * room){
	room->next = root->next;  
	root->next = room;       
}

bool RemoveRoom(Room * root, char * roomName){
	//从头遍历，一个个比较
	Room * pCur = root->next;//pCur指向第一个结点
	Room * pPre = root;      //pPre指向UserList 
	while (pCur)
	{
		if (strcmp(roomName, pCur->roomName)==0)
		{
			pPre->next = pCur->next;
			// 删除usersList内的所有节点
			ClearClientList(pCur->usersList);
			free(pCur);   //释放该结点
			return true;
		}
		pPre = pCur;
		pCur = pCur->next;
	}
	return false;
}

Room * FindRoomByName(Room * root, char* roomName){
	//从头遍历，一个个比较
	Room * pCur = root;
	while (pCur != NULL)
	{	
		if (strcmp(pCur->roomName, roomName) == 0)
			return pCur;
		pCur = pCur->next;
	}
	return NULL;
}

int CountConRoom(Room * root){
	int iCount = 0;
	Room * pCur = root;
	while (pCur != NULL){
		iCount++;
		pCur = pCur->next;
	}
	// 头结点无数据
	return iCount-1;
}

// create user node in room 's usersList
// return the user client *
Client * AddClientInRoom(Room * root, Client * client){
	Client * user = (Client *)malloc(sizeof(Client));
    memcpy(user, client, sizeof(Client));
    // add user into room 's usersList
    AddClient(root->usersList, user);
    return user;
}

// create new room
Room * createNewRoom(Client * creator, char * roomName){
	// create room node in room list
    Room * create_room = (Room *)malloc(sizeof(Room));
    // create head of room 's usersList
    create_room->usersList = (Client *)malloc(sizeof(Client));
    // init userList
    initClientList(create_room->usersList);
    strcpy(create_room->roomName, roomName);
    // create creator client of room , it is copy from UserList usr client
    create_room->creator = AddClientInRoom(create_room, creator);
   
    // add new room in RoomList
    AddRoom(RoomList, create_room);
    return create_room;
}