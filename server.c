#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

const int mtext_size = (sizeof(int) * 2) + (sizeof(char) * 1024);

struct mtext
{
  int type;
  int num;
  char text[1024];
};

struct msgbuf
{
  long type;
  struct mtext text;
};

struct msgbuf msg;
int fd[32], flag[32];
char name[32][32];

void init()
{
  key_t key;
  int i, j;
  for (i = 0; i < 32; i++)
  {
    for (j = 0; j < 32; j++)
      name[i][j] = '\0';
    flag[i] = 0;
  }
  key = ftok("./server", 1);
  fd[0] = msgget(key, IPC_CREAT | 0666);
  flag[0] = 1;
  key = ftok("./server", 2);
  fd[1] = msgget(key, IPC_CREAT | 0666);
  flag[1] = 1;
  printf("SERVER: start\n");
}

void fin(int err)
{
  int i;
  for (i = 0; i < 32; i++)
  {
    if (flag[i]) msgctl(fd[i], IPC_RMID, 0);
  }
  printf("SERVER: finish\n");
  exit(err);
}

void newname()
{
  int i, j;
  for (i = 0; i < 32; i++)
    for (j = 0; j < 32; j++)
      msg.text.text[i * 32 + j] = name[i][j];
  msg.text.type = 1;
  for (i = 2; i < 32; i++)
    if (flag[i]) msgsnd(fd[i], &msg, mtext_size, MSG_NOERROR);
}

void new()
{
  int i, j;
  key_t key;
  i = 2;
  printf("SERVER: request to connect a new client\n");
  while (flag[i] && i < 32) i++;
  if (i == 32) 
  {
    msg.type = 1;
    msg.text.type = -1;
    msg.text.num = 0;
    printf("SERVER: new clienf is not connected\n");
  }
  else
  {
    key = ftok("./server", i + 1);
    fd[i] = msgget(key, IPC_CREAT | 0666);
    flag[i] = 1;
    msg.type = 1;
    msg.text.type = 0;
    msg.text.num = i;
    j = 0;
    while ((msg.text.text[j] != '\n') && (msg.text.text[j] != '\0') && (j < 32))
    {
      name[i][j] = msg.text.text[j];
      j++;
    }
    for (;j < 32; j++)
      name[i][j] = '\0';
    msgsnd(fd[1], &msg, mtext_size, MSG_NOERROR);
    printf("SERVER: new client connected under id %d\n", i);
    newname();
  }
}

void delete()
{
  int i;
  if (flag[msg.text.num]) msgctl(fd[msg.text.num], IPC_RMID, 0);
  flag[msg.text.num] = 0;
  printf("SERVER: client is deletd under id %d\n", msg.text.num);
  for (i = 0; i < 32; i++)
    name[msg.text.num][i] = '\0';
  newname();
}

void message()
{
  int i;
  msg.type = 1;
  msg.text.type = 2;
  printf("SERVER: client ID %d sent a message\n", msg.text.num);
  for (i = 2; i < 32; i++)
    if (flag[i]) msgsnd(fd[i], &msg, mtext_size, MSG_NOERROR);
}

void main(void)
{
  int debaff = 1;
  key_t key;
  int i;
  init();
  while(debaff)
  {
    msgrcv(fd[0], &msg, mtext_size, 0, MSG_NOERROR);
    switch (msg.text.type)
    {
      case 0: 
      {
        new();
        break;
      }
      case 1: 
      {
        delete();
        break;
      }
      case 2:
      {
        if (msg.text.text[0] == '0' &&
            msg.text.text[1] == '0' &&
            msg.text.text[2] == '0') fin(0);
        message();
        break;
      }
    }
  }
  fin(0);
}
