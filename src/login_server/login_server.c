/************************************************************************
	Tethealla Login Server
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


// Notes:
//
// - Limit to 40 guild cards for now.
//

#define NO_SQL
#define NO_CONNECT_TEST

#include	<windows.h>
#include	<stdio.h>
#include	<string.h>
#include	<time.h>

#ifndef NO_SQL
#include	<mysql.h>
#endif
#include	<md5.h>

#include	"resource.h"
#include	"pso_crypt.h"
#include	"bbtable.h"
#include	"prs.cpp"

#define MAX_SIMULTANEOUS_CONNECTIONS 6
#define LOGIN_COMPILED_MAX_CONNECTIONS 300
#define SHIP_COMPILED_MAX_CONNECTIONS 50
#define MAX_EB02 800000
#define SERVER_VERSION "0.048"
#define MAX_ACCOUNTS 2000
#define MAX_DRESS_FLAGS 500
#define DRESS_FLAG_EXPIRY 7200

const char *PSO_CLIENT_VER_STRING = "TethVer12510";
#define PSO_CLIENT_VER 0x41

//#define USEADDR_ANY
#define DEBUG_OUTPUT
#define TCP_BUFFER_SIZE 64000
#define PACKET_BUFFER_SIZE ( TCP_BUFFER_SIZE * 16 )

#define SEND_PACKET_03 0x00 // Done
#define SEND_PACKET_E6 0x01 // Done
#define SEND_PACKET_E2 0x02 // Done
#define SEND_PACKET_E5 0x03 // Done
#define SEND_PACKET_E8 0x04 // Done
#define SEND_PACKET_DC 0x05 // Done
#define SEND_PACKET_EB 0x06 // Done
#define SEND_PACKET_E4 0x07 // Done
#define SEND_PACKET_B1 0x08
#define SEND_PACKET_A0 0x09
#define RECEIVE_PACKET_93 0x0A
#define MAX_SENDCHECK 0x0B

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

struct timeval select_timeout = { 
	0, 
	5000
};

/* functions */

void send_to_server(int sock, char* packet);
int receive_from_server(int sock, char* packet);
void debug(char *fmt, ...);
void debug_perror(char * msg);
void tcp_listen (int sockfd);
int tcp_accept (int sockfd, struct sockaddr *client_addr, int *addr_len );
int tcp_sock_connect(char* dest_addr, int port);
int tcp_sock_open(struct in_addr ip, int port);

/* Ship Packets */

unsigned char ShipPacket00[] = {
	0x00, 0x00, 0x3F, 0x01, 0x03, 0x04, 0x19, 0x55, 0x54, 0x65, 0x74, 0x68, 0x65, 0x61, 0x6C, 0x6C,
	0x61, 0x20, 0x4C, 0x6F, 0x67, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Start Encryption Packet */

unsigned char Packet03[] = {
	0xC8, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x68, 0x61, 0x6E, 0x74, 0x61, 0x73, 0x79,
	0x20, 0x53, 0x74, 0x61, 0x72, 0x20, 0x4F, 0x6E, 0x6C, 0x69, 0x6E, 0x65, 0x20, 0x42, 0x6C, 0x75,
	0x65, 0x20, 0x42, 0x75, 0x72, 0x73, 0x74, 0x20, 0x47, 0x61, 0x6D, 0x65, 0x20, 0x53, 0x65, 0x72,
	0x76, 0x65, 0x72, 0x2E, 0x20, 0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x31,
	0x39, 0x39, 0x39, 0x2D, 0x32, 0x30, 0x30, 0x34, 0x20, 0x53, 0x4F, 0x4E, 0x49, 0x43, 0x54, 0x45,
	0x41, 0x4D, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30
}; 

const unsigned char Message03[] = { "Tethealla Gate v.047" };


/* Server Redirect */

const unsigned char Packet19[] = {
	0x10, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


/* Login message */

const unsigned char Packet1A[] = {
	0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x45, 0x00 
};

/* Ping pong */

const unsigned char Packet1D[] = {
	0x08, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Current time */

const unsigned char PacketB1[] = {
	0x24, 0x00, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Guild Card Data */

unsigned char PacketDC01[0x14] = {0};
unsigned char PacketDC02[0x10] = {0};
unsigned char PacketDC_Check[54672] = {0};

/* No Character Header */

unsigned char PacketE4[0x10] = { 0 };


/* Character Header */

unsigned char PacketE5[0x88] = { 0 };

/* Security Packet */

const unsigned char PacketE6[] = {
0x44, 0x00, 0xE6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x3F, 0x71, 0x8D, 0x34, 0x37, 0x7A, 0xBD,
0x67, 0x39, 0x65, 0x6B, 0x2C, 0xB1, 0xA5, 0x7C, 0x17, 0x93, 0x93, 0x29, 0x4A, 0x90, 0xE9, 0x11,
0xB8, 0xB5, 0x0E, 0x77, 0x41, 0x30, 0x9B, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x01, 0x00, 0x00
};

/* Acknowledging client's expected checksum */

const unsigned char PacketE8[] = {
	0x0C, 0x00, 0xE8, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

/* The "Top Broadcast" Packet */

const unsigned char PacketEE[] = { 
		0x00, 0x00, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

unsigned char E2_Base[2808] = { 0 };
unsigned char PacketE2Data[2808] = { 0 };

/* String sent to server to retrieve IP address. */

char* HTTP_REQ = "GET http://www.pioneer2.net/remote.php HTTP/1.0\r\n\r\n\r\n";

/* Populated by load_config_file(): */

char mySQL_Host[255] = {0};
char mySQL_Username[255] = {0};
char mySQL_Password[255] = {0};
char mySQL_Database[255] = {0};
unsigned int mySQL_Port;
unsigned char serverIP[4];
unsigned short serverPort;
int override_on = 0;
unsigned char overrideIP[4];
unsigned short serverMaxConnections;
unsigned short serverMaxShips;
unsigned serverNumConnections = 0;
unsigned serverConnectionList[LOGIN_COMPILED_MAX_CONNECTIONS];
unsigned serverNumShips = 0;
unsigned serverShipList[SHIP_COMPILED_MAX_CONNECTIONS];
unsigned quest_numallows;
unsigned* quest_allow;
unsigned max_ship_keys = 0;

/* Rare table structure */

unsigned rt_tables_ep1[0x200 * 10 * 4] = {0};
unsigned rt_tables_ep2[0x200 * 10 * 4] = {0};
unsigned rt_tables_ep4[0x200 * 10 * 4] = {0};

unsigned mob_rate[8]; // rare appearance rate

char Welcome_Message[255] = {0};
time_t servertime;

#ifndef NO_SQL

MYSQL * myData;
char myQuery[0x10000] = {0};
MYSQL_ROW myRow ;
MYSQL_RES * myResult;

#endif

#define NO_ALIGN __declspec(align(1))

/* Mag data structure */

typedef struct NO_ALIGN st_mag
{
	unsigned char two; // "02" =P
	unsigned char mtype;
	unsigned char level;
	unsigned char blasts;
	short defense;
	short power;
	short dex;
	short mind;
	unsigned itemid;
	char synchro;
	unsigned char IQ;
	unsigned char PBflags;
	unsigned char color;
} MAG;


/* Character Data Structure */

typedef struct NO_ALIGN st_minichar {
	unsigned short packetSize; // 0x00 - 0x01
	unsigned short command; // 0x02 - 0x03
	unsigned char flags[4]; // 0x04 - 0x07
	unsigned char unknown[8]; // 0x08 - 0x0F
	unsigned short level; // 0x10 - 0x11
	unsigned short reserved; // 0x12 - 0x13
	char gcString[10]; // 0x14 - 0x1D
	unsigned char unknown2[14]; // 0x1E - 0x2B
	unsigned char nameColorBlue; // 0x2C
	unsigned char nameColorGreen; // 0x2D
	unsigned char nameColorRed; // 0x2E
	unsigned char nameColorTransparency; // 0x2F
	unsigned short skinID; // 0x30 - 0x31
	unsigned char unknown3[18]; // 0x32 - 0x43
	unsigned char sectionID; // 0x44
	unsigned char _class; // 0x45
	unsigned char skinFlag; // 0x46
	unsigned char unknown4[5]; // 0x47 - 0x4B (same as unknown5 in E7)
	unsigned short costume; // 0x4C - 0x4D
	unsigned short skin; // 0x4E - 0x4F
	unsigned short face; // 0x50 - 0x51
	unsigned short head; // 0x52 - 0x53
	unsigned short hair; // 0x54 - 0x55
	unsigned short hairColorRed; // 0x56 - 0x57
	unsigned short hairColorBlue; // 0x58 - 0x59
	unsigned short hairColorGreen; // 0x5A - 0x5B
	unsigned proportionX; // 0x5C - 0x5F
	unsigned proportionY; // 0x60 - 0x63
	unsigned char name[24]; // 0x64 - 0x7B
	unsigned char unknown5[8] ; // 0x7C - 0x83
	unsigned playTime;
} MINICHAR;

//packetSize = 0x399C;
//command = 0x00E7;

typedef struct NO_ALIGN st_bank_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare
	unsigned itemid; // player item id
	unsigned char data2[4]; // $setitem4 (mag use only)
	unsigned bank_count; // Why?
} BANK_ITEM;

typedef struct NO_ALIGN st_bank {
	unsigned bankUse;
	unsigned bankMeseta;
	BANK_ITEM bankInventory [200];
} BANK;

typedef struct NO_ALIGN st_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare
	unsigned itemid; // player item id
	unsigned char data2[4]; // $setitem4 (mag use only)
} ITEM;

typedef struct NO_ALIGN st_inventory {
	unsigned in_use; // 0x01 = item slot in use, 0xFF00 = unused
	unsigned flags; // 8 = equipped
	ITEM item;
} INVENTORY;

typedef struct NO_ALIGN st_chardata {
	unsigned short packetSize; // 0x00-0x01  // Always set to 0x399C
	unsigned short command; // 0x02-0x03 // // Always set to 0x00E7
	unsigned char flags[4]; // 0x04-0x07
	unsigned char inventoryUse; // 0x08
	unsigned char HPuse; // 0x09
	unsigned char TPuse; // 0x0A
	unsigned char lang; // 0x0B
	INVENTORY inventory[30]; // 0x0C-0x353
	unsigned short ATP; // 0x354-0x355
	unsigned short MST; // 0x356-0x357
	unsigned short EVP; // 0x358-0x359
	unsigned short HP; // 0x35A-0x35B
	unsigned short DFP; // 0x35C-0x35D
	unsigned short TP; // 0x35E-0x35F
	unsigned short LCK; // 0x360-0x361
	unsigned short ATA; // 0x362-0x363
	unsigned char unknown[8]; // 0x364-0x36B  (Offset 0x360 has 0x0A value on Schthack's...)
	unsigned short level; // 0x36C-0x36D;
	unsigned short unknown2; // 0x36E-0x36F;
	unsigned XP; // 0x370-0x373
	unsigned meseta; // 0x374-0x377;
	char gcString[10]; // 0x378-0x381;
	unsigned char unknown3[14]; // 0x382-0x38F;  // Same as E5 unknown2
	unsigned char nameColorBlue; // 0x390;
	unsigned char nameColorGreen; // 0x391;
	unsigned char nameColorRed; // 0x392;
	unsigned char nameColorTransparency; // 0x393;
	unsigned short skinID; // 0x394-0x395;
	unsigned char unknown4[18]; // 0x396-0x3A7
	unsigned char sectionID; // 0x3A8;
	unsigned char _class; // 0x3A9;
	unsigned char skinFlag; // 0x3AA;
	unsigned char unknown5[5]; // 0x3AB-0x3AF;  // Same as E5 unknown4.
	unsigned short costume; // 0x3B0 - 0x3B1;
	unsigned short skin; // 0x3B2 - 0x3B3;
	unsigned short face; // 0x3B4 - 0x3B5;
	unsigned short head; // 0x3B6 - 0x3B7;
	unsigned short hair; // 0x3B8 - 0x3B9;
	unsigned short hairColorRed; // 0x3BA-0x3BB;
	unsigned short hairColorBlue; // 0x3BC-0x3BD;
	unsigned short hairColorGreen; // 0x3BE-0x3BF;
	unsigned proportionX; // 0x3C0-0x3C3;
	unsigned proportionY; // 0x3C4-0x3C7;
	unsigned char name[24]; // 0x3C8-0x3DF;
	unsigned playTime; // 0x3E0 - 0x3E3
	unsigned char unknown6[4]; // 0x3E4 - 0x3E7;
	unsigned char keyConfig[232]; // 0x3E8 - 0x4CF;
								// Stored from ED 07 packet.
	unsigned char techniques[20]; // 0x4D0 - 0x4E3;
	unsigned char unknown7[16]; // 0x4E4 - 0x4F3;
	unsigned char options[4]; // 0x4F4-0x4F7;
								// Stored from ED 01 packet.
	unsigned char unknown8[520]; // 0x4F8 - 0x6FF;
	unsigned bankUse; // 0x700 - 0x703
	unsigned bankMeseta; // 0x704 - 0x707;
	BANK_ITEM bankInventory [200]; // 0x708 - 0x19C7
	unsigned guildCard; // 0x19C8-0x19CB;
						// Stored from E8 06 packet.
	unsigned char name2[24]; // 0x19CC - 0x19E3;
	unsigned char unknown9[232]; // 0x19E4-0x1ACB;
	unsigned char reserved1;  // 0x1ACC; // Has value 0x01 on Schthack's
	unsigned char reserved2; // 0x1ACD; // Has value 0x01 on Schthack's
	unsigned char sectionID2; // 0x1ACE;
	unsigned char _class2; // 0x1ACF;
	unsigned char unknown10[4]; // 0x1AD0-0x1AD3;
	unsigned char symbol_chats[1248];	// 0x1AD4 - 0x1FB3
										// Stored from ED 02 packet.
	unsigned char shortcuts[2624];	// 0x1FB4 - 0x29F3
									// Stored from ED 03 packet.
	unsigned char unknown11[344]; // 0x29F4 - 0x2B4B;
	unsigned char GCBoard[172]; // 0x2B4C - 0x2BF7;
	unsigned char unknown12[200]; // 0x2BF8 - 0x2CBF;
	unsigned char challengeData[320]; // 0x2CC0 - 0X2DFF
	unsigned char unknown13[172]; // 0x2E00 - 0x2EAB;
	unsigned char unknown14[276]; // 0x2EAC - 0x2FBF; // I don't know what this is, but split from unknown13 because this chunk is
													  // actually copied into the 0xE2 packet during login @ 0x08
	unsigned char keyConfigGlobal[364]; // 0x2FC0 - 0x312B  // Copied into 0xE2 login packet @ 0x11C
										// Stored from ED 04 packet.
	unsigned char joyConfigGlobal[56]; // 0x312C - 0x3163 // Copied into 0xE2 login packet @ 0x288
										// Stored from ED 05 packet.
	unsigned guildCard2; // 0x3164 - 0x3167 (From here on copied into 0xE2 login packet @ 0x2C0...)
	unsigned teamID; // 0x3168 - 0x316B
	unsigned char teamInformation[8]; // 0x316C - 0x3173 (usually blank...)
	unsigned short privilegeLevel; // 0x3174 - 0x3175
	unsigned short reserved3; // 0x3176 - 0x3177
	unsigned char teamName[28]; // 0x3178 - 0x3193
	unsigned unknown15; // 0x3194 - 0x3197
	unsigned char teamFlag[2048]; // 0x3198 - 0x3997
	unsigned char teamRewards[8]; // 0x3998 - 0x39A0
} CHARDATA;


/* Player Structure */

typedef struct st_banana {
	int plySockfd;
	int login;
	unsigned char peekbuf[8];
	unsigned char rcvbuf [TCP_BUFFER_SIZE];
	unsigned short rcvread;
	unsigned short expect;
	unsigned char decryptbuf [TCP_BUFFER_SIZE];
	unsigned char sndbuf [TCP_BUFFER_SIZE];
	unsigned char encryptbuf [TCP_BUFFER_SIZE];
	int snddata, 
		sndwritten;
	unsigned char packet [TCP_BUFFER_SIZE];
	unsigned short packetdata;
	unsigned short packetread;
	int crypt_on;
	PSO_CRYPT server_cipher, client_cipher;
	unsigned guildcard;
	char guildcard_string[12];
	unsigned char guildcard_data[20000];
	int sendingchars;
	short slotnum;
	unsigned lastTick;		// The last second
	unsigned toBytesSec;	// How many bytes per second the server sends to the client
	unsigned fromBytesSec;	// How many bytes per second the server receives from the client
	unsigned packetsSec;	// How many packets per second the server receives from the client
	unsigned connected;
	unsigned char sendCheck[MAX_SENDCHECK+2];
	int todc;
	unsigned char IP_Address[16];
	char hwinfo[18];
	int isgm;
	int dress_flag;
	unsigned connection_index;
} BANANA;

typedef struct st_dressflag {
	unsigned guildcard;
	unsigned flagtime;
} DRESSFLAG;

/* a RC4 expanded key session */
struct rc4_key {
    unsigned char state[256];
    unsigned x, y;
};

/* Ship Structure */

typedef struct st_orange {
	int shipSockfd;
	unsigned char name[13];
	unsigned playerCount;
	unsigned char shipAddr[5];
	unsigned char listenedAddr[4];
	unsigned short shipPort;
	unsigned char rcvbuf [TCP_BUFFER_SIZE];
	unsigned long rcvread;
	unsigned long expect;
	unsigned char decryptbuf [TCP_BUFFER_SIZE];
	unsigned char sndbuf [PACKET_BUFFER_SIZE];
	unsigned char encryptbuf [TCP_BUFFER_SIZE];
	unsigned char packet [PACKET_BUFFER_SIZE];
	unsigned long packetread;
	unsigned long packetdata;
	int snddata, 
		sndwritten;
	unsigned shipID;
	int authed;
	int todc;
	int crypt_on;
	unsigned char user_key[128];
	int key_change[128];
	unsigned key_index;
	struct rc4_key cs_key; // Encryption keys
	struct rc4_key sc_key; // Encryption keys
	unsigned connection_index;
	unsigned connected;
	unsigned last_ping;
	int sent_ping;
} ORANGE;

fd_set ReadFDs, WriteFDs, ExceptFDs;

DRESSFLAG dress_flags[MAX_DRESS_FLAGS];
unsigned char dp[TCP_BUFFER_SIZE*4];
unsigned char tmprcv[PACKET_BUFFER_SIZE];
char Packet1AData[TCP_BUFFER_SIZE];
char PacketEEData[TCP_BUFFER_SIZE];
unsigned char PacketEB01[0x4C8*2] = {0};
unsigned char PacketEB02[MAX_EB02] = {0};
unsigned PacketEB01_Total;
unsigned PacketEB02_Total;

unsigned keys_in_use [SHIP_COMPILED_MAX_CONNECTIONS+1] = { 0 };

#ifdef NO_SQL

typedef struct st_bank_file {
	unsigned guildcard;
	BANK common_bank;
} L_BANK_DATA;

typedef struct st_account {
	char username[18];
	char password[33];
	char email[255];
	unsigned regtime;
	char lastip[16];
	long long lasthwinfo;
	unsigned guildcard;
	int isgm;
	int isbanned;
	int islogged;
	int isactive;
	int teamid;
	int privlevel;
	unsigned char lastchar[24];
} L_ACCOUNT_DATA;


typedef struct st_character {
	unsigned guildcard;
	int slot;
	CHARDATA data;
	MINICHAR header;
} L_CHARACTER_DATA;


typedef struct st_guild {
	unsigned accountid;
	unsigned friendid;
	char friendname[24];
	char friendtext[176];
	unsigned short reserved;
	unsigned short sectionid;
	unsigned short pclass;
	unsigned short comment[45];
	int priority;	
} L_GUILD_DATA;


typedef struct st_hwbans {
	unsigned guildcard;
	long long hwinfo;
} L_HW_BANS;


typedef struct st_ipbans {
	unsigned char ipinfo;
} L_IP_BANS;


typedef struct st_key_data {
	unsigned guildcard;
	unsigned char controls[420];
} L_KEY_DATA;


typedef struct st_security_data {
	unsigned guildcard;
	unsigned thirtytwo;
	long long sixtyfour;
	int slotnum;
	int isgm;
} L_SECURITY_DATA;


typedef struct st_ship_data {
	unsigned char rc4key[128];
	unsigned idx;
} L_SHIP_DATA;


typedef struct st_team_data {
	unsigned short name[12];
	unsigned owner;
	unsigned char flag[2048];
	unsigned teamid;
} L_TEAM_DATA;

// Oh brother :D

L_BANK_DATA *bank_data[MAX_ACCOUNTS];
L_ACCOUNT_DATA *account_data[MAX_ACCOUNTS];
L_CHARACTER_DATA *character_data[MAX_ACCOUNTS*4];
L_GUILD_DATA *guild_data[MAX_ACCOUNTS*40];
L_HW_BANS *hw_bans[MAX_ACCOUNTS];
L_IP_BANS *ip_bans[MAX_ACCOUNTS];
L_KEY_DATA *key_data[MAX_ACCOUNTS];
L_SECURITY_DATA *security_data[MAX_ACCOUNTS];
L_SHIP_DATA *ship_data[SHIP_COMPILED_MAX_CONNECTIONS];
L_TEAM_DATA *team_data[MAX_ACCOUNTS];

unsigned num_accounts = 0;
unsigned num_characters = 0;
unsigned num_guilds = 0;
unsigned num_hwbans = 0;
unsigned num_ipbans = 0;
unsigned num_keydata = 0;
unsigned num_bankdata = 0;
unsigned num_security = 0;
unsigned num_shipkeys = 0;
unsigned num_teams = 0;
unsigned ds,ds2;
int ds_found, new_record, free_record;

#endif

BANK empty_bank;

/* encryption stuff */

void prepare_key(unsigned char *keydata, unsigned len, struct rc4_key *key);

PSO_CRYPT* cipher_ptr;

void encryptcopy (BANANA* client, const unsigned char* src, unsigned size);
void decryptcopy (unsigned char* dest, const unsigned char* src, unsigned size);
void compressShipPacket ( ORANGE* ship, unsigned char* src, unsigned long src_size );
void decompressShipPacket ( ORANGE* ship, unsigned char* dest, unsigned char* src );

#ifdef NO_SQL

void UpdateDataFile ( const char* filename, unsigned count, void* data, unsigned record_size, int new_record );
void DumpDataFile ( const char* filename, unsigned* count, void** data, unsigned record_size );

unsigned lastdump = 0;

#endif

#define MYWM_NOTIFYICON (WM_USER+2)
int program_hidden = 1;
HWND consoleHwnd;
HWND backupHwnd;

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

	fp = fopen ( "login.log", "a");
	if (!fp)
	{
		printf ("Unable to log to login.log\n");
	}

	fprintf (fp, "[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
	fclose (fp);

	printf ("[%02u-%02u-%u, %02u:%02u] %s", rawtime.wMonth, rawtime.wDay, rawtime.wYear, 
		rawtime.wHour, rawtime.wMinute, text);
}


void display_packet ( unsigned char* buf, int len )
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
	printf ("%s\n\n", &dp[0]);
}

/* Computes the message digest for string inString.
   Prints out message digest, a space, the string (in quotes) and a
   carriage return.
 */
void MDString (inString, outString)
char *inString;
char *outString;
{
  unsigned char c;
  MD5_CTX mdContext;
  unsigned int len = strlen (inString);

  MD5Init (&mdContext);
  MD5Update (&mdContext, inString, len);
  MD5Final (&mdContext);
  for (c=0;c<16;c++)
  {
	  *outString = mdContext.digest[c];
	  outString++;
  }
}

void convertIPString (char* IPData, unsigned IPLen, int fromConfig )
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
				serverIP[p2] = atoi (&convert_buffer[0]);
				p2++;
				p3 = 0;
				if (p2>3)
				{
					if (fromConfig)
						printf ("tethealla.ini is corrupted. (Failed to read IP information from file!)\n"); else
						printf ("Failed to determine IP address.\n");
					printf ("Hit [ENTER]");
					gets (&dp[0]);
					exit (1);
				}
			}
			else
			{
				serverIP[p2] = atoi (&convert_buffer[0]);
				if (p2 != 3)
				{
					if (fromConfig)
						printf ("tethealla.ini is corrupted. (Failed to read IP information from file!)\n"); else
						printf ("Failed to determine IP address.\n");
					printf ("Hit [ENTER]");
					gets (&dp[0]);
					exit (1);
				}
				break;
			}
		}
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

unsigned char EBBuffer [0x10000];

void construct0xEB()
{
	char EBFiles[16][255];
	unsigned EBSize;
	unsigned EBOffset = 0;
	unsigned EBChecksum;
	FILE* fp;
	FILE* fpb;
	unsigned char ch, ch6;
	unsigned ch2, ch3, ch4, ch5, ch7;

	if ( ( fp = fopen ("e8send.txt", "r" ) ) == NULL )
	{
		printf ("Missing e8send.txt\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
		exit (1);
	}
	PacketEB01[0x02] = 0xEB;
	PacketEB01[0x03] = 0x01;
	ch = ch6 = 0;
	ch3 = 0x08;
	ch4 = ch5 = 0x00;
	while ( fgets ( &EBFiles[ch][0], 255, fp ) != NULL )
	{
		ch2 = strlen (&EBFiles[ch][0]);
		if (EBFiles[ch][ch2-1] == 0x0A)
			EBFiles[ch][ch2--]  = 0x00;
		EBFiles[ch][ch2] = 0;
		printf ("\nLoading data from %s ... ", &EBFiles[ch][0]);
		if ( ( fpb = fopen (&EBFiles[ch][0], "rb") ) == NULL )
		{
			printf ("Could not open %s!\n", &EBFiles[ch][0]);
			printf ("Hit [ENTER]");
			gets (&dp[0]);
			exit (1);
		}
		fseek (fpb, 0, SEEK_END);
		EBSize = ftell (fpb);
		fseek (fpb, 0, SEEK_SET);
		fread (&EBBuffer[0], 1, EBSize, fpb);
		EBChecksum = (unsigned) CalculateChecksum(&EBBuffer[0], EBSize);
		*(unsigned *) &PacketEB01[ch3] = EBSize;
		ch3 += 4;
		*(unsigned *) &PacketEB01[ch3] = EBChecksum;
		ch3 += 4;
		*(unsigned *) &PacketEB01[ch3] = EBOffset;
		ch3 += 4;
		memcpy (&PacketEB01[ch3], &EBFiles[ch][0], ch2+1 );
		ch3 += 0x40;
		EBOffset += EBSize;
		ch++;
		fclose ( fpb );
		for (ch7=0;ch7<EBSize;ch7++)
		{
			if (ch4 == 0x00)
			{
				ch5 += 2;
				PacketEB02[ch5++] = 0xEB;
				PacketEB02[ch5++] = 0x02;
				ch5 += 4;
				PacketEB02[ch5++] = ch6;
				ch5 += 3;
				ch4 = 0x0C;
			}
			PacketEB02[ch5++] = EBBuffer[ch7];
			ch4++;
			if (ch4 == 26636)
			{
				*(unsigned short*) &PacketEB02[ch5 - 26636] = (unsigned short) ch4;
				ch4 = 0;
				ch6++;
			}
		}
	}

	if ( ch4 ) // Probably have some remaining data.
	{
		*(unsigned short*)&PacketEB02 [ch5 - ch4] = (unsigned short) ch4;
		PacketEB02_Total = ch5;
		if (ch5 > MAX_EB02)
		{
			printf ("Too much patch data to send.\n");
			printf ("Hit [ENTER]");
			gets (&dp[0]);
			exit (1);
		}
	}

	*(unsigned short*) &PacketEB01[0x00] = (unsigned short) ch3;
	PacketEB01[0x04] = ch;
	PacketEB01_Total = ch3;
	fclose ( fp );
}

unsigned normalName = 0xFFFFFFFF;
unsigned globalName = 0xFF1D94F7;
unsigned localName = 0xFFB0C4DE;

unsigned char hexToByte ( char* hs )
{
	unsigned b;

	if ( hs[0] < 58 ) b = (hs[0] - 48); else b = (hs[0] - 55);
	b *= 16;
	if ( hs[1] < 58 ) b += (hs[1] - 48); else b += (hs[1] - 55);
	return (unsigned char) b;
}

void load_config_file()
{
	int config_index = 0;
	char config_data[255];
	unsigned ch;

	FILE* fp;

	if ( ( fp = fopen ("tethealla.ini", "r" ) ) == NULL )
	{
		printf ("The configuration file tethealla.ini appears to be missing.\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
		exit (1);
	}
	else
		while (fgets (&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)
			{
				if ((config_index < 0x04) || (config_index > 0x04))
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
				case 0x05:
					// Server IP address
					{
						if ((config_data[0] == 0x41) || (config_data[0] == 0x61))
						{
							struct sockaddr_in pn_in;
							struct hostent *pn_host;
							int pn_sockfd, pn_len;
							char pn_buf[512];
							char* pn_ipdata;

							printf ("\n** Determining IP address ... ");

							pn_host = gethostbyname ( "www.pioneer2.net" );
							if (!pn_host) {
								printf ("Could not resolve www.pioneer2.net\n");
								printf ("Hit [ENTER]");
								gets (&dp[0]);
								exit (1);
							}

							/* Create a reliable, stream socket using TCP */
							if ((pn_sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
							{
								printf ("Unable to create TCP/IP streaming socket.");
								printf ("Hit [ENTER]");
								gets (&dp[0]);
								exit(1);
							}

							/* Construct the server address structure */
							memset(&pn_in, 0, sizeof(pn_in)); /* Zero out structure */
							pn_in.sin_family = AF_INET; /* Internet address family */

							*(unsigned*) &pn_in.sin_addr.s_addr = *(unsigned *) pn_host->h_addr; /* Web Server IP address */

							pn_in.sin_port = htons(80); /* Web Server port */

							/* Establish the connection to the pioneer2.net Web Server ... */

							if (connect(pn_sockfd, (struct sockaddr *) &pn_in, sizeof(pn_in)) < 0)
							{
								printf ("\nCannot connect to www.pioneer2.net!");
								printf ("Hit [ENTER]");
								gets (&dp[0]);
								exit(1);
							}

							/* Process pioneer2.net's response into the serverIP variable. */

							send_to_server ( pn_sockfd, HTTP_REQ );
							pn_len = recv(pn_sockfd, &pn_buf[0], sizeof(pn_buf) - 1, 0);
							closesocket (pn_sockfd);
							pn_buf[pn_len] = 0;
							pn_ipdata = strstr (&pn_buf[0], "/html");
							if (!pn_ipdata)
							{
								printf ("Failed to determine IP address.\n");
							}
							else
								pn_ipdata += 9;

							convertIPString (pn_ipdata, strlen (pn_ipdata), 0 );
						}
						else
						{
							convertIPString (&config_data[0], ch+1, 1);
						}
					}
					break;
				case 0x06:
					// Welcome Message
					memcpy (&Welcome_Message[0], &config_data[0], ch+1 );
					break;
				case 0x07:
					// Server Listen Port
					serverPort = atoi (&config_data[0]);
					break;
				case 0x08:
					// Max Client Connections
					serverMaxConnections = atoi (&config_data[0]);
					if ( serverMaxConnections > LOGIN_COMPILED_MAX_CONNECTIONS )
					{
						serverMaxConnections = LOGIN_COMPILED_MAX_CONNECTIONS;
						printf ("This version of the login server has not been compiled to handle more than %u login connections.  Adjusted.\n", LOGIN_COMPILED_MAX_CONNECTIONS );
					}
					if (!serverMaxConnections)
						serverMaxConnections = LOGIN_COMPILED_MAX_CONNECTIONS;
					break;
				case 0x09:
					// Max Ship Connections
					serverMaxShips = atoi (&config_data[0]);
					if ( serverMaxShips > SHIP_COMPILED_MAX_CONNECTIONS )
					{
						serverMaxShips = SHIP_COMPILED_MAX_CONNECTIONS;
						printf ("This version of the login server has not been compiled to handle more than %u ship connections.  Adjusted.\n", SHIP_COMPILED_MAX_CONNECTIONS );
					}
					if (!serverMaxShips)
						serverMaxShips = SHIP_COMPILED_MAX_CONNECTIONS;
					break;
				case 0x0A:
					// Override IP address (if specified, this IP will be sent out instead of your own to those who connect)
					if ((config_data[0] > 0x30) && (config_data[0] < 0x3A))
					{
						override_on = 1;
						*(unsigned *) &overrideIP[0] = *(unsigned *) &serverIP[0];
						serverIP[0] = 0;
						convertIPString (&config_data[0], ch+1, 1);							
					}
					break;
				case 0x0B:
					// Hildebear rate
					mob_rate[0] = atoi ( &config_data[0] );
					break;
				case 0x0C:
					// Rappy rate
					mob_rate[1] = atoi ( &config_data[0] );
					break;
				case 0x0D:
					// Lily rate
					mob_rate[2] = atoi ( &config_data[0] );
					break;
				case 0x0E:
					// Slime rate
					mob_rate[3] = atoi ( &config_data[0] );
					break;
				case 0x0F:
					// Merissa rate
					mob_rate[4] = atoi ( &config_data[0] );
					break;
				case 0x10:
					// Pazuzu rate
					mob_rate[5] = atoi ( &config_data[0] );
					break;
				case 0x11:
					// Dorphon Eclair rate
					mob_rate[6] = atoi ( &config_data[0] );
					break;
				case 0x12:
					// Kondrieu rate
					mob_rate[7] = atoi ( &config_data[0] );
					break;
				case 0x13:
					// Global GM name color
					(unsigned char) config_data[6] = hexToByte (&config_data[4]);
					(unsigned char) config_data[7] = hexToByte (&config_data[2]);
					(unsigned char) config_data[8] = hexToByte (&config_data[0]);
					config_data[9]  = 0xFF;
					globalName = *(unsigned *) &config_data[6];
					break;
				case 0x14:
					// Local GM name color
					(unsigned char) config_data[6] = hexToByte (&config_data[4]);
					(unsigned char) config_data[7] = hexToByte (&config_data[2]);
					(unsigned char) config_data[8] = hexToByte (&config_data[0]);
					config_data[9]  = 0xFF;
					localName = *(unsigned *) &config_data[6];
					break;
				case 0x15:
					// Normal name color
					(unsigned char) config_data[6] = hexToByte (&config_data[4]);
					(unsigned char) config_data[7] = hexToByte (&config_data[2]);
					(unsigned char) config_data[8] = hexToByte (&config_data[0]);
					config_data[9]  = 0xFF;
					normalName = *(unsigned *) &config_data[6];
					break;
				default:
					break;
				}
				config_index++;
			}
		}
		fclose (fp);

	if (config_index < 0x13)
	{
		printf ("tethealla.ini seems to be corrupted.\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
		exit (1);
	}
}

BANANA * connections[LOGIN_COMPILED_MAX_CONNECTIONS];
ORANGE * ships[SHIP_COMPILED_MAX_CONNECTIONS];
BANANA * workConnect;
ORANGE * workShip;

unsigned char PacketA0Data[0x4000] = {0};
unsigned short PacketA0Size = 0;
const char serverName[] = { "T\0E\0T\0H\0E\0A\0L\0L\0A\0" };

const char NoShips[9] = "No ships!";

void construct0xA0()
{
	ORANGE* shipcheck;
	unsigned totalShips = 0xFFFFFFF4;
	unsigned short A0Offset;
	char* shipName;
	unsigned char playerCountString[5];
	unsigned ch, ch2, shipNum;

	memset (&PacketA0Data[0], 0, 0x4000);

	PacketA0Data[0x02] = 0xA0;
	PacketA0Data[0x0A] = 0x20;
	*(unsigned *) &PacketA0Data[0x0C] = totalShips;
	PacketA0Data[0x10] = 0x04;
	memcpy (&PacketA0Data[0x12], &serverName[0], 18 );
	A0Offset = 0x36;
	totalShips = 0x00;
	for (ch=0;ch<serverNumShips;ch++)
	{
		shipNum = serverShipList[ch];
		if (ships[shipNum])
		{
			shipcheck = ships[shipNum];
			if ((shipcheck->shipSockfd >= 0) && (shipcheck->playerCount < 10000))
			{
				totalShips++;
				PacketA0Data[A0Offset] = 0x12;
				*(unsigned *) &PacketA0Data[A0Offset+2] = shipcheck->shipID;
				ch2 = A0Offset+0x08;
				shipName = &shipcheck->name[0];
				while (*shipName != 0x00)
				{
					PacketA0Data[ch2++] = *shipName;
					PacketA0Data[ch2++] = 0x00;
					shipName++;
				}
				PacketA0Data[ch2++] = 0x20;
				PacketA0Data[ch2++] = 0x00;
				PacketA0Data[ch2++] = 0x28;
				PacketA0Data[ch2++] = 0x00;
				_itoa (shipcheck->playerCount, &playerCountString[0], 10);
				shipName = &playerCountString[0];
				while (*shipName != 0x00)
				{
					PacketA0Data[ch2++] = *shipName;
					PacketA0Data[ch2++] = 0x00;
					shipName++;
				}
				PacketA0Data[ch2++] = 0x29;
				PacketA0Data[ch2++] = 0x00;
				A0Offset += 0x2C;
			}
		}
	}
	if (!totalShips)
	{
			totalShips++;
			PacketA0Data[A0Offset] = 0x12;
			*(unsigned *) &PacketA0Data[A0Offset+2] = totalShips;
			for (ch=0;ch<9;ch++)
				PacketA0Data[A0Offset+0x08+(ch*2)] = NoShips[ch];
			A0Offset += 0x2C;
	}
	*(unsigned *) &PacketA0Data[0x04] = totalShips;
	while (A0Offset % 8)
		PacketA0Data[A0Offset++] = 0x00;
	*(unsigned short*) &PacketA0Data[0x00] = (unsigned short) A0Offset;
	PacketA0Size = A0Offset;
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

unsigned free_shipconnection()
{
	unsigned fc;
	ORANGE* wc;

	for (fc=0;fc<serverMaxShips;fc++)
	{
		wc = ships[fc];
		if (wc->shipSockfd<0)
			return fc;
	}
	return 0xFFFF;
}


void initialize_connection (BANANA* connect)
{
	unsigned ch, ch2;

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
	memset (connect, 0, sizeof (BANANA));
	connect->plySockfd = -1;
	connect->login = -1;
	connect->lastTick = 0xFFFFFFFF;
	connect->connected = 0xFFFFFFFF;
}

void initialize_ship (ORANGE* ship)
{
	unsigned ch, ch2;

	if (ship->shipSockfd >= 0)
	{
		if ( ( ship->key_index ) && ( ship->key_index <= max_ship_keys ) && ( keys_in_use [ ship->key_index ] ) )
			keys_in_use [ ship->key_index ] = 0; // key no longer in use

		ch2 = 0;
		for (ch=0;ch<serverNumShips;ch++)
		{
			if (serverShipList[ch] != ship->connection_index)
				serverShipList[ch2++] = serverShipList[ch];
		}
		serverNumShips = ch2;
		closesocket (ship->shipSockfd);
	}
	memset (ship, 0, sizeof (ORANGE) );
	for (ch=0;ch<128;ch++)
		ship->key_change[ch] = -1;
	ship->shipSockfd = -1;
	// Will be changed upon a successful authentication
	ship->playerCount = 0xFFFF;
	construct0xA0(); // Removes inactive ships
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
		connect->sndbuf[0x68+c] = (unsigned char) rand() % 255;
		connect->sndbuf[0x98+c] = (unsigned char) rand() % 255;
	}
	connect->snddata += sizeof (Packet03);
	cipher_ptr = &connect->server_cipher;
	pso_crypt_table_init_bb (cipher_ptr, &connect->sndbuf[0x68]);
	cipher_ptr = &connect->client_cipher;
	pso_crypt_table_init_bb (cipher_ptr, &connect->sndbuf[0x98]);
	connect->crypt_on = 1;
	connect->sendCheck[SEND_PACKET_03] = 1;
	connect->connected = (unsigned) servertime;
}

void SendB1 (BANANA* client)
{
	SYSTEMTIME rawtime;

	if ((client->guildcard) && (client->slotnum != -1))
	{
		GetSystemTime (&rawtime);
		*(long long*) &client->encryptbuf[0] = *(long long*) &PacketB1[0];
		memset (&client->encryptbuf[0x08], 0, 28);
		sprintf (&client->encryptbuf[8], "%u:%02u:%02u: %02u:%02u:%02u.%03u", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
			rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds );
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &client->encryptbuf[0], 0x24 );
	}
	else
		client->todc = 1;
}

void Send1A (const char *mes, BANANA* client)
{
	unsigned short x1A_Len;

	memcpy (&Packet1AData[0], &Packet1A[0], sizeof (Packet1A));
	x1A_Len = sizeof (Packet1A);

	while (*mes != 0x00)
	{
		Packet1AData[x1A_Len++] = *(mes++);
		Packet1AData[x1A_Len++] = 0x00;
	}

	Packet1AData[x1A_Len++] = 0x00;
	Packet1AData[x1A_Len++] = 0x00;

	while (x1A_Len % 8)
		Packet1AData[x1A_Len++] = 0x00;
	*(unsigned short*) &Packet1AData[0] = x1A_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &Packet1AData[0], x1A_Len);
	client->todc = 1;
}

void SendA0 (BANANA* client)
{
	if ((client->guildcard) && (client->slotnum != -1))
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketA0Data[0], *(unsigned short*) &PacketA0Data[0]);
	}
	else
		client->todc = 1;
}


void SendEE (const char *mes, BANANA* client)
{
	unsigned short xEE_Len;

	if ((client->guildcard) && (client->slotnum != -1))
	{
		memcpy (&PacketEEData[0], &PacketEE[0], sizeof (PacketEE));
		xEE_Len = sizeof (PacketEE);

		while (*mes != 0x00)
		{
			PacketEEData[xEE_Len++] = *(mes++);
			PacketEEData[xEE_Len++] = 0x00;
		}

		PacketEEData[xEE_Len++] = 0x00;
		PacketEEData[xEE_Len++] = 0x00;

		while (xEE_Len % 8)
			PacketEEData[xEE_Len++] = 0x00;
		*(unsigned short*) &PacketEEData[0] = xEE_Len;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketEEData[0], xEE_Len);
	}
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

unsigned char key_data_send[840+10] = {0};

void SendE2 (BANANA* client)
{
	int key_exists = 0;

	if ((client->guildcard) && (!client->sendCheck[SEND_PACKET_E2]))
	{
		memcpy (&PacketE2Data[0], &E2_Base[0], 2808);

#ifdef NO_SQL

		ds_found = -1;
		for (ds=0;ds<num_keydata;ds++)
		{
			if (key_data[ds]->guildcard == client->guildcard)
			{
				ds_found = ds;
				break;
			}
		}
		if (ds_found != -1)
			memcpy (&PacketE2Data[0x11C], &key_data[ds_found]->controls, 420 );
		else
		{
			key_data[num_keydata] = malloc (sizeof(L_KEY_DATA));
			key_data[num_keydata]->guildcard = client->guildcard;
			memcpy (&key_data[num_keydata]->controls, &E2_Base[0x11C], 420);
			UpdateDataFile ("keydata.dat", num_keydata, key_data[num_keydata], sizeof(L_KEY_DATA), 1);
			num_keydata++;
		}

#else

		sprintf (&myQuery[0], "SELECT * from key_data WHERE guildcard='%u'", client->guildcard );

		//printf ("MySQL query %s\n", myQuery );

		// Check to see if we've got some saved key data.

		if ( ! mysql_query ( myData, &myQuery[0] ) )
		{
			myResult = mysql_store_result ( myData );
			key_exists = (int) mysql_num_rows ( myResult );
			if ( key_exists )
			{
				//debug ("Key data exists, fetching...");
				myRow = mysql_fetch_row ( myResult );
				memcpy (&PacketE2Data[0x11C], myRow[1], 420 );
			}
			else
			{
				//debug ("Key data does not exist...");
				mysql_real_escape_string ( myData, &key_data_send[0], &E2_Base[0x11C], 420 );
				sprintf (&myQuery[0], "INSERT INTO key_data (guildcard, controls) VALUES ('%u','%s')", client->guildcard, (char*) &key_data_send[0] );
				memcpy (&PacketE2Data[0x11C], &E2_Base[0x11C], 420);
				if ( mysql_query ( myData, &myQuery[0] ) )
					debug ("Could not insert key information into database.");
			}
			mysql_free_result ( myResult );
		}
		else
		{
			Send1A ("Could not fetch key data.\n\nThere is a problem with the MySQL database.", client);
			client->todc = 1;
			return;
		}
#endif
		memset (&PacketE2Data[0xAF4], 0xFF, 4); // Enable dressing room, etc,.
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketE2Data[0], sizeof (PacketE2Data));
		client->sendCheck[SEND_PACKET_E2] = 0x01;
	}
	else
		client->todc = 1;
}

