#include "stdio.h"
#include "windows.h"
#include "process.h"
#define _CRT_SECURE_NO_DEPRECATE
#include "string.h"
#include <winuser.h>
#include <winbase.h>

char convert_file_name[255];
unsigned short packet_length;
FILE *fp;
FILE *cp;
char quest_file_name[0x10];
unsigned quest_packet_size;
int readOK;
char packet_buffer [2048];

int main(int argc, char *argv[])
{
	printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf ("Blue Burst .RAW Quest conversion tool v .123 written by Sodaboy\n");
	printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	if (argc < 2)
	{
		printf ("ERROR: You need to provide a RAW quest file for conversion.\n");
	}
	else
	{
		memcpy (&convert_file_name[0], &argv[1][0], strlen (&argv[1][0]));
		printf ("Attempting to open file %s for conversion..\n", &convert_file_name[0]);
		if ( ( cp = fopen (&convert_file_name[0], "rb") ) == NULL )
		{
			printf ("ERROR: Cannot open conversion file.  %s not found.\n", &convert_file_name[0]);
		}
		else
		{
			while ( ( readOK = fread ( &packet_length, 2, 1, cp ) ) == 1 )
			{
				if ( packet_length % 8 ) 
					packet_length += ( 8 - ( packet_length % 8 ) );
				printf ("Reading packet... Length: %u\n", packet_length );
				packet_length -= 2;
				readOK = fread ( &packet_buffer[0], packet_length, 1, cp );
				printf ("Command %02x encountered..\n", packet_buffer[0]);
				if ( packet_buffer[0] == 0x13 )
				{
					memcpy ( &quest_file_name[0], &packet_buffer[0x06], 0x10 );
					memcpy ( &quest_packet_size, &packet_buffer[0x416], 4 );
					printf ("Writing data to %s (%u bytes)..\n", &quest_file_name[0], quest_packet_size );
					fp = fopen ( &quest_file_name[0], "ab");
					fwrite ( &packet_buffer[0x16], 1, quest_packet_size, fp ); 
					fclose ( fp );
				}
			}
			printf ("FINISH: End of quest file reached.\n");
			fclose (cp);
		}
		
		
	}
	
}