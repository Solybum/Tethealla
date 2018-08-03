/************************************************************************
	Tethealla Ship Server
	Copyright (C) 2008  Terry Chatman Jr.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3 as
	published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
************************************************************************/

#define reveal_window \
	ShowWindow ( consoleHwnd, SW_NORMAL ); \
	SetForegroundWindow ( consoleHwnd ); \
	SetFocus ( consoleHwnd )

#define swapendian(x) ( ( x & 0xFF ) << 8 ) + ( x >> 8 )
#define FLOAT_PRECISION 0.00001

// To do:
//
// Firewall for Team
//
// Challenge
//
// Allow quests to be reloaded while people are in them... somehow!

#define SERVER_VERSION "0.144"
#define USEADDR_ANY
#define TCP_BUFFER_SIZE 64000
#define PACKET_BUFFER_SIZE ( TCP_BUFFER_SIZE * 16 )
//#define LOG_60
#define SHIP_COMPILED_MAX_CONNECTIONS 900
#define SHIP_COMPILED_MAX_GAMES 75
#define LOGIN_RECONNECT_SECONDS 15
#define MAX_SIMULTANEOUS_CONNECTIONS 6
#define MAX_SAVED_LOBBIES 20
#define MAX_SAVED_ITEMS 3000
#define MAX_GCSEND 2000
#define ALL_ARE_GM 0
#define PRS_BUFFER 262144

#define SEND_PACKET_03 0x00
#define RECEIVE_PACKET_93 0x0A
#define MAX_SENDCHECK 0x0B

// Our Character Classes

#define CLASS_HUMAR 0x00
#define CLASS_HUNEWEARL 0x01
#define CLASS_HUCAST 0x02
#define CLASS_RAMAR 0x03
#define CLASS_RACAST 0x04
#define CLASS_RACASEAL 0x05
#define CLASS_FOMARL 0x06
#define CLASS_FONEWM 0x07
#define CLASS_FONEWEARL 0x08
#define CLASS_HUCASEAL 0x09
#define CLASS_FOMAR 0x0A
#define CLASS_RAMARL 0x0B
#define CLASS_MAX 0x0C

// Class equip_flags

#define HUNTER_FLAG	1   // Bit 1
#define RANGER_FLAG	2   // Bit 2
#define FORCE_FLAG	4   // Bit 3
#define HUMAN_FLAG	8   // Bit 4
#define	DROID_FLAG	16  // Bit 5
#define	NEWMAN_FLAG	32  // Bit 6
#define	MALE_FLAG	64  // Bit 7
#define	FEMALE_FLAG	128 // Bit 8

#include	<windows.h>
#include	<stdio.h>
#include	<string.h>
#include	<time.h>
#include	<math.h>

#include	"resource.h"
#include	"pso_crypt.h"
#include	"bbtable.h"
#include	"localgms.h"
#include	"prs.cpp"
#include	"def_map.h" // Map file name definitions
#include	"def_block.h" // Blocked packet definitions
#include	"def_packets.h" // Pre-made packet definitions
#include	"def_structs.h" // Various structure definitions
#include	"def_tables.h" // Various pre-made table definitions

const unsigned char Message03[] = { "Tethealla Ship v.144" };

/* function defintions */

extern void	mt_bestseed(void);
extern void	mt_seed(void);	/* Choose seed from random input. */
extern unsigned long	mt_lrand(void);	/* Generate 32-bit random value */

char* Unicode_to_ASCII (unsigned short* ucs);
void WriteLog(char *fmt, ...);
void WriteGM(char *fmt, ...);
void ShipSend04 (unsigned char command, BANANA* client, ORANGE* ship);
void ShipSend0E (ORANGE* ship);
void Send01 (const char *text, BANANA* client);
void ShowArrows (BANANA* client, int to_all);
unsigned char* MakePacketEA15 (BANANA* client);
void SendToLobby (LOBBY* l, unsigned max_send, unsigned char* src, unsigned short size, unsigned nosend );
void removeClientFromLobby (BANANA* client);

void debug(char *fmt, ...);
void debug_perror(char * msg);
void tcp_listen (int sockfd);
int tcp_accept (int sockfd, struct sockaddr *client_addr, int *addr_len );
int tcp_sock_connect(char* dest_addr, int port);
int tcp_sock_open(struct in_addr ip, int port);

void encryptcopy (BANANA* client, const unsigned char* src, unsigned size);
void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned size);

void prepare_key(unsigned char *keydata, unsigned len, struct rc4_key *key);
void compressShipPacket ( ORANGE* ship, unsigned char* src, unsigned long src_size );
void decompressShipPacket ( ORANGE* ship, unsigned char* dest, unsigned char* src );
int qflag (unsigned char* flag_data, unsigned flag, unsigned difficulty);

/* variables */

struct timeval select_timeout = { 
	0,
	5000
};

FILE* debugfile;

// Random drop rates

unsigned WEAPON_DROP_RATE,
ARMOR_DROP_RATE,
MAG_DROP_RATE,
TOOL_DROP_RATE,
MESETA_DROP_RATE,
EXPERIENCE_RATE;
unsigned common_rates[5] = { 0 };

// Rare monster appearance rates

unsigned	hildebear_rate, 
			rappy_rate,
			lily_rate,
			slime_rate,
			merissa_rate,
			pazuzu_rate,
			dorphon_rate,
			kondrieu_rate = 0;

unsigned common_counters[5] = {0};

unsigned char common_table[100000];

unsigned char PacketA0Data[0x4000] = {0};
unsigned char Packet07Data[0x4000] = {0};
unsigned short Packet07Size = 0;
unsigned char PacketData[TCP_BUFFER_SIZE];
unsigned char PacketData2[TCP_BUFFER_SIZE]; // Sometimes we need two...
unsigned char tmprcv[PACKET_BUFFER_SIZE];

/* Populated by load_config_file(): */

unsigned char serverIP[4] = {0};
int autoIP = 0;
unsigned char loginIP[4];
unsigned short serverPort;
unsigned short serverMaxConnections;
int ship_support_extnpc = 0;
unsigned serverNumConnections = 0;
unsigned blockConnections = 0;
unsigned blockTick = 0;
unsigned serverConnectionList[SHIP_COMPILED_MAX_CONNECTIONS];
unsigned short serverBlocks;
unsigned char shipEvent;
unsigned serverID = 0;
time_t servertime;
unsigned normalName = 0xFFFFFFFF;
unsigned globalName = 0xFF1D94F7;
unsigned localName = 0xFFB0C4DE;

unsigned short ship_banmasks[5000][4] = {0}; // IP address ban masks
BANDATA ship_bandata[5000];
unsigned num_masks = 0;
unsigned num_bans = 0;

/* Common tables */

PTDATA pt_tables_ep1[10][4];
PTDATA pt_tables_ep2[10][4];

// Episode I parsed PT data

unsigned short weapon_drops_ep1[10][4][10][4096];
unsigned char slots_ep1[10][4][4096];
unsigned char tech_drops_ep1[10][4][10][4096];
unsigned char tool_drops_ep1[10][4][10][4096];
unsigned char power_patterns_ep1[10][4][4][4096];
char percent_patterns_ep1[10][4][6][4096];
unsigned char attachment_ep1[10][4][10][4096];

// Episode II parsed PT data

unsigned short weapon_drops_ep2[10][4][10][4096];
unsigned char slots_ep2[10][4][4096];
unsigned char tech_drops_ep2[10][4][10][4096];
unsigned char tool_drops_ep2[10][4][10][4096];
unsigned char power_patterns_ep2[10][4][4][4096];
char percent_patterns_ep2[10][4][6][4096];
unsigned char attachment_ep2[10][4][10][4096];


/* Rare tables */

unsigned rt_tables_ep1[0x200 * 10 * 4] = {0}; 
unsigned rt_tables_ep2[0x200 * 10 * 4] = {0};
unsigned rt_tables_ep4[0x200 * 10 * 4] = {0};

unsigned char startingData[12*14];
playerLevel playerLevelData[12][200];

fd_set ReadFDs, WriteFDs, ExceptFDs;

saveLobby savedlobbies[MAX_SAVED_LOBBIES];
unsigned char dp[TCP_BUFFER_SIZE*4];
unsigned ship_ignore_list[300] = {0};
unsigned ship_ignore_count = 0;
unsigned ship_gcsend_list[MAX_GCSEND*3] = {0};
unsigned ship_gcsend_count = 0;
char Ship_Name[255];
SHIPLIST shipdata[200];
BLOCK* blocks[10];
QUEST quests[512];
QUEST_MENU quest_menus[12];
unsigned* quest_allow = 0; // the "allow" list for the 0x60CA command...
unsigned quest_numallows;
unsigned numQuests = 0;
unsigned questsMemory = 0;
char* languageExts[10];
char* languageNames[10];
unsigned numLanguages = 0;
unsigned totalShips = 0;
BATTLEPARAM ep1battle[374];
BATTLEPARAM ep2battle[374];
BATTLEPARAM ep4battle[332];
BATTLEPARAM ep1battle_off[374];
BATTLEPARAM ep2battle_off[374];
BATTLEPARAM ep4battle_off[332];
unsigned battle_count;
SHOP shops[7000];
unsigned shop_checksum;
unsigned shopidx[200];
unsigned ship_index;
unsigned char ship_key[128];

// New leet parameter tables!!!!111oneoneoneeleven

unsigned char armor_equip_table[256] = {0};
unsigned char barrier_equip_table[256] = {0};
unsigned char armor_level_table[256] = {0};
unsigned char barrier_level_table[256] = {0};
unsigned char armor_dfpvar_table[256] = {0};
unsigned char barrier_dfpvar_table[256] = {0};
unsigned char armor_evpvar_table[256] = {0};
unsigned char barrier_evpvar_table[256] = {0};
unsigned char weapon_equip_table[256][256] = {0};
unsigned short weapon_atpmax_table[256][256] = {0};
unsigned char grind_table[256][256] = {0};
unsigned char special_table[256][256] = {0};
unsigned char stackable_table[256] = {0};
unsigned equip_prices[2][13][24][80] = {0};
char max_tech_level[19][12];

PSO_CRYPT* cipher_ptr;

#define MYWM_NOTIFYICON (WM_USER+2)
int program_hidden = 1;
HWND consoleHwnd;

unsigned wstrlen ( unsigned short* dest )
{
	unsigned l = 0;
	while (*dest != 0x0000)
	{
		l+= 2;
		dest++;
	}
	return l;
}

void wstrcpy ( unsigned short* dest, const unsigned short* src )
{
	while (*src != 0x0000)
		*(dest++) = *(src++);
	*(dest++) = 0x0000;
}

void wstrcpy_char ( char* dest, const char* src )
{
	while (*src != 0x00)
	{
		*(dest++) = *(src++);
		*(dest++) = 0x00;
	}
	*(dest++) = 0x00;
	*(dest++) = 0x00;
}

void packet_to_text ( unsigned char* buf, int len )
{
	int c, c2, c3, c4;

	c = c2 = c3 = c4 = 0;

	for (c=0;c<len;c++)
	{
		if (c3==16)
		{
			for (;c4<c;c4++)
				if (buf[c4] >= 0x20) 
					dp[c2++] = buf[c4];
				else
					dp[c2++] = 0x2E;
			c3 = 0;
			sprintf (&dp[c2++], "\n" );
		}

		if ((c == 0) || !(c % 16))
		{
			sprintf (&dp[c2], "(%04X) ", c);
			c2 += 7;
		}

		sprintf (&dp[c2], "%02X ", buf[c]);
		c2 += 3;
		c3++;
	}

	if ( len % 16 )
	{
		c3 = len;
		while (c3 % 16)
		{
			sprintf (&dp[c2], "   ");
			c2 += 3;
			c3++;
		}
	}

	for (;c4<c;c4++)
		if (buf[c4] >= 0x20) 
			dp[c2++] = buf[c4];
		else
			dp[c2++] = 0x2E;

	dp[c2] = 0;
}


void display_packet ( unsigned char* buf, int len )
{
	packet_to_text ( buf, len );
	printf ("%s\n\n", &dp[0]);
}

void convertIPString (char* IPData, unsigned IPLen, int fromConfig, unsigned char* IPStore )
{
	unsigned p,p2,p3;
	char convert_buffer[5];

	p2 = 0;
	p3 = 0;
	for (p=0;p<IPLen;p++)
	{
		if ((IPData[p] > 0x20) && (IPData[p] != 46))
			convert_buffer[p3++] = IPData[p]; else
		{
			convert_buffer[p3] = 0;
			if (IPData[p] == 46) // .
			{
				IPStore[p2] = atoi (&convert_buffer[0]);
				p2++;
				p3 = 0;
				if (p2>3)
				{
					if (fromConfig)
						printf ("ship.ini is corrupted. (Failed to read IP information from file!)\n"); else
						printf ("Failed to determine IP address.\n");
					printf ("Press [ENTER] to quit...");
					gets(&dp[0]);
					exit (1);
				}
			}
			else
			{
				IPStore[p2] = atoi (&convert_buffer[0]);
				if (p2 != 3)
				{
					if (fromConfig)
						printf ("ship.ini is corrupted. (Failed to read IP information from file!)\n"); else
						printf ("Failed to determine IP address.\n");
					printf ("Press [ENTER] to quit...");
					gets(&dp[0]);
					exit (1);
				}
				break;
			}
		}
	}
}

void convertMask (char* IPData, unsigned IPLen, unsigned short* IPStore )
{
	unsigned p,p2,p3;
	char convert_buffer[5];

	p2 = 0;
	p3 = 0;
	for (p=0;p<IPLen;p++)
	{
		if ((IPData[p] > 0x20) && (IPData[p] != 46))
			convert_buffer[p3++] = IPData[p]; else
		{
			convert_buffer[p3] = 0;
			if (IPData[p] == 46) // .
			{
				if (convert_buffer[0] == 42)
					IPStore[p2] = 0x8000;
				else
					IPStore[p2] = atoi (&convert_buffer[0]);
				p2++;
				p3 = 0;
				if (p2>3)
				{
					printf ("Bad mask encountered in masks.txt...\n");
					memset (&IPStore[0], 0, 8 );
					break;
				}
			}
			else
			{
				IPStore[p2] = atoi (&convert_buffer[0]);
				if (p2 != 3)
				{
					printf ("Bad mask encountered in masks.txt...\n");
					memset (&IPStore[0], 0, 8 );
					break;
				}
				break;
			}
		}
	}
}


unsigned char hexToByte ( char* hs )
{
	unsigned b;

	if ( hs[0] < 58 ) b = (hs[0] - 48); else b = (hs[0] - 55);
	b *= 16;
	if ( hs[1] < 58 ) b += (hs[1] - 48); else b += (hs[1] - 55);
	return (unsigned char) b;
}

void load_mask_file()
{
	char mask_data[255];
	unsigned ch = 0;

	FILE* fp;

	// Load masks.txt for IP ban masks

	num_masks = 0;

	if ( ( fp = fopen ("masks.txt", "r" ) ) )
	{
		while (fgets (&mask_data[0], 255, fp) != NULL)
		{
			if (mask_data[0] != 0x23)
			{
				ch = strlen (&mask_data[0]);
				if (mask_data[ch-1] == 0x0A)
					mask_data[ch--]  = 0x00;
				mask_data[ch] = 0;
			}
			convertMask (&mask_data[0], ch+1, &ship_banmasks[num_masks++][0]);
		}
	}
}

void load_language_file()
{
	FILE* fp;
	char lang_data[256];
	int langExt = 0;
	unsigned ch;

	for (ch=0;ch<10;ch++)
	{
		languageNames[ch] = malloc ( 256 );
		memset (languageNames[ch], 0, 256);
		languageExts[ch] = malloc ( 256 );
		memset (languageExts[ch], 0, 256);
	}

	if ( ( fp = fopen ("lang.ini", "r" ) ) == NULL )
	{
		printf ("Language file does not exist...\nWill use English only...\n\n");
		numLanguages = 1;
		strcat (languageNames[0], "English");	
	}
	else
	{
		while ((fgets (&lang_data[0], 255, fp) != NULL) && (numLanguages < 10))
		{
			if (!langExt)
			{
				memcpy (languageNames[numLanguages], &lang_data[0], strlen (&lang_data[0])+1);
				for (ch=0;ch<strlen(languageNames[numLanguages]);ch++)
					if ((languageNames[numLanguages][ch] == 10) || (languageNames[numLanguages][ch] == 13))
						languageNames[numLanguages][ch] = 0;
				langExt = 1;
			}
			else
			{
				memcpy (languageExts[numLanguages], &lang_data[0], strlen (&lang_data[0])+1);
				for (ch=0;ch<strlen(languageExts[numLanguages]);ch++)
					if ((languageExts[numLanguages][ch] == 10) || (languageExts[numLanguages][ch] == 13))
						languageExts[numLanguages][ch] = 0;
				numLanguages++;
				printf ("Language %u (%s:%s)\n", numLanguages, languageNames[numLanguages-1], languageExts[numLanguages-1]);
				langExt = 0;
			}
		}
		fclose (fp);
		if (numLanguages < 1)
		{
			numLanguages = 1;
			strcat (languageNames[0], "English");
		}
	}
}

void load_config_file()
{
	int config_index = 0;
	char config_data[255];
	unsigned ch = 0;

	FILE* fp;

	EXPERIENCE_RATE = 1; // Default to 100% EXP

	if ( ( fp = fopen ("ship.ini", "r" ) ) == NULL )
	{
		printf ("The configuration file ship.ini appears to be missing.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	else
		while (fgets (&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)
			{
				if ((config_index == 0x00) || (config_index == 0x04) || (config_index == 0x05))
				{
					ch = strlen (&config_data[0]);
					if (config_data[ch-1] == 0x0A)
						config_data[ch--]  = 0x00;
					config_data[ch] = 0;
				}
				switch (config_index)
				{
				case 0x00:
					// Server IP address
					{
						if ((config_data[0] == 0x41) || (config_data[0] == 0x61))
						{
							autoIP = 1;
						}
						else
						{
							convertIPString (&config_data[0], ch+1, 1, &serverIP[0] );
						}
					}
					break;
				case 0x01:
					// Server Listen Port
					serverPort = atoi (&config_data[0]);
					break;
				case 0x02:
					// Number of blocks
					serverBlocks = atoi (&config_data[0]);
					if (serverBlocks > 10) 
					{
						printf ("You cannot host more than 10 blocks... Adjusted.\n");
						serverBlocks = 10;
					}
					if (serverBlocks == 0)
					{
						printf ("You have to host at least ONE block... Adjusted.\n");
						serverBlocks = 1;
					}
					break;
				case 0x03:
					// Max Client Connections
					serverMaxConnections = atoi (&config_data[0]);
					if ( serverMaxConnections > ( serverBlocks * 180 ) )
					{
						printf ("\nYou're attempting to server more connections than the amount of blocks\nyou're hosting allows.\nAdjusted...\n");
						serverMaxConnections = serverBlocks * 180;
					}
					if ( serverMaxConnections > SHIP_COMPILED_MAX_CONNECTIONS )
					{
						printf ("This copy of the ship serving software has not been compiled to accept\nmore than %u connections.\nAdjusted...\n", SHIP_COMPILED_MAX_CONNECTIONS);
						serverMaxConnections = SHIP_COMPILED_MAX_CONNECTIONS;
					}
					break;
				case 0x04:
					// Login server host name or IP
					{
						unsigned p;
						unsigned alpha;
						alpha = 0;
						for (p=0;p<ch;p++)
							if (((config_data[p] >= 65 ) && (config_data[p] <= 90)) ||
								((config_data[p] >= 97 ) && (config_data[p] <= 122)))
							{
								alpha = 1;
								break;
							}
						if (alpha)
						{
							struct hostent *IP_host;

							config_data[strlen(&config_data[0])-1] = 0x00;
							printf ("Resolving %s ...\n", (char*) &config_data[0] );
							IP_host = gethostbyname (&config_data[0]);
							if (!IP_host)
							{
								printf ("Could not resolve host name.");
								printf ("Press [ENTER] to quit...");
								gets(&dp[0]);
								exit (1);
							}
							*(unsigned *) &loginIP[0] = *(unsigned *) IP_host->h_addr;
						}
						else
							convertIPString (&config_data[0], ch+1, 1, &loginIP[0] );
					}
					break;
				case 0x05:
					// Ship Name
					memset (&Ship_Name[0], 0, 255 );
					memcpy (&Ship_Name[0], &config_data[0], ch+1 );
					Ship_Name[12] = 0x00;
					break;
				case 0x06:
					// Event
					shipEvent = (unsigned char) atoi (&config_data[0]);
					PacketDA[0x04] = shipEvent;
					break;
				case 0x07:
					WEAPON_DROP_RATE = atoi (&config_data[0]);
					break;
				case 0x08:
					ARMOR_DROP_RATE = atoi (&config_data[0]);
					break;
				case 0x09:
					MAG_DROP_RATE = atoi (&config_data[0]);
					break;
				case 0x0A:
					TOOL_DROP_RATE = atoi (&config_data[0]);
					break;
				case 0x0B:
					MESETA_DROP_RATE = atoi (&config_data[0]);
					break;
				case 0x0C:
					EXPERIENCE_RATE = atoi (&config_data[0]);
					if ( EXPERIENCE_RATE > 99 )
					{
						printf ("\nWARNING: You have your experience rate set to a very high number.\n");
						printf ("As of ship_server.exe version 0.038, you now just use single digits\n");
						printf ("to represent 100%% increments.  (ex. 1 for 100%, 2 for 200%)\n\n");
						 ("If you've set the high value of %u%% experience on purpose,\n", EXPERIENCE_RATE * 100 );
						printf ("press [ENTER] to continue, otherwise press CTRL+C to abort.\n");
						printf (":");
						gets   (&dp[0]);
						printf ("\n\n");
					}
					break;
				case 0x0D:
					ship_support_extnpc = atoi (&config_data[0]);
					break;
				default:
					break;
				}
				config_index++;
			}
		}
		fclose (fp);

		if (config_index < 0x0D)
		{
			printf ("ship.ini seems to be corrupted.\n");
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		common_rates[0] = 100000 / WEAPON_DROP_RATE;
		common_rates[1] = 100000 / ARMOR_DROP_RATE;
		common_rates[2] = 100000 / MAG_DROP_RATE;
		common_rates[3] = 100000 / TOOL_DROP_RATE;
		common_rates[4] = 100000 / MESETA_DROP_RATE;
		load_mask_file();
}

ORANGE logon_structure;
BANANA * connections[SHIP_COMPILED_MAX_CONNECTIONS];
ORANGE * logon_connecion;
BANANA * workConnect;
ORANGE * logon;
unsigned logon_tick = 0;
unsigned logon_ready = 0;

const char serverName[] = { "T\0E\0T\0H\0E\0A\0L\0L\0A\0" };
const char shipSelectString[] = {"S\0h\0i\0p\0 \0S\0e\0l\0e\0c\0t\0"};
const char blockString[] = {"B\0L\0O\0C\0K\0"};

void Send08(BANANA* client)
{
	BLOCK* b;
	unsigned ch,ch2,qNum;
	unsigned char game_flags, total_games;
	LOBBY* l;
	unsigned Offset;
	QUEST* q;

	if (client->block <= serverBlocks)
	{
		total_games = 0;
		b = blocks[client->block-1];
		Offset = 0x34;
		for (ch=16;ch<(16+SHIP_COMPILED_MAX_GAMES);ch++)
		{
			l = &b->lobbies[ch];
			if (l->in_use)
			{
				memset (&PacketData[Offset], 0, 44);
				// Output game
				Offset += 2;
				PacketData[Offset] = 0x03;
				Offset += 2;
				*(unsigned *) &PacketData[Offset] = ch;
				Offset += 4;
				PacketData[Offset++] = 0x22 + l->difficulty;
				PacketData[Offset++] = l->lobbyCount;
				memcpy(&PacketData[Offset], &l->gameName[0], 30);
				Offset += 32;
				if (!l->oneperson)
					PacketData[Offset++] = 0x40 + l->episode;
				else
					PacketData[Offset++] = 0x10 + l->episode;
				if (l->inpquest)
				{
					game_flags = 0x80;
					// Grey out Government quests that the player is not qualified for...
					q = &quests[l->quest_loaded - 1];
					memcpy (&dp[0], &q->ql[0]->qdata[0x31], 3);
					dp[4] = 0;
					qNum = (unsigned) atoi ( &dp[0] );
					switch (l->episode)
					{
					case 0x01:
						qNum -= 401;
						qNum <<= 1;
						qNum += 0x1F3;
						for (ch2=0x1F5;ch2<=qNum;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					case 0x02:
						qNum -= 451;
						qNum <<= 1;
						qNum += 0x211;
						for (ch2=0x213;ch2<=qNum;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					case 0x03:
						qNum -= 701;
						qNum += 0x2BC;
						for (ch2=0x2BD;ch2<=qNum;ch2++)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					}
				}
				else
					game_flags = 0x40;
				// Get flags for battle and one person games...
				if ((l->gamePassword[0x00] != 0x00) || 
					(l->gamePassword[0x01] != 0x00))
					game_flags |= 0x02;
				if ((l->quest_in_progress) || (l->oneperson)) // Can't join!
					game_flags |= 0x04;
				if (l->battle)
					game_flags |= 0x10;
				if (l->challenge)
					game_flags |= 0x20;
				// Wonder what flags 0x01 and 0x08 control....
				PacketData[Offset++] = game_flags;
				total_games++;
			}
		}
		*(unsigned short*) &client->encryptbuf[0x00] = (unsigned short) Offset;
		memcpy (&client->encryptbuf[0x02], &Packet08[2], 0x32);
		client->encryptbuf[0x04] = total_games;
		client->encryptbuf[0x08] = (unsigned char) client->lobbyNum;
		if ( client->block == 10 )
		{
			client->encryptbuf[0x1C] = 0x31;
			client->encryptbuf[0x1E] = 0x30;
		}
		else
			client->encryptbuf[0x1E] = 0x30 + client->block;

		if ( client->lobbyNum > 9 )
			client->encryptbuf[0x24] = 0x30 - ( 10 - client->lobbyNum );
		else
			client->encryptbuf[0x22] = 0x30 + client->lobbyNum;
		memcpy (&client->encryptbuf[0x34], &PacketData[0x34], Offset - 0x34);
		cipher_ptr = &client->server_cipher;
		encryptcopy ( client, &client->encryptbuf[0x00], Offset );
	}
}

void ConstructBlockPacket()
{
	unsigned short Offset;
	unsigned ch;
	char tempName[255];
	char* tn;
	unsigned BlockID;

	memset (&Packet07Data[0], 0, 0x4000);

	Packet07Data[0x02] = 0x07;
	Packet07Data[0x04] = serverBlocks+1;
	_itoa (serverID, &tempName[0], 10);
	if (serverID < 10) 
	{
		tempName[0] = 0x30;
		tempName[1] = 0x30+serverID;
		tempName[2] = 0x00;
	}
	else
		_itoa (serverID, &tempName[0], 10);
	strcat (&tempName[0], ":");
	strcat (&tempName[0], &Ship_Name[0]);
	Packet07Data[0x32] = 0x08;
	Offset = 0x12;
	tn = &tempName[0];
	while (*tn != 0x00)
	{
		Packet07Data[Offset++] = *(tn++);
		Packet07Data[Offset++] = 0x00;
	}
	Offset = 0x36;
	for (ch=0;ch<serverBlocks;ch++)
	{
				Packet07Data[Offset] = 0x12;
				BlockID = 0xEFFFFFFF - ch;
				*(unsigned *) &Packet07Data[Offset+2] = BlockID;
				memcpy (&Packet07Data[Offset+0x08], &blockString[0], 10 );
				if ( ch+1 < 10 )
				{
					Packet07Data[Offset+0x12] = 0x30;
					Packet07Data[Offset+0x14] = 0x30 + (ch+1);
				}
				else
				{
					Packet07Data[Offset+0x12] = 0x31;
					Packet07Data[Offset+0x14] = 0x30;
				}
				Offset += 0x2C;
	}
	Packet07Data[Offset] = 0x12;
	BlockID = 0xFFFFFF00;
	*(unsigned *) &Packet07Data[Offset+2] = BlockID;
	memcpy (&Packet07Data[Offset+0x08], &shipSelectString[0], 22 );
	Offset += 0x2C;
	while (Offset % 8)
		Packet07Data[Offset++] = 0x00;
	*(unsigned short*) &Packet07Data[0x00] = (unsigned short) Offset;
	Packet07Size = Offset;
}


void initialize_logon()
{
	unsigned ch;

	logon_ready = 0;
	logon_tick = 0;
	logon = &logon_structure;
	if ( logon->sockfd >= 0 )
		closesocket ( logon->sockfd );
	memset (logon, 0, sizeof (ORANGE));
	logon->sockfd = -1;
	for (ch=0;ch<128;ch++)
		logon->key_change[ch] = -1;
	*(unsigned *) &logon->_ip.s_addr = *(unsigned *) &loginIP[0];
}

void reconnect_logon()
{
	// Just in case this is called because of an error in communication with the logon server.
	logon->sockfd = tcp_sock_connect (  inet_ntoa (logon->_ip), 3455 );
	if (logon->sockfd >= 0)
	{
		printf ("Connection successful!\n");
		logon->last_ping = (unsigned) time(NULL);
	}
	else
	{
		printf ("Connection failed.  Retry in %u seconds...\n",  LOGIN_RECONNECT_SECONDS);
		logon_tick = 0;
	}
}

unsigned free_connection()
{
	unsigned fc;
	BANANA* wc;

	for (fc=0;fc<serverMaxConnections;fc++)
	{
		wc = connections[fc];
		if (wc->plySockfd<0)
			return fc;
	}
	return 0xFFFF;
}

void initialize_connection (BANANA* connect)
{
	unsigned ch, ch2;

	// Free backup character memory

	if (connect->character_backup)
	{
		if (connect->mode)
			memcpy (&connect->character, connect->character_backup, sizeof (connect->character));
		free (connect->character_backup);
		connect->character_backup = NULL;
	}

	if (connect->guildcard)
	{
		removeClientFromLobby (connect);

		if ((connect->block) && (connect->block <= serverBlocks))
			blocks[connect->block - 1]->count--;

		if (connect->gotchardata == 1)
		{
			connect->character.playTime += (unsigned) servertime - connect->connected;
			ShipSend04 (0x02, connect, logon);
		}
	}

	if (connect->plySockfd >= 0)
	{
		ch2 = 0;
		for (ch=0;ch<serverNumConnections;ch++)
		{
			if (serverConnectionList[ch] != connect->connection_index)
				serverConnectionList[ch2++] = serverConnectionList[ch];
		}
		serverNumConnections = ch2;
		closesocket (connect->plySockfd);
	}

	if (logon_ready)
	{
		printf ("Player Count: %u\n", serverNumConnections);
		ShipSend0E (logon);
	}

	memset (connect, 0, sizeof (BANANA) );
	connect->plySockfd = -1;
	connect->block = -1;
	connect->lastTick = 0xFFFFFFFF;
	connect->slotnum = -1;
	connect->sending_quest = -1;
}

void start_encryption(BANANA* connect)
{
	unsigned c, c3, c4, connectNum;
	BANANA *workConnect, *c5;

	// Limit the number of connections from an IP address to MAX_SIMULTANEOUS_CONNECTIONS.

	c3 = 0;

	for (c=0;c<serverNumConnections;c++)
	{
		connectNum = serverConnectionList[c];
		workConnect = connections[connectNum];
		//debug ("%s comparing to %s", (char*) &workConnect->IP_Address[0], (char*) &connect->IP_Address[0]);
		if ((!strcmp(&workConnect->IP_Address[0], &connect->IP_Address[0])) &&
			(workConnect->plySockfd >= 0))
			c3++;
	}

	//debug ("Matching count: %u", c3);

	if (c3 > MAX_SIMULTANEOUS_CONNECTIONS)
	{
		// More than MAX_SIMULTANEOUS_CONNECTIONS connections from a certain IP address...
		// Delete oldest connection to server.
		c4 = 0xFFFFFFFF;
		c5 = NULL;
		for (c=0;c<serverNumConnections;c++)
		{
			connectNum = serverConnectionList[c];
			workConnect = connections[connectNum];
			if ((!strcmp(&workConnect->IP_Address[0], &connect->IP_Address[0])) &&
				(workConnect->plySockfd >= 0))
			{
				if (workConnect->connected < c4)
				{
					c4 = workConnect->connected;
					c5 = workConnect;
				}
			}
		}
		if (c5)
		{
			workConnect = c5;
			initialize_connection (workConnect);
		}
	}

	memcpy (&connect->sndbuf[0], &Packet03[0], sizeof (Packet03));
	for (c=0;c<0x30;c++)
	{
		connect->sndbuf[0x68+c] = (unsigned char) mt_lrand() % 255;
		connect->sndbuf[0x98+c] = (unsigned char) mt_lrand() % 255;
	}
	connect->snddata += sizeof (Packet03);
	cipher_ptr = &connect->server_cipher;
	pso_crypt_table_init_bb (cipher_ptr, &connect->sndbuf[0x68]);
	cipher_ptr = &connect->client_cipher;
	pso_crypt_table_init_bb (cipher_ptr, &connect->sndbuf[0x98]);
	connect->crypt_on = 1;
	connect->sendCheck[SEND_PACKET_03] = 1;
	connect->connected = connect->response = connect->savetime = (unsigned) servertime;
}

void SendToLobby (LOBBY* l, unsigned max_send, unsigned char* src, unsigned short size, unsigned nosend )
{
	unsigned ch;

	if (!l)
		return;

	for (ch=0;ch<max_send;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]) && (l->client[ch]->guildcard != nosend))
		{
			cipher_ptr = &l->client[ch]->server_cipher;
			encryptcopy (l->client[ch], src, size);
		}
	}
}


void removeClientFromLobby (BANANA* client)
{
	unsigned ch, maxch, lowestID;

	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	if (client->clientID < 12)
	{
		l->slot_use[client->clientID] = 0;
		l->client[client->clientID] = 0;
	}

	if (client->lobbyNum > 0x0F)
		maxch = 4;
	else
		maxch = 12;

	l->lobbyCount = 0;

	for (ch=0;ch<maxch;ch++)
	{
		if ((l->client[ch]) && (l->slot_use[ch]))
			l->lobbyCount++;
	}

	if ( l->lobbyCount )
	{
		if ( client->lobbyNum < 0x10 )
		{
			Packet69[0x08] = client->clientID;
			SendToLobby ( client->lobby, 12, &Packet69[0], 0x0C, client->guildcard );
		}
		else
		{
			Packet66[0x08] = client->clientID;
			if (client->clientID == l->leader)
			{
				// Leader change...
				lowestID = 0xFFFFFFFF;
				for (ch=0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]) && (l->gamePlayerID[ch] < lowestID))
					{
						// Change leader to oldest person to join game...
						lowestID = l->gamePlayerID[ch];
						l->leader = ch;
					}
				}
				Packet66[0x0A] = 0x01;
			}
			else
				Packet66[0x0A] = 0x00;
			Packet66[0x09] = l->leader;
			SendToLobby ( client->lobby, 4, &Packet66[0], 0x0C, client->guildcard );
		}
	}
	else
		memset ( l, 0, sizeof ( LOBBY ) );
	client->hasquest = 0;
	client->lobbyNum = 0;
	client->lobby = 0;
}


void Send1A (const char *mes, BANANA* client)
{
	unsigned short x1A_Len;

	memcpy (&PacketData[0], &Packet1A[0], sizeof (Packet1A));
	x1A_Len = sizeof (Packet1A);

	while (*mes != 0x00)
	{
		PacketData[x1A_Len++] = *(mes++);
		PacketData[x1A_Len++] = 0x00;
	}

	PacketData[x1A_Len++] = 0x00;
	PacketData[x1A_Len++] = 0x00;

	while (x1A_Len % 8)
		PacketData[x1A_Len++] = 0x00;

	*(unsigned short*) &PacketData[0] = x1A_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], x1A_Len);
}

void Send1D (BANANA* client)
{
	unsigned num_minutes;

	if ((((unsigned) servertime - client->savetime) / 60L) >= 5)
	{
		// Backup character data every 5 minutes.
		client->savetime = (unsigned) servertime;
		ShipSend04 (0x02, client, logon);
	}

	num_minutes = ((unsigned) servertime - client->response) / 60L;
	if (num_minutes)
	{
		if (num_minutes > 2)
			initialize_connection (client); // If the client hasn't responded in over two minutes, drop the connection.
		else
		{
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &Packet1D[0], sizeof (Packet1D));
		}
	}
}

void Send83 (BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &Packet83[0], sizeof (Packet83));

}

unsigned free_game (BANANA* client)
{
	unsigned ch;
	LOBBY* l;

	for (ch=16;ch<(16+SHIP_COMPILED_MAX_GAMES);ch++)
	{
		l = &blocks[client->block - 1]->lobbies[ch];
		if (l->in_use == 0)
			return ch;
	}
	return 0;
}

void ParseMapData (LOBBY* l, MAP_MONSTER* mapData, int aMob, unsigned num_records)
{
	MAP_MONSTER* mm;
	unsigned ch, ch2;
	unsigned num_recons;
	int r;
	
	for (ch2=0;ch2<num_records;ch2++)
	{
		if ( l->mapIndex >= 0xB50 )
			break;
		memcpy (&l->mapData[l->mapIndex], mapData, 72);
		mapData++;
		mm = &l->mapData[l->mapIndex];
		mm->exp = 0;
		switch ( mm->base )
		{
		case 64:
			// Hildebear and Hildetorr
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < hildebear_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 0x02;
				mm->exp = l->bptable[0x4A].XP;
			}
			else
			{
				mm->rt_index = 0x01;
				mm->exp = l->bptable[0x49].XP;
			}
			break;
		case 65:
			// Rappies
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < rappy_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( l->episode == 0x03 )
			{
				// Del Rappy and Sand Rappy
				if ( aMob )
				{
					if ( r )
					{
						mm->rt_index = 18;
						mm->exp = l->bptable[0x18].XP;
					}
					else
					{
						mm->rt_index = 17;
						mm->exp = l->bptable[0x17].XP;
					}
				}
				else
				{
					if ( r )
					{
						mm->rt_index = 18;
						mm->exp = l->bptable[0x06].XP;
					}
					else
					{
						mm->rt_index = 17;
						mm->exp = l->bptable[0x05].XP;
					}
				}
			}
			else
			{
				// Rag Rappy, Al Rappy, Love Rappy and Seasonal Rappies
				if ( r )
				{
					if ( l->episode == 0x01 )
						mm->rt_index = 6; // Al Rappy
					else
					{
						switch ( shipEvent )
						{
						case 0x01:
							mm->rt_index = 79; // St. Rappy
							break;
						case 0x04:
							mm->rt_index = 81; // Easter Rappy
							break;
						case 0x05:
							mm->rt_index = 80; // Halo Rappy
							break;
						default:
							mm->rt_index = 51; // Love Rappy
							break;
						}
					}
					mm->exp = l->bptable[0x19].XP;
				}
				else
				{
					mm->rt_index = 5;
					mm->exp = l->bptable[0x18].XP;
				}
			}
			break;
		case 66:
			// Monest + 30 Mothmants
			mm->exp = l->bptable[0x01].XP;
			mm->rt_index = 4;

			for (ch=0;ch<30;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 3;
				mm->exp = l->bptable[0x00].XP;
			}
			break;
		case 67:
			// Savage Wolf and Barbarous Wolf
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
			{
				mm->rt_index = 8;
				mm->exp = l->bptable[0x03].XP;
			}
			else
			{
				mm->rt_index = 7;
				mm->exp = l->bptable[0x02].XP;
			}
			break;
		case 68:
			// Booma family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 11;
				mm->exp = l->bptable[0x4D].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 10;
					mm->exp = l->bptable[0x4C].XP;
				}
				else
				{
					mm->rt_index = 9;
					mm->exp = l->bptable[0x4B].XP;
				}
			break;
		case 96:
			// Grass Assassin
			mm->rt_index = 12;
			mm->exp = l->bptable[0x4E].XP;
			break;
		case 97:
			// Del Lily, Poison Lily, Nar Lily
			r = 0;
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < lily_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( ( l->episode == 0x02 ) && ( aMob ) )
			{
				mm->rt_index = 83;
				mm->exp = l->bptable[0x25].XP;
			}
			else
				if ( r )
				{
					mm->rt_index = 14;
					mm->exp = l->bptable[0x05].XP;
				}
				else
				{
					mm->rt_index = 13;
					mm->exp = l->bptable[0x04].XP;
				}
			break;
		case 98:
			// Nano Dragon
			mm->rt_index = 15;
			mm->exp = l->bptable[0x1A].XP;
			break;
		case 99:
			// Shark family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 18;
				mm->exp = l->bptable[0x51].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 17;
					mm->exp = l->bptable[0x50].XP;
				}
				else
				{
					mm->rt_index = 16;
					mm->exp = l->bptable[0x4F].XP;
				}
			break;
		case 100:
			// Slime + 4 clones
			r = 0;
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < slime_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 20;
				mm->exp = l->bptable[0x2F].XP;
			}
			else
			{
				mm->rt_index = 19;
				mm->exp = l->bptable[0x30].XP;
			}
			for (ch=0;ch<4;ch++)
			{
				l->mapIndex++;
				mm++;
				r = 0;
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < slime_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
				if ( r )
				{
					mm->rt_index = 20;
					mm->exp = l->bptable[0x2F].XP;
				}
				else
				{
					mm->rt_index = 19;
					mm->exp = l->bptable[0x30].XP;
				}
			}
			break;
		case 101:
			// Pan Arms, Migium, Hidoom
			mm->rt_index = 21;
			mm->exp = l->bptable[0x31].XP;
			l->mapIndex++;
			mm++;
			mm->rt_index = 22;
			mm->exp = l->bptable[0x32].XP;
			l->mapIndex++;
			mm++;
			mm->rt_index = 23;
			mm->exp = l->bptable[0x33].XP;
			break;
		case 128:
			// Dubchic and Gilchic
			if (mm->skin & 0x01)
			{
				mm->exp = l->bptable[0x1C].XP;
				mm->rt_index = 50;
			}
			else
			{
				mm->exp = l->bptable[0x1B].XP;
				mm->rt_index = 24;
			}
			break;
		case 129:
			// Garanz
			mm->rt_index = 25;
			mm->exp = l->bptable[0x1D].XP;
			break;
		case 130:
			// Sinow Beat and Gold
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
			{
				mm->rt_index = 27;
				mm->exp = l->bptable[0x13].XP;
			}
			else
			{
				mm->rt_index = 26;
				mm->exp = l->bptable[0x06].XP;
			}

			if ( ( mm->reserved[0] >> 16 ) == 0 )  
				l->mapIndex += 4; // Add 4 clones but only if there's no add value there already...
			break;
		case 131:
			// Canadine
			mm->rt_index = 28;
			mm->exp = l->bptable[0x07].XP;
			break;
		case 132:
			// Canadine Group
			mm->rt_index = 29;
			mm->exp = l->bptable[0x09].XP;
			for (ch=0;ch<8;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 28;
				mm->exp = l->bptable[0x08].XP;
			}
			break;
		case 133:
			// Dubwitch
			break;
		case 160:
			// Delsaber
			mm->rt_index = 30;
			mm->exp = l->bptable[0x52].XP;
			break;
		case 161:
			// Chaos Sorcerer + 2 Bits
			mm->rt_index = 31;
			mm->exp = l->bptable[0x0A].XP;
			l->mapIndex += 2;
			break;
		case 162:
			// Dark Gunner
			mm->rt_index = 34;
			mm->exp = l->bptable[0x1E].XP;
			break;
		case 164:
			// Chaos Bringer
			mm->rt_index = 36;
			mm->exp = l->bptable[0x0D].XP;
			break;
		case 165:
			// Dark Belra
			mm->rt_index = 37;
			mm->exp = l->bptable[0x0E].XP;
			break;
		case 166:
			// Dimenian family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 43;
				mm->exp = l->bptable[0x55].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 42;
					mm->exp = l->bptable[0x54].XP;
				}
				else
				{
					mm->rt_index = 41;
					mm->exp = l->bptable[0x53].XP;
				}
			break;
		case 167:
			// Bulclaw + 4 claws
			mm->rt_index = 40;
			mm->exp = l->bptable[0x1F].XP;
			for (ch=0;ch<4;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 38;
				mm->exp = l->bptable[0x20].XP;
			}
			break;
		case 168:
			// Claw
			mm->rt_index = 38;
			mm->exp = l->bptable[0x20].XP;
			break;
		case 192:
			// Dragon or Gal Gryphon
			if ( l->episode == 0x01 )
			{
				mm->rt_index = 44;
				mm->exp = l->bptable[0x12].XP;
			}
			else
				if ( l->episode == 0x02 )
				{
					mm->rt_index = 77;
					mm->exp = l->bptable[0x1E].XP;
				}
			break;
		case 193:
			// De Rol Le
			mm->rt_index = 45;
			mm->exp = l->bptable[0x0F].XP;
			break;
		case 194:
			// Vol Opt form 1
			break;
		case 197:
			// Vol Opt form 2
			mm->rt_index = 46;
			mm->exp = l->bptable[0x25].XP;
			break;
		case 200:
			// Dark Falz + 510 Helpers
			mm->rt_index = 47;
			if (l->difficulty)
				mm->exp = l->bptable[0x38].XP; // Form 2
			else
				mm->exp = l->bptable[0x37].XP;

			for (ch=0;ch<510;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->base = 200;
				mm->exp = l->bptable[0x35].XP;
			}
			break;
		case 202:
			// Olga Flow
			mm->rt_index = 78;
			mm->exp = l->bptable[0x2C].XP;
			l->mapIndex += 512;
			break;
		case 203:
			// Barba Ray
			mm->rt_index = 73;
			mm->exp = l->bptable[0x0F].XP;
			l->mapIndex += 47;
			break;
		case 204:
			// Gol Dragon
			mm->rt_index = 76;
			mm->exp = l->bptable[0x12].XP;
			l->mapIndex += 5;
			break;
		case 212:
			// Sinow Berill & Spigell
			/* if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) */
			if ( mm->skin >= 0x01 ) // set rare?
			{
				mm->rt_index = 63;
				mm->exp = l->bptable [0x13].XP;
			}
			else
			{
				mm->rt_index = 62;
				mm->exp = l->bptable [0x06].XP;
			}
			l->mapIndex += 4; // Add 4 clones which are never used...
			break;
		case 213:
			// Merillia & Meriltas
			if ( mm->skin & 0x01 )
			{
				mm->rt_index = 53;
				mm->exp = l->bptable [0x4C].XP;
			}
			else
			{
				mm->rt_index = 52;
				mm->exp = l->bptable [0x4B].XP;
			}
			break;
		case 214:
			if ( mm->skin & 0x02 )
			{
				// Mericus
				mm->rt_index = 58;
				mm->exp = l->bptable [0x46].XP;
			}
			else 
				if ( mm->skin & 0x01 )
				{
					// Merikle
					mm->rt_index = 57;
					mm->exp = l->bptable [0x45].XP;
				}
				else
				{
					// Mericarol
					mm->rt_index = 56;
					mm->exp = l->bptable [0x3A].XP;
				}
			break;
		case 215:
			// Ul Gibbon and Zol Gibbon
			if ( mm->skin & 0x01 )
			{
				mm->rt_index = 60;
				mm->exp = l->bptable [0x3C].XP;
			}
			else
			{
				mm->rt_index = 59;
				mm->exp = l->bptable [0x3B].XP;
			}
			break;
		case 216:
			// Gibbles
			mm->rt_index = 61;
			mm->exp = l->bptable [0x3D].XP;
			break;
		case 217:
			// Gee
			mm->rt_index = 54;
			mm->exp = l->bptable [0x07].XP;
			break;
		case 218:
			// Gi Gue
			mm->rt_index = 55;
			mm->exp = l->bptable [0x1A].XP;
			break;
		case 219:
			// Deldepth
			mm->rt_index = 71;
			mm->exp = l->bptable [0x30].XP;
			break;
		case 220:
			// Delbiter
			mm->rt_index = 72;
			mm->exp = l->bptable [0x0D].XP;
			break;
		case 221:
			// Dolmolm and Dolmdarl
			if ( mm->skin & 0x01 )
			{
				mm->rt_index = 65;
				mm->exp = l->bptable[0x50].XP;
			}
			else
			{
				mm->rt_index = 64;
				mm->exp = l->bptable[0x4F].XP;
			}
			break;
		case 222:
			// Morfos
			mm->rt_index = 66;
			mm->exp = l->bptable [0x40].XP;
			break;
		case 223:
			// Recobox & Recons
			mm->rt_index = 67;
			mm->exp = l->bptable[0x41].XP;
			num_recons = ( mm->reserved[0] >> 16 );
			for (ch=0;ch<num_recons;ch++)
			{
				if ( l->mapIndex >= 0xB50 )
					break;
				l->mapIndex++;
				mm++;
				mm->rt_index = 68;
				mm->exp = l->bptable[0x42].XP;
			}
			break;
		case 224:
			if ( ( l->episode == 0x02 ) && ( aMob ) )
			{
				// Epsilon
				mm->rt_index = 84;
				mm->exp = l->bptable[0x23].XP;
				l->mapIndex += 4;
			}
			else
			{
				// Sinow Zoa and Zele
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 70;
					mm->exp = l->bptable[0x44].XP;
				}
				else
				{
					mm->rt_index = 69;
					mm->exp = l->bptable[0x43].XP;
				}
			}
			break;
		case 225:
			// Ill Gill
			mm->rt_index = 82;
			mm->exp = l->bptable[0x26].XP;
			break;
		case 272:
			// Astark
			mm->rt_index = 1;
			mm->exp = l->bptable[0x09].XP;
			break;
		case 273:
			// Satellite Lizard and Yowie
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
			{
				if ( aMob )
				{
					mm->rt_index = 2;
					mm->exp = l->bptable[0x1E].XP;
				}
				else
				{
					mm->rt_index = 2;
					mm->exp = l->bptable[0x0E].XP;
				}
			}
			else
			{
				if ( aMob )
				{
					mm->rt_index = 3;
					mm->exp = l->bptable[0x1D].XP;
				}
				else
				{
					mm->rt_index = 3;
					mm->exp = l->bptable[0x0D].XP;
				}
			}
			break;
		case 274:
			// Merissa A/AA
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < merissa_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 5;
				mm->exp = l->bptable[0x1A].XP;
			}
			else
			{
				mm->rt_index = 4;
				mm->exp = l->bptable[0x19].XP;
			}
			break;
		case 275:
			// Girtablulu
			mm->rt_index = 6;
			mm->exp = l->bptable[0x1F].XP;
			break;
		case 276:
			// Zu and Pazuzu
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < pazuzu_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				if ( aMob )
				{
					mm->rt_index = 8;
					mm->exp = l->bptable[0x1C].XP;
				}
				else
				{
					mm->rt_index = 8;
					mm->exp = l->bptable[0x08].XP;
				}
			}
			else
			{
				if ( aMob )
				{
					mm->rt_index = 7;
					mm->exp = l->bptable[0x1B].XP;
				}
				else
				{
					mm->rt_index = 7;
					mm->exp = l->bptable[0x07].XP;
				}
			}
			break;
		case 277:
			// Boota family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 11;			
				mm->exp = l->bptable [0x03].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 10;
					mm->exp = l->bptable [0x01].XP;
				}
				else
				{
					mm->rt_index = 9;
					mm->exp = l->bptable [0x00].XP;
				}
			break;
		case 278:
			// Dorphon and Eclair
			r = 0;
			if ( mm->skin & 0x01 ) // Set rare from a quest?
				r = 1;
			else
				if ( ( l->rareIndex < 0x1E ) && ( mt_lrand() < dorphon_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
			{
				mm->rt_index = 13;
				mm->exp = l->bptable [0x10].XP;
			}
			else
			{
				mm->rt_index = 12;
				mm->exp = l->bptable [0x0F].XP;
			}
			break;
		case 279:
			// Goran family
			if ( mm->skin & 0x02 )
			{
				mm->rt_index = 15;
				mm->exp = l->bptable [0x13].XP;
			}
			else
				if ( mm->skin & 0x01 )
				{
					mm->rt_index = 16;
					mm->exp = l->bptable [0x12].XP;
				}
				else
				{
					mm->rt_index = 14;
					mm->exp = l->bptable [0x11].XP;
				}
			break;
		case 281:
			// Saint Million, Shambertin, and Kondrieu
			r = 0;
			if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
				 ( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) // set rare?
				r = 1;
			else
				if ( ( l->rareIndex < 0x20 ) && ( mt_lrand() < kondrieu_rate ) )
				{
					*(unsigned short*) &l->rareData[l->rareIndex] = (unsigned short) l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ( r )
				mm->rt_index = 21;
			else
			{
				if ( mm->skin & 0x01 )
					mm->rt_index = 20;
				else
					mm->rt_index = 19;
			}
			mm->exp = l->bptable [0x22].XP;
			break;
		default:
			//debug ("enemy not handled: %u", mm->base);
			break;
		}
		if ( mm->reserved[0] >> 16 ) // Have to do
			l->mapIndex += ( mm->reserved[0] >> 16 );
		l->mapIndex++;
	}
}


void LoadObjectData (LOBBY* l, int unused, const char* filename)
{
	FILE* fp;
	unsigned oldIndex, num_records, ch, ch2;
	char new_file[256];

	if (!l) 
		return;

	memcpy (&new_file[0], filename, strlen (filename) + 1);

	if ( filename [ strlen ( filename ) - 5 ] == 101 )
		new_file [ strlen ( filename ) - 5 ] = 111; // change e to o

	//debug ("Loading object %s... current index: %u", new_file, l->objIndex);

	fp = fopen ( &new_file[0], "rb");
	if (!fp)
		WriteLog ("Could not load object data from %s\n", new_file);
	else
	{
		fseek  ( fp, 0, SEEK_END );
		num_records = ftell ( fp ) / 68;
		fseek  ( fp, 0, SEEK_SET );
		fread  ( &dp[0], 1, 68 * num_records, fp );
		fclose ( fp );
		oldIndex = l->objIndex;
		ch2 = 0;
		for (ch=0;ch<num_records;ch++)
		{
			if ( l->objIndex < 0xB50 )
			{
				memcpy (&l->objData[l->objIndex], &dp[ch2+0x28], 12);
				l->objData[l->objIndex].drop[3] = 0;
				l->objData[l->objIndex].drop[2] = dp[ch2+0x35];
				l->objData[l->objIndex].drop[1] = dp[ch2+0x36];
				l->objData[l->objIndex++].drop[0] = dp[ch2+0x37];
				ch2 += 68;
			}
			else
				break;
		}
		//debug ("Added %u objects, total: %u", l->objIndex - oldIndex, l->objIndex );
	}
};

void LoadMapData (LOBBY* l, int aMob, const char* filename)
{
	FILE* fp;
	unsigned oldIndex, num_records;

	if (!l) 
		return;

	//debug ("Loading map %s... current index: %u", filename, l->mapIndex);

	fp = fopen ( filename, "rb");
	if (!fp)
		WriteLog ("Could not load map data from %s\n", filename);
	else
	{
		fseek  ( fp, 0, SEEK_END );
		num_records = ftell ( fp ) / 72;
		fseek  ( fp, 0, SEEK_SET );
		fread  ( &dp[0], 1, sizeof ( MAP_MONSTER ) * num_records, fp );
		fclose ( fp );
		oldIndex = l->mapIndex;
		ParseMapData ( l, (MAP_MONSTER*) &dp[0], aMob, num_records );
		//debug ("Added %u mids, total: %u", l->mapIndex - oldIndex, l->mapIndex );
	}
};

void initialize_game (BANANA* client)
{
	LOBBY* l;
	unsigned ch;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;
	memset (l, 0, sizeof (LOBBY));

	l->difficulty = client->decryptbuf[0x50];
	l->battle = client->decryptbuf[0x51];
	l->challenge = client->decryptbuf[0x52];
	l->episode = client->decryptbuf[0x53];
	l->oneperson = client->decryptbuf[0x54];
	l->start_time = (unsigned) servertime;
	if (l->difficulty > 0x03)
		client->todc = 1;
	else
		if ((l->battle) && (l->challenge))
			client->todc = 1;
		else
			if (l->episode > 0x03)
				client->todc = 1;
			else
				if ((l->oneperson) && ((l->challenge) || (l->battle)))
					client->todc = 1;
	if (!client->todc)
	{
		if (l->battle)
			l->battle = 1;
		if (l->challenge)
			l->challenge = 1;
		if (l->oneperson)
			l->oneperson = 1;
		memcpy (&l->gameName[0], &client->decryptbuf[0x14], 30);
		memcpy (&l->gamePassword[0], &client->decryptbuf[0x30], 32);
		l->in_use = 1;
		l->gameMonster[0] = (unsigned char) mt_lrand() % 256;
		l->gameMonster[1] = (unsigned char) mt_lrand() % 256;
		l->gameMonster[2] = (unsigned char) mt_lrand() % 256;
		l->gameMonster[3] = (unsigned char) mt_lrand() % 16;
		memset (&l->gameMap[0], 0, 128);
		l->playerItemID[0] = 0x10000;
		l->playerItemID[1] = 0x210000;
		l->playerItemID[2] = 0x410000;
		l->playerItemID[3] = 0x610000;
		l->bankItemID[0] = 0x10000;
		l->bankItemID[1] = 0x210000;
		l->bankItemID[2] = 0x410000;
		l->bankItemID[3] = 0x610000;
		l->leader = 0;
		l->sectionID = client->character.sectionID;
		l->itemID = 0x810000;
		l->mapIndex = 0;
		memset (&l->mapData[0], 0, sizeof (l->mapData));
		l->rareIndex = 0;
		for (ch=0;ch<0x20;ch++)
			l->rareData[ch] = 0xFF;
		switch (l->episode)
		{
		case 0x01:
			// Episode 1
			if (!l->oneperson)
			{
				l->bptable = &ep1battle[0x60 * l->difficulty];

				LoadMapData ( l, 0, "map\\map_city00_00e.dat" );
				LoadObjectData ( l, 0, "map\\map_city00_00o.dat" );

				l->gameMap[12]=(unsigned char) mt_lrand() % 5; // Forest 1
				LoadMapData ( l, 0, Forest1_Online_Maps [l->gameMap[12]] );
				LoadObjectData ( l, 0, Forest1_Online_Maps [l->gameMap[12]] );

				l->gameMap[20]=(unsigned char) mt_lrand() % 5; // Forest 2
				LoadMapData ( l, 0, Forest2_Online_Maps [l->gameMap[20]] );
				LoadObjectData ( l, 0, Forest2_Online_Maps [l->gameMap[20]] );

				l->gameMap[24]=(unsigned char) mt_lrand() % 3; // Cave 1
				l->gameMap[28]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Cave1_Online_Maps [( l->gameMap[24] * 2 ) + l->gameMap[28]] );
				LoadObjectData ( l, 0, Cave1_Online_Maps [( l->gameMap[24] * 2 ) + l->gameMap[28]] );

				l->gameMap[32]=(unsigned char) mt_lrand() % 3; // Cave 2
				l->gameMap[36]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Cave2_Online_Maps [( l->gameMap[32] * 2 ) + l->gameMap[36]] );
				LoadObjectData ( l, 0, Cave2_Online_Maps [( l->gameMap[32] * 2 ) + l->gameMap[36]] );

				l->gameMap[40]=(unsigned char) mt_lrand() % 3; // Cave 3
				l->gameMap[44]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Cave3_Online_Maps [( l->gameMap[40] * 2 ) + l->gameMap[44]] );
				LoadObjectData ( l, 0, Cave3_Online_Maps [( l->gameMap[40] * 2 ) + l->gameMap[44]] );

				l->gameMap[48]=(unsigned char) mt_lrand() % 3; // Mine 1
				l->gameMap[52]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );
				LoadObjectData ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );

				l->gameMap[56]=(unsigned char) mt_lrand() % 3; // Mine 2
				l->gameMap[60]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );
				LoadObjectData ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );

				l->gameMap[64]=(unsigned char) mt_lrand() % 3; // Ruins 1
				l->gameMap[68]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );
				LoadObjectData ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72]=(unsigned char) mt_lrand() % 3; // Ruins 2
				l->gameMap[76]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );
				LoadObjectData ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );

				l->gameMap[80]=(unsigned char) mt_lrand() % 3; // Ruins 3
				l->gameMap[84]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
				LoadObjectData ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
			}
			else
			{
				l->bptable = &ep1battle_off[0x60 * l->difficulty];

				LoadMapData ( l, 0, "map\\map_city00_00e_s.dat");
				LoadObjectData ( l, 0, "map\\map_city00_00o_s.dat");

				l->gameMap[12]=(unsigned char) mt_lrand() % 3; // Forest 1
				LoadMapData ( l, 0, Forest1_Offline_Maps [l->gameMap[12]] );
				LoadObjectData ( l, 0, Forest1_Offline_Objects [l->gameMap[12]] );

				l->gameMap[20]=(unsigned char) mt_lrand() % 3; // Forest 2
				LoadMapData ( l, 0, Forest2_Offline_Maps [l->gameMap[20]] );
				LoadObjectData ( l, 0, Forest2_Offline_Objects [l->gameMap[20]] );

				l->gameMap[24]=(unsigned char) mt_lrand() % 3; // Cave 1
				LoadMapData ( l, 0, Cave1_Offline_Maps [l->gameMap[24]]);
				LoadObjectData ( l, 0, Cave1_Offline_Objects [l->gameMap[24]]);

				l->gameMap[32]=(unsigned char) mt_lrand() % 3; // Cave 2
				LoadMapData ( l, 0, Cave2_Offline_Maps [l->gameMap[32]]);
				LoadObjectData ( l, 0, Cave2_Offline_Objects [l->gameMap[32]]);

				l->gameMap[40]=(unsigned char) mt_lrand() % 3; // Cave 3
				LoadMapData ( l, 0, Cave3_Offline_Maps [l->gameMap[40]]);
				LoadObjectData ( l, 0, Cave3_Offline_Objects [l->gameMap[40]]);

				l->gameMap[48]=(unsigned char) mt_lrand() % 3; // Mine 1
				l->gameMap[52]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );
				LoadObjectData ( l, 0, Mine1_Online_Maps [( l->gameMap[48] * 2 ) + l->gameMap[52]] );

				l->gameMap[56]=(unsigned char) mt_lrand() % 3; // Mine 2
				l->gameMap[60]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );
				LoadObjectData ( l, 0, Mine2_Online_Maps [( l->gameMap[56] * 2 ) + l->gameMap[60]] );

				l->gameMap[64]=(unsigned char) mt_lrand() % 3; // Ruins 1
				l->gameMap[68]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );
				LoadObjectData ( l, 0, Ruins1_Online_Maps [( l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72]=(unsigned char) mt_lrand() % 3; // Ruins 2
				l->gameMap[76]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );
				LoadObjectData ( l, 0, Ruins2_Online_Maps [( l->gameMap[72] * 2 ) + l->gameMap[76]] );

				l->gameMap[80]=(unsigned char) mt_lrand() % 3; // Ruins 3
				l->gameMap[84]=(unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
				LoadObjectData ( l, 0, Ruins3_Online_Maps [( l->gameMap[80] * 2 ) + l->gameMap[84]] );
			}
			LoadMapData ( l, 0, "map\\map_boss01e.dat" );
			LoadObjectData ( l, 0, "map\\map_boss01o.dat");
			LoadMapData ( l, 0, "map\\map_boss02e.dat" );
			LoadObjectData ( l, 0, "map\\map_boss02o.dat");
			LoadMapData ( l, 0, "map\\map_boss03e.dat" );
			LoadObjectData ( l, 0, "map\\map_boss03o.dat");
			LoadMapData ( l, 0, "map\\map_boss04e.dat" );
			if ( l->oneperson )
				LoadObjectData ( l, 0, "map\\map_boss04_offo.dat");
			else
				LoadObjectData ( l, 0, "map\\map_boss04o.dat");
			break;
		case 0x02:
			// Episode 2
			if (!l->oneperson)
			{
				l->bptable = &ep2battle[0x60 * l->difficulty];
				LoadMapData ( l, 0, "map\\map_labo00_00e.dat");
				LoadObjectData ( l, 0, "map\\map_labo00_00o.dat");
				
				l->gameMap[8]  = (unsigned char) mt_lrand() % 2; // Temple 1
				l->gameMap[12] = 0x00;
				LoadMapData ( l, 0, Temple1_Online_Maps [l->gameMap[8]] );
				LoadObjectData ( l, 0, Temple1_Online_Maps [l->gameMap[8]] );

				l->gameMap[16] = (unsigned char) mt_lrand() % 2; // Temple 2
				l->gameMap[20] = 0x00;
				LoadMapData ( l, 0, Temple2_Online_Maps [l->gameMap[16]] );
				LoadObjectData ( l, 0, Temple2_Online_Maps [l->gameMap[16]] );

				l->gameMap[24] = (unsigned char) mt_lrand() % 2; // Spaceship 1
				l->gameMap[28] = 0x00;
				LoadMapData ( l, 0, Spaceship1_Online_Maps [l->gameMap[24]] );
				LoadObjectData ( l, 0, Spaceship1_Online_Maps [l->gameMap[24]] );

				l->gameMap[32] = (unsigned char) mt_lrand() % 2; // Spaceship 2
				l->gameMap[36] = 0x00; 
				LoadMapData ( l, 0, Spaceship2_Online_Maps [l->gameMap[32]] );
				LoadObjectData ( l, 0, Spaceship2_Online_Maps [l->gameMap[32]] );

				l->gameMap[40] = 0x00;
				l->gameMap[44] = (unsigned char) mt_lrand() % 3; // Jungle 1
				LoadMapData ( l, 0, Jungle1_Online_Maps [l->gameMap[44]] );
				LoadObjectData ( l, 0, Jungle1_Online_Maps [l->gameMap[44]] );

				l->gameMap[48] = 0x00;
				l->gameMap[52] = (unsigned char) mt_lrand() % 3; // Jungle 2
				LoadMapData ( l, 0, Jungle2_Online_Maps [l->gameMap[52]] );
				LoadObjectData ( l, 0, Jungle2_Online_Maps [l->gameMap[52]] );

				l->gameMap[56] = 0x00; 
				l->gameMap[60] = (unsigned char) mt_lrand() % 3; // Jungle 3
				LoadMapData ( l, 0, Jungle3_Online_Maps [l->gameMap[60]] );
				LoadObjectData ( l, 0, Jungle3_Online_Maps [l->gameMap[60]] );

				l->gameMap[64] = (unsigned char) mt_lrand() % 2; // Jungle 4
				l->gameMap[68] = (unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Jungle4_Online_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );
				LoadObjectData ( l, 0, Jungle4_Online_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72] = 0x00;
				l->gameMap[76] = (unsigned char) mt_lrand() % 3; // Jungle 5
				LoadMapData ( l, 0, Jungle5_Online_Maps [l->gameMap[76]] );
				LoadObjectData ( l, 0, Jungle5_Online_Maps [l->gameMap[76]] );

				l->gameMap[80] = (unsigned char) mt_lrand() % 2; // Seabed 1
				l->gameMap[84] = (unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Seabed1_Online_Maps [(l->gameMap[80] * 2 ) + l->gameMap[84]] );
				LoadObjectData ( l, 0, Seabed1_Online_Maps [(l->gameMap[80] * 2 ) + l->gameMap[84]] );

				l->gameMap[88] = (unsigned char) mt_lrand() % 2; // Seabed 2
				l->gameMap[92] = (unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Seabed2_Online_Maps [(l->gameMap[88] * 2 ) + l->gameMap[92]] );
				LoadObjectData ( l, 0, Seabed2_Online_Maps [(l->gameMap[88] * 2 ) + l->gameMap[92]] );
			}
			else
			{
				l->bptable = &ep2battle_off[0x60 * l->difficulty];
				LoadMapData ( l, 0, "map\\map_labo00_00e_s.dat");
				LoadObjectData ( l, 0, "map\\map_labo00_00o_s.dat");
				
				l->gameMap[8]  = (unsigned char) mt_lrand() % 2; // Temple 1
				l->gameMap[12] = 0x00;
				LoadMapData ( l, 0, Temple1_Offline_Maps [l->gameMap[8]] );
				LoadObjectData ( l, 0, Temple1_Offline_Maps [l->gameMap[8]] );

				l->gameMap[16] = (unsigned char) mt_lrand() % 2; // Temple 2
				l->gameMap[20] = 0x00;
				LoadMapData ( l, 0, Temple2_Offline_Maps [l->gameMap[16]] );
				LoadObjectData ( l, 0, Temple2_Offline_Maps [l->gameMap[16]] );

				l->gameMap[24] = (unsigned char) mt_lrand() % 2; // Spaceship 1
				l->gameMap[28] = 0x00;
				LoadMapData ( l, 0, Spaceship1_Offline_Maps [l->gameMap[24]] );
				LoadObjectData ( l, 0, Spaceship1_Offline_Maps [l->gameMap[24]] );

				l->gameMap[32] = (unsigned char) mt_lrand() % 2; // Spaceship 2
				l->gameMap[36] = 0x00; 
				LoadMapData ( l, 0, Spaceship2_Offline_Maps [l->gameMap[32]] );
				LoadObjectData ( l, 0, Spaceship2_Offline_Maps [l->gameMap[32]] );

				l->gameMap[40] = 0x00;
				l->gameMap[44] = (unsigned char) mt_lrand() % 3; // Jungle 1
				LoadMapData ( l, 0, Jungle1_Offline_Maps [l->gameMap[44]] );
				LoadObjectData ( l, 0, Jungle1_Offline_Maps [l->gameMap[44]] );

				l->gameMap[48] = 0x00;
				l->gameMap[52] = (unsigned char) mt_lrand() % 3; // Jungle 2
				LoadMapData ( l, 0, Jungle2_Offline_Maps [l->gameMap[52]] );
				LoadObjectData ( l, 0, Jungle2_Offline_Maps [l->gameMap[52]] );

				l->gameMap[56] = 0x00; 
				l->gameMap[60] = (unsigned char) mt_lrand() % 3; // Jungle 3
				LoadMapData ( l, 0, Jungle3_Offline_Maps [l->gameMap[60]] );
				LoadObjectData ( l, 0, Jungle3_Offline_Maps [l->gameMap[60]] );

				l->gameMap[64] = (unsigned char) mt_lrand() % 2; // Jungle 4
				l->gameMap[68] = (unsigned char) mt_lrand() % 2;
				LoadMapData ( l, 0, Jungle4_Offline_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );
				LoadObjectData ( l, 0, Jungle4_Offline_Maps [(l->gameMap[64] * 2 ) + l->gameMap[68]] );

				l->gameMap[72] = 0x00;
				l->gameMap[76] = (unsigned char) mt_lrand() % 3; // Jungle 5
				LoadMapData ( l, 0, Jungle5_Offline_Maps [l->gameMap[76]] );
				LoadObjectData ( l, 0, Jungle5_Offline_Maps [l->gameMap[76]] );

				l->gameMap[80] = (unsigned char) mt_lrand() % 2; // Seabed 1
				LoadMapData ( l, 0, Seabed1_Offline_Maps [l->gameMap[80]] );
				LoadObjectData ( l, 0, Seabed1_Offline_Maps [l->gameMap[80]] );

				l->gameMap[88] = (unsigned char) mt_lrand() % 2; // Seabed 2
				LoadMapData ( l, 0, Seabed2_Offline_Maps [l->gameMap[88]] );
				LoadObjectData ( l, 0, Seabed2_Offline_Maps [l->gameMap[88]] );
			}
			LoadMapData ( l, 0, "map\\map_boss05e.dat");
			LoadMapData ( l, 0, "map\\map_boss06e.dat");
			LoadMapData ( l, 0, "map\\map_boss07e.dat");
			LoadMapData ( l, 0, "map\\map_boss08e.dat");
			if ( l->oneperson )
			{
				LoadObjectData ( l, 0, "map\\map_boss05_offo.dat");
				LoadObjectData ( l, 0, "map\\map_boss06_offo.dat");
				LoadObjectData ( l, 0, "map\\map_boss07_offo.dat");
				LoadObjectData ( l, 0, "map\\map_boss08_offo.dat");
			}
			else
			{
				LoadObjectData ( l, 0, "map\\map_boss05o.dat");
				LoadObjectData ( l, 0, "map\\map_boss06o.dat");
				LoadObjectData ( l, 0, "map\\map_boss07o.dat");
				LoadObjectData ( l, 0, "map\\map_boss08o.dat");
			}
			break;
		case 0x03:
			// Episode 4
			if (!l->oneperson)
			{
				l->bptable = &ep4battle[0x60 * l->difficulty];
				LoadMapData (l, 0, "map\\map_city02_00_00e.dat");
				LoadObjectData (l, 0, "map\\map_city02_00_00o.dat");
			}
			else
			{
				l->bptable = &ep4battle_off[0x60 * l->difficulty];
				LoadMapData (l, 0, "map\\map_city02_00_00e_s.dat");
				LoadObjectData (l, 0, "map\\map_city02_00_00o_s.dat");
			}

			l->gameMap[12] = (unsigned char) mt_lrand() % 3; // Crater East
			LoadMapData ( l, 0, Crater_East_Online_Maps [l->gameMap[12]] );
			LoadObjectData ( l, 0, Crater_East_Online_Maps [l->gameMap[12]] );

			l->gameMap[20] = (unsigned char) mt_lrand() % 3; // Crater West
			LoadMapData ( l, 0, Crater_West_Online_Maps [l->gameMap[20]] );
			LoadObjectData ( l, 0, Crater_West_Online_Maps [l->gameMap[20]] );

			l->gameMap[28] = (unsigned char) mt_lrand() % 3; // Crater South
			LoadMapData ( l, 0, Crater_South_Online_Maps [l->gameMap[28]] );
			LoadObjectData ( l, 0, Crater_South_Online_Maps [l->gameMap[28]] );

			l->gameMap[36] = (unsigned char) mt_lrand() % 3; // Crater North
			LoadMapData ( l, 0, Crater_North_Online_Maps [l->gameMap[36]] );
			LoadObjectData ( l, 0, Crater_North_Online_Maps [l->gameMap[36]] );

			l->gameMap[44] = (unsigned char) mt_lrand() % 3; // Crater Interior
			LoadMapData ( l, 0, Crater_Interior_Online_Maps [l->gameMap[44]] );
			LoadObjectData ( l, 0, Crater_Interior_Online_Maps [l->gameMap[44]] );

			l->gameMap[48] = (unsigned char) mt_lrand() % 3; // Desert 1
			LoadMapData ( l, 1, Desert1_Online_Maps [l->gameMap[48]] );
			LoadObjectData ( l, 1, Desert1_Online_Maps [l->gameMap[48]] );

			l->gameMap[60] = (unsigned char) mt_lrand() % 3; // Desert 2
			LoadMapData ( l, 1, Desert2_Online_Maps [l->gameMap[60]] );
			LoadObjectData ( l, 1, Desert2_Online_Maps [l->gameMap[60]] );

			l->gameMap[64] = (unsigned char) mt_lrand() % 3; // Desert 3
			LoadMapData ( l, 1, Desert3_Online_Maps [l->gameMap[64]] );
			LoadObjectData ( l, 1, Desert3_Online_Maps [l->gameMap[64]] );

			LoadMapData (l, 0, "map\\map_boss09_00_00e.dat");
			LoadObjectData (l, 0, "map\\map_boss09_00_00o.dat");
			//LoadMapData (l, "map\\map_test01_00_00e.dat");
			break;
		default:
			break;
		}
	}
	else
		Send1A ("Bad game arguments supplied.", client);
}

void Send64 (BANANA* client)
{
	LOBBY* l;
	unsigned Offset;
	unsigned ch;

	if (!client->lobby)
		return;
	l = (LOBBY*) client->lobby;

	for (ch=0;ch<4;ch++)
	{
		if (!l->slot_use[ch])
		{
			l->slot_use[ch] = 1; // Slot now in use
			l->client[ch] = client;
			// lobbyNum should be set before joining the game
			client->clientID = ch;
			l->gamePlayerCount++;
			l->gamePlayerID[ch] = l->gamePlayerCount;
			break;
		}
	}
	l->lobbyCount = 0;
	for (ch=0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
			l->lobbyCount++;
	}
	memset (&PacketData[0], 0, 0x1A8);
	PacketData[0x00] = 0xA8;
	PacketData[0x01] = 0x01;
	PacketData[0x02] = 0x64;
	PacketData[0x04] = (unsigned char) l->lobbyCount;
	memcpy (&PacketData[0x08], &l->gameMap[0], 128 );
	Offset = 0x88;
	for (ch=0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
		{
			PacketData[Offset+2] = 0x01;
			Offset += 0x04;
			*(unsigned *) &PacketData[Offset] = l->client[ch]->guildcard;
			Offset += 0x10;
			PacketData[Offset] = l->client[ch]->clientID;
			Offset += 0x0C;
			memcpy (&PacketData[Offset], &l->client[ch]->character.name[0], 24);
			Offset += 0x20;
			PacketData[Offset] = 0x02;
			Offset += 0x04;
			if ((l->client[ch]->guildcard == client->guildcard) && (l->lobbyCount > 1))
			{
				memset (&PacketData2[0], 0, 1316);
				PacketData2[0x00] = 0x34;
				PacketData2[0x01] = 0x05;
				PacketData2[0x02] = 0x65;
				PacketData2[0x04] = 0x01;
				PacketData2[0x08] = client->clientID;
				PacketData2[0x09] = l->leader;
				PacketData2[0x0A] = 0x01; // ??
				PacketData2[0x0B] = 0xFF; // ??
				PacketData2[0x0C] = 0x01; // ??
				PacketData2[0x0E] = 0x01; // ??
				PacketData2[0x16] = 0x01;
				*(unsigned *) &PacketData2[0x18] = client->guildcard;
				PacketData2[0x30] = client->clientID;
				memcpy (&PacketData2[0x34], &client->character.name[0], 24);
				PacketData2[0x54] = 0x02; // ??
				memcpy (&PacketData2[0x58], &client->character.inventoryUse, 0x4DC);
				// Prevent crashing with NPC skins...
				if (client->character.skinFlag)
					memset (&PacketData2[0x58+0x3A8], 0, 10);
				SendToLobby ( client->lobby, 4, &PacketData2[0x00], 1332, client->guildcard );
			}
		}
	}

	if (l->lobbyCount < 4)
		PacketData[0x194] = 0x02;
	if (l->lobbyCount < 3)
		PacketData[0x150] = 0x02;
	if (l->lobbyCount < 2)
		PacketData[0x10C] = 0x02;

	// Most of the 0x64 packet has been generated... now for the important stuff. =p
	// Leader ID @ 0x199
	// Difficulty @ 0x19B
	// Event @ 0x19D
	// Section ID of Leader @ 0x19E
	// Game Monster @ 0x1A0 (4 bytes)
	// Episode @ 0x1A4
	// 0x01 @ 0x1A5
	// One-person @ 0x1A6

	PacketData[0x198] = client->clientID;
	PacketData[0x199] = l->leader;
	PacketData[0x19B] = l->difficulty;
	PacketData[0x19C] = l->battle;
	if ((shipEvent < 7) && (shipEvent != 0x02))
		PacketData[0x19D] = shipEvent;
	else
		PacketData[0x19D] = 0;
	PacketData[0x19E] = l->sectionID;
	PacketData[0x19F] = l->challenge;
	*(unsigned *) &PacketData[0x1A0] = *(unsigned *) &l->gameMonster;
	PacketData[0x1A4] = l->episode;
	PacketData[0x1A5] = 0x01; // ??
	PacketData[0x1A6] = l->oneperson;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], 0x1A8);

	/* Let's send the team data... */

	SendToLobby ( client->lobby, 4, MakePacketEA15 ( client ), 2152, client->guildcard );

	client->bursting = 1;
}

void Send67 (BANANA* client, unsigned char preferred)
{
	BLOCK* b;
	BANANA* lClient;
	LOBBY* l;
	unsigned Offset = 0, Offset2 = 0;
	unsigned ch, ch2;

	if (!client->lobbyOK)
		return;

	client->lobbyOK = 0;

	ch = 0;
	
	b = blocks[client->block - 1];
	if ((preferred != 0xFF) && (preferred < 0x0F))
	{
		if (b->lobbies[preferred].lobbyCount >= 12)
			preferred = 0x00;
		ch = preferred;
	}

	for (ch=ch;ch<15;ch++)
	{
		l = &b->lobbies[ch];
		if (l->lobbyCount < 12)
		{
			for (ch2=0;ch2<12;ch2++)
				if (l->slot_use[ch2] == 0)
				{
					l->slot_use[ch2] = 1;
					l->client[ch2] = client;
					l->arrow_color[ch2] = 0;
					client->lobbyNum = ch + 1;
					client->lobby = (void*) &b->lobbies[ch];
					client->clientID = ch2;
					break;
				}
				// Send68 here with joining client (use ch2 for clientid)
				l->lobbyCount = 0;
				for (ch2=0;ch2<12;ch2++)
				{
					if ((l->slot_use[ch2]) && (l->client[ch2]))
						l->lobbyCount++;
				}

				memset (&PacketData[0x00], 0, 0x10);
				PacketData[0x04] = l->lobbyCount;
				PacketData[0x08] = client->clientID;
				PacketData[0x0B] = ch;
				PacketData[0x0C] = client->block;
				PacketData[0x0E] = shipEvent;
				Offset = 0x16;
				for (ch2=0;ch2<12;ch2++)
				{
					if ((l->slot_use[ch2]) && (l->client[ch2]))
					{
						memset (&PacketData[Offset], 0, 1316);
						Offset2 = Offset;
						PacketData[Offset++] = 0x01;
						PacketData[Offset++] = 0x00;
						lClient = l->client[ch2];
						*(unsigned *) &PacketData[Offset] = lClient->guildcard;
						Offset += 24;
						*(unsigned *) &PacketData[Offset] = ch2;
						Offset += 4;
						memcpy (&PacketData[Offset], &lClient->character.name[0], 24);
						Offset += 32;
						PacketData[Offset++] = 0x02;
						Offset += 3;
						memcpy (&PacketData[Offset], &lClient->character.inventoryUse, 1246);
						// Prevent crashing with NPCs
						if (lClient->character.skinFlag)
							memset (&PacketData[Offset+0x3A8], 0, 10);
						Offset += 1246;
						if (lClient->isgm == 1)
							*(unsigned *) &PacketData[Offset2 + 0x3CA] = globalName;
						else
							if (isLocalGM(lClient->guildcard))
								*(unsigned *) &PacketData[Offset2 + 0x3CA] = localName;
							else
								*(unsigned *) &PacketData[Offset2 + 0x3CA] = normalName;
						if ((lClient->guildcard == client->guildcard) && (l->lobbyCount > 1))
						{
							memcpy (&PacketData2[0x00], &PacketData[0], 0x16 );
							PacketData2[0x00] = 0x34;
							PacketData2[0x01] = 0x05;
							PacketData2[0x02] = 0x68;
							PacketData2[0x04] = 0x01;
							memcpy (&PacketData2[0x16], &PacketData[Offset2], 1316 );
							SendToLobby ( client->lobby, 12, &PacketData2[0x00], 1332, client->guildcard );
						}
					}
				}
				*(unsigned short*) &PacketData[0] = (unsigned short) Offset;
				PacketData[2] = 0x67;
				break;
		}
	}

	if (Offset > 0)
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketData[0], Offset);
	}

	// Quest data

	memset (&client->encryptbuf[0x00], 0, 8);
	client->encryptbuf[0] = 0x10;
	client->encryptbuf[1] = 0x02;
	client->encryptbuf[2] = 0x60;
	client->encryptbuf[8] = 0x6F;
	client->encryptbuf[9] = 0x84;
	memcpy (&client->encryptbuf[0x0C], &client->character.quest_data1[0], 0x210 );
	memset (&client->encryptbuf[0x20C], 0, 4);
	encryptcopy (client, &client->encryptbuf[0x00], 0x210);

	/* Let's send the team data... */

	SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, client->guildcard );

	client->bursting = 1;
}

void Send95 (BANANA* client)
{
	client->lobbyOK = 1;
	memset (&client->encryptbuf[0x00], 0, 8);
	client->encryptbuf[0x00] = 0x08;
	client->encryptbuf[0x02] = 0x95;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &client->encryptbuf[0], 8);
	// Restore permanent character...
	if (client->character_backup)
	{
		if (client->mode)
		{
			memcpy (&client->character, client->character_backup, sizeof (client->character));
			client->mode = 0;
		}
		free (client->character_backup);
		client->character_backup = NULL;
	}
}

int qflag (unsigned char* flag_data, unsigned flag, unsigned difficulty)
{
	if (flag_data[(difficulty * 0x80) + ( flag >> 3 )] & (1 << (7 - ( flag & 0x07 ))))
		return 1;
	else
		return 0;
}

int qflag_ep1solo(unsigned char* flag_data, unsigned difficulty)
{
	int i;
	unsigned int quest_flag;

	for(i=1;i<=25;i++)
	{
		quest_flag = 0x63 + (i << 1);
		if(!qflag (flag_data, quest_flag, difficulty)) return 0;
	}
	return 1;
}

void SendA2 (unsigned char episode, unsigned char solo, unsigned char category, unsigned char gov, BANANA* client)
{
	QUEST_MENU* qm = 0;
	QUEST* q;
	unsigned char qc = 0;
	unsigned short Offset;
	unsigned ch,ch2,ch3,show_quest,quest_flag;
	unsigned char quest_count;
	char quest_num[16];
	int qn, tier1, ep1solo;
	LOBBY* l;
	unsigned char diff;

	if (!client->lobby)
		return;

	l = (LOBBY *) client->lobby;

	memset (&PacketData[0], 0, 0x2510);

	diff = l->difficulty;

	if ( l->battle )
	{
		qm = &quest_menus[9];
		qc = 10;
	}
	else
		if ( l->challenge )
		{
		}
		else
		{
			switch ( episode )
			{
			case 0x01:
				if ( gov )
				{
					qm = &quest_menus[6];
					qc = 7;
				}
				else
				{
					if ( solo )
					{
						qm = &quest_menus[3];
						qc = 4;
					}
					else
					{
						qm = &quest_menus[0];
						qc = 1;
					}
				}
				break;
			case 0x02:
				if ( gov )
				{
					qm = &quest_menus[7];
					qc = 8;
				}
				else
				{
					if ( solo )
					{
						qm = &quest_menus[4];
						qc = 5;
					}
					else
					{
						qm = &quest_menus[1];
						qc = 2;
					}
				}
				break;
			case 0x03:
				if ( gov )
				{
					qm = &quest_menus[8];
					qc = 9;
				}
				else
				{
					if ( solo )
					{
						qm = &quest_menus[5];
						qc = 6;
					}
					else
					{
						qm = &quest_menus[2];
						qc = 3;
					}
				}
				break;
			}
		}

	if ((qm) && (category == 0))
	{
		PacketData[0x02] = 0xA2;
		PacketData[0x04] = qm->num_categories;
		Offset = 0x08;
		for (ch=0;ch<qm->num_categories;ch++)
		{
			PacketData[Offset+0x07]	= 0x0F;
			PacketData[Offset]		= qc;
			PacketData[Offset+2]	= gov;
			PacketData[Offset+4]	= ch + 1;
			memcpy (&PacketData[Offset+0x08], &qm->c_names[ch][0], 0x40);
			memcpy (&PacketData[Offset+0x48], &qm->c_desc[ch][0], 0xF0);
			Offset += 0x13C;
		}
	}
	else
	{
		ch2 = 0;
		PacketData[0x02] = 0xA2;
		category--;
		quest_count = 0;
		Offset = 0x08;
		ep1solo = qflag_ep1solo(&client->character.quest_data1[0], diff);
		for (ch=0;ch<qm->quest_counts[category];ch++)
		{
			q = &quests[qm->quest_indexes[category][ch]];
			show_quest = 0;
			if ((solo) && (episode == 0x01))
			{
				memcpy (&quest_num[0], &q->ql[0]->qdata[49], 3);
				quest_num[4] = 0;
				qn = atoi (&quest_num[0]);
				if ((ep1solo) || (qn > 26))
					show_quest = 1;
				if (!show_quest)
				{
					quest_flag = 0x63 + (qn << 1);
					if (qflag(&client->character.quest_data1[0], quest_flag, diff))
						show_quest = 2; // Sets a variance if they've cleared the quest...
					else
					{
						tier1 = 0;
						if ( (qflag(&client->character.quest_data1[0],0x65,diff)) && // Cleared first tier
							 (qflag(&client->character.quest_data1[0],0x67,diff)) &&
							 (qflag(&client->character.quest_data1[0],0x6B,diff)) )
							 tier1 = 1;
						if (qflag(&client->character.quest_data1[0], quest_flag, diff) == 0)
						{ // When the quest hasn't been completed...
							// Forest quests
							switch (qn)
							{
							case 4: // Battle Training
							case 2: // Claiming a Stake
							case 1: // Magnitude of Metal
								show_quest = 1;
								break;
							case 5: // Journalistic Pursuit
							case 6: // The Fake In Yellow
							case 7: // Native Research
							case 9: // Gran Squall
								if (tier1)
									show_quest = 1;
								break;
							case 8: // Forest of Sorrow
								if (qflag(&client->character.quest_data1[0],0x71,diff)) // Cleared Native Research
									show_quest = 1;
								break;
							case 26: // Central Dome Fire Swirl
								if (qflag(&client->character.quest_data1[0],0x73,diff)) // Cleared Forest of Sorrow
									show_quest = 1;
								break;
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0],0x1F9,diff)))
							{
								// Cave quests (shown after Dragon is defeated)
								switch (qn)
								{
								case 03: // The Value of Money
								case 11: // The Lost Bride
								case 14: // Secret Delivery
								case 17: // Grave's Butler
								case 10: // Addicting Food
									show_quest = 1; // Always shown if first tier was cleared
									break;
								case 12: // Waterfall Tears
								case 15: // Soul of a Blacksmith
									if ( (qflag(&client->character.quest_data1[0],0x77,diff)) && // Cleared Addicting Food
										 (qflag(&client->character.quest_data1[0],0x79,diff)) && // Cleared The Lost Bride
										 (qflag(&client->character.quest_data1[0],0x7F,diff)) && // Cleared Secret Delivery
										 (qflag(&client->character.quest_data1[0],0x85,diff)) )  // Cleared Grave's Butler
										 show_quest = 1;
									break;
								case 13: // Black Paper
									if (qflag(&client->character.quest_data1[0],0x7B,diff)) // Cleared Waterfall Tears
										show_quest = 1;
									break;
								}
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0],0x1FF,diff)))
							{
								// Mine quests (shown after De Rol Le is defeated)
								switch (qn)
								{
								case 16: // Letter from Lionel
								case 18: // Knowing One's Heart
								case 20: // Dr. Osto's Research
									show_quest = 1; // Always shown if first tier was cleared
									break;
								case 21: // The Unsealed Door
									if ( (qflag(&client->character.quest_data1[0],0x8B,diff)) && // Cleared Dr. Osto's Research
										 (qflag(&client->character.quest_data1[0],0x7F,diff)) )  // Cleared Secret Delivery
										show_quest = 1;
									break;
								}
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0],0x207,diff)))
							{
								// Ruins quests (shown after Vol Opt is defeated)
								switch (qn)
								{
								case 19: // The Retired Hunter
								case 24: // Seek My Master
									show_quest = 1;  // Always shown if first tier was cleared
									break;
								case 25: // From the Depths
								case 22: // Soul of Steel
									if (qflag(&client->character.quest_data1[0],0x91,diff)) // Cleared Doc's Secret Plan
										show_quest = 1;
									break;
								case 23: // Doc's Secret Plan
									if (qflag(&client->character.quest_data1[0],0x7F,diff)) // Cleared Secret Delivery
										show_quest = 1;
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				show_quest = 1;
				if ((ch) && (gov))
				{
					// Check party's qualification for quests...
					switch (episode)
					{
					case 0x01:
						quest_flag = (0x1F3 + (ch << 1));
						for (ch2=0x1F5;ch2<=quest_flag;ch2+=2)
							for (ch3=0;ch3<4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
								show_quest = 0;
						break;
					case 0x02:
						quest_flag = (0x211 + (ch << 1));
						for (ch2=0x213;ch2<=quest_flag;ch2+=2)
							for (ch3=0;ch3<4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
								show_quest = 0;
						break;
					case 0x03:
						quest_flag = (0x2BC + ch);
						for (ch2=0x2BD;ch2<=quest_flag;ch2++)
							for (ch3=0;ch3<4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
								show_quest = 0;
						break;
					}
				}
			}
			if (show_quest)
			{
				PacketData[Offset+0x07] = 0x0F;
				PacketData[Offset]      = qc;
				PacketData[Offset+1]	= 0x01;
				PacketData[Offset+2]    = gov;
				PacketData[Offset+3]    = ((unsigned char) qm->quest_indexes[category][ch]) + 1;
				PacketData[Offset+4]    = category;
				if ((client->character.lang < 10) && (q->ql[client->character.lang]))
				  {
					memcpy (&PacketData[Offset+0x08], &q->ql[client->character.lang]->qname[0], 0x40);
					memcpy (&PacketData[Offset+0x48], &q->ql[client->character.lang]->qsummary[0], 0xF0);
				  }
				else
				  {
					memcpy (&PacketData[Offset+0x08], &q->ql[0]->qname[0], 0x40);
					memcpy (&PacketData[Offset+0x48], &q->ql[0]->qsummary[0], 0xF0);
				  }

				if ((solo) && (episode == 0x01))
				{
					if (qn <= 26)
					{
						*(unsigned short*) &PacketData[Offset+0x128] = ep1_unlocks[(qn-1)*2];
						*(unsigned short*) &PacketData[Offset+0x12C] = ep1_unlocks[((qn-1)*2)+1];
						*(int*) &PacketData[Offset+0x130] = qn;
						if ( show_quest == 2 )
							PacketData[Offset + 0x138] = 1;
					}
				}
				Offset += 0x13C;
				quest_count++;
			}
		}
		PacketData[0x04] = quest_count;
	}
	*(unsigned short*) &PacketData[0] = (unsigned short) Offset;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], Offset);
}

void SendA0 (BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketA0Data[0], *(unsigned short *) &PacketA0Data[0]);
}


void Send07 (BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &Packet07Data[0], *(unsigned short *) &Packet07Data[0]);
}


void SendB0 (const char *mes, BANANA* client)
{
	unsigned short xB0_Len;

	memcpy (&PacketData[0], &PacketB0[0], sizeof (PacketB0));
	xB0_Len = sizeof (PacketB0);

	while (*mes != 0x00)
	{
		PacketData[xB0_Len++] = *(mes++);
		PacketData[xB0_Len++] = 0x00;
	}

	PacketData[xB0_Len++] = 0x00;
	PacketData[xB0_Len++] = 0x00;

	while (xB0_Len % 8)
		PacketData[xB0_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xB0_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], xB0_Len);
}


void BroadcastToAll (unsigned short *mes, BANANA* client)
{
	unsigned short xEE_Len;
	unsigned short *pd;
	unsigned short *cname;
	unsigned ch, connectNum;

	memcpy (&PacketData[0], &PacketEE[0], sizeof (PacketEE));
	xEE_Len = sizeof (PacketEE);

	pd = (unsigned short*) &PacketData[xEE_Len];
	cname = (unsigned short*) &client->character.name[4];

	*(pd++) = 91;
	*(pd++) = 71;
	*(pd++) = 77;
	*(pd++) = 93;

	xEE_Len += 8;

	while (*cname != 0x0000)
	{
		*(pd++) = *(cname++);
		xEE_Len += 2;
	}

	*(pd++) = 58;
	*(pd++) = 32;

	xEE_Len += 4;

	while (*mes != 0x0000)
	{
		if (*mes == 0x0024)
		{
			*(pd++) = 0x0009;
			mes++;
		}
		else
		{
			*(pd++) = *(mes++);
		}
		xEE_Len += 2;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xEE_Len;
	if (client->announce == 1)
	{
		// Local announcement
		for (ch=0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			if (connections[connectNum]->guildcard)
			{
				cipher_ptr = &connections[connectNum]->server_cipher;
				encryptcopy (connections[connectNum], &PacketData[0], xEE_Len);
			}	
		}
	}
	else
	{
		// Global announcement
		if ((logon) && (logon->sockfd >= 0))
		{
			// Send the announcement to the logon server.
			logon->encryptbuf[0x00] = 0x12;
			logon->encryptbuf[0x01] = 0x00;
			*(unsigned *) &logon->encryptbuf[0x02] = client->guildcard;
			memcpy (&logon->encryptbuf[0x06], &PacketData[sizeof(PacketEE)], xEE_Len - sizeof (PacketEE));
			compressShipPacket (logon, &logon->encryptbuf[0x00], 6 + xEE_Len - sizeof (PacketEE));
		}
	}
	client->announce = 0;
}

void GlobalBroadcast (unsigned short *mes)
{
	unsigned short xEE_Len;
	unsigned short *pd;
	unsigned ch, connectNum;

	memcpy (&PacketData[0], &PacketEE[0], sizeof (PacketEE));
	xEE_Len = sizeof (PacketEE);

	pd = (unsigned short*) &PacketData[xEE_Len];

	while (*mes != 0x0000)
	{
		*(pd++) = *(mes++);
		xEE_Len += 2;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xEE_Len;
	for (ch=0;ch<serverNumConnections;ch++)
	{
		connectNum = serverConnectionList[ch];
		if (connections[connectNum]->guildcard)
		{
			cipher_ptr = &connections[connectNum]->server_cipher;
			encryptcopy (connections[connectNum], &PacketData[0], xEE_Len);
		}	
	}
}


void SendEE (const char *mes, BANANA* client)
{
	unsigned short xEE_Len;

	memcpy (&PacketData[0], &PacketEE[0], sizeof (PacketEE));
	xEE_Len = sizeof (PacketEE);

	while (*mes != 0x00)
	{
		PacketData[xEE_Len++] = *(mes++);
		PacketData[xEE_Len++] = 0x00;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*) &PacketData[0] = xEE_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], xEE_Len);
}

void Send19 (unsigned char ip1, unsigned char ip2, unsigned char ip3, unsigned char ip4, unsigned short ipp, BANANA* client)
{
	memcpy ( &client->encryptbuf[0], &Packet19, sizeof (Packet19));
	client->encryptbuf[0x08] = ip1;
	client->encryptbuf[0x09] = ip2;
	client->encryptbuf[0x0A] = ip3;
	client->encryptbuf[0x0B] = ip4;
	*(unsigned short*) &client->encryptbuf[0x0C] = ipp;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &client->encryptbuf[0], sizeof (Packet19));
}

void SendEA (unsigned char command, BANANA* client)
{
	switch (command)
	{
	case 0x02:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x02;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x04:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x04;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x0E:
		memset (&client->encryptbuf[0x00], 0, 0x38);
		client->encryptbuf[0x00] = 0x38;
		client->encryptbuf[0x01] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x0E;
		*(unsigned *) &client->encryptbuf[0x08] = client->guildcard;
		*(unsigned *) &client->encryptbuf[0x0C] = client->character.teamID;
		memcpy (&client->encryptbuf[0x18], &client->character.teamName[0], 28 );
		client->encryptbuf[0x34] = 0x84;
		client->encryptbuf[0x35] = 0x6C;
		memcpy (&client->encryptbuf[0x36], &client->character.teamFlag[0], 0x800);
		client->encryptbuf[0x836] = 0xFF;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x838);
		break;
	case 0x10:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x10;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x11:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x11;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	case 0x12:
		memset (&client->encryptbuf[0x00], 0, 0x40);
		client->encryptbuf[0x00] = 0x40;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x12;
		if ( client->character.teamID  )
		{
			*(unsigned *) &client->encryptbuf[0x0C] = client->guildcard;
			*(unsigned *) &client->encryptbuf[0x10] = client->character.teamID;
			client->encryptbuf[0x1C] = (unsigned char) client->character.privilegeLevel;
			memcpy (&client->encryptbuf[0x20], &client->character.teamName[0], 28);
			client->encryptbuf[0x3C] = 0x84;
			client->encryptbuf[0x3D] = 0x6C;
			client->encryptbuf[0x3E] = 0x98;
		}
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x40);
/*
		if ( client->lobbyNum < 0x10 )
			SendToLobby ( client->lobby, 12, &client->encryptbuf[0x00], 0x40, 0 );
		else
			SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x40, 0 );
*/
		break;
	case 0x13:
		{
			LOBBY *l;
			BANANA *lClient;
			unsigned ch, total_clients, EA15Offset, maxc;

			if (!client->lobby)
				break;

			l = (LOBBY*) client->lobby;

			if ( client->lobbyNum < 0x10 ) 
				maxc = 12;
			else
				maxc = 4;
			EA15Offset = 0x08;
			total_clients = 0;
			for (ch=0;ch<maxc;ch++)
			{
				if ((l->slot_use[ch]) && (l->client[ch]))
				{
					lClient = l->client[ch];
					*(unsigned *) &client->encryptbuf[EA15Offset] = lClient->character.guildCard2;
					EA15Offset += 0x04;
					*(unsigned *) &client->encryptbuf[EA15Offset] = lClient->character.teamID;
					EA15Offset += 0x04;
					memset(&client->encryptbuf[EA15Offset], 0, 8);
					EA15Offset += 0x08;
					client->encryptbuf[EA15Offset] = (unsigned char) lClient->character.privilegeLevel;
					EA15Offset += 4;
					memcpy(&client->encryptbuf[EA15Offset], &lClient->character.teamName[0], 28);
					EA15Offset += 28;
					if ( lClient->character.teamID != 0 )
					{
						client->encryptbuf[EA15Offset++] = 0x84;
						client->encryptbuf[EA15Offset++] = 0x6C;
						client->encryptbuf[EA15Offset++] = 0x98;
						client->encryptbuf[EA15Offset++] = 0x00;
					}
					else
					{
						memset (&client->encryptbuf[EA15Offset], 0, 4);
						EA15Offset+= 4;
					}
					*(unsigned *) &client->encryptbuf[EA15Offset] = lClient->character.guildCard;
					EA15Offset += 4;
					client->encryptbuf[EA15Offset++] = lClient->clientID;
					memset(&client->encryptbuf[EA15Offset], 0, 3);
					EA15Offset += 3;
					memcpy(&client->encryptbuf[EA15Offset], &lClient->character.name[0], 24);
					EA15Offset += 24;
					memset(&client->encryptbuf[EA15Offset], 0, 8);
					EA15Offset += 8;
					memcpy(&client->encryptbuf[EA15Offset], &lClient->character.teamFlag[0], 0x800);
					EA15Offset += 0x800;
					total_clients++;
				}
			}
			*(unsigned short*) &client->encryptbuf[0x00] = (unsigned short) EA15Offset;
			client->encryptbuf[0x02] = 0xEA;
			client->encryptbuf[0x03] = 0x13;
			*(unsigned*) &client->encryptbuf[0x04] = total_clients;
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &client->encryptbuf[0], EA15Offset);
		}
		break;
	case 0x18:
		memset (&client->encryptbuf[0x00], 0, 0x4C);
		client->encryptbuf[0x00] = 0x4C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x18;
		client->encryptbuf[0x14] = 0x01;
		client->encryptbuf[0x18] = 0x01;
		client->encryptbuf[0x1C] = (unsigned char) client->character.privilegeLevel;
		*(unsigned *) &client->encryptbuf[0x20] = client->character.guildCard;
		memcpy (&client->encryptbuf[0x24], &client->character.name[0], 24);
		client->encryptbuf[0x48] = 0x02;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x4C);
		break;
	case 0x19:
		memset (&client->encryptbuf[0x00], 0, 0x0C);
		client->encryptbuf[0x00] = 0x0C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x19;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x0C);
		break;
	case 0x1A:
		memset (&client->encryptbuf[0x00], 0, 0x0C);
		client->encryptbuf[0x00] = 0x0C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x1A;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x0C);
		break;
	case 0x1D:
		memset (&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x1D;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 8);
		break;
	default:
		break;
	}
}

unsigned char* MakePacketEA15 (BANANA* client)
{
	sprintf (&PacketData[0x00], "\x64\x08\xEA\x15\x01");
	memset  (&PacketData[0x05], 0, 3);
	*(unsigned *) &PacketData[0x08] = client->guildcard;
	*(unsigned *) &PacketData[0x0C] = client->character.teamID;
	PacketData [0x18] = (unsigned char) client->character.privilegeLevel;
	memcpy  (&PacketData [0x1C], &client->character.teamName[0], 28);
	sprintf (&PacketData[0x38], "\x84\x6C\x98");
	*(unsigned *) &PacketData[0x3C] = client->guildcard;
	PacketData[0x40] = client->clientID;
	memcpy  (&PacketData[0x44], &client->character.name[0], 24);
	memcpy  (&PacketData[0x64], &client->character.teamFlag[0], 0x800);
	return   &PacketData[0];
}

unsigned free_game_item (LOBBY* l)
{
	unsigned ch, ch2, oldest_item;

	ch2 = oldest_item = 0xFFFFFFFF;

	// If the itemid at the current index is 0, just return that...

	if ((l->gameItemCount < MAX_SAVED_ITEMS) && (l->gameItem[l->gameItemCount].item.itemid == 0))
		return l->gameItemCount;

	// Scan the gameItem array for any free item slots...

	for (ch=0;ch<MAX_SAVED_ITEMS;ch++)
	{
		if (l->gameItem[ch].item.itemid == 0)
		{
			ch2 = ch;
			break;
		}
	}

	if (ch2 != 0xFFFFFFFF)
		return ch2;

	// Out of inventory memory!  Time to delete the oldest dropped item in the game...

	for (ch=0;ch<MAX_SAVED_ITEMS;ch++)
	{
		if ((l->gameItem[ch].item.itemid < oldest_item) && (l->gameItem[ch].item.itemid >= 0x810000))
		{
			ch2 = ch;
			oldest_item = l->gameItem[ch].item.itemid;
		}
	}

	if (ch2 != 0xFFFFFFFF)
	{
		l->gameItem[ch2].item.itemid = 0; // Item deleted.
		return ch2;
	}

	for (ch=0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
			SendEE ("Lobby inventory problem!  It's advised you quit this game and recreate it.", l->client[ch]);
	}

	return 0;
}

void UpdateGameItem (BANANA* client)
{
	// Updates the game item list for all of the client's items...  (Used strictly when a client joins a game...)

	LOBBY* l;
	unsigned ch;

	memset (&client->tekked, 0, sizeof (INVENTORY_ITEM)); // Reset tekking data

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++) // By default this should already be sorted at the top, so no need for an in_use check...	
		client->character.inventory[ch].item.itemid = l->playerItemID[client->clientID]++; // Keep synchronized
}

// FFFF

INVENTORY_ITEM sort_data[30];
BANK_ITEM bank_data[200];

void SortClientItems (BANANA* client)
{
	unsigned ch, ch2, ch3, ch4, itemid;

	ch2 = 0x0C;

	memset (&sort_data[0], 0, sizeof (INVENTORY_ITEM) * 30);

	for (ch4=0;ch4<30;ch4++)
	{
		sort_data[ch4].item.data[1] = 0xFF;
		sort_data[ch4].item.itemid = 0xFFFFFFFF;
	}

	ch4 = 0;

	for (ch=0;ch<30;ch++)
	{
		itemid = *(unsigned *) &client->decryptbuf[ch2];
		ch2 += 4;
		if (itemid != 0xFFFFFFFF)
		{
			for (ch3=0;ch3<client->character.inventoryUse;ch3++)
			{
				if ((client->character.inventory[ch3].in_use) && (client->character.inventory[ch3].item.itemid == itemid))
				{
					sort_data[ch4++] = client->character.inventory[ch3];
					break;
				}
			}
		}
	}

	for (ch=0;ch<30;ch++)
		client->character.inventory[ch] = sort_data[ch];

}

void CleanUpBank (BANANA* client)
{
	unsigned ch, ch2 = 0;

	memset (&bank_data[0], 0, sizeof (BANK_ITEM) * 200);

	for (ch2=0;ch2<200;ch2++)
		bank_data[ch2].itemid = 0xFFFFFFFF;

	ch2 = 0;

	for (ch=0;ch<200;ch++)
	{
		if (client->character.bankInventory[ch].itemid != 0xFFFFFFFF)
			bank_data[ch2++] = client->character.bankInventory[ch];
	}

	client->character.bankUse = ch2;

	for (ch=0;ch<200;ch++)
		client->character.bankInventory[ch] = bank_data[ch];

}

void CleanUpInventory (BANANA* client)
{
	unsigned ch, ch2 = 0;

	memset (&sort_data[0], 0, sizeof (INVENTORY_ITEM) * 30);

	ch2 = 0;

	for (ch=0;ch<30;ch++)
	{
		if (client->character.inventory[ch].in_use)
			sort_data[ch2++] = client->character.inventory[ch];
	}

	client->character.inventoryUse = ch2;

	for (ch=0;ch<30;ch++)
		client->character.inventory[ch] = sort_data[ch];
}

void CleanUpGameInventory (LOBBY* l)
{
	unsigned ch, item_count;

	ch = item_count = 0;

	while (ch < l->gameItemCount)
	{
		// Combs the entire game inventory for items in use
		if (l->gameItemList[ch] != 0xFFFFFFFF)
		{
			if (ch > item_count)
				l->gameItemList[item_count] = l->gameItemList[ch];
			item_count++;
		}
		ch++;
	}

	if (item_count < MAX_SAVED_ITEMS)
		memset (&l->gameItemList[item_count], 0xFF, ( ( MAX_SAVED_ITEMS - item_count ) * 4 ) );

	l->gameItemCount = item_count;
}

unsigned AddItemToClient (unsigned itemid, BANANA* client)
{
	unsigned ch, itemNum = 0;
	int found_item = -1;
	unsigned char stackable = 0;
	unsigned count, stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	LOBBY* l;

	// Adds an item to the client's character data, but only if the item exists in the game item data
	// to begin with.

	if (!client->lobby)
		return 0;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<l->gameItemCount;ch++)
	{
		itemNum = l->gameItemList[ch]; // Lookup table for faster searching...
		if ( l->gameItem[itemNum].item.itemid == itemid )
		{
			if (l->gameItem[itemNum].item.data[0] == 0x04)
			{
				// Meseta
				count = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
				client->character.meseta += count;
				if (client->character.meseta > 999999)
					client->character.meseta = 999999;
				item_added = 1;
			}
			else
				if (l->gameItem[itemNum].item.data[0] == 0x03)
					stackable = stackable_table[l->gameItem[itemNum].item.data[1]];
			if ( ( stackable ) && ( !l->gameItem[itemNum].item.data[5] ) )
				l->gameItem[itemNum].item.data[5] = 1;
			found_item = ch;
			break;
		}
	}

	if ( found_item != -1 ) // We won't disconnect if the item isn't found because there's a possibility another
	{						// person may have nabbed it before our client due to lag...
		if ( ( item_added == 0 ) && ( stackable ) )
		{
			memcpy (&compare_item1, &l->gameItem[itemNum].item.data[0], 3);
			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					count = l->gameItem[itemNum].item.data[5];

					stack_count = client->character.inventory[ch].item.data[5];
					if ( !stack_count )
						stack_count = 1;

					if ( ( stack_count + count ) > stackable )
					{
						Send1A ("Trying to stack over the limit...", client);
						client->todc = 1;
					}
					else
					{
						// Add item to the client's current count...
						client->character.inventory[ch].item.data[5] = (unsigned char) ( stack_count + count );
						item_added = 1;
					}
					break;
				}
			}
		}

		if ( ( !client->todc ) && ( item_added == 0 ) ) // Make sure the client isn't trying to pick up more than 30 items...
		{
			if ( client->character.inventoryUse >= 30 )
			{
				Send1A ("Inventory limit reached.", client);
				client->todc = 1;
			}
			else
			{
				// Give item to client...
				client->character.inventory[client->character.inventoryUse].in_use = 0x01;
				client->character.inventory[client->character.inventoryUse].flags = 0x00;
				memcpy (&client->character.inventory[client->character.inventoryUse].item, &l->gameItem[itemNum].item, sizeof (ITEM));
				client->character.inventoryUse++;
				item_added = 1;
			}
		}

		if ( item_added )
		{
			// Delete item from game's inventory
			memset (&l->gameItem[itemNum], 0, sizeof (GAME_ITEM));
			l->gameItemList[found_item] = 0xFFFFFFFF;
			CleanUpGameInventory(l);
		}
	}
	return item_added;
}

void DeleteMesetaFromClient (unsigned count, unsigned drop, BANANA* client)
{
	unsigned stack_count, newItemNum;
	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	stack_count = client->character.meseta;
	if (stack_count < count)
	{
		client->character.meseta = 0;
		count = stack_count;
	}
	else
		client->character.meseta -= count;
	if ( drop )
	{
		memset (&PacketData[0x00], 0, 16);
		PacketData[0x00] = 0x2C;
		PacketData[0x01] = 0x00;
		PacketData[0x02] = 0x60;
		PacketData[0x03] = 0x00;
		PacketData[0x08] = 0x5D;
		PacketData[0x09] = 0x09;
		PacketData[0x0A] = client->clientID;
		*(unsigned *) &PacketData[0x0C] = client->drop_area;
		*(long long *) &PacketData[0x10] = client->drop_coords;
		PacketData[0x18] = 0x04;
		PacketData[0x19] = 0x00;
		PacketData[0x1A] = 0x00;
		PacketData[0x1B] = 0x00;
		memset (&PacketData[0x1C], 0, 0x08);
		*(unsigned *) &PacketData[0x24] = l->playerItemID[client->clientID];
		*(unsigned *) &PacketData[0x28] = count;
		SendToLobby (client->lobby, 4, &PacketData[0], 0x2C, 0);

		// Generate new game item...

		newItemNum = free_game_item (l);
		if (l->gameItemCount < MAX_SAVED_ITEMS)
			l->gameItemList[l->gameItemCount++] = newItemNum;
		memcpy (&l->gameItem[newItemNum].item.data[0], &PacketData[0x18], 12);
		*(unsigned *) &l->gameItem[newItemNum].item.data2[0] = count;
		l->gameItem[newItemNum].item.itemid = l->playerItemID[client->clientID];
		l->playerItemID[client->clientID]++;
	}
}


void SendItemToEnd (unsigned itemid, BANANA* client)
{
	unsigned ch;
	INVENTORY_ITEM i;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			i = client->character.inventory[ch];
			i.flags = 0x00;
			client->character.inventory[ch].in_use = 0;
			break;
		}
	}

	CleanUpInventory (client);
	
	// Add item to client.

	client->character.inventory[client->character.inventoryUse] = i;
	client->character.inventoryUse++;
}


void DeleteItemFromClient (unsigned itemid, unsigned count, unsigned drop, BANANA* client)
{
	unsigned ch, ch2, itemNum;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned delete_item = 0;
	unsigned stack_count;

	// Deletes an item from the client's character data.

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
			{
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];
				if ( ( stackable ) && ( !count ) && ( !drop ) )
					count = 1;
			}

			if ( ( stackable ) && ( count ) )
			{
				stack_count = client->character.inventory[ch].item.data[5];
				if (!stack_count)
					stack_count = 1;

				if ( stack_count < count )
				{
					Send1A ("Trying to delete more items than posssessed!", client);
					client->todc = 1;
				}
				else
				{
					stack_count -= count;
					client->character.inventory[ch].item.data[5] = (unsigned char) stack_count;

					if ( !stack_count )
						delete_item = 1;

					if ( drop )
					{
						memset (&PacketData[0x00], 0, 16);
						PacketData[0x00] = 0x28;
						PacketData[0x01] = 0x00;
						PacketData[0x02] = 0x60;
						PacketData[0x03] = 0x00;
						PacketData[0x08] = 0x5D;
						PacketData[0x09] = 0x08;
						PacketData[0x0A] = client->clientID;
						*(unsigned *) &PacketData[0x0C] = client->drop_area;
						*(long long *) &PacketData[0x10] = client->drop_coords;
						memcpy (&PacketData[0x18], &client->character.inventory[ch].item.data[0], 12);
						PacketData[0x1D] = (unsigned char) count;
						*(unsigned *) &PacketData[0x24] = l->playerItemID[client->clientID];

						SendToLobby (client->lobby, 4, &PacketData[0], 0x28, 0);

						// Generate new game item...

						itemNum = free_game_item (l);
						if (l->gameItemCount < MAX_SAVED_ITEMS)
							l->gameItemList[l->gameItemCount++] = itemNum;
						memcpy (&l->gameItem[itemNum].item.data[0], &PacketData[0x18], 12);
						l->gameItem[itemNum].item.itemid =  l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
					}
				}
			}
			else
			{
				delete_item = 1; // Not splitting a stack, item goes byebye from character's inventory.
				if ( drop ) // Client dropped the item on the floor?
				{
					// Copy to game's inventory
					itemNum = free_game_item (l);
					if (l->gameItemCount < MAX_SAVED_ITEMS)
						l->gameItemList[l->gameItemCount++] = itemNum;
					memcpy (&l->gameItem[itemNum].item, &client->character.inventory[ch].item, sizeof (ITEM));
				}
			}

			if ( delete_item )
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[4] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
					}
				}
				client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}

	if ( found_item == -1 )
	{
		Send1A ("Could not find item to delete.", client);
		client->todc = 1;
	}
	else
		CleanUpInventory (client);

}

unsigned WithdrawFromBank (unsigned itemid, unsigned count, BANANA* client)
{
	unsigned ch;
	int found_item = -1;
	unsigned char stackable = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	unsigned delete_item = 0;
	LOBBY* l;

	// Adds an item to the client's character from it's bank, if the item is really there...

	if (!client->lobby)
		return 0;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.bankUse;ch++)
	{
		if ( client->character.bankInventory[ch].itemid == itemid )
		{
			found_item = ch;
			if ( client->character.bankInventory[ch].data[0] == 0x03 )
			{
				stackable = stackable_table[client->character.bankInventory[ch].data[1]];

				if ( stackable )
				{
					if ( !count )
						count = 1;

					stack_count = ( client->character.bankInventory[ch].bank_count & 0xFF );
					if ( !stack_count )
						stack_count = 1;				

					if ( stack_count < count ) // Naughty!
					{
						Send1A ("Trying to pull a fast one on the bank teller.", client);
						client->todc = 1;
						found_item = -1;
					}
					else
					{
						stack_count -= count;
						client->character.bankInventory[ch].bank_count = 0x10000 + stack_count;
						if ( !stack_count )
							delete_item = 1;
					}
				}
			}
			break;
		}
	}

	if ( found_item != -1 )
	{
		if ( stackable )
		{
			memcpy (&compare_item1, &client->character.bankInventory[found_item].data[0], 3);
			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;
					if ( ( stack_count + count ) > stackable )
					{
						count = stackable - stack_count;
						client->character.inventory[ch].item.data[5] = stackable;
					}
					else
						client->character.inventory[ch].item.data[5] = (unsigned char) (stack_count + count);
					item_added = 1;
					break;
				}
			}
		}

		if ( (!client->todc ) && ( item_added == 0 ) ) // Make sure the client isn't trying to withdraw more than 30 items...
		{
			if ( client->character.inventoryUse >= 30 )
			{
				Send1A ("Inventory limit reached.", client);
				client->todc = 1;
			}
			else
			{
				// Give item to client...
				client->character.inventory[client->character.inventoryUse].in_use = 0x01;
				client->character.inventory[client->character.inventoryUse].flags = 0x00;
				memcpy (&client->character.inventory[client->character.inventoryUse].item, &client->character.bankInventory[found_item].data[0], sizeof (ITEM));
				if ( stackable )
				{
					memset (&client->character.inventory[client->character.inventoryUse].item.data[4], 0, 4);
					client->character.inventory[client->character.inventoryUse].item.data[5] = (unsigned char) count;
				}
				client->character.inventory[client->character.inventoryUse].item.itemid = l->itemID;
				client->character.inventoryUse++;
				item_added = 1;
				//debug ("Item added to client...");
			}
		}

		if ( item_added )
		{
			// Let people know the client has a new toy...
			memset (&client->encryptbuf[0x00], 0, 0x24);
			client->encryptbuf[0x00] = 0x24;
			client->encryptbuf[0x02] = 0x60;
			client->encryptbuf[0x08] = 0xBE;
			client->encryptbuf[0x09] = 0x09;
			client->encryptbuf[0x0A] = client->clientID;
			memcpy (&client->encryptbuf[0x0C], &client->character.bankInventory[found_item].data[0], 12);
			*(unsigned *) &client->encryptbuf[0x18] = l->itemID;
			l->itemID++;
			if (!stackable)
				*(unsigned *) &client->encryptbuf[0x1C] = *(unsigned *) &client->character.bankInventory[found_item].data2[0];
			else
				client->encryptbuf[0x11] = count;
			memset (&client->encryptbuf[0x20], 0, 4);
			SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x24, 0 );
			if ( ( delete_item ) || ( !stackable ) )
				// Delete item from bank inventory
				client->character.bankInventory[found_item].itemid = 0xFFFFFFFF;
		}
		CleanUpBank (client);
	}
	else
	{
		Send1A ("Could not find bank item to withdraw.", client);
		client->todc = 1;
	}

	return item_added;
}

void SortBankItems (BANANA* client)
{
	unsigned ch, ch2;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned char swap_c;
	BANK_ITEM swap_item;
	BANK_ITEM b1;
	BANK_ITEM b2;

	if ( client->character.bankUse > 1 )
	{
		for ( ch=0;ch<client->character.bankUse - 1;ch++ )
		{
			memcpy (&b1, &client->character.bankInventory[ch], sizeof (BANK_ITEM));
			swap_c     = b1.data[0];
			b1.data[0] = b1.data[2];
			b1.data[2] = swap_c;
			memcpy (&compare_item1, &b1.data[0], 3);
			for ( ch2=ch+1;ch2<client->character.bankUse;ch2++ )
			{
				memcpy (&b2, &client->character.bankInventory[ch2], sizeof (BANK_ITEM));
				swap_c     = b2.data[0];
				b2.data[0] = b2.data[2];
				b2.data[2] = swap_c;
				memcpy (&compare_item2, &b2.data[0], 3);
				if (compare_item2 < compare_item1) // compare_item2 should take compare_item1's place
				{
					memcpy (&swap_item, &client->character.bankInventory[ch], sizeof (BANK_ITEM));
					memcpy (&client->character.bankInventory[ch], &client->character.bankInventory[ch2], sizeof (BANK_ITEM));
					memcpy (&client->character.bankInventory[ch2], &swap_item, sizeof (BANK_ITEM));
					memcpy (&compare_item1, &compare_item2, 3);
				}
			}
		}
	}
}

void DepositIntoBank (unsigned itemid, unsigned count, BANANA* client)
{
	unsigned ch, ch2;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned deposit_item = 0, deposit_done = 0;
	unsigned delete_item = 0;
	unsigned stack_count;

	// Moves an item from the client's character to it's bank.

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];

			if ( stackable )
			{
				if (!count)
					count = 1;

				stack_count = client->character.inventory[ch].item.data[5];

				if (!stack_count)
					stack_count = 1;

				if ( stack_count < count )
				{
					Send1A ("Trying to deposit more items than in possession.", client);
					client->todc = 1; // Tried to deposit more than had?
				}
				else
				{
					deposit_item = 1;

					stack_count -= count;
					client->character.inventory[ch].item.data[5] = (unsigned char) stack_count;

					if ( !stack_count )
						delete_item = 1;
				}
			}
			else
			{
				// Not stackable, remove from client completely.
				deposit_item = 1;
				delete_item = 1;
			}

			if ( deposit_item )
			{
				if ( stackable )
				{
					memcpy (&compare_item1, &client->character.inventory[ch].item.data[0], 3);
					for (ch2=0;ch2<client->character.bankUse;ch2++)
					{
						memcpy (&compare_item2, &client->character.bankInventory[ch2].data[0], 3);
						if (compare_item1 == compare_item2)
						{
							stack_count = ( client->character.bankInventory[ch2].bank_count & 0xFF );
							if ( ( stack_count + count ) > stackable )
							{
								count = stackable - stack_count;
								client->character.bankInventory[ch2].bank_count = 0x10000 + stackable;
							}
							else
								client->character.bankInventory[ch2].bank_count += count;
							deposit_done = 1;
							break;
						}
					}
				}

				if ( ( !client->todc ) && ( !deposit_done ) )
				{
					if (client->character.inventory[ch].item.data[0] == 0x01)
					{
						if ((client->character.inventory[ch].item.data[1] == 0x01) &&
							(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
								if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
									(client->character.inventory[ch2].item.data[1] != 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
								{
									client->character.inventory[ch2].flags &= ~(0x08);
									client->character.inventory[ch2].item.data[4] = 0x00;
								}
						}
					}

					memcpy (&client->character.bankInventory[client->character.bankUse].data[0],
						&client->character.inventory[ch].item.data[0],
						sizeof (ITEM));

					if ( stackable )
					{
						memset ( &client->character.bankInventory[client->character.bankUse].data[4], 0, 4 );
						client->character.bankInventory[client->character.bankUse].bank_count = 0x10000 + count;
					}
					else
						client->character.bankInventory[client->character.bankUse].bank_count = 0x10001;

					client->character.bankInventory[client->character.bankUse].itemid = client->character.inventory[ch].item.itemid; // for now
					client->character.bankUse++;
				}

				if ( delete_item )
					client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}

	if ( found_item == -1 )
	{
		Send1A ("Could not find item to deposit.", client);
		client->todc = 1;
	}
	else
		CleanUpInventory (client);
}

void DeleteFromInventory (INVENTORY_ITEM* i, unsigned count, BANANA* client)
{
	unsigned ch, ch2;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned delete_item = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned compare_id;

	// Deletes an item from the client's character data.

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	memcpy (&compare_item1, &i->item.data[0], 3);
	if (i->item.itemid)
		compare_id = i->item.itemid;
	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
		if (!i->item.itemid)
			compare_id = client->character.inventory[ch].item.itemid;
		if ((compare_item1 == compare_item2) && (compare_id == client->character.inventory[ch].item.itemid)) // Found the item?
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];

			if ( stackable )
			{
				if ( !count )
					count = 1;

				stack_count = client->character.inventory[ch].item.data[5];
				if ( !stack_count )
					stack_count = 1;

				if ( stack_count < count )
					count = stack_count;

				stack_count -= count;

				client->character.inventory[ch].item.data[5] = (unsigned char) stack_count;

				if (!stack_count)
					delete_item = 1;
			}
			else
				delete_item = 1;

			memset (&client->encryptbuf[0x00], 0, 0x14);
			client->encryptbuf[0x00] = 0x14;
			client->encryptbuf[0x02] = 0x60;
			client->encryptbuf[0x08] = 0x29;
			client->encryptbuf[0x09] = 0x05;
			client->encryptbuf[0x0A] = client->clientID;
			*(unsigned *) &client->encryptbuf[0x0C] =  client->character.inventory[ch].item.itemid;
			client->encryptbuf[0x10] = (unsigned char) count;

			SendToLobby (l, 4, &client->encryptbuf[0x00], 0x14, 0);

			if ( delete_item )
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[4] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
					}
				}
				client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}
	if ( found_item == -1 )
	{
		Send1A ("Could not find item to delete from inventory.", client);
		client->todc = 1;
	}
	else
		CleanUpInventory (client);

}

unsigned AddToInventory (INVENTORY_ITEM* i, unsigned count, int shop, BANANA* client)
{
	unsigned ch;
	unsigned char stackable = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	unsigned notsend;
	LOBBY* l;

	// Adds an item to the client's inventory... (out of thin air)
	// The new itemid must already be set to i->item.itemid

	if (!client->lobby)
		return 0;

	l = (LOBBY*) client->lobby;

	if (i->item.data[0] == 0x04)
	{
		// Meseta
		count = *(unsigned *) &i->item.data2[0];
		client->character.meseta += count;
		if (client->character.meseta > 999999)
			client->character.meseta = 999999;
		item_added = 1;
	}
	else
	{
		if ( i->item.data[0] == 0x03 )
			stackable = stackable_table [i->item.data[1]];
	}

	if ( ( !client->todc ) && ( !item_added ) )
	{
		if ( stackable )
		{
			if (!count)
				count = 1;
			memcpy (&compare_item1, &i->item.data[0], 3);
			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;
					if ( ( stack_count + count ) > stackable )
					{
						count = stackable - stack_count;
						client->character.inventory[ch].item.data[5] = stackable;
					}
					else
						client->character.inventory[ch].item.data[5] = (unsigned char) ( stack_count + count );
					item_added = 1;
					break;
				}
			}
		}

		if ( item_added == 0 ) // Make sure we don't go over the max inventory
		{
			if ( client->character.inventoryUse >= 30 )
			{
				Send1A ("Inventory limit reached.", client);
				client->todc = 1;
			}
			else
			{
				// Give item to client...
				client->character.inventory[client->character.inventoryUse].in_use = 0x01;
				client->character.inventory[client->character.inventoryUse].flags = 0x00;
				memcpy (&client->character.inventory[client->character.inventoryUse].item, &i->item, sizeof (ITEM));
				if ( stackable )
				{
					memset (&client->character.inventory[client->character.inventoryUse].item.data[4], 0, 4);
					client->character.inventory[client->character.inventoryUse].item.data[5] = (unsigned char) count;
				}
				client->character.inventoryUse++;
				item_added = 1;
			}
		}
	}

	if ((!client->todc) && ( item_added ) )
	{
		// Let people know the client has a new toy...
		memset (&client->encryptbuf[0x00], 0, 0x24);
		client->encryptbuf[0x00] = 0x24;
		client->encryptbuf[0x02] = 0x60;
		client->encryptbuf[0x08] = 0xBE;
		client->encryptbuf[0x09] = 0x09;
		client->encryptbuf[0x0A] = client->clientID;
		memcpy (&client->encryptbuf[0x0C], &i->item.data[0], 12);
		*(unsigned *) &client->encryptbuf[0x18] = i->item.itemid;
		if ((!stackable) || (i->item.data[0] == 0x04))
			*(unsigned *) &client->encryptbuf[0x1C] = *(unsigned *) &i->item.data2[0];
		else
			client->encryptbuf[0x11] = count;
		memset (&client->encryptbuf[0x20], 0, 4);
		if ( shop )
			notsend = client->guildcard;
		else
			notsend = 0;
		SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x24, notsend );
	}
	return item_added;
}


void ShipSend04 (unsigned char command, BANANA* client, ORANGE* ship)
{
	//unsigned ch;

	ship->encryptbuf[0x00] = 0x04;
	switch (command)
	{
	case 0x00:
		// Request character data from server
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned *) &ship->encryptbuf[0x02] = client->guildcard;
		*(unsigned short *) &ship->encryptbuf[0x06] = (unsigned short) client->slotnum;
		*(int *) &ship->encryptbuf[0x08] = client->plySockfd;
		*(unsigned *) &ship->encryptbuf[0x0C] = serverID;
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x10 );
		break;
	case 0x02:
		// Send character data to server when not using a temporary character.
		if ((!client->mode) && (client->gotchardata == 1))
		{
			ship->encryptbuf[0x01] = 0x02;
			*(unsigned *) &ship->encryptbuf[0x02] = client->guildcard;
			*(unsigned short*) &ship->encryptbuf[0x06] = (unsigned short) client->slotnum;
			memcpy (&ship->encryptbuf[0x08], &client->character.packetSize, sizeof (CHARDATA));
			// Include character bank in packet
			memcpy (&ship->encryptbuf[0x08+0x700], &client->char_bank, sizeof (BANK));
			// Include common bank in packet
			memcpy (&ship->encryptbuf[0x08+sizeof(CHARDATA)], &client->common_bank, sizeof (BANK));
			compressShipPacket ( ship, &ship->encryptbuf[0x00], sizeof(BANK) + sizeof(CHARDATA) + 8 );
		}
		break;
	}
}

void ShipSend0E (ORANGE* ship)
{
	if (logon_ready)
	{
		ship->encryptbuf[0x00] = 0x0E;
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned *) &ship->encryptbuf[0x02] = serverID;
		*(unsigned *) &ship->encryptbuf[0x06] = serverNumConnections;
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
	}
}

void ShipSend0D (unsigned char command, BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x0D;
	switch (command)
	{
	case 0x00:
		// Requesting ship list.
		ship->encryptbuf[0x01] = 0x00;
		*(int *) &ship->encryptbuf[0x02]= client->plySockfd;
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 6 );
		break;
	default:
		break;
	}
}

void ShipSend0B (BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x0B;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned *) &ship->encryptbuf[0x02] = *(unsigned *) &client->decryptbuf[0x0C];
	*(unsigned *) &ship->encryptbuf[0x06] = *(unsigned *) &client->decryptbuf[0x18];
	*(long long *) &ship->encryptbuf[0x0A] = *(long long*) &client->decryptbuf[0x8C];
	*(long long *) &ship->encryptbuf[0x12] = *(long long*) &client->decryptbuf[0x94];
	*(long long *) &ship->encryptbuf[0x1A] = *(long long*) &client->decryptbuf[0x9C];
	*(long long *) &ship->encryptbuf[0x22] = *(long long*) &client->decryptbuf[0xA4];
	*(long long *) &ship->encryptbuf[0x2A] = *(long long*) &client->decryptbuf[0xAC];
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x32 );
}

void FixItem (ITEM* i )
{
	unsigned ch3;

	if (i->data[0] == 2) // Mag
	{
		MAG* m;
		short mDefense, mPower, mDex, mMind;
		int total_levels;

		m = (MAG*) &i->data[0];

		if ( m->synchro > 120 )
			m->synchro = 120;

		if ( m->synchro < 0 )
			m->synchro = 0;

		if ( m->IQ > 200 )
			m->IQ = 200;

		if ( ( m->defense < 0 ) || ( m->power < 0 ) || ( m->dex < 0 ) || ( m->mind < 0 ) )
			total_levels = 201; // Auto fail if any stat is under 0...
		else
		{
			mDefense = m->defense / 100;
			mPower = m->power / 100;
			mDex = m->dex / 100;
			mMind = m->mind / 100;
			total_levels = mDefense + mPower + mDex + mMind;
		}

		if ( ( total_levels > 200 ) || ( m->level > 200 ) )
		{
			// Mag fails IRL, initialize it
			m->defense = 500;
			m->power = 0;
			m->dex = 0;
			m->mind = 0;
			m->level = 5;
			m->blasts = 0;
			m->IQ = 0;
			m->synchro = 20;
			m->mtype = 0;
			m->PBflags = 0;
		}
	}

	if (i->data[0] == 1) // Normalize Armor & Barriers
	{
		switch (i->data[1])
		{
		case 0x01:
			if (i->data[6] > armor_dfpvar_table[ i->data[2] ])
				i->data[6] = armor_dfpvar_table[ i->data[2] ];
			if (i->data[8] > armor_evpvar_table[ i->data[2] ])
				i->data[8] = armor_evpvar_table[ i->data[2] ];
			break;
		case 0x02:
			if (i->data[6] > barrier_dfpvar_table[ i->data[2] ])
				i->data[6] = barrier_dfpvar_table[ i->data[2] ];
			if (i->data[8] > barrier_evpvar_table[ i->data[2] ])
				i->data[8] = barrier_evpvar_table[ i->data[2] ];
			break;
		}
	}

	if (i->data[0] == 0) // Weapon
	{
		signed char percent_table[6];
		signed char percent;
		unsigned max_percents, num_percents;
		int srank;

		if ( ( i->data[1] == 0x33 ) ||  // SJS & Lame max 2 percents
			 ( i->data[1] == 0xAB ) )
			max_percents = 2;
		else
			max_percents = 3;

		srank = 0;
		memset (&percent_table[0], 0, 6);
		num_percents = 0;

		for (ch3=6;ch3<=4+(max_percents*2);ch3+=2)
		{
			if ( i->data[ch3] & 128 )
			{
				srank = 1; // S-Rank
				break; 
			}

			if ( ( i->data[ch3] ) &&
				( i->data[ch3] < 0x06 ) )
			{
				// Percents over 100 or under -100 get set to 0
				percent = (char) i->data[ch3+1];
				if ( ( percent > 100 ) || ( percent < -100 ) )
					percent = 0;
				// Save percent
				percent_table[i->data[ch3]] = 
					percent;
			}
		}

		if (!srank)
		{
			for (ch3=6;ch3<=4+(max_percents*2);ch3+=2)
			{
				// Reset all %s
				i->data[ch3]   = 0;
				i->data[ch3+1] = 0;
			}

			for (ch3=1;ch3<=5;ch3++)
			{
				// Rebuild %s
				if ( percent_table[ch3] )
				{
					i->data[6 + ( num_percents * 2 )] = ch3;
					i->data[7 + ( num_percents * 2 )] = (unsigned char) percent_table[ch3];
					num_percents ++;
					if ( num_percents == max_percents )
						break;
				}
			}
		}
	}
}

const char lobbyString[] = { "L\0o\0b\0b\0y\0 \0" };

void LogonProcessPacket (ORANGE* ship)
{
	unsigned gcn, ch, ch2, connectNum;
	unsigned char episode, part;
	unsigned mob_rate;
	long long mob_calc;

	switch (ship->decryptbuf[0x04])
	{
	case 0x00:
		// Server has sent it's welcome packet.  Start encryption and send ship info...
		memcpy (&ship->user_key[0], &RC4publicKey[0], 32);
		ch2 = 0;
		for (ch=0x1C;ch<0x5C;ch+=2)
		{
			ship->key_change [ch2+(ship->decryptbuf[ch] % 4)] = ship->decryptbuf[ch+1];
			ch2 += 4;
		}
		prepare_key(&ship->user_key[0], 32, &ship->cs_key);
		prepare_key(&ship->user_key[0], 32, &ship->sc_key);
		ship->crypt_on = 1;
		memcpy (&ship->encryptbuf[0x00], &ship->decryptbuf[0x04], 0x28);
		memcpy (&ship->encryptbuf[0x00], &ShipPacket00[0x00], 0x10); // Yep! :)
		ship->encryptbuf[0x00] = 1;
		memcpy (&ship->encryptbuf[0x28], &Ship_Name[0], 12 );
		*(unsigned *) &ship->encryptbuf[0x34] = serverNumConnections;
		*(unsigned *) &ship->encryptbuf[0x38] = *(unsigned *) &serverIP[0];
		*(unsigned short*) &ship->encryptbuf[0x3C] = (unsigned short) serverPort;
		*(unsigned *) &ship->encryptbuf[0x3E] = shop_checksum;
		*(unsigned *) &ship->encryptbuf[0x42] = ship_index;
		memcpy (&ship->encryptbuf[0x46], &ship_key[0], 32);
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x66 );
		break;
	case 0x02:
		// Server's result of our authentication packet.
		if (ship->decryptbuf[0x05] != 0x01)
		{
			switch (ship->decryptbuf[0x05])
			{
			case 0x00:
				printf ("This ship's version is incompatible with the login server.\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			case 0x02:
				printf ("This ship's IP address is already registered with the logon server.\n");
				printf ("The IP address cannot be registered twice.  Retry in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				reveal_window;
				break;
			case 0x03:
				printf ("This ship did not pass the connection test the login server ran on it.\n");
				printf ("Please be sure the IP address specified in ship.ini is correct, your\n");
				printf ("firewall has ship_serv.exe on allow.  If behind a router, please be\n");
				printf ("sure your ports are forwarded.  Retry in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				reveal_window;
				break;
			case 0x04:
				printf ("Please do not modify any data not instructed to when connecting to this\n");
				printf ("login server...\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			case 0x05:
				printf ("Your ship_key.bin file seems to be invalid.\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			case 0x06:
				printf ("Your ship key appears to already be in use!\n");
				printf ("Press [ENTER] to quit...");
				reveal_window;
				gets (&dp[0]);
				exit (1);
				break;
			}
			initialize_logon();
		}
		else
		{
			serverID = *(unsigned *) &ship->decryptbuf[0x06];
			if (serverIP[0] == 0x00)
			{
				*(unsigned *) &serverIP[0] = *(unsigned *) &ship->decryptbuf[0x0A];
				printf ("Updated IP address to %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
			}
			serverID++;
			if (serverID != 0xFFFFFFFF)
			{
				printf ("Ship has successfully registered with the login server!!! Ship ID: %u\n", serverID );
				printf ("Constructing Block List packet...\n\n");
				ConstructBlockPacket();
				printf ("Load quest allowance...\n");
				quest_numallows = *(unsigned *) &ship->decryptbuf[0x0E];
				if ( quest_allow )
					free ( quest_allow );
				quest_allow = malloc ( quest_numallows * 4  );
				memcpy ( quest_allow, &ship->decryptbuf[0x12], quest_numallows * 4 );
				printf ("Quest allowance item count: %u\n\n", quest_numallows );
				normalName = *(unsigned *) &ship->decryptbuf[0x12 + ( quest_numallows * 4 )];
				localName = *(unsigned *) &ship->decryptbuf[0x16 + ( quest_numallows * 4 )];
				globalName = *(unsigned *) &ship->decryptbuf[0x1A + ( quest_numallows * 4 )];
				memcpy (&ship->user_key[0], &ship_key[0], 128 ); // 1024-bit key

				// Change keys

				for (ch2=0;ch2<128;ch2++)
					if (ship->key_change[ch2] != -1)
						ship->user_key[ch2] = (unsigned char) ship->key_change[ch2]; // update the key

				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->cs_key);
				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->sc_key);
				memset ( &ship->encryptbuf[0x00], 0, 8 );
				ship->encryptbuf[0x00] = 0x0F;
				ship->encryptbuf[0x01] = 0x00;
				printf ( "Requesting drop charts from server...\n");
				compressShipPacket ( ship, &ship->encryptbuf[0x00], 4 );
			}
			else
			{
				printf ("The ship has failed authentication to the logon server.  Retry in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				initialize_logon();
			}
		}
		break;
	case 0x03:
		// Reserved
		break;
	case 0x04:
		switch (ship->decryptbuf[0x05])
		{
		case 0x01:
			{
				// Receive and store full player data here.
				//
				BANANA* client;
				unsigned guildcard,ch,ch2,eq_weapon,eq_armor,eq_shield,eq_mag;
				int sockfd;
				unsigned short baseATP, baseMST, baseEVP, baseHP, baseDFP, baseATA;
				unsigned char* cd;

				guildcard = *(unsigned *) &ship->decryptbuf[0x06];
				sockfd = *(int *) &ship->decryptbuf[0x0C];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->plySockfd == sockfd) && (connections[connectNum]->guildcard == guildcard))
					{
						client = connections[connectNum];
						client->gotchardata = 1;
						memcpy (&client->character.packetSize, &ship->decryptbuf[0x10], sizeof (CHARDATA));

						/* Set up copies of the banks */

						memcpy (&client->char_bank, &client->character.bankUse, sizeof (BANK));
						memcpy (&client->common_bank, &ship->decryptbuf[0x10+sizeof(CHARDATA)], sizeof (BANK));

						cipher_ptr = &client->server_cipher;
						if (client->isgm == 1)
							*(unsigned *) &client->character.nameColorBlue = globalName;
						else
							if (isLocalGM(client->guildcard))
								*(unsigned *) &client->character.nameColorBlue = localName;
							else
								*(unsigned *) &client->character.nameColorBlue = normalName;

						if (client->character.inventoryUse > 30)
							client->character.inventoryUse = 30;

						client->equip_flags = 0;
						switch (client->character._class)
						{
						case CLASS_HUMAR:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_HUNEWEARL:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= NEWMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_HUCAST:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_HUCASEAL:
							client->equip_flags |= HUNTER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_RAMAR:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_RACAST:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_RACASEAL:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= DROID_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_RAMARL:
							client->equip_flags |= RANGER_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_FONEWM:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= NEWMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						case CLASS_FONEWEARL:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= NEWMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_FOMARL:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= FEMALE_FLAG;
							break;
						case CLASS_FOMAR:
							client->equip_flags |= FORCE_FLAG;
							client->equip_flags |= HUMAN_FLAG;
							client->equip_flags |= MALE_FLAG;
							break;
						}

						// Let's fix hacked mags and weapons

						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						{
							if (client->character.inventory[ch2].in_use)
								FixItem ( &client->character.inventory[ch2].item );
						}

						// Fix up equipped weapon, armor, shield, and mag equipment information

						eq_weapon = 0;
						eq_armor = 0;
						eq_shield = 0;
						eq_mag = 0;

						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						{
							if (client->character.inventory[ch2].flags & 0x08)
							{
								switch (client->character.inventory[ch2].item.data[0])
								{
								case 0x00:
									eq_weapon++;
									break;
								case 0x01:
									switch (client->character.inventory[ch2].item.data[1])
									{
									case 0x01:
										eq_armor++;
										break;
									case 0x02:
										eq_shield++;
										break;
									}
									break;
								case 0x02:
									eq_mag++;
									break;
								}
							}
						}

						if (eq_weapon > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all weapons when there is more than one equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
									(client->character.inventory[ch2].flags & 0x08))
									client->character.inventory[ch2].flags &= ~(0x08);
							}

						}

						if (eq_armor > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all armor and slot items when there is more than one armor equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
									(client->character.inventory[ch2].item.data[1] != 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
								{
									client->character.inventory[ch2].item.data[3] = 0x00;
									client->character.inventory[ch2].flags &= ~(0x08);
								}
							}
						}

						if (eq_shield > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all shields when there is more than one equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
									(client->character.inventory[ch2].item.data[1] == 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
								{
									client->character.inventory[ch2].item.data[3] = 0x00;
									client->character.inventory[ch2].flags &= ~(0x08);
								}
							}
						}

						if (eq_mag > 1)
						{
							for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							{
								// Unequip all mags when there is more than one equipped.
								if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
									client->character.inventory[ch2].flags &= ~(0x08);
							}
						}

						for (ch2=0;ch2<client->character.bankUse;ch2++)
							FixItem ( (ITEM*) &client->character.bankInventory[ch2] );

						baseATP = *(unsigned short*) &startingData[(client->character._class*14)];
						baseMST = *(unsigned short*) &startingData[(client->character._class*14)+2];
						baseEVP = *(unsigned short*) &startingData[(client->character._class*14)+4];
						baseHP  = *(unsigned short*) &startingData[(client->character._class*14)+6];
						baseDFP = *(unsigned short*) &startingData[(client->character._class*14)+8];
						baseATA = *(unsigned short*) &startingData[(client->character._class*14)+10];

						for (ch2=0;ch2<client->character.level;ch2++)
						{
							baseATP += playerLevelData[client->character._class][ch2].ATP;
							baseMST += playerLevelData[client->character._class][ch2].MST;
							baseEVP += playerLevelData[client->character._class][ch2].EVP;
							baseHP  += playerLevelData[client->character._class][ch2].HP;
							baseDFP += playerLevelData[client->character._class][ch2].DFP;
							baseATA += playerLevelData[client->character._class][ch2].ATA;
						}

						client->matuse[0] = ( client->character.ATP - baseATP ) / 2;
						client->matuse[1] = ( client->character.MST - baseMST ) / 2;
						client->matuse[2] = ( client->character.EVP - baseEVP ) / 2;
						client->matuse[3] = ( client->character.DFP - baseDFP ) / 2;
						client->matuse[4] = ( client->character.LCK - 10 ) / 2;

						//client->character.lang = 0x00;

						cd = (unsigned char*) &client->character.packetSize;

						cd[(8*28)+0x0F]  = client->matuse[0];
						cd[(9*28)+0x0F]  = client->matuse[1];
						cd[(10*28)+0x0F] = client->matuse[2];
						cd[(11*28)+0x0F] = client->matuse[3];
						cd[(12*28)+0x0F] = client->matuse[4];

						encryptcopy (client, (unsigned char*) &client->character.packetSize, sizeof (CHARDATA) );
						client->preferred_lobby = 0xFF;

						cd[(8*28)+0x0F]  = 0x00; // Clear this stuff out to not mess up our item procedures.
						cd[(9*28)+0x0F]  = 0x00;
						cd[(10*28)+0x0F] = 0x00;
						cd[(11*28)+0x0F] = 0x00;
						cd[(12*28)+0x0F] = 0x00;

						for (ch2=0;ch2<MAX_SAVED_LOBBIES;ch2++)
						{
							if (savedlobbies[ch2].guildcard == client->guildcard)
							{
								client->preferred_lobby = savedlobbies[ch2].lobby - 1;
								savedlobbies[ch2].guildcard = 0;
								break;
							}
						}

						Send95 (client);

						if ( (client->isgm) || (isLocalGM(client->guildcard)) )
							WriteGM ("GM %u (%s) has connected", client->guildcard, Unicode_to_ASCII ((unsigned short*) &client->character.name[4]));
						else
							WriteLog ("User %u (%s) has connected", client->guildcard, Unicode_to_ASCII ((unsigned short*) &client->character.name[4]));
						break;
					}
				}
			}
			break;
		case 0x03:
			{
				unsigned guildcard;
				BANANA* client;
				
				guildcard = *(unsigned *) &ship->decryptbuf[0x06];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard == guildcard) && (connections[connectNum]->released == 1))
					{
						// Let the released client roam free...!
						client = connections[connectNum];
						Send19 (client->releaseIP[0], client->releaseIP[1], client->releaseIP[2], client->releaseIP[3], 
							client->releasePort, client);
						break;
					}
				}
			}
		}
		break;
	case 0x05:
		// Reserved
		break;
	case 0x06:
		// Reserved
		break;
	case 0x07:
		// Card database full.
		gcn = *(unsigned *) &ship->decryptbuf[0x06];

		for (ch=0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			if (connections[connectNum]->guildcard == gcn)
			{
				Send1A ("Your guild card database on the server is full.\n\nYou were unable to accept the guild card.\n\nPlease delete some cards.  (40 max)", connections[connectNum]);
				break;
			}
		}
		break;
	case 0x08:
		switch (ship->decryptbuf[0x05])
		{
		case 0x00:
			{
				gcn = *(unsigned *) &ship->decryptbuf[0x06];
				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{
						Send1A ("This account has just logged on.\n\nYou are now being disconnected.", connections[connectNum]);
						connections[connectNum]->todc = 1;
						break;
					}
				}
			}
			break;
		case 0x01:
			{
				// Someone's doing a guild card search...   Check to see if that guild card is on our ship...

				unsigned client_gcn, ch2;
				unsigned char *n;
				unsigned char *c;
				unsigned short blockPort;

				gcn = *(unsigned *) &ship->decryptbuf[0x06];
				client_gcn = *(unsigned *) &ship->decryptbuf[0x0A];

				// requesting ship ID @ 0x0E

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard == gcn) && (connections[connectNum]->lobbyNum))
					{
						if (connections[connectNum]->lobbyNum < 0x10)
							for (ch2=0;ch2<MAX_SAVED_LOBBIES;ch2++)
							{
								if (savedlobbies[ch2].guildcard == 0)
								{
									savedlobbies[ch2].guildcard = client_gcn;
									savedlobbies[ch2].lobby = connections[connectNum]->lobbyNum;
									break;
								}
							}
						ship->encryptbuf[0x00] = 0x08;
						ship->encryptbuf[0x01] = 0x02;
						*(unsigned *) &ship->encryptbuf[0x02] = serverID;
						*(unsigned *) &ship->encryptbuf[0x06] = *(unsigned *) &ship->decryptbuf[0x0E];
						// 0x10 = 41 result packet
						memset (&ship->encryptbuf[0x0A], 0, 0x136);
						ship->encryptbuf[0x10] = 0x30;
						ship->encryptbuf[0x11] = 0x01;
						ship->encryptbuf[0x12] = 0x41;
						ship->encryptbuf[0x1A] = 0x01;
						*(unsigned *) &ship->encryptbuf[0x1C] = client_gcn;
						*(unsigned *) &ship->encryptbuf[0x20] = gcn;
						ship->encryptbuf[0x24] = 0x10;
						ship->encryptbuf[0x26] = 0x19;
						*(unsigned *) &ship->encryptbuf[0x2C] = *(unsigned *) &serverIP[0];
						blockPort = serverPort + connections[connectNum]->block;
						*(unsigned short *) &ship->encryptbuf[0x30] = (unsigned short) blockPort;					
						memcpy (&ship->encryptbuf[0x34], &lobbyString[0], 12 );						
						if ( connections[connectNum]->lobbyNum < 0x10 )
						{
							if ( connections[connectNum]->lobbyNum < 10 )
							{
								ship->encryptbuf[0x40] = 0x30;
								ship->encryptbuf[0x42] = 0x30 + connections[connectNum]->lobbyNum;
							}
							else
							{
								ship->encryptbuf[0x40] = 0x31;
								ship->encryptbuf[0x42] = 0x26 + connections[connectNum]->lobbyNum;
							}
						}
						else
						{
								ship->encryptbuf[0x40] = 0x30;
								ship->encryptbuf[0x42] = 0x31;
						}
						ship->encryptbuf[0x44] = 0x2C;
						memcpy ( &ship->encryptbuf[0x46], &blockString[0], 10 );
						if ( connections[connectNum]->block < 10 )
						{
							ship->encryptbuf[0x50] = 0x30;
							ship->encryptbuf[0x52] = 0x30 + connections[connectNum]->block;
						}
						else
						{
							ship->encryptbuf[0x50] = 0x31;
							ship->encryptbuf[0x52] = 0x26 + connections[connectNum]->block;
						}

						ship->encryptbuf[0x54] = 0x2C;
						if (serverID < 10)
						{
							ship->encryptbuf[0x56] = 0x30;
							ship->encryptbuf[0x58] = 0x30 + serverID;
						}
						else
						{
							ship->encryptbuf[0x56] = 0x30 + ( serverID / 10 );
							ship->encryptbuf[0x58] = 0x30 + ( serverID % 10 );
						}
						ship->encryptbuf[0x5A] = 0x3A;
						n = (unsigned char*) &ship->encryptbuf[0x5C];
						c = (unsigned char*) &Ship_Name[0];
						while (*c != 0x00)
						{
							*(n++) = *(c++);
							n++;
						}
						if ( connections[connectNum]->lobbyNum < 0x10 )
						ship->encryptbuf[0xBC] = (unsigned char) connections[connectNum]->lobbyNum; else
						ship->encryptbuf[0xBC] = 0x01;
						ship->encryptbuf[0xBE] = 0x1A;
						memcpy (&ship->encryptbuf[0x100], &connections[connectNum]->character.name[0], 24);
						compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x140 );
						break;
					}
				}
			}
			break;
		case 0x02:
			// Send guild result to user
			{
				gcn = *(unsigned *) &ship->decryptbuf[0x20];

				// requesting ship ID @ 0x0E

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{
						cipher_ptr = &connections[connectNum]->server_cipher;
						encryptcopy (connections[connectNum], &ship->decryptbuf[0x14], 0x130);
						break;
					}
				}
			}
			break;
		case 0x03:
			// Send mail to user
			{
				gcn = *(unsigned *) &ship->decryptbuf[0x36];

				// requesting ship ID @ 0x0E

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{
						cipher_ptr = &connections[connectNum]->server_cipher;
						encryptcopy (connections[connectNum], &ship->decryptbuf[0x06], 0x45C);
						break;
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	case 0x09:
		// Reserved for team functions.
		switch (ship->decryptbuf[0x05])
		{
			BANANA* client;
			unsigned char CreateResult;

		case 0x00:
			CreateResult = ship->decryptbuf[0x06];
			gcn = *(unsigned *) &ship->decryptbuf[0x07];
			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					client = connections[connectNum];
					switch (CreateResult)
					{
					case 0x00:
						// All good!!!
						client->character.teamID = *(unsigned *) &ship->decryptbuf[0x823];
						memcpy (&client->character.teamFlag[0], &ship->decryptbuf[0x0B], 0x800);
						client->character.privilegeLevel = 0x40;
						client->character.unknown15 = 0x00986C84; // ??
						client->character.teamName[0] = 0x09;
						client->character.teamName[2] = 0x45;
						client->character.privilegeLevel = 0x40;
						memcpy (&client->character.teamName[4], &ship->decryptbuf[0x80B], 24);
						SendEA (0x02, client);
						SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
						SendEA (0x12, client);
						SendEA (0x1D, client);
						break;
					case 0x01:
						Send1A ("The server failed to create the team due to a MySQL error.\n\nPlease contact the server administrator.", client);
						break;
					case 0x02:
						Send01 ("Cannot create team\nbecause team\n already exists!!!", client);
						break;
					case 0x03:
						Send01 ("Cannot create team\nbecause you are\nalready in a team!", client);
						break;
					}
					break;
				}
			}
			break;
		case 0x01:
			// Flag updated
			{
				unsigned teamid;
				BANANA* tClient;

				teamid = *(unsigned *) &ship->decryptbuf[0x07];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
					{
						tClient = connections[connectNum];
						memcpy ( &tClient->character.teamFlag[0], &ship->decryptbuf[0x0B], 0x800);
						SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
					}
				}
			}
			break;
		case 0x02:
			// Team dissolved
			{
				unsigned teamid;
				BANANA* tClient;

				teamid = *(unsigned *) &ship->decryptbuf[0x07];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
					{
						tClient = connections[connectNum];
						memset ( &tClient->character.guildCard2, 0, 2108 );
						SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
						SendEA ( 0x12, tClient );
					}
				}
			}
			break;
		case 0x04:
			// Team chat
			{
				unsigned teamid, size;
				BANANA* tClient;

				size = *(unsigned *) &ship->decryptbuf[0x00];
				size -= 10;

				teamid = *(unsigned *) &ship->decryptbuf[0x06];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
					{
						tClient = connections[connectNum];
						cipher_ptr = &tClient->server_cipher;
						encryptcopy ( tClient, &ship->decryptbuf[0x0A], size );
					}
				}
			}
			break;
		case 0x05:
			// Request Team List
			{
				unsigned gcn;
				unsigned short size;
				BANANA* tClient;

				gcn = *(unsigned *) &ship->decryptbuf[0x0A];
				size = *(unsigned short*) &ship->decryptbuf[0x0E];

				for (ch=0;ch<serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{					
						tClient = connections[connectNum];
						cipher_ptr = &tClient->server_cipher;
						encryptcopy (tClient, &ship->decryptbuf[0x0E], size);
						break;
					}
				}
			}
			break;
		}
		break;
	case 0x0A:
		// Reserved
		break;
	case 0x0B:
		// Player authentication result from the logon server.
		gcn = *(unsigned *) &ship->decryptbuf[0x06];
		if (ship->decryptbuf[0x05] == 0)
		{
			BANANA* client;

			// Finish up the logon process here.

			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->temp_guildcard == gcn)
				{
					client = connections[connectNum];
					client->slotnum = ship->decryptbuf[0x0A];
					client->isgm = ship->decryptbuf[0x0B];
					memcpy (&client->encryptbuf[0], &PacketE6[0], sizeof (PacketE6));
					*(unsigned *) &client->encryptbuf[0x10] = gcn;
					client->guildcard = gcn;
					*(unsigned *) &client->encryptbuf[0x14] = *(unsigned*) &ship->decryptbuf[0x0C];
					*(long long *) &client->encryptbuf[0x38] = *(long long*) &ship->decryptbuf[0x10];
					if (client->decryptbuf[0x16] < 0x05)
					{
						Send1A ("Client/Server synchronization error.", client);
						client->todc = 1;
					}
					else
					{
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0], sizeof (PacketE6));
						client->lastTick = (unsigned) servertime;
						if (client->block == 0)
						{
							if (logon->sockfd >= 0)
								Send07(client);
							else
							{
								Send1A("This ship has unfortunately lost it's connection with the logon server...\nData cannot be saved.\n\nPlease reconnect later.", client);
								client->todc = 1;
							}
						}
						else
						{
							blocks[client->block - 1]->count++;
							// Request E7 information from server...
							Send83(client); // Lobby data
							ShipSend04 (0x00, client, logon);
						}
					}
					break;
				}
			}
		}
		else
		{
			// Deny connection here.
			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->temp_guildcard == gcn)
				{
					Send1A ("Security violation.", connections[connectNum]);
					connections[connectNum]->todc = 1;
					break;
				}
			}
		}
		break;
	case 0x0D:
		// 00 = Request ship list
		// 01 = Ship list data (include IP addresses)
		switch (ship->decryptbuf[0x05])
		{
			case 0x01:
				{
					unsigned char ch;
					int sockfd;
					unsigned short pOffset;

					// Retrieved ship list data.  Send to client...

					sockfd = *(int *) &ship->decryptbuf[0x06];
					pOffset = *(unsigned short *) &ship->decryptbuf[0x0A];
					memcpy (&PacketA0Data[0x00], &ship->decryptbuf[0x0A], pOffset);
					pOffset += 0x0A;

					totalShips = 0;

					for (ch=0;ch<PacketA0Data[0x04];ch++)
					{
						shipdata[ch].shipID = *(unsigned *) &ship->decryptbuf[pOffset];
						pOffset +=4;
						*(unsigned *) &shipdata[ch].ipaddr[0] = *(unsigned *) &ship->decryptbuf[pOffset];
						pOffset +=4;
						shipdata[ch].port = *(unsigned short *) &ship->decryptbuf[pOffset];
						pOffset +=2;
						totalShips++;
					}

					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->plySockfd == sockfd)
						{
							SendA0 (connections[connectNum]);
							break;
						}
					}
				}
				break;
			default:
				break;
		}
		break;
	case 0x0F:
		// Receiving drop chart
		episode = ship->decryptbuf[0x05];
		part = ship->decryptbuf[0x06];
		if ( ship->decryptbuf[0x06] == 0 )
			printf ("Received drop chart from login server...\n");
		switch ( episode )
		{
		case 0x01:
			if ( part == 0 )
				printf ("Episode I ..." );
			else
				printf (" OK!\n");
			memcpy ( &rt_tables_ep1[(sizeof(rt_tables_ep1) >> 3) * part], &ship->decryptbuf[0x07], sizeof (rt_tables_ep1) >> 1 );
			break;
		case 0x02:
			if ( part == 0 )
				printf ("Episode II ..." );
			else
				printf (" OK!\n");
			memcpy ( &rt_tables_ep2[(sizeof(rt_tables_ep2) >> 3) * part], &ship->decryptbuf[0x07], sizeof (rt_tables_ep2) >> 1 );
			break;
		case 0x03:
			if ( part == 0 )
				printf ("Episode IV ..." );
			else
				printf (" OK!\n");
			memcpy ( &rt_tables_ep4[(sizeof(rt_tables_ep4) >> 3) * part], &ship->decryptbuf[0x07], sizeof (rt_tables_ep4) >> 1 );
			break;
		}
		*(unsigned *) &ship->encryptbuf[0x00] = *(unsigned *) &ship->decryptbuf[0x04];
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x04 );
		break;
	case 0x10:
		// Monster appearance rates
		printf ("\nReceived rare monster appearance rates from server...\n");
		for (ch=0;ch<8;ch++)
		{
			mob_rate = *(unsigned *) &ship->decryptbuf[0x06 + (ch * 4)];
			mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
/*
			times_won = 0;
			for (ch2=0;ch2<1000000;ch2++)
			{
				if (mt_lrand() < mob_calc)
					times_won++;
			}
*/
			switch (ch)
			{
			case 0x00:
				printf ("Hildebear appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				hildebear_rate = (unsigned) mob_calc;
				break;
			case 0x01:
				printf ("Rappy appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				rappy_rate = (unsigned) mob_calc;
				break;
			case 0x02:
				printf ("Lily appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				lily_rate = (unsigned) mob_calc;
				break;
			case 0x03:
				printf ("Pouilly Slime appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				slime_rate = (unsigned) mob_calc;
				break;
			case 0x04:
				printf ("Merissa AA appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				merissa_rate = (unsigned) mob_calc;
				break;
			case 0x05:
				printf ("Pazuzu appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				pazuzu_rate = (unsigned) mob_calc;
				break;
			case 0x06:
				printf ("Dorphon Eclair appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				dorphon_rate = (unsigned) mob_calc;
				break;
			case 0x07:
				printf ("Kondrieu appearance rate: %3f%%\n", (float) mob_rate / 1000 );
				kondrieu_rate = (unsigned) mob_calc;
				break;
			}
			//debug ("Actual rate: %3f%%\n", ((float) times_won / 1000000) * 100);
		}
		printf ("\nNow ready to serve players...\n");
		logon_ready = 1;
		break;
	case 0x11:
		// Ping received
		ship->last_ping = (unsigned) servertime;
		*(unsigned *) &ship->encryptbuf[0x00] = *(unsigned *) &ship->decryptbuf[0x04];
		compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x04 );
		break;
	case 0x12:
		// Global announce
		gcn = *(unsigned *) &ship->decryptbuf[0x06];
		GlobalBroadcast ((unsigned short*) &ship->decryptbuf[0x0A]);
		WriteGM ("GM %u made a global announcement: %s", gcn, Unicode_to_ASCII ((unsigned short*) &ship->decryptbuf[0x0A]));
		break;
	default:
		// Unknown
		break;
	}
}

void AddPB ( unsigned char* flags, unsigned char* blasts, unsigned char pb )
{
	int pb_exists = 0;
	unsigned char pbv;
	unsigned pb_slot;

	if ( ( *flags & 0x01 ) == 0x01 )
	{
		if ( ( *blasts & 0x07 ) == pb )
			pb_exists = 1;
	}

	if ( ( *flags & 0x02 ) == 0x02 )
	{
		if ( ( ( *blasts / 8 ) & 0x07 ) == pb )
			pb_exists = 1;
	}

	if ( ( *flags  & 0x04 ) == 0x04 )
		pb_exists = 1;

	if (!pb_exists)
	{
		if ( ( *flags & 0x01 ) == 0 )
			pb_slot = 0;
		else
		if ( ( *flags & 0x02 ) == 0 )
			pb_slot = 1;
		else
			pb_slot = 2;
		switch ( pb_slot )
		{
		case 0x00:
			*blasts &= 0xF8;
			*flags  |= 0x01;
			break;
		case 0x01:
			pb *= 8;
			*blasts &= 0xC7;
			*flags  |= 0x02;
			break;
		case 0x02:
			pbv = pb;
			if ( ( *blasts & 0x07 ) < pb )
				pbv--;
			if ( ( ( *blasts / 8 ) & 0x07 ) < pb )
				pbv--;
			pb = pbv * 0x40;
			*blasts &= 0x3F;
			*flags  |= 0x04;
		}
		*blasts |= pb;
	}
}


int MagAlignment ( MAG* m )
{
	int v1, v2, v3, v4, v5, v6;

	v4 = 0;
	v3 = m->power;
	v2 = m->dex;
	v1 = m->mind;
	if ( v2 < v3 )
	{
		if ( v1 < v3 )
			v4 = 8;
	}
	if ( v3 < v2 )
	{
		if ( v1 < v2 )
			v4 |= 0x10u;
	}
	if ( v2 < v1 )
	{
		if ( v3 < v1 )
			v4 |= 0x20u;
	}
	v6 = 0;
	v5 = v3;
	if ( v3 <= v2 )
		v5 = v2;
	if ( v5 <= v1 )
		v5 = v1;
	if ( v5 == v3 )
		v6 = 1;
	if ( v5 == v2 )
		++v6;
	if ( v5 == v1 )
		++v6;
	if ( v6 >= 2 )
		v4 |= 0x100u;
	return v4;
}

int MagSpecialEvolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	unsigned char oldType;
	short mDefense, mPower, mDex, mMind;

	oldType = m->mtype;

	if (m->level >= 100)
	{
		mDefense = m->defense / 100;
		mPower = m->power / 100;
		mDex = m->dex / 100;
		mMind = m->mind / 100;

		switch ( sectionID )
		{
		case ID_Viridia:
		case ID_Bluefull:
		case ID_Redria:
		case ID_Whitill:
			if ( ( mDefense + mDex ) == ( mPower + mMind ) )
			{
				switch ( type )
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Deva;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Sato;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Skyly:
		case ID_Pinkal:
		case ID_Yellowboze:
			if ( ( mDefense + mPower ) == ( mDex + mMind ) )
			{
				switch ( type )
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Dewari;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Greennill:
		case ID_Oran:
		case ID_Purplenum:
			if ( ( mDefense + mMind ) == ( mPower + mDex ) )
			{
				switch ( type )
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		}
	}
	return (int)(oldType != m->mtype);
}

void MagLV50Evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	int v10, v11, v12, v13;

	int Alignment = MagAlignment ( m );

	if ( EvolutionClass > 3 ) // Don't bother to check if a special mag.
		return;

	v10 = m->power / 100;
	v11 = m->dex / 100;
	v12 = m->mind / 100;
	v13 = m->defense / 100;

	switch ( type )
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if ( Alignment & 0x108 )
		{
			if ( sectionID & 1 )
			{
				if ( v12 > v11 )
				{
					m->mtype = Mag_Apsaras;
					AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					m->mtype = Mag_Kama;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
			}
			else
			{
				if ( v12 > v11 )
				{
					m->mtype = Mag_Bhirava;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				}
			}
		}
		else
		{
			if ( Alignment & 0x10 )
			{
				if ( sectionID & 1 )
				{
					if ( v10 > v12 )
					{
						m->mtype = Mag_Garuda;
						AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Yaksa;
						AddPB ( &m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if ( v10 > v12 )
					{
						m->mtype = Mag_Ila;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Nandin;
						AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
					}
				}
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					if ( sectionID & 1 )
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Soma;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Bana;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
					}
					else
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Ushasu;
							AddPB ( &m->PBflags, &m->blasts, PB_Golla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
					}
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if ( Alignment & 0x110 )
		{
			if ( sectionID & 1 )
			{
				if ( v10 > v12 )
				{
					m->mtype = Mag_Kaitabha;
					AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				}
			}
			else
			{
				if ( v10 > v12 )
				{
					m->mtype = Mag_Bhirava;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Kama;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				}
			}
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				if ( sectionID & 1 )
				{
					if ( v12 > v11 )
					{
						m->mtype = Mag_Kaitabha;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Madhu;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
				}
				else
				{
					if ( v12 > v11 )
					{
						m->mtype = Mag_Bhirava;
						AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Kama;
						AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					}
				}
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					if ( sectionID & 1 )
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Durga;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
					}
					else
					{
						if ( v11 > v10 )
						{
							m->mtype = Mag_Apsaras;
							AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Varaha;
							AddPB ( &m->PBflags, &m->blasts, PB_Golla);
						}
					}
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if ( Alignment & 0x120 )
		{
			if ( v13 > 44 )
			{
				m->mtype = Mag_Bana;
				AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
			}
			else
			{
				if ( sectionID & 1 )
				{
					if ( v11 > v10 )
					{
						m->mtype = Mag_Ila;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Kumara;
						AddPB ( &m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if ( v11 > v10 )
					{
						m->mtype = Mag_Kabanda;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Naga;
						AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
				}
			}
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				if ( v13 > 44 )
				{
					m->mtype = Mag_Andhaka;
					AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					if ( sectionID & 1 )
					{
						if ( v12 > v11 )
						{
							m->mtype = Mag_Naga;
							AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
						else
						{
							m->mtype = Mag_Marica;
							AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
						}
					}
					else
					{
						if ( v12 > v11 )
						{
							m->mtype = Mag_Ravana;
							AddPB ( &m->PBflags, &m->blasts, PB_Farlla);
						}
						else
						{
							m->mtype = Mag_Naraka;
							AddPB ( &m->PBflags, &m->blasts, PB_Golla);
						}
					}
				}
			}
			else
			{
				if ( Alignment & 0x10 )
				{
					if ( v13 > 44 )
					{
						m->mtype = Mag_Bana;
						AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
					}
					else
					{
						if ( sectionID & 1 )
						{
							if ( v10 > v12 )
							{
								m->mtype = Mag_Garuda;
								AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
							}
							else
							{
								m->mtype = Mag_Bhirava;
								AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
							}
						}
						else
						{
							if ( v10 > v12 )
							{
								m->mtype = Mag_Ribhava;
								AddPB ( &m->PBflags, &m->blasts, PB_Farlla);
							}
							else
							{
								m->mtype = Mag_Sita;
								AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
							}
						}
					}
				}
			}
		}
		break;
	}
}

void MagLV35Evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	int Alignment = MagAlignment ( m );

	if ( EvolutionClass > 3 ) // Don't bother to check if a special mag.
		return;

	switch ( type )
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if ( Alignment & 0x108 )
		{
			m->mtype = Mag_Rudra;
			AddPB ( &m->PBflags, &m->blasts, PB_Golla);
			return;
		}
		else
		{
			if ( Alignment & 0x10 )
			{
				m->mtype = Mag_Marutah;
				AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
				return;
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					m->mtype = Mag_Vayu;
					AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if ( Alignment & 0x110 )
		{
			m->mtype = Mag_Mitra;
			AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
			return;
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				m->mtype = Mag_Surya;
				AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if ( Alignment & 0x20 )
				{
					m->mtype = Mag_Tapas;
					AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if ( Alignment & 0x120 )
		{
			m->mtype = Mag_Namuci;
			AddPB ( &m->PBflags, &m->blasts, PB_Mylla_Youlla);
			return;
		}
		else
		{
			if ( Alignment & 0x08 )
			{
				m->mtype = Mag_Sumba;
				AddPB ( &m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if ( Alignment & 0x10 )
				{
					m->mtype = Mag_Ashvinau;
					AddPB ( &m->PBflags, &m->blasts, PB_Pilla);
					return;
				}
			}
		}
		break;
	}
}

void MagLV10Evolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	switch ( type )
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		m->mtype = Mag_Varuna;
		AddPB ( &m->PBflags, &m->blasts, PB_Farlla);
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		m->mtype = Mag_Kalki;
		AddPB ( &m->PBflags, &m->blasts, PB_Estlla);
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		m->mtype = Mag_Vritra;
		AddPB ( &m->PBflags, &m->blasts, PB_Leilla);
		break;
	}
}

void CheckMagEvolution ( MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass )
{
	if ( ( m->level < 10 ) || ( m->level >= 35 ) )
	{
		if ( ( m->level < 35 ) || ( m->level >= 50 ) )
		{
			if ( m->level >= 50 )
			{
				if ( ! ( m->level % 5 ) ) // Divisible by 5 with no remainder.
				{
					if ( EvolutionClass <= 3 )
					{
						if ( !MagSpecialEvolution ( m, sectionID, type, EvolutionClass ) )
							MagLV50Evolution ( m, sectionID, type, EvolutionClass );
					}
				}
			}
		}
		else
		{
			if ( EvolutionClass < 2 )
				MagLV35Evolution ( m, sectionID, type, EvolutionClass );
		}
	}
	else
	{
		if ( EvolutionClass <= 0 )
			MagLV10Evolution ( m, sectionID, type, EvolutionClass );
	}
}


void FeedMag (unsigned magid, unsigned itemid, BANANA* client)
{
	int found_mag = -1;
	int found_item = -1;
	unsigned ch, ch2, mt_index;
	int EvolutionClass = 0;
	MAG* m;
	unsigned short* ft;
	short mIQ, mDefense, mPower, mDex, mMind;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == magid)
		{
			// Found mag
			if ((client->character.inventory[ch].item.data[0] == 0x02) &&
				(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
			{
				found_mag = ch;
				m = (MAG*) &client->character.inventory[ch].item.data[0];
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
				{
					if (client->character.inventory[ch2].item.itemid == itemid)
					{
						// Found item to feed
						if  (( client->character.inventory[ch2].item.data[0] == 0x03 ) &&
							( client->character.inventory[ch2].item.data[1]  < 0x07 ) &&
							( client->character.inventory[ch2].item.data[1] != 0x02 ) &&
							( client->character.inventory[ch2].item.data[5] >  0x00 ))
						{
							found_item = ch2;
							switch (client->character.inventory[ch2].item.data[1])
							{
							case 0x00:
								mt_index = client->character.inventory[ch2].item.data[2];
								break;
							case 0x01:
								mt_index = 3 + client->character.inventory[ch2].item.data[2];
								break;
							case 0x03:
							case 0x04:
							case 0x05:
								mt_index = 5 + client->character.inventory[ch2].item.data[1];
								break;
							case 0x06:
								mt_index = 6 + client->character.inventory[ch2].item.data[2];
								break;
							}
						}
						break;
					}
				}
			}
			break;
		}
	}

	if ( ( found_mag == -1 ) || ( found_item == -1 ) )
	{
		Send1A ("Could not find mag to feed or item to feed said mag.", client);
		client->todc = 1;
	}
	else
	{
		DeleteItemFromClient ( itemid, 1, 0, client );

		// Rescan to update Mag pointer (if changed due to clean up)
		for (ch=0;ch<client->character.inventoryUse;ch++)
		{
			if (client->character.inventory[ch].item.itemid == magid)
			{
				// Found mag (again)
				if ((client->character.inventory[ch].item.data[0] == 0x02) &&
					(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
				{
					found_mag = ch;
					m = (MAG*) &client->character.inventory[ch].item.data[0];
					break;
				}
			}
		}
			
		// Feed that mag (Updates to code by Lee from schtserv.com)
		switch (m->mtype)
		{
		case Mag_Mag:
			ft = &Feed_Table0[0];
			EvolutionClass = 0;
			break;
		case Mag_Varuna:
		case Mag_Vritra:
		case Mag_Kalki:
			EvolutionClass = 1;
			ft = &Feed_Table1[0];
			break;
		case Mag_Ashvinau:
		case Mag_Sumba:
		case Mag_Namuci:
		case Mag_Marutah:
		case Mag_Rudra:
			ft = &Feed_Table2[0];
			EvolutionClass = 2;
			break;
		case Mag_Surya:
		case Mag_Tapas:
		case Mag_Mitra:
			ft = &Feed_Table3[0];
			EvolutionClass = 2;
			break;
		case Mag_Apsaras:
		case Mag_Vayu:
		case Mag_Varaha:
		case Mag_Ushasu:
		case Mag_Kama:
		case Mag_Kaitabha:
		case Mag_Kumara:
		case Mag_Bhirava:
			EvolutionClass = 3;
			ft = &Feed_Table4[0];
			break;
		case Mag_Ila:
		case Mag_Garuda:
		case Mag_Sita:
		case Mag_Soma:
		case Mag_Durga:
		case Mag_Nandin:
		case Mag_Yaksa:
		case Mag_Ribhava:
			EvolutionClass = 3;
			ft = &Feed_Table5[0];
			break;
		case Mag_Andhaka:
		case Mag_Kabanda:
		case Mag_Naga:
		case Mag_Naraka:
		case Mag_Bana:
		case Mag_Marica:
		case Mag_Madhu:
		case Mag_Ravana:
			EvolutionClass = 3;
			ft = &Feed_Table6[0];
			break;
		case Mag_Deva:
		case Mag_Rukmin:
		case Mag_Sato:
			ft = &Feed_Table5[0];
			EvolutionClass = 4;
			break;
		case Mag_Rati:
		case Mag_Pushan:
		case Mag_Bhima:
			ft = &Feed_Table6[0];
			EvolutionClass = 4;
			break;
		default:
			ft = &Feed_Table7[0];
			EvolutionClass = 4;
			break;
		}
		mt_index *= 6;
		m->synchro += ft[mt_index];
		if (m->synchro < 0)
			m->synchro = 0;
		if (m->synchro > 120)
			m->synchro = 120;
		mIQ = m->IQ;
		mIQ += ft[mt_index+1];
		if (mIQ < 0)
			mIQ = 0;
		if (mIQ > 200)
			mIQ = 200;
		m->IQ = (unsigned char) mIQ;

		// Add Defense

		mDefense  = m->defense % 100;
		mDefense += ft[mt_index+2];

		if ( mDefense < 0 )
			mDefense = 0;

		if ( mDefense >= 100 )
		{
			if ( m->level == 200 )
				mDefense = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->defense  = ( ( m->defense / 100 ) * 100 ) + mDefense;
			CheckMagEvolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->defense  = ( ( m->defense / 100 ) * 100 ) + mDefense;

		// Add Power

		mPower  = m->power % 100;
		mPower += ft[mt_index+3];

		if ( mPower < 0 )
			mPower = 0;

		if ( mPower >= 100 )
		{
			if ( m->level == 200 )
				mPower = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->power  = ( ( m->power / 100 ) * 100 ) + mPower;
			CheckMagEvolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->power  = ( ( m->power / 100 ) * 100 ) + mPower;

		// Add Dex

		mDex  = m->dex % 100;
		mDex += ft[mt_index+4];

		if ( mDex < 0 )
			mDex = 0;

		if ( mDex >= 100 )
		{
			if ( m->level == 200 )
				mDex = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->dex  = ( ( m->dex / 100 ) * 100 ) + mDex;
			CheckMagEvolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->dex  = ( ( m->dex / 100 ) * 100 ) + mDex;

		// Add Mind

		mMind  = m->mind % 100;
		mMind += ft[mt_index+5];

		if ( mMind < 0 )
			mMind = 0;

		if ( mMind >= 100 )
		{
			if ( m->level == 200 )
				mMind = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->mind  = ( ( m->mind / 100 ) * 100 ) + mMind;
			CheckMagEvolution ( m, client->character.sectionID, client->character._class, EvolutionClass );
		}
		else
			m->mind  = ( ( m->mind / 100 ) * 100 ) + mMind;
	}
}

void CheckMaxGrind (INVENTORY_ITEM* i)
{
	if (i->item.data[3] > grind_table[i->item.data[1]][i->item.data[2]])
		i->item.data[3] = grind_table[i->item.data[1]][i->item.data[2]];
}


void UseItem (unsigned itemid, BANANA* client)
{
	unsigned found_item = 0, ch, ch2;
	INVENTORY_ITEM i;
	int eq_wep, eq_armor, eq_shield, eq_mag = -1;
	LOBBY* l;
	unsigned new_item, TotalMatUse, HPMatUse, max_mat;
	int mat_exceed;

	// Check item stuff here...  Like converting certain things to certain things...
	//

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;

			// Copy item before deletion (needed for consumables)
			memcpy (&i, &client->character.inventory[ch], sizeof (INVENTORY_ITEM));

			// Unwrap mag
			if ((i.item.data[0] == 0x02) && (i.item.data2[2] & 0x40))
			{
				client->character.inventory[ch].item.data2[2] &= ~(0x40);
				break;
			}

			// Unwrap item
			if ((i.item.data[0] != 0x02) && (i.item.data[4] & 0x40))
			{
				client->character.inventory[ch].item.data[4] &= ~(0x40);
				break;
			}

			if (i.item.data[0] == 0x03) // Delete consumable item right away
				DeleteItemFromClient (itemid, 1, 0, client);

			break;
		}
	}

	if (!found_item)
	{
		Send1A ("Could not find item to \"use\".", client);
		client->todc = 1;
	}
	else
	{
		// Setting the eq variables here should fix problem with ADD SLOT and such.
		eq_wep = eq_armor = eq_shield = eq_mag = -1;

		for (ch2=0;ch2<client->character.inventoryUse;ch2++)
		{
			if ( client->character.inventory[ch2].flags & 0x08 )
			{
				switch ( client->character.inventory[ch2].item.data[0] )
				{
				case 0x00:
					eq_wep = ch2;
					break;
				case 0x01:
					switch ( client->character.inventory[ch2].item.data[1] )
					{
					case 0x01:
						eq_armor = ch2;
						break;
					case 0x02:
						eq_shield = ch2;
						break;
					}
					break;
				case 0x02:
					eq_mag = ch2;
					break;
				}
			}
		}

		switch (i.item.data[0])
		{
		case 0x00:
			switch (i.item.data[1])
			{
			case 0x33:
				client->character.inventory[ch].item.data[1] = 0x32; // Sealed J-Sword -> Tsumikiri J-Sword
				SendItemToEnd (itemid, client);
				break;
			case 0x1E:
				// Heaven Punisher used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xAF) &&
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB0; // Mille Marteaux
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Heaven Punisher
				break;
			case 0x42:
				// Handgun: Guld or Master Raven used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x43) && 
					(client->character.inventory[eq_wep].item.data[2] == i.item.data[2]) &&
					(client->character.inventory[eq_wep].item.data[3] == 0x09))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x4B; // Guld Milla or Dual Bird
					client->character.inventory[eq_wep].item.data[2] = i.item.data[2];
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Guld or Raven...
				break;
			case 0x43:
				// Handgun: Milla or Last Swan used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x42) && 
					(client->character.inventory[eq_wep].item.data[2] == i.item.data[2]) &&
					(client->character.inventory[eq_wep].item.data[3] == 0x09))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x4B; // Guld Milla or Dual Bird
					client->character.inventory[eq_wep].item.data[2] = i.item.data[2];
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Milla or Swan...
				break;
			case 0x8A:
				// Sange or Yasha...
				if (eq_wep != -1)
				{
					if (client->character.inventory[eq_wep].item.data[2] == !(i.item.data[2]))
					{
						client->character.inventory[eq_wep].item.data[1] = 0x89;
						client->character.inventory[eq_wep].item.data[2] = 0x03;
						client->character.inventory[eq_wep].item.data[3] = 0x00;
						client->character.inventory[eq_wep].item.data[4] = 0x00;
						SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
					}
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of the other sword...
				break;
			case 0xAB:
				client->character.inventory[ch].item.data[1] = 0xAC; // Convert Lame d'Argent into Excalibur
				SendItemToEnd (itemid, client);
				break;
			case 0xAF:
				// Ophelie Seize used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x1E) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB0; // Mille Marteaux
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Ophelie Seize
				break;
			case 0xB6:
				// Guren used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xB7) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB8; // Jizai
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Guren
				break;
			case 0xB7:
				// Shouren used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xB6) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB8; // Jizai
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Shouren
				break;
			}
			break;
		case 0x01:
			if (i.item.data[1] == 0x03)
			{
				if (i.item.data[2] == 0x4D) // Limiter -> Adept
				{
					client->character.inventory[ch].item.data[2] = 0x4E;
					SendItemToEnd (itemid, client);
				}

				if (i.item.data[2] == 0x4F) // Swordsman Lore -> Proof of Sword-Saint
				{
					client->character.inventory[ch].item.data[2] = 0x50;
					SendItemToEnd (itemid, client);
				}
			}
			break;
		case 0x02:
			switch (i.item.data[1])
			{
			case 0x2B:
				// Chao Mag used
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x68) && 
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x58; // Striker of Chao
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Chao
				break;
			case 0x2C:
				// Chu Chu mag used
				if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[2] == 0x1C))
				{
					client->character.inventory[eq_armor].item.data[2] = 0x2C; // Chuchu Fever
					SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
				}
				DeleteItemFromClient (itemid, 1, 0, client); // Get rid of Chu Chu
				break;
			}
			break;
		case 0x03:
			switch (i.item.data[1])
			{
			case 0x02:
				if (i.item.data[4] < 19)
				{
					if (((char)i.item.data[2] > max_tech_level[i.item.data[4]][client->character._class]) ||
						(client->equip_flags & DROID_FLAG))
					{
						Send1A ("You can't learn that technique.", client);
						client->todc = 1;
					}
					else
						client->character.techniques[i.item.data[4]] = i.item.data[2]; // Learn technique
				}
				break;
			case 0x0A:
				if (eq_wep != -1)
				{
					client->character.inventory[eq_wep].item.data[3] += ( i.item.data[2] + 1 );
					CheckMaxGrind (&client->character.inventory[eq_wep]);
					break;
				}
				break;
			case 0x0B:
				if (!client->mode)
				{
					HPMatUse = ( client->character.HPmat + client->character.TPmat ) / 2;
					TotalMatUse = 0;
					for (ch2=0;ch2<5;ch2++)
						TotalMatUse += client->matuse[ch2];
					mat_exceed = 0;
					if ( client->equip_flags & HUMAN_FLAG )
						max_mat = 250;
					else
						max_mat = 150;
				}
				else
				{
					TotalMatUse = 0;
					HPMatUse = 0;
					max_mat = 999;
					mat_exceed = 0;
				}
				switch (i.item.data[2])  // Materials
				{
				case 0x00:
					if ( TotalMatUse < max_mat )
					{
						client->character.ATP += 2;
						if (!client->mode)
							client->matuse[0]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x01:
					if ( TotalMatUse < max_mat )
					{
						client->character.MST += 2;
						if (!client->mode)
							client->matuse[1]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x02:
					if ( TotalMatUse < max_mat )
					{
						client->character.EVP += 2;
						if (!client->mode)
							client->matuse[2]++;
					}
					else 
						mat_exceed = 1;
					break;
				case 0x03:
					if ( ( client->character.HPmat < 250 ) && ( HPMatUse < 250 ) )
						client->character.HPmat += 2;
					else
						mat_exceed = 1;
					break;
				case 0x04:
					if ( ( client->character.TPmat < 250 ) && ( HPMatUse < 250 ) )
						client->character.TPmat += 2;
					else
						mat_exceed = 1;
					break;
				case 0x05:
					if ( TotalMatUse < max_mat )
					{
						client->character.DFP += 2;
						if (!client->mode)
							client->matuse[3]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x06:
					if ( TotalMatUse < max_mat )
					{
						client->character.LCK += 2;
						if (!client->mode)
							client->matuse[4]++;
					}
					else
						mat_exceed = 1;
					break;
				default:
					break;
				}
				if ( mat_exceed )
				{
					Send1A ("Attempt to exceed material usage limit.", client);
					client->todc = 1;
				}
				break;
			case 0x0C:
				switch ( i.item.data[2] )
				{
				case 0x00: // Mag Cell 502
					if ( eq_mag != -1 )
					{
						if ( client->character.sectionID & 0x01 )
							client->character.inventory[eq_mag].item.data[1] = 0x1D;
						else
							client->character.inventory[eq_mag].item.data[1] = 0x21;
					}
					break;
				case 0x01: // Mag Cell 213
					if ( eq_mag != -1 )
					{
						if ( client->character.sectionID & 0x01 )
							client->character.inventory[eq_mag].item.data[1] = 0x27;
						else
							client->character.inventory[eq_mag].item.data[1] = 0x22;
					}
					break;
				case 0x02: // Parts of RoboChao
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x28;
					break;
				case 0x03: // Heart of Opa Opa
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x29;
					break;
				case 0x04: // Heart of Pian
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x2A;
					break;
				case 0x05: // Heart of Chao
					if ( eq_mag != -1 )
						client->character.inventory[eq_mag].item.data[1] = 0x2B;
					break;
				}
				break;
			case 0x0E:
				if ( ( eq_shield != -1 ) && ( i.item.data[2] > 0x15 ) && ( i.item.data[2] < 0x26 ) )
				{
					// Merges
					client->character.inventory[eq_shield].item.data[2] = 0x3A + ( i.item.data[2] - 0x16 );
					SendItemToEnd (client->character.inventory[eq_shield].item.itemid, client);
				}
				else
					switch ( i.item.data[2] )
				{
					case 0x00: 
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x8E ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x8E;
							client->character.inventory[eq_wep].item.data[2]  = 0x01; // S-Berill's Hands #1
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x01: // Parasitic Gene "Flow"
						if ( eq_wep != -1 )
						{
							switch ( client->character.inventory[eq_wep].item.data[1] )
							{
							case 0x02:
								client->character.inventory[eq_wep].item.data[1]  = 0x9D; // Dark Flow
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x09:
								client->character.inventory[eq_wep].item.data[1]  = 0x9E; // Dark Meteor
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x0B:
								client->character.inventory[eq_wep].item.data[1]  = 0x9F; // Dark Bridge
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
						}
						break;
					case 0x02: // Magic Stone "Iritista"
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x05 ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x9C; // Rainbow Baton
							client->character.inventory[eq_wep].item.data[2]  = 0x00;
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x03: // Blue-Black Stone
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x2F ) && 
							( client->character.inventory[eq_wep].item.data[2] == 0x00 ) && 
							( client->character.inventory[eq_wep].item.data[3] == 0x19 ) )
						{
							client->character.inventory[eq_wep].item.data[2]  = 0x01; // Black King Bar
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x04: // Syncesta
						if ( eq_wep != -1 )
						{
							switch ( client->character.inventory[eq_wep].item.data[1] )
							{
							case 0x1F:
								client->character.inventory[eq_wep].item.data[1]  = 0x38; // Lavis Blade
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x38:
								client->character.inventory[eq_wep].item.data[1]  = 0x30; // Double Cannon
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x30:
								client->character.inventory[eq_wep].item.data[1]  = 0x1F; // Lavis Cannon
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
						}
						break;
					case 0x05: // Magic Water
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x56 ) ) 								
						{
							if ( client->character.inventory[eq_wep].item.data[2] == 0x00 )
							{
								client->character.inventory[eq_wep].item.data[1]  = 0x5D; // Plantain Fan
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00;
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
							else
								if ( client->character.inventory[eq_wep].item.data[2] == 0x01 )
								{
									client->character.inventory[eq_wep].item.data[1]  = 0x63; // Plantain Huge Fan
									client->character.inventory[eq_wep].item.data[2]  = 0x00;
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
									break;
								}
						}
						break;
					case 0x06: // Parasitic Cell Type D
						if ( eq_armor != -1 )
							switch ( client->character.inventory[eq_armor].item.data[2] ) 
						{
							case 0x1D:  
								client->character.inventory[eq_armor].item.data[2] = 0x20; // Parasite Wear: De Rol
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x20: 
								client->character.inventory[eq_armor].item.data[2] = 0x21; // Parsite Wear: Nelgal
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x21: 
								client->character.inventory[eq_armor].item.data[2] = 0x22; // Parasite Wear: Vajulla
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x22: 
								client->character.inventory[eq_armor].item.data[2] = 0x2F; // Virus Armor: Lafuteria
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								break;
						}
						break;
					case 0x07: // Magic Rock "Heart Key"
						if ( eq_armor != -1 )
						{
							if ( client->character.inventory[eq_armor].item.data[2] == 0x1C )
							{
								client->character.inventory[eq_armor].item.data[2] = 0x2D; // Love Heart
								SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
							}
							else
								if ( client->character.inventory[eq_armor].item.data[2] == 0x2D ) 
								{
									client->character.inventory[eq_armor].item.data[2] = 0x45; // Sweetheart
									SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
								}
								else
									if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x0C ) )
									{
										client->character.inventory[eq_wep].item.data[1]  = 0x24; // Magical Piece
										client->character.inventory[eq_wep].item.data[2]  = 0x00;
										client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
										client->character.inventory[eq_wep].item.data[4]  = 0x00;
										SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
									}
									else
										if ( ( eq_shield != -1 ) && ( client->character.inventory[eq_shield].item.data[2] == 0x15 ) )
										{
											client->character.inventory[eq_shield].item.data[2]  = 0x2A; // Safety Heart
											SendItemToEnd (client->character.inventory[eq_shield].item.itemid, client);
										}
						}
						break;
					case 0x08: // Magic Rock "Moola"
						if ( ( eq_armor != -1 ) && ( client->character.inventory[eq_armor].item.data[2] == 0x1C ) )
						{
							client->character.inventory[eq_armor].item.data[2] = 0x31; // Aura Field
							SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
						}
						else
							if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x0A ) )
							{
								client->character.inventory[eq_wep].item.data[1]  = 0x4F; // Summit Moon
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00; // No attribute
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							}
							break;
					case 0x09: // Star Amplifier
						if ( ( eq_armor != -1 ) && ( client->character.inventory[eq_armor].item.data[2] == 0x1C ) )
						{
							client->character.inventory[eq_armor].item.data[2] = 0x30; // Brightness Circle
							SendItemToEnd (client->character.inventory[eq_armor].item.itemid, client);
						}
						else
							if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x0C ) )
							{
								client->character.inventory[eq_wep].item.data[1]  = 0x5C; // Twinkle Star
								client->character.inventory[eq_wep].item.data[2]  = 0x00;
								client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4]  = 0x00; // No attribute
								SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
							}
							break;
					case 0x0A: // Book of Hitogata
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x8C ) &&
							( client->character.inventory[eq_wep].item.data[2] == 0x02 ) && 
							( client->character.inventory[eq_wep].item.data[3] == 0x09 ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x8C;
							client->character.inventory[eq_wep].item.data[2]  = 0x03;
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
						}
						break;
					case 0x0B: // Heart of Chu Chu
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x2C;
						break;
					case 0x0C: // Parts of Egg Blaster
						if ( ( eq_wep != -1 ) && ( client->character.inventory[eq_wep].item.data[1] == 0x06 ) )
						{
							client->character.inventory[eq_wep].item.data[1]  = 0x1C; // Egg Blaster
							client->character.inventory[eq_wep].item.data[2]  = 0x00;
							client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4]  = 0x00;
							SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
						}
						break;
					case 0x0D: // Heart of Angel
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x2E;
						break;
					case 0x0E: // Heart of Devil
						if ( eq_mag != -1 )
						{
							if ( client->character.inventory[eq_mag].item.data[1] == 0x2F )
								client->character.inventory[eq_mag].item.data[1] = 0x38;
							else
								client->character.inventory[eq_mag].item.data[1] = 0x2F;
						}
						break;
					case 0x0F: // Kit of Hamburger
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x36;
						break;
					case 0x10: // Panther's Spirit
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x37;
						break;
					case 0x11: // Kit of Mark 3
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x31;
						break;
					case 0x12: // Kit of Master System
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x32;
						break;
					case 0x13: // Kit of Genesis
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x33;
						break;
					case 0x14: // Kit of Sega Saturn
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x34;
						break;
					case 0x15: // Kit of Dreamcast
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x35;
						break;
					case 0x26: // Heart of Kapukapu
						if ( eq_mag != -1 )
							client->character.inventory[eq_mag].item.data[1] = 0x2D;
						break;
					case 0x27: // Photon Booster
						if ( eq_wep != -1 )
						{
							switch ( client->character.inventory[eq_wep].item.data[1] )
							{
							case 0x15:
								if ( client->character.inventory[eq_wep].item.data[3] == 0x09 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Burning Visit
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x45:
								if ( client->character.inventory[eq_wep].item.data[3] == 0x09 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Snow Queen
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x4E:
								if ( client->character.inventory[eq_wep].item.data[3] == 0x09 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Iron Faust
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x6D: 
								if ( client->character.inventory[eq_wep].item.data[3] == 0x14 )
								{
									client->character.inventory[eq_wep].item.data[2]  = 0x01; // Power Maser
									client->character.inventory[eq_wep].item.data[3]  = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4]  = 0x00;
									SendItemToEnd (client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							}
						}
						break;

				}
				break;
			case 0x0F: // Add Slot
				if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[5] < 0x04))
					client->character.inventory[eq_armor].item.data[5]++;
				break;
			case 0x15:
				new_item = 0;
				switch ( i.item.data[2] )
				{
				case 0x00:
					new_item = 0x0D0E03 + ( ( mt_lrand() % 9 ) * 0x10000 );
					break;
				case 0x01:
					new_item = easter_drops[mt_lrand() % 9];
					break;
				case 0x02:
					new_item = jacko_drops[mt_lrand() % 8];
					break;
				default:
					break;
				}

				if ( new_item )
				{
					INVENTORY_ITEM add_item;

					memset (&add_item, 0, sizeof (INVENTORY_ITEM));
					*(unsigned *) &add_item.item.data[0] = new_item;
					add_item.item.itemid = l->playerItemID[client->clientID];
					l->playerItemID[client->clientID]++;
					AddToInventory (&add_item, 1, 0, client);
				}
				break;
			case 0x18: // Ep4 Mag Cells
				if ( eq_mag != -1 )
					client->character.inventory[eq_mag].item.data[1] = 0x42 + ( i.item.data[2] );
				break;
			}
			break;
		default:
			break;
		}
	}
}

int check_equip (unsigned char eq_flags, unsigned char cl_flags)
{
	int eqOK = 1;
	unsigned ch;

	for (ch=0;ch<8;ch++)
	{
		if ( ( cl_flags & (1 << ch) ) && (!( eq_flags & ( 1 << ch ) )) )
		{
			eqOK = 0;
			break;
		}
	}
	return eqOK;
}

void EquipItem (unsigned itemid, BANANA* client)
{
	unsigned ch, ch2, found_item, found_slot;
	unsigned slot[4];

	found_item = 0;
	found_slot = 0;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			//debug ("Equipped %u", itemid);
			found_item = 1;
			switch (client->character.inventory[ch].item.data[0])
			{
			case 0x00:
				// Check weapon equip requirements
				if (!check_equip(weapon_equip_table[client->character.inventory[ch].item.data[1]][client->character.inventory[ch].item.data[2]], client->equip_flags))
				{
					Send1A ("\"God/Equip\" is disallowed.", client);
					client->todc = 1;
				}
				if (!client->todc)
				{
					// De-equip any other weapon on character. (Prevent stacking)
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
							(client->character.inventory[ch2].flags & 0x08))
							client->character.inventory[ch2].flags &= ~(0x08);
				}
				break;
			case 0x01:
				switch (client->character.inventory[ch].item.data[1])
				{
				case 0x01: // Check armor equip requirements
					if ((!check_equip (armor_equip_table[client->character.inventory[ch].item.data[2]], client->equip_flags)) || 
						(client->character.level < armor_level_table[client->character.inventory[ch].item.data[2]]))
					{
						Send1A ("\"God/Equip\" is disallowed.", client);
						client->todc = 1;
					}
					if (!client->todc)
					{
						// Remove any other armor and equipped slot items.
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].flags &= ~(0x08);
								client->character.inventory[ch2].item.data[4] = 0x00;
							}
							break;
					}
					break;
				case 0x02: // Check barrier equip requirements
					if ((!check_equip (barrier_equip_table[client->character.inventory[ch].item.data[2]] & client->equip_flags, client->equip_flags)) ||
						(client->character.level < barrier_level_table[client->character.inventory[ch].item.data[2]]))
					{
						Send1A ("\"God/Equip\" is disallowed.", client);
						client->todc = 1;
					}
					if (!client->todc)
					{
						// Remove any other barrier
						for (ch2=0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] == 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].flags &= ~(0x08);
								client->character.inventory[ch2].item.data[4] = 0x00;
							}
					}
					break;
				case 0x03: // Assign unit a slot
					for (ch2=0;ch2<4;ch2++)
						slot[ch2] = 0;
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					{
						// Another loop ;(
						if ((client->character.inventory[ch2].item.data[0] == 0x01) && 
							(client->character.inventory[ch2].item.data[1] == 0x03))
						{
							if ((client->character.inventory[ch2].flags & 0x08) && 
								(client->character.inventory[ch2].item.data[4] < 0x04))
								slot [client->character.inventory[ch2].item.data[4]] = 1;
						}
					}
					for (ch2=0;ch2<4;ch2++)
						if (slot[ch2] == 0)
						{
							found_slot = ch2 + 1;
							break;
						}
						if (found_slot)
						{
							found_slot --;
							client->character.inventory[ch].item.data[4] = (unsigned char)(found_slot);
						}
						else
						{
							client->character.inventory[ch].flags &= ~(0x08);
							Send1A ("There are no free slots on your armor.  Equip unit failed.", client);
							client->todc = 1;
						}
						break;
				}
				break;
			case 0x02:
				// Remove equipped mag
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			if ( !client->todc )  // Finally, equip item
				client->character.inventory[ch].flags |= 0x08;
			break;
		}
	}
	if (!found_item)
	{
		Send1A ("Could not find item to equip.", client);
		client->todc = 1;
	}
}

void DeequipItem (unsigned itemid, BANANA* client)
{
	unsigned ch, ch2, found_item = 0;

	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;
			client->character.inventory[ch].flags &= ~(0x08);
			switch (client->character.inventory[ch].item.data[0])
			{
			case 0x00:
				// Remove any other weapon (in case of a glitch... or stacking)
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			case 0x01:
				switch (client->character.inventory[ch].item.data[1])
				{
				case 0x01:
					// Remove any other armor (stacking?) and equipped slot items.
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
							(client->character.inventory[ch2].item.data[1] != 0x02) &&
							(client->character.inventory[ch2].flags & 0x08))
						{
							client->character.inventory[ch2].flags &= ~(0x08);
							client->character.inventory[ch2].item.data[4] = 0x00;
						}
						break;
				case 0x02:
					// Remove any other barrier (stacking?)
					for (ch2=0;ch2<client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
							(client->character.inventory[ch2].item.data[1] == 0x02) &&
							(client->character.inventory[ch2].flags & 0x08))
						{
							client->character.inventory[ch2].flags &= ~(0x08);
							client->character.inventory[ch2].item.data[4] = 0x00;
						}
						break;
				case 0x03:
					// Remove unit from slot
					client->character.inventory[ch].item.data[4] = 0x00;
					break;
				}
				break;
			case 0x02:
				// Remove any other mags (stacking?)
				for (ch2=0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			break;
		}
	}
	if (!found_item)
	{
		Send1A ("Could not find item to unequip.", client);
		client->todc = 1;
	}
}

unsigned GetShopPrice(INVENTORY_ITEM* ci)
{
	unsigned compare_item, ch;
	int percent_add, price;
	unsigned char variation;
	float percent_calc;
	float price_calc;

	price = 10;

/*	printf ("Raw item data for this item is:\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n", 
		ci->item.data[0], ci->item.data[1], ci->item.data[2], ci->item.data[3], 
		ci->item.data[4], ci->item.data[5], ci->item.data[6], ci->item.data[7], 
		ci->item.data[8], ci->item.data[9], ci->item.data[10], ci->item.data[11], 
		ci->item.data[12], ci->item.data[13], ci->item.data[14], ci->item.data[15] ); */

	switch (ci->item.data[0])
	{
	case 0x00: // Weapons
		if (ci->item.data[4] & 0x80)
			price  = 1; // Untekked = 1 meseta
		else
		{
			if ((ci->item.data[1] < 0x0D) && (ci->item.data[2] < 0x05))
			{
				if ((ci->item.data[1] > 0x09) && (ci->item.data[2] > 0x03)) // Canes, Rods, Wands become rare faster
					break;
				price = weapon_atpmax_table[ci->item.data[1]][ci->item.data[2]] + ci->item.data[3];
				price *= price;
				price_calc = (float) price;
				switch (ci->item.data[1])
				{
				case 0x01:
					price_calc /= 5.0;
					break;
				case 0x02:
					price_calc /= 4.0;
					break;
				case 0x03:
				case 0x04:
					price_calc *= 2.0;
					price_calc /= 3.0;
					break;
				case 0x05:
					price_calc *= 4.0;
					price_calc /= 5.0;
					break;
				case 0x06:
					price_calc *= 10.0;
					price_calc /= 21.0;
					break;
				case 0x07:
					price_calc /= 3.0;
					break;
				case 0x08:
					price_calc *= 25.0;
					break;
				case 0x09:
					price_calc *= 10.0;
					price_calc /= 9.0;
					break;
				case 0x0A:
					price_calc /= 2.0;
					break;
				case 0x0B:
					price_calc *= 2.0;
					price_calc /= 5.0;
					break;
				case 0x0C:
					price_calc *= 4.0;
					price_calc /= 3.0;
					break;
				}

				percent_add = 0;
				if (ci->item.data[6])
					percent_add += (char) ci->item.data[7];
				if (ci->item.data[8])
					percent_add += (char) ci->item.data[9];
				if (ci->item.data[10])
					percent_add += (char) ci->item.data[11];

				if ( percent_add != 0 )
				{
					percent_calc = price_calc;
					percent_calc /= 300.0;
					percent_calc *= percent_add;
					price_calc += percent_calc;
				}
				price_calc /= 8.0;
				price = (int) ( price_calc );
				price += attrib[ci->item.data[4]];
			}
		}
		break;
	case 0x01:
		switch (ci->item.data[1])
		{
		case 0x01: // Armor
			if (ci->item.data[2] < 0x18)
			{
				// Calculate the amount to boost because of slots...
				if (ci->item.data[5] > 4)
					price = armor_prices[(ci->item.data[2] * 5) + 4];
				else
					price = armor_prices[(ci->item.data[2] * 5) + ci->item.data[5]];
				price -= armor_prices[(ci->item.data[2] * 5)];
				if (ci->item.data[6] > armor_dfpvar_table[ci->item.data[2]])
					variation = 0;
				else
					variation = ci->item.data[6];
				if (ci->item.data[8] <= armor_evpvar_table[ci->item.data[2]])
					variation += ci->item.data[8];
				price += equip_prices[1][1][ci->item.data[2]][variation];
			}
			break;
		case 0x02: // Shield
			if (ci->item.data[2] < 0x15)
			{
				if (ci->item.data[6] > barrier_dfpvar_table[ci->item.data[2]])
					variation = 0;
				else
					variation = ci->item.data[6];
				if (ci->item.data[8] <= barrier_evpvar_table[ci->item.data[2]])
					variation += ci->item.data[8];
				price = equip_prices[1][2][ci->item.data[2]][variation];
			}
			break;
		case 0x03: // Units
			if (ci->item.data[2] < 0x40)
				price = unit_prices [ci->item.data[2]];
			break;
		}
		break;
	case 0x03:
		// Tool
		if (ci->item.data[1] == 0x02) // Technique
		{
			if (ci->item.data[4] < 0x13)
				price = ((int) (ci->item.data[2] + 1) * tech_prices[ci->item.data[4]]) / 100L;
		}
		else
		{
			compare_item = 0;
			memcpy (&compare_item, &ci->item.data[0], 3);
			for (ch=0;ch<(sizeof(tool_prices)/4);ch+=2)
				if (compare_item == tool_prices[ch])
				{
					price = tool_prices[ch+1];
					break;
				}		
		}
		break;
	}
	if ( price < 0 )
		price = 0;
	//printf ("GetShopPrice = %u\n", price);
	return (unsigned) price;
}

void SkipToLevel (unsigned short target_level, BANANA* client, int quiet)
{
	MAG* m;
	unsigned short ch, finalDFP, finalATP, finalATA, finalMST;

	if (target_level > 199)
		target_level = 199;

	if ((!client->lobby) || (client->character.level >= target_level))
		return;

	if (!quiet)
	{
		PacketBF[0x0A] = (unsigned char) client->clientID;
		*(unsigned *) &PacketBF[0x0C] = tnlxp [target_level - 1] - client->character.XP;
		SendToLobby (client->lobby, 4, &PacketBF[0], 0x10, 0);
	}

	while (client->character.level < target_level)
	{
		client->character.ATP += playerLevelData[client->character._class][client->character.level].ATP;
		client->character.MST += playerLevelData[client->character._class][client->character.level].MST;
		client->character.EVP += playerLevelData[client->character._class][client->character.level].EVP;
		client->character.HP  += playerLevelData[client->character._class][client->character.level].HP;
		client->character.DFP += playerLevelData[client->character._class][client->character.level].DFP;
		client->character.ATA += playerLevelData[client->character._class][client->character.level].ATA;
		client->character.level++;
	}
	
	client->character.XP = tnlxp [target_level - 1];

	finalDFP = client->character.DFP;
	finalATP = client->character.ATP;
	finalATA = client->character.ATA;
	finalMST = client->character.MST;

	// Add the mag bonus to the 0x30 packet
	for (ch=0;ch<client->character.inventoryUse;ch++)
	{
		if ((client->character.inventory[ch].item.data[0] == 0x02) &&
			(client->character.inventory[ch].flags & 0x08))
		{
			m = (MAG*) &client->character.inventory[ch].item.data[0];
			finalDFP += ( m->defense / 100 );
			finalATP += ( m->power / 100 ) * 2;
			finalATA += ( m->dex / 100 ) / 2;
			finalMST += ( m->mind / 100 ) * 2;
			break;
		}
	}

	if (!quiet)
	{
		*(unsigned short*) &Packet30[0x0C] = finalATP;
		*(unsigned short*) &Packet30[0x0E] = finalMST;
		*(unsigned short*) &Packet30[0x10] = client->character.EVP;
		*(unsigned short*) &Packet30[0x12] = client->character.HP;
		*(unsigned short*) &Packet30[0x14] = finalDFP;
		*(unsigned short*) &Packet30[0x16] = finalATA;
		*(unsigned short*) &Packet30[0x18] = client->character.level;
		Packet30[0x0A] = (unsigned char) client->clientID;
		SendToLobby ( client->lobby, 4, &Packet30[0x00], 0x1C, 0);
	}
}


void AddExp (unsigned XP, BANANA* client)
{
	MAG* m;
	unsigned short levelup, ch, finalDFP, finalATP, finalATA, finalMST;

	if (!client->lobby)
		return;

	client->character.XP += XP;

	PacketBF[0x0A] = (unsigned char) client->clientID;
	*(unsigned *) &PacketBF[0x0C] = XP;
	SendToLobby (client->lobby, 4, &PacketBF[0], 0x10, 0);
	if (client->character.XP >= tnlxp[client->character.level])
		levelup = 1;
	else
		levelup = 0;

	while (levelup)
	{
		client->character.ATP += playerLevelData[client->character._class][client->character.level].ATP;
		client->character.MST += playerLevelData[client->character._class][client->character.level].MST;
		client->character.EVP += playerLevelData[client->character._class][client->character.level].EVP;
		client->character.HP  += playerLevelData[client->character._class][client->character.level].HP;
		client->character.DFP += playerLevelData[client->character._class][client->character.level].DFP;
		client->character.ATA += playerLevelData[client->character._class][client->character.level].ATA;
		client->character.level++;
		if ((client->character.level == 199) || (client->character.XP < tnlxp[client->character.level]))
			break;
	}

	if (levelup)
	{
		finalDFP = client->character.DFP;
		finalATP = client->character.ATP;
		finalATA = client->character.ATA;
		finalMST = client->character.MST;

		// Add the mag bonus to the 0x30 packet
		for (ch=0;ch<client->character.inventoryUse;ch++)
		{
			if ((client->character.inventory[ch].item.data[0] == 0x02) &&
				(client->character.inventory[ch].flags & 0x08))
			{
				m = (MAG*) &client->character.inventory[ch].item.data[0];
				finalDFP += ( m->defense / 100 );
				finalATP += ( m->power / 100 ) * 2;
				finalATA += ( m->dex / 100 ) / 2;
				finalMST += ( m->mind / 100 ) * 2;
				break;
			}
		}

		*(unsigned short*) &Packet30[0x0C] = finalATP;
		*(unsigned short*) &Packet30[0x0E] = finalMST;
		*(unsigned short*) &Packet30[0x10] = client->character.EVP;
		*(unsigned short*) &Packet30[0x12] = client->character.HP;
		*(unsigned short*) &Packet30[0x14] = finalDFP;
		*(unsigned short*) &Packet30[0x16] = finalATA;
		*(unsigned short*) &Packet30[0x18] = client->character.level;
		Packet30[0x0A] = (unsigned char) client->clientID;
		SendToLobby ( client->lobby, 4, &Packet30[0x00], 0x1C, 0);
	}
}

void PrepGuildCard ( unsigned from, unsigned to )
{
	int gc_present = 0;
	unsigned hightime = 0xFFFFFFFF;
	unsigned highidx = 0;
	unsigned ch;

	if (ship_gcsend_count < MAX_GCSEND)
	{
		for (ch=0;ch<(ship_gcsend_count*3);ch+=3)
		{
			if ((ship_gcsend_list[ch] == from) && (ship_gcsend_list[ch+1] == to))
			{
				gc_present = 1;
				break;
			}
		}

		if (!gc_present)
		{
			highidx = ship_gcsend_count * 3;
			ship_gcsend_count++;
		}
	}
	else
	{
		// Erase oldest sent card
		for (ch=0;ch<(MAX_GCSEND*3);ch+=3)
		{
			if (ship_gcsend_list[ch+2] < hightime)
			{
				hightime = ship_gcsend_list[ch+2];
				highidx  = ch;
			}
		}
	}
	ship_gcsend_list[highidx]	= from;
	ship_gcsend_list[highidx+1]	= to;
	ship_gcsend_list[highidx+2]	= (unsigned) servertime;
}

int PreppedGuildCard ( unsigned from, unsigned to )
{
	int gc_present = 0;
	unsigned ch, ch2;

	for (ch=0;ch<(ship_gcsend_count*3);ch+=3)
	{
		if ((ship_gcsend_list[ch] == from) && (ship_gcsend_list[ch+1] == to))
		{
			ship_gcsend_list[ch]   = 0;
			ship_gcsend_list[ch+1] = 0;
			ship_gcsend_list[ch+2] = 0;
			gc_present = 1;
			break;
		}
	}

	if (gc_present)
	{
		// Clean up the list
		ch2 = 0;
		for (ch=0;ch<(ship_gcsend_count*3);ch+=3)
		{
			if ((ship_gcsend_list[ch] != 0) && (ch != ch2))
			{
				ship_gcsend_list[ch2]   = ship_gcsend_list[ch];
				ship_gcsend_list[ch2+1] = ship_gcsend_list[ch+1];
				ship_gcsend_list[ch2+2] = ship_gcsend_list[ch+2];
				ch2 += 3;
			}
		}
		ship_gcsend_count = ch2 / 3;
	}

	return gc_present;
}

int ban ( unsigned gc_num, unsigned* ipaddr, long long* hwinfo, unsigned type, BANANA* client )
{
	int banned = 1;
	unsigned ch, ch2;
	FILE* fp;

	for (ch=0;ch<num_bans;ch++)
	{
		if ( ( ship_bandata[ch].guildcard == gc_num ) && ( ship_bandata[ch].type == type ) )
		{
			banned = 0;
			ship_bandata[ch].guildcard = 0;
			ship_bandata[ch].type = 0;
			break;
		}
	}

	if (banned)
	{
		if (num_bans < 5000)
		{
			ship_bandata[num_bans].guildcard = gc_num;
			ship_bandata[num_bans].type = type;
			ship_bandata[num_bans].ipaddr = *ipaddr;
			ship_bandata[num_bans++].hwinfo = *hwinfo;
			fp = fopen ("bandata.dat", "wb");
			if (fp)
			{
				fwrite (&ship_bandata[0], 1, sizeof (BANDATA) * num_bans, fp);
				fclose (fp);
			}
			else
				WriteLog ("Could not open bandata.dat for writing!!");
		}
		else
		{
			banned = 0; // Can't ban with a full list...
			SendB0 ("Ship ban list is full.", client);
		}
	}
	else
	{
		ch2 = 0;
		for (ch=0;ch<num_bans;ch++)
		{
			if ((ship_bandata[ch].type != 0) && (ch != ch2))
				memcpy (&ship_bandata[ch2++], &ship_bandata[ch], sizeof (BANDATA));
		}
		num_bans = ch2;
		fp = fopen ("bandata.dat", "wb");
		if (fp)
		{
			fwrite (&ship_bandata[0], 1, sizeof (BANDATA) * num_bans, fp);
			fclose (fp);
		}
		else
			WriteLog ("Could not open bandata.dat for writing!!");
	}
	return banned;
}

int stfu ( unsigned gc_num )
{
	int result = 0;
	unsigned ch;

	for (ch=0;ch<ship_ignore_count;ch++)
	{
		if (ship_ignore_list[ch] == gc_num)
		{
			result = 1;
			break;
		}
	}

	return result;
}

int toggle_stfu ( unsigned gc_num, BANANA* client )
{
	int ignored = 1;
	unsigned ch, ch2;

	for (ch=0;ch<ship_ignore_count;ch++)
	{
		if (ship_ignore_list[ch] == gc_num)
		{
			ignored = 0;
			ship_ignore_list[ch] = 0;
			break;
		}
	}

	if (ignored)
	{
		if (ship_ignore_count < 300)
			ship_ignore_list[ship_ignore_count++] = gc_num;
		else
		{
			ignored = 0; // Can't ignore with a full list...
			SendB0 ("Ship ignore list is full.", client);
		}
	}
	else
	{
		ch2 = 0;
		for (ch=0;ch<ship_ignore_count;ch++)
		{
			if ((ship_ignore_list[ch] != 0) && (ch != ch2))
				ship_ignore_list[ch2++] = ship_ignore_list[ch];
		}
		ship_ignore_count = ch2;
	}
	return ignored;
}

void Send60 (BANANA* client)
{
	unsigned short size, size_check_index;
	unsigned short sizecheck = 0;
	int dont_send = 0;
	LOBBY* l;
	int boss_floor = 0;
	unsigned itemid, magid, count, drop;
	unsigned short mid;
	short mHP;
	unsigned XP, ch, ch2, max_send, shop_price;
	int mid_mismatch;
	int ignored;
	int ws_ok;
	unsigned short ws_data, counter;
	BANANA* lClient;

	size = *(unsigned short*) &client->decryptbuf[0x00];
	sizecheck = client->decryptbuf[0x09];

	sizecheck *= 4;
	sizecheck += 8;

	if (!client->lobby)
		return;

#ifdef LOG_60
			packet_to_text (&client->decryptbuf[0], size);
			fprintf(debugfile, "%s\n", dp);
#endif

	if (size != sizecheck)
	{
		debug ("Client sent a 0x60 packet whose sizecheck != size.\n");
		debug ("Command: %02X | Size: %04X | Sizecheck(%02x): %04x\n", client->decryptbuf[0x08],
			size, client->decryptbuf[0x09], sizecheck);
		client->decryptbuf[0x09] = ((size / 4) - 2);
	}

	l = (LOBBY*) client->lobby;

	if ( client->lobbyNum < 0x10 )
	{
		size_check_index  = client->decryptbuf[0x08];
		size_check_index *= 2;

		if (client->decryptbuf[0x08] == 0x06)
			sizecheck = 0x114; 
		else
			sizecheck = size_check_table[size_check_index+1] + 4;

		if ( ( size != sizecheck ) && ( sizecheck > 4 ) )
			dont_send = 1;

		if ( sizecheck == 4 ) // No size check packet encountered while in lobby mode...
		{
			debug ("No size check information for 0x60 lobby packet %02x", client->decryptbuf[0x08]);
			dont_send = 1;
		}
	}
	else
	{
		if (dont_send_60[(client->decryptbuf[0x08]*2) + 1] == 1)
		{
			dont_send = 1;
			WriteLog ("60 command \"%02x\" blocked in game. (Data below)", client->decryptbuf[0x08]);
			packet_to_text ( &client->decryptbuf[0x00], size );
			WriteLog ("%s", &dp[0]);
		}
	}

	if ( ( client->decryptbuf[0x0A] != client->clientID )				&&
		 ( size_check_table[(client->decryptbuf[0x08]*2)+1] != 0x00 )	&&
		 ( client->decryptbuf[0x08] != 0x07 )							&&
		 ( client->decryptbuf[0x08] != 0x79 ) )
		dont_send = 1;

	if ( ( client->decryptbuf[0x08] == 0x07 ) && 
		 ( client->decryptbuf[0x0C] != client->clientID ) )
		dont_send = 1;

	if ( client->decryptbuf[0x08] == 0x72 )
		dont_send = 1;

	if (!dont_send)
	{
		switch ( client->decryptbuf[0x08] )
		{
		case 0x07:
			// Symbol chat (throttle for spam)
			dont_send = 1;
			if ( ( ((unsigned) servertime - client->command_cooldown[0x07]) >= 1 ) && ( !stfu ( client->guildcard ) ) )
			{
				client->command_cooldown[0x07] = (unsigned) servertime;
				if ( client->lobbyNum < 0x10 )
					max_send = 12;
				else
					max_send = 4;
				for (ch=0;ch<max_send;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]))
					{
						ignored = 0;
						lClient = l->client[ch];
						for (ch2=0;ch2<lClient->ignore_count;ch2++)
						{
							if (lClient->ignore_list[ch2] == client->guildcard)
							{
								ignored = 1;
								break;
							}
						}
						if ( ( !ignored ) && ( lClient->guildcard != client->guildcard ) )
						{
							cipher_ptr = &lClient->server_cipher;
							encryptcopy ( lClient, &client->decryptbuf[0x00], size );
						}
					}
				}
			}
			break;
		case 0x0A:
			if ( client->lobbyNum > 0x0F )
			{
				// Player hit a monster
				mid = *(unsigned short*) &client->decryptbuf[0x0A];
				mid &= 0xFFF;
				if ( ( mid < 0xB50 ) && ( l->floor[client->clientID] != 0 ) )
				{
					mHP = *(short*) &client->decryptbuf[0x0E];
					l->monsterData[mid].HP = mHP;
				}
			}
			else
				client->todc = 1;
			break;
		case 0x1F:
			// Remember client's position.
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			break;
		case 0x20:
			// Remember client's position.
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned *) &client->decryptbuf[0x18];
			l->clientx[client->clientID] = *(unsigned *) &client->decryptbuf[0x10];
			break;
		case 0x25:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				EquipItem (itemid, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x26:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				DeequipItem (itemid, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x27:
			// Use item
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				UseItem (itemid, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x28:
			// Mag feeding
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				magid = *(unsigned *) &client->decryptbuf[0x0C];
				itemid = *(unsigned *) &client->decryptbuf[0x10];
				FeedMag ( magid, itemid, client );
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x29:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				// Client dropping or destroying an item...
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				count = *(unsigned *) &client->decryptbuf[0x10];
				if (client->drop_item == itemid)
				{
					client->drop_item = 0;
					drop = 1;
				}
				else
					drop = 0;
				if (itemid != 0xFFFFFFFF)
					DeleteItemFromClient (itemid, count, drop, client); // Item
				else
					DeleteMesetaFromClient (count, drop, client); // Meseta
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x2A:
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				// Client dropping complete item
				itemid = *(unsigned*) &client->decryptbuf[0x10];
				DeleteItemFromClient (itemid, 0, 1, client);
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0x3E:
		case 0x3F:
			l->clientx[client->clientID] = *(unsigned *) &client->decryptbuf[0x14];
			l->clienty[client->clientID] = *(unsigned *) &client->decryptbuf[0x1C];
			break;
		case 0x40:
		case 0x42:
			l->clientx[client->clientID] = *(unsigned *) &client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned *) &client->decryptbuf[0x10];
			client->dead = 0;
			break;
		case 0x47:
		case 0x48:
			if (l->floor[client->clientID] == 0)
			{
				Send1A ("Using techniques on Pioneer 2 is disallowed.", client);
				dont_send = 1;
				client->todc = 1;
				break;
			}
			else
				if (client->clientID == client->decryptbuf[0x0A])
				{
					if (client->equip_flags & DROID_FLAG)
					{
						Send1A ("Androids cannot cast techniques.", client);
						dont_send = 1;
						client->todc = 1;
					}
					else
					{
						if (client->decryptbuf[0x0C] > 18)
						{
							Send1A ("Invalid technique cast.", client);
							dont_send = 1;
							client->todc = 1;
						}
						else
						{
							if (max_tech_level[client->decryptbuf[0x0C]][client->character._class] == -1)
							{
								Send1A ("You cannot cast that technique.", client);
								dont_send = 1;
								client->todc = 1;
							}
						}
					}
				}
			break;
		case 0x4D:
			// Decrease mag sync on death
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				client->dead = 1;
				for (ch=0;ch<client->character.inventoryUse;ch++)
				{
					if ((client->character.inventory[ch].item.data[0] == 0x02) &&
						(client->character.inventory[ch].flags & 0x08))
					{
						if (client->character.inventory[ch].item.data2[0] >= 5)
							client->character.inventory[ch].item.data2[0] -= 5;
						else
							client->character.inventory[ch].item.data2[0] = 0;
					}
				}
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}		
			break;
		case 0x68:
			// Telepipe check
			if ( ( client->lobbyNum < 0x10 ) || ( client->decryptbuf[0x0E] > 0x11 ) )
			{
				Send1A ("Incorrect telepipe.", client);
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0x74:
			// W/S (throttle for spam)
			dont_send = 1;
			if ( ( ((unsigned) servertime - client->command_cooldown[0x74]) >= 1 ) && ( !stfu ( client->guildcard ) ) )
			{
				client->command_cooldown[0x74] = (unsigned) servertime;
				ws_ok = 1;
				ws_data = *(unsigned short*) &client->decryptbuf[0x0C];
				if ( ( ws_data == 0 ) || ( ws_data > 3 ) )
					ws_ok = 0;
				ws_data = *(unsigned short*) &client->decryptbuf[0x0E];
				if ( ( ws_data == 0 ) || ( ws_data > 3 ) )
					ws_ok = 0;
				if ( ws_ok )
				{
					for (ch=0;ch<client->decryptbuf[0x0C];ch++)
					{
						ws_data = *(unsigned short*) &client->decryptbuf[0x10+(ch*2)];
						if ( ws_data > 0x685 )
						{
							if ( ws_data > 0x697 )
								ws_ok = 0;
							else
							{
								ws_data -= 0x68C;
								if ( ws_data >= l->lobbyCount )
									ws_ok = 0;
							}
						}
					}
					ws_data = 0xFFFF;
					for (ch=client->decryptbuf[0x0C];ch<8;ch++)
						*(unsigned short*) &client->decryptbuf[0x10+(ch*2)] = ws_data;

					if ( ws_ok ) 
					{
						if ( client->lobbyNum < 0x10 )
							max_send = 12;
						else
							max_send = 4;

						for (ch=0;ch<max_send;ch++)
						{
							if ((l->slot_use[ch]) && (l->client[ch]))
							{
								ignored = 0;
								lClient = l->client[ch];
								for (ch2=0;ch2<lClient->ignore_count;ch2++)
								{
									if (lClient->ignore_list[ch2] == client->guildcard)
									{
										ignored = 1;
										break;
									}
								}
								if ( ( !ignored ) && ( lClient->guildcard != client->guildcard ) )
								{
									cipher_ptr = &lClient->server_cipher;
									encryptcopy ( lClient, &client->decryptbuf[0x00], size );
								}
							}
						}
					}
				}
			}
			break;
		case 0x75:
			{
				// Set player flag

				unsigned short flag;

				if (!client->decryptbuf[0x0E])
				{
					flag = *(unsigned short*) &client->decryptbuf[0x0C];
					if ( flag < 1024 )
						client->character.quest_data1[((unsigned)l->difficulty * 0x80) + (flag >> 3)] |= 1 << (7 - ( flag & 0x07 ));
				}
			}
			break;
		case 0xC0:
			// Client selling item
			if ( ( client->lobbyNum > 0x0F ) && ( l->floor[client->clientID] == 0 ) )
			{
				itemid = *(unsigned *) &client->decryptbuf[0x0C];
				for (ch=0;ch<client->character.inventoryUse;ch++)
				{
					if (client->character.inventory[ch].item.itemid == itemid)
					{
						count = client->decryptbuf[0x10];
						if ((count > 1) && (client->character.inventory[ch].item.data[0] != 0x03))
							client->todc = 1;
						else
						{
							shop_price = GetShopPrice ( &client->character.inventory[ch] ) * count;
							DeleteItemFromClient ( itemid, count, 0, client );
							if (!client->todc)
							{
								client->character.meseta += shop_price;
								if ( client->character.meseta > 999999 )
									client->character.meseta = 999999;
							}
						}
						break;
					}
				}
				if (client->todc)
					dont_send = 1;
			}
			else
			{
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xC3:
			// Client setting coordinates for stack drop
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				client->drop_area = *(unsigned *) &client->decryptbuf[0x0C];
				client->drop_coords = *(long long*) &client->decryptbuf[0x10];
				client->drop_item = *(unsigned *) &client->decryptbuf[0x18];
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xC4:
			// Inventory sort
			if ( client->lobbyNum > 0x0F )
			{
				dont_send = 1;
				SortClientItems ( client );
			}
			else
				client->todc = 1;
			break;
		case 0xC5:
			// Visiting hospital
			if ( client->lobbyNum > 0x0F )
				DeleteMesetaFromClient (10, 0, client);
			else
				client->todc = 1;
			break;
		case 0xC6:
			// Steal Exp
			if ( client->lobbyNum > 0x0F )
			{
				unsigned exp_percent = 0;
				unsigned exp_to_add;
				unsigned char special = 0;

				mid = *(unsigned short*) &client->decryptbuf[0x0C];
				mid &= 0xFFF;
				if (mid < 0xB50)
				{
					for (ch=0;ch<client->character.inventoryUse;ch++)
					{
						if ((client->character.inventory[ch].flags & 0x08)	&&
							(client->character.inventory[ch].item.data[0] == 0x00))
						{
							if ((client->character.inventory[ch].item.data[1] < 0x0A)	&&
								(client->character.inventory[ch].item.data[2] < 0x05))
								special = (client->character.inventory[ch].item.data[4] & 0x1F);
							else
								if ((client->character.inventory[ch].item.data[1] < 0x0D)	&&
									(client->character.inventory[ch].item.data[2] < 0x04))
									special = (client->character.inventory[ch].item.data[4] & 0x1F);
								else
									special = special_table[client->character.inventory[ch].item.data[1]]
								[client->character.inventory[ch].item.data[2]];
								switch (special)
								{
								case 0x09:
									// Master's
									exp_percent = 8;
									break;
								case 0x0A:
									// Lord's
									exp_percent = 10;
									break;
								case 0x0B:
									// King's
									exp_percent = 12;
									if (( l->difficulty	== 0x03 ) &&
										( client->equip_flags & DROID_FLAG ))
										exp_percent += 30;
									break;
								}
								break;
						}
					}

					if (exp_percent)
					{
						exp_to_add = ( l->mapData[mid].exp * exp_percent ) / 100L;
						if ( exp_to_add > 80 )  // Limit the amount of exp stolen to 80
							exp_to_add = 80;
						AddExp ( exp_to_add, client );
					}
				}
			}
			else
				client->todc = 1;
			break;
		case 0xC7:
			// Charge action
			if ( client->lobbyNum > 0x0F )
			{
				int meseta;

				meseta = *(int *) &client->decryptbuf[0x0C];
				if (meseta > 0)
				{
					if (client->character.meseta >= (unsigned) meseta)
						DeleteMesetaFromClient (meseta, 0, client);
					else
						DeleteMesetaFromClient (client->character.meseta, 0, client);
				}
				else
				{
					meseta = -meseta;
					client->character.meseta += (unsigned) meseta;
					if ( client->character.meseta > 999999 )
						client->character.meseta = 999999;
				}
			}
			else
				client->todc = 1;
			break;
		case 0xC8:
			// Monster is dead
			if ( client->lobbyNum > 0x0F )
			{
				mid = *(unsigned short*) &client->decryptbuf[0x0A];
				mid &= 0xFFF;
				if ( mid < 0xB50 )
				{
					if ( l->monsterData[mid].dead[client->clientID] == 0 )
					{
						l->monsterData[mid].dead[client->clientID] = 1;

						XP = l->mapData[mid].exp * EXPERIENCE_RATE;

						if (!l->quest_loaded)
						{
							mid_mismatch = 0;

							switch ( l->episode )
							{
							case 0x01:
								if ( l->floor[client->clientID] > 10 )
								{
									switch ( l->floor[client->clientID] )
									{
									case 11:
										// Dragon
										if ( l->mapData[mid].base != 192 )
											mid_mismatch = 1;
										break;
									case 12:
										// De Rol Le
										if ( l->mapData[mid].base != 193 )
											mid_mismatch = 1;
										break;
									case 13:
										// Vol Opt
										if ( ( l->mapData[mid].base != 197 ) && ( l->mapData[mid].base != 194 ) )
											mid_mismatch = 1;
										break;
									case 14:
										// Dark Falz
										if ( l->mapData[mid].base != 200 )
											mid_mismatch = 1;
										break;
									}
								}
								break;
							case 0x02:
								if ( l->floor[client->clientID] > 10 )
								{
									switch ( l->floor[client->clientID] )
									{
									case 12:
										// Gal Gryphon
										if ( l->mapData[mid].base != 192 )
											mid_mismatch = 1;
										break;
									case 13:
										// Olga Flow
										if ( l->mapData[mid].base != 202 )
											mid_mismatch = 1;
										break;
									case 14:
										// Barba Ray
										if ( l->mapData[mid].base != 203 )
											mid_mismatch = 1;
										break;
									case 15:
										// Gol Dragon
										if ( l->mapData[mid].base != 204 )
											mid_mismatch = 1;
										break;
									}
								}
								break;
							case 0x03:
								if ( ( l->floor[client->clientID] == 9 ) &&
									( l->mapData[mid].base != 280 ) && 
									( l->mapData[mid].base != 281 ) && 
									( l->mapData[mid].base != 41 ) )
									mid_mismatch = 1;
								break;
							}

							if ( mid_mismatch )
							{
								SendEE ("Client/server data synchronization error.  Please reinstall your client and all patches.", client);
								client->todc = 1;
							}
						}

						//debug ("mid death: %u  base: %u, skin: %u, reserved11: %f, exp: %u", mid, l->mapData[mid].base, l->mapData[mid].skin, l->mapData[mid].reserved11, XP);

						if ( client->decryptbuf[0x10] != 1 ) // Not the last player who hit?
							XP = ( XP * 77 ) / 100L;

						if ( client->character.level < 199 )
							AddExp ( XP, client );

						// Increase kill counters for SJS, Lame d'Argent, Limiter and Swordsman Lore

						for (ch=0;ch<client->character.inventoryUse;ch++)
						{
							if (client->character.inventory[ch].flags & 0x08)
							{
								counter = 0;
								switch (client->character.inventory[ch].item.data[0])
								{
								case 0x00:
									if ((client->character.inventory[ch].item.data[1] == 0x33) ||
										(client->character.inventory[ch].item.data[1] == 0xAB))
										counter = 1;
									break;
								case 0x01:
									if ((client->character.inventory[ch].item.data[1] == 0x03) &&
										((client->character.inventory[ch].item.data[2] == 0x4D) ||
										(client->character.inventory[ch].item.data[2] == 0x4F)))
										counter = 1;
									break;
								default:
									break;
								}
								if (counter)
								{
									counter = *(unsigned short*) &client->character.inventory[ch].item.data[10];
									if (counter < 0x8000)
										counter = 0x8000;
									counter++;
									*(unsigned short*) &client->character.inventory[ch].item.data[10] = counter;
								}
							}
						}
					}
				}
			}
			else
				client->todc = 1;
			break;
		case 0xCC:
			// Exchange item for team points
			{
				unsigned deleteid;

				deleteid = *(unsigned*) &client->decryptbuf[0x0C];
				DeleteItemFromClient (deleteid, 1, 0, client);
				if (!client->todc)
				{
					SendB0 ("Item donated to server.", client);
				}
			}
			break;
		case 0xCF:
			if ((l->battle) && (client->mode))
			{
				// Battle restarting...
				//
				// If rule #1 we'll copy the character backup to the character array, otherwise
				// we'll reset the character...
				//
				for (ch=0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]))
					{
						lClient = l->client[ch];
						switch (lClient->mode)
						{
						case 0x01:
						case 0x02:
							// Copy character backup
							if (lClient->character_backup)
								memcpy (&lClient->character, lClient->character_backup, sizeof (lClient->character));
							if (lClient->mode == 0x02)
							{
								for (ch2=0;ch2<lClient->character.inventoryUse;ch2++)
								{
									if (lClient->character.inventory[ch2].item.data[0] == 0x02)
										lClient->character.inventory[ch2].in_use = 0;
								}
								CleanUpInventory (lClient);
								lClient->character.meseta = 0;
							}
							break;
						case 0x03:
							// Wipe items and reset level.
							for (ch2=0;ch2<30;ch2++)
								lClient->character.inventory[ch2].in_use = 0;
							CleanUpInventory (lClient);
							lClient->character.level = 0;
							lClient->character.XP = 0;
							lClient->character.ATP = *(unsigned short*) &startingData[(lClient->character._class*14)];
							lClient->character.MST = *(unsigned short*) &startingData[(lClient->character._class*14)+2];
							lClient->character.EVP = *(unsigned short*) &startingData[(lClient->character._class*14)+4];
							lClient->character.HP  = *(unsigned short*) &startingData[(lClient->character._class*14)+6];
							lClient->character.DFP = *(unsigned short*) &startingData[(lClient->character._class*14)+8];
							lClient->character.ATA = *(unsigned short*) &startingData[(lClient->character._class*14)+10];
							if (l->battle_level > 1)
								SkipToLevel (l->battle_level - 1, lClient, 1);
							lClient->character.meseta = 0;
							break;
						default:
							// Unknown mode?
							break;
						}
					}
				}
				// Reset boxes and monsters...
				memset (&l->boxHit, 0, 0xB50); // Reset box and monster data
				memset (&l->monsterData, 0, sizeof (l->monsterData));
			}
			break;
		case 0xD2:
			// Gallon seems to write to this area...
			dont_send = 1;
			if ( client->lobbyNum > 0x0F )
			{
				unsigned qofs;

				qofs = *(unsigned *) &client->decryptbuf[0x0C];
				if (qofs < 23)
				{
					qofs *= 4;
					*(unsigned *) &client->character.quest_data2[qofs] = *(unsigned*) &client->decryptbuf[0x10];
					memcpy ( &client->encryptbuf[0x00], &client->decryptbuf[0x00], 0x14 );
					cipher_ptr = &client->server_cipher;
					encryptcopy ( client, &client->decryptbuf[0x00], 0x14 );
				}
			}
			else
			{
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0xD5:
			// Exchange an item
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				INVENTORY_ITEM work_item;
				unsigned compare_item = 0, ci;

				memset (&work_item, 0, sizeof (INVENTORY_ITEM));
				memcpy (&work_item.item.data[0], &client->decryptbuf[0x0C], 3 );
				DeleteFromInventory (&work_item, 1, client);

				if (!client->todc)
				{
					memset (&work_item, 0, sizeof (INVENTORY_ITEM));
					memcpy (&compare_item, &client->decryptbuf[0x20], 3 );
					for ( ci = 0; ci < quest_numallows; ci ++)
					{
						if (compare_item == quest_allow[ci])
						{
							memcpy (&work_item.item.data[0], &client->decryptbuf[0x20], 3 );
							work_item.item.itemid = l->playerItemID[client->clientID];
							l->playerItemID[client->clientID]++;
							AddToInventory (&work_item, 1, 0, client);
							memset (&client->encryptbuf[0x00], 0, 0x0C);
							client->encryptbuf[0x00] = 0x0C;
							client->encryptbuf[0x02] = 0xAB;
							client->encryptbuf[0x03] = 0x01;
							// BLAH :)
							*(unsigned short*) &client->encryptbuf[0x08] = *(unsigned short*) &client->decryptbuf[0x34];
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
							break;
						}
					}
					if ( !work_item.item.itemid )
					{
						Send1A ("Attempting to exchange for disallowed item.", client);
						client->todc = 1;
					}
				}
			}
			else
			{
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0xD7:
			// Trade PDs for an item from Hopkins' dad
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				INVENTORY_ITEM work_item;
				unsigned ci, compare_item = 0;

				memset ( &work_item, 0, sizeof (INVENTORY_ITEM) );
				memcpy ( &compare_item, &client->decryptbuf[0x0C], 3 );
				for ( ci = 0; ci < (sizeof (gallons_shop_hopkins) / 4); ci += 2)
				{
					if (compare_item == gallons_shop_hopkins[ci])
					{
						work_item.item.data[0] = 0x03;
						work_item.item.data[1] = 0x10;
						work_item.item.data[2] = 0x00;
						break;
					}
				}
				if ( work_item.item.data[0] == 0x03 )
				{
					DeleteFromInventory (&work_item, 0xFF, client); // Delete all Photon Drops
					if (!client->todc)
					{
						memcpy (&work_item.item.data[0], &client->decryptbuf[0x0C], 12);
						*(unsigned *) &work_item.item.data2[0] = *(unsigned *) &client->decryptbuf[0x18];
						work_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory (&work_item, 1, 0, client);
						memset (&client->encryptbuf[0x00], 0, 0x0C);
						// I guess this is a sort of action confirmed by server thing...
						// Also starts an animation and sound... with the wrong values, the camera pans weirdly...
						client->encryptbuf[0x00] = 0x0C;
						client->encryptbuf[0x02] = 0xAB;
						client->encryptbuf[0x03] = 0x01;
						*(unsigned short*) &client->encryptbuf[0x08] = *(unsigned short*) &client->decryptbuf[0x20];
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
					}
				}
				else
				{
					Send1A ("No photon drops in user's inventory\nwhen encountering exchange command.", client);
					dont_send = 1;
					client->todc = 1;
				}
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xD8:
			// Add attribute to S-rank weapon (not implemented yet)
			break;
		case 0xD9:
			// Momoka Item Exchange
			{
				unsigned compare_item, ci;
				unsigned itemid = 0;
				INVENTORY_ITEM add_item;

				dont_send = 1;

				if ( client->lobbyNum > 0x0F )
				{
					compare_item = 0x00091203;
					for ( ci=0; ci < client->character.inventoryUse; ci++)
					{
						if ( *(unsigned *) &client->character.inventory[ci].item.data[0] == compare_item )
						{
							itemid = client->character.inventory[ci].item.itemid;
							break;
						}
					}
					if (!itemid)
					{
						memset (&client->encryptbuf[0x00], 0, 8);
						client->encryptbuf[0x00] = 0x08;
						client->encryptbuf[0x02] = 0x23;
						client->encryptbuf[0x04] = 0x01;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 8);
					}
					else
					{
						memset (&add_item, 0, sizeof (INVENTORY_ITEM));
						compare_item = *(unsigned *) &client->decryptbuf[0x20];
						for ( ci=0; ci < quest_numallows; ci++)
						{
							if ( compare_item == quest_allow[ci] )
							{
								*(unsigned *) &add_item.item.data[0] = *(unsigned *) &client->decryptbuf[0x20];
								break;
							}
						}
						if (*(unsigned *) &add_item.item.data[0] == 0)
						{
							client->todc = 1;
							Send1A ("Requested item not allowed.", client);
						}
						else
						{
							DeleteItemFromClient (itemid, 1, 0, client);
							memset (&client->encryptbuf[0x00], 0, 0x18);
							client->encryptbuf[0x00] = 0x18;
							client->encryptbuf[0x02] = 0x60;
							client->encryptbuf[0x08] = 0xDB;
							client->encryptbuf[0x09] = 0x06;
							client->encryptbuf[0x0C] = 0x01;
							*(unsigned *) &client->encryptbuf[0x10] = itemid;
							client->encryptbuf[0x14] = 0x01;
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 0x18);

							// Let everybody else know that item no longer exists...

							memset (&client->encryptbuf[0x00], 0, 0x14);
							client->encryptbuf[0x00] = 0x14;
							client->encryptbuf[0x02] = 0x60;
							client->encryptbuf[0x08] = 0x29;
							client->encryptbuf[0x09] = 0x05;
							client->encryptbuf[0x0A] = client->clientID;
							*(unsigned *) &client->encryptbuf[0x0C] = itemid;
							client->encryptbuf[0x10] = 0x01;
							SendToLobby ( l, 4, &client->encryptbuf[0x00], 0x14, client->guildcard );
							add_item.item.itemid = l->playerItemID[client->clientID];
							l->playerItemID[client->clientID]++;
							AddToInventory (&add_item, 1, 0, client);
							memset (&client->encryptbuf[0x00], 0, 8);
							client->encryptbuf[0x00] = 0x08;
							client->encryptbuf[0x02] = 0x23;
							client->encryptbuf[0x04] = 0x00;
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 8);
						}
					}
				}
			}
			break;
		case 0xDA:
			// Upgrade Photon of weapon
			if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0A] == client->clientID ) )
			{
				INVENTORY_ITEM work_item, work_item2;
				unsigned ci, ai,
					compare_itemid = 0, compare_item1 = 0, compare_item2 = 0, num_attribs = 0;
				char attrib_add;

				memcpy ( &compare_item1,  &client->decryptbuf[0x0C], 3);
				compare_itemid = *(unsigned *) &client->decryptbuf[0x20];
				for ( ci=0; ci < client->character.inventoryUse; ci++)
				{
					memcpy (&compare_item2, &client->character.inventory[ci].item.data[0], 3);
					if ( ( client->character.inventory[ci].item.itemid == compare_itemid ) && 
						( compare_item1 == compare_item2 ) && ( client->character.inventory[ci].item.data[0] == 0x00 ) )
					{
						memset (&work_item, 0, sizeof (INVENTORY_ITEM));
						work_item.item.data[0] = 0x03;
						work_item.item.data[1] = 0x10;
						if ( client->decryptbuf[0x2C] )
							work_item.item.data[2] = 0x01;
						else
							work_item.item.data[2] = 0x00;
						// Copy before shift
						memcpy ( &work_item2, &client->character.inventory[ci], sizeof (INVENTORY_ITEM) );
						DeleteFromInventory ( &work_item, client->decryptbuf[0x28], client );
						if (!client->todc)
						{
							switch (client->decryptbuf[0x28])
							{
							case 0x01:
								// 1 PS = 30%
								if ( client->decryptbuf[0x2C] )
									attrib_add = 30;
								break;
							case 0x04:
								// 4 PDs = 1%
								attrib_add = 1;
								break;
							case 0x14:
								// 20 PDs = 5%
								attrib_add = 5;
								break;
							default:
								attrib_add = 0;
								break;
							}
							ai = 0;
							if ((work_item2.item.data[6] > 0x00) && 
								(!(work_item2.item.data[6] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[6] == client->decryptbuf[0x24])
									ai = 7;
							}
							if ((work_item2.item.data[8] > 0x00) && 
								(!(work_item2.item.data[8] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[8] == client->decryptbuf[0x24])
									ai = 9;
							}
							if ((work_item2.item.data[10] > 0x00) && 
								(!(work_item2.item.data[10] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[10] == client->decryptbuf[0x24])
									ai = 11;
							}
							if (ai)
							{
								// Attribute already on weapon, increase it
								(char) work_item2.item.data[ai] += attrib_add;
								if (work_item2.item.data[ai] > 100)
									work_item2.item.data[ai] = 100;
							}
							else
							{
								// Attribute not on weapon, add it if there isn't already 3 attributes
								if (num_attribs < 3)
								{
									work_item2.item.data[6 + (num_attribs * 2)] = client->decryptbuf[0x24];
									(char) work_item2.item.data[7 + (num_attribs * 2)] = attrib_add;
								}
							}
							DeleteItemFromClient ( work_item2.item.itemid, 1, 0, client );
							memset (&client->encryptbuf[0x00], 0, 0x14);
							client->encryptbuf[0x00] = 0x14;
							client->encryptbuf[0x02] = 0x60;
							client->encryptbuf[0x08] = 0x29;
							client->encryptbuf[0x09] = 0x05;
							client->encryptbuf[0x0A] = client->clientID;
							*(unsigned *) &client->encryptbuf[0x0C] = work_item2.item.itemid;
							client->encryptbuf[0x10] = 0x01;
							SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x14, 0 );
							AddToInventory ( &work_item2, 1, 0, client );
							memset (&client->encryptbuf[0x00], 0, 0x0C);
							// Don't know...
							client->encryptbuf[0x00] = 0x0C;
							client->encryptbuf[0x02] = 0xAB;
							client->encryptbuf[0x03] = 0x01;
							*(unsigned short*) &client->encryptbuf[0x08] = *(unsigned short*) &client->decryptbuf[0x30];
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
						}
						break;
					}
				}
				if (client->todc)
					dont_send = 1;
			}
			else
			{
				dont_send = 1;
				if ( client->lobbyNum < 0x10 )
					client->todc = 1;
			}
			break;
		case 0xDE:
			// Good Luck
			{
				unsigned compare_item, ci;
				unsigned itemid = 0;
				INVENTORY_ITEM add_item;

				dont_send = 1;

				if ( client->lobbyNum > 0x0F )
				{
					compare_item = 0x00031003;
					for ( ci=0; ci < client->character.inventoryUse; ci++)
					{
						if ( *(unsigned *) &client->character.inventory[ci].item.data[0] == compare_item )
						{
							itemid = client->character.inventory[ci].item.itemid;
							break;
						}
					}
					if (!itemid)
					{
						memset (&client->encryptbuf[0x00], 0, 0x2C);
						client->encryptbuf[0x00] = 0x2C;
						client->encryptbuf[0x02] = 0x24;
						client->encryptbuf[0x04] = 0x01;
						client->encryptbuf[0x08] = client->decryptbuf[0x0E];
						client->encryptbuf[0x0A] = client->decryptbuf[0x0D];
						for (ci=0;ci<8;ci++)
							client->encryptbuf[0x0C + (ci << 2)] = (mt_lrand() % (sizeof(good_luck) >> 2)) + 1;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x2C);
					}
					else
					{
						memset (&add_item, 0, sizeof (INVENTORY_ITEM));
						*(unsigned *) &add_item.item.data[0] = good_luck[mt_lrand() % (sizeof(good_luck) >> 2)];
						DeleteItemFromClient (itemid, 1, 0, client);
						memset (&client->encryptbuf[0x00], 0, 0x18);
						client->encryptbuf[0x00] = 0x18;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0xDB;
						client->encryptbuf[0x09] = 0x06;
						client->encryptbuf[0x0C] = 0x01;
						*(unsigned *) &client->encryptbuf[0x10] = itemid;
						client->encryptbuf[0x14] = 0x01;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x18);

						// Let everybody else know that item no longer exists...

						memset (&client->encryptbuf[0x00], 0, 0x14);
						client->encryptbuf[0x00] = 0x14;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0x29;
						client->encryptbuf[0x09] = 0x05;
						client->encryptbuf[0x0A] = client->clientID;
						*(unsigned *) &client->encryptbuf[0x0C] = itemid;
						client->encryptbuf[0x10] = 0x01;
						SendToLobby ( l, 4, &client->encryptbuf[0x00], 0x14, client->guildcard );
						add_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory (&add_item, 1, 0, client);
						memset (&client->encryptbuf[0x00], 0, 0x2C);
						client->encryptbuf[0x00] = 0x2C;
						client->encryptbuf[0x02] = 0x24;
						client->encryptbuf[0x04] = 0x00;
						client->encryptbuf[0x08] = client->decryptbuf[0x0E];
						client->encryptbuf[0x0A] = client->decryptbuf[0x0D];
						for (ci=0;ci<8;ci++)
							client->encryptbuf[0x0C + (ci << 2)] = (mt_lrand() % (sizeof(good_luck) >> 2)) + 1;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x2C);
					}
				}
			}
			break;
		case 0xE1:
			{
				// Gallon's Plan opcode

				INVENTORY_ITEM work_item;
				unsigned ch, compare_item1, compare_item2, pt_itemid;

				compare_item2 = 0;
				compare_item1 = 0x041003;
				pt_itemid = 0;

				for (ch=0;ch<client->character.inventoryUse;ch++)
				{
					memcpy (&compare_item2, &client->character.inventory[ch].item.data[0], 3);
					if (compare_item1 == compare_item2)
					{
						pt_itemid = client->character.inventory[ch].item.itemid;
						break;
					}
				}

				if (!pt_itemid)
					client->todc = 1;

				if (!client->todc)
				{
					memset ( &work_item, 0, sizeof (INVENTORY_ITEM) );
					switch (client->decryptbuf[0x0E])
					{
					case 0x01:
						// Kan'ei Tsuho
						DeleteItemFromClient (pt_itemid, 10, 0, client); // Delete Photon Tickets
						if (!client->todc)
						{
							work_item.item.data[0] = 0x00;
							work_item.item.data[1] = 0xD5;
							work_item.item.data[2] = 0x00;
						}
						break;
					case 0x02:
						// Lollipop
						DeleteItemFromClient (pt_itemid, 15, 0, client); // Delete Photon Tickets
						if (!client->todc)
						{
							work_item.item.data[0] = 0x00;
							work_item.item.data[1] = 0x0A;
							work_item.item.data[2] = 0x07;
						}
						break;
					case 0x03:
						// Stealth Suit
						DeleteItemFromClient (pt_itemid, 20, 0, client); // Delete Photon Tickets
						if (!client->todc)
						{
							work_item.item.data[0] = 0x01;
							work_item.item.data[1] = 0x01;
							work_item.item.data[2] = 0x57;
						}
						break;
					default:
						client->todc = 1;
						break;
					}

					if (!client->todc)
					{
						memset (&client->encryptbuf[0x00], 0, 0x18);
						client->encryptbuf[0x00] = 0x18;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0xDB;
						client->encryptbuf[0x09] = 0x06;
						client->encryptbuf[0x0C] = 0x01;
						*(unsigned *) &client->encryptbuf[0x10] = pt_itemid;
						client->encryptbuf[0x14] = 0x05 + ( client->decryptbuf[0x0E] * 5 );
						cipher_ptr = &client->server_cipher;
						encryptcopy ( client, &client->encryptbuf[0x00], 0x18 );
						work_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory (&work_item, 1, 0, client);
						// Gallon's Plan result
						memset (&client->encryptbuf[0x00], 0, 0x10);
						client->encryptbuf[0x00] = 0x10;
						client->encryptbuf[0x02] = 0x25;
						client->encryptbuf[0x08] = client->decryptbuf[0x10];
						client->encryptbuf[0x0A] = 0x3C;
						client->encryptbuf[0x0B] = 0x08;
						client->encryptbuf[0x0D] = client->decryptbuf[0x0E];
						client->encryptbuf[0x0E] = 0x9A;
						client->encryptbuf[0x0F] = 0x08;
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &client->encryptbuf[0x00], 0x10);
					}
				}
			}
			break;
		case 0x17:
		case 0x18:
			boss_floor = 0;
			switch (l->episode)
			{
			case 0x01:
				if ((l->floor[client->clientID] > 10) && (l->floor[client->clientID] < 15))
					boss_floor = 1;
				break;
			case 0x02:
				if ((l->floor[client->clientID] > 11) && (l->floor[client->clientID] < 16))
					boss_floor = 1;
				break;
			case 0x03:
				if (l->floor[client->clientID] == 9)
					boss_floor = 1;
				break;
			}
			if (!boss_floor)
				dont_send = 1;
			break;
		default:
			/* Temporary
			debug ("0x60 from %u:", client->guildcard);
			display_packet (&client->decryptbuf[0x00], size);
			WriteLog ("0x60 from %u\n%s\n\n:", client->guildcard, (char*) &dp[0]);
			*/
			break;
		}
		if ((!dont_send) && (!client->todc))
		{
			if ( client->lobbyNum < 0x10 )				
				SendToLobby ( client->lobby, 12, &client->decryptbuf[0], size, client->guildcard);
			else
				SendToLobby ( client->lobby, 4, &client->decryptbuf[0], size, client->guildcard);
		}
	}
}

unsigned long ExpandDropRate(unsigned char pc)
{
    long shift = ((pc >> 3) & 0x1F) - 4;
    if (shift < 0) shift = 0;
    return ((2 << (unsigned long) shift) * ((pc & 7) + 7));
}

void GenerateRandomAttributes (unsigned char sid, GAME_ITEM* i, LOBBY* l, BANANA* client)
{
	unsigned ch, num_percents, max_percent, meseta, do_area, r;
	PTDATA* ptd;
	int rare;
	unsigned area;
	unsigned did_area[6] = {0};
	char percent;

	if ((!l) || (!i))
		return;

	if (l->episode == 0x02)
		ptd = &pt_tables_ep2[sid][l->difficulty];
	else
		ptd = &pt_tables_ep1[sid][l->difficulty];

	area = 0;

	switch ( l->episode )
	{
	case 0x01:
		switch ( l->floor[client->clientID ] )
		{
		case 11:
			// dragon
			area = 3;
			break;
		case 12:
			// de rol
			area = 6;
			break;
		case 13:
			// vol opt
			area = 8;
			break;
		case 14:
			// falz
			area = 10;
			break;
		default:
			area = l->floor[client->clientID ];
			break;
		}	
		break;
	case 0x02:
		switch ( l->floor[client->clientID ] )
		{
		case 14:
			// barba ray
			area = 3;
			break;
		case 15:
			// gol dragon
			area = 6;
			break;
		case 12:
			// gal gryphon
			area = 9;
			break;
		case 13:
			// olga flow
			area = 10;
			break;
		default:
			// could be tower
			if ( l->floor[client->clientID] <= 11 )
				area = ep2_rtremap[(l->floor[client->clientID] * 2)+1];
			else
				area = 0x0A;
			break;
		}	
		break;
	case 0x03:
		area = l->floor[client->clientID ] + 1;
		break;
	}

	if ( !area )
		return;

	if ( area > 10 )
		area = 10;

	area--; // Our tables are zero based.

	switch (i->item.data[0])
	{
	case 0x00:
		rare = 0;
		if ( i->item.data[1] > 0x0C )
			rare = 1;
		else
			if ( ( i->item.data[1] > 0x09 ) && ( i->item.data[2] > 0x03 ) )
				rare = 1;
			else
				if ( ( i->item.data[1] < 0x0A ) && ( i->item.data[2] > 0x04 ) )
					rare = 1;
		if ( !rare )
		{

			r = 100 - ( mt_lrand() % 100 );

			if ( ( r > ptd->element_probability[area] ) && ( ptd->element_ranking[area] ) )
			{
				i->item.data[4] = 0xFF;
				while (i->item.data[4] == 0xFF) // give a special
					i->item.data[4] = elemental_table[(12 * ( ptd->element_ranking[area] - 1 ) ) + ( mt_lrand() % 12 )];
			}
			else
				i->item.data[4] = 0;

			if ( i->item.data[4] )
				i->item.data[4] |= 0x80; // untekked

			// Add a grind

			if ( l->episode == 0x02 )
				ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
			else
				ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

			i->item.data[3] = (unsigned char) ch;
		}
		else
			i->item.data[4] |= 0x80; // rare

		num_percents = 0;

		if ( ( i->item.data[1] == 0x33 ) || 
			 ( i->item.data[1] == 0xAB ) ) // SJS and Lame max 2 percents
			max_percent = 2;
		else
			max_percent = 3;

		for (ch=0;ch<max_percent;ch++)
		{
			if (l->episode == 0x02)
				do_area = attachment_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				do_area = attachment_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			if ( ( do_area ) && ( !did_area[do_area] ) )
			{
				did_area[do_area] = 1;
				i->item.data[6+(num_percents*2)] = (unsigned char) do_area;
				if ( l->episode == 0x02 )
					percent = percent_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				else
					percent = percent_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				percent -= 2;
				percent *= 5;
				(char) i->item.data[6+(num_percents*2)+1] = percent;
				num_percents++;
			}
		}
		break;
	case 0x01:
		switch ( i->item.data[1] )
		{
		case 0x01:
			// Armor variance
			r = mt_lrand() % 11;
			if (r < 7)
			{
				if (armor_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char) (mt_lrand() % (armor_dfpvar_table[i->item.data[2]] + 1));
				if (armor_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char) (mt_lrand() % (armor_evpvar_table[i->item.data[2]] + 1));
			}

			// Slots
			if ( l->episode == 0x02 )
				i->item.data[5] = slots_ep2[sid][l->difficulty][mt_lrand() % 4096]; 
			else
				i->item.data[5] = slots_ep1[sid][l->difficulty][mt_lrand() % 4096];
			break;
		case 0x02:
			// Shield variance
			r = mt_lrand() % 11;
			if (r < 2)
			{
				if (barrier_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char) (mt_lrand() % (barrier_dfpvar_table[i->item.data[2]] + 1));
				if (barrier_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char) (mt_lrand() % (barrier_evpvar_table[i->item.data[2]] + 1));
			}
			break;
		}
		break;
	case 0x02:
		// Mag
		i->item.data [2]  = 0x05;
		i->item.data [4]  = 0xF4;
		i->item.data [5]  = 0x01;
		i->item.data2[3] = mt_lrand() % 0x11;
		break;
	case 0x03:
		if ( i->item.data[1] == 0x02 ) // Technique
		{
			if ( l->episode == 0x02 )
				i->item.data[4] = tech_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				i->item.data[4] = tech_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			i->item.data[2] = (unsigned char) ptd->tech_levels[i->item.data[4]][area*2];
			if ( ptd->tech_levels[i->item.data[4]][(area*2)+1] > ptd->tech_levels[i->item.data[4]][area*2] )
				i->item.data[2] += (unsigned char) mt_lrand() % ( ( ptd->tech_levels[i->item.data[4]][(area*2)+1] - ptd->tech_levels[i->item.data[4]][(area*2)] ) + 1 );
		}
		if (stackable_table[i->item.data[1]])
			i->item.data[5] = 0x01;
		break;
	case 0x04:
		// meseta
		meseta = ptd->box_meseta[area][0];
		if ( ptd->box_meseta[area][1] > ptd->box_meseta[area][0] )
			meseta += mt_lrand() % ( ( ptd->box_meseta[area][1] - ptd->box_meseta[area][0] ) + 1 );
		*(unsigned *) &i->item.data2[0] = meseta;
		break;
	default:
		break;
	}
}

void GenerateCommonItem (int item_type, int is_enemy, unsigned char sid, GAME_ITEM* i, LOBBY* l, BANANA* client)
{
	unsigned ch, num_percents, item_set, meseta, do_area, r, eq_type;
	unsigned short ch2;
	PTDATA* ptd;
	unsigned area,fl;
	unsigned did_area[6] = {0};
	char percent;

	if ((!l) || (!i))
		return;

	if (l->episode == 0x02)
		ptd = &pt_tables_ep2[sid][l->difficulty];
	else
		ptd = &pt_tables_ep1[sid][l->difficulty];

	area = 0;

	switch ( l->episode )
	{
	case 0x01:
		switch ( l->floor[client->clientID ] )
		{
		case 11:
			// dragon
			area = 3;
			break;
		case 12:
			// de rol
			area = 6;
			break;
		case 13:
			// vol opt
			area = 8;
			break;
		case 14:
			// falz
			area = 10;
			break;
		default:
			area = l->floor[client->clientID ];
			break;
		}	
		break;
	case 0x02:
		switch ( l->floor[client->clientID ] )
		{
		case 14:
			// barba ray
			area = 3;
			break;
		case 15:
			// gol dragon
			area = 6;
			break;
		case 12:
			// gal gryphon
			area = 9;
			break;
		case 13:
			// olga flow
			area = 10;
			break;
		default:
			// could be tower
			if ( l->floor[client->clientID] <= 11 )
				area = ep2_rtremap[(l->floor[client->clientID] * 2)+1];
			else
				area = 0x0A;
			break;
		}	
		break;
	case 0x03:
		area = l->floor[client->clientID ] + 1;
		break;
	}

	if ( !area )
		return;

	if ( ( l->battle ) && ( l->quest_loaded ) )
	{
		if ( ( !l->battle_level ) || ( l->battle_level > 5 ) )
			area = 6; // Use mines equipment for rule #1, #5 and #8
		else
			area = 3; // Use caves 1 equipment for all other rules...
	}

	if ( area > 10 )
		area = 10;

	fl = area;
	area--; // Our tables are zero based.

	if (is_enemy)
	{
		if ( ( mt_lrand() % 100 ) > 40)
			item_set = 3;
		else
		{
			switch (item_type)
			{
			case 0x00:
				item_set = 0;
				break;
			case 0x01:
				item_set = 1;
				break;
			case 0x02:
				item_set = 1;
				break;
			case 0x03:
				item_set = 1;
				break;
			default:
				item_set = 3;
				break;
			}
		}
	}
	else
	{
		if ( ( l->meseta_boost ) && ( ( mt_lrand() % 100 ) > 25 ) )
			item_set = 4; // Boost amount of meseta dropped during rules #4 and #5
		else
		{
			if ( item_type == 0xFF )
				item_set = common_table[mt_lrand() % 100000];
			else
				item_set = item_type;
		}
	}

	memset (&i->item.data[0], 0, 12);
	memset (&i->item.data2[0], 0, 4);

	switch (item_set)
	{
	case 0x00:
		// Weapon

		if ( l->episode == 0x02 )
			ch2 = weapon_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
		else
			ch2 = weapon_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

		i->item.data[1] = ch2 & 0xFF;
		i->item.data[2] = ch2 >> 8;

		if (i->item.data[1] > 0x09)
		{
			if (i->item.data[2] > 0x03)
				i->item.data[2] = 0x03;
		}
		else
		{
			if (i->item.data[2] > 0x04)
				i->item.data[2] = 0x04;
		}
		
		r = 100 - ( mt_lrand() % 100 );

		if ( ( r > ptd->element_probability[area] ) && ( ptd->element_ranking[area] ) )
		{
			i->item.data[4] = 0xFF;
			while (i->item.data[4] == 0xFF) // give a special
				i->item.data[4] = elemental_table[(12 * ( ptd->element_ranking[area] - 1 ) ) + ( mt_lrand() % 12 )];
		}
		else
			i->item.data[4] = 0;

		if ( i->item.data[4] )
			i->item.data[4] |= 0x80; // untekked

		num_percents = 0;

		for (ch=0;ch<3;ch++)
		{
			if (l->episode == 0x02)
				do_area = attachment_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				do_area = attachment_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			if ( ( do_area ) && ( !did_area[do_area] ) )
			{
				did_area[do_area] = 1;
				i->item.data[6+(num_percents*2)] = (unsigned char) do_area;
				if ( l->episode == 0x02 )
					percent = percent_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				else
					percent = percent_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				percent -= 2;
				percent *= 5;
				(char) i->item.data[6+(num_percents*2)+1] = percent;
				num_percents++;
			}
		}

		// Add a grind

		if ( l->episode == 0x02 )
			ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
		else
			ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

		i->item.data[3] = (unsigned char) ch;

		break;		
	case 0x01:
		r = mt_lrand() % 100;
		if (!is_enemy)
		{
			// Probabilities (Box): Armor 41%, Shields 41%, Units 18%
			if ( r > 82 )
				eq_type = 3;
			else
				if ( r > 59 )
					eq_type = 2;
				else
					eq_type = 1;
		}
		else
			eq_type = (unsigned) item_type;

		switch ( eq_type )
		{
		case 0x01:
			// Armor
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x01;
			i->item.data[2] = (unsigned char) ( fl / 3L ) + ( 5 * l->difficulty ) + ( mt_lrand() % ( ( (unsigned char) fl / 2L ) + 2 ) );
			if ( i->item.data[2] > 0x17 )
				i->item.data[2] = 0x17;
			r = mt_lrand() % 11;
			if (r < 7)
			{
				if (armor_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char) (mt_lrand() % (armor_dfpvar_table[i->item.data[2]] + 1));
				if (armor_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char) (mt_lrand() % (armor_evpvar_table[i->item.data[2]] + 1));
			}

			// Slots
			if ( l->episode == 0x02 )
				i->item.data[5] = slots_ep2[sid][l->difficulty][mt_lrand() % 4096]; 
			else
				i->item.data[5] = slots_ep1[sid][l->difficulty][mt_lrand() % 4096];

			break;
		case 0x02:
			// Shield
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x02;
			i->item.data[2] = (unsigned char) ( fl / 3L ) + ( 4 * l->difficulty ) + ( mt_lrand() % ( ( (unsigned char) fl / 2L ) + 2 ) );
			if ( i->item.data[2] > 0x14 )
				i->item.data[2] = 0x14;
			r = mt_lrand() % 11;
			if (r < 2)
			{
				if (barrier_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char) (mt_lrand() % (barrier_dfpvar_table[i->item.data[2]] + 1));
				if (barrier_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char) (mt_lrand() % (barrier_evpvar_table[i->item.data[2]] + 1));
			}
			break;
		case 0x03:
			// unit
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x03;
			if ( ( ptd->unit_level[area] >= 2 ) && ( ptd->unit_level[area] <= 8 ) )
			{
				i->item.data[2] = 0xFF;
				while (i->item.data[2] == 0xFF)
					i->item.data[2] = unit_drop [mt_lrand() % ((ptd->unit_level[area] - 1) * 10)];
			}
			else
			{
				i->item.data[0] = 0x03;
				i->item.data[1] = 0x00;
				i->item.data[2] = 0x00; // Give a monomate when failed to look up unit.
			}
			break;
		}
		break;
	case 0x02:
		// Mag
		i->item.data [0]  = 0x02;
		i->item.data [2]  = 0x05;
		i->item.data [4]  = 0xF4;
		i->item.data [5]  = 0x01;
		i->item.data2[3] = mt_lrand() % 0x11;
		break;
	case 0x03:
		// Tool
		if ( l->episode == 0x02 )
			*(unsigned *) &i->item.data[0] = tool_remap[tool_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096]];
		else
			*(unsigned *) &i->item.data[0] = tool_remap[tool_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096]];
		if ( i->item.data[1] == 0x02 ) // Technique
		{
			if ( l->episode == 0x02 )
				i->item.data[4] = tech_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				i->item.data[4] = tech_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			i->item.data[2] = (unsigned char) ptd->tech_levels[i->item.data[4]][area*2];
			if ( ptd->tech_levels[i->item.data[4]][(area*2)+1] > ptd->tech_levels[i->item.data[4]][area*2] )
				i->item.data[2] += (unsigned char) mt_lrand() % ( ( ptd->tech_levels[i->item.data[4]][(area*2)+1] - ptd->tech_levels[i->item.data[4]][(area*2)] ) + 1 );
		}
		if (stackable_table[i->item.data[1]])
			i->item.data[5] = 0x01;
		break;
	case 0x04:
		// Meseta
		i->item.data[0] = 0x04;
		meseta  = ptd->box_meseta[area][0];
		if ( ptd->box_meseta[area][1] > ptd->box_meseta[area][0] )
			meseta += mt_lrand() % ( ( ptd->box_meseta[area][1] - ptd->box_meseta[area][0] ) + 1 );
		*(unsigned *) &i->item.data2[0] = meseta;
		break;
	default:
		break;
	}
	i->item.itemid = l->itemID++;
}

void Send62 (BANANA* client)
{
	BANANA* lClient;
	unsigned bank_size, bank_use;
	unsigned short size;
	unsigned short sizecheck = 0;
	unsigned char t,maxt;
	unsigned itemid;
	int dont_send = 1;
	LOBBY* l;
	unsigned rt_index = 0;
	unsigned rare_lookup, rare_rate, rare_item, 
		rare_roll, box_rare, ch, itemNum;
	unsigned short mid, count;
	unsigned char* rt_table;
	unsigned char* rt_table2;
	unsigned meseta;
	unsigned DAR;
	unsigned floor_check = 0;
	SHOP* shopp;
	SHOP_ITEM* shopi;
	PTDATA* ptd;
	MAP_BOX* mb;

	if (!client->lobby)
		return;

	l = (LOBBY*) client->lobby;
	// don't support target @ 0x02
	t = client->decryptbuf[0x04];
	if (client->lobbyNum < 0x10)
		maxt = 12;
	else
		maxt = 4;

	size = *(unsigned short*) &client->decryptbuf[0x00];
	sizecheck = client->decryptbuf[0x09];

	sizecheck *= 4;
	sizecheck += 8;

	if (size != sizecheck)
	{
		debug ("Client sent a 0x62 packet whose sizecheck != size.\n");
		debug ("Command: %02X | Size: %04X | Sizecheck(%02x): %04x\n", client->decryptbuf[0x08],
			size, client->decryptbuf[0x09], sizecheck);
		client->decryptbuf[0x09] = ((size / 4) - 2);
	}

	switch (client->decryptbuf[0x08])
	{
	case 0x06:
		// Send guild card
		if ( ( size == 0x0C ) && ( t < maxt ) )
		{
			if ((l->slot_use[t]) && (l->client[t]))
			{
				lClient = l->client[t];
				PrepGuildCard ( client->guildcard, lClient->guildcard );
				memset (&PacketData[0], 0, 0x114);
				sprintf (&PacketData[0x00], "\x14\x01\x60");
				PacketData[0x03] = 0x00;
				PacketData[0x04] = 0x00;
				PacketData[0x08] = 0x06;
				PacketData[0x09] = 0x43;
				*(unsigned *) &PacketData[0x0C] = client->guildcard;
				memcpy (&PacketData[0x10], &client->character.name[0], 24 );
				memcpy (&PacketData[0x60], &client->character.guildcard_text[0], 176);
				PacketData[0x110] = 0x01; // ?
				PacketData[0x112] = (char)client->character.sectionID;
				PacketData[0x113] = (char)client->character._class;
				cipher_ptr = &lClient->server_cipher;
				encryptcopy (lClient, &PacketData[0], 0x114);
			}
		}
		break;
	case 0x5A:
		if ( client->lobbyNum > 0x0F )
		{			
			itemid = *(unsigned *) &client->decryptbuf[0x0C];
			if ( AddItemToClient ( itemid, client ) == 1 )
			{
				memset (&PacketData[0], 0, 16);
				PacketData[0x00] = 0x14;
				PacketData[0x02] = 0x60;
				PacketData[0x08] = 0x59;
				PacketData[0x09] = 0x03;
				PacketData[0x0A] = (unsigned char) client->clientID;
				PacketData[0x0E] = client->decryptbuf[0x10];
				PacketData[0x0C] = (unsigned char) client->clientID;
				*(unsigned *) &PacketData[0x10] = itemid;
				SendToLobby ( client->lobby, 4, &PacketData[0x00], 0x14, 0);
			}
		}
		else
			client->todc = 1;
		break;
	case 0x60:
		// Requesting a drop from a monster.
		if ( client->lobbyNum > 0x0F ) 
		{
			if ( !l->drops_disabled )
			{
				mid = *(unsigned short*) &client->decryptbuf[0x0E];
				mid &= 0xFFF;				

				if ( ( mid < 0xB50 ) && ( l->monsterData[mid].drop == 0 ) )
				{
					if (l->episode == 0x02)
						ptd = &pt_tables_ep2[client->character.sectionID][l->difficulty];
					else
						ptd = &pt_tables_ep1[client->character.sectionID][l->difficulty];

					if ( ( l->episode == 0x01 ) && ( client->decryptbuf[0x0D] == 35 ) &&
						 ( l->mapData[mid].rt_index == 34 ) )
						rt_index = 35; // Save Death Gunner index...
					else
						rt_index = l->mapData[mid].rt_index; // Use map's index instead of what the client says...

					if ( rt_index < 0x64 )
					{
						if ( l->episode == 0x03 )
						{
							if ( rt_index < 0x16 )
							{
								meseta = ep4_rtremap[(rt_index * 2)+1];
								// Past a certain point is Episode II data...
								if ( meseta > 0x2F )
									ptd = &pt_tables_ep2[client->character.sectionID][l->difficulty];
							}
							else
								meseta = 0;
						}
						else
							meseta = rt_index;
						if ( ( l->episode == 0x03 ) && 
							 ( rt_index >= 19 ) && 
							 ( rt_index <= 21 ) )
							DAR = 1;
						else
						{
							if ( ( ptd->enemy_dar[meseta] == 100 ) || ( l->redbox ) )
								DAR = 1;
							else
							{

								DAR = 100 - ptd->enemy_dar[meseta];
								if ( ( mt_lrand() % 100 ) >= DAR )
									DAR = 1;
								else
									DAR = 0;
							}
						}
					}
					else
						DAR = 0;

					if ( DAR )
					{
						if (rt_index < 0x64)
						{
							rt_index += ((0x1400 * l->difficulty) + ( client->character.sectionID * 0x200 ));
							switch (l->episode)
							{
							case 0x02:
								rare_lookup = rt_tables_ep2[rt_index];
								break;
							case 0x03:
								rare_lookup = rt_tables_ep4[rt_index];
								break;
							default:
								rare_lookup = rt_tables_ep1[rt_index];
								break;
							}
							rare_rate = ExpandDropRate ( rare_lookup & 0xFF );
							rare_item = rare_lookup >> 8;
							rare_roll = mt_lrand();
							//debug ("rare_roll = %u", rare_roll );
							if  ( ( ( rare_lookup & 0xFF ) != 0 ) && ( ( rare_roll < rare_rate ) || ( l->redbox ) ) )
							{
								// Drop a rare item
								itemNum = free_game_item (l);
								memset (&l->gameItem[itemNum].item.data[0], 0, 12);
								memset (&l->gameItem[itemNum].item.data2[0], 0, 4);
								memcpy (&l->gameItem[itemNum].item.data[0], &rare_item, 3);
								GenerateRandomAttributes (client->character.sectionID, &l->gameItem[itemNum], l, client);
								l->gameItem[itemNum].item.itemid = l->itemID++;
							}
							else
							{
								// Drop a common item
								itemNum = free_game_item (l);
								if ( ( ( mt_lrand() % 100 ) < 60 ) || ( ptd->enemy_drop < 0 ) )
								{
									memset (&l->gameItem[itemNum].item.data[0], 0, 12 );
									memset (&l->gameItem[itemNum].item.data2[0], 0, 4 );
									l->gameItem[itemNum].item.data[0] = 0x04;
									rt_index = meseta;
									meseta  = ptd->enemy_meseta[rt_index][0];
									if ( ptd->enemy_meseta[rt_index][1] > ptd->enemy_meseta[rt_index][0] )
										meseta += mt_lrand() % ( ( ptd->enemy_meseta[rt_index][1] - ptd->enemy_meseta[rt_index][0] ) + 1 );
									*(unsigned *) &l->gameItem[itemNum].item.data2[0] = meseta;
									l->gameItem[itemNum].item.itemid = l->itemID++;
								}
								else
								{
									rt_index = meseta;
									GenerateCommonItem (ptd->enemy_drop[rt_index], 1, client->character.sectionID, &l->gameItem[itemNum], l, client);
								}
							}

							if ( l->gameItem[itemNum].item.itemid != 0 )
							{
								if (l->gameItemCount < MAX_SAVED_ITEMS)
									l->gameItemList[l->gameItemCount++] = itemNum;
								memset (&PacketData[0x00], 0, 16);
								PacketData[0x00] = 0x30;
								PacketData[0x01] = 0x00;
								PacketData[0x02] = 0x60;
								PacketData[0x03] = 0x00;
								PacketData[0x08] = 0x5F;
								PacketData[0x09] = 0x0D;
								*(unsigned *) &PacketData[0x0C] = *(unsigned *) &client->decryptbuf[0x0C];
								memcpy (&PacketData[0x10], &client->decryptbuf[0x10], 10);
								memcpy (&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12 );
								*(unsigned *) &PacketData[0x28] = l->gameItem[itemNum].item.itemid;
								*(unsigned *) &PacketData[0x2C] = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
								SendToLobby ( client->lobby, 4, &PacketData[0], 0x30, 0);
							}
						}
					}
					l->monsterData[mid].drop = 1;
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0x6F:
	case 0x71:
		if ( ( client->lobbyNum > 0x0F ) && ( t < maxt ) )
		{
			if (l->leader == client->clientID)
			{
				if ((l->slot_use[t]) && (l->client[t]))
				{
					if (l->client[t]->bursting == 1)
						dont_send = 0; // More user joining game stuff...
				}
			}
		}
		break;
	case 0xA2:
		if (client->lobbyNum > 0x0F)
		{
			if (!l->drops_disabled)
			{
				// box drop
				mid = *(unsigned short*) &client->decryptbuf[0x0E];
				mid &= 0xFFF;

				if ( ( mid < 0xB50 ) && ( l->boxHit[mid] == 0 ) )
				{
					box_rare = 0;
					mb = 0;
					
					//debug ("quest loaded: %i", l->quest_loaded);

					if ( ( l->quest_loaded ) && ( (unsigned) l->quest_loaded <= numQuests ) )
					{
						QUEST* q;

						q = &quests[l->quest_loaded - 1];
						if ( mid < q->max_objects )
							mb = (MAP_BOX*) &q->objectdata[(unsigned) ( 68 * mid ) + 0x28];
					}
					else
						mb = &l->objData[mid];

					if ( mb )
					{

						if ( mb->flag1 == 0 )
						{
							if ( ( ( mb->flag2 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
								 ( ( mb->flag2 + FLOAT_PRECISION ) > (float) 1.00000 ) )
							{
								// Fixed item alert!!!

								box_rare = 1;
								itemNum = free_game_item (l);
								if ( ( ( mb->flag3 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
									 ( ( mb->flag3 + FLOAT_PRECISION ) > (float) 1.00000 ) )
								{
									// Fully fixed!

									*(unsigned *) &l->gameItem[itemNum].item.data[0] = *(unsigned *) &mb->drop[0];

									// Not used... for now.
									l->gameItem[itemNum].item.data[3] = 0;

									if (l->gameItem[itemNum].item.data[0] == 0x04)
										GenerateCommonItem (0x04, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
									else
										if ((l->gameItem[itemNum].item.data[0] == 0x00) && 
											(l->gameItem[itemNum].item.data[1] == 0x00))
											GenerateCommonItem (0xFF, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
										else
										{
											memset (&l->gameItem[itemNum].item.data2[0], 0, 4);
											if (l->gameItem[itemNum].item.data[0] <  0x02)
												l->gameItem[itemNum].item.data[1]++; // Fix item offset
											GenerateRandomAttributes (client->character.sectionID, &l->gameItem[itemNum], l, client);
											l->gameItem[itemNum].item.itemid = l->itemID++;
										}
								}
								else
									GenerateCommonItem (mb->drop[0], 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
							}
						}
					}

					if (!box_rare)
					{
						switch (l->episode)
						{
						case 0x02:
							rt_table = (unsigned char*) &rt_tables_ep2[0];
							break;
						case 0x03:
							rt_table = (unsigned char*) &rt_tables_ep4[0];
							break;
						default:
							rt_table = (unsigned char*) &rt_tables_ep1[0];
							break;
						}
						rt_table += ( ( 0x5000 * l->difficulty ) + ( client->character.sectionID * 0x800 ) ) + 0x194;
						rt_table2 = rt_table + 0x1E;
						rare_item = 0;

						switch ( l->episode )
						{
						case 0x01:
							switch ( l->floor[client->clientID ] )
							{
							case 11:
								// dragon
								floor_check = 3;
								break;
							case 12:
								// de rol
								floor_check = 6;
								break;
							case 13:
								// vol opt
								floor_check = 8;
								break;
							case 14:
								// falz
								floor_check = 10;
								break;
							default:
								floor_check = l->floor[client->clientID ];
								break;
							}	
							break;
						case 0x02:
							switch ( l->floor[client->clientID ] )
							{
							case 14:
								// barba ray
								floor_check = 3;
								break;
							case 15:
								// gol dragon
								floor_check = 6;
								break;
							case 12:
								// gal gryphon
								floor_check = 9;
								break;
							case 13:
								// olga flow
								floor_check = 10;
								break;
							default:
								// could be tower
								if ( l->floor[client->clientID] <= 11 )
									floor_check = ep2_rtremap[(l->floor[client->clientID] * 2)+1];
								else
									floor_check = 10;
								break;
							}	
							break;
						case 0x03:
							floor_check = l->floor[client->clientID ];
							break;
						}

						for (ch=0;ch<30;ch++)
						{
							if (*rt_table == floor_check)
							{
								rare_rate = ExpandDropRate ( *rt_table2 );
								memcpy (&rare_item, &rt_table2[1], 3);
								rare_roll = mt_lrand();
								if ( ( rare_roll < rare_rate ) || ( l->redbox == 1 ) )
								{
									box_rare = 1;
									itemNum = free_game_item (l);
									memset (&l->gameItem[itemNum].item.data[0], 0, 12);
									memset (&l->gameItem[itemNum].item.data2[0], 0, 4);
									memcpy (&l->gameItem[itemNum].item.data[0], &rare_item, 3);
									GenerateRandomAttributes (client->character.sectionID, &l->gameItem[itemNum], l, client);
									l->gameItem[itemNum].item.itemid = l->itemID++;
									break;
								}
							}
							rt_table++;
							rt_table2 += 0x04;
						}
					}

					if (!box_rare)
					{
						itemNum = free_game_item (l);
						GenerateCommonItem (0xFF, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
					}

					if (l->gameItem[itemNum].item.itemid != 0)
					{
						if (l->gameItemCount < MAX_SAVED_ITEMS)
							l->gameItemList[l->gameItemCount++] = itemNum;
						memset (&PacketData[0], 0, 16);
						PacketData[0x00] = 0x30;
						PacketData[0x01] = 0x00;
						PacketData[0x02] = 0x60;
						PacketData[0x03] = 0x00;
						PacketData[0x08] = 0x5F;
						PacketData[0x09] = 0x0D;
						*(unsigned *) &PacketData[0x0C] = *(unsigned *) &client->decryptbuf[0x0C];
						memcpy (&PacketData[0x10], &client->decryptbuf[0x10], 10);
						memcpy (&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12 );
						*(unsigned *) &PacketData[0x28] = l->gameItem[itemNum].item.itemid;
						*(unsigned *) &PacketData[0x2C] = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
						SendToLobby ( client->lobby, 4, &PacketData[0], 0x30, 0);
					}
					l->boxHit[mid] = 1;
				}
			}
		}
		break;
	case 0xA6:
		// Trade (not done yet)
		break;
	case 0xAE:
		// Chair info
		if ((size == 0x18) && (client->lobbyNum < 0x10) && (t < maxt))
			dont_send = 0;
		break;
	case 0xB5:
		// Client requesting shop
		if ( client->lobbyNum > 0x0F )
		{			 
			if ((l->floor[client->clientID] == 0) 
				&& (client->decryptbuf[0x0C] < 0x03))
			{
				client->doneshop[client->decryptbuf[0x0C]] = shopidx[client->character.level] + ( 333 * ((unsigned)client->decryptbuf[0x0C]) ) + ( mt_lrand() % 333 ) ;
				shopp = &shops[client->doneshop[client->decryptbuf[0x0C]]];
				cipher_ptr = &client->server_cipher;
				encryptcopy (client, (unsigned char*) &shopp->packet_length, shopp->packet_length);
			}
		}
		else
			client->todc = 1;
		break;
	case 0xB7:
		// Client buying an item
		if ( client->lobbyNum > 0x0F )
		{
			if ((l->floor[client->clientID] == 0)
				&& (client->decryptbuf[0x10] < 0x03) 
				&& (client->doneshop[client->decryptbuf[0x10]]))
			{
				if (client->decryptbuf[0x11] < shops[client->doneshop[client->decryptbuf[0x10]]].num_items)
				{
					shopi = &shops[client->doneshop[client->decryptbuf[0x10]]].item[client->decryptbuf[0x11]];
					if ((client->decryptbuf[0x12] > 1) && (shopi->data[0] != 0x03))
						client->todc = 1;
					else
						if (client->character.meseta < ((unsigned)client->decryptbuf[0x12] * shopi->price))
						{
							Send1A ("Not enough meseta for purchase.", client);
							client->todc = 1;
						}
						else
						{
							INVENTORY_ITEM i;

							memset (&i, 0, sizeof (INVENTORY_ITEM));
							memcpy (&i.item.data[0], &shopi->data[0], 12);
							// Update player item ID
							l->playerItemID[client->clientID] = *(unsigned *) &client->decryptbuf[0x0C];
							i.item.itemid = l->playerItemID[client->clientID]++;
							AddToInventory (&i, client->decryptbuf[0x12], 1, client);
							DeleteMesetaFromClient (shopi->price * (unsigned) client->decryptbuf[0x12], 0, client);
						}
				}
				else
					client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xB8:
		// Client is tekking a weapon.
		if ( client->lobbyNum > 0x0F )
		{
			unsigned compare_item;

			INVENTORY_ITEM* i;

			i = NULL;

			compare_item = *(unsigned *) &client->decryptbuf[0x0C];

			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				if ((client->character.inventory[ch].item.itemid == compare_item) &&
					(client->character.inventory[ch].item.data[0] == 0x00) &&
					(client->character.inventory[ch].item.data[4] & 0x80) &&
					(client->character.meseta >= 100))
				{
					char percent_mod;
					unsigned attrib;

					i = &client->character.inventory[ch];
					attrib = i->item.data[4] & ~(0x80);
					
					client->tekked = *i;

					if ( attrib < 0x29)
					{
						client->tekked.item.data[4] = tekker_attributes [( attrib * 3) + 1];
						if ( ( mt_lrand() % 100 ) > 70 )
							client->tekked.item.data[4] += mt_lrand() % ( ( tekker_attributes [(attrib * 3) + 2] - tekker_attributes [(attrib * 3) + 1] ) + 1 );
					}
					else
						client->tekked.item.data[4] = 0;
					if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = -10;
					else
						if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = -5;
						else
							if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = 5;
							else
								if ( ( mt_lrand() % 10 ) < 2 ) percent_mod = 10;
								else
									percent_mod = 0;
					if ((!(i->item.data[6] & 128)) && (i->item.data[7] > 0))
						(char)client->tekked.item.data[7] += percent_mod;
					if ((!(i->item.data[8] & 128)) && (i->item.data[9] > 0))
						(char)client->tekked.item.data[9] += percent_mod;
					if ((!(i->item.data[10] & 128)) && (i->item.data[11] > 0))
						(char)client->tekked.item.data[11] += percent_mod;
					DeleteMesetaFromClient (100, 0, client);
					memset (&client->encryptbuf[0x00], 0, 0x20);
					client->encryptbuf[0x00] = 0x20;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xB9;
					client->encryptbuf[0x09] = 0x08;
					client->encryptbuf[0x0A] = 0x79;
					memcpy (&client->encryptbuf[0x0C], &client->tekked.item.data[0], 16);
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x20);
					break;
				}
			}

			if ( i == NULL )
			{
				Send1A ("Could not find item to Tek.", client);
				client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xBA:
		// Client accepting tekked version of weapon.
		if ( ( client->lobbyNum > 0x0F ) && ( client->tekked.item.itemid ) )
		{
			unsigned ch2;

			for (ch=0;ch<4;ch++)
			{
				if ((l->slot_use[ch]) && (l->client[ch]))
				{
					for (ch2=0;ch2<l->client[ch]->character.inventoryUse;ch2++)
						if (l->client[ch]->character.inventory[ch2].item.itemid == client->tekked.item.itemid)
						{
							Send1A ("Item duplication attempt!", client);
							client->todc = 1;
							break;
						}
				}
			}

			for (ch=0;ch<l->gameItemCount;l++)
			{
				itemNum = l->gameItemList[ch];
				if (l->gameItem[itemNum].item.itemid == client->tekked.item.itemid)
				{
					// Added to the game's inventory by the client...
					// Delete it and avoid duping...
					memset (&l->gameItem[itemNum], 0, sizeof (GAME_ITEM));
					l->gameItemList[ch] = 0xFFFFFFFF;
					break;
				}
			}

			CleanUpGameInventory (l);

			if (!client->todc)
			{
				AddToInventory (&client->tekked, 1, 1, client);
				memset (&client->tekked, 0, sizeof (INVENTORY_ITEM));
			}
		}
		else
			client->todc = 1;
		break;
	case 0xBB:
		// Client accessing bank
		if ( client->lobbyNum < 0x10 )
			client->todc = 1;
		else
		{
			if ( ( l->floor[client->clientID] == 0) && ( ((unsigned) servertime - client->command_cooldown[0xBB]) >= 1 ))
			{
				client->command_cooldown[0xBB] = (unsigned) servertime;

				/* Which bank are we accessing? */

				client->bankAccess = client->bankType;

				if (client->bankAccess)
					memcpy (&client->character.bankUse, &client->common_bank, sizeof (BANK));
				else
					memcpy (&client->character.bankUse, &client->char_bank, sizeof (BANK));

				for (ch=0;ch<client->character.bankUse;ch++)
					client->character.bankInventory[ch].itemid = l->bankItemID[client->clientID]++;
				memset (&client->encryptbuf[0x00], 0, 0x34);
				client->encryptbuf[0x02] = 0x6C;
				client->encryptbuf[0x08] = 0xBC;
				bank_size = 0x18 * (client->character.bankUse + 1);
				*(unsigned *) &client->encryptbuf[0x0C] = bank_size;
				bank_size += 4;
				*(unsigned short *) &client->encryptbuf[0x00] = (unsigned short) bank_size;
				bank_use = mt_lrand();
				*(unsigned *) &client->encryptbuf[0x10] = bank_use;
				bank_use = client->character.bankUse;
				*(unsigned *) &client->encryptbuf[0x14] = bank_use;
				*(unsigned *) &client->encryptbuf[0x18] = client->character.bankMeseta;
				if (client->character.bankUse)
					memcpy (&client->encryptbuf[0x1C], &client->character.bankInventory[0], sizeof (BANK_ITEM) * client->character.bankUse);
				cipher_ptr = &client->server_cipher;
				encryptcopy (client, &client->encryptbuf[0x00], bank_size);
			}
		}
		break;
	case 0xBD:
		if ( client->lobbyNum < 0x10 )
		{
			dont_send = 1;
			client->todc = 1;
		}
		else
		{
			if ( l->floor[client->clientID] == 0)
			{
				switch (client->decryptbuf[0x14])
				{
				case 0x00: 
					// Making a deposit
					itemid = *(unsigned *) &client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned *) &client->decryptbuf[0x10];

						if (client->character.meseta >= meseta)
						{
							client->character.bankMeseta += meseta;
							client->character.meseta -= meseta;
							if (client->character.bankMeseta > 999999)
								client->character.bankMeseta = 999999;
						}
						else
						{
							Send1A ("Client/server data synchronization error.", client);
							client->todc = 1;
						}
					}
					else
					{
						if ( client->character.bankUse < 200 )
						{
							// Depositing something else...
							count = client->decryptbuf[0x15];
							DepositIntoBank (itemid, count, client);
							if (!client->todc)
								SortBankItems(client);
						}
						else
						{						
							Send1A ("Can't deposit.  Bank is full.", client);
							client->todc = 1;
						}
					}
					break;
				case 0x01:
					itemid = *(unsigned *) &client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned *) &client->decryptbuf[0x10];
						if (client->character.bankMeseta >= meseta)
						{
							client->character.bankMeseta -= meseta;
							client->character.meseta += meseta;
						}
						else
							client->todc = 1;
					}
					else
					{
						// Withdrawing something else...
						count = client->decryptbuf[0x15];
						WithdrawFromBank (itemid, count, client);
					}
					break;
				default:
					break;
				}

				/* Update bank. */

				if (client->bankAccess)
					memcpy (&client->common_bank, &client->character.bankUse, sizeof (BANK));
				else
					memcpy (&client->char_bank, &client->character.bankUse, sizeof (BANK));

			}
		}
		break;
	case 0xC1:
	case 0xC2:
	//case 0xCD:
	//case 0xCE:
		if (t < maxt)
		{
			// Team invite for C1 & C2, Master Transfer for CD & CE.
			if (size == 0x64)
				dont_send = 0;

			if (client->decryptbuf[0x08] == 0xC2)
			{
				unsigned gcn;

				gcn = *(unsigned *) &client->decryptbuf[0x0C];
				if ((client->decryptbuf[0x10] == 0x02) &&
					(client->guildcard == gcn))
					client->teamaccept = 1;
			}

			if (client->decryptbuf[0x08] == 0xCD)
			{
				if (client->character.privilegeLevel != 0x40)
				{
					dont_send = 1;
					Send01 ("You aren't the master of your team.", client);
				}
				else
					client->masterxfer = 1;
			}
		}
		break;
	case 0xC9:
		if ( client->lobbyNum > 0x0F )
		{
			INVENTORY_ITEM add_item;
			int meseta;

			if ( l->quest_loaded )
			{
				meseta = *(int *) &client->decryptbuf[0x0C];
				if (meseta < 0)
				{
					meseta = -meseta;
					client->character.meseta -= (unsigned) meseta;
				}
				else
				{
					memset (&add_item, 0, sizeof (INVENTORY_ITEM));
					add_item.item.data[0] = 0x04;
					*(unsigned *) &add_item.item.data2[0] = *(unsigned *) &client->decryptbuf[0x0C];
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory (&add_item, 1, 0, client);
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0xCA:
		if ( client->lobbyNum > 0x0F )
		{
			INVENTORY_ITEM add_item;

			if ( l->quest_loaded )
			{
				unsigned ci, compare_item = 0;

				memset (&add_item, 0, sizeof (INVENTORY_ITEM));
				memcpy ( &compare_item, &client->decryptbuf[0x0C], 3 );
				for ( ci = 0; ci < quest_numallows; ci ++)
				{
					if (compare_item == quest_allow[ci])
					{
						add_item.item.data[0] = 0x01;
						break;
					}
				}
				if (add_item.item.data[0] == 0x01)
				{
					memcpy (&add_item.item.data[0], &client->decryptbuf[0x0C], 12);
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory (&add_item, 1, 0, client);
				}
				else
				{
					SendEE ("You did not receive the quest reward.  The item requested is not on the allow list.  Your request and guild card have been logged for the server administrator.", client);
					WriteLog ("User %u attempted to claim quest reward %08x but item is not in the allow list.", client->guildcard, compare_item );
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0xD0:
		// Level up player?
		// Player to level @ 0x0A
		// Levels to gain @ 0x0C
		if ( ( t < maxt ) && ( l->battle ) && ( l->quest_loaded ) )
		{
			if ( ( client->decryptbuf[0x0A] < 4 ) && ( l->client[client->decryptbuf[0x0A]] ) )
			{
				unsigned target_lv;

				lClient = l->client[client->decryptbuf[0x0A]];
				target_lv = lClient->character.level;
				target_lv += client->decryptbuf[0x0C];
				
				if ( target_lv > 199 )
					 target_lv = 199;

				SkipToLevel ( target_lv, lClient, 0 );
			}
		}
		break;
	case 0xD6:
		// Wrap an item
		if ( client->lobbyNum > 0x0F )
		{
			unsigned wrap_id;
			INVENTORY_ITEM backup_item;

			memset (&backup_item, 0, sizeof (INVENTORY_ITEM));
			wrap_id = *(unsigned *) &client->decryptbuf[0x18];

			for (ch=0;ch<client->character.inventoryUse;ch++)
			{
				if (client->character.inventory[ch].item.itemid == wrap_id)
				{
					memcpy (&backup_item, &client->character.inventory[ch], sizeof (INVENTORY_ITEM));
					break;
				}
			}

			if (backup_item.item.itemid)
			{
				DeleteFromInventory (&backup_item, 1, client);					
				if (!client->todc)
				{
					if (backup_item.item.data[0] == 0x02)
						backup_item.item.data2[2] |= 0x40; // Wrap a mag
					else
						backup_item.item.data[4] |= 0x40; // Wrap other
					AddToInventory (&backup_item, 1, 0, client);
				}
			}
			else
			{
				Send1A ("Could not find item to wrap.", client);
				client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xDF:
		if ( client->lobbyNum > 0x0F )
		{
			if ( ( l->oneperson ) && ( l->quest_loaded ) && ( !l->drops_disabled ) )
			{
				INVENTORY_ITEM work_item;

				memset (&work_item, 0, sizeof (INVENTORY_ITEM));
				work_item.item.data[0] = 0x03;
				work_item.item.data[1] = 0x10;
				work_item.item.data[2] = 0x02;
				DeleteFromInventory (&work_item, 1, client);
				if (!client->todc)
					l->drops_disabled = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xE0:
		if ( client->lobbyNum > 0x0F )
		{
			if ( ( l->oneperson ) && ( l->quest_loaded ) && ( l->drops_disabled ) && ( !l->questE0 ) )
			{
				unsigned bp, bpl, new_item;

				if ( client->decryptbuf[0x0D] > 0x03 )
					bpl = 1;
				else
					bpl = l->difficulty + 1;

				for (bp=0;bp<bpl;bp++)
				{
					new_item = 0;

					switch ( client->decryptbuf[0x0D] )
					{
					case 0x00:
						// bp1 dorphon route
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp_dorphon_normal[mt_lrand() % (sizeof(bp_dorphon_normal)/4)];
							break;
						case 0x01:
							new_item = bp_dorphon_hard[mt_lrand() % (sizeof(bp_dorphon_hard)/4)];
							break;
						case 0x02:
							new_item = bp_dorphon_vhard[mt_lrand() % (sizeof(bp_dorphon_vhard)/4)];
							break;
						case 0x03:
							new_item = bp_dorphon_ultimate[mt_lrand() % (sizeof(bp_dorphon_ultimate)/4)];
							break;
						}
						break;
					case 0x01:
						// bp1 rappy route
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp_rappy_normal[mt_lrand() % (sizeof(bp_rappy_normal)/4)];
							break;
						case 0x01:
							new_item = bp_rappy_hard[mt_lrand() % (sizeof(bp_rappy_hard)/4)];
							break;
						case 0x02:
							new_item = bp_rappy_vhard[mt_lrand() % (sizeof(bp_rappy_vhard)/4)];
							break;
						case 0x03:
							new_item = bp_rappy_ultimate[mt_lrand() % (sizeof(bp_rappy_ultimate)/4)];
							break;
						}
						break;
					case 0x02:
						// bp1 zu route
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp_zu_normal[mt_lrand() % (sizeof(bp_zu_normal)/4)];
							break;
						case 0x01:
							new_item = bp_zu_hard[mt_lrand() % (sizeof(bp_zu_hard)/4)];
							break;
						case 0x02:
							new_item = bp_zu_vhard[mt_lrand() % (sizeof(bp_zu_vhard)/4)];
							break;
						case 0x03:
							new_item = bp_zu_ultimate[mt_lrand() % (sizeof(bp_zu_ultimate)/4)];
							break;
						}
						break;
					case 0x04:
						// bp2
						switch ( l->difficulty )
						{
						case 0x00:
							new_item = bp2_normal[mt_lrand() % (sizeof(bp2_normal)/4)];
							break;
						case 0x01:
							new_item = bp2_hard[mt_lrand() % (sizeof(bp2_hard)/4)];
							break;
						case 0x02:
							new_item = bp2_vhard[mt_lrand() % (sizeof(bp2_vhard)/4)];
							break;
						case 0x03:
							new_item = bp2_ultimate[mt_lrand() % (sizeof(bp2_ultimate)/4)];
							break;
						}
						break;
					}
					l->questE0 = 1;
					memset (&client->encryptbuf[0x00], 0, 0x2C);
					client->encryptbuf[0x00] = 0x2C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0x5D;
					client->encryptbuf[0x09] = 0x09;
					client->encryptbuf[0x0A] = 0xFF;
					client->encryptbuf[0x0B] = 0xFB;
					memcpy (&client->encryptbuf[0x0C], &client->decryptbuf[0x0C], 12 );
					*(unsigned *) &client->encryptbuf[0x18] = new_item;
					*(unsigned *) &client->encryptbuf[0x24] = l->itemID;
					itemNum = free_game_item (l);
					if (l->gameItemCount < MAX_SAVED_ITEMS)
						l->gameItemList[l->gameItemCount++] = itemNum;
					memset (&l->gameItem[itemNum], 0, sizeof (GAME_ITEM));
					*(unsigned *) &l->gameItem[itemNum].item.data[0] = new_item;
					if (new_item == 0x04)
					{
						new_item  = pt_tables_ep1[client->character.sectionID][l->difficulty].enemy_meseta[0x2E][0];
						new_item += mt_lrand() % 100;
						*(unsigned *) &client->encryptbuf[0x28] = new_item;
						*(unsigned *) &l->gameItem[itemNum].item.data2[0] = new_item;
					}
					if (l->gameItem[itemNum].item.data[0] == 0x00)
					{
						l->gameItem[itemNum].item.data[4] = 0x80; // Untekked
						client->encryptbuf[0x1C] = 0x80;
					}
					l->gameItem[itemNum].item.itemid = l->itemID++;
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x2C);
				}
			}
		}
		break;
	default:
		if (client->lobbyNum > 0x0F)
		{
			WriteLog ("62 command \"%02x\" not handled in game. (Data below)", client->decryptbuf[0x08]);
			packet_to_text ( &client->decryptbuf[0x00], size );
			WriteLog ("%s", &dp[0]);
		}
		break;
	}

	if ( ( dont_send == 0 ) && ( !client->todc ) )
	{
		if ((l->slot_use[t]) && (l->client[t]))
		{
			lClient = l->client[t];
			cipher_ptr = &lClient->server_cipher;
			encryptcopy (lClient, &client->decryptbuf[0], size);
		}
	}
}


void Send6D (BANANA* client)
{
	BANANA* lClient;
	unsigned short size;
	unsigned short sizecheck = 0;
	unsigned char t;
	int dont_send = 0;
	LOBBY* l;

	if (!client->lobby)
		return;

	size = *(unsigned short*) &client->decryptbuf[0x00];
	sizecheck = *(unsigned short*) &client->decryptbuf[0x0C];
	sizecheck += 8;

	if (size != sizecheck)
	{
		debug ("Client sent a 0x6D packet whose sizecheck != size.\n");
		debug ("Command: %02X | Size: %04X | Sizecheck: %04x\n", client->decryptbuf[0x08],
			size, sizecheck);
		dont_send = 1;
	}

	l = (LOBBY*) client->lobby;
	t = client->decryptbuf[0x04];
	if (t >= 0x04)
		dont_send = 1;

	if (!dont_send)
	{
		switch (client->decryptbuf[0x08])
		{
		case 0x70:
			if (client->lobbyNum > 0x0F)
			{
				if ((l->slot_use[t]) && (l->client[t]))
				{
					if (l->client[t]->bursting == 1)
					{
						unsigned ch;

						dont_send = 0; // It's cool to send them as long as this user is bursting.

						// Let's reconstruct the 0x70 as much as possible.
						//
						// Could check guild card # here
						*(unsigned *) &client->decryptbuf[0x7C] = client->guildcard;
						// Check techniques...
						if (!(client->equip_flags & DROID_FLAG))
						{
							for (ch=0;ch<19;ch++)
							{
								if ((char) client->decryptbuf[0xC4+ch] > max_tech_level[ch][client->character._class])
								{
									(char) client->character.techniques[ch] = -1; // Unlearn broken technique.
									client->todc = 1;
								}
							}
							if (client->todc)
								Send1A ("Technique data check failed.\n\nSome techniques have been unlearned.", client);
						}
						memcpy (&client->decryptbuf[0xC4], &client->character.techniques, 20);
						// Could check character structure here
						memcpy (&client->decryptbuf[0xD8], &client->character.gcString, 104);
						// Prevent crashing with NPC skins...
						if (client->character.skinFlag)
							memset (&client->decryptbuf[0x110], 0, 10);
						// Could check stats here
						memcpy (&client->decryptbuf[0x148], &client->character.ATP, 36);
						// Could check inventory here
						client->decryptbuf[0x16C] = client->character.inventoryUse;
						memcpy (&client->decryptbuf[0x170], &client->character.inventory[0], 30 * sizeof (INVENTORY_ITEM) );
					}
				}
			}
			break;
		case 0x6B:
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x72:
			if (client->lobbyNum > 0x0F)
			{
				if (l->leader == client->clientID)
				{
					if ((l->slot_use[t]) && (l->client[t]))
					{
						if (l->client[t]->bursting == 1)
							dont_send = 0; // It's cool to send them as long as this user is bursting and we're the leader.
					}
				}
			}
			break;
		default:
			dont_send = 1;
			break;
		}
	}

	if ( dont_send == 0 )
	{
		lClient = l->client[t];
		cipher_ptr = &lClient->server_cipher;
		encryptcopy (lClient, &client->decryptbuf[0], size);
	}
}


void Send01 (const char *text, BANANA* client)
{
	unsigned short mesgOfs = 0x10;
	unsigned ch;

	memset(&PacketData[0], 0, 16);
	PacketData[mesgOfs++] = 0x09;
	PacketData[mesgOfs++] = 0x00;
	PacketData[mesgOfs++] = 0x45;
	PacketData[mesgOfs++] = 0x00;
	for (ch=0;ch<strlen(text);ch++)
	{
		PacketData[mesgOfs++] = text[ch];
		PacketData[mesgOfs++] = 0x00;
	}
	PacketData[mesgOfs++] = 0x00;
	PacketData[mesgOfs++] = 0x00;
	while (mesgOfs % 8)
		PacketData[mesgOfs++] = 0x00;
	*(unsigned short*) &PacketData[0] = mesgOfs;
	PacketData[0x02] = 0x01;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], mesgOfs);
}

unsigned char chatBuf[4000];
unsigned char cmdBuf[4000];
char* myCommand;
char* myArgs[64];

char* Unicode_to_ASCII (unsigned short* ucs)
{
	char *s,c;

	s = (char*) &chatBuf[0];
	while (*ucs != 0x0000)
	{
		c = *(ucs++) & 0xFF;
		if ((c >= 0x20) && (c <= 0x80))
			*(s++) = c;
		else
			*(s++) = 0x20;
	}
	*(s++) = 0x00;
	return (char*) &chatBuf[0];
}

void WriteLog(char *fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[ MAX_GM_MESG_LEN ];
	SYSTEMTIME rawtime;

	FILE *fp;

	GetLocalTime (&rawtime);
	va_start (args, fmt);
	strcpy (text + vsprintf( text,fmt,args), "\r\n"); 
	va_end (args);

	fp = fopen ( "ship.log", "a");
	if (!fp)
	{
		printf ("Unable to log to ship.log\n");
	}

	fprintf (fp, "[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
	fclose (fp);

	printf ("[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
}


void WriteGM(char *fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[ MAX_GM_MESG_LEN ];
	SYSTEMTIME rawtime;

	FILE *fp;

	GetLocalTime (&rawtime);
	va_start (args, fmt);
	strcpy (text + vsprintf( text,fmt,args), "\r\n"); 
	va_end (args);

	fp = fopen ( "gm.log", "a");
	if (!fp)
	{
		printf ("Unable to log to gm.log\n");
	}

	fprintf (fp, "[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
	fclose (fp);

	printf ("[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
}


char character_file[255];

void Send06 (BANANA* client)
{
	FILE* fp;
	unsigned short chatsize;
	unsigned pktsize;
	unsigned ch, ch2;
	unsigned char stackable, count;
	unsigned short *n;
	unsigned short target;
	unsigned myCmdArgs, itemNum, connectNum, gc_num;
	unsigned short npcID;
	unsigned max_send;
	BANANA* lClient;
	int i, z, commandLen, ignored, found_ban, writeData;
	LOBBY* l;
	INVENTORY_ITEM ii;

	writeData = 0;
	fp = NULL;

	if (!client->lobby)
		return;

	l = client->lobby;

	pktsize = *(unsigned short*) &client->decryptbuf[0x00];

	if (pktsize > 0x100)
		return;

	memset (&chatBuf[0x00], 0, 0x0A);

	chatBuf[0x02] = 0x06;
	chatBuf[0x0A] = client->clientID;
	*(unsigned *) &chatBuf[0x0C] = client->guildcard;
	chatsize = 0x10;
	n = (unsigned short*) &client->character.name[4];
	for (ch=0;ch<10;ch++)
	{
		if (*n == 0x0000)
			break;
		*(unsigned short*) &chatBuf[chatsize] = *n;
		chatsize += 2;
		n++;
	}
	chatBuf[chatsize++] = 0x09;
	chatBuf[chatsize++] = 0x00;
	chatBuf[chatsize++] = 0x09;
	chatBuf[chatsize++] = 0x00;
	n = (unsigned short*) &client->decryptbuf[0x12];
	if ((*(n+1) == 0x002F) && (*(n+2) != 0x002F))
	{
		commandLen = 0;

		for (ch=0;ch<(pktsize - 0x14);ch+= 2)
		{
			if (client->decryptbuf[(0x14+ch)] != 0x00)
				cmdBuf[commandLen++] = client->decryptbuf[(0x14+ch)];
			else
				break;
		}

		cmdBuf[commandLen] = 0;

		myCmdArgs = 0;
		myCommand = &cmdBuf[1];

		if ( ( i = strcspn ( &cmdBuf[1], " ," ) ) != ( strlen ( &cmdBuf[1] ) ) )
		{
			i++;
			cmdBuf[i++] = 0;
			while ( ( i < commandLen ) && ( myCmdArgs < 64 ) )
			{
				z = strcspn ( &cmdBuf[i], "," );
				myArgs[myCmdArgs++] = &cmdBuf[i];
				i += z;
				cmdBuf[i++] = 0;
			}
		}

		if ( commandLen )
		{

			if ( !strcmp ( myCommand, "debug" ) )
				writeData = 1;

			if ( !strcmp ( myCommand, "setpass" ) )
			{
				if (!client->lobbyNum < 0x10)
				{
					if ( myCmdArgs == 0 )
						SendB0 ("Need new password.", client);
					else
					{
						ch = 0;
						while (myArgs[0][ch] != 0)
						{
							l->gamePassword [ch*2] = myArgs[0][ch];
							l->gamePassword [(ch*2)+1] = 0;
							ch++;
							if (ch==31) break; // Limit amount of characters...
						}
						l->gamePassword[ch*2] = 0;
						l->gamePassword[(ch*2)+1] = 0;
						for (ch=0;ch<4;ch++)
						{
							if ((l->slot_use[ch]) && (l->client[ch]))
								SendB0 ("Room password changed.", l->client[ch]);
						}
					}
				}
			}

			if ( !strcmp ( myCommand, "arrow" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need arrow digit.", client);
				else
				{
					l->arrow_color[client->clientID] = atoi ( myArgs[0] );
					ShowArrows (client, 1);
				}
			}

			if ( !strcmp ( myCommand, "lang" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need language digit.", client);
				else
				{
					npcID = atoi ( myArgs[0] );

					if ( npcID > numLanguages ) npcID = 1;

					if ( npcID == 0 )
						npcID = 1;

					npcID--;

					client->character.lang = (unsigned char) npcID;

					SendB0 ("Current language:\n", client);
					SendB0 (languageNames[npcID], client);

				}
			}


			if ( !strcmp ( myCommand, "npc" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need NPC digit. (max = 11, 0 to unskin)", client);
				else
				{
					npcID = atoi ( myArgs[0] );

					if ( npcID > 7 ) 
					{
						if ( ( npcID > 11 ) || ( !ship_support_extnpc ) )
							npcID = 7;
					}

					if ( npcID == 0 )
					{
						client->character.skinFlag = 0x00;
						client->character.skinID = 0x00;
					}
					else
					{
						client->character.skinFlag = 0x02;
						client->character.skinID = npcID - 1;
					}
					SendB0 ("Skin updated, change blocks for it to take effect.", client );
				}
			}

			// Process GM commands

			if ( ( !strcmp ( myCommand, "event" ) ) && ( (client->isgm) || (playerHasRights(client->guildcard, 0))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need event digit.", client);
				else
				{

					shipEvent = atoi ( myArgs[0] );

					WriteGM ("GM %u has changed ship event to %u", client->guildcard, shipEvent );

					PacketDA[0x04] = shipEvent;

					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard)
						{
							cipher_ptr = &connections[connectNum]->server_cipher;
							encryptcopy (connections[connectNum], &PacketDA[0], 8);
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "redbox") ) && ( client->isgm ) )
			{
				if (l->redbox)
				{
					l->redbox = 0;
					SendB0 ("Red box mode turned off.", client);
					WriteGM ("GM %u has deactivated redbox mode", client->guildcard ); 
				}
				else
				{
					l->redbox = 1;
					SendB0 ("Red box mode turned on!", client);
					WriteGM ("GM %u has activated redbox mode", client->guildcard ); 
				}
			}

			if (( !strcmp ( myCommand, "item" ) )  && (client->isgm))
			{
				// Item creation...
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Cannot make items in the lobby!!!", client);
				else
					if ( myCmdArgs < 4 )
						SendB0 ("You must specify at least four arguments for the desired item.", client);
					else
						if ( strlen ( myArgs[0] ) < 8 ) 
							SendB0 ("Main arguments is an incorrect length.", client);
						else
						{
							if ( ( strlen ( myArgs[1] ) < 8 ) ||
								( strlen ( myArgs[2] ) < 8 ) || 
								( strlen ( myArgs[3] ) < 8 ) )
								SendB0 ("Some arguments were incorrect and replaced.", client);

							WriteGM ("GM %u created an item", client->guildcard);

							itemNum = free_game_item (l);

							_strupr ( myArgs[0] );
							l->gameItem[itemNum].item.data[0]  = hexToByte (&myArgs[0][0]);
							l->gameItem[itemNum].item.data[1]  = hexToByte (&myArgs[0][2]);
							l->gameItem[itemNum].item.data[2]  = hexToByte (&myArgs[0][4]);
							l->gameItem[itemNum].item.data[3]  = hexToByte (&myArgs[0][6]);


							if ( strlen ( myArgs[1] ) >= 8 ) 
							{
								_strupr ( myArgs[1] );
								l->gameItem[itemNum].item.data[4]  = hexToByte (&myArgs[1][0]);
								l->gameItem[itemNum].item.data[5]  = hexToByte (&myArgs[1][2]);
								l->gameItem[itemNum].item.data[6]  = hexToByte (&myArgs[1][4]);
								l->gameItem[itemNum].item.data[7]  = hexToByte (&myArgs[1][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data[4]  = 0;
								l->gameItem[itemNum].item.data[5]  = 0;
								l->gameItem[itemNum].item.data[6]  = 0;
								l->gameItem[itemNum].item.data[7]  = 0;
							}

							if ( strlen ( myArgs[2] ) >= 8 ) 
							{
								_strupr ( myArgs[2] );
								l->gameItem[itemNum].item.data[8]  = hexToByte (&myArgs[2][0]);
								l->gameItem[itemNum].item.data[9]  = hexToByte (&myArgs[2][2]);
								l->gameItem[itemNum].item.data[10] = hexToByte (&myArgs[2][4]);
								l->gameItem[itemNum].item.data[11] = hexToByte (&myArgs[2][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data[8]  = 0;
								l->gameItem[itemNum].item.data[9]  = 0;
								l->gameItem[itemNum].item.data[10] = 0;
								l->gameItem[itemNum].item.data[11] = 0;
							}

							if ( strlen ( myArgs[3] ) >= 8 ) 
							{
								_strupr ( myArgs[3] );
								l->gameItem[itemNum].item.data2[0]  = hexToByte (&myArgs[3][0]);
								l->gameItem[itemNum].item.data2[1]  = hexToByte (&myArgs[3][2]);
								l->gameItem[itemNum].item.data2[2]  = hexToByte (&myArgs[3][4]);
								l->gameItem[itemNum].item.data2[3]  = hexToByte (&myArgs[3][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data2[0]  = 0;
								l->gameItem[itemNum].item.data2[1]  = 0;
								l->gameItem[itemNum].item.data2[2]  = 0;
								l->gameItem[itemNum].item.data2[3]  = 0;
							}

							// check stackable shit

							stackable = 0;

							if (l->gameItem[itemNum].item.data[0] == 0x03)
								stackable = stackable_table[l->gameItem[itemNum].item.data[1]];

							if ( ( stackable ) && ( l->gameItem[itemNum].item.data[5] == 0x00 ) )
								l->gameItem[itemNum].item.data[5] = 0x01; // force at least 1 of a stack to drop

							WriteGM ("Item data: %02X%02X%02X%02X,%02X%02X%02X%02X,%02X%02x%02x%02x,%02x%02x%02x%02x",
								l->gameItem[itemNum].item.data[0], l->gameItem[itemNum].item.data[1], l->gameItem[itemNum].item.data[2], l->gameItem[itemNum].item.data[3],
								l->gameItem[itemNum].item.data[4], l->gameItem[itemNum].item.data[5], l->gameItem[itemNum].item.data[6], l->gameItem[itemNum].item.data[7],
								l->gameItem[itemNum].item.data[8], l->gameItem[itemNum].item.data[9], l->gameItem[itemNum].item.data[10], l->gameItem[itemNum].item.data[11],
								l->gameItem[itemNum].item.data2[0], l->gameItem[itemNum].item.data2[1], l->gameItem[itemNum].item.data2[2], l->gameItem[itemNum].item.data2[3] );

							l->gameItem[itemNum].item.itemid = l->itemID++;
							if (l->gameItemCount < MAX_SAVED_ITEMS)
								l->gameItemList[l->gameItemCount++] = itemNum;
							memset (&PacketData[0], 0, 0x2C);
							PacketData[0x00] = 0x2C;
							PacketData[0x02] = 0x60;
							PacketData[0x08] = 0x5D;
							PacketData[0x09] = 0x09;
							PacketData[0x0A] = 0xFF;
							PacketData[0x0B] = 0xFB;
							*(unsigned *) &PacketData[0x0C] = l->floor[client->clientID];
							*(unsigned *) &PacketData[0x10] = l->clientx[client->clientID];
							*(unsigned *) &PacketData[0x14] = l->clienty[client->clientID];
							memcpy (&PacketData[0x18], &l->gameItem[itemNum].item.data[0], 12 );
							*(unsigned *) &PacketData[0x24] = l->gameItem[itemNum].item.itemid;
							*(unsigned *) &PacketData[0x28] = *(unsigned *) &l->gameItem[itemNum].item.data2[0];
							SendToLobby ( client->lobby, 4, &PacketData[0], 0x2C, 0);
							SendB0 ("Item created.", client);
						}								
			}

			if (( !strcmp ( myCommand, "give" ) )  && (client->isgm))
			{
				// Insert item into inventory
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Cannot give items in the lobby!!!", client);
				else
					if ( myCmdArgs < 4 )
						SendB0 ("You must specify at least four arguments for the desired item.", client);
					else
						if ( strlen ( myArgs[0] ) < 8 )
							SendB0 ("Main arguments is an incorrect length.", client);
						else
						{
							if ( ( strlen ( myArgs[1] ) < 8 ) ||
								 ( strlen ( myArgs[2] ) < 8 ) || 
								 ( strlen ( myArgs[3] ) < 8 ) )
								SendB0 ("Some arguments were incorrect and replaced.", client);

							WriteGM ("GM %u obtained an item", client->guildcard);

							_strupr ( myArgs[0] );
							ii.item.data[0]  = hexToByte (&myArgs[0][0]);
							ii.item.data[1]  = hexToByte (&myArgs[0][2]);
							ii.item.data[2]  = hexToByte (&myArgs[0][4]);
							ii.item.data[3]  = hexToByte (&myArgs[0][6]);


							if ( strlen ( myArgs[1] ) >= 8 ) 
							{
								_strupr ( myArgs[1] );
								ii.item.data[4]  = hexToByte (&myArgs[1][0]);
								ii.item.data[5]  = hexToByte (&myArgs[1][2]);
								ii.item.data[6]  = hexToByte (&myArgs[1][4]);
								ii.item.data[7]  = hexToByte (&myArgs[1][6]);
							}
							else
							{
								ii.item.data[4]  = 0;
								ii.item.data[5]  = 0;
								ii.item.data[6]  = 0;
								ii.item.data[7]  = 0;
							}

							if ( strlen ( myArgs[2] ) >= 8 ) 
							{
								_strupr ( myArgs[2] );
								ii.item.data[8]  = hexToByte (&myArgs[2][0]);
								ii.item.data[9]  = hexToByte (&myArgs[2][2]);
								ii.item.data[10] = hexToByte (&myArgs[2][4]);
								ii.item.data[11] = hexToByte (&myArgs[2][6]);
							}
							else
							{
								ii.item.data[8]  = 0;
								ii.item.data[9]  = 0;
								ii.item.data[10] = 0;
								ii.item.data[11] = 0;
							}

							if ( strlen ( myArgs[3] ) >= 8 ) 
							{
								_strupr ( myArgs[3] );
								ii.item.data2[0]  = hexToByte (&myArgs[3][0]);
								ii.item.data2[1]  = hexToByte (&myArgs[3][2]);
								ii.item.data2[2]  = hexToByte (&myArgs[3][4]);
								ii.item.data2[3]  = hexToByte (&myArgs[3][6]);
							}
							else
							{
								ii.item.data2[0]  = 0;
								ii.item.data2[1]  = 0;
								ii.item.data2[2]  = 0;
								ii.item.data2[3]  = 0;
							}

							// check stackable shit

							stackable = 0;

							if (ii.item.data[0] == 0x03)
								stackable = stackable_table[ii.item.data[1]];

							if ( stackable )
							{
								if ( ii.item.data[5] == 0x00 )
									ii.item.data[5] = 0x01; // force at least 1 of a stack to drop
								count = ii.item.data[5];
							}
							else
								count = 1;

							WriteGM ("Item data: %02X%02X%02X%02X,%02X%02X%02X%02X,%02X%02x%02x%02x,%02x%02x%02x%02x",
								ii.item.data[0], ii.item.data[1], ii.item.data[2], ii.item.data[3],
								ii.item.data[4], ii.item.data[5], ii.item.data[6], ii.item.data[7],
								ii.item.data[8], ii.item.data[9], ii.item.data[10], ii.item.data[11],
								ii.item.data2[0], ii.item.data2[1], ii.item.data2[2], ii.item.data2[3] );

							ii.item.itemid = l->itemID++;
							AddToInventory ( &ii, count, 0, client );
							SendB0 ("Item obtained.", client);
						}
			}

			if ( ( !strcmp ( myCommand, "warpme" ) )  && ((client->isgm) || (playerHasRights(client->guildcard, 3))) )
			{
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Can't warp in the lobby!!!", client);
				else
					if ( myCmdArgs == 0 )
						SendB0 ("Need area to warp to...", client);
					else
					{
						target = atoi ( myArgs[0] );
						if ( target > 17 )
							SendB0 ("Warping past area 17 would probably crash your client...", client);
						else
						{
							warp_packet[0x0C] = (unsigned char) atoi ( myArgs[0] );
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, &warp_packet[0], sizeof (warp_packet));
						}
					}
			}

			if ( ( !strcmp ( myCommand, "dc" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 4))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to disconnect.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gc_num)
						{
							if ((connections[connectNum]->isgm) && (isLocalGM(client->guildcard)))
								SendB0 ("You may not disconnect this user.", client);
							else
							{
								WriteGM ("GM %u has disconnected user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII ((unsigned short*) &connections[connectNum]->character.name[4]));
								Send1A ("You've been disconnected by a GM.", connections[connectNum]);
								connections[connectNum]->todc = 1;
								break;
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "ban" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 11))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to ban.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					found_ban = 0;

					for (ch=0;ch<num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 1))
						{
							found_ban = 1;
							ban ( gc_num, (unsigned*) &client->ipaddr, &client->hwinfo, 1, client ); // Should unban...
							WriteGM ("GM %u has removed ban from guild card %u.", client->guildcard, gc_num);
							SendB0 ("Ban removed.", client );
							break;
						}
					}

					if (!found_ban)
					{
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0 ("You may not ban this user.", client);
								else
								{
									if ( ban ( gc_num, (unsigned*) &connections[connectNum]->ipaddr, 
										&connections[connectNum]->hwinfo, 1, client ) )
									{
										WriteGM ("GM %u has banned user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII ((unsigned short*) &connections[connectNum]->character.name[4]));
										Send1A ("You've been banned by a GM.", connections[connectNum]);
										SendB0 ("User has been banned.", client );
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "ipban" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 12))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to IP ban.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					found_ban = 0;

					for (ch=0;ch<num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 2))
						{
							found_ban = 1;
							ban ( gc_num, (unsigned*) &client->ipaddr, &client->hwinfo, 2, client ); // Should unban...
							WriteGM ("GM %u has removed IP ban from guild card %u.", client->guildcard, gc_num);
							SendB0 ("IP ban removed.", client );
							break;
						}
					}

					if (!found_ban)
					{
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0 ("You may not ban this user.", client);
								else
								{
									if ( ban ( gc_num, (unsigned*) &connections[connectNum]->ipaddr, 
										&connections[connectNum]->hwinfo, 2, client ) )
									{
										WriteGM ("GM %u has IP banned user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII ((unsigned short*) &connections[connectNum]->character.name[4]));
										Send1A ("You've been banned by a GM.", connections[connectNum]);
										SendB0 ("User has been IP banned.", client );
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "hwban" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 12))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to HW ban.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					found_ban = 0;

					for (ch=0;ch<num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 3))
						{
							found_ban = 1;
							ban ( gc_num, (unsigned*) &client->ipaddr, &client->hwinfo, 3, client ); // Should unban...
							WriteGM ("GM %u has removed HW ban from guild card %u.", client->guildcard, gc_num);
							SendB0 ("HW ban removed.", client );
							break;
						}
					}

					if (!found_ban)
					{
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0 ("You may not ban this user.", client);
								else
								{
									if ( ban ( gc_num, (unsigned*) &connections[connectNum]->ipaddr, 
										&connections[connectNum]->hwinfo, 3, client ) )
									{
										WriteGM ("GM %u has HW banned user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII ((unsigned short*) &connections[connectNum]->character.name[4]));
										Send1A  ("You've been banned by a GM.", connections[connectNum]);
										SendB0  ("User has been HW banned.", client );
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}


			if ( ( !strcmp ( myCommand, "dcall" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 5))) )
			{
				printf ("Blocking connections until all users are disconnected...\n");
				WriteGM ("GM %u has disconnected all users", client->guildcard);
				blockConnections = 1;
			}

			if ( ( !strcmp ( myCommand, "announce" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 6))) )
			{
				if (client->announce != 0)
				{
					SendB0 ("Announce\ncancelled.", client);
					client->announce = 0;
				}
				else
				{
					SendB0 ("Announce by\nsending a\nmail.", client);
					client->announce = 1;
				}
			}

			if ( ( !strcmp ( myCommand, "global" ) ) && (client->isgm) )
			{
				if (client->announce != 0)
				{
					SendB0 ("Announce\ncancelled.", client);
					client->announce = 0;
				}
				else
				{
					SendB0 ("Global announce\nby sending\na mail.", client);
					client->announce = 2;
				}
			}

			if ( ( !strcmp ( myCommand, "levelup" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 7))) )
			{
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Cannot level up in the lobby!!!", client);
				else
					if ( l->floor[client->clientID] == 0 )
						SendB0 ("Please leave Pioneer 2 before using this command...", client);
					else
						if ( myCmdArgs == 0 )
							SendB0 ("Must specify a target level to level up to...", client);
						else
						{
							target = atoi ( myArgs[0] );
							if ( ( client->character.level + 1 ) >= target )
								SendB0 ("Target level must be higher than your current level...", client);
							else
							{
								// Do the level up!!!

								if (target > 200)
									target = 200;

								target -= 2;

								AddExp (tnlxp[target] - client->character.XP, client);
							}
						}
			}

			if ( (!strcmp ( myCommand, "updatelocalgms" )) && ((client->isgm) || (playerHasRights(client->guildcard, 8))) )
			{
				SendB0 ("Local GM file reloaded.", client);
				readLocalGMFile();
			}

			if ( (!strcmp ( myCommand, "updatemasks" )) && ((client->isgm) || (playerHasRights(client->guildcard, 12))) )
			{
				SendB0 ("IP ban masks file reloaded.", client);
				load_mask_file();
			}

			if ( !strcmp ( myCommand, "bank" ) )
			{
				if (client->bankType)
				{
					client->bankType = 0;
					SendB0 ("Bank: Character", client);
				}
				else
				{
					client->bankType = 1;
					SendB0 ("Bank: Common", client);
				}
			}

			if ( !strcmp ( myCommand, "ignore" ) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to ignore.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					ignored = 0;

					for (ch=0;ch<client->ignore_count;ch++)
					{
						if (client->ignore_list[ch] == gc_num)
						{
							ignored = 1;
							client->ignore_list[ch] = 0;
							SendB0 ("User no longer being ignored.", client);
							break;
						}
					}

					if (!ignored)
					{
						if (client->ignore_count < 100)
						{
							client->ignore_list[client->ignore_count++] = gc_num;
							SendB0 ("User is now ignored.", client);
						}
						else
							SendB0 ("Ignore list is full.", client);
					}
					else
					{
						ch2 = 0;
						for (ch=0;ch<client->ignore_count;ch++)
						{
							if ((client->ignore_list[ch] != 0) && (ch != ch2))
								client->ignore_list[ch2++] = client->ignore_list[ch];
						}
						client->ignore_count = ch2;
					}
				}
			}

			if ( ( !strcmp ( myCommand, "stfu" ) ) && ((client->isgm) || (playerHasRights(client->guildcard, 9))) )
			{
				if ( myCmdArgs == 0 )
					SendB0 ("Need a guild card # to silence.", client);
				else
				{
					gc_num = atoi ( myArgs[0] );
					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gc_num)
						{
							if ((connections[connectNum]->isgm) && (isLocalGM(client->guildcard)))
								SendB0 ("You may not silence this user.", client);
							else
							{							
								if (toggle_stfu(connections[connectNum]->guildcard, client))
								{
									WriteGM ("GM %u has silenced user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII ((unsigned short*) &connections[connectNum]->character.name[4]));
									SendB0  ("User has been silenced.", client);
									SendB0  ("You've been silenced.", connections[connectNum]);
								}
								else
								{
									WriteGM ("GM %u has removed silence from user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII ((unsigned short*) &connections[connectNum]->character.name[4]));
									SendB0  ("User is now allowed to speak.", client);
									SendB0  ("You may now speak freely.", connections[connectNum]);
								}
								break;
							}
						}
					}
				}
			}

			if ( ( !strcmp ( myCommand, "warpall" ) )  && ((client->isgm) || (playerHasRights(client->guildcard, 10))) )
			{
				if ( client->lobbyNum < 0x10 )
					SendB0 ("Can't warp in the lobby!!!", client);
				else
					if ( myCmdArgs == 0 )
						SendB0 ("Need area to warp to...", client);
					else
					{
						target = atoi ( myArgs[0] );
						if ( target > 17 )
							SendB0 ("Warping past area 17 would probably crash your client...", client);
						else
						{
							warp_packet[0x0C] = (unsigned char) atoi ( myArgs[0] );
							SendToLobby ( client->lobby, 4, &warp_packet[0], sizeof (warp_packet), 0 );
						}
					}
			}
		}
	}
	else
	{
		for (ch=0;ch<(pktsize - 0x12);ch+=2)
		{
			if ((*n == 0x0000) || (chatsize == 0xC0))
				break;
			if ((*n == 0x0009) || (*n == 0x000A))
				*n = 0x0020;
			*(unsigned short*) &chatBuf[chatsize] = *n;
			chatsize += 2;
			n++;
		}
		chatBuf[chatsize++] = 0x00;
		chatBuf[chatsize++] = 0x00;
		while (chatsize % 8)
			chatBuf[chatsize++] = 0x00;
		*(unsigned short*) &chatBuf[0x00] = chatsize;
		if ( !stfu ( client->guildcard ) )
		{
			if ( client->lobbyNum < 0x10 )
				max_send = 12;
			else
				max_send = 4;
			for (ch=0;ch<max_send;ch++)
			{
				if ((l->slot_use[ch]) && (l->client[ch]))
				{
					ignored = 0;
					lClient = l->client[ch];
					for (ch2=0;ch2<lClient->ignore_count;ch2++)
					{
						if (lClient->ignore_list[ch2] == client->guildcard)
						{
							ignored = 1;
							break;
						}
					}
					if (!ignored)
					{
						cipher_ptr = &lClient->server_cipher;
						encryptcopy ( lClient, &chatBuf[0x00], chatsize );
					}
				}
			}
		}
	}

	if ( writeData )
	{
		if (!client->debugged)
		{
			client->debugged = 1;
			_itoa (client->character.guildCard, &character_file[0], 10);
			strcat (&character_file[0], Unicode_to_ASCII ((unsigned short*) &client->character.name[4]));
			strcat (&character_file[0], ".bbc");
			fp = fopen (&character_file[0], "wb");
			if (fp)
			{
				fwrite (&client->character.packetSize, 1, sizeof (CHARDATA), fp);
				fclose (fp);
			}
			WriteLog ("User %u (%s) has wrote character debug data.", client->guildcard, Unicode_to_ASCII ((unsigned short*) &client->character.name[4]) );
			SendB0 ("Your debug data has been saved.", client);
		}
		else
			SendB0 ("Your debug data has already been saved.", client);
	}
}

void CommandED(BANANA* client)
{
	switch (client->decryptbuf[0x03])
	{
	case 0x01:
		// Options
		*(unsigned *) &client->character.options[0] = *(unsigned *) &client->decryptbuf[0x08];
		break;
	case 0x02:
		// Symbol Chats
		memcpy (&client->character.symbol_chats, &client->decryptbuf[0x08], 1248);
		break;
	case 0x03:
		// Shortcuts
		memcpy (&client->character.shortcuts, &client->decryptbuf[0x08], 2624);
		break;
	case 0x04:
		// Global Key Config
		memcpy (&client->character.keyConfigGlobal, &client->decryptbuf[0x08], 364);
		break;
	case 0x05:
		// Global Joystick Config
		memcpy (&client->character.joyConfigGlobal, &client->decryptbuf[0x08], 56);
		break;
	case 0x06:
		// Technique Config
		memcpy (&client->character.techConfig, &client->decryptbuf[0x08], 40);
		break;
	case 0x07:
		// Character Key Config
		memcpy (&client->character.keyConfig, &client->decryptbuf[0x08], 232);
		break;
	case 0x08:
		// C-Rank and Battle Config
		//memcpy (&client->character.challengeData, &client->decryptbuf[0x08], 320);
		break;
	}
}

void Command40(BANANA* client, ORANGE* ship)
{
	// Guild Card Search

	ship->encryptbuf[0x00] = 0x08;
	ship->encryptbuf[0x01] = 0x01;
	*(unsigned *) &ship->encryptbuf[0x02] = *(unsigned *) &client->decryptbuf[0x10];
	*(unsigned *) &ship->encryptbuf[0x06] = *(unsigned *) &client->guildcard;
	*(unsigned *) &ship->encryptbuf[0x0A] = serverID;
	*(unsigned *) &ship->encryptbuf[0x0E] = client->character.teamID;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x21);
}

void Command09(BANANA* client)
{
	QUEST* q;
	BANANA* c;
	LOBBY* l;
	unsigned lobbyNum, Packet11_Length, ch;
	char lb[10];
	int num_hours, num_minutes;

	switch ( client->decryptbuf[0x0F] )
	{
	case 0x00:
		// Team info
		if ( client->lobbyNum < 0x10 )
		{
			if ((!client->block) || (client->block > serverBlocks))
			{
				initialize_connection (client);
				return;
			}

			lobbyNum = *(unsigned *) &client->decryptbuf[0x0C];

			if ((lobbyNum < 0x10) || (lobbyNum >= 16+SHIP_COMPILED_MAX_GAMES))
			{
				initialize_connection (client);
				return;
			}

			l = &blocks[client->block - 1]->lobbies[lobbyNum];
			memset (&PacketData, 0, 0x10);
			PacketData[0x02] = 0x11;
			PacketData[0x0A] = 0x20;
			PacketData[0x0C] = 0x20;
			PacketData[0x0E] = 0x20;
			if (l->in_use)
			{
				Packet11_Length = 0x10;
				if ((client->team_info_request != lobbyNum) || (client->team_info_flag == 0))
				{
					client->team_info_request = lobbyNum;
					client->team_info_flag = 1;
					for (ch=0;ch<4;ch++)
						if ((l->slot_use[ch]) && (l->client[ch]))
						{
							c = l->client[ch];
							wstrcpy((unsigned short*) &PacketData[Packet11_Length], (unsigned short*) &c->character.name[0]);
							Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x4C;
							PacketData[Packet11_Length++] = 0x00;
							_itoa (l->client[ch]->character.level + 1, &lb[0], 10);
							wstrcpy_char(&PacketData[Packet11_Length], &lb[0]);
							Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x0A;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							switch (c->character._class)
							{
							case CLASS_HUMAR:
								wstrcpy_char (&PacketData[Packet11_Length], "HUmar");
								break;
							case CLASS_HUNEWEARL:
								wstrcpy_char (&PacketData[Packet11_Length], "HUnewearl");
								break;
							case CLASS_HUCAST:
								wstrcpy_char (&PacketData[Packet11_Length], "HUcast");
								break;
							case CLASS_HUCASEAL:
								wstrcpy_char (&PacketData[Packet11_Length], "HUcaseal");
								break;
							case CLASS_RAMAR:
								wstrcpy_char (&PacketData[Packet11_Length], "RAmar");
								break;
							case CLASS_RACAST:
								wstrcpy_char (&PacketData[Packet11_Length], "RAcast");
								break;
							case CLASS_RACASEAL:
								wstrcpy_char (&PacketData[Packet11_Length], "RAcaseal");
								break;
							case CLASS_RAMARL:
								wstrcpy_char (&PacketData[Packet11_Length], "RAmarl");
								break;
							case CLASS_FONEWM:
								wstrcpy_char (&PacketData[Packet11_Length], "FOnewm");
								break;
							case CLASS_FONEWEARL:
								wstrcpy_char (&PacketData[Packet11_Length], "FOnewearl");
								break;
							case CLASS_FOMARL:
								wstrcpy_char (&PacketData[Packet11_Length], "FOmarl");
								break;
							case CLASS_FOMAR:
								wstrcpy_char (&PacketData[Packet11_Length], "FOmar");
								break;
							default:
								wstrcpy_char (&PacketData[Packet11_Length], "Unknown");
								break;
							}
							Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x4A;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x0A;
							PacketData[Packet11_Length++] = 0x00;
						}
				}
				else
				{
					client->team_info_request = lobbyNum;
					client->team_info_flag = 0;
					wstrcpy_char (&PacketData[Packet11_Length], "Time : ");
					Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
					num_minutes  = ((unsigned) servertime - l->start_time ) / 60L;
					num_hours    = num_minutes / 60L;
					num_minutes %= 60;
					_itoa (num_hours,&lb[0], 10);
					wstrcpy_char (&PacketData[Packet11_Length], &lb[0]);
					Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
					PacketData[Packet11_Length++] = 0x3A;
					PacketData[Packet11_Length++] = 0x00;
					_itoa (num_minutes,&lb[0], 10);
					if (num_minutes < 10)
					{
						lb[1] = lb[0];
						lb[0] = 0x30;
						lb[2] = 0x00;
					}
					wstrcpy_char (&PacketData[Packet11_Length], &lb[0]);
					Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);					
					PacketData[Packet11_Length++] = 0x0A;
					PacketData[Packet11_Length++] = 0x00;
					if (l->quest_loaded)
					{
						wstrcpy_char (&PacketData[Packet11_Length], "Quest : ");
						Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
						PacketData[Packet11_Length++] = 0x0A;
						PacketData[Packet11_Length++] = 0x00;
						PacketData[Packet11_Length++] = 0x20;
						PacketData[Packet11_Length++] = 0x00;
						PacketData[Packet11_Length++] = 0x20;
						PacketData[Packet11_Length++] = 0x00;
						q = &quests[l->quest_loaded - 1];
						if ((client->character.lang < 10) && (q->ql[client->character.lang]))
							wstrcpy((unsigned short*) &PacketData[Packet11_Length], (unsigned short*) &q->ql[client->character.lang]->qname[0]);
						else
							wstrcpy((unsigned short*) &PacketData[Packet11_Length], (unsigned short*) &q->ql[0]->qname[0]);
						Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
					}
				}
			}
			else
			{
				wstrcpy_char (&PacketData[0x10], "Game no longer active.");
				Packet11_Length = 0x10 + (strlen ("Game no longer active.") * 2);
			}
			PacketData[Packet11_Length++] = 0x00;
			PacketData[Packet11_Length++] = 0x00;
			*(unsigned short*) &PacketData[0x00] = (unsigned short) Packet11_Length;
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &PacketData[0], Packet11_Length );
		}
		break;
	case 0x0F:
		// Quest info
		if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0B] <= numQuests ) )
		{
			q = &quests[client->decryptbuf[0x0B] - 1];
			memset (&PacketData[0x00], 0, 8);
			PacketData[0x00] = 0x48;
			PacketData[0x01] = 0x02;
			PacketData[0x02] = 0xA3;
			if ((client->character.lang < 10) && (q->ql[client->character.lang]))
				memcpy (&PacketData[0x08], &q->ql[client->character.lang]->qdetails[0], 0x200 );
			else
				memcpy (&PacketData[0x08], &q->ql[0]->qdetails[0], 0x200 );
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &PacketData[0], 0x248 );
		}
		break;
	default:
		break;
	}
}

void Command10(unsigned blockServer, BANANA* client)
{
	unsigned char select_type, selected;
	unsigned full_select, ch, ch2, failed_to_join, lobbyNum, password_match, oldIndex;
	LOBBY* l;
	unsigned short* p;
	unsigned short* c;
	unsigned short fqs, barule;
	unsigned qm_length, qa, nr;
	unsigned char* qmap;
	QUEST* q;
	char quest_num[16];
	unsigned qn;
	int do_quest;
	unsigned quest_flag;

	if (client->guildcard)
	{
		select_type = (unsigned char) client->decryptbuf[0x0F];
		selected = (unsigned char) client->decryptbuf[0x0C];
		full_select = *(unsigned *) &client->decryptbuf[0x0C];

		switch (select_type)
		{
		case 0x00:
			if ( ( blockServer ) && ( client->lobbyNum < 0x10 ) )
			{
				// Team

				if ((!client->block) || (client->block > serverBlocks))
				{
					initialize_connection (client);
					return;
				}

				lobbyNum = *(unsigned *) &client->decryptbuf[0x0C];

				if ((lobbyNum < 0x10) || (lobbyNum >= 16+SHIP_COMPILED_MAX_GAMES))
				{
					initialize_connection (client);
					return;
				}

				failed_to_join = 0;
				l = &blocks[client->block - 1]->lobbies[lobbyNum];

				if ((!client->isgm) && (!isLocalGM(client->guildcard)))
				{
					switch (l->episode)
					{
					case 0x01:
						if ((l->difficulty == 0x01) && (client->character.level < 19))
						{
							Send01 ("Episode I\n\nYou must be level\n20 or higher\nto play on the\nhard difficulty.", client);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 49))
							{
								Send01 ("Episode I\n\nYou must be level\n50 or higher\nto play on the\nvery hard\ndifficulty.", client);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 89))
								{
									Send01 ("Episode I\n\nYou must be level\n90 or higher\nto play on the\nultimate\ndifficulty.", client);
									failed_to_join = 1;
								}
								break;
					case 0x02:
						if ((l->difficulty == 0x01) && (client->character.level < 29))
						{
							Send01 ("Episode II\n\nYou must be level\n30 or higher\nto play on the\nhard difficulty.", client);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 59))
							{
								Send01 ("Episode II\n\nYou must be level\n60 or higher\nto play on the\nvery hard\ndifficulty.", client);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 99))
								{
									Send01 ("Episode II\n\nYou must be level\n100 or higher\nto play on the\nultimate\ndifficulty.", client);
									failed_to_join = 1;
								}
								break;
					case 0x03:
						if ((l->difficulty == 0x01) && (client->character.level < 39))
						{
							Send01 ("Episode IV\n\nYou must be level\n40 or higher\nto play on the\nhard difficulty.", client);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 69))
							{
								Send01 ("Episode IV\n\nYou must be level\n70 or higher\nto play on the\nvery hard\ndifficulty.", client);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 109))
								{
									Send01 ("Episode IV\n\nYou must be level\n110 or higher\nto play on the\nultimate\ndifficulty.", client);
									failed_to_join = 1;
								}
								break;
					}
				}


				if ((!l->in_use) && (!failed_to_join))
				{
					Send01 ("Game no longer active.", client);
					failed_to_join = 1;
				}

				if ((l->lobbyCount == 4)  && (!failed_to_join))
				{
					Send01 ("Game is full", client);
					failed_to_join = 1;
				}

				if ((l->quest_in_progress) && (!failed_to_join))
				{
					Send01 ("Quest already in progress.", client);
					failed_to_join = 1;
				}

				if ((l->oneperson) && (!failed_to_join))
				{
					Send01 ("Cannot join a one\nperson game.", client);
					failed_to_join = 1;
				}

				if (((l->gamePassword[0x00] != 0x00) || (l->gamePassword[0x01] != 0x00)) &&
					(!failed_to_join))
				{
					password_match = 1;
					p = (unsigned short*) &l->gamePassword[0x00];
					c = (unsigned short*) &client->decryptbuf[0x10];
					while (*p != 0x00)
					{
						if (*p != *c)
							password_match = 0;
						p++;
						c++;
					}
					if ((password_match == 0) && (client->isgm == 0) && (isLocalGM(client->guildcard) == 0))
					{
						Send01 ("Incorrect password.", client);
						failed_to_join = 1;
					}
				}

				if (!failed_to_join)
				{
					for (ch=0;ch<4;ch++)
					{
						if ((l->slot_use[ch]) && (l->client[ch]))
						{
							if (l->client[ch]->bursting == 1)
							{
								Send01 ("Player is bursting.\nPlease wait a\nmoment.", client);
								failed_to_join = 1;
							}
							else
								if ((l->inpquest) && (!l->client[ch]->hasquest))
								{
									Send01 ("Player is loading\nquest.\nPlease wait a\nmoment.", client);
									failed_to_join = 1;
								}
						}
					}
				}

				if ((l->inpquest) && (!failed_to_join))
				{
					// Check if player qualifies to join Government quest...
					q = &quests[l->quest_loaded - 1];
					memcpy (&dp[0], &q->ql[0]->qdata[0x31], 3);
					dp[4] = 0;
					qn = (unsigned) atoi ( &dp[0] );
					switch (l->episode)
					{
					case 0x01:
						qn -= 401;
						qn <<= 1;
						qn += 0x1F3;
						for (ch2=0x1F5;ch2<=qn;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					case 0x02:
						qn -= 451;
						qn <<= 1;
						qn += 0x211;
						for (ch2=0x213;ch2<=qn;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					case 0x03:
						qn -= 701;
						qn += 0x2BC;
						for (ch2=0x2BD;ch2<=qn;ch2++)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					}
					if (failed_to_join)
					{
						if ((client->isgm == 0) && (isLocalGM(client->guildcard) == 0))
							Send01 ("You must progress\nfurther in the\ngame before you\ncan join this\nquest.", client);
						else
							failed_to_join = 0;
					}
				}

				if (failed_to_join == 0)
				{
					removeClientFromLobby (client);
					client->lobbyNum = lobbyNum + 1;
					client->lobby = (void*) l;
					Send64 (client);
					memset (&client->encryptbuf[0x00], 0, 0x0C);
					client->encryptbuf[0x00] = 0x0C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xDD;
					client->encryptbuf[0x09] = 0x03;
					client->encryptbuf[0x0A] = (unsigned char) EXPERIENCE_RATE;
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
					UpdateGameItem (client);
				}
			}
			break;
		case 0x0F:
			// Quest selection
			if ( ( blockServer ) && ( client->lobbyNum > 0x0F ) )
			{
				if (!client->lobby)
					break;

				l = (LOBBY*) client->lobby;

				if ( client->decryptbuf[0x0B] == 0 )
				{
					if ( client->decryptbuf[0x0C] < 11 )
						SendA2 ( l->episode, l->oneperson, client->decryptbuf[0x0C], client->decryptbuf[0x0A], client );
				}
				else
				{
					if ( l->leader == client->clientID )
					{
						if ( l->quest_loaded == 0 )
						{
							if ( client->decryptbuf[0x0B] <= numQuests )
							{
								q = &quests[client->decryptbuf[0x0B] - 1];

								do_quest = 1;

								// Check "One-Person" quest ability to repeat...

								if ( ( l->oneperson ) && ( l->episode == 0x01 ) )
								{
									memcpy (&quest_num[0], &q->ql[0]->qdata[49], 3);
									quest_num[4] = 0;
									qn = atoi (&quest_num[0]);
									quest_flag = 0x63 + (qn << 1);
									if (qflag(&client->character.quest_data1[0], quest_flag, l->difficulty))
									{
										if (!qflag_ep1solo(&client->character.quest_data1[0], l->difficulty))
											do_quest = 0;
									}
									if ( !do_quest )
										Send01 ("Please clear\nthe remaining\nquests before\nredoing this one.", client);
								}

								// Check party Government quest qualification.  (Teamwork?!)

								if ( client->decryptbuf[0x0A] )
								{
									memcpy (&dp[0], &q->ql[0]->qdata[0x31], 3);
									dp[4] = 0;
									qn = (unsigned) atoi ( &dp[0] );
									switch (l->episode)
									{
									case 0x01:
										qn -= 401;
										qn <<= 1;
										qn += 0x1F3;
										for (ch2=0x1F5;ch2<=qn;ch2+=2)
											for (ch=0;ch<4;ch++)
												if ((l->client[ch]) && (!qflag(&l->client[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									case 0x02:
										qn -= 451;
										qn <<= 1;
										qn += 0x211;
										for (ch2=0x213;ch2<=qn;ch2+=2)
											for (ch=0;ch<4;ch++)
												if ((l->client[ch]) && (!qflag(&l->client[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									case 0x03:
										qn -= 701;
										qn += 0x2BC;
										for (ch2=0x2BD;ch2<=qn;ch2++)
											for (ch=0;ch<4;ch++)
												if ((l->client[ch]) && (!qflag(&l->client[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									}
									if (!do_quest)
										Send01 ("The party no longer\nqualifies to\nstart this quest.", client);
								}

								if ( do_quest )
								{
									ch2 = 0;
									barule = 0;

									while (q->ql[0]->qname[ch2] != 0x00)
									{
										// Search for a number in the quest name to determine battle rule #
										if ((q->ql[0]->qname[ch2] >= 0x31) && (q->ql[0]->qname[ch2] <= 0x38))
										{
											barule = q->ql[0]->qname[ch2];
											break;
										}
										ch2++;
									}

									for (ch=0;ch<4;ch++)
										if ((l->slot_use[ch]) && (l->client[ch]))
										{
											if ((l->battle) || (l->challenge))
											{
												// Copy character to backup buffer.
												if (l->client[ch]->character_backup)
													free (l->client[ch]->character_backup);
												l->client[ch]->character_backup = malloc (sizeof (l->client[ch]->character));
												memcpy ( l->client[ch]->character_backup, &l->client[ch]->character, sizeof (l->client[ch]->character));

												l->battle_level = 0;

												switch ( barule )
												{
												case 0x31:
													// Rule #1
													l->client[ch]->mode = 1;
													break;
												case 0x32:
													// Rule #2
													l->battle_level = 1;
													l->client[ch]->mode = 3;
													break;
												case 0x33:
													// Rule #3
													l->battle_level = 5;
													l->client[ch]->mode = 3;
													break;
												case 0x34:
													// Rule #4
													l->battle_level = 2;
													l->client[ch]->mode = 3;
													l->meseta_boost = 1;
													break;
												case 0x35:
													// Rule #5
													l->client[ch]->mode = 2;
													l->meseta_boost = 1;
													break;
												case 0x36:
													// Rule #6
													l->battle_level = 20;
													l->client[ch]->mode = 3;
													break;
												case 0x37:
													// Rule #7
													l->battle_level = 1;
													l->client[ch]->mode = 3;
													break;
												case 0x38:
													// Rule #8
													l->battle_level = 20;
													l->client[ch]->mode = 3;
													break;
												default:
													WriteLog ("Unknown battle rule loaded...");
													l->client[ch]->mode = 1;
													break;
												}

												switch (l->client[ch]->mode)
												{
												case 0x02:
													// Delete all mags and meseta...
													for (ch2=0;ch2<l->client[ch]->character.inventoryUse;ch2++)
													{
														if (l->client[ch]->character.inventory[ch2].item.data[0] == 0x02)
															l->client[ch]->character.inventory[ch2].in_use = 0;
													}
													CleanUpInventory (l->client[ch]);
													l->client[ch]->character.meseta = 0;
													break;
												case 0x03:
													// Wipe items and reset level.
													for (ch2=0;ch2<30;ch2++)
														l->client[ch]->character.inventory[ch2].in_use = 0;
													CleanUpInventory (l->client[ch]);
													l->client[ch]->character.level = 0;
													l->client[ch]->character.XP = 0;
													l->client[ch]->character.ATP = *(unsigned short*) &startingData[(l->client[ch]->character._class*14)];
													l->client[ch]->character.MST = *(unsigned short*) &startingData[(l->client[ch]->character._class*14)+2];
													l->client[ch]->character.EVP = *(unsigned short*) &startingData[(l->client[ch]->character._class*14)+4];
													l->client[ch]->character.HP  = *(unsigned short*) &startingData[(l->client[ch]->character._class*14)+6];
													l->client[ch]->character.DFP = *(unsigned short*) &startingData[(l->client[ch]->character._class*14)+8];
													l->client[ch]->character.ATA = *(unsigned short*) &startingData[(l->client[ch]->character._class*14)+10];
													if (l->battle_level > 1)
														SkipToLevel (l->battle_level - 1, l->client[ch], 1);
													l->client[ch]->character.meseta = 0;
												}
											}

											if ((l->client[ch]->character.lang < 10) &&
												(q->ql[l->client[ch]->character.lang]))
											{
												fqs = *(unsigned short*) &q->ql[l->client[ch]->character.lang]->qdata[0];
												if (fqs % 8)
													fqs += ( 8 - ( fqs % 8 ) );
												cipher_ptr = &l->client[ch]->server_cipher;
												encryptcopy (l->client[ch], &q->ql[l->client[ch]->character.lang]->qdata[0], fqs);
											}
											else
											{
												fqs = *(unsigned short*) &q->ql[0]->qdata[0];
												if (fqs % 8)
													fqs += ( 8 - ( fqs % 8 ) );
												cipher_ptr = &l->client[ch]->server_cipher;
												encryptcopy (l->client[ch], &q->ql[0]->qdata[0], fqs);
											}
											l->client[ch]->bursting = 1;
											l->client[ch]->sending_quest = client->decryptbuf[0x0B] - 1;
											l->client[ch]->qpos = fqs;
										}
										if (!client->decryptbuf[0x0A])
											l->quest_in_progress = 1; // when a government quest, this won't be set
										else
											l->inpquest = 1;

										l->quest_loaded = client->decryptbuf[0x0B];

										// Time to load the map data...

										memset ( &l->mapData[0], 0, 0xB50 * sizeof (MAP_MONSTER) ); // Erase!
										l->mapIndex = 0;
										l->rareIndex = 0;
										for (ch=0;ch<0x20;ch++)
											l->rareData[ch] = 0xFF;

										qmap = q->mapdata;
										qm_length = *(unsigned*) qmap;
										qmap += 4;
										ch = 4;
										while ( ( qm_length - ch ) >= 80 )
										{
											oldIndex = l->mapIndex;
											qa = *(unsigned*) qmap; // Area
											qmap += 4;
											nr = *(unsigned*) qmap; // Number of monsters
											qmap += 4;
											if ( ( l->episode == 0x03 ) && ( qa > 5 ) )
												ParseMapData ( l, (MAP_MONSTER*) qmap, 1, nr );
											else
												if ( ( l->episode == 0x02 ) && ( qa > 15 ) )
													ParseMapData ( l, (MAP_MONSTER*) qmap, 1, nr );
												else
													ParseMapData ( l, (MAP_MONSTER*) qmap, 0, nr );
											qmap += ( nr * 72 );
											ch += ( ( nr * 72 ) + 8 );
											//debug ("loaded quest area %u, mid count %u, total mids: %u", qa, l->mapIndex - oldIndex, l->mapIndex);
										}
								}
							}
						}
						else
							Send01 ("Quest already loaded.", client);
					}
					else
						Send01 ("Only the leader of a team can start quests.", client);
				}
			}
			break;
		case 0xEF:
			if ( client->lobbyNum < 0x10 )
			{
				// Blocks

				unsigned blockNum;

				blockNum = 0x100 - selected;

				if (blockNum <= serverBlocks)
				{
					if ( blocks[blockNum - 1]->count < 180 )
					{
						if ((client->lobbyNum) && (client->lobbyNum < 0x10))
						{
							for (ch=0;ch<MAX_SAVED_LOBBIES;ch++)
							{
								if (savedlobbies[ch].guildcard == 0)
								{
									savedlobbies[ch].guildcard = client->guildcard;
									savedlobbies[ch].lobby = client->lobbyNum;
									break;
								}
							}
						}

						if (client->gotchardata)
						{
							client->character.playTime += (unsigned) servertime - client->connected; 
							ShipSend04 (0x02, client, logon);
							client->gotchardata = 0;
							client->released = 1;
							*(unsigned *) &client->releaseIP[0] = *(unsigned *) &serverIP[0];
							client->releasePort = serverPort + blockNum;
						}
						else
							Send19 (serverIP[0], serverIP[1], serverIP[2], serverIP[3],
							serverPort + blockNum, client);
					}
					else
					{
						Send01 ("Block is full.", client);
						Send07 (client);
					}
				}
			}
			break;
		case 0xFF:
			if ( client->lobbyNum < 0x10 )
			{
				// Ship select
				if ( selected == 0x00 )
					ShipSend0D (0x00, client, logon);
				else
					// Ships
					for (ch=0;ch<totalShips;ch++)
					{
						if (full_select == shipdata[ch].shipID)
						{
							if (client->gotchardata)
							{
								client->character.playTime += (unsigned) servertime - client->connected;
								ShipSend04 (0x02, client, logon);
								client->gotchardata = 0;
								client->released = 1;
								*(unsigned *) &client->releaseIP[0] = *(unsigned*) &shipdata[ch].ipaddr[0];
								client->releasePort = shipdata[ch].port;
							}
							else
								Send19 (shipdata[ch].ipaddr[0], shipdata[ch].ipaddr[1], 
								shipdata[ch].ipaddr[2], shipdata[ch].ipaddr[3],
								shipdata[ch].port, client);

							break;
						}
					}
			}
			break;
		default:
			break;
		}
	}
}

void CommandD9 (BANANA* client)
{
	unsigned short *n;
	unsigned short *g;
	unsigned short s = 2;


	// Client writing to info board

	n = (unsigned short*) &client->decryptbuf[0x0A];
	g = (unsigned short*) &client->character.GCBoard[0];

	*(g++) = 0x0009;

	while ((*n != 0x0000) && (s < 85))
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*(g++) = 0x0020;
		else
			*(g++) = *n;
		n++;
		s++;
	}
	// null terminate
	*(g++) = 0x0000;
}


void AddGuildCard (unsigned myGC, unsigned friendGC, unsigned char* friendName, 
				   unsigned char* friendText, unsigned char friendSecID, unsigned char friendClass,
				   ORANGE* ship)
{
	// Instruct the logon server to add the guild card

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned*) &ship->encryptbuf[0x02] = myGC;
	*(unsigned*) &ship->encryptbuf[0x06] = friendGC;
	memcpy (&ship->encryptbuf[0x0A], friendName, 24);
	memcpy (&ship->encryptbuf[0x22], friendText, 176);
	ship->encryptbuf[0xD2] = friendSecID;
	ship->encryptbuf[0xD3] = friendClass;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0xD4 );
}

void DeleteGuildCard (unsigned myGC, unsigned friendGC, ORANGE* ship)
{
	// Instruct the logon server to delete the guild card

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x01;
	*(unsigned*) &ship->encryptbuf[0x02] = myGC;
	*(unsigned*) &ship->encryptbuf[0x06] = friendGC;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}

void ModifyGuildCardComment (unsigned myGC, unsigned friendGC, unsigned short* n, ORANGE* ship)
{
	unsigned s = 1;
	unsigned short* g;

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x02;
	*(unsigned*) &ship->encryptbuf[0x02] = myGC;
	*(unsigned*) &ship->encryptbuf[0x06] = friendGC;

	// Client writing to info board

	g = (unsigned short*) &ship->encryptbuf[0x0A];

	memset (g, 0, 0x44);

	*(g++) = 0x0009;

	while ((*n != 0x0000) && (s < 33))
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*(g++) = 0x0020;
		else
			*(g++) = *n;
		n++;
		s++;
	}
	*(g++) = 0x0000;

	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x4E );
}

void SortGuildCard (BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x03;
	*(unsigned*) &ship->encryptbuf[0x02] = client->guildcard;
	*(unsigned*) &ship->encryptbuf[0x06] = *(unsigned*) &client->decryptbuf[0x08];
	*(unsigned*) &ship->encryptbuf[0x0A] = *(unsigned*) &client->decryptbuf[0x0C];
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x10 );
}


void CommandE8 (BANANA* client)
{
	unsigned gcn;


	switch (client->decryptbuf[0x03])
	{
	case 0x04:
		{
			// Accepting sent guild card
			LOBBY* l;
			BANANA* lClient;
			unsigned ch, maxch;

			if (!client->lobby)
				break;

			l = (LOBBY*) client->lobby;
			gcn = *(unsigned*) &client->decryptbuf[0x08];
			if ( client->lobbyNum < 0x10 )
				maxch = 12;
			else
				maxch = 4;
			for (ch=0;ch<maxch;ch++)
			{
				if ((l->client[ch]) && (l->client[ch]->character.guildCard == gcn))
				{
					lClient = l->client[ch];
					if (PreppedGuildCard(lClient->guildcard,client->guildcard))
					{
						AddGuildCard (client->guildcard, gcn, &client->decryptbuf[0x0C], &client->decryptbuf[0x5C],
							client->decryptbuf[0x10E], client->decryptbuf[0x10F], logon );
					}
					break;
				}
			}
		}
		break;
	case 0x05:
		// Deleting a guild card
		gcn = *(unsigned*) &client->decryptbuf[0x08];
		DeleteGuildCard (client->guildcard, gcn, logon);
		break;
	case 0x06:
		// Setting guild card text
		{
			unsigned short *n;
			unsigned short *g;
			unsigned short s = 2;

			// Client writing to info board

			n = (unsigned short*) &client->decryptbuf[0x5E];
			g = (unsigned short*) &client->character.guildcard_text[0];

			*(g++) = 0x0009;

			while ((*n != 0x0000) && (s < 85))
			{
				if ((*n == 0x0009) || (*n == 0x000A))
					*(g++) = 0x0020;
				else
					*(g++) = *n;
				n++;
				s++;
			}
			// null terminate
			*(g++) = 0x0000;
		}
		break;
	case 0x07:
		// Add blocked user
		// User @ 0x08, Name of User @ 0x0C
		break;
	case 0x08:
		// Remove blocked user
		// User @ 0x08
		break;
	case 0x09:
		// Write comment on user
		// E8 09 writing a comment on a user...  not sure were comment goes in the DC packet... 
		// User @ 0x08 comment @ 0x0C
		gcn = *(unsigned*) &client->decryptbuf[0x08];
		ModifyGuildCardComment (client->guildcard, gcn, (unsigned short*) &client->decryptbuf[0x0E], logon);
		break;
	case 0x0A:
		// Sort guild card
		// (Moves from one position to another)
		SortGuildCard (client, logon);
		break;
	}
}

void CommandD8 (BANANA* client)
{
	unsigned ch,maxch;
	unsigned short D8Offset;
	unsigned char totalClients = 0;
	LOBBY* l;
	BANANA* lClient;

	if (!client->lobby)
		return;

	memset (&PacketData[0], 0, 8);

	PacketData[0x02] = 0xD8;
	D8Offset = 8;

	l = client->lobby;

	if (client->lobbyNum < 0x10)
		maxch = 12;
	else
		maxch = 4;

	for (ch=0;ch<maxch;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
		{
			totalClients++;
			lClient = l->client[ch];
			memcpy (&PacketData[D8Offset], &lClient->character.name[4], 20 );
			D8Offset += 0x20;
			memcpy (&PacketData[D8Offset], &lClient->character.GCBoard[0], 172 );
			D8Offset += 0x158;
		}
	}
	PacketData[0x04] = totalClients;
	*(unsigned short*) &PacketData[0x00] = (unsigned short) D8Offset;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], D8Offset);
}

void Command81 (BANANA* client, ORANGE* ship)
{
	unsigned short* n;

	ship->encryptbuf[0x00] = 0x08;
	ship->encryptbuf[0x01] = 0x03;
	memcpy (&ship->encryptbuf[0x02], &client->decryptbuf[0x00], 0x45C);
	*(unsigned*) &ship->encryptbuf[0x0E] = client->guildcard;
	memcpy (&ship->encryptbuf[0x12], &client->character.name[0], 24);
	n = (unsigned short*) &ship->encryptbuf[0x62];
	while (*n != 0x0000)
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*n = 0x0020;
		n++;
	}
	*n = 0x0000;
	*(unsigned*) &ship->encryptbuf[0x45E] = client->character.teamID;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x462 );
}


void CreateTeam (unsigned short* teamname, unsigned guildcard, ORANGE* ship)
{
	unsigned short *g;
	unsigned n;

	n = 0;

	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x00;

	g = (unsigned short*) &ship->encryptbuf[0x02];

	memset (g, 0, 24);
	while ((*teamname != 0x0000) && (n<11))
	{
		if ((*teamname != 0x0009) && (*teamname != 0x000A))
			*(g++) = *teamname;
		else
			*(g++) = 0x0020;
		teamname++;
		n++;
	}
	*(unsigned*) &ship->encryptbuf[0x1A] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x1E );
}

void UpdateTeamFlag (unsigned char* flag, unsigned teamid, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x01;
	memcpy (&ship->encryptbuf[0x02], flag, 0x800);
	*(unsigned*) &ship->encryptbuf[0x802] = teamid;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x806 );
}

void DissolveTeam (unsigned teamid, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x02;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x06 );
}

void RemoveTeamMember ( unsigned teamid, unsigned guildcard, ORANGE* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x03;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}

void TeamChat ( unsigned short* text, unsigned short chatsize, unsigned teamid, ORANGE* ship )
{
	unsigned size;

	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x04;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	while (chatsize % 8)
		ship->encryptbuf[6 + (chatsize++)] = 0x00;
	*text = chatsize;;
	memcpy (&ship->encryptbuf[0x06], text, chatsize);
	size = chatsize + 6;;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], size );
}

void RequestTeamList ( unsigned teamid, unsigned guildcard, ORANGE* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x05;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}

void PromoteTeamMember ( unsigned teamid, unsigned guildcard, unsigned char newlevel, ORANGE* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x06;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	(unsigned char) ship->encryptbuf[0x0A] = newlevel;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0B );
}

void AddTeamMember ( unsigned teamid, unsigned guildcard, ORANGE* ship )
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x07;
	*(unsigned*) &ship->encryptbuf[0x02] = teamid;
	*(unsigned*) &ship->encryptbuf[0x06] = guildcard;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x0A );
}


void CommandEA (BANANA* client, ORANGE* ship)
{
	unsigned connectNum;

	if ((client->decryptbuf[0x03] < 32) && ((unsigned) servertime - client->team_cooldown[client->decryptbuf[0x03]] >= 1))
	{
		client->team_cooldown[client->decryptbuf[0x03]] = (unsigned) servertime;
		switch (client->decryptbuf[0x03])
		{
		case 0x01:
			// Create team
			if (client->character.teamID == 0)
				CreateTeam ((unsigned short*) &client->decryptbuf[0x0C], client->guildcard, ship);
			break;
		case 0x03:
			// Add a team member
			{
				BANANA* tClient;
				unsigned gcn, ch;

				if ((client->character.teamID != 0) && (client->character.privilegeLevel >= 0x30))
				{
					gcn = *(unsigned*) &client->decryptbuf[0x08];
					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gcn)
						{
							if ( ( connections[connectNum]->character.teamID == 0 ) && ( connections[connectNum]->teamaccept == 1 ) )
							{
								AddTeamMember ( client->character.teamID, gcn, ship );
								tClient = connections[connectNum];
								tClient->teamaccept = 0;
								memset ( &tClient->character.guildCard2, 0, 2108 );
								tClient->character.teamID = client->character.teamID;
								tClient->character.privilegeLevel = 0;
								tClient->character.unknown15 = client->character.unknown15;
								memcpy ( &tClient->character.teamName[0], &client->character.teamName[0], 28 );
								memcpy ( &tClient->character.teamFlag[0], &client->character.teamFlag[0], 2048 );
								*(long long*) &tClient->character.teamRewards[0] = *(long long*) &client->character.teamRewards[0];
								if ( tClient->lobbyNum < 0x10 )
									SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
								else
									SendToLobby ( tClient->lobby, 4, MakePacketEA15 ( tClient ), 2152, 0 );
								SendEA ( 0x12, tClient );
								SendEA ( 0x04, client );
								SendEA ( 0x04, tClient );
								break;
							}
							else
								Send01 ("Player already\nbelongs to a team!", client);
						}
					}
				}
			}
			break;
		case 0x05:
			// Remove member from team
			if (client->character.teamID != 0)
			{
				unsigned gcn,ch;
				BANANA* tClient;

				gcn = *(unsigned*) &client->decryptbuf[0x08];

				if (gcn != client->guildcard)
				{
					if (client->character.privilegeLevel == 0x40)
					{
						RemoveTeamMember (client->character.teamID, gcn, ship);
						SendEA ( 0x06, client );
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gcn)
							{
								tClient = connections[connectNum];
								if ( tClient->character.privilegeLevel < client->character.privilegeLevel )
								{
									memset (&tClient->character.guildCard2, 0, 2108);
									memset (&client->encryptbuf[0x00], 0, 0x40);
									client->encryptbuf[0x00] = 0x40;
									client->encryptbuf[0x02] = 0xEA;
									client->encryptbuf[0x03] = 0x12;
									*(unsigned *) &client->encryptbuf[0x0C] = tClient->guildcard;
									if ( tClient->lobbyNum < 0x10 )
									{
										SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
										SendToLobby ( tClient->lobby, 12, &client->encryptbuf[0x00], 0x40, 0 );
									}
									else
									{
										SendToLobby ( tClient->lobby, 4, MakePacketEA15 ( tClient ), 2152, 0 );
										SendToLobby ( tClient->lobby, 4, &client->encryptbuf[0x00], 0x40, 0 );
									}
									Send01 ("Member removed.", client);
								}
								else
									Send01 ("Your privilege level is\ntoo low.", client);
								break;
							}
						}
					}
					else
						Send01 ("Your privilege level is\ntoo low.", client);
				}
				else
				{
					RemoveTeamMember ( client->character.teamID, gcn, ship );
					memset (&client->character.guildCard2, 0, 2108);
					memset (&client->encryptbuf[0x00], 0, 0x40);
					client->encryptbuf[0x00] = 0x40;
					client->encryptbuf[0x02] = 0xEA;
					client->encryptbuf[0x03] = 0x12;
					*(unsigned *) &client->encryptbuf[0x0C] = client->guildcard;
					if ( client->lobbyNum < 0x10 )
					{
						SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
						SendToLobby ( client->lobby, 12, &client->encryptbuf[0x00], 0x40, 0 );
					}
					else
					{
						SendToLobby ( client->lobby, 4, MakePacketEA15 ( client ), 2152, 0 );
						SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x40, 0 );
					}
				}
			}
			break;
		case 0x07:
			if (client->character.teamID != 0)
			{
				unsigned short size;
				unsigned short *n;

				size = *(unsigned short*) &client->decryptbuf[0x00];

				if (size > 0x2B)
				{
					n = (unsigned short*) &client->decryptbuf[0x2C];
					while (*n != 0x0000)
					{
						if ((*n == 0x0009) || (*n == 0x000A))
							*n = 0x0020;
						n++;
					}
					TeamChat ((unsigned short*) &client->decryptbuf[0x00], size, client->character.teamID, ship);
				}
			}
			break;
		case 0x08:
			// Member Promotion / Demotion / Expulsion / Master Transfer
			//
			if (client->character.teamID != 0)
				RequestTeamList (client->character.teamID, client->guildcard, ship);
			break;
		case 0x0D:
			SendEA (0x0E, client);
			break;
		case 0x0F:
			// Set flag
			if ((client->character.privilegeLevel == 0x40) && (client->character.teamID != 0))
				UpdateTeamFlag (&client->decryptbuf[0x08], client->character.teamID, ship);
			break;
		case 0x10:
			// Dissolve team
			if ((client->character.privilegeLevel == 0x40) && (client->character.teamID != 0))
			{
				DissolveTeam (client->character.teamID, ship);
				SendEA ( 0x10, client );
				memset ( &client->character.guildCard2, 0, 2108 );
				SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
				SendEA ( 0x12, client );
			}
			break;
		case 0x11:
			// Promote member
			if (client->character.teamID != 0)
			{
				unsigned gcn, ch;
				BANANA* tClient;

				gcn = *(unsigned*) &client->decryptbuf[0x08];

				if (gcn != client->guildcard)
				{
					if (client->character.privilegeLevel == 0x40)
					{
						PromoteTeamMember (client->character.teamID, gcn, client->decryptbuf[0x04], ship);

						if (client->decryptbuf[0x04] == 0x40)
						{
							// Master Transfer
							PromoteTeamMember (client->character.teamID, client->guildcard, 0x30, ship);
							client->character.privilegeLevel = 0x30;
							SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
						}

						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gcn)
							{
								tClient = connections[connectNum];
								if (tClient->character.privilegeLevel != client->decryptbuf[0x04]) // only if changed
								{
									tClient->character.privilegeLevel = client->decryptbuf[0x04];
									if ( tClient->lobbyNum < 0x10 )
										SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
									else
										SendToLobby ( tClient->lobby, 4, MakePacketEA15 ( tClient ), 2152, 0 );
								}
								SendEA ( 0x12, tClient );
								SendEA ( 0x11, client );
								break;
							}
						}
					}
				}
			}
			break;
		case 0x13:
			// A type of lobby list...
			SendEA (0x13, client);
			break;
		case 0x14:
			// Do nothing.
			break;
		case 0x18:
			// Buying privileges and point information
			SendEA (0x18, client);
			break;
		case 0x19:
			// Privilege list
			SendEA (0x19, client);
			break;
		case 0x1C:
			// Ranking
			Send1A ("Tethealla Ship Server coded by Sodaboy\nhttp://www.pioneer2.net/\n\nEnjoy!", client );
			break;
		case 0x1A:
			SendEA (0x1A, client);
			break;
		default:
			break;
		}
	}
}


void ShowArrows (BANANA* client, int to_all)
{
	LOBBY *l;
	unsigned ch, total_clients, Packet88Offset;

	total_clients = 0;
	memset (&PacketData[0x00], 0, 8);
	PacketData[0x02] = 0x88;
	PacketData[0x03] = 0x00;
	Packet88Offset = 8;

	if (!client->lobby)
		return;
	l = (LOBBY*) client->lobby;

	for (ch=0;ch<12;ch++)
	{
		if ((l->slot_use[ch] != 0) && (l->client[ch]))
		{
			total_clients++;
			PacketData[Packet88Offset+2] = 0x01;
			*(unsigned*) &PacketData[Packet88Offset+4] = l->client[ch]->character.guildCard;
			PacketData[Packet88Offset+8] = l->arrow_color[ch];
			Packet88Offset += 12;
		}
	}
	*(unsigned*) &PacketData[0x04] = total_clients;
	*(unsigned short*) &PacketData[0x00] = (unsigned short) Packet88Offset;
	if (to_all)
		SendToLobby (client->lobby, 12, &PacketData[0x00], Packet88Offset, 0);
	else
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketData[0x00], Packet88Offset);
	}
}

void BlockProcessPacket (BANANA* client)
{
	if (client->guildcard)
	{
		switch (client->decryptbuf[0x02])
		{
		case 0x05:
			break;
		case 0x06:
			if ( ((unsigned) servertime - client->command_cooldown[0x06]) >= 1 )
			{
				client->command_cooldown[0x06] = (unsigned) servertime;
				Send06 (client);
			}
			break;
		case 0x08:
			// Get game list
			if ( ( client->lobbyNum < 0x10 ) && ( ((unsigned) servertime - client->command_cooldown[0x08]) >= 1 ) )
			{
				client->command_cooldown[0x08] = (unsigned) servertime;
				Send08 (client);
			}
			else
				if ( client->lobbyNum < 0x10 )
					Send01 ("You must wait\nawhile before\ntrying that.", client);
			break;
		case 0x09:
			Command09 (client);
			break;
		case 0x10:
			Command10 (1, client);
			break;
		case 0x1D:
			client->response = (unsigned) servertime;
			break;
		case 0x40:
			// Guild Card search
			if ( ((unsigned) servertime - client->command_cooldown[0x40]) >= 1 )
			{
				client->command_cooldown[0x40] = (unsigned) servertime;
				Command40 (client, logon);
			}
			break;
		case 0x13:
		case 0x44:
			if ( ( client->lobbyNum > 0x0F ) && ( client->sending_quest != -1 ) )
			{
				unsigned short qps;

				if ((client->character.lang < 10) && (quests[client->sending_quest].ql[client->character.lang]))
				{
					if (client->qpos < quests[client->sending_quest].ql[client->character.lang]->qsize)
					{
						qps = *(unsigned short*) &quests[client->sending_quest].ql[client->character.lang]->qdata[client->qpos];
						if ( qps % 8 )
							qps += ( 8 - ( qps % 8 ) );
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &quests[client->sending_quest].ql[client->character.lang]->qdata[client->qpos], qps);
						client->qpos += qps;
					}
					else
						client->sending_quest = -1;
				}
				else
				{
					if (client->qpos < quests[client->sending_quest].ql[0]->qsize)
					{
						qps = *(unsigned short*) &quests[client->sending_quest].ql[0]->qdata[client->qpos];
						if ( qps % 8 )
							qps += ( 8 - ( qps % 8 ) );
						cipher_ptr = &client->server_cipher;
						encryptcopy (client, &quests[client->sending_quest].ql[0]->qdata[client->qpos], qps);
						client->qpos += qps;
					}
					else
						client->sending_quest = -1;
				}
			}
			break;
		case 0x60:
			if ((client->bursting) && (client->decryptbuf[0x08] == 0x23) && (client->lobbyNum < 0x10) && (client->lobbyNum))
			{
				// If a client has just appeared, send him the team information of everyone in the lobby
				if (client->guildcard)
				{
					unsigned ch;
					LOBBY* l;

					if (!client->lobby)
					break;

					l = client->lobby;

					for (ch=0;ch<12;ch++)
						if ( ( l->slot_use[ch] != 0 ) && ( l->client[ch] ) )
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, MakePacketEA15 (l->client[ch]), 2152 );
						}
						ShowArrows (client, 0);
						client->bursting = 0;
				}
			}
			// Lots of fun commands here.
			Send60 (client);
			break;
		case 0x61:
			memcpy (&client->character.unknown[0], &client->decryptbuf[0x362], 10);
			Send67 (client, client->preferred_lobby);
			break;
		case 0x62:
			Send62 (client);
			break;
		case 0x6D:
			if (client->lobbyNum > 0x0F)
				Send6D (client);
			else
				initialize_connection (client);
			break;
		case 0x6F:
			if ((client->lobbyNum > 0x0F) && (client->bursting))
			{
				LOBBY* l;
				unsigned short fqs,ch;

				if (!client->lobby)
					break;

				l = client->lobby;

				if ((l->inpquest) && (!client->hasquest))
				{
					// Send the quest
					client->bursting = 1;
					if ((client->character.lang < 10) && (quests[l->quest_loaded - 1].ql[client->character.lang]))
					{
						fqs =  *(unsigned short*) &quests[l->quest_loaded - 1].ql[client->character.lang]->qdata[0];
						if (fqs % 8)
							fqs += ( 8 - ( fqs % 8 ) );
						client->sending_quest = l->quest_loaded - 1;
						client->qpos = fqs;
						cipher_ptr = &client->server_cipher;
						encryptcopy ( client, &quests[l->quest_loaded - 1].ql[client->character.lang]->qdata[0], fqs);
					}
					else
					{
						fqs =  *(unsigned short*) &quests[l->quest_loaded - 1].ql[0]->qdata[0];
						if (fqs % 8)
							fqs += ( 8 - ( fqs % 8 ) );
						client->sending_quest = l->quest_loaded - 1;
						client->qpos = fqs;
						cipher_ptr = &client->server_cipher;
						encryptcopy ( client, &quests[l->quest_loaded - 1].ql[0]->qdata[0], fqs);
					}
				}
				else
				{
					// Rare monster data go...
					memset (&client->encryptbuf[0x00], 0, 0x08);
					client->encryptbuf[0x00] = 0x28;
					client->encryptbuf[0x02] = 0xDE;
					memcpy (&client->encryptbuf[0x08], &l->rareData[0], 0x20);
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x28);
					memset (&client->encryptbuf[0x00], 0, 0x0C);
					client->encryptbuf[0x00] = 0x0C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0x72;
					client->encryptbuf[0x09] = 0x03;
					client->encryptbuf[0x0A] = 0x18;
					client->encryptbuf[0x0B] = 0x08;
					SendToLobby (client->lobby, 4, &client->encryptbuf[0x00], 0x0C, 0);
					for (ch=0;ch<4;ch++)
						if ( ( l->slot_use[ch] != 0 ) && ( l->client[ch] ) )
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy (client, MakePacketEA15 (l->client[ch]), 2152 );
						}
					client->bursting = 0;
				}
			}
			break;
		case 0x81:
			if (client->announce)
			{
				if (client->announce == 1)
					WriteGM ("GM %u made an announcement: %s", client->guildcard, Unicode_to_ASCII ((unsigned short*) &client->decryptbuf[0x60]));
				BroadcastToAll ((unsigned short*) &client->decryptbuf[0x60], client);
			}
			else
			{
				if ( ((unsigned) servertime - client->command_cooldown[0x81]) >= 1 )
				{
					client->command_cooldown[0x81] = (unsigned) servertime;
					Command81 (client, logon);
				}
			}
			break;
		case 0x84:
			if (client->decryptbuf[0x0C] < 0x0F)
			{
				BLOCK* b;
				b = blocks[client->block - 1];
				if (client->lobbyNum > 0x0F)
				{
					removeClientFromLobby (client);
					client->preferred_lobby = client->decryptbuf[0x0C];
					Send95 (client);
				}
				else
				{
					if ( ((unsigned) servertime - client->command_cooldown[0x84]) >= 1 )
					{
						if (b->lobbies[client->decryptbuf[0x0C]].lobbyCount < 12)
						{
							client->command_cooldown[0x84] = (unsigned) servertime;
							removeClientFromLobby (client);
							client->preferred_lobby = client->decryptbuf[0x0C];
							Send95 (client);
						}
						else
							Send01 ("Lobby is full!", client);
					}
					else 
						Send01 ("You must wait\nawhile before\ntrying that.", client);
				}
			}
			break;
		case 0x89:
			if ((client->lobbyNum < 0x10) && (client->lobbyNum) && ( ((unsigned) servertime - client->command_cooldown[0x89]) >= 1 ) )
			{
				LOBBY* l;

				client->command_cooldown[0x89] = (unsigned) servertime;
				if (!client->lobby)
					break;
				l = client->lobby;
				l->arrow_color[client->clientID] = client->decryptbuf[0x04];
				ShowArrows(client, 1);
			}
			break;
		case 0x8A:
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;

				if (!client->lobby)
					break;
				l = client->lobby;
				if (l->in_use)
				{
					memset (&PacketData[0], 0, 0x28);
					PacketData[0x00] = 0x28;
					PacketData[0x02] = 0x8A;
					memcpy (&PacketData[0x08], &l->gameName[0], 30);
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &PacketData[0], 0x28);
				}
			}
			break;
		case 0xA0:
			// Ship list
			if (client->lobbyNum < 0x10)
				ShipSend0D (0x00, client, logon);
			break;
		case 0xA1:
			// Block list
			if (client->lobbyNum < 0x10)
				Send07 (client);
			break;
		case 0xA2:
			// Quest list
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;

				if (!client->lobby)
					break;
				l = client->lobby;

				if ( l->floor[client->clientID] == 0 )
				{
					if ( client->decryptbuf[0x04] )
						SendA2 ( l->episode, l->oneperson, 0, 1, client );
					else
						SendA2 ( l->episode, l->oneperson, 0, 0, client );
				}
			}
			break;
		case 0xAC:
			// Quest load complete
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;
				int all_quest;
				unsigned ch;

				if (!client->lobby)
					break;

				l = client->lobby;

				client->hasquest = 1;
				all_quest = 1;

				for (ch=0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]) && (l->client[ch]->hasquest == 0))
						all_quest = 0;
				}

				if (all_quest)
				{ 
					// Send the 0xAC when all clients have the quest

					client->decryptbuf[0x00] = 0x08;
					client->decryptbuf[0x01] = 0x00;
					SendToLobby (l, 4, &client->decryptbuf[0x00], 8, 0);
				}

				client->sending_quest = -1;

				if ((l->inpquest) && (client->bursting))
				{
					// Let the leader know it's time to send the remaining state of the quest...
					cipher_ptr = &l->client[l->leader]->server_cipher;
					memset (&client->encryptbuf[0], 0, 8);
					client->encryptbuf[0] = 0x08;
					client->encryptbuf[2] = 0xDD;
					client->encryptbuf[4] = client->clientID;
					encryptcopy (l->client[l->leader], &client->encryptbuf[0], 8);
				}
			}
			break;
		case 0xC1:
			// Create game
			if (client->lobbyNum < 0x10)
			{
				if (client->decryptbuf[0x52])
					Send1A ("Challenge games are NOT supported right now.\nCheck back later.\n\n- Sodaboy", client);
				else
							{
								unsigned lNum, failed_to_create;

								failed_to_create = 0;

								if ( (!client->isgm) && (!isLocalGM(client->guildcard)))
								{
									switch (client->decryptbuf[0x53])
									{
									case 0x01:
										if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 19))
										{
											Send01 ("Episode I\n\nYou must be level\n20 or higher\nto play on the\nhard difficulty.", client);
											failed_to_create = 1;
										}
										else
											if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 49))
											{
												Send01 ("Episode I\n\nYou must be level\n50 or higher\nto play on the\nvery hard\ndifficulty.", client);
												failed_to_create = 1;
											}
											else
												if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 89))
												{
													Send01 ("Episode I\n\nYou must be level\n90 or higher\nto play on the\nultimate\ndifficulty.", client);
													failed_to_create = 1;
												}
												break;
									case 0x02:
										if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 29))
										{
											Send01 ("Episode II\n\nYou must be level\n30 or higher\nto play on the\nhard difficulty.", client);
											failed_to_create = 1;
										}
										else
											if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 59))
											{
												Send01 ("Episode II\n\nYou must be level\n60 or higher\nto play on the\nvery hard\ndifficulty.", client);
												failed_to_create = 1;
											}
											else
												if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 99))
												{
													Send01 ("Episode II\n\nYou must be level\n100 or higher\nto play on the\nultimate\ndifficulty.", client);
													failed_to_create = 1;
												}
												break;
									case 0x03:
										if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 39))
										{
											Send01 ("Episode IV\n\nYou must be level\n40 or higher\nto play on the\nhard difficulty.", client);
											failed_to_create = 1;
										}
										else
											if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 69))
											{
												Send01 ("Episode IV\n\nYou must be level\n70 or higher\nto play on the\nvery hard\ndifficulty.", client);
												failed_to_create = 1;
											}
											else
												if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 109))
												{
													Send01 ("Episode IV\n\nYou must be level\n110 or higher\nto play on the\nultimate\ndifficulty.", client);
													failed_to_create = 1;
												}
												break;
									default:
										SendB0 ("Lol, nub.", client);
										break;
									}
								}

								if (!failed_to_create)
								{
									lNum = free_game (client);
									if (lNum)
									{
										removeClientFromLobby (client);
										client->lobbyNum = (unsigned short) lNum + 1;
										client->lobby = &blocks[client->block - 1]->lobbies[lNum];
										initialize_game (client);
										Send64 (client);
										memset (&client->encryptbuf[0x00], 0, 0x0C);
										client->encryptbuf[0x00] = 0x0C;
										client->encryptbuf[0x02] = 0x60;
										client->encryptbuf[0x08] = 0xDD;
										client->encryptbuf[0x09] = 0x03;
										client->encryptbuf[0x0A] = (unsigned char) EXPERIENCE_RATE;
										cipher_ptr = &client->server_cipher;
										encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
										UpdateGameItem (client);
									}
									else
										Send01 ("Sorry, limit of game\ncreation has been\nreached.\n\nPlease join a game\nor change ships.", client);
								}
							}
			}
			break;
		case 0xD8:
			// Show info board
			if ( ((unsigned) servertime - client->command_cooldown[0xD8]) >= 1 )
			{
				client->command_cooldown[0xD8] = (unsigned) servertime;
				CommandD8 (client);
			}
			break;
		case 0xD9:
			// Write on info board
			CommandD9 (client);
			break;
		case 0xE7:
			// Client sending character data...
			if ( client->guildcard )
			{
				if ( (client->isgm) || (isLocalGM(client->guildcard)) )
					WriteGM ("GM %u (%s) has disconnected", client->guildcard, Unicode_to_ASCII ((unsigned short*) &client->character.name[4]) );
				else
					WriteLog ("User %u (%s) has disconnected", client->guildcard, Unicode_to_ASCII ((unsigned short*) &client->character.name[4]) );
				client->todc = 1;
			}
			break;
		case 0xE8:
			// Guild card stuff
			CommandE8 (client);
			break;
		case 0xEA:
			// Team shit
			CommandEA (client, logon);
			break;
		case 0xED:
			// Set options
			CommandED (client);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (client->decryptbuf[0x02])
		{
		case 0x05:
			printf ("Client has closed the connection.\n");
			client->todc = 1;
			break;
		case 0x93:
			{
				unsigned ch,ch2,ipaddr;
				int banned = 0, match;

				client->temp_guildcard = *(unsigned*) &client->decryptbuf[0x0C];
				client->hwinfo = *(long long*) &client->decryptbuf[0x84];
				ipaddr = *(unsigned*) &client->ipaddr[0];
				for (ch=0;ch<num_bans;ch++)
				{
					if ((ship_bandata[ch].guildcard == client->temp_guildcard) && (ship_bandata[ch].type == 1))
					{
						banned = 1;
						break;
					}
					if ((ship_bandata[ch].ipaddr == ipaddr) && (ship_bandata[ch].type == 2))
					{
						banned = 1;
						break;
					}
					if ((ship_bandata[ch].hwinfo == client->hwinfo) && (ship_bandata[ch].type == 3))
					{
						banned = 1;
						break;
					}
				}

				for (ch=0;ch<num_masks;ch++)
				{
					match = 1;
					for (ch2=0;ch2<4;ch2++)
					{
						if ((ship_banmasks[ch][ch2] != 0x8000) &&
						    ((unsigned char) ship_banmasks[ch][ch2] != client->ipaddr[ch2]))
							match = 0;
					}
					if ( match )
					{
						banned = 1;
						break;
					}
				}
				
				if (banned)
				{
					Send1A ("You are banned from this ship.", client);
					client->todc = 1;
				}
				else
					if (!client->sendCheck[RECEIVE_PACKET_93])
					{
						ShipSend0B ( client, logon );
						client->sendCheck[RECEIVE_PACKET_93] = 0x01;
					}
			}
			break;

		default:
			break;
		}
	}
}

void ShipProcessPacket (BANANA* client)
{
	switch (client->decryptbuf[0x02])
	{
	case 0x05:
		printf ("Client has closed the connection.\n");
		client->todc = 1;
		break;
	case 0x10:
		Command10 (0, client);
		break;
	case 0x1D:
		client->response = (unsigned) servertime;
		break;
	case 0x93:
		{
			unsigned ch,ch2,ipaddr;
			int banned = 0, match;

			client->temp_guildcard = *(unsigned*) &client->decryptbuf[0x0C];
			client->hwinfo = *(long long*) &client->decryptbuf[0x84];
			ipaddr = *(unsigned*) &client->ipaddr[0];

			for (ch=0;ch<num_bans;ch++)
			{
				if ((ship_bandata[ch].guildcard == client->temp_guildcard) && (ship_bandata[ch].type == 1))
				{
					banned = 1;
					break;
				}
				if ((ship_bandata[ch].ipaddr == ipaddr) && (ship_bandata[ch].type == 2))
				{
					banned = 1;
					break;
				}
				if ((ship_bandata[ch].hwinfo == client->hwinfo) && (ship_bandata[ch].type == 3))
				{
					banned = 1;
					break;
				}
			}

			for (ch=0;ch<num_masks;ch++)
			{
				match = 1;
				for (ch2=0;ch2<4;ch2++)
				{
					if ((ship_banmasks[ch][ch2] != 0x8000) &&
						((unsigned char) ship_banmasks[ch][ch2] != client->ipaddr[ch2]))
						match = 0;
				}
				if ( match )
				{
					banned = 1;
					break;
				}
			}

			if (banned)
			{
				Send1A ("You are banned from this ship.", client);
				client->todc = 1;
			}
			else
				if (!client->sendCheck[RECEIVE_PACKET_93])
				{
					ShipSend0B ( client, logon );
					client->sendCheck[RECEIVE_PACKET_93] = 0x01;
				}
		}
		break;
	default:
		break;
	}
}

long CalculateChecksum(void* data,unsigned long size)
{
    long offset,y,cs = 0xFFFFFFFF;
    for (offset = 0; offset < (long)size; offset++)
    {
        cs ^= *(unsigned char*)((long)data + offset);
        for (y = 0; y < 8; y++)
        {
            if (!(cs & 1)) cs = (cs >> 1) & 0x7FFFFFFF;
            else cs = ((cs >> 1) & 0x7FFFFFFF) ^ 0xEDB88320;
        }
    }
    return (cs ^ 0xFFFFFFFF);
}


void LoadBattleParam (BATTLEPARAM* dest, const char* filename, unsigned num_records, long expected_checksum)
{
	FILE* fp;
	long battle_checksum;

	printf ("Loading %s ... ", filename);
	fp = fopen ( filename, "rb");
	if (!fp)
	{
		printf ("%s is missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	if ( ( fread ( dest, 1, sizeof (BATTLEPARAM) * num_records, fp ) != sizeof (BATTLEPARAM) * num_records ) )
	{
		printf ("%s is corrupted.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	fclose ( fp );

	printf ("OK!\n");

	battle_checksum = CalculateChecksum (dest, sizeof (BATTLEPARAM) * num_records);

	if ( battle_checksum != expected_checksum )
	{
		printf ("Checksum of file: %08x\n", battle_checksum );
		printf ("WARNING: Battle parameter file has been modified.\n");
	}
}

unsigned char qpd_buffer  [PRS_BUFFER];
unsigned char qpdc_buffer [PRS_BUFFER];
//LOBBY fakelobby;

void LoadQuests (const char* filename, unsigned category)
{
	/*unsigned oldIndex;
	unsigned qm_length, qa, nr;
	unsigned char* qmap;
	LOBBY *l;*/
	FILE* fp;
	FILE* qf;
	FILE* qd;
	unsigned qs;
	char qfile[256];
	char qfile2[256];
	char qfile3[256];
	char qfile4[256];
	char qname[256];
	unsigned qnl = 0;
	QUEST* q;
	unsigned ch, ch2, ch3, ch4, ch5, qf2l;
	unsigned short qps, qpc;
	unsigned qps2;
	QUEST_MENU* qm;
	unsigned* ed;
	unsigned ed_size, ed_ofs;
	unsigned num_records, num_objects, qm_ofs = 0, qb_ofs = 0;
	char true_filename[16];
	QDETAILS* ql;
	int extf;

	qm = &quest_menus[category];
	printf ("Loading quest list from %s ... \n", filename);
	fp = fopen ( filename, "r");
	if (!fp)
	{
		printf ("%s is missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	while (fgets (&qfile[0], 255, fp) != NULL)
	{
		for (ch=0;ch<strlen(&qfile[0]);ch++)
			if ((qfile[ch] == 10) || (qfile[ch] == 13))
				qfile[ch] = 0; // Reserved
		qfile3[0] = 0;
		strcat ( &qfile3[0], "quest\\");
		strcat ( &qfile3[0], &qfile[0] );
		memcpy ( &qfile[0], &qfile3[0], strlen ( &qfile3[0] ) + 1 );
		strcat ( &qfile3[0], "quest.lst");
		qf = fopen ( &qfile3[0], "r");
		if (!qf)
		{
			printf ("%s is missing.\n", qfile3);
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		if (fgets (&qname[0], 64, fp) == NULL)
		{
			printf ("%s is corrupted.\n", filename);
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		for (ch=0;ch<64;ch++)
		{
			if (qname[ch] != 0x00)
			{
				qm->c_names[qm->num_categories][ch*2] = qname[ch];
				qm->c_names[qm->num_categories][(ch*2)+1] = 0x00;
			}
			else
			{
				qm->c_names[qm->num_categories][ch*2] = 0x00;
				qm->c_names[qm->num_categories][(ch*2)+1] = 0x00;
				break;
			}
		}
		if (fgets (&qname[0], 120, fp) == NULL)
		{
			printf ("%s is corrupted.\n", filename);
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
		for (ch=0;ch<120;ch++)
		{
			if (qname[ch] != 0x00)
			{
				if (qname[ch] == 0x24)
					qm->c_desc[qm->num_categories][ch*2] = 0x0A;
				else
					qm->c_desc[qm->num_categories][ch*2] = qname[ch];
				qm->c_desc[qm->num_categories][(ch*2)+1] = 0x00;
			}
			else
			{
				qm->c_desc[qm->num_categories][ch*2] = 0x00;
				qm->c_desc[qm->num_categories][(ch*2)+1] = 0x00;
				break;
			}
		}
		memcpy (&qfile2[0], &qfile[0], strlen (&qfile[0])+1);
		qf2l = strlen (&qfile2[0]);
		while (fgets (&qfile2[qf2l], 255, qf) != NULL)
		{
			for (ch=0;ch<strlen(&qfile2[0]);ch++)
				if ((qfile2[ch] == 10) || (qfile2[ch] == 13))
					qfile2[ch] = 0; // Reserved

			for (ch4=0;ch4<numLanguages;ch4++)
			{
				memcpy (&qfile4[0], &qfile2[0], strlen (&qfile2[0])+1);

				// Add extension to .qst and .raw for languages

				extf = 0;

				if (strlen(languageExts[ch4]))
				{
					if ( (strlen(&qfile4[0]) - qf2l) > 3)
						for (ch5=qf2l;ch5<strlen(&qfile4[0]) - 3;ch5++)
						{
							if ((qfile4[ch5] == 46) &&
								(tolower(qfile4[ch5+1]) == 113) &&
								(tolower(qfile4[ch5+2]) == 115) &&
								(tolower(qfile4[ch5+3]) == 116))
							{
								qfile4[ch5] = 0;
								strcat (&qfile4[ch5], "_");
								strcat (&qfile4[ch5], languageExts[ch4]);
								strcat (&qfile4[ch5], ".qst");
								extf = 1;
								break;
							}
						}

						if (( (strlen(&qfile4[0]) - qf2l) > 3) && (!extf))
							for (ch5=qf2l;ch5<strlen(&qfile4[0]) - 3;ch5++)
							{
								if ((qfile4[ch5] == 46) &&
									(tolower(qfile4[ch5+1]) == 114) &&
									(tolower(qfile4[ch5+2]) == 97) &&
									(tolower(qfile4[ch5+3]) == 119))
								{
									qfile4[ch5] = 0;
									strcat (&qfile4[ch5], "_");
									strcat (&qfile4[ch5], languageExts[ch4]);
									strcat (&qfile4[ch5], ".raw");
									break;
								}
							}
				}

				qd = fopen ( &qfile4[0], "rb" );
				if (qd != NULL)
				{
					if (ch4 == 0)
					{
						q = &quests[numQuests];
						memset (q, 0, sizeof (QUEST));
					}
					ql = q->ql[ch4] = malloc ( sizeof ( QDETAILS ));
					memset (ql, 0, sizeof ( QDETAILS ));
					fseek ( qd, 0, SEEK_END );
					ql->qsize = qs = ftell ( qd );
					fseek ( qd, 0, SEEK_SET );
					ql->qdata = malloc ( qs );
					questsMemory += qs;
					fread ( ql->qdata, 1, qs, qd );
					ch = 0;
					ch2 = 0;
					while (ch < qs)
					{
						qpc = *(unsigned short*) &ql->qdata[ch+2];
						if ( (qpc == 0x13) && (strstr(&ql->qdata[ch+8], ".bin")) && (ch2 < PRS_BUFFER))
						{
							memcpy ( &true_filename[0], &ql->qdata[ch+8], 16 );
							qps2 = *(unsigned*) &ql->qdata[ch+0x418];
							memcpy (&qpd_buffer[ch2], &ql->qdata[ch+0x18], qps2);
							ch2 += qps2;
						}
						else
							if (ch2 >= PRS_BUFFER)
							{
								printf ("PRS buffer too small...\n");
								printf ("Press [ENTER] to quit...");
								gets (&dp[0]);
								exit (1);
							}
							qps = *(unsigned short*) &ql->qdata[ch];
							if (qps % 8) 
								qps += ( 8 - ( qps % 8 ) );
							ch += qps;
					}
					ed_size = prs_decompress (&qpd_buffer[0], &qpdc_buffer[0]);
					if (ed_size > PRS_BUFFER)
					{
						printf ("Memory corrupted!\n", ed_size );
						printf ("Press [ENTER] to quit...");
						gets (&dp[0]);
						exit (1);
					}
					fclose (qd);

					if (ch4 == 0)
						qm->quest_indexes[qm->num_categories][qm->quest_counts[qm->num_categories]++] = numQuests++;

					qnl = 0;
					for (ch2=0x18;ch2<0x48;ch2+=2)
					{
						if ( *(unsigned short *) &qpdc_buffer[ch2] != 0x0000 )
						{
							qname[qnl] = qpdc_buffer[ch2];
							if (qname[qnl] < 32)
								qname[qnl] = 32;
							qnl++;
						}
						else
							break;
					}

					qname[qnl] = 0;
					memcpy (&ql->qname[0], &qpdc_buffer[0x18], 0x40);
					ql->qname[qnl] = 0x0000;
					memcpy (&ql->qsummary[0], &qpdc_buffer[0x58], 0x100);
					memcpy (&ql->qdetails[0], &qpdc_buffer[0x158], 0x200);

					if (ch4 == 0)
					{
						// Load enemy data

						ch = 0;
						ch2 = 0;

						while (ch < qs)
						{
							qpc = *(unsigned short*) &ql->qdata[ch+2];
							if ( (qpc == 0x13) && (strstr(&ql->qdata[ch+8], ".dat")) && (ch2 < PRS_BUFFER))
							{
								qps2 = *(unsigned *) &ql->qdata[ch+0x418];
								memcpy (&qpd_buffer[ch2], &ql->qdata[ch+0x18], qps2);
								ch2 += qps2;
							}
							else
								if (ch2 >= PRS_BUFFER)
								{
									printf ("PRS buffer too small...\n");
									printf ("Press [ENTER] to quit...");
									gets (&dp[0]);
									exit (1);
								}

								qps = *(unsigned short*) &ql->qdata[ch];
								if (qps % 8) 
									qps += ( 8 - ( qps % 8 ) );
								ch += qps;
						}
						ed_size = prs_decompress (&qpd_buffer[0], &qpdc_buffer[0]);
						if (ed_size > PRS_BUFFER)
						{
							printf ("Memory corrupted!\n", ed_size );
							printf ("Press [ENTER] to quit...");
							gets (&dp[0]);
							exit (1);
						}
						ed_ofs = 0;
						ed = (unsigned*) &qpdc_buffer[0];
						qm_ofs = 0;
						qb_ofs = 0;
						num_objects = 0;
						while ( ed_ofs < ed_size )
						{
							switch ( *ed )
							{
							case 0x01:
								if (ed[2] > 17)
								{
									printf ("Area out of range in quest!\n");
									printf ("Press [ENTER] to quit...");
									gets (&dp[0]);
									exit(1);
								}
								num_records = ed[3] / 68L;
								num_objects += num_records;
								*(unsigned *) &qpd_buffer[qb_ofs] = *(unsigned *) &ed[2];
								qb_ofs += 4;
								//printf ("area: %u, object count: %u\n", ed[2], num_records);
								*(unsigned *) &qpd_buffer[qb_ofs] = num_records;
								qb_ofs += 4;
								memcpy ( &qpd_buffer[qb_ofs], &ed[4], ed[3] );
								qb_ofs += num_records * 68L;
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							case 0x03:
								//printf ("data type: %u\n", *ed );
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							case 0x02:
								num_records = ed[3] / 72L;
								*(unsigned *) &dp[qm_ofs] = *(unsigned *) &ed[2];
								//printf ("area: %u, mid count: %u\n", ed[2], num_records);
								if (ed[2] > 17)
								{
									printf ("Area out of range in quest!\n");
									printf ("Press [ENTER] to quit...");
									gets (&dp[0]);
									exit(1);
								}
								qm_ofs += 4;
								*(unsigned *) &dp[qm_ofs] = num_records;
								qm_ofs += 4;
								memcpy ( &dp[qm_ofs], &ed[4], ed[3] );
								qm_ofs += num_records * 72L;
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							default:
								// Probably done loading...
								ed_ofs = ed_size;
								break;
							}
						}

						// Do objects
						q->max_objects = num_objects;
						questsMemory += qb_ofs;
						q->objectdata = malloc ( qb_ofs );
						// Need to sort first...
						ch3 = 0;
						for (ch=0;ch<18;ch++)
						{
							ch2 = 0;
							while (ch2 < qb_ofs)
							{
								unsigned qa;

								qa = *(unsigned *) &qpd_buffer[ch2];
								num_records = *(unsigned *) &qpd_buffer[ch2+4];
								if (qa == ch)
								{
									memcpy (&q->objectdata[ch3], &qpd_buffer[ch2+8], ( num_records * 68 ) );
									ch3 += ( num_records * 68 );
								}
								ch2 += ( num_records * 68 ) + 8;
							}
						}

						// Do enemies

						qm_ofs += 4;
						questsMemory += qm_ofs;
						q->mapdata = malloc ( qm_ofs );
						*(unsigned *) q->mapdata = qm_ofs;
						// Need to sort first...
						ch3 = 4;
						for (ch=0;ch<18;ch++)
						{
							ch2 = 0;
							while (ch2 < ( qm_ofs - 4 ))
							{
								unsigned qa;

								qa = *(unsigned *) &dp[ch2];
								num_records = *(unsigned *) &dp[ch2+4];
								if (qa == ch)
								{
									memcpy (&q->mapdata[ch3], &dp[ch2], ( num_records * 72 ) + 8 );
									ch3 += ( num_records * 72 ) + 8;
								}
								ch2 += ( num_records * 72 ) + 8;
							}
						}
						for (ch=0;ch<num_objects;ch++)
						{
							// Swap fields in advance
							dp[0] = q->objectdata[(ch*68)+0x37];
							dp[1] = q->objectdata[(ch*68)+0x36];
							dp[2] = q->objectdata[(ch*68)+0x35];
							dp[3] = q->objectdata[(ch*68)+0x34];
							*(unsigned *) &q->objectdata[(ch*68)+0x34] = *(unsigned *) &dp[0];
						}
						printf ("Loaded quest %s (%s),\nObject count: %u, Enemy count: %u\n", qname, true_filename, num_objects, ( qm_ofs - 4 ) / 72L );
					}
					/*
					// Time to load the map data...
					l = &fakelobby;
					memset ( l, 0, sizeof (LOBBY) );
					l->bptable = &ep2battle[0];
					memset ( &l->mapData[0], 0, 0xB50 * sizeof (MAP_MONSTER) ); // Erase!
					l->mapIndex = 0;
					l->rareIndex = 0;
					for (ch=0;ch<0x20;ch++)
					l->rareData[ch] = 0xFF;

					qmap = q->mapdata;
					qm_length = *(unsigned*) qmap;
					qmap += 4;
					ch = 4;
					while ( ( qm_length - ch ) >= 80 )
					{
					oldIndex = l->mapIndex;
					qa = *(unsigned*) qmap; // Area
					qmap += 4;
					nr = *(unsigned*) qmap; // Number of monsters
					qmap += 4;
					if ( ( l->episode == 0x03 ) && ( qa > 5 ) )
					ParseMapData ( l, (MAP_MONSTER*) qmap, 1, nr );
					else
					if ( ( l->episode == 0x02 ) && ( qa > 15 ) )
					ParseMapData ( l, (MAP_MONSTER*) qmap, 1, nr );
					else
					ParseMapData ( l, (MAP_MONSTER*) qmap, 0, nr );
					qmap += ( nr * 72 );
					ch += ( ( nr * 72 ) + 8 );
					debug ("loaded quest area %u, mid count %u, total mids: %u", qa, l->mapIndex - oldIndex, l->mapIndex);
					}
					exit (1);
					*/
				}
				else
				{
					if (ch4 == 0)
					{
						printf ("Quest file %s is missing!  Could not load the quest.\n", qfile4);
						printf ("Press [ENTER] to quit...");
						gets (&dp[0]);
						exit(1);
					}
					else
					{
						printf ("Non-fatal: Alternate quest language file %s is missing.\n", qfile4);
					}
				}
			}
		}
		fclose (qf);
		qm->num_categories++;
	}
	fclose (fp);
}

unsigned csv_lines = 0;
char* csv_params[1024][64]; // 1024 lines which can carry 64 parameters each

// Release RAM from loaded CSV

void FreeCSV()
{
	unsigned ch, ch2;

	for (ch=0;ch<csv_lines;ch++)
	{
		for (ch2=0;ch2<64;ch2++)
			if (csv_params[ch][ch2] != NULL) free (csv_params[ch][ch2]);
	}
	csv_lines = 0;
	memset (&csv_params, 0, sizeof (csv_params));
}

// Load CSV into RAM

void LoadCSV(const char* filename)
{
	FILE* fp;
	char csv_data[1024];
	unsigned ch, ch2, ch3 = 0;
	//unsigned ch4;
	int open_quote = 0;
	char* csv_param;
	
	csv_lines = 0;
	memset (&csv_params, 0, sizeof (csv_params));

	//printf ("Loading CSV file %s ...\n", filename );

	if ( ( fp = fopen (filename, "r" ) ) == NULL )
	{
		printf ("The parameter file %s appears to be missing.\n", filename);
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	while (fgets (&csv_data[0], 1023, fp) != NULL)
	{
		// ch2 = current parameter we're on
		// ch3 = current index into the parameter string
		ch2 = ch3 = 0;
		open_quote = 0;
		csv_param = csv_params[csv_lines][0] = malloc (256); // allocate memory for parameter
		for (ch=0;ch<strlen(&csv_data[0]);ch++)
		{
			if ((csv_data[ch] == 44) && (!open_quote)) // comma not surrounded by quotations
			{
				csv_param[ch3] = 0; // null terminate current parameter
				ch3 = 0;
				ch2++; // new parameter
				csv_param = csv_params[csv_lines][ch2] = malloc (256); // allocate memory for parameter
			}
			else
			{
				if (csv_data[ch] == 34) // quotation mark
					open_quote = !open_quote;
				else
					if (csv_data[ch] > 31) // no loading low ascii
						csv_param[ch3++] = csv_data[ch];
			}
		}
		if (ch3)
		{
			ch2++;
			csv_param[ch3] = 0;
		}
		/*
		for (ch4=0;ch4<ch2;ch4++)
			printf ("%s,", csv_params[csv_lines][ch4]);
		printf ("\n");
		*/
		csv_lines++;
		if (csv_lines > 1023)
		{
			printf ("CSV file has too many entries.\n");
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}
	}
	printf ("Loaded %u lines...\r\n", csv_lines);
	fclose (fp);
}

void LoadArmorParam()
{
	unsigned ch,wi1;

	LoadCSV ("param\\armorpmt.ini");
	for (ch=0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte (&csv_params[ch][0][6]);
		armor_dfpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][17]);
		armor_evpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][18]);
		armor_equip_table[wi1] = (unsigned char) atoi (csv_params[ch][10]);
		armor_level_table[wi1] = (unsigned char) atoi (csv_params[ch][11]);
		//printf ("armor index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, armor_dfpvar_table[wi1], armor_evpvar_table[wi1], armor_equip_table[wi1], armor_level_table[wi1]);
	}
	FreeCSV ();
	LoadCSV ("param\\shieldpmt.ini");
	for (ch=0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte (&csv_params[ch][0][6]);
		barrier_dfpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][17]);
		barrier_evpvar_table[wi1] = (unsigned char) atoi (csv_params[ch][18]);
		barrier_equip_table[wi1] = (unsigned char) atoi (csv_params[ch][10]);
		barrier_level_table[wi1] = (unsigned char) atoi (csv_params[ch][11]);
		//printf ("barrier index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, barrier_dfpvar_table[wi1], barrier_evpvar_table[wi1], barrier_equip_table[wi1], barrier_level_table[wi1]);
	}
	FreeCSV ();
	// Set up the stack table too.
	for (ch=0;ch<0x09;ch++)
	{
		if (ch != 0x02)
			stackable_table[ch] = 10;
	}
	stackable_table[0x10] = 99;

}

void LoadWeaponParam()
{
	unsigned ch,wi1,wi2;

	LoadCSV ("param\\weaponpmt.ini");
	for (ch=0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte (&csv_params[ch][0][4]);
		wi2 = hexToByte (&csv_params[ch][0][6]);
		weapon_equip_table[wi1][wi2] = (unsigned) atoi (csv_params[ch][6]);
		*(unsigned short*) &weapon_atpmax_table[wi1][wi2] = (unsigned) atoi (csv_params[ch][8]);
		grind_table[wi1][wi2] = (unsigned char) atoi (csv_params[ch][14]);
		if (( ((wi1 >= 0x70) && (wi1 <= 0x88)) ||
			  ((wi1 >= 0xA5) && (wi1 <= 0xA9)) ) &&
			  (wi2 == 0x10))
			special_table[wi1][wi2] = 0x0B; // Fix-up S-Rank King's special
		else
			special_table[wi1][wi2] = (unsigned char) atoi (csv_params[ch][16]);
		//printf ("weapon index %02x%02x, eq: %u, grind: %u, atpmax: %u, special: %u \n", wi1, wi2, weapon_equip_table[wi1][wi2], grind_table[wi1][wi2], weapon_atpmax_table[wi1][wi2], special_table[wi1][wi2] );
	}
	FreeCSV ();
}

void LoadTechParam()
{
	unsigned ch,ch2;

	LoadCSV ("param\\tech.ini");
	if (csv_lines != 19)
	{
		printf ("Technique CSV file is corrupt.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	for (ch=0;ch<19;ch++) // For technique
	{
		for (ch2=0;ch2<12;ch2++) // For class
		{
			if (csv_params[ch][ch2+1])
				max_tech_level[ch][ch2] = ((char) atoi (csv_params[ch][ch2+1])) - 1;
			else
			{
				printf ("Technique CSV file is corrupt.\n");
				printf ("Press [ENTER] to quit...");
				gets(&dp[0]);
				exit (1);
			}
		}
	}
	FreeCSV ();
}

void LoadShopData2()
{
	FILE *fp;
	fp = fopen ("shop\\shop2.dat", "rb");
	if (!fp)
	{
		printf ("shop\\shop2.dat is missing.");
		printf ("Press [ENTER] to quit...");
		gets (&dp[0]);
		exit (1);
	}
	fread (&equip_prices[0], 1, sizeof (equip_prices), fp);
	fclose (fp);
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	if(message == MYWM_NOTIFYICON)
	{
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			switch (wParam) 
			{
			case 100:
				if (program_hidden)
				{
					program_hidden = 0;
					reveal_window;
				}
				else
				{
					program_hidden = 1;
					ShowWindow (consoleHwnd, SW_HIDE);
				}
				return TRUE;
				break;
			}
			break;
		}
	}
	return DefWindowProc( hwnd, message, wParam, lParam );
}

/********************************************************
**
**		main  :-
**
********************************************************/

int main()
{
	unsigned ch,ch2,ch3,ch4,ch5,connectNum;
	int wep_rank;
	PTDATA ptd;
	unsigned wep_counters[24] = {0};
	unsigned tool_counters[28] = {0};
	unsigned tech_counters[19] = {0};
	struct in_addr ship_in;
	struct sockaddr_in listen_in;
	unsigned listen_length;
	int block_sockfd[10] = {-1};
	struct in_addr block_in[10];
	int ship_sockfd = -1;
	int pkt_len, pkt_c, bytes_sent;
	int wserror;
	WSADATA winsock_data;
	FILE* fp;
	unsigned char* connectionChunk;
	unsigned char* connectionPtr;
	unsigned char* blockPtr;
	unsigned char* blockChunk;
	//unsigned short this_packet;
	unsigned long logon_this_packet;
	HINSTANCE hinst;
    NOTIFYICONDATA nid = {0};
	WNDCLASS wc = {0};
	HWND hwndWindow;
	MSG msg;
		
	ch = 0;

	consoleHwnd = GetConsoleWindow();
	hinst = GetModuleHandle(NULL);

	dp[0] = 0;

	strcat (&dp[0], "Tethealla Ship Server version ");
	strcat (&dp[0], SERVER_VERSION );
	strcat (&dp[0], " coded by Sodaboy");
	SetConsoleTitle (&dp[0]);

	printf ("\nTethealla Ship Server version %s  Copyright (C) 2008  Terry Chatman Jr.\n", SERVER_VERSION);
	printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf ("This program comes with ABSOLUTELY NO WARRANTY; for details\n");
	printf ("see section 15 in gpl-3.0.txt\n");
    printf ("This is free software, and you are welcome to redistribute it\n");
    printf ("under certain conditions; see gpl-3.0.txt for details.\n");

	/*for (ch=0;ch<5;ch++)
	{
		printf (".");
		Sleep (1000);
	}*/
	printf ("\n\n");

	WSAStartup(MAKEWORD(1,1), &winsock_data);
	
	printf ("Loading configuration from ship.ini ... ");
#ifdef LOG_60
	debugfile = fopen ("60packets.txt", "a");
	if (!debugfile)
	{
		printf ("Could not create 60packets.txt");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit(1);
	}
#endif
	mt_bestseed();
	load_config_file();
	printf ("OK!\n\n");

	printf ("Loading language file...\n");

	load_language_file();

	printf ("OK!\n\n");

	printf ("Loading ship_key.bin... ");

	fp = fopen ("ship_key.bin", "rb" );
	if (!fp)
	{
		printf ("Could not locate ship_key.bin!\n");
		printf ("Hit [ENTER] to quit...");
		gets (&dp[0]);
		exit (1);
	}

	fread (&ship_index, 1, 4, fp );
	fread (&ship_key[0], 1, 128, fp );
	fclose (fp);

	printf ("OK!\n\nLoading weapon parameter file...\n");
	LoadWeaponParam();
	printf ("\n.. done!\n\n");

	printf ("Loading armor & barrier parameter file...\n");
	LoadArmorParam();
	printf ("\n.. done!\n\n");

	printf ("Loading technique parameter file...\n");
	LoadTechParam();
	printf ("\n.. done!\n\n");

	for (ch=1;ch<200;ch++)
		tnlxp[ch] = tnlxp[ch-1] + tnlxp[ch];

	printf ("Loading battle parameter files...\n\n");
	LoadBattleParam (&ep1battle_off[0], "param\\BattleParamEntry.dat", 374, 0x8fef1ffe);
	LoadBattleParam (&ep1battle[0], "param\\BattleParamEntry_on.dat", 374, 0xb8a2d950);
	LoadBattleParam (&ep2battle_off[0], "param\\BattleParamEntry_lab.dat", 374, 0x3dc217f5);
	LoadBattleParam (&ep2battle[0], "param\\BattleParamEntry_lab_on.dat", 374, 0x4d4059cf);
	LoadBattleParam (&ep4battle_off[0], "param\\BattleParamEntry_ep4.dat", 332, 0x50841167);
	LoadBattleParam (&ep4battle[0], "param\\BattleParamEntry_ep4_on.dat", 332, 0x42bf9716);

	for (ch=0;ch<374;ch++)
		if (ep2battle_off[ch].HP)
		{
			ep2battle_off[ch].XP = ( ep2battle_off[ch].XP * 130 ) / 100; // 30% boost to EXP
			ep2battle[ch].XP     = ( ep2battle[ch].XP * 130 ) / 100;
		}

	printf ("\n.. done!\n\nBuilding common tables... \n\n");
	printf ("Weapon drop rate: %03f%%\n", (float) WEAPON_DROP_RATE / 1000);
	printf ("Armor drop rate: %03f%%\n", (float) ARMOR_DROP_RATE / 1000);
	printf ("Mag drop rate: %03f%%\n", (float) MAG_DROP_RATE / 1000);
	printf ("Tool drop rate: %03f%%\n", (float) TOOL_DROP_RATE / 1000);
	printf ("Meseta drop rate: %03f%%\n", (float) MESETA_DROP_RATE / 1000);
	printf ("Experience rate: %u%%\n\n", EXPERIENCE_RATE * 100);

	ch = 0;
	while (ch < 100000)
	{
		for (ch2=0;ch2<5;ch2++)
		{
			common_counters[ch2]++;
			if ((common_counters[ch2] >= common_rates[ch2]) && (ch<100000))
			{
				common_table[ch++] = (unsigned char) ch2;
				common_counters[ch2] = 0;
			}
		}
	}

	printf (".. done!\n\n");

	printf ("Loading param\\ItemPT.gsl...\n");
	fp = fopen ("param\\ItemPT.gsl", "rb");
	if (!fp)
	{
		printf ("Can't proceed without ItemPT.gsl\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	fseek (fp, 0x3000, SEEK_SET);

	// Load up that EP1 data
	printf ("Parse Episode I data... (This may take awhile...)\n");
	for (ch2=0;ch2<4;ch2++) // For each difficulty
	{
		for (ch=0;ch<10;ch++) // For each ID
		{
			fread  (&ptd, 1, sizeof (PTDATA), fp);

			ptd.enemy_dar[44] = 100; // Dragon
			ptd.enemy_dar[45] = 100; // De Rol Le
			ptd.enemy_dar[46] = 100; // Vol Opt
			ptd.enemy_dar[47] = 100; // Falz

			for (ch3=0;ch3<10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian ( ptd.box_meseta[ch3][0] );
				ptd.box_meseta[ch3][1] = swapendian ( ptd.box_meseta[ch3][1] );
			}

			for (ch3=0;ch3<0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian ( ptd.enemy_meseta[ch3][0] );
				ptd.enemy_meseta[ch3][1] = swapendian ( ptd.enemy_meseta[ch3][1] );
			}

			ptd.enemy_meseta[47][0] = ptd.enemy_meseta[46][0] + 400 + ( 100 * ch2 ); // Give Falz some meseta
			ptd.enemy_meseta[47][1] = ptd.enemy_meseta[46][1] + 400 + ( 100 * ch2 );

			for (ch3=0;ch3<23;ch3++)
			{
				for (ch4=0;ch4<6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian ( ptd.percent_pattern[ch3][ch4] );
			}

			for (ch3=0;ch3<28;ch3++)
			{
				for (ch4=0;ch4<10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian ( ptd.tool_frequency[ch3][ch4] );
				}
			}

			memcpy (&pt_tables_ep1[ch][ch2], &ptd, sizeof (PTDATA));

			// Set up the weapon drop table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3<4096))
						{
							wep_rank  = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if ( wep_rank >= 0 )
							{
								weapon_drops_ep1[ch][ch2][ch5][ch3++] = ( ch4 + 1 ) + ( (unsigned char) wep_rank << 8 );
								wep_counters[ch4] = 0;
							}
						}
					}
				}
			}

			// Set up the slot table

			memset (&wep_counters[0], 0, 4 * 24 );
			ch3 = 0;

			while (ch3 < 4096)
			{
				for (ch4=0;ch4<5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
					{
						slots_ep1[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5=0;ch5<4;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
						{
							power_patterns_ep1[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5=0;ch5<6;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							percent_patterns_ep1[ch][ch2][ch5][ch3++] = (char) ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tool_counters[0], 0, 4 * 28 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							tool_drops_ep1[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3<4096))
						{
							attachment_ep1[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5*2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3<4096))
							{
								tech_drops_ep1[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}

	// Load up that EP2 data
	printf ("Parse Episode II data... (This may take awhile...)\n");
	for (ch2=0;ch2<4;ch2++) // For each difficulty
	{
		for (ch=0;ch<10;ch++) // For each ID
		{
			fread (&ptd, 1, sizeof (PTDATA), fp);

			ptd.enemy_dar[73] = 100; // Barba Ray
			ptd.enemy_dar[76] = 100; // Gol Dragon
			ptd.enemy_dar[77] = 100; // Gar Gryphon
			ptd.enemy_dar[78] = 100; // Olga Flow

			for (ch3=0;ch3<10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian ( ptd.box_meseta[ch3][0] );
				ptd.box_meseta[ch3][1] = swapendian ( ptd.box_meseta[ch3][1] );
			}

			for (ch3=0;ch3<0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian ( ptd.enemy_meseta[ch3][0] );
				ptd.enemy_meseta[ch3][1] = swapendian ( ptd.enemy_meseta[ch3][1] );
			}

			ptd.enemy_meseta[78][0] = ptd.enemy_meseta[77][0] + 400 + ( 100 * ch2 ); // Give Flow some meseta
			ptd.enemy_meseta[78][1] = ptd.enemy_meseta[77][1] + 400 + ( 100 * ch2 );

			for (ch3=0;ch3<23;ch3++)
			{
				for (ch4=0;ch4<6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian ( ptd.percent_pattern[ch3][ch4] );
			}

			for (ch3=0;ch3<28;ch3++)
			{
				for (ch4=0;ch4<10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian ( ptd.tool_frequency[ch3][ch4] );
				}
			}

			memcpy ( &pt_tables_ep2[ch][ch2], &ptd, sizeof (PTDATA) );

			// Set up the weapon drop table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3<4096))
						{
							wep_rank  = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if ( wep_rank >= 0 )
							{
								weapon_drops_ep2[ch][ch2][ch5][ch3++] = ( ch4 + 1 ) + ( (unsigned char) wep_rank << 8 );
								wep_counters[ch4] = 0;
							}
						}
					}
				}
			}


			// Set up the slot table

			memset (&wep_counters[0], 0, 4 * 24 );
			ch3 = 0;

			while (ch3 < 4096)
			{
				for (ch4=0;ch4<5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
					{
						slots_ep2[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5=0;ch5<4;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
						{
							power_patterns_ep2[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5=0;ch5<6;ch5++)
			{
				memset (&wep_counters[0], 0, 4 * 24 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							percent_patterns_ep2[ch][ch2][ch5][ch3++] = (char) ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tool_counters[0], 0, 4 * 28 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							tool_drops_ep2[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3<4096))
						{
							attachment_ep2[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5=0;ch5<10;ch5++)
			{
				memset (&tech_counters[0], 0, 4 * 19 );
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4=0;ch4<19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5*2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3<4096))
							{
								tech_drops_ep2[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}


	fclose (fp);
	printf ("\n.. done!\n\n");
	printf ("Loading param\\PlyLevelTbl.bin ... ");
	fp = fopen ( "param\\PlyLevelTbl.bin", "rb" );
	if (!fp)
	{
		printf ("Can't proceed without PlyLevelTbl.bin!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	fread ( &startingData, 1, 12*14, fp );
	fseek ( fp, 0xE4, SEEK_SET );
	fread ( &playerLevelData, 1, 28800, fp );
	fclose ( fp );

	printf ("OK!\n\n.. done!\n\nLoading quests...\n\n");
	
	memset (&quest_menus[0], 0, sizeof (quest_menus));

	// 0 = Episode 1 Team
	// 1 = Episode 2 Team
	// 2 = Episode 4 Team
	// 3 = Episode 1 Solo
	// 4 = Episode 2 Solo
	// 5 = Episode 4 Solo
	// 6 = Episode 1 Government
	// 7 = Episode 2 Government
	// 8 = Episode 4 Government
	// 9 = Battle
	// 10 = Challenge

	LoadQuests ("quest\\ep1team.ini", 0);
	LoadQuests ("quest\\ep2team.ini", 1);
	LoadQuests ("quest\\ep4team.ini", 2);
	LoadQuests ("quest\\ep1solo.ini", 3);
	LoadQuests ("quest\\ep2solo.ini", 4);
	LoadQuests ("quest\\ep4solo.ini", 5);
	LoadQuests ("quest\\ep1gov.ini", 6);
	LoadQuests ("quest\\ep2gov.ini", 7);
	LoadQuests ("quest\\ep4gov.ini", 8);
	LoadQuests ("quest\\battle.ini", 9);

	printf ("\n%u bytes of memory allocated for %u quests...\n\n", questsMemory, numQuests);

	printf ("Loading shop\\shop.dat ...");

	fp = fopen ( "shop\\shop.dat", "rb" );

	if (!fp)
	{
		printf ("Can't proceed without shop.dat!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	if ( fread ( &shops[0], 1, 7000 * sizeof (SHOP), fp )  != (7000 * sizeof (SHOP)) )
	{
		printf ("Failed to read shop data...\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	fclose ( fp );

	shop_checksum = CalculateChecksum ( &shops[0], 7000 * sizeof (SHOP) );

	printf ("done!\n\n");

	LoadShopData2();

	readLocalGMFile();

	// Set up shop indexes based on character levels...

	for (ch=0;ch<200;ch++)
	{
		switch (ch / 20L)
		{
		case 0:	// Levels 1-20
			shopidx[ch] = 0;
			break;
		case 1: // Levels 21-40
			shopidx[ch] = 1000;
			break;
		case 2: // Levels 41-80
		case 3:
			shopidx[ch] = 2000;
			break;
		case 4: // Levels 81-120
		case 5:
			shopidx[ch] = 3000;
			break;
		case 6: // Levels 121-160
		case 7:
			shopidx[ch] = 4000;
			break;
		case 8: // Levels 161-180
			shopidx[ch] = 5000;
			break;
		default: // Levels 180+
			shopidx[ch] = 6000;
			break;
		}
	}

	memcpy (&Packet03[0x54], &Message03[0], sizeof (Message03));
	printf ("\nShip server parameters\n");
	printf ("///////////////////////\n");
	printf ("IP: %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3] );
	printf ("Ship Port: %u\n", serverPort );
	printf ("Number of Blocks: %u\n", serverBlocks );
	printf ("Maximum Connections: %u\n", serverMaxConnections );
	printf ("Logon server IP: %u.%u.%u.%u\n", loginIP[0], loginIP[1], loginIP[2], loginIP[3] );

	printf ("\nConnecting to the logon server...\n");
	initialize_logon();
	reconnect_logon();

	printf ("\nAllocating %u bytes of memory for blocks... ", sizeof (BLOCK) * serverBlocks );
	blockChunk = malloc ( sizeof (BLOCK) * serverBlocks );
	if (!blockChunk)
	{
		printf ("Out of memory!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	blockPtr = blockChunk;
	memset (blockChunk, 0, sizeof (BLOCK) * serverBlocks);
	for (ch=0;ch<serverBlocks;ch++)
	{
		blocks[ch] = (BLOCK*) blockPtr;
		blockPtr += sizeof (BLOCK);
	}

	printf ("OK!\n");

	printf ("\nAllocating %u bytes of memory for connections... ", sizeof (BANANA) * serverMaxConnections );
	connectionChunk = malloc ( sizeof (BANANA) * serverMaxConnections );
	if (!connectionChunk )
	{
		printf ("Out of memory!\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	connectionPtr = connectionChunk;
	for (ch=0;ch<serverMaxConnections;ch++)
	{
		connections[ch] = (BANANA*) connectionPtr;
		connections[ch]->guildcard = 0;
		connections[ch]->character_backup = NULL;
		connections[ch]->mode = 0;
		initialize_connection (connections[ch]);
		connectionPtr += sizeof (BANANA);
	}

	printf ("OK!\n\n");

	printf ("Loading ban data... ");
	fp = fopen ("bandata.dat", "rb");
	if (fp)
	{
		fseek ( fp, 0, SEEK_END );
		ch = ftell ( fp );
		num_bans = ch / sizeof (BANDATA);
		if ( num_bans > 5000 )
			num_bans = 5000;
		fseek ( fp, 0, SEEK_SET );
		fread ( &ship_bandata[0], 1, num_bans * sizeof (BANDATA), fp );
		fclose ( fp );
	}
	printf ("done!\n\n%u bans loaded.\n%u IP mask bans loaded.\n\n",num_bans,num_masks);

	/* Open the ship port... */

	printf ("Opening ship port %u for connections.\n", serverPort);

#ifdef USEADDR_ANY
	ship_in.s_addr = INADDR_ANY;
#else
	memcpy (&ship_in.s_addr, &serverIP[0], 4 );
#endif
	ship_sockfd = tcp_sock_open( ship_in, serverPort );

	tcp_listen (ship_sockfd);

	for (ch=1;ch<=serverBlocks;ch++)
	{
		printf ("Opening block port %u (BLOCK%u) for connections.\n", serverPort+ch, ch);
#ifdef USEADDR_ANY
		block_in[ch-1].s_addr = INADDR_ANY;
#else
		memcpy (&block_in[ch-1].s_addr, &serverIP[0], 4 );
#endif
		block_sockfd[ch-1] = tcp_sock_open( block_in[ch-1], serverPort+ch );
		if (block_sockfd[ch-1] < 0)
		{
			printf ("Failed to open port %u for connections.\n", serverPort+ch );
			printf ("Press [ENTER] to quit...");
			gets(&dp[0]);
			exit (1);
		}

		tcp_listen (block_sockfd[ch-1]);

	}

	if (ship_sockfd < 0)
	{
		printf ("Failed to open ship port for connections.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}

	printf ("\nListening...\n");
	wc.hbrBackground =(HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hIcon = LoadIcon( hinst, IDI_APPLICATION );
	wc.hCursor = LoadCursor( hinst, IDC_ARROW );
	wc.hInstance = hinst;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = "sodaboy";
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (! RegisterClass( &wc ) )
	{
		printf ("RegisterClass failure.\n");
		exit (1);
	}

	hwndWindow = CreateWindow ("sodaboy","hidden window", WS_MINIMIZE, 1, 1, 1, 1, 
		NULL, 
		NULL,
		hinst,
		NULL );

	if (!hwndWindow)
	{
		printf ("Failed to create window.");
		exit (1);
	}

	ShowWindow ( hwndWindow, SW_HIDE );
	UpdateWindow ( hwndWindow );
	ShowWindow ( consoleHwnd, SW_HIDE );
	UpdateWindow ( consoleHwnd );

    nid.cbSize				= sizeof(nid);
	nid.hWnd				= hwndWindow;
	nid.uID					= 100;
	nid.uCallbackMessage	= MYWM_NOTIFYICON;
	nid.uFlags				= NIF_MESSAGE|NIF_ICON|NIF_TIP;
    nid.hIcon				= LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON1));
	nid.szTip[0] = 0;
	strcat (&nid.szTip[0], "Tethealla Ship ");
	strcat (&nid.szTip[0], SERVER_VERSION);
	strcat (&nid.szTip[0], " - Double click to show/hide");
    Shell_NotifyIcon(NIM_ADD, &nid);

	for (;;)
	{
		int nfds = 0;

		/* Process the system tray icon */

		if ( PeekMessage( &msg, hwndWindow, 0, 0, 1 ) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		/* Ping pong?! */

		servertime = time(NULL);

		/* Clear socket activity flags. */

		FD_ZERO (&ReadFDs);
		FD_ZERO (&WriteFDs);
		FD_ZERO (&ExceptFDs);

		// Stop blocking connections after everyone has been disconnected...

		if ((serverNumConnections == 0) && (blockConnections))
		{
			blockConnections = 0;
			printf ("No longer blocking new connections...\n");
		}

		// Process player packets

		for (ch=0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			workConnect = connections[connectNum];

			if (workConnect->plySockfd >= 0) 
			{
				if (blockConnections)
				{
					if (blockTick != (unsigned) servertime)
					{
						blockTick = (unsigned) servertime;
						printf ("Disconnected user %u, left to disconnect: %u\n", workConnect->guildcard, serverNumConnections - 1);
						Send1A ("You were disconnected by a GM...", workConnect);
						workConnect->todc = 1;
					}
				}

				if (workConnect->lastTick != (unsigned) servertime)
				{
					Send1D (workConnect);
					if (workConnect->lastTick > (unsigned) servertime)
						ch2 = 1;
					else
						ch2 = 1 + ((unsigned) servertime - workConnect->lastTick);
						workConnect->lastTick = (unsigned) servertime;
						workConnect->packetsSec /= ch2;
						workConnect->toBytesSec /= ch2;
						workConnect->fromBytesSec /= ch2;
				}

				FD_SET (workConnect->plySockfd, &ReadFDs);
				nfds = max (nfds, workConnect->plySockfd);
				FD_SET (workConnect->plySockfd, &ExceptFDs);
				nfds = max (nfds, workConnect->plySockfd);

				if (workConnect->snddata - workConnect->sndwritten)
				{
					FD_SET (workConnect->plySockfd, &WriteFDs);
					nfds = max (nfds, workConnect->plySockfd);
				}
			}
		}


		// Read from logon server (if connected)

		if (logon->sockfd >= 0)
		{
			if ((unsigned) servertime - logon->last_ping > 60)
			{
				printf ("Logon server ping timeout.  Attempting reconnection in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
				initialize_logon ();
			}
			else
			{
				if (logon->packetdata)
				{
					logon_this_packet = *(unsigned *) &logon->packet[logon->packetread];
					memcpy (&logon->decryptbuf[0], &logon->packet[logon->packetread], logon_this_packet);

					LogonProcessPacket ( logon );

					logon->packetread += logon_this_packet;

					if (logon->packetread == logon->packetdata)
						logon->packetread = logon->packetdata = 0;
				}

				FD_SET (logon->sockfd, &ReadFDs);
				nfds = max (nfds, logon->sockfd);

				if (logon->snddata - logon->sndwritten)
				{
					FD_SET (logon->sockfd, &WriteFDs);
					nfds = max (nfds, logon->sockfd);
				}
			}
		}
		else
		{
			logon_tick++;
			if (logon_tick >= LOGIN_RECONNECT_SECONDS * 100)
			{
				printf ("Reconnecting to login server...\n");
				reconnect_logon();
			}
		}


		// Listen for block connections

		for (ch=0;ch<serverBlocks;ch++)
		{
			FD_SET (block_sockfd[ch], &ReadFDs);
			nfds = max (nfds, block_sockfd[ch]);
		}

		// Listen for ship connections

		FD_SET (ship_sockfd, &ReadFDs);
		nfds = max (nfds, ship_sockfd);

		/* Check sockets for activity. */

		if ( select ( nfds + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &select_timeout ) > 0 ) 
		{
			if (FD_ISSET (ship_sockfd, &ReadFDs))
			{
				// Someone's attempting to connect to the ship server.
				ch = free_connection();
				if (ch != 0xFFFF)
				{
					listen_length = sizeof (listen_in);
					workConnect = connections[ch];
					if ( ( workConnect->plySockfd = tcp_accept ( ship_sockfd, (struct sockaddr*) &listen_in, &listen_length ) ) > 0 )
					{
						if ( !blockConnections )
						{
							workConnect->connection_index = ch;
							serverConnectionList[serverNumConnections++] = ch;
							memcpy ( &workConnect->IP_Address[0], inet_ntoa (listen_in.sin_addr), 16 );
							*(unsigned *) &workConnect->ipaddr = *(unsigned *) &listen_in.sin_addr;
							printf ("Accepted SHIP connection from %s:%u\n", workConnect->IP_Address, listen_in.sin_port );
							printf ("Player Count: %u\n", serverNumConnections);
							ShipSend0E (logon);
							start_encryption (workConnect);
							/* Doin' ship process... */
							workConnect->block = 0;
						}
						else
							initialize_connection ( workConnect );
					}
				}
			}

			for (ch=0;ch<serverBlocks;ch++)
			{
				if (FD_ISSET (block_sockfd[ch], &ReadFDs))
				{
					// Someone's attempting to connect to the block server.
					ch2 = free_connection();
					if (ch2 != 0xFFFF)
					{
						listen_length = sizeof (listen_in);
						workConnect = connections[ch2];
						if  ( ( workConnect->plySockfd = tcp_accept ( block_sockfd[ch], (struct sockaddr*) &listen_in, &listen_length ) ) > 0 )
						{
							if ( !blockConnections )
							{
								workConnect->connection_index = ch2;
								serverConnectionList[serverNumConnections++] = ch2;
								memcpy ( &workConnect->IP_Address[0], inet_ntoa (listen_in.sin_addr), 16 );
								printf ("Accepted BLOCK connection from %s:%u\n", inet_ntoa (listen_in.sin_addr), listen_in.sin_port );
								*(unsigned *) &workConnect->ipaddr = *(unsigned *) &listen_in.sin_addr;
								printf ("Player Count: %u\n", serverNumConnections);
								ShipSend0E (logon);
								start_encryption (workConnect);
								/* Doin' block process... */
								workConnect->block = ch+1;
							}
							else
								initialize_connection ( workConnect );
						}
					}
				}
			}


			// Process client connections

			for (ch=0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				workConnect = connections[connectNum];

				if (workConnect->plySockfd >= 0)
				{
					if (FD_ISSET(workConnect->plySockfd, &WriteFDs))
					{
						// Write shit.

						bytes_sent = send (workConnect->plySockfd, &workConnect->sndbuf[workConnect->sndwritten],
							workConnect->snddata - workConnect->sndwritten, 0);

						if (bytes_sent == SOCKET_ERROR)
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not send data to client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							initialize_connection (workConnect);							
						}
						else
						{
							workConnect->toBytesSec += bytes_sent;
							workConnect->sndwritten += bytes_sent;
						}

						if (workConnect->sndwritten == workConnect->snddata)
							workConnect->sndwritten = workConnect->snddata = 0;
					}

					// Disconnect those violators of the law...

					if (workConnect->todc)
						initialize_connection (workConnect);

					if (FD_ISSET(workConnect->plySockfd, &ReadFDs))
					{
						// Read shit.
						if ( ( pkt_len = recv (workConnect->plySockfd, &tmprcv[0], TCP_BUFFER_SIZE - 1, 0) ) <= 0 )
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not read data from client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							initialize_connection (workConnect);
						}
						else
						{
							workConnect->fromBytesSec += (unsigned) pkt_len;
							// Work with it.

							for (pkt_c=0;pkt_c<pkt_len;pkt_c++)
							{
								workConnect->rcvbuf[workConnect->rcvread++] = tmprcv[pkt_c];

								if (workConnect->rcvread == 8)
								{
									// Decrypt the packet header after receiving 8 bytes.

									cipher_ptr = &workConnect->client_cipher;

									decryptcopy ( &workConnect->decryptbuf[0], &workConnect->rcvbuf[0], 8 );

									// Make sure we're expecting a multiple of 8 bytes.

									workConnect->expect = *(unsigned short*) &workConnect->decryptbuf[0];

									if ( workConnect->expect % 8 )
										workConnect->expect += ( 8 - ( workConnect->expect % 8 ) );

									if ( workConnect->expect > TCP_BUFFER_SIZE )
									{
										initialize_connection ( workConnect );
										break;
									}
								}

								if ( ( workConnect->rcvread == workConnect->expect ) && ( workConnect->expect != 0 ) )
								{
									// Decrypt the rest of the data if needed.

									cipher_ptr = &workConnect->client_cipher;

									if ( workConnect->rcvread > 8 )
										decryptcopy ( &workConnect->decryptbuf[8], &workConnect->rcvbuf[8], workConnect->expect - 8 );

									workConnect->packetsSec ++;

									if (
										//(workConnect->packetsSec   > 89)    ||
										(workConnect->fromBytesSec > 30000) ||
										(workConnect->toBytesSec   > 150000)
										)
									{
										printf ("%u disconnected for possible DDOS. (p/s: %u, tb/s: %u, fb/s: %u)\n", workConnect->guildcard, workConnect->packetsSec, workConnect->toBytesSec, workConnect->fromBytesSec);
										initialize_connection(workConnect);
										break;
									}
									else
									{
										switch (workConnect->block)
										{
										case 0x00:
											// Ship Server
											ShipProcessPacket (workConnect);
											break;
										default:
											// Block server
											BlockProcessPacket (workConnect);
											break;
										}
									}
									workConnect->rcvread = 0;
								}
							}
						}
					}

					if (FD_ISSET(workConnect->plySockfd, &ExceptFDs)) // Exception?
						initialize_connection (workConnect);

				}
			}


			// Process logon server connection

			if ( logon->sockfd >= 0 )
			{
				if (FD_ISSET(logon->sockfd, &WriteFDs))
				{
					// Write shit.

					bytes_sent = send (logon->sockfd, &logon->sndbuf[logon->sndwritten],
						logon->snddata - logon->sndwritten, 0);

					if (bytes_sent == SOCKET_ERROR)
					{
						wserror = WSAGetLastError();
						printf ("Could not send data to logon server...\n");
						printf ("Socket Error %u.\n", wserror );
						initialize_logon();
						printf ("Lost connection with the logon server...\n");
						printf ("Reconnect in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
					}
					else
						logon->sndwritten += bytes_sent;

					if (logon->sndwritten == logon->snddata)
						logon->sndwritten = logon->snddata = 0;
				}

				if (FD_ISSET(logon->sockfd, &ReadFDs))
				{
					// Read shit.
					if ( ( pkt_len = recv (logon->sockfd, &tmprcv[0], PACKET_BUFFER_SIZE - 1, 0) ) <= 0 )
					{
						wserror = WSAGetLastError();
						printf ("Could not read data from logon server...\n");
						printf ("Socket Error %u.\n", wserror );
						initialize_logon();
						printf ("Lost connection with the logon server...\n");
						printf ("Reconnect in %u seconds...\n", LOGIN_RECONNECT_SECONDS);
					}
					else
					{
						// Work with it.
						for (pkt_c=0;pkt_c<pkt_len;pkt_c++)
						{
							logon->rcvbuf[logon->rcvread++] = tmprcv[pkt_c];

							if (logon->rcvread == 4)
							{
								/* Read out how much data we're expecting this packet. */
								logon->expect = *(unsigned *) &logon->rcvbuf[0];

								if ( logon->expect > TCP_BUFFER_SIZE  )
								{
									printf ("Received too much data from the logon server.\nSevering connection and will reconnect in %u seconds...\n",  LOGIN_RECONNECT_SECONDS);
									initialize_logon();
								}
							}

							if ( ( logon->rcvread == logon->expect ) && ( logon->expect != 0 ) )
							{
								decompressShipPacket ( logon, &logon->decryptbuf[0], &logon->rcvbuf[0] );

								logon->expect = *(unsigned *) &logon->decryptbuf[0];

								if (logon->packetdata + logon->expect < PACKET_BUFFER_SIZE)
								{
									memcpy ( &logon->packet[logon->packetdata], &logon->decryptbuf[0], logon->expect );
									logon->packetdata += logon->expect;
								}
								else
									initialize_logon();

								if ( logon->sockfd < 0 )
									break;

								logon->rcvread = 0;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}



void tcp_listen (int sockfd)
{
	if (listen(sockfd, 10) < 0)
	{
		debug_perror ("Could not listen for connection");
		debug_perror ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit(1);
	}
}

int tcp_accept (int sockfd, struct sockaddr *client_addr, int *addr_len )
{
	int fd;

	if ((fd = accept (sockfd, client_addr, addr_len)) < 0)
		debug_perror ("Could not accept connection");

	return (fd);
}

int tcp_sock_connect(char* dest_addr, int port)
{
	int fd;
	struct sockaddr_in sa;

	/* Clear it out */
	memset((void *)&sa, 0, sizeof(sa));

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error */
	if( fd < 0 )
		debug_perror("Could not create socket");
	else
	{

		memset (&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = inet_addr (dest_addr);
		sa.sin_port = htons((unsigned short) port);

		if (connect(fd, (struct sockaddr*) &sa, sizeof(sa)) < 0)
		{
			debug_perror("Could not make TCP connection");
			return -1;
		}
	}
	return(fd);
}

/*****************************************************************************/
int tcp_sock_open(struct in_addr ip, int port)
{
	int fd, turn_on_option_flag = 1, rcSockopt;

	struct sockaddr_in sa;

	/* Clear it out */
	memset((void *)&sa, 0, sizeof(sa));

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error */
	if( fd < 0 ){
		debug_perror ("Could not create socket");
		debug_perror ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit(1);
	} 

	sa.sin_family = AF_INET;
	memcpy((void *)&sa.sin_addr, (void *)&ip, sizeof(struct in_addr));
	sa.sin_port = htons((unsigned short) port);

	/* Reuse port */

	rcSockopt = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &turn_on_option_flag, sizeof(turn_on_option_flag));

	/* bind() the socket to the interface */
	if (bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr)) < 0){
		debug_perror("Could not bind to port");
		debug_perror("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit(1);
	}

	return(fd);
}

/*****************************************************************************
* same as debug_perror but writes to debug output.
* 
*****************************************************************************/
void debug_perror( char * msg ) {
	debug( "%s : %s\n" , msg , strerror(errno) );
}
/*****************************************************************************/
void debug(char *fmt, ...)
{
#define MAX_MESG_LEN 1024

	va_list args;
	char text[ MAX_MESG_LEN ];

	va_start (args, fmt);
	strcpy (text + vsprintf( text,fmt,args), "\r\n"); 
	va_end (args);

	fprintf( stderr, "%s", text);
}

/* Blue Burst encryption routines */

static void pso_crypt_init_key_bb(unsigned char *data)
{
	unsigned x;
	for (x = 0; x < 48; x += 3)
	{
		data[x] ^= 0x19;
		data[x + 1] ^= 0x16;
		data[x + 2] ^= 0x18;
	}
}


void pso_crypt_decrypt_bb(PSO_CRYPT *pcry, unsigned char *data, unsigned
						  length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long *) &data[edx];
		ebx = ebx ^ pcry->tbl[5];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[4];
		ebp ^= *(unsigned long *) &data[edx+4];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[3];
		ebx = ebx ^ edi;
		esi = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ esi ^ pcry->tbl[2];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[1];
		ebp = ebp ^ pcry->tbl[0];
		ebx = ebx ^ edi;
		*(unsigned long *) &data[edx] = ebp;
		*(unsigned long *) &data[edx+4] = ebx;
		edx = edx+8;
	}
}


void pso_crypt_encrypt_bb(PSO_CRYPT *pcry, unsigned char *data, unsigned
						  length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long *) &data[edx];
		ebx = ebx ^ pcry->tbl[0];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[1];
		ebp ^= *(unsigned long *) &data[edx+4];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[2];
		ebx = ebx ^ edi;
		esi = ((pcry->tbl[(ebx >> 0x18) + 0x12]+pcry->tbl[((ebx >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ esi ^ pcry->tbl[3];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12]+pcry->tbl[((ebp >> 0x10)& 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[4];
		ebp = ebp ^ pcry->tbl[5];
		ebx = ebx ^ edi;
		*(unsigned long *) &data[edx] = ebp;
		*(unsigned long *) &data[edx+4] = ebx;
		edx = edx+8;
	}
}

void encryptcopy (BANANA* client, const unsigned char* src, unsigned size)
{
	unsigned char* dest;

	// Bad pointer check...
	if ( ((unsigned) client < (unsigned)connections[0]) || 
		 ((unsigned) client > (unsigned)connections[serverMaxConnections-1]) )
		return;
	if (TCP_BUFFER_SIZE - client->snddata < ( (int) size + 7 ) )
		client->todc = 1;
	else
	{
		dest = &client->sndbuf[client->snddata];
		memcpy (dest,src,size);
		while (size % 8)
			dest[size++] = 0x00;
		client->snddata += (int) size;
		pso_crypt_encrypt_bb(cipher_ptr,dest,size);
	}
}


void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned size)
{
	memcpy (dest,src,size);
	pso_crypt_decrypt_bb(cipher_ptr,dest,size);
}


void pso_crypt_table_init_bb(PSO_CRYPT *pcry, const unsigned char *salt)
{
	unsigned long eax, ecx, edx, ebx, ebp, esi, edi, ou, x;
	unsigned char s[48];
	unsigned short* pcryp;
	unsigned short* bbtbl;
	unsigned short dx;

	pcry->cur = 0;
	pcry->mangle = NULL;
	pcry->size = 1024 + 18;

	memcpy(s, salt, sizeof(s));
	pso_crypt_init_key_bb(s);

	bbtbl = (unsigned short*) &bbtable[0];
	pcryp = (unsigned short*) &pcry->tbl[0];

	eax = 0;
	ebx = 0;

	for(ecx=0;ecx<0x12;ecx++)
	{
		dx = bbtbl[eax++];
		dx = ( ( dx & 0xFF ) << 8 ) + ( dx >> 8 );
		pcryp[ebx] = dx;
		dx = bbtbl[eax++];
		dx ^= pcryp[ebx++];
		pcryp[ebx++] = dx;
	}

/*
	pcry->tbl[0] = 0x243F6A88;
	pcry->tbl[1] = 0x85A308D3;
	pcry->tbl[2] = 0x13198A2E;
	pcry->tbl[3] = 0x03707344;
	pcry->tbl[4] = 0xA4093822;
	pcry->tbl[5] = 0x299F31D0;
	pcry->tbl[6] = 0x082EFA98;
	pcry->tbl[7] = 0xEC4E6C89;
	pcry->tbl[8] = 0x452821E6;
	pcry->tbl[9] = 0x38D01377;
	pcry->tbl[10] = 0xBE5466CF;
	pcry->tbl[11] = 0x34E90C6C;
	pcry->tbl[12] = 0xC0AC29B7;
	pcry->tbl[13] = 0xC97C50DD;
	pcry->tbl[14] = 0x3F84D5B5;
	pcry->tbl[15] = 0xB5470917;
	pcry->tbl[16] = 0x9216D5D9;
	pcry->tbl[17] = 0x8979FB1B;
	
	*/

	memcpy(&pcry->tbl[18], &bbtable[18], 4096);

	ecx=0;
	//total key[0] length is min 0x412
	ebx=0;

	while (ebx < 0x12)
	{
		//in a loop
		ebp=((unsigned long) (s[ecx])) << 0x18;
		eax=ecx+1;
		edx=eax-((eax / 48)*48);
		eax=(((unsigned long) (s[edx])) << 0x10) & 0xFF0000;
		ebp=(ebp | eax) & 0xffff00ff;
		eax=ecx+2;
		edx=eax-((eax / 48)*48);
		eax=(((unsigned long) (s[edx])) << 0x8) & 0xFF00;
		ebp=(ebp | eax) & 0xffffff00;
		eax=ecx+3;
		ecx=ecx+4;
		edx=eax-((eax / 48)*48);
		eax=(unsigned long) (s[edx]);
		ebp=ebp | eax;
		eax=ecx;
		edx=eax-((eax / 48)*48);
		pcry->tbl[ebx]=pcry->tbl[ebx] ^ ebp;
		ecx=edx;
		ebx++;
	}

	ebp=0;
	esi=0;
	ecx=0;
	edi=0;
	ebx=0;
	edx=0x48;

	while (edi < edx)
	{
		esi=esi ^ pcry->tbl[0];
		eax=esi >> 0x18;
		ebx=(esi >> 0x10) & 0xff;
		eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
		ebx=(esi >> 8) & 0xFF;
		eax=eax ^ pcry->tbl[ebx+0x212];
		ebx=esi & 0xff;
		eax=eax + pcry->tbl[ebx+0x312];

		eax=eax ^ pcry->tbl[1];
		ecx= ecx ^ eax;
		ebx=ecx >> 0x18;
		eax=(ecx >> 0x10) & 0xFF;
		ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
		eax=(ecx >> 8) & 0xff;
		ebx=ebx ^ pcry->tbl[eax+0x212];
		eax=ecx & 0xff;
		ebx=ebx + pcry->tbl[eax+0x312];

		for (x = 0; x <= 5; x++)
		{
			ebx=ebx ^ pcry->tbl[(x*2)+2];
			esi= esi ^ ebx;
			ebx=esi >> 0x18;
			eax=(esi >> 0x10) & 0xFF;
			ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
			eax=(esi >> 8) & 0xff;
			ebx=ebx ^ pcry->tbl[eax+0x212];
			eax=esi & 0xff;
			ebx=ebx + pcry->tbl[eax+0x312];

			ebx=ebx ^ pcry->tbl[(x*2)+3];
			ecx= ecx ^ ebx;
			ebx=ecx >> 0x18;
			eax=(ecx >> 0x10) & 0xFF;
			ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
			eax=(ecx >> 8) & 0xff;
			ebx=ebx ^ pcry->tbl[eax+0x212];
			eax=ecx & 0xff;
			ebx=ebx + pcry->tbl[eax+0x312];
		}

		ebx=ebx ^ pcry->tbl[14];
		esi= esi ^ ebx;
		eax=esi >> 0x18;
		ebx=(esi >> 0x10) & 0xFF;
		eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
		ebx=(esi >> 8) & 0xff;
		eax=eax ^ pcry->tbl[ebx+0x212];
		ebx=esi & 0xff;
		eax=eax + pcry->tbl[ebx+0x312];

		eax=eax ^ pcry->tbl[15];
		eax= ecx ^ eax;
		ecx=eax >> 0x18;
		ebx=(eax >> 0x10) & 0xFF;
		ecx=pcry->tbl[ecx+0x12]+pcry->tbl[ebx+0x112];
		ebx=(eax >> 8) & 0xff;
		ecx=ecx ^ pcry->tbl[ebx+0x212];
		ebx=eax & 0xff;
		ecx=ecx + pcry->tbl[ebx+0x312];

		ecx=ecx ^ pcry->tbl[16];
		ecx=ecx ^ esi;
		esi= pcry->tbl[17];
		esi=esi ^ eax;
		pcry->tbl[(edi / 4)]=esi;
		pcry->tbl[(edi / 4)+1]=ecx;
		edi=edi+8;
	}


	eax=0;
	edx=0;
	ou=0;
	while (ou < 0x1000)
	{
		edi=0x48;
		edx=0x448;

		while (edi < edx)
		{
			esi=esi ^ pcry->tbl[0];
			eax=esi >> 0x18;
			ebx=(esi >> 0x10) & 0xff;
			eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
			ebx=(esi >> 8) & 0xFF;
			eax=eax ^ pcry->tbl[ebx+0x212];
			ebx=esi & 0xff;
			eax=eax + pcry->tbl[ebx+0x312];

			eax=eax ^ pcry->tbl[1];
			ecx= ecx ^ eax;
			ebx=ecx >> 0x18;
			eax=(ecx >> 0x10) & 0xFF;
			ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
			eax=(ecx >> 8) & 0xff;
			ebx=ebx ^ pcry->tbl[eax+0x212];
			eax=ecx & 0xff;
			ebx=ebx + pcry->tbl[eax+0x312];

			for (x = 0; x <= 5; x++)
			{
				ebx=ebx ^ pcry->tbl[(x*2)+2];
				esi= esi ^ ebx;
				ebx=esi >> 0x18;
				eax=(esi >> 0x10) & 0xFF;
				ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
				eax=(esi >> 8) & 0xff;
				ebx=ebx ^ pcry->tbl[eax+0x212];
				eax=esi & 0xff;
				ebx=ebx + pcry->tbl[eax+0x312];

				ebx=ebx ^ pcry->tbl[(x*2)+3];
				ecx= ecx ^ ebx;
				ebx=ecx >> 0x18;
				eax=(ecx >> 0x10) & 0xFF;
				ebx=pcry->tbl[ebx+0x12]+pcry->tbl[eax+0x112];
				eax=(ecx >> 8) & 0xff;
				ebx=ebx ^ pcry->tbl[eax+0x212];
				eax=ecx & 0xff;
				ebx=ebx + pcry->tbl[eax+0x312];
			}

			ebx=ebx ^ pcry->tbl[14];
			esi= esi ^ ebx;
			eax=esi >> 0x18;
			ebx=(esi >> 0x10) & 0xFF;
			eax=pcry->tbl[eax+0x12]+pcry->tbl[ebx+0x112];
			ebx=(esi >> 8) & 0xff;
			eax=eax ^ pcry->tbl[ebx+0x212];
			ebx=esi & 0xff;
			eax=eax + pcry->tbl[ebx+0x312];

			eax=eax ^ pcry->tbl[15];
			eax= ecx ^ eax;
			ecx=eax >> 0x18;
			ebx=(eax >> 0x10) & 0xFF;
			ecx=pcry->tbl[ecx+0x12]+pcry->tbl[ebx+0x112];
			ebx=(eax >> 8) & 0xff;
			ecx=ecx ^ pcry->tbl[ebx+0x212];
			ebx=eax & 0xff;
			ecx=ecx + pcry->tbl[ebx+0x312];

			ecx=ecx ^ pcry->tbl[16];
			ecx=ecx ^ esi;
			esi= pcry->tbl[17];
			esi=esi ^ eax;
			pcry->tbl[(ou / 4)+(edi / 4)]=esi;
			pcry->tbl[(ou / 4)+(edi / 4)+1]=ecx;
			edi=edi+8;
		}
		ou=ou+0x400;
	}
}

unsigned RleEncode(unsigned char *src, unsigned char *dest, unsigned src_size)
{
	unsigned char currChar, prevChar;             /* current and previous characters */
	unsigned short count;                /* number of characters in a run */
	unsigned src_end, dest_start;

	dest_start = (unsigned)dest;
	src_end = (unsigned)src + src_size;

	prevChar  = 0xFF - *src;

	while ((unsigned) src < src_end)
	{
		currChar = *(dest++) = *(src++);

		if ( currChar == prevChar )
		{
			if ( (unsigned) src == src_end )
			{
				*(dest++) = 0;
				*(dest++) = 0;
			}
			else
			{
				count = 0;
				while (((unsigned)src < src_end) && (count < 0xFFF0))
				{
					if (*src == prevChar)
					{
						count++;
						src++;
						if ( (unsigned) src == src_end )
						{
							*(unsigned short*) dest = count;
							dest += 2;
						}
					}
					else
					{
						*(unsigned short*) dest = count;
						dest += 2;
						prevChar = 0xFF - *src;
						break;
					}
				}
			}
		}
		else
			prevChar = currChar;
	}
	return (unsigned)dest - dest_start;
}

void RleDecode(unsigned char *src, unsigned char *dest, unsigned src_size)
{
    unsigned char currChar, prevChar;             /* current and previous characters */
    unsigned short count;                /* number of characters in a run */
	unsigned src_end;

	src_end = (unsigned) src + src_size;

    /* decode */

    prevChar = 0xFF - *src;     /* force next char to be different */

    /* read input until there's nothing left */

    while ((unsigned) src < src_end)
    {
		currChar = *(src++);

		*(dest++) = currChar;

        /* check for run */
        if (currChar == prevChar)
        {
            /* we have a run.  write it out. */
			count = *(unsigned short*) src;
			src += 2;
            while (count > 0)
            {
				*(dest++) = currChar;
                count--;
            }

            prevChar = 0xFF - *src;     /* force next char to be different */
        }
        else
        {
            /* no run */
            prevChar = currChar;
        }
    }
}

/* expand a key (makes a rc4_key) */

void prepare_key(unsigned char *keydata, unsigned len, struct rc4_key *key)
{
    unsigned index1, index2, counter;
    unsigned char *state;

    state = key->state;

    for (counter = 0; counter < 256; counter++)
        state[counter] = (unsigned char) counter;

    key->x = key->y = index1 = index2 = 0;

    for (counter = 0; counter < 256; counter++) {
        index2 = (keydata[index1] + state[counter] + index2) & 255;

        /* swap */
        state[counter] ^= state[index2];
        state[index2]  ^= state[counter];
        state[counter] ^= state[index2];

        index1 = (index1 + 1) % len;
    }
}

/* reversible encryption, will encode a buffer updating the key */

void rc4(unsigned char *buffer, unsigned len, struct rc4_key *key)
{
    unsigned x, y, xorIndex, counter;
    unsigned char *state;

    /* get local copies */
    x = key->x; y = key->y;
    state = key->state;

    for (counter = 0; counter < len; counter++) {
        x = (x + 1) & 255;
        y = (state[x] + y) & 255;

        /* swap */
        state[x] ^= state[y];
        state[y] ^= state[x];
        state[x] ^= state[y];

        xorIndex = (state[y] + state[x]) & 255;

        buffer[counter] ^= state[xorIndex];
    }

    key->x = x; key->y = y;
}

void compressShipPacket ( ORANGE* ship, unsigned char* src, unsigned long src_size )
{
	unsigned char* dest;
	unsigned long result;

	if (ship->sockfd >= 0)
	{
		if (PACKET_BUFFER_SIZE - ship->snddata < (int) ( src_size + 100 ) )
			initialize_logon();
		else
		{
			dest = &ship->sndbuf[ship->snddata];
			// Store the original packet size before RLE compression at offset 0x04 of the new packet.
			dest += 4;
			*(unsigned *) dest = src_size;
			// Compress packet using RLE, storing at offset 0x08 of new packet.
			//
			// result = size of RLE compressed data + a DWORD for the original packet size.
			result = RleEncode (src, dest+4, src_size) + 4;
			// Encrypt with RC4
			rc4 (dest, result, &ship->sc_key);
			// Increase result by the size of a DWORD for the final ship packet size.
			result += 4;
			// Copy it to the front of the packet.
			*(unsigned *) &ship->sndbuf[ship->snddata] = result;
			ship->snddata += (int) result;
		}
	}
}

void decompressShipPacket ( ORANGE* ship, unsigned char* dest, unsigned char* src )
{
	unsigned src_size, dest_size;
	unsigned char *srccpy;

	if (ship->crypt_on)
	{
		src_size = *(unsigned *) src;
		src_size -= 8;
		src += 4;
		srccpy = src;
		// Decrypt RC4
		rc4 (src, src_size+4, &ship->cs_key);
		// The first four bytes of the src should now contain the expected uncompressed data size.
		dest_size = *(unsigned *) srccpy;
		// Increase expected size by 4 before inserting into the destination buffer.  (To take account for the packet
		// size DWORD...)
		dest_size += 4;
		*(unsigned *) dest = dest_size;
		// Decompress the data...
		RleDecode (srccpy+4, dest+4, src_size);
	}
	else
	{
		src_size = *(unsigned *) src;
		memcpy (dest + 4, src + 4, src_size);
		src_size += 4;
		*(unsigned *) dest = src_size;
	}
}