unsigned StringLength (const char* src)
{
	unsigned ch = 0;

	while (*src != 0x00)
	{
		ch++;
		src++;
	}
	return ch;
}

CHARDATA E7_Base, E7_Work;
unsigned char chardata[0x200];
unsigned char E7chardata[(sizeof(CHARDATA)*2)+100];
unsigned char startingStats[12*14];
unsigned char DefaultTeamFlagSlashes[4098];
unsigned char DefaultTeamFlag[2048];
unsigned char FlagSlashes[4098];

void AckCharacter_Creation(unsigned char slotnum, BANANA* client)
{
	unsigned short *n;
#ifndef NO_SQL
	int char_exists;
#endif
	unsigned short packetSize;
	MINICHAR* clientchar;
	CHARDATA* E7Base;
	CHARDATA* NewE7;
	unsigned short maxFace, maxHair, maxHairColorRed, maxHairColorBlue, maxHairColorGreen, 
		maxCostume, maxSkin, maxHead;
	unsigned ch;

	packetSize = *(unsigned short*) &client->decryptbuf[0];
	if ( ( client->guildcard ) && ( slotnum < 4 ) && ( client->sendingchars == 0 ) && 
		( packetSize == 0x88 ) && ( client->decryptbuf[0x45] < CLASS_MAX ) )
	{
		clientchar = (MINICHAR*) &client->decryptbuf[0x00];
		clientchar->name[0] = 0x09; // Filter colored names
		clientchar->name[1] = 0x00;
		clientchar->name[2] = 0x45;
		clientchar->name[3] = 0x00;
		clientchar->name[22] = 0; // Truncate names too long
		clientchar->name[23] = 0;
		if ((clientchar->_class == CLASS_HUMAR) ||
			(clientchar->_class == CLASS_HUNEWEARL) ||
			(clientchar->_class == CLASS_RAMAR) ||
			(clientchar->_class == CLASS_RAMARL) ||
			(clientchar->_class == CLASS_FOMARL) ||
			(clientchar->_class == CLASS_FONEWM) ||
			(clientchar->_class == CLASS_FONEWEARL) ||
			(clientchar->_class == CLASS_FOMAR))
		{
			maxFace = 0x05;
			maxHair = 0x0A;
			maxHairColorRed = 0xFF;
			maxHairColorBlue = 0xFF;
			maxHairColorGreen = 0xFF;
			maxCostume = 0x11;
			maxSkin = 0x03;
			maxHead = 0x00;
		}
		else
		{
			maxFace = 0x00;
			maxHair = 0x00;
			maxHairColorRed = 0x00;
			maxHairColorBlue = 0x00;
			maxHairColorGreen = 0x00;
			maxCostume = 0x00;
			maxSkin = 0x18;
			maxHead = 0x04;
		}
		if (clientchar->skinID > 0x06)
			clientchar->skinID = 0x00;
		clientchar->nameColorTransparency = 0xFF;
		if (clientchar->sectionID > 0x09)
			clientchar->sectionID = 0x00;
		if (clientchar->proportionX > 0x3F800000)
			clientchar->proportionX = 0x3F800000;
		if (clientchar->proportionY > 0x3F800000)
			clientchar->proportionY = 0x3F800000;
		if (clientchar->face > maxFace)
			clientchar->face = 0x00;
		if (clientchar->hair > maxHair)
			clientchar->hair = 0x00;
		if (clientchar->hairColorRed > maxHairColorRed)
			clientchar->hairColorRed = 0x00;
		if (clientchar->hairColorBlue > maxHairColorBlue)
			clientchar->hairColorBlue = 0x00;
		if (clientchar->hairColorGreen > maxHairColorGreen)
			clientchar->hairColorGreen = 0x00;
		if (clientchar->costume > maxCostume)
			clientchar->costume = 0x00;
		if (clientchar->skin > maxSkin)
			clientchar->skin = 0x00;
		if (clientchar->head > maxHead)
			clientchar->head = 0x00;
		for (ch=0;ch<8;ch++)
			clientchar->unknown5[ch] = 0x00;
		clientchar->playTime = 0;

		if ( client->dress_flag == 0 )
		{
			/* Yeah!  MySQL! :D */

#ifndef NO_SQL

			mysql_real_escape_string ( myData, &chardata[0], &client->decryptbuf[0x10], 0x78 );

#endif

			/* Let's construct the FULL character now... */

			E7Base = &E7_Base;
			NewE7 = &E7_Work;
			memset (NewE7, 0, sizeof (CHARDATA) );
			NewE7->packetSize = 0x399C;
			NewE7->command = 0x00E7;
			memset (&NewE7->flags, 0, 4);
			NewE7->HPuse = 0;
			NewE7->TPuse = 0;
			NewE7->lang = 0;
			switch (clientchar->_class)
			{
			case CLASS_HUMAR:
			case CLASS_HUNEWEARL:
			case CLASS_HUCAST:
			case CLASS_HUCASEAL:
				// Saber
				NewE7->inventory[0].in_use = 0x01;
				NewE7->inventory[0].flags = 0x08;
				NewE7->inventory[0].item.data[1] = 0x01;
				NewE7->inventory[0].item.itemid = 0x00010000;
				break;
			case CLASS_RAMAR:
			case CLASS_RACAST:
			case CLASS_RACASEAL:
			case CLASS_RAMARL:
				// Handgun
				NewE7->inventory[0].in_use = 0x01;
				NewE7->inventory[0].flags = 0x08;
				NewE7->inventory[0].item.data[1] = 0x06;
				NewE7->inventory[0].item.itemid = 0x00010000;
				break;
			case CLASS_FONEWM:
			case CLASS_FONEWEARL:
			case CLASS_FOMARL:
			case CLASS_FOMAR:
				// Cane
				NewE7->inventory[0].in_use = 0x01;
				NewE7->inventory[0].flags = 0x08;
				NewE7->inventory[0].item.data[1] = 0x0A;
				NewE7->inventory[0].item.itemid = 0x00010000;
				break;
			default:
				break;
			}

			// Frame
			NewE7->inventory[1].in_use = 0x01;
			NewE7->inventory[1].flags = 0x08;
			NewE7->inventory[1].item.data[0] = 0x01;
			NewE7->inventory[1].item.data[1] = 0x01;
			NewE7->inventory[1].item.itemid = 0x00010001;

			// Mag
			NewE7->inventory[2].in_use = 0x01;
			NewE7->inventory[2].flags = 0x08;
			NewE7->inventory[2].item.data[0] = 0x02;
			NewE7->inventory[2].item.data[2] = 0x05;
			NewE7->inventory[2].item.data[4] = 0xF4;
			NewE7->inventory[2].item.data[5] = 0x01;
			NewE7->inventory[2].item.data2[0] = 0x14; // 20% synchro
			NewE7->inventory[2].item.itemid = 0x00010002;

			if ((clientchar->_class == CLASS_HUCAST) || (clientchar->_class == CLASS_HUCASEAL) ||
				(clientchar->_class == CLASS_RACAST) || (clientchar->_class == CLASS_RACASEAL))
				NewE7->inventory[2].item.data2[3] = (unsigned char) clientchar->skin;
			else
				NewE7->inventory[2].item.data2[3] = (unsigned char) clientchar->costume;

			if (NewE7->inventory[2].item.data2[3] > 0x11)
				NewE7->inventory[2].item.data2[3] -= 0x11;

			// Monomates
			NewE7->inventory[3].in_use = 0x01;
			NewE7->inventory[3].item.data[0] = 0x03;
			NewE7->inventory[3].item.data[5] = 0x04;
			NewE7->inventory[3].item.itemid = 0x00010003;

			memcpy (&NewE7->techniques, &E7Base->techniques, 20);

			if  ( ( clientchar->_class == CLASS_FONEWM ) || ( clientchar->_class == CLASS_FONEWEARL ) ||
				( clientchar->_class == CLASS_FOMARL ) || ( clientchar->_class == CLASS_FOMAR ) )
			{
				// Monofluids
				NewE7->techniques[0] = 0x00;
				NewE7->inventory[4].in_use = 0x01;
				NewE7->inventory[4].flags = 0x00;
				NewE7->inventory[4].item.data[0] = 0x03;
				NewE7->inventory[4].item.data[1] = 0x01;
				NewE7->inventory[4].item.data[2] = 0x00;
				NewE7->inventory[4].item.data[3] = 0x00;
				memset (&NewE7->inventory[4].item.data[4], 0x00, 8);
				NewE7->inventory[4].item.data[5] = 0x04;
				NewE7->inventory[4].item.itemid = 0x00010004;
				memset (&NewE7->inventory[3].item.data2[0], 0x00, 4);
				NewE7->inventoryUse = 5;
			}
			else
				NewE7->inventoryUse = 4;

			for (ch=NewE7->inventoryUse;ch<30;ch++)
			{
				NewE7->inventory[ch].in_use = 0x00;
				NewE7->inventory[ch].item.data[1] = 0xFF;
				NewE7->inventory[ch].item.itemid = 0xFFFFFFFF;
			}

			memcpy (&NewE7->ATP, &startingStats[clientchar->_class * 14], 14);
			*(long long*) &NewE7->unknown = *(long long*) &E7Base->unknown;
			//NewE7->level = 0x00;
			NewE7->unknown2 = E7Base->unknown2;
			//NewE7->XP = 0;
			NewE7->meseta = 300;
			memset (&NewE7->gcString[0], 0x20, 2);
			*(long long*) &NewE7->gcString[2] = *(long long*) &client->guildcard_string[0];
			memcpy (&NewE7->unknown3, &clientchar->unknown2, 14 );
			*(unsigned *) &NewE7->nameColorBlue = *(unsigned *) &clientchar->nameColorBlue; // Will copy all 4 values.
			NewE7->skinID = clientchar->skinID;
			memcpy (&NewE7->unknown4, &clientchar->unknown3, 18);
			NewE7->sectionID = clientchar->sectionID;
			NewE7->_class = clientchar->_class;
			NewE7->skinFlag = clientchar->skinFlag;
			memcpy (&NewE7->unknown5, &clientchar->unknown4, 5);
			NewE7->costume = clientchar->costume;
			NewE7->skin = clientchar->skin;
			NewE7->face = clientchar->face;
			NewE7->head = clientchar->head;
			NewE7->hair = clientchar->hair;
			NewE7->hairColorRed = clientchar->hairColorRed;
			NewE7->hairColorBlue = clientchar->hairColorBlue;
			NewE7->hairColorGreen = clientchar->hairColorGreen;
			NewE7->proportionX = clientchar->proportionX;
			NewE7->proportionY = clientchar->proportionY;
			n = (unsigned short*) &clientchar->name[4];
			for (ch=0;ch<10;ch++)
			{
				if (*n == 0x0000)
					break;
				if ((*n == 0x0009) || (*n == 0x000A))
					*n = 0x0020;
				n++;
			}
			memcpy (&NewE7->name, &clientchar->name, 24);
			*(unsigned *) &NewE7->unknown6 = *(unsigned *) &clientchar->unknown5;
			memcpy (&NewE7->keyConfig, &E7Base->keyConfig, 232);
			// TO DO: Give Foie to starting Forces.
			memcpy (&NewE7->unknown7, &E7Base->unknown7, 16);
			*(unsigned *) &NewE7->options = *(unsigned *) &E7Base->options;
			memcpy (&NewE7->unknown8, &E7Base->unknown8, 520);
			NewE7->bankUse = E7Base->bankUse;
			NewE7->bankMeseta = E7Base->bankMeseta;
			memcpy (&NewE7->bankInventory, &E7Base->bankInventory, 24*200);
			NewE7->guildCard = client->guildcard;
			memcpy (&NewE7->name2, &clientchar->name, 24);
			memcpy (&NewE7->unknown9, &E7Base->unknown9, 232);
			NewE7->reserved1 = 0x01;
			NewE7->reserved2 = 0x01;
			NewE7->sectionID2 = clientchar->sectionID;
			NewE7->_class2 = clientchar->_class;
			*(unsigned *) &NewE7->unknown10[0] = *(unsigned *) &E7Base->unknown10[0];
			memcpy (&NewE7->symbol_chats, &E7Base->symbol_chats, 1248);
			memcpy (&NewE7->shortcuts, &E7Base->shortcuts, 2624);
			memcpy (&NewE7->unknown11, &E7Base->unknown11, 344);
			memcpy (&NewE7->GCBoard, &E7Base->GCBoard, 172);
			memcpy (&NewE7->unknown12, &E7Base->unknown12, 200);
			memcpy (&NewE7->challengeData, &E7Base->challengeData, 320);
			memcpy (&NewE7->unknown13, &E7Base->unknown13, 172);
			memcpy (&NewE7->unknown14, &E7Base->unknown14, 276);
			// TO DO: Actually, we should use the values from the database, but I haven't added columns for them yet...
			// For now, we'll use the "base" packet's values.
			memcpy (&NewE7->keyConfigGlobal, &E7Base->keyConfigGlobal, 364);
			memcpy (&NewE7->joyConfigGlobal, &E7Base->joyConfigGlobal, 56);
			memcpy (&NewE7->guildCard2, &E7Base->guildCard2, 2108);

#ifndef NO_SQL
			
			mysql_real_escape_string ( myData, &E7chardata[0], (unsigned char*) NewE7, sizeof (CHARDATA) );

#endif

		}

		// Check to see if the character exists in that slot.

#ifdef NO_SQL

		ds_found = -1;
		free_record = -1;

		for (ds=0;ds<num_characters;ds++)
		{
			if (character_data[ds]->guildcard == 0)
				free_record = ds;

			if ((character_data[ds]->guildcard == client->guildcard) &&
				(character_data[ds]->slot == slotnum))
			{
				ds_found = ds;
				break;
			}
		}

		if ( ( client->dress_flag ) && ( ds_found != -1 ) )
		{
			debug ("Update character %u", client->guildcard);
			NewE7 = &E7_Work;
			memcpy (NewE7, &character_data[ds_found]->data, sizeof (CHARDATA) );
			memcpy (&NewE7->gcString[0], &clientchar->gcString[0], 0x68);
			*(long long*) &clientchar->unknown5[0] = *(long long*) &NewE7->unknown6[0];
			//NewE7->playTime = 0;
			memcpy (&character_data[ds_found]->header, &client->decryptbuf[0x10], 0x78);
			memcpy (&character_data[ds_found]->data, NewE7, sizeof (CHARDATA));
			UpdateDataFile ("character.dat", ds_found, character_data[ds_found], sizeof(L_CHARACTER_DATA), 0);
		}
		else
		{
			if (ds_found == -1)
			{
				if (free_record != -1)
				{
					ds_found = free_record;
					new_record = 0;
				}
				else
				{
					ds_found = num_characters;
					new_record = 1;
					character_data[num_characters++] = malloc (sizeof(L_CHARACTER_DATA));
				}
			}
			else
				new_record = 0;
			character_data[ds_found]->guildcard = client->guildcard;
			character_data[ds_found]->slot = slotnum;
			memcpy (&character_data[ds_found]->data, NewE7, sizeof (CHARDATA));
			memcpy (&character_data[ds_found]->header, &client->decryptbuf[0x10], 0x78);
			UpdateDataFile ("character.dat", ds_found, character_data[ds_found], sizeof(L_CHARACTER_DATA), new_record);
		}
		PacketE4[0x00] = 0x10;
		PacketE4[0x02] = 0xE4;
		PacketE4[0x03] = 0x00;
		PacketE4[0x08] = slotnum;
		PacketE4[0x0C] = 0x00;
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketE4[0], sizeof (PacketE4));
		ds_found = -1;
		for (ds=0;ds<num_security;ds++)
		{
			if (security_data[ds]->guildcard == client->guildcard)
			{
				ds_found = ds;
				security_data[ds]->slotnum = slotnum;
				UpdateDataFile ("security.dat", ds, security_data[ds], sizeof (L_SECURITY_DATA), 0);
				break;
			}
		}

		if (ds_found == -1)
		{
			Send1A ("Could not select character.", client);
			client->todc = 1;
		}

#else

		sprintf (&myQuery[0], "SELECT * from character_data WHERE guildcard='%u' AND slot='%u'", client->guildcard, slotnum );
		//printf ("MySQL query %s\n", myQuery );

		if ( ! mysql_query ( myData, &myQuery[0] ) )
		{
			myResult = mysql_store_result ( myData );
			char_exists = (int) mysql_num_rows ( myResult );
			if (char_exists)
			{
				if (client->dress_flag == 0)
				{
					// Delete character if recreating...
					sprintf (&myQuery[0], "DELETE from character_data WHERE guildcard='%u' AND slot ='%u'", client->guildcard, slotnum );
					mysql_query ( myData, &myQuery[0] );
				}
				else
				{
					// Updating character only...
					myRow = mysql_fetch_row ( myResult );
					NewE7 = &E7_Work;
					memcpy (NewE7, myRow[2], sizeof (CHARDATA) );
					memcpy (&NewE7->gcString[0], &clientchar->gcString[0], 0x68);
					*(long long*) &clientchar->unknown5[0] = *(long long*) &NewE7->unknown6[0];
					//NewE7->playTime = 0;
					mysql_real_escape_string ( myData, &chardata[0], &client->decryptbuf[0x10], 0x78 );
					mysql_real_escape_string ( myData, &E7chardata[0], (unsigned char*) NewE7, sizeof (CHARDATA) );
				}
			}
			mysql_free_result ( myResult );
		}
		else
		{
			Send1A ("Could not check character information.\n\nThere is a problem with the MySQL database.", client);
			client->todc = 1;
			return;
		}
		if (client->dress_flag == 0)
			sprintf (&myQuery[0], "INSERT INTO character_data (guildcard,slot,data,header) VALUES ('%u','%u','%s','%s')", client->guildcard, slotnum, (char*) &E7chardata[0], (char*) &chardata[0] );
		else
		{
			debug ("Update character...");
			sprintf (&myQuery[0], "UPDATE character_data SET data='%s', header='%s' WHERE guildcard='%u' AND slot='%u'", (char*) &E7chardata[0], (char*) &chardata[0], client->guildcard, slotnum  );
		}
		if ( ! mysql_query ( myData, &myQuery[0] ) )
		{
			PacketE4[0x00] = 0x10;
			PacketE4[0x02] = 0xE4;
			PacketE4[0x03] = 0x00;
			PacketE4[0x08] = slotnum;
			PacketE4[0x0C] = 0x00;
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &PacketE4[0], sizeof (PacketE4));
			sprintf (&myQuery[0], "UPDATE security_data SET slotnum = '%u' WHERE guildcard = '%u'", slotnum, client->guildcard );
			if ( mysql_query ( myData, &myQuery[0] ) )
			{
				Send1A ("Could not select character.", client);
				client->todc = 1;
			}
		}
		else
		{
			Send1A ("Could not save character to database.\nPlease contact the server administrator.", client);
			client->todc = 1;
		}
