#ifndef _PSO_CRYPT_H_
#define _PSO_CRYPT_H_

#define TABLE_SIZE (1024 + 18)

#define PSO_CRYPT_TYPE_DC 0
#define PSO_CRYPT_TYPE_GC 1
#define PSO_CRYPT_TYPE_BB 2

typedef struct pso_crypt {
	unsigned long tbl[TABLE_SIZE];
	int type;
	int cur;
	int size;
	void (*mangle)(struct pso_crypt *);
} PSO_CRYPT;

/* for DC */
void pso_crypt_table_init_dc(PSO_CRYPT *pcry, const unsigned char *salt);

/* for GC */
void pso_crypt_table_init_gc(PSO_CRYPT *pcry, const unsigned char *salt);

/* for BB */
void pso_crypt_table_init_bb(PSO_CRYPT *pcry, const unsigned char *salt);
void pso_crypt_decrypt_bb(PSO_CRYPT *pcry, unsigned char *data, unsigned
	length);
void pso_crypt_encrypt_bb(PSO_CRYPT *pcry, unsigned char *data, unsigned
	length);

/* common */
void pso_crypt_init(PSO_CRYPT *pcry, const unsigned char *salt, int type);
void pso_crypt(PSO_CRYPT *pcry, unsigned char *data, int len, int enc);
unsigned long pso_crypt_get_num(PSO_CRYPT *pcry);

#endif /* _PSO_CRYPT_H_ */
