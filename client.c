#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <ncurses.h>
#include <termios.h>
#include <sys/ioctl.h>

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

struct msgbuf msg, msg_w;
int fd[2], flag[32], num, buf2[32];
char name[32][32], myname[32], buf[32][1024];
pthread_mutex_t mutex;
WINDOW * infowin, * msgwin, * namewin, * chatwin;
WINDOW * subinfowin, * submsgwin, * subnamewin, * subchatwin;

void init_winpan()
{
  delwin(subinfowin);
  delwin(submsgwin);
  delwin(subnamewin);
  delwin(subchatwin);
  delwin(infowin);
  delwin(msgwin);
  delwin(namewin);
  delwin(chatwin);
  
  bkgd(COLOR_PAIR(1));
  clear();

  infowin = newwin(3, getmaxx(stdscr), getmaxy(stdscr) - 3, 0);
  wbkgd(infowin, COLOR_PAIR(1));
  box(infowin, 0, 0);
  subinfowin = derwin(infowin, getmaxy(infowin) - 2, getmaxx(infowin) - 2, 1, 1);
  wrefresh(infowin);
  
  msgwin = newwin(3, getmaxx(stdscr) - 34, getmaxy(stdscr) - 6, 0);
  wbkgd(msgwin, COLOR_PAIR(1));
  box(msgwin, 0, 0);
  submsgwin = derwin(msgwin, getmaxy(msgwin) - 2, getmaxx(msgwin) - 2, 1, 1);
  wrefresh(msgwin);
  
  namewin = newwin(getmaxy(stdscr) - 3, 34, 0, getmaxx(stdscr) - 34);
  wbkgd(namewin, COLOR_PAIR(1));
  box(namewin, 0, 0);
  subnamewin = derwin(namewin, getmaxy(namewin) - 2, getmaxx(namewin) - 2, 1, 1);
  wrefresh(namewin);
  
  chatwin = newwin(getmaxy(stdscr) - 6, getmaxx(stdscr) - 34, 0, 0);
  wbkgd(chatwin, COLOR_PAIR(1));
  box(chatwin, 0, 0);
  subchatwin = derwin(chatwin, getmaxy(chatwin) - 2, getmaxx(chatwin) - 2, 1, 1);
  wrefresh(chatwin);
}

void sig_winch(int signo)
{
  struct winsize size;
  ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
  resizeterm(size.ws_row, size.ws_col);
  init_winpan();
}

void init_w()
{
  initscr();
  //cbreak();
  //noecho();
  curs_set(0);
  start_color();
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(2, COLOR_GREEN, COLOR_WHITE);
  //init_pair(3, COLR_BLUE, COLOR_WHITE);
  init_pair(4, COLOR_RED, COLOR_WHITE);
  signal(SIGWINCH, sig_winch);
  init_winpan();
}

void init()
{
  key_t key;
  int i, j;
  for (i = 0; i < 32; i++)
  {
    for (j = 0; j < 32; j++)
    {
      name[i][j] = '\0';
      msg.text.text[i * 32 + j] = '\0';
      msg_w.text.text[i * 32 + j] = '\0';
    }
    flag[i] = 0;
    myname[i] = '\0';
    for (j = 0; j < 1024; j++)
      buf[i][j] = '\0';
  }
  num = 0;
  key = ftok("./server", 1);
  fd[0] = msgget(key, 0);
  flag[0] = 1;
  key = ftok("./server", 2);
  fd[1] = msgget(key, 0);
  flag[1] = 1;
  pthread_mutex_init(&mutex, NULL);
  init_w();
  //printf("CLIENT: start\n");
}

void fin(pthread_t tid, int err)
{
  delwin(subinfowin);
  delwin(submsgwin);
  delwin(subnamewin);
  delwin(subchatwin);
  delwin(infowin);
  delwin(msgwin);
  delwin(namewin);
  delwin(chatwin);
  endwin();

  pthread_mutex_destroy(&mutex);
  pthread_cancel(tid);
  printf("CLIENT: fin\n");
  exit(err);
}