#endif
	}
	else
		client->todc = 1;
}
void SendE4_E5(unsigned char slotnum, unsigned char selecting, BANANA* client)
{
	int char_exists = 0;
	MINICHAR* mc;
	
	if ((client->guildcard) && (slotnum < 0x04))
	{
#ifdef NO_SQL
		ds_found = -1;
		for (ds=0;ds<num_characters;ds++)
		{
			if ((character_data[ds]->guildcard == client->guildcard) &&
				(character_data[ds]->slot == slotnum))
			{
				ds_found = ds;
				break;
			}
		}
		if (ds_found != -1)
		{
				char_exists = 1;
				if (!selecting)
				{
					mc = (MINICHAR*) &PacketE5[0x00];
					*(unsigned short*) &PacketE5[0x10] = character_data[ds_found]->data.level;		// Updated level
					memcpy (&PacketE5[0x14], &character_data[ds_found]->data.gcString[0], 0x70 );	// Updated data
					*(unsigned *) &PacketE5[0x84] = character_data[ds_found]->data.playTime;		// Updated playtime
					if ( mc->skinFlag )
					{
						// In case we got bots that crashed themselves...
						mc->skin = 0;
						mc->head = 0;
						mc->hair = 0;
					}
				}
		}


#else
		sprintf (&myQuery[0], "SELECT * from character_data WHERE guildcard='%u' AND slot='%u'", client->guildcard, slotnum );
		//printf ("MySQL query %s\n", myQuery );

		// Check to see if the character exists in that slot.

		if ( ! mysql_query ( myData, &myQuery[0] ) )
		{
			myResult = mysql_store_result ( myData );
			char_exists = (int) mysql_num_rows ( myResult );

			if (char_exists)
			{
				if (!selecting)
				{
					myRow = mysql_fetch_row ( myResult );
					mc = (MINICHAR*) &PacketE5[0x00];
					memcpy (&PacketE5[0x10], myRow[2] + 0x36C, 2 );		// Updated level
					memcpy (&PacketE5[0x14], myRow[2] + 0x378, 0x70 );  // Updated data
					memcpy (&PacketE5[0x84], myRow[2] + 0x3E0, 4 );     // Updated playtime
					if ( mc->skinFlag )
					{
						// In case we got bots that crashed themselves...
						mc->skin = 0;
						mc->head = 0;
						mc->hair = 0;
					}
				}
			}
			mysql_free_result ( myResult );
		}
		else
		{
			Send1A ("Could not check character information.\n\nThere is a problem with the MySQL database.", client);
			client->todc = 1;
			return;
		}
#endif

		if (!selecting)
		{
			if (char_exists)
			{
				PacketE5[0x00] = 0x88;
				PacketE5[0x02] = 0xE5;
				PacketE5[0x03] = 0x00;
				PacketE5[0x08] = slotnum;
				PacketE5[0x0C] = 0x00;
				if (client->sendCheck[SEND_PACKET_E5] < 0x04)
				{
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &PacketE5[0], sizeof (PacketE5));
					client->sendCheck[SEND_PACKET_E5] ++;
				}
				else
					client->todc = 1;
			}
			else
			{
				PacketE4[0x00] = 0x10;
				PacketE4[0x02] = 0xE4;
				PacketE4[0x03] = 0x00;
				PacketE4[0x08] = slotnum;
				PacketE4[0x0C] = 0x02;
				if (client->sendCheck[SEND_PACKET_E4] < 0x04)
				{
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &PacketE4[0], sizeof (PacketE4));
					client->sendCheck[SEND_PACKET_E4]++;
				}
				else
					client->todc = 1;
			}
		}
		else
		{
			if (char_exists)
			{
				PacketE4[0x00] = 0x10;
				PacketE4[0x02] = 0xE4;
				PacketE4[0x03] = 0x00;
				PacketE4[0x08] = slotnum;
				PacketE4[0x0C] = 0x01;
				if ((client->sendCheck[SEND_PACKET_E4] < 0x04) && (client->sendingchars == 0))
				{
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &PacketE4[0], sizeof (PacketE4));
#ifdef NO_SQL
					ds_found = -1;
					for (ds=0;ds<num_security;ds++)
					{
						if (security_data[ds]->guildcard == client->guildcard)
						{
							ds_found = ds;
							security_data[ds]->slotnum = slotnum;
							UpdateDataFile ("security.dat", ds, security_data[ds], sizeof (L_SECURITY_DATA), 0);
							break;
						}
					}

					if (ds_found == -1)
					{
						Send1A ("Could not select character.", client);
						client->todc = 1;
					}
#else
					sprintf (&myQuery[0], "UPDATE security_data SET slotnum = '%u' WHERE guildcard = '%u'", slotnum, client->guildcard );
					if ( mysql_query ( myData, &myQuery[0] ) )
					{
						Send1A ("Could not select character.", client);
						client->todc = 1;
					}
#endif
					client->sendCheck[SEND_PACKET_E4]++;
				}
				else
					client->todc = 1;
			}
			else
				client->todc = 1;
		}
	}
	else
		client->todc = 1;
}

void SendE8 (BANANA* client)
{
	if ((client->guildcard) && (!client->sendCheck[SEND_PACKET_E8]))
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy (client, &PacketE8[0], sizeof (PacketE8));
		client->sendCheck[SEND_PACKET_E8] = 1;
	}
	else
		client->todc = 1;
}

void SendEB (unsigned char subCommand, unsigned char EBOffset, BANANA* client)
{
	unsigned CalcOffset;

	if ((client->guildcard) && (client->sendCheck[SEND_PACKET_EB] < 17))
	{
		client->sendCheck[SEND_PACKET_EB]++;
		switch (subCommand)
		{
		case 0x01:
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &PacketEB01[0], PacketEB01_Total);
			break;
		case 0x02:
			cipher_ptr = &client->server_cipher;
			CalcOffset = (unsigned) EBOffset * 26636;
			if (CalcOffset < PacketEB02_Total)
			{
				if (PacketEB02_Total - CalcOffset >= 26636)
					encryptcopy (client, &PacketEB02[CalcOffset], 26636 );
				else
					encryptcopy (client, &PacketEB02[CalcOffset], PacketEB02_Total - CalcOffset );
			}
			else
				client->todc = 1;
			break;
		}
	}
	else
		client->todc = 1;
}

void SendDC (int sendChecksum, unsigned char PacketNum, BANANA* client)
{
	unsigned gc_ofs = 0, 
		total_guilds = 0;
	unsigned friendid;
	unsigned short sectionid, _class;
	unsigned GCChecksum = 0;
	unsigned ch;
	unsigned CalcOffset;
	int numguilds = 0;
	unsigned to_send;

	if ((client->guildcard) && (client->sendCheck[SEND_PACKET_DC] < 0x04))
	{
		client->sendCheck[SEND_PACKET_DC]++;
		if (sendChecksum)
		{
#ifdef NO_SQL
			for (ds=0;ds<num_guilds;ds++)
			{
				if (guild_data[ds]->accountid == client->guildcard)
				{
					if (total_guilds < 40)
					{
						friendid = guild_data[ds]->friendid;
						sectionid = guild_data[ds]->sectionid;
						_class = guild_data[ds]->pclass;
						for (ch=0;ch<444;ch++)
							client->guildcard_data[gc_ofs+ch] = 0x00;
						*(unsigned*) &client->guildcard_data[gc_ofs] = friendid;
						memcpy (&client->guildcard_data[gc_ofs+0x04], &guild_data[ds]->friendname, 0x18 );
						memcpy (&client->guildcard_data[gc_ofs+0x54], &guild_data[ds]->friendtext, 0xB0 );
						client->guildcard_data[gc_ofs+0x104] = 0x01;
						client->guildcard_data[gc_ofs+0x106] = (unsigned char) sectionid;
						*(unsigned short*) &client->guildcard_data[gc_ofs+0x107] = _class;
						// comment @ 0x10C
						memcpy (&client->guildcard_data[gc_ofs+0x10C], &guild_data[ds]->comment, 0x44 );
						total_guilds++;
						gc_ofs += 444;
					}
					else
						break;
				}
			}
#else
			sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u' ORDER BY priority", client->guildcard );
			//printf ("MySQL query %s\n", myQuery );

			// Check to see if the account has any guild cards.

			if ( ! mysql_query ( myData, &myQuery[0] ) )
			{

				myResult = mysql_store_result ( myData );
				numguilds = (int) mysql_num_rows ( myResult );

				if (numguilds)
				{
					while ((myRow = mysql_fetch_row ( myResult )) && (total_guilds < 40))
					{
						friendid = atoi (myRow[1]);
						sectionid = (unsigned short) atoi (myRow[5]);
						_class = (unsigned short) atoi (myRow[6]);
						for (ch=0;ch<444;ch++)
							client->guildcard_data[gc_ofs+ch] = 0x00;
						*(unsigned*) &client->guildcard_data[gc_ofs] =  friendid;
						memcpy (&client->guildcard_data[gc_ofs+0x04], myRow[2], 0x18 );
						memcpy (&client->guildcard_data[gc_ofs+0x54], myRow[3], 0xB0 );
						client->guildcard_data[gc_ofs+0x104] = 0x01;
						client->guildcard_data[gc_ofs+0x106] = (unsigned char) sectionid;
						*(unsigned short*) &client->guildcard_data[gc_ofs+0x107] = _class;
						// comment @ 0x10C
						memcpy (&client->guildcard_data[gc_ofs+0x10C], myRow[7], 0x44 );
						total_guilds++;
						gc_ofs += 444;
					}
				}
				mysql_free_result ( myResult );
			}
			else
			{
				Send1A ("Could not check guild card information.\n\nThere is a problem with the MySQL database.", client);
				client->todc = 1;
				return;
			}
#endif

			if (total_guilds)
			{
				ch = 0x1F74 + (total_guilds * 444 );
				memcpy ( &PacketDC_Check[0x1F74], &client->guildcard_data[0], total_guilds * 444 );
			}
			else
				ch = 0x1F74;
			for (ch;ch<26624;ch++)
				PacketDC_Check[ch] = 0x00;
			PacketDC_Check[0x06] = 0x01;
			PacketDC_Check[0x07] = 0x02;
			GCChecksum = (unsigned) CalculateChecksum (&PacketDC_Check[0], 54672);
			PacketDC01[0x00] = 0x14;
			PacketDC01[0x02] = 0xDC;
			PacketDC01[0x03] = 0x01;
			PacketDC01[0x08] = 0x01;
			PacketDC01[0x0C] = 0x90;
			PacketDC01[0x0D] = 0xD5;
			*(unsigned *) &PacketDC01[0x10] = GCChecksum;
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &PacketDC01[0], sizeof (PacketDC01));
		}
		else
		{
			CalcOffset = ((unsigned) PacketNum * 26624);
			if (PacketNum > 0x02)
				client->todc = 1;
			else
			{
				if (PacketNum < 0x02)
					to_send = 26640;
				else
					to_send = 1440;
				*(unsigned short*) &PacketDC02[0x00] = to_send;
				PacketDC02[0x02] = 0xDC;
				PacketDC02[0x03] = 0x02;
				PacketDC02[0x0C] = PacketNum;
				memcpy (&client->encryptbuf[0x00], &PacketDC02[0], 0x10);
				memcpy (&client->encryptbuf[0x10], &PacketDC_Check[CalcOffset], to_send);
				cipher_ptr = &client->server_cipher;
				encryptcopy (client, &client->encryptbuf[0x00], to_send);
			}
		}
	}
	else
		client->todc = 1;
}

