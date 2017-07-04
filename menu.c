/***************************************************************************/
/*                                                                         */
/*	Program:	Menu.c                                             */
/*                                                                         */
/*	Author:		Tony Hughes                                        */
/*	                                                                   */
/*	Last Modified:	11/12/2005                                         */
/*	                                                                   */
/*	Version     	1.8                                                */
/*                                                                         */
/*	Target:		VAX / UNIX / LINUX / NT / XP / Win2000 / Win2003   */
/*	                Windows Vista / Win 7 / Win 8 / Win 8.1 / Interix  */
/*                                                                         */
/*	Purpose:        Provides a hierarchical menu system for easy       */
/*			Execution of DCL, EXE, and O/S Scripts  	   */
/*			/ Programs or commands.                   	   */
/*                                                                         */
/*	Notes:								   */
/*                                                                         */
/*	VMS compile with:                                  		   */
/*                                                                         */
/*      cc /noopt menu.c                                           	   */
/*      define lnk$library   sys$library:vaxccurse.olb             	   */
/*      define lnk$library_1 sys$library:vaxcrtl.olb               	   */
/*      link menu                                                  	   */
/*                                                                         */
/*	UNIX compile with: cc menu.c -o menu               	  	   */
/*                                                                         */
/*      Modification History                                               */
/*                                                                         */
/*      1.1	Created Unix version Tony Hughes                           */
/*      1.2	Modified for Generic DOS / VMS use. Tony Hughes            */
/*      1.3     Increased path length available for command strings        */
/*              to 250 characters. Tony Hughes                             */
/*      1.4     Fixed "core dump" bug when unable to find men files.       */
/*      1.5     Increased menu description items  to 160 characters.       */
/*      1.6     Added usage message. Tony Hughes                           */
/*      1.7     Added example mlc to usage message. Tony Hughes.           */
/*      1.8     Added include<stdlib.h> for compatibility.                 */
/*                                                                         */
/*      (c) Jscore Ltd 2013                                                */
/*                                                                         */
/*      All rights reserved.                                               */
/*                                                                         */
/***************************************************************************/			

#include <stdio.h>
#include <stdlib.h>

#define QUIT 255
#define NEXT 254

#include <string.h>

/* Program to Run menus - Nested to 15 levels */

#define LEVELS 15
#define BASEMENU mlc
#define DOWN_LEVEL 1
#define QUIT_MENU  0


/* Global Variables */

FILE  *fp;

/* Define Structure to hold menus */

typedef struct {
		char	title[30];
		char	item[LEVELS][2];
		int	menunum[LEVELS];
		char	description[LEVELS][160];
		char	program[LEVELS][250];
		int	num_opts;

} menu;

/* Define array of menus structures */

menu Menu_list[50];

/* Define Global Variables */

int menu_number,menu_cnt,NEST_LEVEL=0,menu_stack[20];
char line[160], *lp;
char mendir[50], *mlcpointer;

/* Main Program */

main()
{
	int loop=1,opts,sel,stack_pointer;
	char ch;
	menu *Menus;
	/* Get environment */
	mlcpointer = mendir;
	*mlcpointer=0; 
 	strcat(mlcpointer,getenv("MLC"));  
	if ( strlen(mlcpointer) == 0 )
	{
		usage_message();
		return(255);
 	}
	printf("");
	/* Point to Structure */
	Menus =  Menu_list + 1;
	/* Open Top level menu (MLC) */
	menu_number=1;
	menu_cnt=1;
	read_menu("mlc",menu_number,NEST_LEVEL); /* Function to recursivley read in all menus */
	opts=Menus->num_opts;
	menu_number=1;
	stack_pointer=0;
	menu_stack[stack_pointer] = 1;
	display_menu(menu_number); /* Display Top level Menu */

	/* Loop */
	while ( loop == 1 )
	{
		sel=get_opts(opts);

	/* Check if quitting from previous menu */

		if ( sel == QUIT )
		{
			if ( stack_pointer == 0 )
			{
				loop=0;
			}else{
				/* Decrement menu_stack */

				stack_pointer--;
				
				Menus =  Menu_list + menu_stack[stack_pointer]; 
				display_menu(menu_stack[stack_pointer]);
				opts=Menus->num_opts;
			}

		}else{
		/* Either select new menu or execute program */
			if ( strncmp(Menus->item[sel],"M",1) )
			{
				proc_prog(sel,menu_stack[stack_pointer]);
			 	/* Re-display Current Menu */
				Menus = Menu_list + menu_stack[stack_pointer];
				display_menu(menu_stack[stack_pointer]); 
			}else{
				/* Increment menu_stack */
				stack_pointer++;
				menu_stack[stack_pointer] = Menus->menunum[sel];
				/* Display New Menu */
				menu_number=Menus->menunum[sel];
				Menus =  Menu_list+menu_number; 
				display_menu(menu_number);
				opts=Menus->num_opts;

			}
		}

	}
}

