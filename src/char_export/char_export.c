/****************************************************************/
/*	Author:	Sodaboy												*/
/*	Date:	07/22/2008											*/
/*	accountadd.c :  Adds an account to the Tethealla PSO		*/
/*			server...											*/
/*																*/
/*	History:													*/
/*		07/22/2008  TC  First version...						*/
/****************************************************************/


#include	<windows.h>
#include	<stdio.h>
#include	<string.h>
#include	<time.h>

#include	<mysql.h>


/********************************************************
**
**		main  :-
**
********************************************************/

int
main( int argc, char * argv[] )
{
	char inputstr[255] = {0};
	MYSQL * myData;
	char myQuery[255] = {0};
	MYSQL_ROW myRow ;
	MYSQL_RES * myResult;
	int num_rows;
	unsigned gc_num, slot;

	char mySQL_Host[255] = {0};
	char mySQL_Username[255] = {0};
	char mySQL_Password[255] = {0};
	char mySQL_Database[255] = {0};
	unsigned int mySQL_Port;
	int config_index = 0;
	char config_data[255];
	unsigned ch;

	FILE* fp;
	FILE* cf;

	if ( ( fp = fopen ("tethealla.ini", "r" ) ) == NULL )
	{
		printf ("The configuration file tethealla.ini appears to be missing.\n");
		return 1;
	}
	else
		while (fgets (&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)
			{
				if (config_index < 0x04)
				{
					ch = strlen (&config_data[0]);
					if (config_data[ch-1] == 0x0A)
						config_data[ch--]  = 0x00;
					config_data[ch] = 0;
				}
				switch (config_index)
				{
				case 0x00:
					// MySQL Host
					memcpy (&mySQL_Host[0], &config_data[0], ch+1);
					break;
				case 0x01:
					// MySQL Username
					memcpy (&mySQL_Username[0], &config_data[0], ch+1);
					break;
				case 0x02:
					// MySQL Password
					memcpy (&mySQL_Password[0], &config_data[0], ch+1);
					break;
				case 0x03:
					// MySQL Database
					memcpy (&mySQL_Database[0], &config_data[0], ch+1);
					break;
				case 0x04:
					// MySQL Port
					mySQL_Port = atoi (&config_data[0]);
					break;
				default:
					break;
				}
				config_index++;
			}
		}
		fclose (fp);

		if (config_index < 5)
		{
			printf ("tethealla.ini seems to be corrupted.\n");
			return 1;
		}

		if ( (myData = mysql_init((MYSQL*) 0)) && 
			mysql_real_connect( myData, &mySQL_Host[0], &mySQL_Username[0], &mySQL_Password[0], NULL, mySQL_Port,
			NULL, 0 ) )
		{
			if ( mysql_select_db( myData, &mySQL_Database[0] ) < 0 ) {
				printf( "Can't select the %s database !\n", mySQL_Database ) ;
				mysql_close( myData ) ;
				return 2 ;
			}
		}
		else {
			printf( "Can't connect to the mysql server (%s) on port %d !\nmysql_error = %s\n",
				mySQL_Host, mySQL_Port, mysql_error(myData) ) ;

			mysql_close( myData ) ;
			return 1 ;
		}

		printf ("Tethealla Server Character Export Tool\n");
		printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
		printf ("Guild card #: ");
		scanf ("%s", inputstr );
		gc_num = atoi ( inputstr );
		printf ("Slot #: ");
		scanf ("%s", inputstr );
		slot = atoi ( inputstr );
		sprintf (&myQuery[0], "SELECT * from character_data WHERE guildcard='%u' AND slot='%u'", gc_num, slot );
		// Check to see if that character exists.
		//printf ("Executing MySQL query: %s\n", myQuery );
		if ( ! mysql_query ( myData, &myQuery[0] ) )
		{
			myResult = mysql_store_result ( myData );
			num_rows = (int) mysql_num_rows ( myResult );
			if (num_rows)
			{
				myRow = mysql_fetch_row ( myResult );
				cf = fopen ("exported.dat", "wb");
				fwrite (myRow[2], 1, 14752, cf);
				fclose ( cf );
				printf ("Character exported!\n");
			}
			else
				printf ("Character not found.\n");
			mysql_free_result ( myResult );
		}
		else
		{
			printf ("Couldn't query the MySQL server.\n");
			return 1;
		}
		mysql_close( myData ) ;
		return 0;
}