/* Ship start authentication */

const unsigned char RC4publicKey[32] = {
	103, 196, 247, 176, 71, 167, 89, 233, 200, 100, 044, 209, 190, 231, 83, 42,
	6, 95, 151, 28, 140, 243, 130, 61, 107, 234, 243, 172, 77, 24, 229, 156
};

void ShipSend0F (unsigned char episode, unsigned char part, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x0F;
	ship->encryptbuf[0x01] = episode;
	ship->encryptbuf[0x02] = part;
	switch (episode)
	{
	case 0x01:
		memcpy (&ship->encryptbuf[0x03], &rt_tables_ep1[(sizeof(rt_tables_ep1) >> 3) * part], sizeof(rt_tables_ep1) >> 1);
		break;
	case 0x02:
		memcpy (&ship->encryptbuf[0x03], &rt_tables_ep2[(sizeof(rt_tables_ep2) >> 3) * part], sizeof(rt_tables_ep2) >> 1);
		break;
	case 0x03:
		memcpy (&ship->encryptbuf[0x03], &rt_tables_ep4[(sizeof(rt_tables_ep4) >> 3) * part], sizeof(rt_tables_ep4) >> 1);
		break;
	}
	compressShipPacket ( ship, &ship->encryptbuf[0], 3 + (sizeof (rt_tables_ep1) >> 1) );
}

void ShipSend10 (ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x10;
	ship->encryptbuf[0x01] = 0x00;
	memcpy (&ship->encryptbuf[0x02], &mob_rate[0], 32);
	compressShipPacket ( ship, &ship->encryptbuf[0], 34 );
}

void ShipSend11 (ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x11;
	ship->encryptbuf[0x01] = 0x00;
	ship->encryptbuf[0x02] = 0x00;
	ship->encryptbuf[0x03] = 0x00;
	compressShipPacket ( ship, &ship->encryptbuf[0], 4 );
}


void ShipSend00 (ORANGE* ship)
{
	unsigned char ch, ch2;

	ch2 = 0;

	for (ch=0x18;ch<0x58;ch+=2) // change 32 bytes of the key
	{
		ShipPacket00[ch]   = (unsigned char) rand() % 255;
		ShipPacket00[ch+1] = (unsigned char) rand() % 255;
		ship->key_change [ch2+(ShipPacket00[ch] % 4)] = ShipPacket00[ch+1];
		ch2 += 4;
	}
	compressShipPacket ( ship, &ShipPacket00[0], sizeof (ShipPacket00) );
	// use the public key to get the ship's index first...
	memcpy (&ship->user_key[0], &RC4publicKey[0], 32 );
    prepare_key(&ship->user_key[0], 32, &ship->cs_key);
    prepare_key(&ship->user_key[0], 32, &ship->sc_key);
	ship->crypt_on = 1;
}

/* Ship authentication result */

void ShipSend02 (unsigned char result, ORANGE* ship)
{
	unsigned si,ch,shipNum;
	ORANGE* tempShip;

	ship->encryptbuf [0x00] = 0x02;
	ship->encryptbuf [0x01] = result;
	si = 0xFFFFFFFF;
	if ( result == 0x01 )
	{
		for (ch=0;ch<serverNumShips;ch++)
		{
			shipNum = serverShipList[ch];
			tempShip = ships[shipNum];
			if (tempShip->shipSockfd == ship->shipSockfd)
			{
				si = shipNum;
				ship->shipID = 0xFFFFFFFF - shipNum;
				construct0xA0();
				break;
			}
		}
	}
	*(unsigned *) &ship->encryptbuf[0x02] = si;
	*(unsigned *) &ship->encryptbuf[0x06] = *(unsigned *) &ship->shipAddr[0];
	*(unsigned *) &ship->encryptbuf[0x0A] = quest_numallows;
	memcpy (&ship->encryptbuf[0x0E], quest_allow, quest_numallows * 4 );
	si = 0x0E + ( quest_numallows * 4 );
	*(unsigned *) &ship->encryptbuf[si] = normalName;
	si += 4;
	*(unsigned *) &ship->encryptbuf[si] = localName;
	si += 4;
	*(unsigned *) &ship->encryptbuf[si] = globalName;
	si += 4;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], si );
}

void ShipSend08 (unsigned gcn, ORANGE* ship)
{
	// Tell the other ships this user logged on and to disconnect him/her if they're still active...
	ship->encryptbuf[0x00] = 0x08;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned *) &ship->encryptbuf[0x02] = gcn;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x06 );
}


void ShipSend0D (unsigned char command, ORANGE* ship)
{
	unsigned shipNum;
	ORANGE* tship;

	switch (command)
	{
	case 0x00:
		{
			unsigned ch;
			// Send ship list data.
			unsigned short tempdw;

			tempdw = *(unsigned short*) &PacketA0Data[0];
			// Ship requesting the ship list.
			memcpy (&ship->encryptbuf[0x00], &ship->decryptbuf[0x04], 6);
			memcpy (&ship->encryptbuf[0x06], &PacketA0Data[0], tempdw);
			ship->encryptbuf[0x01] = 0x01;
			tempdw += 6;
			for (ch=0;ch<serverNumShips;ch++)
			{
				shipNum = serverShipList[ch];
				tship = ships[shipNum];
				if ((tship->shipSockfd >= 0) && (tship->authed == 1))
				{
					*(unsigned *) &ship->encryptbuf[tempdw] = tship->shipID;
					tempdw += 4;
					*(unsigned *) &ship->encryptbuf[tempdw] = *(unsigned *) &tship->shipAddr[0];
					tempdw += 4;
					*(unsigned short*) &ship->encryptbuf[tempdw] = (unsigned short) tship->shipPort;
					tempdw += 2;
				}
			}
			//display_packet (&ship->encryptbuf[0x00], tempdw);
			compressShipPacket ( ship, &ship->encryptbuf[0x00], tempdw );
		}
		break;
	default:
		break;
	}
}


void FixItem (ITEM* i)
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

		if ((m->defense < 0) || (m->power < 0) || (m->dex < 0) || (m->mind < 0))
			total_levels = 201;
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


unsigned char gcname[24*2];
unsigned char gctext[176*2];
unsigned short gccomment[45] = {0x305C};
unsigned char check_key[32];
unsigned char check_key2[32];

void ShipProcessPacket (ORANGE* ship)
{
	unsigned cv, shipNum;
	int pass;
	unsigned char sv;
	unsigned shop_checksum;
	int key_exists;
#ifndef NO_CONNECT_TEST
	int tempfd;
	struct sockaddr_in sa;
#endif

	switch (ship->decryptbuf[0x04])
	{
	case 0x01:
		// Authentication
		//
		// Remember to reconstruct the A0 packet upon a successful authentication!
		// Also, reset the playerCount.
		//
		pass = 1;
		for (sv=0x06;sv<0x14;sv++)
			if (ship->decryptbuf[sv] != ShipPacket00[sv-0x04])
			{
				// Yadda
				pass = 0;
				break;
			}
		if (pass == 0)
		{
			printf ("Ship connected but invalid authorization values were received.\n");
			ShipSend02 (0x00, ship);
			ship->todc = 1;
		}
		else
		{
			//unsigned ch, sameIP;
			unsigned ch2, shipOK;
			//ORANGE* tship;

			shipOK = 1;

			memcpy (&ship->name[0], &ship->decryptbuf[0x2C], 12);
			ship->name[13] = 0x00;
			ship->playerCount = *(unsigned *) &ship->decryptbuf[0x38];
			if (ship->decryptbuf[0x3C] == 0x00)
				*(unsigned *) &ship->shipAddr[0] = *(unsigned *) &ship->listenedAddr[0];
			else
				*(unsigned *) &ship->shipAddr[0] = *(unsigned *) &ship->decryptbuf[0x3C];
			ship->shipAddr[5] = 0;
			ship->shipPort = *(unsigned short*) &ship->decryptbuf[0x40];

			/*
			for (ch=0;ch<serverNumShips;ch++)
			{
				shipNum = serverShipList[ch];
				tship = ships[shipNum];
				if ((tship->shipSockfd >= 0) && (tship->authed == 1))
				{
					sameIP = 1;
					for (ch2=0;ch2<4;ch2++)
						if (ship->shipAddr[ch2] != tship->shipAddr[ch2])
							sameIP = 0;
					if (sameIP == 1)
					{
						printf ("Ship IP address was already registered.  Disconnecting...\n");
						ShipSend02 (0x02, ship);
						ship->todc = 1;
						shipOK = 0;
					}
				}
			}*/

			if (shipOK == 1)
			{
				// if (ship->shipAddr[0] == 10)  shipOK = 0;
				// if ((ship->shipAddr[0] == 192) && (ship->shipAddr[1] == 168)) shipOK = 0;
				// if ((ship->shipAddr[0] == 127) && (ship->shipAddr[1] == 0) &&
				//	   (ship->shipAddr[2] == 0)   && (ship->shipAddr[3] == 1)) shipOK = 0;

				shop_checksum = *(unsigned *) &ship->decryptbuf[0x42];
				memcpy (&check_key[0], &ship->decryptbuf[0x4A], 32);

				if ( shop_checksum != 0xa3552fda )
				{
					ShipSend02 ( 0x04, ship );
					ship->todc = 1;
					shipOK = 0;
				}
				else
					if ( shipOK )
					{
						ship->key_index = *(unsigned *) &ship->decryptbuf[0x46];

						// update max ship key count on the fly
#ifdef NO_SQL
						max_ship_keys = 0;
						for (ds=0;ds<num_shipkeys;ds++)
						{
							if (ship_data[ds]->idx >= max_ship_keys)
								max_ship_keys = ship_data[ds]->idx;
						}
#else

						sprintf (&myQuery[0], "SELECT * from ship_data" );

						if ( ! mysql_query ( myData, &myQuery[0] ) )
						{
							unsigned key_rows;

							myResult = mysql_store_result ( myData );
							key_rows = (int) mysql_num_rows ( myResult );
							max_ship_keys = 0;
							while ( key_rows )
							{
								myRow = mysql_fetch_row ( myResult );
								if ( (unsigned) atoi ( myRow[1] ) > max_ship_keys )
									max_ship_keys = atoi ( myRow[1] );
								key_rows --;
							}
							mysql_free_result ( myResult );
						}
						else
						{
							ship->key_index = 0xFFFF; // MySQL problem, don't allow the ship to connect.
							printf ("Unable to query the key database.\n");
						}

#endif

						if ( ( ship->key_index ) && ( ship->key_index <= max_ship_keys ) )
						{
							if ( keys_in_use [ ship->key_index ] )  // key already in use?
							{
								ShipSend02 ( 0x06, ship );
								ship->todc = 1;
								shipOK = 0;
							}
							else
							{
								key_exists = 0;
#ifdef NO_SQL
								ds_found = -1;

								for (ds=0;ds<num_shipkeys;ds++)
								{
									if (ship_data[ds]->idx == ship->key_index)
									{
										ds_found = ds;
										for (ch2=0;ch2<32;ch2++)
											if (ship_data[ds]->rc4key[ch2] != check_key[ch2])
											{
												ds_found = -1;
												break;
											}
										break;
									}
								}

								if (ds_found != -1)
									key_exists = 1;
								else
								{
									ShipSend02 (0x05, ship); // Ship key doesn't exist
									ship->todc = 1;
									shipOK = 0;
								}
#else
								sprintf (&myQuery[0], "SELECT * from ship_data WHERE idx='%u'", ship->key_index );

								if ( ! mysql_query ( myData, &myQuery[0] ) )
								{
									myResult = mysql_store_result ( myData );
									key_exists = (int) mysql_num_rows ( myResult );
									myRow = mysql_fetch_row ( myResult );
									memcpy (&check_key2[0], myRow[0], 32 ); // 1024-bit key
									for (ch2=0;ch2<32;ch2++)
										if (check_key2[ch2] != check_key[ch2])
										{
											key_exists  = 0;
											break;
										}
								}
								else
								{
									ShipSend02 (0x05, ship); // Could not query MySQL or ship key doesn't exist
									mysql_free_result ( myResult );
									ship->todc = 1;
									shipOK = 0;
								}
#endif

									if ( key_exists )
									{
#ifndef NO_CONNECT_TEST
										tempfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

										memset (&sa, 0, sizeof(sa));
										sa.sin_family = AF_INET;
										*(unsigned *) &sa.sin_addr.s_addr = *(unsigned *) &ship->shipAddr[0];
										sa.sin_port = htons((unsigned short) ship->shipPort);

										if ((tempfd >= 0) && (connect(tempfd, (struct sockaddr*) &sa, sizeof(sa)) >= 0))
										{
											closesocket ( tempfd );
#endif

											printf ("Ship %s (%u.%u.%u.%u:%u) has been successfully registered.\nPlayer count: %u\nShip key index: %u\n", ship->name, 
												ship->shipAddr[0], ship->shipAddr[1], ship->shipAddr[2], ship->shipAddr[3], ship->shipPort, 
												ship->playerCount, ship->key_index );

											ship->authed = 1;
											ShipSend02 (0x01, ship);  // At this point, we should change keys...

#ifdef NO_SQL
											memcpy (&ship->user_key[0], &ship_data[ds_found]->rc4key, 128);
#else
											memcpy (&ship->user_key[0], myRow[0], 128 ); // 1024-bit key
											mysql_free_result ( myResult );
#endif

											// change keys

											for (ch2=0;ch2<128;ch2++)
												if (ship->key_change[ch2] != -1)
													ship->user_key[ch2] = (unsigned char) ship->key_change[ch2]; // update the key

											prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->cs_key);
											prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->sc_key);
#ifndef NO_CONNECT_TEST
										}
										else
										{
											debug ("Could not make a connection to the registering ship...  Disconnecting...\n");
											ShipSend02 (0x03, ship);
											ship->todc = 1;
											shipOK = 0;
										}
#endif

									}
									else
									{
										ShipSend02 ( 0x05, ship );
										ship->todc = 1;
										shipOK = 0;
									}

							}
						}
						else
							initialize_ship ( ship );
					}
			}
		}
		break;
	case 0x03:
		break;
	case 0x04:
		switch (ship->decryptbuf[0x05])
		{
		case 0x00:
			{
				// Send full player data here.

				unsigned guildcard;
				unsigned short slotnum;
				unsigned size;
				int char_exists = 0;
				int bank_exists = 0;
				int teamid;
				unsigned short privlevel;
				CHARDATA* PlayerData;
				unsigned shipid;
				int sockfd;

				guildcard = *(unsigned*) &ship->decryptbuf[0x06];
				slotnum = *(unsigned short*) &ship->decryptbuf[0x0A];
				sockfd = *(int *) &ship->decryptbuf[0x0C];
				shipid = *(unsigned*) &ship->decryptbuf[0x10];

				ship->encryptbuf[0x00] = 0x04;

				*(unsigned*) &ship->encryptbuf[0x02] = *(unsigned*)  &ship->decryptbuf[0x06];
				*(unsigned short*) &ship->encryptbuf[0x06] = *(unsigned short*) &ship->decryptbuf[0x0A];
				*(unsigned*) &ship->encryptbuf[0x08] = *(unsigned*) &ship->decryptbuf[0x0C];

#ifdef NO_SQL
				size = 0x0C;

				ds_found = -1;

				for (ds=0;ds<num_characters;ds++)
				{
					if ((character_data[ds]->guildcard == guildcard) && 
						(character_data[ds]->slot == slotnum))
					{
						ds_found = ds;
						break;
					}
				}

				if (ds_found == -1)
					ship->encryptbuf[0x01] = 0x02; // fail
				else
				{
					PlayerData = (CHARDATA*) &ship->encryptbuf[0x0C];
					memcpy (&ship->encryptbuf[0x0C], &character_data[ds_found]->data, sizeof (CHARDATA) );
					size += sizeof (CHARDATA);
					ship->encryptbuf[0x01] = 0x01; // success

					// Copy common bank into packet.

					ds_found = -1;

					for (ds=0;ds<num_bankdata;ds++)
					{
						if (bank_data[ds]->guildcard == guildcard)
						{
							ds_found = ds;
							memcpy (&ship->encryptbuf[0x0C+sizeof(CHARDATA)], &bank_data[ds]->common_bank, sizeof(BANK));
							break;
						}
					}

					if (ds_found == -1)
					{
						// Common bank needs to be created.

						bank_data[num_bankdata] = malloc (sizeof(L_BANK_DATA));
						bank_data[num_bankdata]->guildcard = guildcard;
						memcpy (&bank_data[num_bankdata]->common_bank, &empty_bank, sizeof (BANK));
						memcpy (&ship->encryptbuf[0x0C+sizeof(CHARDATA)], &empty_bank, sizeof(BANK));
						UpdateDataFile ("bank.dat", num_bankdata, bank_data[num_bankdata], sizeof (L_BANK_DATA), 1);
						num_bankdata++;
					}

					size += sizeof (BANK);

					ds_found = 1;
					for (ds=0;ds<num_accounts;ds++)
					{
						if (account_data[ds]->guildcard == guildcard)
						{
							memcpy (&account_data[ds]->lastchar[0], &PlayerData->name[0], 24);
							teamid = account_data[ds]->teamid;
							privlevel = account_data[ds]->privlevel;
							UpdateDataFile ("account.dat", ds, account_data[ds], sizeof (L_ACCOUNT_DATA), 0);
							break;
						}
					}

					if (teamid != -1)
					{
						//debug ("Retrieving some shit... ");
						// Store the team information in the E7 packet...
						PlayerData->guildCard2 = PlayerData->guildCard;
						PlayerData->teamID = (unsigned) teamid;
						PlayerData->privilegeLevel = privlevel;
						ds_found = -1;
						for (ds=0;ds<num_teams;ds++)
						{
							if (team_data[ds]->teamid == teamid)
							{
								PlayerData->teamName[0] = 0x09;
								PlayerData->teamName[2] = 0x45;
								memcpy ( &PlayerData->teamName[4], team_data[ds]->name, 24 );
								memcpy ( &PlayerData->teamFlag[0], team_data[ds]->flag, 2048 );
								break;
							}
						}
					}
					else
						memset ( &PlayerData->guildCard2, 0, 0x83C );

					PlayerData->unknown15 = 0x00986C84; // ??
					memset ( &PlayerData->teamRewards[0], 0xFF, 4 );
				}

				compressShipPacket ( ship, &ship->encryptbuf[0x00], size );
#else

				sprintf (&myQuery[0], "SELECT * from character_data WHERE guildcard='%u' AND slot='%u'", guildcard, slotnum );

				//debug ("MySQL query executing ... %s ", &myQuery[0] );

				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					char_exists = (int) mysql_num_rows ( myResult );

					size = 0x0C;

					PlayerData = (CHARDATA*) &ship->encryptbuf[0x0C];

					if (char_exists)
					{
						myRow = mysql_fetch_row ( myResult );
						memcpy (&ship->encryptbuf[0x0C], myRow[2], sizeof (CHARDATA) );
						size += sizeof (CHARDATA);
						ship->encryptbuf[0x01] = 0x01; // success

						// Get the common bank or create it

						sprintf (&myQuery[0], "SELECT * from bank_data WHERE guildcard='%u'", guildcard );

						//debug ("MySQL query executing ... %s ", &myQuery[0] );

						if ( ! mysql_query ( myData, &myQuery[0] ) )
						{
							myResult = mysql_store_result ( myData );
							bank_exists = (int) mysql_num_rows ( myResult );

							if (bank_exists)
							{
								// Copy bank
								myRow = mysql_fetch_row ( myResult );
								memcpy (&ship->encryptbuf[0x0C+sizeof(CHARDATA)], myRow[1], sizeof (BANK));
							}
							else
							{
								// Create bank
								memcpy (&ship->encryptbuf[0x0C+sizeof(CHARDATA)], &empty_bank, sizeof (BANK));
								mysql_real_escape_string ( myData, &E7chardata[0], (unsigned char*) &empty_bank, sizeof (BANK) );

								sprintf (&myQuery[0], "INSERT into bank_data (guildcard, data) VALUES ('%u','%s')", guildcard, (char*) &E7chardata[0] );
								if ( mysql_query ( myData, &myQuery[0] ) )
								{
									debug ("Could not create common bank for guild card %u.", guildcard);
									return;
								}
							}
						}
						else
						{
							// Something goofed up, let's just copy the blank bank.
							memcpy (&ship->encryptbuf[0x0C+sizeof(CHARDATA)], &empty_bank, sizeof (BANK));
						}

						size += sizeof (BANK);


						// Update the last used character info...

						mysql_real_escape_string ( myData, &E7chardata[0], (unsigned char*) &PlayerData->name[0], 24 );
						sprintf (&myQuery[0], "UPDATE account_data SET lastchar = '%s' WHERE guildcard = '%u'", (char*) &E7chardata[0], PlayerData->guildCard );
						mysql_query ( myData, &myQuery[0] );

					}
					else
						ship->encryptbuf[0x01] = 0x02; // fail

					mysql_free_result ( myResult );

					if ( ship->encryptbuf[0x01] != 0x02 )
					{
						sprintf (&myQuery[0], "SELECT teamid,privlevel from account_data WHERE guildcard='%u'", guildcard );

						if ( ! mysql_query ( myData, &myQuery[0] ) )
						{
							myResult = mysql_store_result ( myData );
							myRow = mysql_fetch_row ( myResult );

							teamid = atoi (myRow[0]);
							privlevel = atoi (myRow[1]);

							mysql_free_result ( myResult );

							if (teamid != -1)
							{
								//debug ("Retrieving some shit... ");
								// Store the team information in the E7 packet...
								PlayerData->guildCard2 = PlayerData->guildCard;
								PlayerData->teamID = (unsigned) teamid;
								PlayerData->privilegeLevel = privlevel;
								sprintf (&myQuery[0], "SELECT name,flag from team_data WHERE teamid = '%i'", teamid);
								if ( ! mysql_query ( myData, &myQuery[0] ) )
								{
									myResult = mysql_store_result ( myData );
									myRow = mysql_fetch_row ( myResult );
									PlayerData->teamName[0] = 0x09;
									PlayerData->teamName[2] = 0x45;
									memcpy ( &PlayerData->teamName[4], myRow[0], 24 );
									memcpy ( &PlayerData->teamFlag[0], myRow[1], 2048);

									mysql_free_result ( myResult );
								}
							}
							else
								memset ( &PlayerData->guildCard2, 0, 0x83C );
						}
						else
							ship->encryptbuf[0x01] = 0x02; // fail

						PlayerData->unknown15 = 0x00986C84; // ??
						memset ( &PlayerData->teamRewards[0], 0xFF, 4 );
					}

					compressShipPacket ( ship, &ship->encryptbuf[0x00], size );
				}
				else
					debug ("Could not select character information for user %u", guildcard);
#endif
			}
			break;
		case 0x02:
			{
				unsigned guildcard, ch2;
				unsigned short slotnum;
				CHARDATA* character;
				unsigned short maxFace, maxHair, maxHairColorRed, maxHairColorBlue, maxHairColorGreen, 
					maxCostume, maxSkin, maxHead;

				guildcard = *(unsigned*) &ship->decryptbuf[0x06];
				slotnum = *(unsigned short*) &ship->decryptbuf[0x0A];

				character = (CHARDATA*) &ship->decryptbuf[0x0C];

				// Update common bank (A common bank SHOULD exist since they've logged on...)

#ifdef NO_SQL

				for (ds=0;ds<num_bankdata;ds++)
				{
					if (bank_data[ds]->guildcard == guildcard)
					{
						memcpy (&bank_data[ds]->common_bank, &ship->decryptbuf[0x0C+sizeof(CHARDATA)], sizeof (BANK));
						UpdateDataFile ("bank.dat", ds, bank_data[ds], sizeof (L_BANK_DATA), 0);
						break;
					}
				}

#else

				mysql_real_escape_string ( myData, &E7chardata[0], (unsigned char*) &ship->decryptbuf[0x0C+sizeof(CHARDATA)], sizeof (BANK) );
				sprintf (&myQuery[0], "UPDATE bank_data set data = '%s' WHERE guildcard = '%u'", (char*) &E7chardata[0], guildcard );
				if ( mysql_query ( myData, &myQuery[0] ) )
				{
					debug ("Could not save common bank for guild card %u.", guildcard);
					return;
				}

#endif
				// Repair malformed data

				character->name[0] = 0x09; // Filter colored names
				character->name[1] = 0x00;
				character->name[2] = 0x45;
				character->name[3] = 0x00;
				character->name[22] = 0x00;
				character->name[23] = 0x00; // Truncate names too long.
				if (character->level > 199) // Bad levels?
					character->level = 199;

				if ((character->_class == CLASS_HUMAR) ||
					(character->_class == CLASS_HUNEWEARL) ||
					(character->_class == CLASS_RAMAR) ||
					(character->_class == CLASS_RAMARL) ||
					(character->_class == CLASS_FOMARL) ||
					(character->_class == CLASS_FONEWM) ||
					(character->_class == CLASS_FONEWEARL) ||
					(character->_class == CLASS_FOMAR))
				{
					maxFace = 0x05;
					maxHair = 0x0A;
					maxHairColorRed = 0xFF;
					maxHairColorBlue = 0xFF;
					maxHairColorGreen = 0xFF;
					maxCostume = 0x11;
					maxSkin = 0x03;
					maxHead = 0x00;
				}
				else
				{
					maxFace = 0x00;
					maxHair = 0x00;
					maxHairColorRed = 0x00;
					maxHairColorBlue = 0x00;
					maxHairColorGreen = 0x00;
					maxCostume = 0x00;
					maxSkin = 0x18;
					maxHead = 0x04;
				}
				character->nameColorTransparency = 0xFF;
				if (character->sectionID > 0x09)
					character->sectionID = 0x00;
				if (character->proportionX > 0x3F800000)
					character->proportionX = 0x3F800000;
				if (character->proportionY > 0x3F800000)
					character->proportionY = 0x3F800000;
				if (character->face > maxFace)
					character->face = 0x00;
				if (character->hair > maxHair)
					character->hair = 0x00;
				if (character->hairColorRed > maxHairColorRed)
					character->hairColorRed = 0x00;
				if (character->hairColorBlue > maxHairColorBlue)
					character->hairColorBlue = 0x00;
				if (character->hairColorGreen > maxHairColorGreen)
					character->hairColorGreen = 0x00;
				if (character->costume > maxCostume)
					character->costume = 0x00;
				if (character->skin > maxSkin)
					character->skin = 0x00;
				if (character->head > maxHead)
					character->head = 0x00;

				// Let's fix hacked mags and weapons
				for (ch2=0;ch2<character->inventoryUse;ch2++)
				{
					if (character->inventory[ch2].in_use)
						FixItem ( &character->inventory[ch2].item );
				}

				for (ch2=0;ch2<character->bankUse;ch2++)
				{
					if (character->inventory[ch2].in_use)
						FixItem ( (ITEM*) &character->bankInventory[ch2] );
				}

#ifdef NO_SQL
				for (ds=0;ds<num_characters;ds++)
				{
					if ((character_data[ds]->guildcard == guildcard) &&
						(character_data[ds]->slot == slotnum))
					{
						memcpy (&character_data[ds]->data, &ship->decryptbuf[0x0C], sizeof (CHARDATA));
						UpdateDataFile ("character.dat", ds, character_data[ds], sizeof (L_CHARACTER_DATA), 0);
						break;
					}
				}
#else
				mysql_real_escape_string ( myData, &E7chardata[0], (unsigned char*) &ship->decryptbuf[0x0C], sizeof (CHARDATA) );
				sprintf (&myQuery[0], "UPDATE character_data set data = '%s' WHERE guildcard = '%u' AND slot = '%u'", (char*) &E7chardata[0], guildcard, slotnum );
				if ( mysql_query ( myData, &myQuery[0] ) )
				{
					debug ("Could not save character information for guild card user %u (%02x)", guildcard, slotnum);
					return;
				}
				else
#endif
				{
					ship->encryptbuf[0x00] = 0x04;
					ship->encryptbuf[0x01] = 0x03;
					*(unsigned*) &ship->encryptbuf[0x02] = guildcard;
					// Send the OK, data saved.
					compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x06 );
				}
