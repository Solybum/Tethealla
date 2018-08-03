// For remapping Episode IV monsters to Episode I counterparts...

unsigned ep2_rtremap[] =
{
	0x00, 0x00, // Pioneer 2
	0x01, 0x01, // VR Temple Alpha
	0x02, 0x02, // VR Temple Beta
	0x03, 0x03, // VR Spaceship Alpha
	0x04, 0x04, // VR Spaceship Beta
	0x05, 0x08, // CCA
	0x06, 0x05, // Jungle North
	0x07, 0x06, // Jungle East
	0x08, 0x07, // Mountain
	0x09, 0x08, // Seaside
	0x0A, 0x09, // Seabed Upper
	0x0B, 0x0A, // Seabed Lower
};

unsigned ep4_rtremap[] = 
{
	0x00, 0x00, // None to None
	0x01, 0x01, // Remap Astark to Hildebear
	0x02, 0x08, // Remap Yowie to Barbarous Wolf
	0x03, 0x07, // Remap Satellite Lizard to Savage Wolf
	0x04, 0x13, // Remap Merissa A to Poufilly Slime
	0x05, 0x14, // Remap Merissa AA to Pouilly Slime
	0x06, 0x38, // Remap Girtablulu to Mericarol
	0x07, 0x37, // Remap Zu to Gi Gue
	0x08, 0x02, // Remap Pazuzu to Hildeblue
	0x09, 0x09, // Remap Boota to Booma
	0x0A, 0x0A, // Remap Ze Boota to Gobooma
	0x0B, 0x0B, // Remap Ba Boota to Gigobooma
	0x0C, 0x48, // Remap Dorphon to Delbiter
	0x0D, 0x02, // Remap Dorphon Eclair to Hildeblue
	0x0E, 0x29, // Remap Goran to Dimenian
	0x0F, 0x2B, // Remap Goran Detonator to So Dimenian
	0x10, 0x2A, // Remap Pyro Goran to La Dimenian
	0x11, 0x05, // Remap Sand Rappy to Rappy
	0x12, 0x06, // Remap Del Rappy to Al Rappy
	0x13, 0x2F, // Remap Saint Million to Dark Falz
	0x14, 0x2F, // Remap Shambertin to Dark Falz
	0x15, 0x2F, // Remap Kondrieu to Dark Falz
};


// Units dropped per difficulty (24 definitions per level)

unsigned char elemental_table[] = 
{
	0x01, 0x05, 0x0F, 0x13, 0x17, 0x1B, 0x1F, 0x23, 0xFF, 0xFF, 0xFF, 0xFF, 
	0x02, 0x06, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x09, 0x0C, 0xFF, 0xFF, 
	0x03, 0x07, 0x11, 0x15, 0x19, 0x1D, 0x21, 0x25, 0x0A, 0x0C, 0x0D, 0x27,
	0x04, 0x08, 0x12, 0x16, 0x1A, 0x1E, 0x22, 0x26, 0x0B, 0x0C, 0x0E, 0x28
};

unsigned tool_remap [28] = 
{
	0x000003,
	0x010003,
	0x020003,
	0x000103,
	0x010103,
	0x020103,
	0x000603,
	0x010603,
	0x000303,
	0x000403,
	0x000503,
	0x000703,
	0x000803,
	0x000A03,
	0x010A03,
	0x020A03,
	0x000B03,
	0x010B03,
	0x020B03,
	0x030B03,
	0x040B03,
	0x050B03,
	0x060B03,
	0x000003,
	0x000903,
	0x000203,
	0x001003,
	0x000003
};