void print_name_w()
{
  int i, j;
  wclear(subnamewin);
  for (i = 0; i < 32; i++)
    for (j = 0; j < 32; j++)
      name[i][j] = msg_w.text.text[i * 32 + j];
  mvwprintw(subnamewin, 0, 0, "Online customer nicknames\n");
  for (i = 2; i < 32; i++)
  {
    if (i == num) 
    {
      wattron(subnamewin, COLOR_PAIR(2));
      wprintw(subnamewin, "%s\n", name[i]);
      wattron(subnamewin, COLOR_PAIR(1));
    }
    else if (name[i][0] != '\0') 
    {
      wprintw(subnamewin, "%s\n", name[i]);
    }
  }
  wrefresh(subnamewin);
}

void print_name_t()
{
  int i, j;
  for (i = 0; i < 32; i++)
    for (j = 0; j < 32; j++)
      name[i][j] = msg_w.text.text[i * 32 + j];
  j = 1;
  printf("\nCLIENT: online customer nicknames\n");
  for (i = 2; i < 32; i++)
  {
    if (name[i][0] != '\0')
    {
      printf("%d) %s\n", j, name[i]);
      j++;
    }
  }
}

void print_message_w()
{
  int i, j;
  for (i = 31; i > 0; i--)
  {
    for (j = 0; j < 1024; j++)
      buf[i][j] = buf[i - 1][j];
    buf2[i] = buf2[i - 1];
  }
  for (i = 0; i < 1024; i++)
    buf[0][i] = msg_w.text.text[i];
  buf2[0] = msg_w.text.num;
  wmove(subchatwin, 0, 0);
  for(i = 0; i < 32, buf2[i] != 0; i++)
    wprintw(subchatwin, "@%s\n%s\n", name[buf2[i]], buf[i]);
  wrefresh(subchatwin);
}

void print_message_t()
{
  int i;
  printf("\n%s\n%s\n", name[msg_w.text.num], msg_w.text.text);
}

void * wait(void * arg)
{
  while(1)
  {
    msgrcv(fd[1], &msg_w, mtext_size, 0, MSG_NOERROR);
    switch (msg_w.text.type)
    {
      case 1:
      {
        print_name_w();
        //print_name_t();
        break;
      }
      case 2:
      {
        print_message_w();
        //print_message_t();
        break;
      }
    }
  } 
}

void main(void)
{
  pthread_t tid;
  int i;
  key_t key;
  char temp[1024];
  init();
  msg.type = 1;
  msg.text.type = 0;
  mvwprintw(subinfowin, 0, 0, "Enter nuckname (max 32 charecters)");
  wrefresh(subinfowin);
  //printf("CLIENT: enter nickname (max 32 characters)\n");
  //printf("CLIENT> ");
  mvwgetnstr(submsgwin, 0, 0, myname, 32 * sizeof(char));
  wclear(submsgwin);
  wclear(subinfowin);
  wrefresh(submsgwin);
  wrefresh(subinfowin);
  //fgets(myname, 32 * sizeof(char), stdin);
  for (i = 0; i < 32; i++)
    msg.text.text[i] = myname[i];
  msgsnd(fd[0], &msg, mtext_size, MSG_NOERROR);
  msgrcv(fd[1], &msg, mtext_size, 0, MSG_NOERROR);
  if (msg.text.type == -1)
  {
    mvwprintw(subinfowin, 0, 0 , "Error: the server is full!");
    wrefresh(subinfowin);
    getch();
    //printf("CLIENT: the server is full\n");
    fin(tid, 1);
  }
  for (i = 0; i < 32; i++)
    myname[i] = msg.text.text[i];
  num = msg.text.num;
  key = ftok("./server", num + 1);
  fd[1] = msgget(key, 0);
  pthread_create(&tid, NULL, wait, NULL); 
  while(1)
  {
    for (i = 0; i < 1024; i++)
      temp[i] = '\0';
    msg.text.type = 2;
    msg.type = 1;
    msg.text.num = num;
    //printf("CLIENT> ");
    mvwgetnstr(submsgwin, 0, 0, temp, 1024 * sizeof(char));
    wclear(submsgwin);
    wrefresh(submsgwin);
    //fgets(temp, 1024 * sizeof(char), stdin);
    if (temp[0] == 'q' && temp[1] == 'u' && temp[2] == 'i' && temp[3] == 't')
    {
      msg.text.type = 1;
      msgsnd(fd[0], &msg, mtext_size, MSG_NOERROR);
      fin(tid, 0);
    }
    for (i = 0; i < 1024; i++)
      msg.text.text[i] = temp[i];
    msgsnd(fd[0], &msg, mtext_size, MSG_NOERROR);
  }
}