#ifdef NO_SQL
				for (ds=0;ds<num_keydata;ds++)
				{
					if (key_data[ds]->guildcard == guildcard)
					{
						memcpy (&key_data[ds]->controls, &ship->decryptbuf[0x2FCC], 420);
						UpdateDataFile ("keydata.dat", ds, key_data[ds], sizeof (L_KEY_DATA), 0);
						break;
					}
				}
#else
				mysql_real_escape_string ( myData, &E7chardata[0], (unsigned char*) &ship->decryptbuf[0x2FCC], 420 );
				sprintf (&myQuery[0], "UPDATE key_data set controls = '%s' WHERE guildcard = '%u'", (char*) &E7chardata[0], guildcard );
				if ( mysql_query ( myData, &myQuery[0] ) )
					debug ("Could not save control information for guild card user %u", guildcard);
#endif
			}
		}
		break;
	case 0x05:
		break;
	case 0x06:
		break;
	case 0x07:
		switch (ship->decryptbuf[0x05])
		{
		case 0x00:
			// Add a guild card here.
			{
				unsigned clientGcn, friendGcn;
				int gc_priority;
#ifdef NO_SQL
				unsigned ch2;
#endif
#ifndef NO_SQL
				unsigned num_gcs;
				int gc_exists;
				unsigned char friendSecID, friendClass;
#endif

				clientGcn = *(unsigned*) &ship->decryptbuf[0x06];
				friendGcn = *(unsigned*) &ship->decryptbuf[0x0A];

#ifdef NO_SQL
				ds_found = -1;
				for (ds=0;ds<num_guilds;ds++)
				{
					if ((guild_data[ds]->accountid == clientGcn) &&
						(guild_data[ds]->friendid == friendGcn))
					{
						ds_found = ds;
						break;
					}
				}
				gc_priority = 0;
				ch2 = 0;
				free_record = -1;
				for (ds=0;ds<num_guilds;ds++)
				{
					if (guild_data[ds]->accountid == clientGcn)
					{
						ch2++;
						if (guild_data[ds]->priority > gc_priority)
							gc_priority = guild_data[ds]->priority;
					}
					if (guild_data[ds]->accountid == 0)
						free_record = ds;
				}
				gc_priority++;
				if ( ( ch2 < 40 ) || ( ds_found != -1 ) )
				{
					if ( ds_found != -1 )
						new_record = 0;
					else
					{
						if (free_record != -1)
							ds_found = free_record;
						else
						{
							new_record = 1;
							ds_found = num_guilds;
							guild_data[num_guilds++] = malloc ( sizeof (L_GUILD_DATA) );
						}
					}

					guild_data[ds_found]->sectionid = ship->decryptbuf[0xD6];
					guild_data[ds_found]->pclass = ship->decryptbuf[0xD7];

					gccomment[44] = 0x0000;

					guild_data[ds_found]->accountid = clientGcn;
					guild_data[ds_found]->friendid = friendGcn;
					guild_data[ds_found]->reserved = 257;
					memcpy (&guild_data[ds_found]->friendname[0], &ship->decryptbuf[0x0E], 24);
					memcpy (&guild_data[ds_found]->friendtext[0], &ship->decryptbuf[0x26], 176);
					memcpy (&guild_data[ds_found]->comment[0], &gccomment[0], 90);
					guild_data[ds_found]->priority = gc_priority;
					UpdateDataFile ("guild.dat", ds_found, guild_data[ds_found], sizeof(L_GUILD_DATA), new_record);
				}
				else
				{
					// Card list full.
					ship->encryptbuf[0x00] = 0x07;
					ship->encryptbuf[0x01] = 0x00;
					*(unsigned*) &ship->encryptbuf[0x02] = clientGcn;
					compressShipPacket (ship, &ship->encryptbuf[0x00], 6);
				}

#else

				// Delete guild card if it exists...

				sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u' AND friendid='%u'", clientGcn, friendGcn );

				//printf ("MySQL query %s\n", myQuery );

				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					gc_exists = (int) mysql_num_rows ( myResult );
					mysql_free_result ( myResult );
					if ( gc_exists )
					{
						sprintf (&myQuery[0], "DELETE from guild_data WHERE accountid='%u' AND friendid='%u'", clientGcn, friendGcn );
						mysql_query ( myData, &myQuery[0] );
					}
				}
				else
				{
					debug ("Could not delete existing guild card information for user %u", clientGcn);
					return;
				}

				gc_priority = 1;

				sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u'", clientGcn );

				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );

					num_gcs = (int) mysql_num_rows ( myResult );
					mysql_free_result ( myResult );

					if (num_gcs)
					{
						sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u' ORDER by priority DESC", clientGcn );
						if ( ! mysql_query ( myData, &myQuery[0] ) )
						{
							myResult = mysql_store_result ( myData );
							myRow = mysql_fetch_row ( myResult );
							gc_priority = atoi ( myRow[8] ) + 1;
							mysql_free_result ( myResult );
						}
					}
				}
				else
				{
					debug ("Could not select existing guild card information for user %u", clientGcn);
					return;
				}

				if ( num_gcs < 40 )
				{
					mysql_real_escape_string (myData, &gcname[0], &ship->decryptbuf[0x0E], 24);
					mysql_real_escape_string (myData, &gctext[0], &ship->decryptbuf[0x26], 176);

					friendSecID = ship->decryptbuf[0xD6];
					friendClass = ship->decryptbuf[0xD7];

					gccomment[44] = 0x0000;

					sprintf (&myQuery[0], "INSERT INTO guild_data (accountid,friendid,friendname,friendtext,sectionid,class,comment,priority) VALUES ('%u','%u','%s','%s','%u','%u','%s','%u')", 
						clientGcn, friendGcn, (char*) &gcname[0], (char*) &gctext[0], friendSecID, friendClass, (char*) &gccomment[0], gc_priority );

					if ( mysql_query ( myData, &myQuery[0] ) )
						debug ("Could not insert guild card into database for user %u", clientGcn);
				}
				else
				{
					// Card list full.
					ship->encryptbuf[0x00] = 0x07;
					ship->encryptbuf[0x01] = 0x00;
					*(unsigned*) &ship->encryptbuf[0x02] = clientGcn;
					compressShipPacket (ship, &ship->encryptbuf[0x00], 6);
				}
#endif
			}
			break;
		case 0x01:
			// Delete a guild card here.
			{
				unsigned clientGcn, deletedGcn;

				clientGcn = *(unsigned*) &ship->decryptbuf[0x06];
				deletedGcn = *(unsigned*) &ship->decryptbuf[0x0A];

#ifdef NO_SQL
				for (ds=0;ds<num_guilds;ds++)
				{
					if ((guild_data[ds]->accountid == clientGcn) &&
						(guild_data[ds]->friendid == deletedGcn))
					{
						guild_data[ds]->accountid = 0;
						UpdateDataFile ("guild.dat", ds, guild_data[ds], sizeof (L_GUILD_DATA), 0);
						break;
					}
				}
#else

				sprintf (&myQuery[0], "DELETE from guild_data WHERE accountid = '%u' AND friendid = '%u'", clientGcn, deletedGcn );
				if ( mysql_query ( myData, &myQuery[0] ) )
					debug ("Could not delete guild card for user %u", clientGcn );
#endif
			}
			break;
		case 0x02:
			// Modify guild card comment.
			{
				unsigned clientGcn, friendGcn;

				clientGcn = *(unsigned*) &ship->decryptbuf[0x06];
				friendGcn = *(unsigned*) &ship->decryptbuf[0x0A];
#ifdef NO_SQL
				for (ds=0;ds<num_guilds;ds++)
				{
					if ((guild_data[ds]->accountid == clientGcn) &&
						(guild_data[ds]->friendid == friendGcn))
					{
						memcpy (&guild_data[ds]->comment[0], &ship->decryptbuf[0x0E], 0x44);
						guild_data[ds]->comment[34] = 0; // ??
						UpdateDataFile ("guild.dat", ds, guild_data[ds], sizeof(L_GUILD_DATA), 0);
					}
				}
#else
				mysql_real_escape_string (myData, &gctext[0], &ship->decryptbuf[0x0E], 0x44);

				sprintf (&myQuery[0], "UPDATE guild_data set comment = '%s' WHERE accountid = '%u' AND friendid = '%u'", (char*) &gctext[0], clientGcn, friendGcn );

				if ( mysql_query ( myData, &myQuery[0] ) )
					debug ("Could not update guild card comment for user %u", clientGcn );
#endif
			}
			break;
		case 0x03:
			// Sort guild card
			{
				unsigned clientGcn, gcn1, gcn2;
				int priority1, priority2, priority_save;
#ifdef NO_SQL
				L_GUILD_DATA tempgc;
#endif

				priority1 = -1;
				priority2 = -1;

				clientGcn = *(unsigned*) &ship->decryptbuf[0x06];
				gcn1 = *(unsigned*) &ship->decryptbuf[0x0A];
				gcn2 = *(unsigned*) &ship->decryptbuf[0x0E];

#ifdef NO_SQL
				for (ds=0;ds<num_guilds;ds++)
				{
					if ((guild_data[ds]->accountid == clientGcn) &&
						(guild_data[ds]->friendid == gcn1))
					{
						priority1 = guild_data[ds]->priority;
						ds_found = ds;
						break;
					}
				}
#else
				sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u' AND friendid='%u'", clientGcn, gcn1 );

				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					if ( mysql_num_rows ( myResult ) )
					{
						myRow = mysql_fetch_row ( myResult );
						priority1 = atoi (myRow[8]);
					}
					mysql_free_result ( myResult );
				}
				else
				{
					debug ("Could not select existing guild card information for user %u", clientGcn);
					return;
				}
#endif

#ifdef NO_SQL
				for (ds=0;ds<num_guilds;ds++)
				{
					if ((guild_data[ds]->accountid == clientGcn) &&
						(guild_data[ds]->friendid == gcn2))
					{
						priority2 = guild_data[ds]->priority;
						ds2 = ds;
						break;
					}
				}
#else
				sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u' AND friendid='%u'", clientGcn, gcn2 );

				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					if ( mysql_num_rows ( myResult ) )
					{
						myRow = mysql_fetch_row ( myResult );
						priority2 = atoi (myRow[8]);
					}
					mysql_free_result ( myResult );
				}
				else
				{
					debug ("Could not select existing guild card information for user %u", clientGcn);
					return;
				}
#endif

				if ((priority1 != -1) && (priority2 != -1))
				{
					priority_save = priority1;
					priority1 = priority2;
					priority2 = priority_save;

#ifdef NO_SQL
					guild_data[ds_found]->priority = priority2;
					guild_data[ds2]->priority = priority_save;
					UpdateDataFile ("guild.dat", ds_found, guild_data[ds2], sizeof (L_GUILD_DATA), 0);
					UpdateDataFile ("guild.dat", ds2, guild_data[ds_found], sizeof (L_GUILD_DATA), 0);
					memcpy (&tempgc, guild_data[ds_found], sizeof (L_GUILD_DATA));
					memcpy (guild_data[ds_found], guild_data[ds2], sizeof (L_GUILD_DATA));
					memcpy (guild_data[ds2], &tempgc, sizeof (L_GUILD_DATA));
#else
					sprintf (&myQuery[0], "UPDATE guild_data SET priority = '%u' WHERE accountid = '%u' AND friendid = '%u'", priority1, clientGcn, gcn1 );
					if ( mysql_query ( myData, &myQuery[0] ) )
						debug ("Could not update guild card sort information for user %u", clientGcn);
					sprintf (&myQuery[0], "UPDATE guild_data SET priority = '%u' WHERE accountid = '%u' AND friendid = '%u'", priority2, clientGcn, gcn2 );
					if ( mysql_query ( myData, &myQuery[0] ) )
						debug ("Could not update guild card sort information for user %u", clientGcn);
#endif
				}
			}
			break;
		}
		break;
	case 0x08:
		switch (ship->decryptbuf[0x05])
		{
		case 0x01:
			// Ship requesting a guild search
			{
				unsigned clientGcn, friendGcn, ch, teamid;
				int gc_exists = 0;
				ORANGE* tship;

				friendGcn = *(unsigned*) &ship->decryptbuf[0x06];
				clientGcn = *(unsigned*) &ship->decryptbuf[0x0A];
				teamid = *(unsigned*) &ship->decryptbuf[0x12];

				// First let's be sure our friend has this person's guild card....

#ifdef NO_SQL
				for (ds=0;ds<num_guilds;ds++)
				{
					if ((guild_data[ds]->accountid == clientGcn) &&
						(guild_data[ds]->friendid == friendGcn))
					{
						gc_exists = 1;
						break;
					}
				}
#else

				sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u' AND friendid='%u'", clientGcn, friendGcn );

				//printf ("MySQL query %s\n", myQuery );

				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					gc_exists = (int) mysql_num_rows ( myResult );
					mysql_free_result ( myResult );
				}
				else
				{
					debug ("Could not select existing guild card information for user %u", clientGcn);
					return;
				}
#endif

#ifdef NO_SQL
				if ( ( gc_exists == 0 ) && ( teamid != 0 ) )
				{
					for (ds=0;ds<num_accounts;ds++)
					{
						if ((account_data[ds]->guildcard == friendGcn) &&
							(account_data[ds]->teamid == teamid))
						{
							gc_exists = 1;
							break;
						}
					}
				}
#else
				if ( ( gc_exists == 0 ) && ( teamid != 0 ) )
				{
					// Well, they don't appear to have this person's guild card... so let's check the team list...
					sprintf (&myQuery[0], "SELECT * from account_data WHERE guildcard='%u' AND teamid='%u'", friendGcn, teamid );

					//printf ("MySQL query %s\n", myQuery );

					if ( ! mysql_query ( myData, &myQuery[0] ) )
					{
						myResult = mysql_store_result ( myData );
						gc_exists = (int) mysql_num_rows ( myResult );
						mysql_free_result ( myResult );
					}
					else
					{
						debug ("Could not select account information for user %u", friendGcn);
						return;
					}
				}
#endif

				if ( gc_exists )
				{
					// OK!  Let's tell the ships to do the search...
					for (ch=0;ch<serverNumShips;ch++)
					{
						shipNum = serverShipList[ch];
						tship = ships[shipNum];
						if ((tship->shipSockfd >= 0) && (tship->authed == 1))
							compressShipPacket ( tship, &ship->decryptbuf[0x04], 0x1D);
					}
				}
			}
			break;
		case 0x02:
			// Got a hit on a guild search
			cv = *(unsigned*) &ship->decryptbuf[0x0A];
			cv--;
			if (cv < serverMaxShips)
			{
				ORANGE* tship;

				tship = ships[cv];
				if ((tship->shipSockfd >= 0) && (tship->authed == 1))
					compressShipPacket ( tship, &ship->decryptbuf[0x04], 0x140 );
			}
			break;
		case 0x03:
			// Send mail
			{
				unsigned clientGcn, friendGcn, ch, teamid;
				int gc_exists = 0;
				ORANGE* tship;

				friendGcn = *(unsigned*) &ship->decryptbuf[0x36];
				clientGcn = *(unsigned*) &ship->decryptbuf[0x12];
				teamid = *(unsigned*) &ship->decryptbuf[0x462];

				// First let's be sure our friend has this person's guild card....

#ifdef NO_SQL
				for (ds=0;ds<num_guilds;ds++)
				{
					if ((guild_data[ds]->accountid == clientGcn) &&
						(guild_data[ds]->friendid == friendGcn))
					{
						gc_exists = 1;
						break;
					}
				}
#else
				sprintf (&myQuery[0], "SELECT * from guild_data WHERE accountid='%u' AND friendid='%u'", clientGcn, friendGcn );

				//printf ("MySQL query %s\n", myQuery );

				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					gc_exists = (int) mysql_num_rows ( myResult );
					mysql_free_result ( myResult );
				}
				else
				{
					debug ("Could not select existing guild card information for user %u", clientGcn);
					return;
				}
#endif


#ifdef NO_SQL
				if ( ( gc_exists == 0 ) && ( teamid != 0 ) )
				{
					for (ds=0;ds<num_accounts;ds++)
					{
						if ((account_data[ds]->guildcard == friendGcn) &&
							(account_data[ds]->teamid == teamid))
						{
							gc_exists = 1;
							break;
						}
					}
				}
#else
				if ( ( gc_exists == 0 ) && ( teamid != 0 ) )
				{
					// Well, they don't appear to have this person's guild card... so let's check the team list...
					sprintf (&myQuery[0], "SELECT * from account_data WHERE guildcard='%u' AND teamid='%u'", friendGcn, teamid );

					//printf ("MySQL query %s\n", myQuery );

					if ( ! mysql_query ( myData, &myQuery[0] ) )
					{
						myResult = mysql_store_result ( myData );
						gc_exists = (int) mysql_num_rows ( myResult );
						mysql_free_result ( myResult );
					}
					else
					{
						debug ("Could not select existing account information for user %u", friendGcn);
						return;
					}
				}
