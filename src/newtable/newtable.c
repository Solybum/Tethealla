/* random number functions */

#include <stdio.h>

extern void		mt_bestseed(void);
extern void		mt_seed(void);	/* Choose seed from random input. */
extern unsigned long	mt_lrand(void);	/* Generate 32-bit random value */

typedef struct st_pcrys
{
	unsigned tbl[18];
} PCRYS;

void main()
{
	PCRYS pcrys;
	PCRYS* pcry;
	unsigned value;
	FILE* fp;
	FILE* bp;
	unsigned ch;

	pcry = &pcrys;
	mt_bestseed();
		
	fp = fopen ("bbtable.h","w");
	bp = fopen ("bbtable.bin", "wb");
	fprintf (fp, "\n\nstatic const unsigned long bbtable[18+1024] =\n{\n");
	for (ch=0;ch<1024+18;ch++)
	{
		value = mt_lrand();
		fprintf (fp, "0x%08x,\n", value, value );
		fwrite (&value, 1, 4, bp);
	}
	fprintf (fp, "};\n");
	fclose (fp);

}