/* Function to read in "Tree" of menus and submenus */
int read_menu(menu_name,number,level)
char *menu_name;
int number,level;
{
	char Menu[255], *mp;
	int cnt,cnt1,pos,opts;
	menu *Menus;
        mp = Menu;
	level++;
	/* point at specific array element */

	Menus = &Menu_list[number];

	*mp = 0;
	strcat(mp,mlcpointer);
	strcat(mp,menu_name);
	if ((fp = fopen(mp,"r")) == NULL )
	{
		menu_error(mp);
		exit(255);
	}

	/* Get First line from Menu */

	lp = line;
	fgets(lp,160,fp);

	/* Get Menu Title */

	*Menus->title=0;
	strcat(Menus->title,lp);

	/* Read in Menu items */

	cnt1=0;
	lp=line;
	while ( fgets(line,160,fp) != NULL )
	{
	/* Read in each individual item */
		if ( cnt1 < LEVELS )
		{
			*Menus->item[cnt1] = 0;
			*Menus->description[cnt1] = 0;
			*Menus->program[cnt1] = 0;
			strncat(Menus->item[cnt1],lp,1);
			/* Increment pointer */
			*lp++;
			*lp++;
			pos=strpos(",",lp);
			strncat(Menus->description[cnt1],lp,pos);
			/* increment pointer to next field */
			for(cnt=0;cnt<=pos;cnt++)
				*lp++;
			strncat(Menus->program[cnt1],lp,(strlen(lp) -1));
			cnt1++;
		}

		lp=line;
	}
	/* Close Menu File */
	fclose(fp);
	Menus->num_opts = cnt1; /* Store the number of options in menu */
	/* Scan through each menu item to determine which is a submenu */
	/* Then call "read_menu" recursivley until each menu has been read in */
	opts=Menus->num_opts;
	for ( cnt1=0;cnt1<opts;cnt1++ )
	{
		/* Check if option is a menu option */
		if ( strncmp(Menus->item[cnt1],"M",1) )
		{
			/* Reading Program Item */
		}else{
			menu_number++; /* Increment Menu Number */
			Menus->menunum[cnt1] = menu_number; /* Store pointer to sub menu */
			read_menu(Menus->program[cnt1],menu_number,level);
		}
	}
	
}


/* Function to display particular menu number */

display_menu(number)
int number;
{
	int cnt;
	menu *Menus;
	/* VMS SPECIFIC */
	/* system("CLS"); */
	/* UNIX SPECIFIC */
	system("tput clear");

	Menus = &Menu_list[number];  /* Point at relevant menu */
	centre_text(Menus->title);   /* Print Title */
	printf("\n\n\n");

	/* Display Menu Options */
	for(cnt=1;cnt<=Menus->num_opts;cnt++)
		printf("		%d) 	%s\n",cnt,Menus->description[cnt-1]);

}

menu_error(menu_name)
char *menu_name;
{
	printf("Unable to open menu '%s' ... aborting\n",menu_name);
	return(0);
}