#endif

				if ( gc_exists )
				{
					// OK!  Let's tell the ships to do the search...
					for (ch=0;ch<serverNumShips;ch++)
					{
						shipNum = serverShipList[ch];
						tship = ships[shipNum];
						if ((tship->shipSockfd >= 0) && (tship->authed == 1))
							compressShipPacket ( tship, &ship->decryptbuf[0x04], 0x45E );
					}
				}				
			}
			break;
		case 0x04:
			// Flag account
			break;
		case 0x05:
			// Ban guild card.
			break;
		case 0x06:
			// Ban IP address.
			break;
		case 0x07:
			// Ban HW info.
			break;
		}
		break;
	case 0x09:
		// Reserved for team functions.
		switch (ship->decryptbuf[0x05])
		{
		case 0x00:
			{
				// Create Team
				//
				// 0x06 = Team name (pre-stripped)
				// 0x1E = Guild card of creator
				//
				//
				unsigned char CreateResult;
				int team_exists, teamid;
				unsigned highid;
				unsigned gcn;
#ifndef NO_SQL
				unsigned char TeamNameCheck[50];
#else
				unsigned short char_check;
				int match;
#endif

				gcn = *(unsigned*) &ship->decryptbuf[0x1E];
				// First check to see if the team exists...

				team_exists = 0;
				highid = 0;

#ifdef NO_SQL
				free_record = -1;
				for (ds=0;ds<num_teams;ds++)
				{
					if (team_data[ds]->owner == 0)
						free_record = ds;
					if (team_data[ds]->teamid >= highid)
						highid = team_data[ds]->teamid;
					match = 1;
					for (ds2=0;ds2<12;ds2++)
					{
						char_check = *(unsigned short*) &ship->decryptbuf[0x06+(ds2*2)];
						if (team_data[ds]->name[ds2] != char_check)
						{
							match = 0;
							break;
						}
						if (char_check == 0x0000)
							break;
					}
					if (match)
					{
						team_exists = 1;
						break;
					}
				}
#else
				mysql_real_escape_string (myData, &TeamNameCheck[0], &ship->decryptbuf[0x06], 24);
				sprintf (&myQuery[0], "SELECT * from team_data WHERE name='%s'", (char*) &TeamNameCheck[0] );
				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					team_exists = (int) mysql_num_rows ( myResult );
					mysql_free_result ( myResult );
				}
				else
					CreateResult = 1;
#endif

					if ( !team_exists )
					{
						// It doesn't... but it will now. :)
#ifdef NO_SQL
						if (free_record != -1)
						{
							new_record = 0;
							ds_found = free_record;
						}
						else
						{
							new_record = 1;
							ds_found = num_teams;
							team_data[num_teams++] = malloc ( sizeof (L_TEAM_DATA) );
						}
						memcpy (&team_data[ds_found]->name[0], &ship->decryptbuf[0x06], 24);
						memcpy (&team_data[ds_found]->flag, &DefaultTeamFlag, 2048);
						team_data[ds_found]->owner = gcn;
						highid++;
						team_data[ds_found]->teamid = teamid = highid;
						UpdateDataFile ("team.dat", ds_found, team_data[ds_found], sizeof (L_TEAM_DATA), new_record);
						CreateResult = 0;
						for (ds=0;ds<num_accounts;ds++)
						{
							if (account_data[ds]->guildcard == gcn)
							{
								account_data[ds]->teamid = highid;
								account_data[ds]->privlevel = 0x40;
								UpdateDataFile ("account.dat", ds, account_data[ds], sizeof (L_ACCOUNT_DATA), 0);
								break;
							}
						}
#else
						sprintf (&myQuery[0], "INSERT into team_data (name,owner,flag) VALUES ('%s','%u','%s')", (char*) &TeamNameCheck[0], gcn, (char*) &DefaultTeamFlagSlashes[0]);
						if ( ! mysql_query ( myData, &myQuery[0] ) )
						{
							teamid = (unsigned) mysql_insert_id( myData );
							sprintf (&myQuery[0], "UPDATE account_data SET teamid='%u', privlevel='%u' WHERE guildcard='%u'", teamid, 0x40, gcn );
							if ( mysql_query ( myData, &myQuery[0] ) )
								CreateResult = 1;
							else
								CreateResult = 0;
						}
						else
							CreateResult = 1;
#endif
					}
					else
						CreateResult = 2;
				// 1 = MySQL error
				// 2 = Team Exists
				ship->encryptbuf[0x00] = 0x09;
				ship->encryptbuf[0x01] = 0x00;
				ship->encryptbuf[0x02] = CreateResult;
				*(unsigned*) &ship->encryptbuf[0x03] = gcn;
				memcpy (&ship->encryptbuf[0x07], &DefaultTeamFlag[0], 0x800);
				memcpy (&ship->encryptbuf[0x807], &ship->decryptbuf[0x06], 24);
				*(unsigned*) &ship->encryptbuf[0x81F] = teamid;
				compressShipPacket (ship, &ship->encryptbuf[0x00], 0x823);
			}
			break;
		case 0x01:
			// Update Team Flag
			//
			// 0x06 = Flag (2048/0x800) bytes
			// 0x806 = Guild card of invoking person
			//
			{
				unsigned teamid;

				teamid = *(unsigned*) &ship->decryptbuf[0x806];
#ifdef NO_SQL
				for (ds=0;ds<num_teams;ds++)
				{
					if (team_data[ds]->teamid == teamid)
					{
						memcpy (&team_data[ds]->flag, &ship->decryptbuf[0x06], 0x800);
						ship->encryptbuf[0x00] = 0x09;
						ship->encryptbuf[0x01] = 0x01;
						ship->encryptbuf[0x02] = 0x01;
						*(unsigned*) &ship->encryptbuf[0x03] = teamid;
						memcpy ( &ship->encryptbuf[0x07], &ship->decryptbuf[0x06], 0x800 );
						compressShipPacket (ship, &ship->encryptbuf[0x00], 0x807);
						UpdateDataFile ("team.dat", ds, team_data[ds], sizeof (L_TEAM_DATA), 0);
						break;
					}
				}
#else
				mysql_real_escape_string ( myData, &FlagSlashes[0], &ship->decryptbuf[0x06], 0x800 );
				sprintf ( &myQuery[0], "UPDATE team_data SET flag='%s' WHERE teamid='%u'", (char*) &FlagSlashes[0], teamid );
				if (! mysql_query ( myData, &myQuery[0] ) )
				{
					ship->encryptbuf[0x00] = 0x09;
					ship->encryptbuf[0x01] = 0x01;
					ship->encryptbuf[0x02] = 0x01;
					*(unsigned*) &ship->encryptbuf[0x03] = teamid;
					memcpy ( &ship->encryptbuf[0x07], &ship->decryptbuf[0x06], 0x800 );
					compressShipPacket (ship, &ship->encryptbuf[0x00], 0x807);
				}
				else
				{
					debug ("Could not update team flag for team %u", teamid);
					return;
				}
#endif

			}
			break;
		case 0x02:
			// Dissolve Team
			//
			// 0x06 = Guild card of invoking person
			//
			{
				unsigned teamid;

				teamid = *(unsigned*) &ship->decryptbuf[0x06];
#ifdef NO_SQL
				for (ds=0;ds<num_teams;ds++)
				{
					if (team_data[ds]->teamid == teamid)
					{
						team_data[ds]->teamid = 0;
						team_data[ds]->owner = 0;
						UpdateDataFile ("team.dat", ds, team_data[ds], sizeof (L_TEAM_DATA), 0);
						break;
					}
				}

				for (ds=0;ds<num_accounts;ds++)
				{
					if (account_data[ds]->teamid == teamid)
						account_data[ds]->teamid = -1;
					UpdateDataFile ("account.dat", ds, account_data[ds], sizeof (L_ACCOUNT_DATA), 0);
				}
#else
				sprintf (&myQuery[0], "DELETE from team_data WHERE teamid='%u'", teamid );
				mysql_query (myData, &myQuery[0]);
				sprintf (&myQuery[0], "UPDATE account_data SET teamid='-1' WHERE teamid='%u'", teamid);
				mysql_query (myData, &myQuery[0]);
#endif
				ship->encryptbuf[0x00] = 0x09;
				ship->encryptbuf[0x01] = 0x02;
				ship->encryptbuf[0x02] = 0x01;
				*(unsigned*) &ship->encryptbuf[0x03] = teamid;
				compressShipPacket (ship, &ship->encryptbuf[0x00], 7);
			}
			break;
		case 0x03:
			// Remove member
			//
			// 0x06 = Team ID
			// 0x0A = Guild card
			//
			{
				unsigned gcn, teamid;

				teamid = *(unsigned*) &ship->decryptbuf[0x06];
				gcn = *(unsigned*) &ship->decryptbuf[0x0A];
#ifdef NO_SQL
				for (ds=0;ds<num_accounts;ds++)
				{
					if ((account_data[ds]->guildcard == gcn) &&
						(account_data[ds]->teamid == teamid))
					{
						account_data[ds]->teamid = -1;
						UpdateDataFile ("account.dat", ds, account_data[ds], sizeof (L_ACCOUNT_DATA), 0);
						break;
					}
				}
#else
				sprintf (&myQuery[0], "UPDATE account_data SET teamid='-1' WHERE guildcard='%u' AND teamid = '%u'", gcn, teamid);
				mysql_query (myData, &myQuery[0]);
#endif
				ship->encryptbuf[0x00] = 0x09;
				ship->encryptbuf[0x01] = 0x03;
				ship->encryptbuf[0x02] = 0x01;
				compressShipPacket (ship, &ship->encryptbuf[0x00], 3);
			}
			break;
		case 0x04:
			// Team Chat
			{
				ORANGE* tship;
				unsigned size,ch;

				size = *(unsigned*) &ship->decryptbuf[0x00];
				size -= 4;

				// Just pass the packet along...
				for (ch=0;ch<serverNumShips;ch++)
				{
					shipNum = serverShipList[ch];
					tship = ships[shipNum];
					if ((tship->shipSockfd >= 0) && (tship->authed == 1))
						compressShipPacket ( tship, &ship->decryptbuf[0x04], size );
				}
			}
			break;
		case 0x05:
			// Request team list
			{
				unsigned teamid, packet_offset;
				int num_mates;
#ifndef NO_SQL
				int ch;
				unsigned guildcard, privlevel;
#else
				unsigned save_offset;
#endif


				teamid = *(unsigned*) &ship->decryptbuf[0x06];
				ship->encryptbuf[0x00] = 0x09;
				ship->encryptbuf[0x01] = 0x05;
				*(unsigned*) &ship->encryptbuf[0x02] = teamid;
				*(unsigned*) &ship->encryptbuf[0x06] = *(unsigned*) &ship->decryptbuf[0x0A];
				// @ 0x0A store the size
				ship->encryptbuf[0x0C] = 0xEA;
				ship->encryptbuf[0x0D] = 0x09;
				memset (&ship->encryptbuf[0x0E], 0, 4);
				packet_offset = 0x12;
#ifdef NO_SQL
				save_offset = packet_offset;
				packet_offset += 4;
				num_mates = 0;
				for (ds=0;ds<num_accounts;ds++)
				{
					if (account_data[ds]->teamid == teamid)
					{
						num_mates++;
						*(unsigned*) &ship->encryptbuf[packet_offset] = num_mates;
						packet_offset += 4;
						*(unsigned*) &ship->encryptbuf[packet_offset] = account_data[ds]->privlevel;
						packet_offset += 4;
						*(unsigned*) &ship->encryptbuf[packet_offset] = account_data[ds]->guildcard;
						packet_offset += 4;
						memcpy (&ship->encryptbuf[packet_offset], &account_data[ds]->lastchar, 24);
						packet_offset += 24;
						memset (&ship->encryptbuf[packet_offset], 0, 8);
						packet_offset += 8;
					}
				}
				*(unsigned*) &ship->encryptbuf[save_offset] = num_mates;
				packet_offset -= 0x0A;
				*(unsigned short*) &ship->encryptbuf[0x0A] = (unsigned short) packet_offset;
				packet_offset += 0x0A;
				compressShipPacket (ship, &ship->encryptbuf[0x00], packet_offset);
#else
				sprintf (&myQuery[0], "SELECT guildcard,privlevel,lastchar from account_data WHERE teamid='%u'", teamid );
				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					myResult = mysql_store_result ( myData );
					num_mates = (int) mysql_num_rows ( myResult );
					*(unsigned*) &ship->encryptbuf[packet_offset] = num_mates;
					packet_offset += 4;
					for (ch=1;ch<=num_mates;ch++)
					{
						myRow = mysql_fetch_row ( myResult );
						guildcard = atoi (myRow[0]);
						privlevel = atoi (myRow[1]);
						*(unsigned*) &ship->encryptbuf[packet_offset] = ch;
						packet_offset += 4;
						*(unsigned*) &ship->encryptbuf[packet_offset] = privlevel;
						packet_offset += 4;
						*(unsigned*) &ship->encryptbuf[packet_offset] = guildcard;
						packet_offset += 4;
						memcpy (&ship->encryptbuf[packet_offset], myRow[2], 24);
						packet_offset += 24;
						memset (&ship->encryptbuf[packet_offset], 0, 8);
						packet_offset += 8;
					}
					mysql_free_result ( myResult );
					packet_offset -= 0x0A;
					*(unsigned short*) &ship->encryptbuf[0x0A] = (unsigned short) packet_offset;
					packet_offset += 0x0A;
					compressShipPacket (ship, &ship->encryptbuf[0x00], packet_offset);
				}
				else
				{
					debug ("Could not get team list for team %u", teamid);
					return;
				}
#endif
			}
			break;
		case 0x06:
			// Promote member
			//
			// 0x06 = Team ID
			// 0x0A = Guild card
			// 0x0B = New level
			//
			{
				unsigned gcn, teamid;
				unsigned char privlevel;

				teamid = *(unsigned*) &ship->decryptbuf[0x06];
				gcn = *(unsigned*) &ship->decryptbuf[0x0A];
				privlevel = (unsigned char) ship->decryptbuf[0x0E];
#ifdef NO_SQL
				for (ds=0;ds<num_accounts;ds++)
				{
					if ((account_data[ds]->guildcard == gcn) &&
						(account_data[ds]->teamid == teamid))
					{
						account_data[ds]->privlevel = privlevel;
						UpdateDataFile ("account.dat", ds, account_data[ds], sizeof (L_ACCOUNT_DATA), 0);
						break;
					}
				}

				if (privlevel == 0x40)
				{
					for (ds=0;ds<num_accounts;ds++)
					{
						if (team_data[ds]->teamid == teamid)
						{
							team_data[ds]->owner = gcn;
							UpdateDataFile ("team.dat", ds, team_data[ds], sizeof (L_TEAM_DATA), 0);
							break;
						}
					}
				}
#else
				sprintf (&myQuery[0], "UPDATE account_data SET privlevel='%u' WHERE guildcard='%u' AND teamid='%u'", privlevel, gcn, teamid);
				mysql_query (myData, &myQuery[0]);
				if (privlevel == 0x40)  // Master Transfer
					sprintf (&myQuery[0], "UPDATE team_data SET owner='%u' WHERE teamid='%u'", gcn, teamid);
				mysql_query (myData, &myQuery[0]);
#endif
				ship->encryptbuf[0x00] = 0x09;
				ship->encryptbuf[0x01] = 0x06;
				ship->encryptbuf[0x02] = 0x01;
				compressShipPacket (ship, &ship->encryptbuf[0x00], 3);
			}
			break;
		case 0x07:
			// Add member
			//
			// 0x06 = Team ID
			// 0x0A = Guild card
			//
			{
				unsigned gcn, teamid;

				teamid = *(unsigned*) &ship->decryptbuf[0x06];
				gcn = *(unsigned*) &ship->decryptbuf[0x0A];
#ifdef NO_SQL
				for (ds=0;ds<num_accounts;ds++)
				{
					if (account_data[ds]->guildcard == gcn)
					{
						account_data[ds]->teamid = teamid;
						account_data[ds]->privlevel = 0;
						UpdateDataFile ("account.dat", ds, account_data[ds], sizeof (L_ACCOUNT_DATA), 0);
						break;
					}
				}
#else
				sprintf (&myQuery[0], "UPDATE account_data SET teamid='%u', privlevel='0' WHERE guildcard='%u'", teamid, gcn);
				mysql_query (myData, &myQuery[0]);
#endif
				ship->encryptbuf[0x00] = 0x09;
				ship->encryptbuf[0x01] = 0x07;
				ship->encryptbuf[0x02] = 0x01;
				compressShipPacket (ship, &ship->encryptbuf[0x00], 3);
			}
			break;
		}
		break;
	case 0x0A:
		break;
	case 0x0B:
		// Checking authentication...
		switch (ship->decryptbuf[0x05])
		{
		case 0x00:
			{
				int security_client_thirtytwo, security_thirtytwo_check;
				long long security_client_sixtyfour, security_sixtyfour_check;
				unsigned char fail_to_auth = 0;
				unsigned gcn;
				unsigned char slotnum;
				unsigned char isgm;

				gcn = *(unsigned*) &ship->decryptbuf[0x06];
#ifdef NO_SQL
				for (ds=0;ds<num_security;ds++)
				{
					if (security_data[ds]->guildcard == gcn)
					{
						int found_match;

						security_thirtytwo_check = security_data[ds]->thirtytwo;
						security_sixtyfour_check = security_data[ds]->sixtyfour;
						slotnum = security_data[ds]->slotnum;
						isgm = security_data[ds]->isgm;

						found_match = 0;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x0E];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x16];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x1E];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x26];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x2E];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						if (found_match == 0)
							fail_to_auth = 1;

						security_client_thirtytwo = *(unsigned *) &ship->decryptbuf[0x0A];

						if (security_client_thirtytwo != security_thirtytwo_check)
							fail_to_auth = 1;

						break;
					}
				}
#else
				sprintf (&myQuery[0], "SELECT * from security_data WHERE guildcard='%u'", gcn );
				// Nom nom nom
				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					int num_rows, found_match;

					found_match = 0;

					myResult = mysql_store_result ( myData );
					num_rows = (int) mysql_num_rows ( myResult );

					if (num_rows)
					{
						myRow = mysql_fetch_row ( myResult );

						security_thirtytwo_check = atoi ( myRow[1] );
						memcpy (&security_sixtyfour_check, myRow[2], 8 );
						slotnum = atoi (myRow[3]);
						isgm = atoi (myRow[4]);

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x0E];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x16];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x1E];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x26];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &ship->decryptbuf[0x2E];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						if (found_match == 0)
							fail_to_auth = 1;
					}
					else
						fail_to_auth = 1;

					security_client_thirtytwo = *(unsigned *) &ship->decryptbuf[0x0A];
					
					if (security_client_thirtytwo != security_thirtytwo_check)
						fail_to_auth = 1;

					mysql_free_result ( myResult );
				}
				else
					fail_to_auth = 1;
#endif
				ship->encryptbuf[0x00] = 0x0B;
				ship->encryptbuf[0x01] = fail_to_auth;
				*(unsigned *) &ship->encryptbuf[0x02] = gcn;
				ship->encryptbuf[0x06] = slotnum;
				ship->encryptbuf[0x07] = isgm;
				*(unsigned *) &ship->encryptbuf[0x08] = security_thirtytwo_check;
				*(long long*) &ship->encryptbuf[0x0C] = security_sixtyfour_check;
				compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x14 );
			}
			break;
		}
		break;
	case 0x0C:
		break;
	case 0x0D:
		// Various commands related to ship transfers...
		ShipSend0D (ship->decryptbuf[0x05], ship);
		break;
	case 0x0E:
		{
			// Update player count.
			unsigned updateID;

			updateID = *(unsigned *) &ship->decryptbuf[0x06];
			updateID--;

			if ( updateID < serverMaxShips )
				ships[updateID]->playerCount = *(unsigned *) &ship->decryptbuf[0x0A];

			construct0xA0();
		}
		break;
	case 0x0F:
		switch ( ship->decryptbuf[0x05] )
		{
		case 0x00:
				keys_in_use[ ship->key_index ] = 1;
				ShipSend0F (0x01, 0x00, ship);
				break;
		case 0x01:
			if ( ship->decryptbuf[0x06] == 0 )
				ShipSend0F (0x01, 0x01, ship);
			else
				ShipSend0F (0x02, 0x00, ship);
			break;
		case 0x02:
			if ( ship->decryptbuf[0x06] == 0 )
				ShipSend0F (0x02, 0x01, ship);
			else
				ShipSend0F (0x03, 0x00, ship);
			break;
		case 0x03:
			if ( ship->decryptbuf[0x06] == 0 )
				ShipSend0F (0x03, 0x01, ship);
			else
				ShipSend10 (ship);
			break;
		default:
			break;
		}
		break;
	case 0x11:
		ship->last_ping = (unsigned) servertime;
		ship->sent_ping = 0;
		break;
	case 0x12:
		// Global announcement
		{
			ORANGE* tship;
			unsigned size,ch;

			size = *(unsigned *) &ship->decryptbuf[0x00];
			size -= 4;

			// Just pass the packet along...
			for (ch=0;ch<serverNumShips;ch++)
			{
				shipNum = serverShipList[ch];
				tship = ships[shipNum];
				if ((tship->shipSockfd >= 0) && (tship->authed == 1))
					compressShipPacket ( tship, &ship->decryptbuf[0x04], size );
			}
		}
		break;
	default:
		// Unknown packet received from ship?
		ship->todc = 1;
		break;
	}
}