unsigned char unit_drop [7*10] = 
{ 
	0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x21, 0x24, 0x27, 
	0x01, 0x05, 0x09, 0x0D, 0x19, 0x22, 0x25, 0x28, 0x2A, 0x2D, 
	0x11, 0x15, 0x2B, 0x2E, 0x33, 0x36, 0x39, 0xFF, 0xFF, 0xFF,
	0x02, 0x06, 0x0A, 0x0E, 0x1A, 0x23, 0x26, 0x29, 0xFF, 0xFF,
	0x12, 0x16, 0x2C, 0x2F, 0x34, 0x37, 0x3A, 0x3C, 0x3F, 0xFF,
	0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x1C, 0x1E, 0x31, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


// Gallon's shop "Hopkins' Dad's" item list + Photon Drops required for each

unsigned gallons_shop_hopkins[] = {
	0x000003, 1,
	0x000103, 1,
	0x000F03, 1,
	0x4A0201, 1,
	0x4B0201, 1,
	0x050E03, 20,
	0x060E03, 20,
	0x000E03, 20,
	0x4C0201, 20,
	0x4C0201, 20,
	0x4E0201, 20,
	0x020D00, 20,
	0x5900, 20,
	0x5200, 20,
	0x030E03, 50, 
	0x070E03, 50,
	0x270E03, 50,
	0x011003, 99
};


// Gallon's shop "Hopkins' Dad's" amount of PDs required for S-Rank attribute

unsigned char gallon_srank_pds[] = 
{
	0x01, 60,
	0x02, 60,
	0x03, 20,
	0x04, 20,
	0x05, 30,
	0x06, 30,
	0x07, 30,
	0x08, 50,
	0x09, 40,
	0x0A, 50,
	0x0B, 40,
	0x0C, 40,
	0x0D, 50,
	0x0E, 40,
	0x0F, 40,
	0x10, 40
};


// Dangerous Deal with Black Paper reward lists

unsigned bp_rappy_normal[] = 
{ 
	0x8B0201, 0x280201, 0x340101, 0x030301, 
	0x0B0301, 0x071803, 0x5500, 0x290301,
	0x2F0301, 0x2C0301, 0x230301, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_rappy_hard[] = 
{ 
	0x8C0201, 0x150201, 0x8A0201, 0x400101,
	0x440301, 0x460301, 0x450301, 0x470301,
	0x071803, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_rappy_vhard[] = 
{
	0xCB00, 0x3A00, 0x028C00, 0x2B0201,
	0x5000, 0x060B00, 0x060A00, 0x040A00,
	0x5500, 0x2300, 0x3B00, 0x071803,
	0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_rappy_ultimate[] = 
{
	0x5100, 0x520301, 0x200301, 0x3E0301,
	0x290201, 0x071803, 0x040B00, 0x060A00,
	0x5600, 0x3B00, 0x2300, 0x050A00,
	0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_zu_normal[] = 
{
	0x320101, 0x012F00, 0xB300, 0x5E00,
	0x020E00, 0x2E00, 0x9500, 0x9A00,
	0x2F00, 0x1B0301, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_zu_hard[] = 
{
	0xC000, 0xD200, 0x8D00, 0x2E0101,
	0x8B00, 0x070900, 0x4E00, 0x6D00,
	0x1500, 0x028B00, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_zu_vhard[] =
{
	0xAA00, 0x410101, 0x510101, 0x230201,
	0x3F00, 0x4100, 0x070500, 0x060500,
	0x050500, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_zu_ultimate[] = 
{
	0xAF00, 0x4300, 0x510301, 0xCD00, 
	0x9900, 0x6C00, 0x4500, 0x6B00, 
	0x1200, 0x6500, 0x290201, 0x1300,
	0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_dorphon_normal[] = 
{
	0x9000, 0x019000, 0x029000, 0x039000, 
	0x049000, 0x059000, 0x069000, 0x079000, 
	0x089000, 0xB400, 0x4E0101, 0x070301,
	0x410301, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_dorphon_hard[] = 
{
	0xB900, 0x3400, 0x010900, 0x029000, 
	0x079000, 0x2C00, 0x2D00, 0x350201,
	0x060100, 0x050100, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_dorphon_vhard[] =
{
	0xB600, 0x018A00, 0x011000, 0x021000,
	0x031000, 0x041000, 0x051000, 0x061000,
	0x2700, 0x070100, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};

unsigned bp_dorphon_ultimate[] = 
{
	0xB700, 0x011000, 0x021000, 0x031000, 
	0x041000, 0x051000, 0x061000, 0x2900,
	0x8A00, 0x028A00, 0x04,
	0x04, 0x04, 0x04, 0x04, 
	0x04, 0x04, 0x04, 0x04, 
};


// Dangerous Deal with Black Paper 2 reward lists

unsigned bp2_normal[] = 
{ 
	0x00BA00, 0x030D00, 0x014300, 0x080700,
	0x014200, 0x00c900, 0x001003, 0x950201,
	0x8F0201, 0x910201
};



unsigned bp2_hard[] =
{
	0x00BB00, 0x030D00, 0x00B700, 0x014200,
	0x080700, 0x00c900, 0x360101, 0x8A0201,
	0x990201, 0x510301, 0x5B0301, 0x520301,
	0x001003, 0x0A1803
};

unsigned bp2_vhard[] = 
{
	0x00BA00, 0x00B400, 0x030D00, 0x00B600,
	0x00B300, 0x080700, 0x014300, 0x00c900,
	0x360101, 0x8A0201, 0x990201, 0x850201,
	0x480301, 0x510301, 0x5B0301, 0x520301,
	0x001003
};


unsigned bp2_ultimate[] =
{
	0x00BA00, 0x00B400, 0x030D00, 0x00B600,
	0x00B300, 0x080700, 0x014300, 0x00c900,
	0x360101, 0x8A0201, 0x990201, 0x850201,
	0x480301, 0x510301, 0x5B0301, 0x520301
};


// Tekking Attribute arrays

unsigned char tekker_attributes[] = 
{
	0x00, 0x00, 0x00,
	0x01, 0x01, 0x02,
	0x02, 0x01, 0x02,
	0x03, 0x02, 0x04,
	0x04, 0x03, 0x04,
	0x05, 0x05, 0x06,
	0x06, 0x05, 0x07,
	0x07, 0x06, 0x08,
	0x08, 0x07, 0x08,
	0x09, 0x09, 0x0A,
	0x0A, 0x09, 0x0B,
	0x0B, 0x0A, 0x0B,
	0x0C, 0x0C, 0x0C,
	0x0D, 0x0D, 0x0D,
	0x0E, 0x0E, 0x0E,
	0x0F, 0x0F, 0x10,
	0x10, 0x0F, 0x11,
	0x11, 0x10, 0x12,
	0x12, 0x11, 0x12,
	0x13, 0x13, 0x14,
	0x14, 0x13, 0x15,
	0x15, 0x14, 0x15,
	0x16, 0x15, 0x16,
	0x17, 0x17, 0x18,
	0x18, 0x17, 0x19,
	0x19, 0x18, 0x1A,
	0x1A, 0x19, 0x1A,
	0x1B, 0x1B, 0x1C,
	0x1C, 0x1B, 0x1D,
	0x1D, 0x1C, 0x1E,
	0x1E, 0x1D, 0x1E,
	0x1F, 0x1F, 0x20,
	0x20, 0x1F, 0x21,
	0x21, 0x20, 0x22,
	0x22, 0x21, 0x22,
	0x23, 0x23, 0x24,
	0x24, 0x23, 0x25,
	0x25, 0x24, 0x26,
	0x26, 0x25, 0x26,
	0x27, 0x27, 0x28,
	0x28, 0x27, 0x28,
};


// Experience required to each level

unsigned tnlxp[200] =
{
	50,
	150,
	250,
	350,
	450,
	550,
	650,
	750,
	850,
	950,
	1050,
	1150,
	1250,
	1398,
	1579,
	1825,
	2112,
	2421,
	2754,
	3105,
	3474,
	3861,
	4266,
	4679,
	5098,
	5526,
	5967,
	6420,
	6881,
	7349,
	7828,
	8316,
	8817,
	9329,
	9855,
	10388,
	10927,
	11478,
	12042,
	12617,
	13200,
	13794,
	14397,
	15013,
	15641,
	16279,
	16926,
	17583,
	18252,
	18932,
	19625,
	20326,
	21036,
	21758,
	22492,
	23233,
	23989,
	24753,
	25522,
	26302,
	27093,
	27892,
	28702,
	29521,
	30352,
	31191,
	32038,
	32898,
	33769,
	34649,
	35540,
	36444,
	37355,
	38274,
	39204,
	40145,
	41098,
	42060,
	43031,
	44010,
	45011,
	46033,
	47076,
	48147,
	49251,
	50384,
	51553,
	52764,
	54021,
	55326,
	56683,
	58098,
	59576,
	61122,
	62741,
	64443,
	66229,
	68109,
	70087,
	72168,
	76261,
	80550,
	85041,
	89730,
	94620,
	99715,
	105106,
	110799,
	116798,
	123098,
	129696,
	136593,
	143787,
	151286,
	159084,
	167184,
	175685,
	184584,
	193885,
	203587,
	213685,
	223883,
	234179,
	244577,
	255076,
	265672,
	276368,
	287165,
	298061,
	309059,
	320159,
	331356,
	342649,
	354046,
	365545,
	377144,
	388842,
	400637,
	412535,
	424533,
	436632,
	448832,
	461134,
	473538,
	486045,
	498650,
	511352,
	524152,
	537053,
	550053,
	563155,
	576360,
	589664,
	603071,
	616578,
	630186,
	643996,
	658012,
	672224,
	686635,
	701247,
	716062,
	731074,
	746285,
	761701,
	777423,
	793449,
	809873,
	826700,
	844048,
	861986,
	880613,
	899964,
	920301,
	941658,
	964238,
	988241,
	1013827,
	1041196,
	1070387,
	1103565,
	1142725,
	1189775,
	1246811,
	1315744,
	1398635,
	1497718,
	1614624,
	1751532,
	1910009,
	2092175,
	2301028,
	2537846,
	2804676,
	3108493,
	3452189,
	3842757,
	4283324,
	4812416,
	99999999
};


// Flags set in the A2 packet when viewing the "Main Story" quest category
// for Episode 1 One-Person

unsigned short ep1_unlocks[] = 
{
	0x13c2, 0x11a4,
	0x1536, 0xb53,
	0x58f, 0xbb7,
	0x1487, 0x1548,
	0x2372, 0xe46,
	0x18a7, 0x13fc,
	0x28e0, 0xb86,
	0x2a0, 0xfea,
	0x2a90, 0x1258,
	0x4f38, 0xa99,
	0x2962, 0xde,
	0x2e7e, 0x1386,
	0x452b, 0x11c9,
	0x2cf3, 0x15f6,
	0x7c2, 0x10a,
	0x5194, 0x1584,
	0x2b20, 0xecc,
	0x1936, 0x1156,
	0x3069, 0x1b3b,
	0x1aa3, 0x17ef,
	0x3272, 0x1be6,
	0x3f39, 0x80bf,
	0x22dd, 0xeec,
	0x3df7, 0x1db4,
	0x3d24, 0x1ff9,
	0x321a, 0x2db1
};


// Character Section IDs

enum SectionIDs {
	ID_Viridia,
	ID_Greennill,
	ID_Skyly,
	ID_Bluefull,
	ID_Purplenum,
	ID_Pinkal,
	ID_Redria,
	ID_Oran,
	ID_Yellowboze,
	ID_Whitill
};


// Mag PB Types

enum PBTypes {
	PB_Farlla,
	PB_Estlla,
	PB_Golla,
	PB_Pilla,
	PB_Leilla,
	PB_Mylla_Youlla
};


// Mag Types

enum MagTypes {
   Mag_Mag,
   Mag_Varuna,
   Mag_Mitra,
   Mag_Surya,
   Mag_Vayu,
   Mag_Varaha,
   Mag_Kama,
   Mag_Ushasu,
   Mag_Apsaras,
   Mag_Kumara,
   Mag_Kaitabha,
   Mag_Tapas,
   Mag_Bhirava,
   Mag_Kalki,
   Mag_Rudra,
   Mag_Marutah,
   Mag_Yaksa,
   Mag_Sita,
   Mag_Garuda,
   Mag_Nandin,
   Mag_Ashvinau,
   Mag_Ribhava,
   Mag_Soma,
   Mag_Ila,
   Mag_Durga,
   Mag_Vritra,
   Mag_Namuci,
   Mag_Sumba,
   Mag_Naga,
   Mag_Pitri,
   Mag_Kabanda,
   Mag_Ravana,
   Mag_Marica,
   Mag_Soniti,
   Mag_Preta,
   Mag_Andhaka,
   Mag_Bana,
   Mag_Naraka,
   Mag_Madhu,
   Mag_Churel,
   Mag_RoboChao,
   Mag_OpaOpa,
   Mag_Pian,
   Mag_Chao,
   Mag_CHUCHU,
   Mag_KAPUKAPU,
   Mag_ANGELSWING,
   Mag_DEVILSWING,
   Mag_ELENOR,
   Mag_MARK3,
   Mag_MASTERSYSTEM,
   Mag_GENESIS,
   Mag_SEGASATURN,
   Mag_DREAMCAST,
   Mag_HAMBURGER,
   Mag_PANZERSTAIL,
   Mag_DEVILSTAIL,
   Mag_Deva,
   Mag_Rati,
   Mag_Savitri,
   Mag_Rukmin,
   Mag_Pushan,
   Mag_Dewari,
   Mag_Sato,
   Mag_Bhima,
   Mag_Nidra,
   Mag_Geungsi,
   Mag_BlankMag,
   Mag_Tellusis,
   Mag_StrikerUnit,
   Mag_Pioneer,
   Mag_Puyo,
   Mag_Muyo,
   Mag_Rappy,
   Mag_Yahoo,  // renamed
   Mag_Gaelgiel,
   Mag_Agastya,
   Mag_0503,    // They do exist lol
   Mag_0504,
   Mag_0505,
   Mag_0506,
   Mag_0507};


// Mag Feeding Tables (updated by Lee from schtserv.com)

   short Feed_Table0[11*6] =
   {
      3, 3, 5, 40, 5, 0,
      3, 3, 10, 45, 5, 0,
      4, 4, 15, 50, 10, 0,
      3, 3, 5, 0, 5, 40,
      3, 3, 10, 0, 5, 45,
      4, 4, 15, 0, 10, 50,
      3, 3, 5, 10, 40, 0,
      3, 3, 5, 0, 44, 10,
      4, 1, 15, 30, 15, 25,
      4, 1, 15, 25, 15, 30,
      6, 5, 25, 25, 25, 25
   };

   short Feed_Table1[11*6] =
   {
      0, 0, 5, 10, 0, -1,
      2, 1, 6, 15, 3, -3,
      3, 2, 12, 21, 4, -7,
      0, 0, 5, 0, 0, 8,
      2, 1, 7, 0, 3, 13,
      3, 2, 7, -7, 6, 19,
      0, 1, 0, 5, 15, 0,
      2, 0, -1, 0, 14, 5,
      -2, 2, 10, 11, 8, 0,
      3, -2, 9, 0, 9, 11,
      4, 3, 14, 9, 18, 11
   };

   short Feed_Table2[11*6] =
   {
      0, -1, 1, 9, 0, -5,
      3, 0, 1, 13, 0, -10,
      4, 1, 8, 16, 2, -15,
      0, -1, 0, -5, 0, 9,
      3, 0, 4, -10, 0, 13,
      3, 2, 6, -15, 5, 17,
      -1, 1, -5, 4, 12, -5,
      0, 0, -5, -6, 11, 4,
      4, -2, 0, 11, 3, -5,
      -1, 1, 4, -5, 0, 11,
      4, 2, 7, 8, 6, 9
   };

   short Feed_Table3[11*6] =
   {
      0, -1, 0, 3, 0, 0,
      2, 0, 5, 7, 0, -5,
      3, 1, 4, 14, 6, -10,
      0, 0, 0, 0, 0, 4,
      0, 1, 4, -5, 0, 8,
      2, 2, 4, -10, 3, 15,
      -3, 3, 0, 0, 7, 0,
      3, 0, -4, -5, 20, -5,
      3, -2, -10, 9, 6, 9,
      -2, 2, 8, 5, -8, 7,
      3, 2, 7, 7, 7, 7
   };

   short Feed_Table4[11*6] =
   {
      2, -1, -5, 9, -5, 0,
      2, 0, 0, 11, 0, -10,
      0, 1, 4, 14, 0, -15,
      2, -1, -5, 0, -6, 10,
      2, 0, 0, -10, 0, 11,
      0, 1, 4, -15, 0, 15,
      2, -1, -5, -5, 16, -5,
      -2, 3, 7, -3, 0, -3,
      4, -2, 5, 21, -5, -20,
      3, 0, -5, -20, 5, 21,
      3, 2, 4, 6, 8, 5
   };

   short Feed_Table5[11*6] =
   {
      2, -1, -4, 13, -5, -5,
      0, 1, 0, 16, 0, -15,
      2, 0, 3, 19, -2, -18,
      2, -1, -4, -5, -5, 13,
      0, 1, 0, -15, 0, 16,
      2, 0, 3, -20, 0, 19,
      0, 1, 5, -6, 6, -5,
      -1, 1, 0, -4, 14, -10,
      4, -1, 4, 17, -5, -15,
      2, 0, -10, -15, 5, 21,
      3, 2, 2, 8, 3, 6
   };

   short Feed_Table6[11*6] =
   {
      -1, 1, -3, 9, -3, -4,
      2, 0, 0, 11, 0, -10,
      2, 0, 2, 15, 0, -16,
      -1, 1, -3, -4, -3, 9,
      2, 0, 0, -10, 0, 11,
      2, 0, -2, -15, 0, 19,
      2, -1, 0, 6, 9, -15,
      -2, 3, 0, -15, 9, 6,
      3, -1, 9, -20, -5, 17,
      0, 2, -5, 20, 5, -20,
      3, 2, 0, 11, 0, 11
   };

   short Feed_Table7[11*6] =
   {
      -1, 0, -4, 21, -15, -5, // Fixed the 2 incorrect bytes in table 7 (was cell table)
      0, 1, -1, 27, -10, -16,
      2, 0, 5, 29, -7, -25,
      -1, 0, -10, -5, -10, 21,
      0, 1, -5, -16, -5, 25,
      2, 0, -7, -25, 6, 29,
      -1, 1, -10, -10, 28, -10,
      2, -1, 9, -18, 24, -15,
      2, 1, 19, 18, -15, -20,
      2, 1, -15, -20, 19, 18,
      4, 2, 3, 7, 3, 3
   }; 

// Items Obtained from Jack-O-Lantern

unsigned jacko_drops[8] = { 0x0C03, 0x010C03, 0x0B0E03, 0x030C03, 0x260E03, 0x050C03, 0x020C03, 0x040C03 };


// Items Obtained from Easter Egg

unsigned easter_drops[9] = { 0x3700, 0x3100, 0x6100, 0x4A00, 0x4700, 0x5500, 0xA400, 0x5F00, 0x030F00 };


// Shop Sellback Price per Attribute

int attrib [] =
{
	0,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	500,
	1125,
	2000,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	1125,
	2000
};

// Shop Sellback Price per Common Armor

int armor_prices[] = 
{
	24, 33, 41, 50, 59,
	65, 100, 135, 170, 205,
	123, 185, 246, 307, 368,
	201, 288, 376, 463, 551,
	305, 418, 532, 646, 760,
	415, 555, 695, 835, 975,
	556, 723, 889, 1055, 1221,
	708, 910, 1111, 1312, 1513,
	896, 1132, 1368, 1605, 1841,
	1081, 1352, 1623, 1895, 2166,
	1306, 1612, 1918, 2225, 2531,
	1523, 1865, 2206, 2547, 2888,
	1786, 2162, 2538, 2915, 3291,
	2036, 2448, 2859, 3270, 3681,
	2336, 2783, 3229, 3675, 4121,
	2620, 3101, 3582, 4063, 4545,
	2957, 3473, 3990, 4506, 5022,
	3273, 3825, 4376, 4927, 5478,
	3648, 4235, 4821, 5407, 5993,
	3997, 4618, 5240, 5861, 6482,
	4410, 5066, 5722, 6378, 7035,
	4783, 5465, 6148, 6830, 7513,
	5215, 5915, 6615, 7315, 8015,
	6503, 7247, 7991, 8735, 9478
};


// Shop Sellback Price per Common Unit

int unit_prices [0x40] = 
{
	250,
	375,
	625,
	10,
	250,
	375,
	625,
	10,
	250,
	375,
	625,
	10,
	250,
	375,
	625,
	10,
	250,
	500,
	750,
	10,
	250,
	500,
	750,
	10,
	250,
	375,
	625,
	10,
	1000,
	10,
	1000,
	10,
	10,
	250,
	375,
	625,
	250,
	375,
	625,
	250,
	375,
	625,
	375,
	500,
	750,
	375,
	500,
	625,
	875,
	1000,
	10,
	500,
	750,
	10,
	500,
	750,
	10,
	500,
	750,
	10,
	750,
	10,
	10,
	750
};


// Shop Sellback Price per Technique

int tech_prices[] = 
{
	1250,
	3750,
	5625,
	1250,
	3750,
	5625,
	1250,
	3750,
	5625,
	6250,
	1250,
	1250,
	1250,
	1250,
	62500,
	3750,
	1250,
	62500,
	6250,
};


// Shop Sellback Price per Tool

int tool_prices[] =
{
	0x000003, 6,
	0x010003, 37,
	0x020003, 250,
	0x000103, 12,
	0x010103, 62,
	0x020103, 450,
	0x000303, 37,
	0x000403, 62,
	0x000503, 625,
	0x000603, 7,
	0x010603, 7,
	0x000703, 43,
	0x000803, 12,
	0x000903, 1250,
	0x000A03, 625,
	0x010A03, 1250,
	0x020A03, 1875,
	0x000B03, 300,
	0x010B03, 300,
	0x020B03, 300,
	0x030B03, 300,
	0x040B03, 300,
	0x050B03, 300,
	0x060B03, 300,
	0x001003, 1000,
	0x011003, 2000,
	0x021003, 3000
};

// Item list for Good Luck quest...

unsigned good_luck[] =
{
	0x00004C00,
	0x00004500,
	0x00078F00,
	0x00004400,
	0x00009A00,
	0x00006A00,
	0x002B0201,
	0x00310101,
	0x00300101,
	0x00320101,
	0x00003400,
	0x00001300,
	0x00003C00,
	0x00002700,
	0x00003E00,
	0x00003900,
	0x00004100,
	0x00290101,
	0x00290101,
	0x00290201,
	0x00350201,
	0x00330101,
	0x001C0101,
	0x00002000,
	0x00060600,
	0x00002C00,
	0x00060100,
	0x00070100,
	0x00210201,
	0x00070800,
	0x00070400,
	0x00010D00,
	0x00240201,
	0x001B0101,
	0x00060200,
	0x00001003
};