strpos ( search,string )
char *search,*string;
{
	int cnt,len,search_len;
	len=strlen(string);
	search_len=strlen(search);
	for(cnt=0;cnt<=len;cnt++)
	{
		if ( strncmp(string,search,search_len) == 0 )
			return(cnt);
		*string++;
	}
}

centre_text(string)
char *string;
{
	int len,pos,cnt;
	len=strlen(string);
	pos=(80 - len) / 2;
	printf("(Ver 1.8)");
	for(cnt=1;cnt<=(pos-9);cnt++)
		printf(" ");
	printf("%s\n",string);
}


get_opts(opts)
int opts;
{
	int sel,chk;
	char num;
	char select[10], *se;
	se = select;
	flushstr(se,10);
	if ( opts < 15 )
	{
	 	printf("\n	Enter Selection ( 1 - %d ) or Q to Quit : ",opts);
       	}else{
		printf("\nEnter Selection ( 1 - %d ) or Q to Quit, N for Next Page : ",opts);
                                                                      
	}
	/* scanf("%d", &sel); */
        gets(se);
/* NEW 8/7/97 */
	chk = strlen(se);	
	/* Determine Whether 'N' or 'Q' has been pressed */
	if ( strncmp(se,"N",1) == 0 || strncmp(se,"n",1) == 0 )
		return(NEXT);
	if ( strncmp(se,"Q",1) == 0 || strncmp(se,"q",1) == 0 )
		return(QUIT);
	if ( strncmp(se,"X",1) == 0 || strncmp(se,"x",1) == 0 )
		return(QUIT);
	sel=atoi(se);
	if ( sel < 10 && chk > 1 )
		sel = 1000;
	if ( sel < 1 || sel > opts )
	{
		printf("Error - Number must be 1 to %d",opts);
		sel=get_opts(opts) + 1;
	}
	return(sel - 1);
}

proc_prog(sel,menunum)
int sel,menunum;
{
	menu *Menus;
	char command[255],*cp;
	cp = command;
	*cp = 0;
	Menus =   Menu_list+menunum; 
	
	/* VMS Specific */
	if ( strncmp(Menus->item[sel],"E",1) == 0 )
	{
		strcat(cp,"RUN ");
		strcat(cp,Menus->program[sel]);
		/* Execute EXE */
		system(cp); 
	}
	if ( strncmp(Menus->item[sel],"P",1) == 0 )
	{
		/* VMS SPECIFIC LINE. COMMENT OUT FOR UNIX OR DOS */
		/* strcat(cp,"@"); */
		strcat(cp,Menus->program[sel]);
		/* Run DCL Script */
		system(cp); 
	}
	if ( strncmp(Menus->item[sel],"O",1) == 0 )
	{
		/* Execute System Command */
		system(Menus->program[sel]); 
	}			
}

flushstr(string,length)
char *string;
int length;
{
	int cnt;
	for(cnt=1;cnt<=length;cnt++)
		*string++ = 0;
}

usage_message(){
	system("tput clear");
	printf("menu: Error\n\n");
	printf("You have not set the environment variable MLC. This needs to be\n");
	printf("Assigned i.e export MLC=/home/mylogin/menus/  to point at the  \n");
	printf("directory containing the menu definitions. The root menu file  \n");
	printf("is ALWAYS named mlc. An example mlc file is shown below:       \n");
	printf("\n");
	printf("Main Menu\n");
	printf("P,This is option 1,/home/mylogin/bin/myscript1                 \n");
	printf("P,This is option 2,/home/mylogin/bin/myscript2                 \n");
	printf("P,This is option 3,/home/mylogin/bin/myscript3                 \n");
	printf("M,This is submenu 1,sub1                                       \n");
	printf("M,This is submenu 2,sub2                                       \n");
	printf("\n");
	printf("Note that sub1 and sub2 are menufiles to be constructed in the \n");
	printf("same format as the mlc file above, and they must also reside in\n");
	printf("the menus directory defined by the environment variable MLC.   \n");
	printf("Menus can be nested to 15 levels.                              \n");
	printf("\n");
}