void CharacterProcessPacket (BANANA* client)
{
	char username[17];
	char password[34];
	char hwinfo[18];
	unsigned short clientver;
	char md5password[34] = {0};
	unsigned char MDBuffer[17] = {0};
	unsigned gcn;
	unsigned ch;
	unsigned selected;
	unsigned shipNum;
	int security_client_thirtytwo, security_thirtytwo_check;
	long long security_client_sixtyfour, security_sixtyfour_check;
#ifdef NO_SQL
	long long truehwinfo;
#endif

	switch (client->decryptbuf[0x02])
	{
	case 0x05:
		printf ("Client has closed the connection.\n");
		client->todc = 1;
		break;
	case 0x10:
		if ((client->guildcard) && (client->slotnum != -1))
		{
			ORANGE* tship;

			selected = *(unsigned *) &client->decryptbuf[0x0C];
			for (ch=0;ch<serverNumShips;ch++)
			{
				shipNum = serverShipList[ch];
				tship = ships[shipNum];

				if ((tship->shipSockfd >= 0) && (tship->authed == 1)  &&
					(tship->shipID == selected))
				{
						Send19 (tship->shipAddr[0], tship->shipAddr[1], 
								tship->shipAddr[2], tship->shipAddr[3], 
								tship->shipPort, client);
					break;
				}
			}
		}
		break;
	case 0x1D:
		// Do nothing.
		break;
	case 0x93:
		if (!client->sendCheck[RECEIVE_PACKET_93])
		{
			int fail_to_auth = 0;

			clientver = *(unsigned short*) &client->decryptbuf[0x10];
			memcpy (&username[0], &client->decryptbuf[0x1C], 17 );
			memcpy (&password[0], &client->decryptbuf[0x4C], 17 );
			memset (&hwinfo[0], 0, 18);
#ifdef NO_SQL
			*(long long*) &client->hwinfo[0] = *(long long*) &client->decryptbuf[0x84];
			truehwinfo = *(long long*) &client->decryptbuf[0x84];
			fail_to_auth = 2; // default fail with wrong username
			for (ds=0;ds<num_accounts;ds++)
			{
				if (!strcmp(&account_data[ds]->username[0], &username[0]))
				{
					fail_to_auth = 0;
					sprintf (&password[strlen(password)], "_%u_salt", account_data[ds]->regtime );
					MDString (&password[0], &MDBuffer[0] );
					for (ch=0;ch<16;ch++)
						sprintf (&md5password[ch*2], "%02x", (unsigned char) MDBuffer[ch]);
					md5password[32] = 0;
					if (!strcmp(&md5password[0],&account_data[ds]->password[0]))
					{
						if (account_data[ds]->isbanned)
							fail_to_auth = 3;
						if (!account_data[ds]->isactive)
							fail_to_auth = 5;
						if (!fail_to_auth)
							gcn = account_data[ds]->guildcard;
						 if (client->decryptbuf[0x10] != PSO_CLIENT_VER)
							fail_to_auth = 7;
						client->isgm = account_data[ds]->isgm;
					}
					else
						fail_to_auth = 2;
					break;
				}
			}

			// DO HW BAN LATER

			if (!fail_to_auth)
			{
				for (ds=0;ds<num_security;ds++)
				{
					if (security_data[ds]->guildcard == gcn)
					{
						int found_match;

						client->dress_flag = 0;
						for (ch=0;ch<MAX_DRESS_FLAGS;ch++)
						{
							if (dress_flags[ch].guildcard == gcn)
								client->dress_flag = 1;
						}

						security_thirtytwo_check = security_data[ds]->thirtytwo;
						security_sixtyfour_check = security_data[ds]->sixtyfour;
						client->slotnum = security_data[ds]->slotnum;

						found_match = 0;

						security_client_sixtyfour = *(long long*) &client->decryptbuf[0x8C];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &client->decryptbuf[0x94];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;
					
						security_client_sixtyfour = *(long long*) &client->decryptbuf[0x9C];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &client->decryptbuf[0xA4];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;
					
						security_client_sixtyfour = *(long long*) &client->decryptbuf[0xAC];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						if (found_match == 0)
							fail_to_auth = 6;

						security_client_thirtytwo = *(unsigned *) &client->decryptbuf[0x18];

						if (security_client_thirtytwo == 0)
							client->sendingchars = 1;
						else
						{
							client->sendingchars = 0;
							if (security_client_thirtytwo != security_thirtytwo_check)
								fail_to_auth = 6;
						}
						break;
					}
				}
			}

#else
			mysql_real_escape_string ( myData, &hwinfo[0], &client->decryptbuf[0x84], 8);
			memcpy (&client->hwinfo[0], &hwinfo[0], 18);
			sprintf (&myQuery[0], "SELECT * from account_data WHERE username='%s'", username );

			// Check to see if that account already exists.
			if ( ! mysql_query ( myData, &myQuery[0] ) )
			{
				int num_rows, max_fields;

				myResult = mysql_store_result ( myData );
				num_rows = (int) mysql_num_rows ( myResult );

				if (num_rows)
				{
					myRow = mysql_fetch_row ( myResult );
					max_fields = mysql_num_fields ( myResult );
					sprintf (&password[strlen(password)], "_%s_salt", myRow[3] );
					MDString (&password[0], &MDBuffer[0] );
					for (ch=0;ch<16;ch++)
						sprintf (&md5password[ch*2], "%02x", (unsigned char) MDBuffer[ch]);
					md5password[32] = 0;
					if (!strcmp(&md5password[0],myRow[1]))
					{
						if (!strcmp("1", myRow[8]))
							fail_to_auth = 3;
						if (!strcmp("1", myRow[9]))
							fail_to_auth = 4;
						if (!strcmp("0", myRow[10]))
							fail_to_auth = 5;
						if (!fail_to_auth)
							gcn = atoi (myRow[6]);
						if (client->decryptbuf[0x10] != PSO_CLIENT_VER)
							fail_to_auth = 7;
						client->isgm = atoi (myRow[7]);
					}
					else
						fail_to_auth = 2;
				}
				else
					fail_to_auth = 2;

				mysql_free_result ( myResult );
			}
			else
				fail_to_auth = 1; // MySQL error.

			// Hardware info ban check...

			sprintf (&myQuery[0], "SELECT * from hw_bans WHERE hwinfo='%s'", hwinfo );
			if ( ! mysql_query ( myData, &myQuery[0] ) )
			{
				myResult = mysql_store_result ( myData );
				if ((int) mysql_num_rows ( myResult ))
					fail_to_auth = 3;
				mysql_free_result ( myResult );
			}
			else
				fail_to_auth = 1;

			if ( fail_to_auth == 0)
			{
				sprintf (&myQuery[0], "SELECT * from security_data WHERE guildcard='%u'", gcn );
				// Nom nom nom
				if ( ! mysql_query ( myData, &myQuery[0] ) )
				{
					int num_rows, found_match;

					found_match = 0;

					myResult = mysql_store_result ( myData );
					num_rows = (int) mysql_num_rows ( myResult );

					if (num_rows)
					{
						myRow = mysql_fetch_row ( myResult );

						client->dress_flag = 0;
						for (ch=0;ch<MAX_DRESS_FLAGS;ch++)
						{
							if (dress_flags[ch].guildcard == gcn)
								client->dress_flag = 1;
						}

						security_thirtytwo_check = atoi ( myRow[1] );
						memcpy (&security_sixtyfour_check, myRow[2], 8 );
						client->slotnum = atoi (myRow[3]);

						security_client_sixtyfour = *(long long*) &client->decryptbuf[0x8C];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &client->decryptbuf[0x94];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;
					
						security_client_sixtyfour = *(long long*) &client->decryptbuf[0x9C];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;

						security_client_sixtyfour = *(long long*) &client->decryptbuf[0xA4];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;
					
						security_client_sixtyfour = *(long long*) &client->decryptbuf[0xAC];
						if (security_client_sixtyfour == security_sixtyfour_check)
							found_match = 1;
					
						if (found_match == 0)
						{
							debug ("Couldn't find 64-bit information.");
							fail_to_auth = 6;
						}
					}
					else
						fail_to_auth = 6;

					security_client_thirtytwo = *(unsigned *) &client->decryptbuf[0x18];

					if (security_client_thirtytwo == 0)
						client->sendingchars = 1;
					else
					{
						client->sendingchars = 0;
						if (security_client_thirtytwo != security_thirtytwo_check)
						{
							fail_to_auth = 6;
							debug ("Couldn't find 32-bit information.");
							debug ("Looking for %i, have %i", security_thirtytwo_check, security_client_thirtytwo);
						}
					}
					mysql_free_result ( myResult );
				}
			}
#endif

			switch (fail_to_auth)
			{
			case 0x00:
				// OK
				memcpy (&client->encryptbuf[0], &PacketE6[0], sizeof (PacketE6));
				*(unsigned *) &client->encryptbuf[0x10] = gcn;
				client->guildcard = gcn;
				_itoa (gcn, &client->guildcard_string[0], 10); /* auth'd, bitch */
				// Store some security shit in the E6 packet.
				*(long long*) &client->encryptbuf[0x38] = security_sixtyfour_check;
				if (security_thirtytwo_check == 0)
				{
					for (ch=0;ch<4;ch++)
						MDBuffer[ch] = (unsigned char)( rand() % 256 );
					security_thirtytwo_check = *(unsigned *) &MDBuffer[0];
#ifdef NO_SQL
					for (ds=0;ds<num_security;ds++)
					{
						if (security_data[ds]->guildcard == gcn)
						{
							security_data[ds]->thirtytwo = security_thirtytwo_check;
							UpdateDataFile ("security.dat", ds, security_data[ds], sizeof (L_SECURITY_DATA), 0);
							break;
						}
					}
#else
					sprintf (&myQuery[0], "UPDATE security_data set thirtytwo = '%i' WHERE guildcard = '%u'", security_thirtytwo_check, gcn );
					// Nom, nom, nom.
					if ( mysql_query ( myData, &myQuery[0] ) )
					{
						Send1A ("Couldn't update security information in MySQL database.\nPlease contact the server administrator.", client);
						client->todc = 1;
						return;
					}
#endif
				}
				*(unsigned *) &client->encryptbuf[0x14] = security_thirtytwo_check;
				cipher_ptr = &client->server_cipher;
				encryptcopy (client, &client->encryptbuf[0], sizeof (PacketE6));
				if (client->slotnum != -1)
				{
					if (client->decryptbuf[0x16] != 0x04)
					{
						Send1A ("Client/Server synchronization error.", client);
						client->todc = 1;
					}
					else
					{
						// User has completed the login process, after updating the SQL info with their
						// access information, give 'em the ship select screen.
#ifdef NO_SQL
						for (ds=0;ds<num_accounts;ds++)
						{
							if (account_data[ds]->guildcard == gcn)
							{
								memcpy (&account_data[ds]->lastip[0], &client->IP_Address[0], 16);
								account_data[ds]->lasthwinfo = truehwinfo;
								UpdateDataFile ("account.dat", ds, account_data[ds], sizeof (L_ACCOUNT_DATA), 0);
								break;
							}
						}
#else
						sprintf (&myQuery[0], "UPDATE account_data set lastip = '%s', lasthwinfo = '%s' WHERE username = '%s'", client->IP_Address, hwinfo, username );
						mysql_query ( myData, &myQuery[0] );
#endif
						client->lastTick = (unsigned) servertime;
						SendB1 (client);
						SendA0 (client);
						SendEE (&Welcome_Message[0], client);
					}
				}
				break;
			case 0x01:
				// MySQL error.
				Send1A ("There is a problem with the MySQL database.\n\nPlease contact the server administrator.", client);
				break;
			case 0x02:
				// Username or password incorrect.
				Send1A ("Username or password is incorrect.", client);
				break;
			case 0x03:
				// Account is banned.
				Send1A ("You are banned from this server.", client);
				break;
			case 0x04:
				// Already logged on.
				Send1A ("This account is already logged on.\n\nPlease wait 120 seconds and try again.", client);
				break;
			case 0x05:
				// Account has not completed e-mail validation.
				Send1A ("Please complete the registration of this account through\ne-mail validation.\n\nThank you.", client);
				break;
			case 0x06:
				// Security violation.
				Send1A ("Security violation.", client);
				break;
			case 0x07:
				// Client version too old.
				Send1A ("Your client executable is too old.\nPlease update your client through the patch server.", client);
				break;
			default:
				Send1A ("Unknown error.", client);
				break;
			}
			client->sendCheck[RECEIVE_PACKET_93] = 0x01;
		}
		break;
	case 0xDC:
		switch (client->decryptbuf[0x03])
		{
		case 0x03:
			// Send another chunk of guild card data.
			if ((client->decryptbuf[0x08] == 0x01) && (client->decryptbuf[0x10] == 0x01))
				SendDC (0x00, client->decryptbuf[0x0C], client );
			break;
		default:
			break;
		}
		break;
	case 0xE0:
		// The gamepad, keyboard config, and other options....
		SendE2 (client);
		break;
	case 0xE3:
		// Client selecting or requesting character.
		SendE4_E5(client->decryptbuf[0x08], client->decryptbuf[0x0C], client);
		break;
	case 0xE5:
		// Create a character in slot.
		// Check invalid data and store character in MySQL store.
		AckCharacter_Creation ( client->decryptbuf[0x08], client );
		break;
	case 0xE8:
		switch (client->decryptbuf[0x03])
		{
		case 0x01:
			// Client just communicating the expected guild card checksum.  (Ignoring for now.)
			SendE8 (client);
			break;
		case 0x03:
			// Client requesting guild card checksum.
			 SendDC (0x01, 0, client);
			break;
		default:
			break;
		}
		break;
	case 0xEB:
		switch (client->decryptbuf[0x03])
		{
		case 0x03:
			// Send another chunk of the parameter files.
			SendEB (0x02, client->decryptbuf[0x04], client );
			break;
		case 0x04:
			// Send header for parameter files.
			SendEB (0x01, 0x00, client);
			break;
		}
		break;
	case 0xEC:
		if (client->decryptbuf[0x08] == 0x02)
		{
			// Set the dressing room flag (Don't overwrite the character...)
			for (ch=0;ch<MAX_DRESS_FLAGS;ch++)
				if (dress_flags[ch].guildcard == 0)
				{
					dress_flags[ch].guildcard = client->guildcard;
					dress_flags[ch].flagtime = (unsigned) servertime;
					break;
					if (ch == (MAX_DRESS_FLAGS - 1))
					{
						Send1A ("Unable to save dress flag.", client);
						client->todc = 1;
					}
				}
			client->dress_flag = 1;
		}
		break;
	default:
		break;
	}
}

void LoginProcessPacket (BANANA* client)
{
	char username[17];
	char password[34];
	long long security_sixtyfour_check;
	char hwinfo[18];
	unsigned short clientver;
	char md5password[34] = {0};
	unsigned char MDBuffer[17] = {0};
	unsigned gcn;
	unsigned ch,connectNum,shipNum;
#ifdef NO_SQL
	long long truehwinfo;
#endif
	ORANGE* tship;
#ifndef NO_SQL
	char security_sixtyfour_binary[18];
#endif


	/* Only packet we're expecting during the login is 0x93 and 0x05. */

	switch (client->decryptbuf[0x02])
	{
	case 0x05:
		printf ("Client has closed the connection.\n");
		client->todc = 1;
		break;
	case 0x93:
		if (!client->sendCheck[RECEIVE_PACKET_93])
		{
			int fail_to_auth = 0;
			clientver = *(unsigned short*) &client->decryptbuf[0x10];
			memcpy (&username[0], &client->decryptbuf[0x1C], 17 );
			memcpy (&password[0], &client->decryptbuf[0x4C], 17 );
			memset (&hwinfo[0], 0, 18);
#ifdef NO_SQL
			*(long long*) &client->hwinfo[0] = *(long long*) &client->decryptbuf[0x84];
			truehwinfo = *(long long*) &client->decryptbuf[0x84];
			fail_to_auth = 2; // default fail with wrong username
			for (ds=0;ds<num_accounts;ds++)
			{
				if (!strcmp(&account_data[ds]->username[0], &username[0]))
				{
					fail_to_auth = 0;
					sprintf (&password[strlen(password)], "_%u_salt", account_data[ds]->regtime );
					MDString (&password[0], &MDBuffer[0] );
					for (ch=0;ch<16;ch++)
						sprintf (&md5password[ch*2], "%02x", (unsigned char) MDBuffer[ch]);
					md5password[32] = 0;
					if (!strcmp(&md5password[0],&account_data[ds]->password[0]))
					{
						if (account_data[ds]->isbanned)
							fail_to_auth = 3;
						if (!account_data[ds]->isactive)
							fail_to_auth = 5;
						if (!fail_to_auth)
							gcn = account_data[ds]->guildcard;
						if ((strcmp(&client->decryptbuf[0x8C], PSO_CLIENT_VER_STRING) != 0) || (client->decryptbuf[0x10] != PSO_CLIENT_VER))
							fail_to_auth = 7;
						client->isgm = account_data[ds]->isgm;
					}
					else
						fail_to_auth = 2;
					break;
				}
			}

			// DO HW BAN LATER

#else
			mysql_real_escape_string ( myData, &hwinfo[0], &client->decryptbuf[0x84], 8);
			memcpy (&client->hwinfo[0], &hwinfo[0], 18);

			sprintf (&myQuery[0], "SELECT * from account_data WHERE username='%s'", username );

			// Check to see if that account already exists.
			if ( ! mysql_query ( myData, &myQuery[0] ) )
			{
				int num_rows, max_fields;

				myResult = mysql_store_result ( myData );
				num_rows = (int) mysql_num_rows ( myResult );

				if (num_rows)
				{
					myRow = mysql_fetch_row ( myResult );
					max_fields = mysql_num_fields ( myResult );
					sprintf (&password[strlen(password)], "_%s_salt", myRow[3] );
					MDString (&password[0], &MDBuffer[0] );
					for (ch=0;ch<16;ch++)
						sprintf (&md5password[ch*2], "%02x", (unsigned char) MDBuffer[ch]);
					md5password[32] = 0;
					if (!strcmp(&md5password[0],myRow[1]))
					{
						if (!strcmp("1", myRow[8]))
							fail_to_auth = 3;
						if (!strcmp("1", myRow[9]))
							fail_to_auth = 4;
						if (!strcmp("0", myRow[10]))
							fail_to_auth = 5;
						if (!fail_to_auth)
							gcn = atoi (myRow[6]);
						if ((strcmp(&client->decryptbuf[0x8C], PSO_CLIENT_VER_STRING) != 0) || (client->decryptbuf[0x10] != PSO_CLIENT_VER))
							fail_to_auth = 7;
						client->isgm = atoi (myRow[7]);
		}
					else
						fail_to_auth = 2;
				}
				else
					fail_to_auth = 2;
				mysql_free_result ( myResult );
			}
			else
				fail_to_auth = 1; // MySQL error.

			// Hardware info ban check...

			sprintf (&myQuery[0], "SELECT * from hw_bans WHERE hwinfo='%s'", hwinfo );
			if ( ! mysql_query ( myData, &myQuery[0] ) )
			{
				myResult = mysql_store_result ( myData );
				if ((int) mysql_num_rows ( myResult ))
					fail_to_auth = 3;
				mysql_free_result ( myResult );
			}
			else
				fail_to_auth = 1;
#endif

			switch (fail_to_auth)
			{
			case 0x00:
				// OK

				// If guild card is connected to the login server already, disconnect it.

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

				// If guild card is connected to ships, disconnect it.

				for (ch=0;ch<serverNumShips;ch++)
				{
					shipNum = serverShipList[ch];
					tship = ships[shipNum];
					if ((tship->shipSockfd >= 0) && (tship->authed == 1))
						ShipSend08 (gcn, tship);
				}


				memcpy (&client->encryptbuf[0], &PacketE6[0], sizeof (PacketE6));
				*(unsigned *) &client->encryptbuf[0x10] = gcn;
				
				// Store some security shit
				for (ch=0;ch<8;ch++)
					client->encryptbuf[0x38+ch] = (unsigned char) rand() % 255;
				
				security_sixtyfour_check = *(long long*) &client->encryptbuf[0x38];

				// Nom, nom, nom.

#ifdef NO_SQL
				free_record = -1;
				new_record = 1;
				for (ds=0;ds<num_security;ds++)
				{
					if (security_data[ds]->guildcard == gcn)
						security_data[ds]->guildcard = 0;
					UpdateDataFile ("security.dat", ds, security_data[ds], sizeof (L_SECURITY_DATA), 0);
					if (security_data[ds]->guildcard == 0)
					{
						free_record = ds;
						new_record = 0;
					}
				}
				if (new_record)
				{
					free_record = num_security++;
					security_data[free_record] = malloc ( sizeof (L_SECURITY_DATA) );
				}
				security_data[free_record]->guildcard = gcn;
				security_data[free_record]->thirtytwo = 0;
				security_data[free_record]->sixtyfour = security_sixtyfour_check;
				security_data[free_record]->isgm = client->isgm;
				security_data[free_record]->slotnum = -1;
				UpdateDataFile ("security.dat", free_record, security_data[free_record], sizeof (L_SECURITY_DATA), new_record);
#else
				sprintf (&myQuery[0], "DELETE from security_data WHERE guildcard = '%u'", gcn );
				mysql_query ( myData, &myQuery[0] );
				mysql_real_escape_string ( myData, &security_sixtyfour_binary[0], (char*) &security_sixtyfour_check, 8);
				sprintf (&myQuery[0], "INSERT INTO security_data (guildcard, thirtytwo, sixtyfour, isgm) VALUES ('%u','0','%s', '%u')", gcn, (char*) &security_sixtyfour_binary, client->isgm );
				if ( mysql_query ( myData, &myQuery[0] ) )
				{
					Send1A ("Couldn't update security information in MySQL database.\nPlease contact the server administrator.", client);
					client->todc = 1;
					return;
				}
#endif

				cipher_ptr = &client->server_cipher;
				encryptcopy (client, &client->encryptbuf[0], sizeof (PacketE6));

				Send19 (serverIP[0], serverIP[1], serverIP[2], serverIP[3], serverPort+1, client );
				for (ch=0;ch<MAX_DRESS_FLAGS;ch++)
				{
					if ((dress_flags[ch].guildcard == gcn) || ((unsigned) servertime - dress_flags[ch].flagtime > DRESS_FLAG_EXPIRY))
						 dress_flags[ch].guildcard = 0;
				}
				break;
			case 0x01:
				// MySQL error.
				Send1A ("There is a problem with the MySQL database.\n\nPlease contact the server administrator.", client);
				break;
			case 0x02:
				// Username or password incorrect.
				Send1A ("Username or password is incorrect.", client);
				break;
			case 0x03:
				// Account is banned.
				Send1A ("You are banned from this server.", client);
				break;
			case 0x04:
				// Already logged on.
				Send1A ("This account is already logged on.\n\nPlease wait 120 seconds and try again.", client);
				break;
			case 0x05:
				// Account has not completed e-mail validation.
				Send1A ("Please complete the registration of this account through\ne-mail validation.\n\nThank you.", client);
				break;
			case 0x07:
				// Client version too old.
				Send1A ("Your client executable is too old.\nPlease update your client through the patch server.", client);
				break;
			default:
				Send1A ("Unknown error.", client);
				break;
			}
			client->sendCheck[RECEIVE_PACKET_93] = 0x01;
		}
		break;
	default:
		client->todc = 1;
		break;
	}
}

void LoadQuestAllow ()
{
	unsigned ch;
	char allow_data[256];
	unsigned char* qa;
	FILE* fp;

	quest_numallows = 0;
	if ( ( fp = fopen ("questitem.txt", "r" ) ) == NULL )
	{
		printf ("questitem.txt is missing.\n");
		printf ("Press [ENTER] to quit...");
		gets(&dp[0]);
		exit (1);
	}
	else
	{
		while (fgets (&allow_data[0], 255, fp) != NULL)
		{
			if ((allow_data[0] != 35) && (strlen(&allow_data[0]) > 5))
				quest_numallows++;
		}
		quest_allow = malloc (quest_numallows * 4);
		ch = 0;
		fseek ( fp, 0, SEEK_SET );
		while (fgets (&allow_data[0], 255, fp) != NULL)
		{
			if ((allow_data[0] != 35) && (strlen(&allow_data[0]) > 5))
			{
				quest_allow[ch] = 0;
				qa = (unsigned char*) &quest_allow[ch++];
				qa[0] = hexToByte (&allow_data[0]);
				qa[1] = hexToByte (&allow_data[2]);
				qa[2] = hexToByte (&allow_data[4]);
			}
		}
		fclose ( fp );
	}
	printf ("Number of quest item allowances: %u\n", quest_numallows);
}


void LoadDropData()
{	
	unsigned ch,ch2,ch3,d;
	unsigned char* rt_table;
	char id_file[256];
	FILE* fp;
	char convert_ch[10];
	int look_rate;

	printf ("Loading drop data...\n");

	// Each episode
	for (ch=1;ch<5;ch++)
	{
		if (ch != 3)
		{
			switch (ch)
			{
			case 0x01:
				rt_table = (unsigned char*) &rt_tables_ep1[0];
				break;
			case 0x02:
				rt_table = (unsigned char*) &rt_tables_ep2[0];
				break;
			case 0x04:
				rt_table = (unsigned char*) &rt_tables_ep4[0];
				break;
			}
			// Each difficulty
			for (d=0;d<4;d++)
			{
				// Each section ID
				for (ch2=0;ch2<10;ch2++)
				{
					id_file[0] = 0;
					switch (ch)
					{
					case 0x01:
						strcat (&id_file[0], "drop\\ep1_mob_");
						break;
					case 0x02:
						strcat (&id_file[0], "drop\\ep2_mob_");
						break;
					case 0x04:
						strcat (&id_file[0], "drop\\ep4_mob_");
						break;
					}
					_itoa (d, &convert_ch[0], 10);
					strcat (&id_file[0], &convert_ch[0]);
					strcat (&id_file[0], "_");
					_itoa (ch2, &convert_ch[0], 10);
					strcat (&id_file[0], &convert_ch[0]);
					strcat (&id_file[0], ".txt");
					ch3 = 0;
					fp = fopen ( &id_file[0], "r" );
					if (!fp)
					{
						printf ("Drop table not found \"%s\"", id_file[0] );
						printf ("Hit [ENTER] to quit...");
						gets   (&dp[0]);
						exit   (1);
					}
					look_rate = 1;
					while ( fgets ( &dp[0],  255, fp ) != NULL )
					{
						if ( dp[0] != 35 ) // not a comment
						{
							if ( look_rate )
							{
								rt_table[ch3++] = (unsigned char) atoi (&dp[0]);
								look_rate = 0;
							}
							else
							{
								if ( strlen ( &dp[0] ) < 6 )
								{
									printf ("Corrupted drop table \"%s\"", id_file[0] );
									printf ("Hit [ENTER] to quit...");
									gets   (&dp[0]);
									exit   (1);
								}
								_strupr ( &dp[0] );
								rt_table[ch3++] = hexToByte ( &dp[0] );
								rt_table[ch3++] = hexToByte ( &dp[2] );
								rt_table[ch3++] = hexToByte ( &dp[4] );
								look_rate = 1;
							}
						}
					}
					fclose (fp);
					ch3 = 0x194;
					memset (&rt_table[ch3], 0xFF, 30);
					id_file[9]  = 98;
					id_file[10] = 111;
					id_file[11] = 120;
					fp = fopen ( &id_file[0], "r" );
					if (!fp)
					{
						printf ("Drop table not found \"%s\"", id_file[0] );
						printf ("Hit [ENTER] to quit...");
						gets   (&dp[0]);
						exit   (1);
					}
					look_rate = 0;
					while ( ( fgets ( &dp[0],  255, fp ) != NULL ) && ( ch3 < 0x1B2 ) )
					{
						if ( dp[0] != 35 ) // not a comment
						{
							switch ( look_rate )
							{
							case 0x00:
								rt_table[ch3] = (unsigned char) atoi (&dp[0]);
								look_rate = 1;
								break;
							case 0x01:
								rt_table[0x1B2 + ((ch3-0x194)*4)] = (unsigned char) atoi (&dp[0]);
								look_rate = 2;
								break;
							case 0x02:
								if ( strlen ( &dp[0] ) < 6 )
								{
									printf ("Corrupted drop table \"%s\"", id_file[0] );
									printf ("Hit [ENTER] to quit...");
									gets   (&dp[0]);
									exit   (1);
								}
								_strupr ( &dp[0] );
								rt_table[0x1B3 + ((ch3-0x194)*4)] = hexToByte ( &dp[0] );
								rt_table[0x1B4 + ((ch3-0x194)*4)] = hexToByte ( &dp[2] );
								rt_table[0x1B5 + ((ch3-0x194)*4)] = hexToByte ( &dp[4] );
								look_rate = 0;
								ch3++;
								break;
							}
						}
					}
					fclose (fp);
					rt_table += 0x800;
				}
			}
		}
	}
}

#ifdef NO_SQL

void UpdateDataFile ( const char* filename, unsigned count, void* data, unsigned record_size, int new_record )
{
	FILE* fp;
	unsigned fs;

	fp = fopen (filename, "r+b");
	if (fp)
	{
		fseek (fp, 0, SEEK_END);
		fs = ftell (fp);
		if ((count * record_size) <= fs)
		{
			fseek (fp, count * record_size, SEEK_SET);
			fwrite (data, 1, record_size, fp);
		}
		else
			debug ("Could not seek to record for updating in %s", filename);
		fclose (fp);
	}
	else
	{
		fp = fopen (filename, "wb");
		if (fp)
		{
			fwrite (data, 1, record_size, fp); // Has to be the first record...
			fclose (fp);
		}
		else
			debug ("Could not open %s for writing!\n", filename);
	}
}


void DumpDataFile ( const char* filename, unsigned* count, void** data, unsigned record_size )
{
	FILE* fp;
	unsigned ch;

	printf ("Dumping \"%s\" ... ", filename);
	fp = fopen (filename, "wb");
	if (fp)
	{
		for (ch=0;ch<*count;ch++)
			fwrite (data[ch], 1, record_size, fp);
		fclose (fp);
	}
	else
		debug ("Could not open %s for writing!\n", filename);
	printf ("done!\n");
}

void LoadDataFile ( const char* filename, unsigned* count, void** data, unsigned record_size )
{
	FILE* fp;
	unsigned ch;

	printf ("Loading \"%s\" ... ", filename);
	fp = fopen (filename, "rb");
	if (fp)
	{
		fseek (fp, 0, SEEK_END);
		*count = ftell (fp) / record_size;
		fseek (fp, 0, SEEK_SET);
		for (ch=0;ch<*count;ch++)
		{
			data[ch] = malloc (record_size);
			if (!data[ch])
			{
				printf ("Out of memory!\nHit [ENTER]");
				gets (&dp[0]);
				exit (1);
			}
			fread (data[ch], 1, record_size, fp);
		}
		fclose (fp);
	}
	printf ("done!\n");
}

#endif


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
					ShowWindow (consoleHwnd, SW_NORMAL);
					SetForegroundWindow (consoleHwnd);
					SetFocus(consoleHwnd);
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

