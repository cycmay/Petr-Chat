#include "ClientLinkList.h"
/*
* function  初始化链表
* return    无返回值
*/
void Init()
{
	UserList->next = NULL;
}

void initClientList(Client * root){
	root->next = NULL;
}

/*
* function  获取头节点
* return    返回头节点
*/
pClient GetUserListNode()
{
	return UserList;
}

/*
* function	添加一个客户端
* param     client表示一个客户端对象
* return    无返回值
//比如：UserList->1->2,然后添加一个3进来后是
 //3->1->2,UserList->3->1->2
*/

void AddClient(Client * root, Client * client){
	client->next = root->next;  
	root->next = client; 		
}

/*
* function	删除一个客户端
* param     flag标识一个客户端对象
* return    返回true表示删除成功，false表示失败
*/
bool RemoveClient(Client * root, char * name){
	//pCur指向第一个结点
	pClient pCur = root->next;
	//pPre指向UserList 
	pClient pPre = root;      
	while (pCur)
	{
		// UserList->1->2->3->4,要删除2，则直接让1->3
		if (strcmp(pCur->userName, name) == 0)
		{
			pPre->next = pCur->next;
			free(pCur);   //释放该结点
			return true;
		}
		pPre = pCur;
		pCur = pCur->next;
	}
	return false;
}

/*
* function  查找指定客户端
* param     name是指定客户端的用户名
* return    返回socket表示查找成功，返回INVALID_SOCKET表示无此用户
*/
SOCKET FindClientByName(char* name)
{
	//从头遍历，一个个比较
	pClient pCur = UserList;
	while (pCur != NULL)
	{	
		if (strcmp(pCur->userName, name) == 0)
			return pCur->sClient;
		pCur = pCur->next;
	}
	return -1;
}

/*
* function  根据SOCKET查找指定客户端
* param     client是指定客户端的套接字
* return    返回一个pClient表示查找成功，返回NULL表示无此用户
*/
pClient FindClient(Client * root, SOCKET client)
{
	//从头遍历，一个个比较
	pClient pCur = root;
	while (pCur != NULL)
	{
		if (pCur->sClient == client)
			return pCur;
		pCur = pCur->next;
	}
	return NULL;
}

/*
* function  计算客户端连接数
* param     client表示一个客户端对象
* return    返回连接数
*/
int CountCon(Client * root)
{
	int iCount = 0;
	pClient pCur = root;
	while (pCur != NULL){
		iCount++;
		pCur = pCur->next;
	}
	// 头结点无数据
	return iCount-1;
}



/*
* function  清空链表
* return    无返回值
*/
void ClearClientList(Client * root)
{
	pClient pCur = root->next;
	pClient pPre = root;
	while (pCur)
	{
		//root->1->2->3->4,root->2,然后free 1
		pClient p = pCur;
		pPre->next = p->next;
		free(p);
		pCur = pPre->next;
	}
}

/*
* function 检查连接状态并关闭一个连接
* return 返回值
*/
void CheckConnection()
{
	// pClient pclient = GetUserListNode();
	// while (pclient != NULL)
	// {
	// 	if (send(pclient->sClient, "", sizeof(""), 0) == -1)
	// 	{
	// 		if (pclient->sClient != 0)
	// 		{
	// 			printf("Disconnect from IP: %s,UserName: %s\n", pclient->IP, pclient->userName);
	// 			char error[128] = { 0 };   //发送下线消息给发消息的人
	// 			sprintf(error, "The %s was downline.\n", pclient->userName);
	// 			send(FindClient(pclient->ChatName), error, sizeof(error), 0);
	// 			closesocket(pclient->sClient);   //这里简单的判断：若发送消息失败，则认为连接中断(其原因有多种)，关闭该套接字
	// 			RemoveClient(pclient->flag);
	// 			break;
	// 		}
	// 	}
	// 	pclient = pclient->next;
	// }
}

/*
* function  指定发送给哪个客户端
* param     FromName，发信人
* param     ToName,   收信人
* param		data,	  发送的消息
*/
void SendData(char* FromName, char* ToName, char* data)
{
	// SOCKET client = FindClient(ToName);   //查找是否有此用户
	// char error[128] = { 0 };
	// int ret = 0;
	// if (client != -1 && strlen(data) != 0)
	// {
	// 	char buf[128] = { 0 };
	// 	sprintf(buf, "%s: %s", FromName, data);   //添加发送消息的用户名
	// 	ret = send(client, buf, sizeof(buf), 0);
	// }
	// else//发送错误消息给发消息的人
	// {
	// 	if(client == -1)
	// 		sprintf(error, "The %s was downline.\n", ToName);
	// 	else
	// 		sprintf(error, "Send to %s message not allow empty, Please try again!\n", ToName);
	// 	send(FindClient(FromName), error, sizeof(error), 0);
	// }
	// if (ret == SOCKET_ERROR)//发送下线消息给发消息的人
	// {
	// 	sprintf(error, "The %s was downline.\n", ToName);
	// 	send(FindClient(FromName), error, sizeof(error), 0);
	// }

}