int
main( int argc, char * argv[] )
{
	unsigned ch,ch2;
	struct in_addr login_in;
	struct in_addr character_in;
	struct in_addr ship_in;
	struct sockaddr_in listen_in;
	unsigned listen_length;
	int login_sockfd = -1, character_sockfd = -1, ship_sockfd = -1;
	int pkt_len, pkt_c, bytes_sent;
	unsigned short this_packet;
	unsigned ship_this_packet;

	FILE* fp;
	//int wserror;
	unsigned char MDBuffer[17] = {0};
	unsigned connectNum, shipNum;
	HINSTANCE hinst;
    NOTIFYICONDATA nid = {0};
	WNDCLASS wc = {0};
	HWND hwndWindow;
	MSG msg;
	WSADATA winsock_data;

	consoleHwnd = GetConsoleWindow();
	hinst = GetModuleHandle(NULL);

	dp[0] = 0;

	memset (&dp[0], 0, sizeof (dp));

	strcat (&dp[0], "Tethealla Login Server version ");
	strcat (&dp[0], SERVER_VERSION );
	strcat (&dp[0], " coded by Sodaboy");
	SetConsoleTitle (&dp[0]);

	printf ("\nTethealla Login Server version %s  Copyright (C) 2008  Terry Chatman Jr.\n", SERVER_VERSION);
	printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf ("This program comes with ABSOLUTELY NO WARRANTY; for details\n");
	printf ("see section 15 in gpl-3.0.txt\n");
    printf ("This is free software, and you are welcome to redistribute it\n");
    printf ("under certain conditions; see gpl-3.0.txt for details.\n");
/*
	for (ch=0;ch<5;ch++)
	{
		printf (".");
		Sleep (1000);
	}*/
	printf ("\n\n");

	WSAStartup(MAKEWORD(1,1), &winsock_data);

	srand ( (unsigned) time(NULL) );

	memset ( &dress_flags[0], 0, sizeof (DRESSFLAG) * MAX_DRESS_FLAGS);

	printf ("Loading configuration from tethealla.ini ...");
	load_config_file();
	printf ("  OK!\n");

	/* Set this up for later. */

	memset (&empty_bank, 0, sizeof (BANK));

	for (ch=0;ch<200;ch++)
		empty_bank.bankInventory[ch].itemid = 0xFFFFFFFF;

#ifdef NO_SQL
	LoadDataFile ("account.dat", &num_accounts, &account_data[0], sizeof(L_ACCOUNT_DATA));
	LoadDataFile ("bank.dat", &num_bankdata, &bank_data[0], sizeof(L_BANK_DATA));
	LoadDataFile ("character.dat", &num_characters, &character_data[0], sizeof(L_CHARACTER_DATA));
	LoadDataFile ("guild.dat", &num_guilds, &guild_data[0], sizeof(L_GUILD_DATA));
	//LoadDataFile ("hwbans.dat", &num_hwbans, &hw_bans[0], sizeof(L_HW_BANS));
	//LoadDataFile ("ipbans.dat", &num_ipbans, &ip_bans[0], sizeof(L_IP_BANS));
	LoadDataFile ("keydata.dat", &num_keydata, &key_data[0], sizeof(L_KEY_DATA));
	LoadDataFile ("security.dat", &num_security, &security_data[0], sizeof(L_SECURITY_DATA));
	LoadDataFile ("shipkey.dat", &num_shipkeys, &ship_data[0], sizeof(L_SHIP_DATA));
	LoadDataFile ("team.dat", &num_teams, &team_data[0], sizeof(L_TEAM_DATA));
#endif
	printf ("Loading PlyLevelTbl.bin ...");
	fp = fopen ( "PlyLevelTbl.bin", "rb" );
	if (!fp)
	{
		printf ("Can't proceed without plyleveltbl.bin!\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
		exit (1);
	}
	fread ( &startingStats[0], 1, 12*14, fp );
	fclose ( fp );
	printf (" OK!\n");
	printf ("Loading 0xE2 base packet ...");
	fp = fopen ( "e2base.bin", "rb" );
	if (!fp)
	{
		printf ("Can't proceed without e2base.bin!\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
		exit (1);
	}
	fread ( &E2_Base[0], 1, 2808, fp );
	fclose ( fp );
	printf (" OK!\n");
	printf ("Loading 0xE7 base packet ...");
	fp = fopen ( "e7base.bin", "rb" );
	if (!fp)
	{
		printf ("Can't proceed without e7base.bin!\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
		exit (1);
	}
	fread ( &E7_Base, 1, sizeof(CHARDATA), fp );
	fclose ( fp );
	printf (" OK!\n");
	printf ("Loading parameter files specified in e8send.txt ...");
	construct0xEB();
	memcpy (&Packet03[0x54], &Message03[0], sizeof (Message03));
	printf ("\nLoading quest item allowances...\n");
	LoadQuestAllow();
	LoadDropData();
	printf ("... done!\n");
#ifdef DEBUG_OUTPUT
#ifndef NO_SQL
	printf ("\nMySQL connection parameters\n");
	printf ("///////////////////////////\n");
	printf ("Host: %s\n", mySQL_Host );
	printf ("Port: %u\n", mySQL_Port );
	printf ("Username: %s\n", mySQL_Username );
	printf ("Password: %s\n", mySQL_Password );
	printf ("Database: %s\n", mySQL_Database );
#endif
#endif
	printf ("\nLogin server parameters\n");
	printf ("///////////////////////\n");
	if (override_on)
		printf ("NOTE: IP override feature is turned on.\nThe server will bind to %u.%u.%u.%u but will send out the IP listed below.\n", overrideIP[0], overrideIP[1], overrideIP[2], overrideIP[3] );
	printf ("IP: %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3] );
	printf ("Login Port: %u\n", serverPort );
	printf ("Character Port: %u\n", serverPort+1 );
	printf ("Maximum Connections: %u\n", serverMaxConnections );
	printf ("Maximum Ships: %u\n\n", serverMaxShips );
	printf ("Allocating %u bytes of memory for connections...", sizeof (BANANA) * serverMaxConnections );
	for (ch=0;ch<serverMaxConnections;ch++)
	{
		connections[ch] = malloc ( sizeof (BANANA) );
		if ( !connections[ch] )
		{
			printf ("Out of memory!\n");
			printf ("Hit [ENTER]");
			gets (&dp[0]);
			exit (1);
		}
		initialize_connection (connections[ch]);
	}
	printf (" OK!\n");
	printf ("Allocating %u bytes of memory for ships...", sizeof (ORANGE) * serverMaxShips );
	memset (&ships, 0, 4 * serverMaxShips);
	for (ch=0;ch<serverMaxShips;ch++)
	{
		ships[ch] = malloc ( sizeof (ORANGE) );
		if ( !ships[ch] )
		{
			printf ("Out of memory!\n");
			printf ("Hit [ENTER]");
			gets (&dp[0]);
			exit (1);
		}
		initialize_ship (ships[ch]);
	}
	printf (" OK!\n\n");
	printf ("Constructing default ship list packet ...");
	construct0xA0();
	printf ("  OK!\n\n");

#ifndef NO_SQL
	printf ("Connecting up to the MySQL database ...");

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

	printf ("  OK!\n\n");

	printf ("Setting session wait_timeout ...");

	/* Set MySQL to time out after 7 days of inactivity... lulz :D */

	sprintf (&myQuery[0], "SET SESSION wait_timeout = 604800");
	mysql_query (myData, &myQuery[0]);
	printf ("  OK!\n\n");

#endif

	printf ("Getting max ship key count... ");

#ifdef NO_SQL

	max_ship_keys = 0;
	for (ds=0;ds<num_shipkeys;ds++)
	{
		if (ship_data[ds]->idx >= max_ship_keys)
			max_ship_keys = ship_data[ds]->idx;					
	}

#else

	sprintf (&myQuery[0], "SELECT * from ship_data" );

	if ( ! mysql_query ( myData, &myQuery[0] ) )
	{
		unsigned key_rows;

		myResult = mysql_store_result ( myData );
		key_rows = (int) mysql_num_rows ( myResult );
		max_ship_keys = 0;
		while ( key_rows )
		{
			myRow = mysql_fetch_row ( myResult );
			if ( (unsigned) atoi ( myRow[1] ) > max_ship_keys )
				max_ship_keys = atoi ( myRow[1] );
			key_rows --;
		}
		printf ("(%u) ", max_ship_keys);
		mysql_free_result ( myResult );

	}
	else
	{
		printf ("Unable to query the key database.\n");
	}

#endif

	printf (" OK!\n");

	printf ("Loading default.flag ...");
	fp = fopen ( "default.flag", "rb" );
	if (!fp)
	{
		printf ("Can't proceed without default.flag!\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
		exit (1);
	}
	fread ( &DefaultTeamFlag[0], 1, 2048, fp );
	fclose ( fp );
#ifndef NO_SQL
	mysql_real_escape_string (myData, &DefaultTeamFlagSlashes[0], &DefaultTeamFlag[0], 2048);
#endif
	printf (" OK!\n");

	/* Open the PSO BB Login Server Port... */

	printf ("Opening server login port %u for connections.\n", serverPort);

#ifdef USEADDR_ANY
	login_in.s_addr = INADDR_ANY;
#else
	if (override_on)
		*(unsigned *) &login_in.s_addr = *(unsigned *) &overrideIP[0];
	else
		*(unsigned *) &login_in.s_addr = *(unsigned *) &serverIP[0];
#endif
	login_sockfd = tcp_sock_open( login_in, serverPort );

	tcp_listen (login_sockfd);

	/* Open the PSO BB Character Server Port... */

	printf ("Opening server character port %u for connections.\n", serverPort + 1);

#ifdef USEADDR_ANY
	character_in.s_addr = INADDR_ANY;
#else
	if (override_on)
		*(unsigned *) &character_in.s_addr = *(unsigned*) &overrideIP[0]; 
	else
		*(unsigned *) &character_in.s_addr = *(unsigned *) &serverIP[0];
#endif
	character_sockfd = tcp_sock_open( character_in, serverPort + 1 );

	tcp_listen (character_sockfd);

	/* Open the Ship Port... */

	printf ("Opening ship port 3455 for connections.\n" );

#ifdef USEADDR_ANY
	ship_in.s_addr = INADDR_ANY;
#else
	if (override_on)
		*(unsigned *) &ship_in.s_addr = *(unsigned *) &overrideIP[0];
	else
		*(unsigned *) &ship_in.s_addr = *(unsigned *) &serverIP[0];
#endif
	ship_sockfd = tcp_sock_open( ship_in, 3455 );

	tcp_listen (ship_sockfd);

	if ((login_sockfd<0) || (character_sockfd<0) || (ship_sockfd<0))
	{
		printf ("Failed to open ports for connections.\n");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
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

	backupHwnd = hwndWindow;

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
	strcat (&nid.szTip[0], "Tethealla Login ");
	strcat (&nid.szTip[0], SERVER_VERSION);
	strcat (&nid.szTip[0], " - Double click to show/hide");
    Shell_NotifyIcon(NIM_ADD, &nid);


#ifdef NO_SQL

	lastdump = (unsigned) time(NULL);

#endif

	for (;;)
	{
		int nfds = 0;

		/* Ping pong?! */

		servertime = time(NULL);

		/* Process the system tray icon */

		if ( backupHwnd != hwndWindow )
		{
			debug ("hwndWindow has been corrupted...");
			display_packet ( (unsigned char*) &hwndWindow, sizeof (HWND));
			hwndWindow = backupHwnd;
			WriteLog ("hwndWindow corrupted %s", (char*) &dp[0] );
		}

		if ( PeekMessage( &msg, hwndWindow, 0, 0, 1 ) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


#ifdef NO_SQL

		if ( (unsigned) servertime - lastdump > 600 )
		{
			printf ("Refreshing account and ship key databases...\n");
			for (ch=0;ch<num_accounts;ch++)
				free (account_data[ch]);
			num_accounts = 0;
			LoadDataFile ("account.dat", &num_accounts, &account_data[0], sizeof(L_ACCOUNT_DATA));
			for (ch=0;ch<num_shipkeys;ch++)
				free (ship_data[ch]);
			num_shipkeys = 0;
			LoadDataFile ("shipkey.dat", &num_shipkeys, &ship_data[0], sizeof(L_SHIP_DATA));
			lastdump = (unsigned) servertime;
		}

#endif

		/* Clear socket activity flags. */

		FD_ZERO (&ReadFDs);
		FD_ZERO (&WriteFDs);
		FD_ZERO (&ExceptFDs);

		for (ch=0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			workConnect = connections[connectNum];

			if (workConnect->plySockfd >= 0) 
			{
				if (workConnect->packetdata)
				{
					this_packet = *(unsigned short*) &workConnect->packet[workConnect->packetread];
					memcpy (&workConnect->decryptbuf[0], &workConnect->packet[workConnect->packetread], this_packet);

					switch (workConnect->login)
					{
					case 0x01:
						// Login server
						LoginProcessPacket (workConnect);
						break;
					case 0x02:
						// Character server
						CharacterProcessPacket (workConnect);
						break;
					}
					workConnect->packetread += this_packet;
					if (workConnect->packetread == workConnect->packetdata)
						workConnect->packetread = workConnect->packetdata = 0;
				}

				if (workConnect->lastTick != (unsigned) servertime)
				{
					if (workConnect->lastTick > (unsigned) servertime)
						ch2 = 1;
					else
						ch2 = 1 + ((unsigned) servertime - workConnect->lastTick);
						workConnect->lastTick = (unsigned) servertime;
						workConnect->packetsSec /= ch2;
						workConnect->toBytesSec /= ch2;
						workConnect->fromBytesSec /= ch2;
				}

				/*
					if (((unsigned) servertime - workConnect->connected >= 300) || 
						(workConnect->connected > (unsigned) servertime))
					{
						Send1A ("You have been idle for too long.  Disconnecting...", workConnect);
						workConnect->todc = 1;
					}
					*/

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

		for (ch=0;ch<serverNumShips;ch++)
		{
			shipNum = serverShipList[ch];
			workShip = ships[shipNum];

			if (workShip->shipSockfd >= 0) 
			{
				// Send a ping request to the ship when 30 seconds passes...
				if (((unsigned) servertime - workShip->last_ping >= 30) && (workShip->sent_ping == 0))
				{
					workShip->sent_ping = 1;
					ShipSend11 (workShip);
				}

				// If it's been over a minute since we've heard from a ship, terminate 
				// the connection with it.

				if ((unsigned) servertime - workShip->last_ping > 60)
				{
					printf ("%s ship ping timeout.\n", workShip->name );
					initialize_ship (workShip);
				}
				else
				{
					if (workShip->packetdata)
					{
						ship_this_packet = *(unsigned *)&workShip->packet[workShip->packetread];
						memcpy (&workShip->decryptbuf[0], &workShip->packet[workShip->packetread], ship_this_packet);

						ShipProcessPacket (workShip);

						workShip->packetread += ship_this_packet;

						if (workShip->packetread == workShip->packetdata)
							workShip->packetread = workShip->packetdata = 0;
					}


					/* Limit time of authorization to 60 seconds... */

					if ((workShip->authed == 0) && ((unsigned) servertime - workShip->connected >= 60))
						workShip->todc = 1;


					FD_SET (workShip->shipSockfd, &ReadFDs);
					nfds = max (nfds, workShip->shipSockfd);
					if (workShip->snddata - workShip->sndwritten)
					{
						FD_SET (workShip->shipSockfd, &WriteFDs);
						nfds = max (nfds, workShip->shipSockfd);
					}
				}
			}
		}

		FD_SET (login_sockfd, &ReadFDs);
		nfds = max (nfds, login_sockfd);
		FD_SET (character_sockfd, &ReadFDs);
		nfds = max (nfds, character_sockfd);
		FD_SET (ship_sockfd, &ReadFDs);
		nfds = max (nfds, ship_sockfd);

		/* Check sockets for activity. */

		if ( select ( nfds + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &select_timeout ) > 0 ) 
		{
			if (FD_ISSET (login_sockfd, &ReadFDs))
			{
				// Someone's attempting to connect to the login server.
				ch = free_connection();
				if (ch != 0xFFFF)
				{
					listen_length = sizeof (listen_in);
					workConnect = connections[ch];
					if ( ( workConnect->plySockfd = tcp_accept ( login_sockfd, (struct sockaddr*) &listen_in, &listen_length ) ) >= 0 )
					{
						workConnect->connection_index = ch;
						serverConnectionList[serverNumConnections++] = ch;
						memcpy ( &workConnect->IP_Address[0], inet_ntoa (listen_in.sin_addr), 16 );
						printf ("Accepted LOGIN connection from %s:%u\n", workConnect->IP_Address, listen_in.sin_port );
						start_encryption (workConnect);
						/* Doin' login process... */
						workConnect->login = 1;
					}
				}
			}

			if (FD_ISSET (character_sockfd, &ReadFDs))
			{
				// Someone's attempting to connect to the character server.
				ch = free_connection();
				if (ch != 0xFFFF)
				{
					listen_length = sizeof (listen_in);
					workConnect = connections[ch];
					if ( ( workConnect->plySockfd = tcp_accept ( character_sockfd, (struct sockaddr*) &listen_in, &listen_length ) ) >= 0 )
					{
						workConnect->connection_index = ch;
						serverConnectionList[serverNumConnections++] = ch;
						memcpy ( &workConnect->IP_Address[0], inet_ntoa (listen_in.sin_addr), 16 );
						printf ("Accepted CHARACTER connection from %s:%u\n", inet_ntoa (listen_in.sin_addr), listen_in.sin_port );
						start_encryption (workConnect);
						/* Doin' character process... */
						workConnect->login = 2;
					}
				}
			}

			if (FD_ISSET (ship_sockfd, &ReadFDs))
			{
				// A ship is attempting to connect to the ship transfer port.
				ch = free_shipconnection();
				if (ch != 0xFFFF)
				{
					listen_length = sizeof (listen_in);
					workShip = ships[ch];
					if ( ( workShip->shipSockfd = tcp_accept ( ship_sockfd, (struct sockaddr*) &listen_in, &listen_length ) ) >= 0 )
					{
						workShip->connection_index = ch;
						serverShipList[serverNumShips++] = ch;
						printf ("Accepted SHIP connection from %s:%u\n", inet_ntoa (listen_in.sin_addr), listen_in.sin_port );
						*(unsigned *) &workShip->listenedAddr[0] = *(unsigned*) &listen_in.sin_addr;
						workShip->connected = workShip->last_ping = (unsigned) servertime;
						ShipSend00 (workShip);
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
									/* Decrypt the packet header after receiving 8 bytes. */

									cipher_ptr = &workConnect->client_cipher;

									decryptcopy ( &workConnect->peekbuf[0], &workConnect->rcvbuf[0], 8 );

									/* Make sure we're expecting a multiple of 8 bytes. */

									workConnect->expect = *(unsigned short*) &workConnect->peekbuf[0];

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
									if ( workConnect->packetdata + workConnect->expect > TCP_BUFFER_SIZE )
									{
										initialize_connection ( workConnect );
										break;
									}
									else
									{
										/* Decrypt the rest of the data if needed. */

										cipher_ptr = &workConnect->client_cipher;

										*(long long*) &workConnect->packet[workConnect->packetdata] = *(long long*) &workConnect->peekbuf[0];

										if ( workConnect->rcvread > 8 )
											decryptcopy ( &workConnect->packet[workConnect->packetdata + 8], &workConnect->rcvbuf[8], workConnect->expect - 8 );

										this_packet = *(unsigned short*) &workConnect->peekbuf[0];
										workConnect->packetdata += this_packet;

										workConnect->packetsSec ++;

										if ((workConnect->packetsSec   > 40)    ||
											(workConnect->fromBytesSec > 15000) ||
											(workConnect->toBytesSec   > 500000))
										{
											printf ("%u disconnected for possible DDOS. (p/s: %u, tb/s: %u, fb/s: %u)\n", workConnect->guildcard, workConnect->packetsSec, workConnect->toBytesSec, workConnect->fromBytesSec);
											initialize_connection(workConnect);
											break;
										}

										workConnect->rcvread = 0;
									}
								}
							}
						}
					}

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
							workConnect->sndwritten += bytes_sent;
							workConnect->toBytesSec += (unsigned) bytes_sent;
						}

						if (workConnect->sndwritten == workConnect->snddata)
							workConnect->sndwritten = workConnect->snddata = 0;
					}

					if (workConnect->todc)
					{
						if ( workConnect->snddata - workConnect->sndwritten )
							send (workConnect->plySockfd, &workConnect->sndbuf[workConnect->sndwritten],
								workConnect->snddata - workConnect->sndwritten, 0);
						initialize_connection (workConnect);
					}

					if (FD_ISSET(workConnect->plySockfd, &ExceptFDs)) // Exception?
						initialize_connection (workConnect);
				}
			}

			// Process ship connections

			for (ch=0;ch<serverNumShips;ch++)
			{
				shipNum = serverShipList[ch];
				workShip = ships[shipNum];

				if (workShip->shipSockfd >= 0)
				{
					if (FD_ISSET(workShip->shipSockfd, &ReadFDs))
					{
						// Read shit.
						if ( ( pkt_len = recv (workShip->shipSockfd, &tmprcv[0], PACKET_BUFFER_SIZE - 1, 0) ) <= 0 )
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not read data from client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							printf ("Lost connection with the %s ship...\n", workShip->name );
							initialize_ship (workShip);
						}
						else
						{
							// Work with it.
							for (pkt_c=0;pkt_c<pkt_len;pkt_c++)
							{
								workShip->rcvbuf[workShip->rcvread++] = tmprcv[pkt_c];

								if (workShip->rcvread == 4)
								{
									/* Read out how much data we're expecting this packet. */
									workShip->expect = *(unsigned*) &workShip->rcvbuf[0];

									if ( workShip->expect > TCP_BUFFER_SIZE )
									{
										printf ("Lost connection with the %s ship...\n", workShip->name );
										initialize_ship ( workShip ); /* This shouldn't happen, lol. */
									}
								}

								if ( ( workShip->rcvread == workShip->expect ) && ( workShip->expect != 0 ) )
								{
									decompressShipPacket ( workShip, &workShip->decryptbuf[0], &workShip->rcvbuf[0] );

									workShip->expect = *(unsigned *) &workShip->decryptbuf[0];

									if ( workShip->packetdata + workShip->expect < PACKET_BUFFER_SIZE )
									{
										memcpy ( &workShip->packet[workShip->packetdata], &workShip->decryptbuf[0], workShip->expect );
										workShip->packetdata += workShip->expect;
									}
									else
									{
										initialize_ship ( workShip );
										break;
									}
									workShip->rcvread = 0;
								}
							}
						}
					}

					if (FD_ISSET(workShip->shipSockfd, &WriteFDs))
					{
						// Write shit.

						bytes_sent = send (workShip->shipSockfd, &workShip->sndbuf[workShip->sndwritten],
							workShip->snddata - workShip->sndwritten, 0);
						if (bytes_sent == SOCKET_ERROR)
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not send data to client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							printf ("Lost connection with the %s ship...\n", workShip->name );
							initialize_ship (workShip);							
						}
						else
							workShip->sndwritten += bytes_sent;

						if (workShip->sndwritten == workShip->snddata)
							workShip->sndwritten = workShip->snddata = 0;

					}

					if (workShip->todc)
						{
							if ( workShip->snddata - workShip->sndwritten )
								send (workShip->shipSockfd, &workShip->sndbuf[workShip->sndwritten],
									workShip->snddata - workShip->sndwritten, 0);
							printf ("Terminated connection with ship...\n" );
							initialize_ship (workShip);
						}

				}
			}
		}
	}
#ifndef NO_SQL
	mysql_close( myData ) ;
#endif
	return 0;
}


void send_to_server(int sock, char* packet)
{
 int pktlen;

 pktlen = strlen (packet);

	if (send(sock, packet, pktlen, 0) != pktlen)
	{
	  printf ("send_to_server(): failure");
	  printf ("Hit [ENTER]");
	  gets (&dp[0]);
	  exit(1);
	}

}

int receive_from_server(int sock, char* packet)
{
 int pktlen;

  if ((pktlen = recv(sock, packet, TCP_BUFFER_SIZE - 1, 0)) <= 0)
	{
	  printf ("receive_from_server(): failure");
	  printf ("Hit [ENTER]");
	  gets (&dp[0]);
	  exit(1);
	}
  packet[pktlen] = 0;
  return pktlen;
}

void tcp_listen (int sockfd)
{
	if (listen(sockfd, 10) < 0)
	{
		debug_perror ("Could not listen for connection");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
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
			debug_perror("Could not make TCP connection");
		else
			debug ("tcp_sock_connect %s:%u", inet_ntoa (sa.sin_addr), sa.sin_port );
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
		debug_perror("Could not create socket");
		printf ("Hit [ENTER]");
		gets (&dp[0]);
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
		printf ("Hit [ENTER]");
		gets (&dp[0]);
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
        state[counter] = counter;

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

	if (ship->shipSockfd >= 0)
	{
		if (PACKET_BUFFER_SIZE - ship->snddata < (int) ( src_size + 100 ) )
			initialize_ship(ship);
		else
		{
			if ( ship->crypt_on )
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
			else
			{
				memcpy ( &ship->sndbuf[ship->snddata+4], src, src_size );
				src_size += 4;
				*(unsigned *) &ship->sndbuf[ship->snddata] = src_size;
				ship->snddata += src_size;
			}
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