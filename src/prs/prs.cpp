#include <memory.h>

typedef unsigned long u32;
typedef unsigned char u8;
typedef unsigned long DWORD;

////////////////////////////////////////////////////////////////////////////////

typedef struct {
    u8 bitpos;
    //u8 controlbyte;
    u8* controlbyteptr;
    u8* srcptr_orig;
    u8* dstptr_orig;
    u8* srcptr;
    u8* dstptr;
} PRS_COMPRESSOR;

void prs_put_control_bit(PRS_COMPRESSOR* pc,u8 bit)
{
    *pc->controlbyteptr = *pc->controlbyteptr >> 1;
    *pc->controlbyteptr |= ((!!bit) << 7);
    pc->bitpos++;
    if (pc->bitpos >= 8)
    {
        pc->bitpos = 0;
        pc->controlbyteptr = pc->dstptr;
        pc->dstptr++;
    }
}

void prs_put_control_bit_nosave(PRS_COMPRESSOR* pc,u8 bit)
{
    *pc->controlbyteptr = *pc->controlbyteptr >> 1;
    *pc->controlbyteptr |= ((!!bit) << 7);
    pc->bitpos++;
}

void prs_put_control_save(PRS_COMPRESSOR* pc)
{
    if (pc->bitpos >= 8)
    {
        pc->bitpos = 0;
        pc->controlbyteptr = pc->dstptr;
        pc->dstptr++;
    }
}

void prs_put_static_data(PRS_COMPRESSOR* pc,u8 data)
{
    *pc->dstptr = data;
    pc->dstptr++;
}

u8 prs_get_static_data(PRS_COMPRESSOR* pc)
{
    u8 data = *pc->srcptr;
    pc->srcptr++;
    return data;
}

////////////////////////////////////////////////////////////////////////////////

void prs_init(PRS_COMPRESSOR* pc,void* src,void* dst)
{
    pc->bitpos = 0;
    //pc->controlbyte = 0;
    pc->srcptr = (u8*)src;
    pc->srcptr_orig = (u8*)src;
    pc->dstptr = (u8*)dst;
    pc->dstptr_orig = (u8*)dst;
    pc->controlbyteptr = pc->dstptr;
    pc->dstptr++;
}

void prs_finish(PRS_COMPRESSOR* pc)
{
    //printf("> > > prs_finish[1]: %08X->%08X\n",pc->srcptr - pc->srcptr_orig,pc->dstptr - pc->dstptr_orig);
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,1);
    //printf("> > > prs_finish[2]: %08X->%08X %d\n",pc->srcptr - pc->srcptr_orig,pc->dstptr - pc->dstptr_orig,pc->bitpos);
    //pc->controlbyte = pc->controlbyte << (8 - pc->bitpos);
    if (pc->bitpos != 0)
    {
        *pc->controlbyteptr = ((*pc->controlbyteptr << pc->bitpos) >> 8);
        //*pc->controlbyteptr = pc->controlbyte;
        //pc->dstptr++;
    }
    //printf("> > > prs_finish[3]: %08X->%08X\n",pc->srcptr - pc->srcptr_orig,pc->dstptr - pc->dstptr_orig);
    prs_put_static_data(pc,0);
    prs_put_static_data(pc,0);
    //printf("> > > prs_finish[4]: %08X->%08X\n",pc->srcptr - pc->srcptr_orig,pc->dstptr - pc->dstptr_orig);
}

void prs_rawbyte(PRS_COMPRESSOR* pc)
{
    prs_put_control_bit_nosave(pc,1);
    prs_put_static_data(pc,prs_get_static_data(pc));
    prs_put_control_save(pc);
}

void prs_shortcopy(PRS_COMPRESSOR* pc,int offset,u8 size)
{
    size -= 2;
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,0);
    prs_put_control_bit(pc,(size >> 1) & 1);
    prs_put_control_bit_nosave(pc,size & 1);
    prs_put_static_data(pc,offset & 0xFF);
    prs_put_control_save(pc);
}

void prs_longcopy(PRS_COMPRESSOR* pc,int offset,u8 size)
{
    //u8 byte1,byte2;
    if (size <= 9)
    {
        //offset = ((offset << 3) & 0xFFF8) | ((size - 2) & 7);
        prs_put_control_bit(pc,0);
        prs_put_control_bit_nosave(pc,1);
        prs_put_static_data(pc,((offset << 3) & 0xF8) | ((size - 2) & 0x07));
        prs_put_static_data(pc,(offset >> 5) & 0xFF);
        prs_put_control_save(pc);
    } else {
        //offset = (offset << 3) & 0xFFF8;
        prs_put_control_bit(pc,0);
        prs_put_control_bit_nosave(pc,1);
        prs_put_static_data(pc,(offset << 3) & 0xF8);
        prs_put_static_data(pc,(offset >> 5) & 0xFF);
        prs_put_static_data(pc,size - 1);
        prs_put_control_save(pc);
        //printf("[%08X, %08X, %02X%02X %02X]",offset,size,(offset >> 5) & 0xFF,(offset << 3) & 0xF8,size - 1);
    }
}

void prs_copy(PRS_COMPRESSOR* pc,int offset,u8 size)
{
    /*bool err = false;
    if (((offset & 0xFFFFE000) != 0xFFFFE000) || ((offset & 0x1FFF) == 0))
    {
        printf("> > > error: bad offset: %08X\n",offset);
        err = true;
    }
    if ((size > 0xFF) || (size < 3))
    {
        printf("> > > error: bad size: %08X\n",size);
        err = true;
    }
    if (err) system("PAUSE"); */
    //else printf("> > > prs_copy: %08X->%08X @ %08X:%08X\n",pc->srcptr - pc->srcptr_orig,pc->dstptr - pc->dstptr_orig,offset,size);
    if ((offset > -0x100) && (size <= 5))
    {
        //printf("> > > %08X->%08X sdat %08X %08X\n",pc->srcptr - pc->srcptr_orig,pc->dstptr - pc->dstptr_orig,offset,size);
        prs_shortcopy(pc,offset,size);
    } else {
        //printf("> > > %08X->%08X ldat %08X %08X\n",pc->srcptr - pc->srcptr_orig,pc->dstptr - pc->dstptr_orig,offset,size);
        prs_longcopy(pc,offset,size);
    }
    pc->srcptr += size;
}

////////////////////////////////////////////////////////////////////////////////

u32 prs_compress(void* source,void* dest,u32 size)
{
    PRS_COMPRESSOR pc;
    int x,y; // int z;
    u32 xsize;
    int lsoffset,lssize;

    if (size > 2147483648) // keep within signed range
    {
	printf ("prs_compress failure\n");
    	memcpy (dest,source,size);
	return size;
    }
    prs_init(&pc,source,dest);
    //printf("\n> compressing %08X bytes\n",size);
    for (x = 0; x < (int) size; x++)
    {
        lsoffset = lssize = xsize = 0;
        for (y = x - 3; (y > 0) && (y > (x - 0x1FF0)) && (xsize < 255); y--)
        {
            xsize = 3;
            if (!memcmp((void*)((DWORD)source + y),(void*)((DWORD)source + x),xsize))
            {
                do xsize++;
                while (!memcmp((void*)((DWORD)source + y),
                               (void*)((DWORD)source + x),
                               xsize) &&
                       (xsize < 256) &&
                       ((y + (int)xsize) < x) &&
                       ((x + xsize) <= (int)size)
                );
                xsize--;
                if ((int)xsize > lssize)
                {
                    lsoffset = -(x - y);
                    lssize = xsize;
                }
            }
        }
        if (lssize == 0)
        {
            //printf("> > > %08X->%08X byte\n",x,pc.dstptr - pc.dstptr_orig);
            //pc.srcptr = (u8*)((DWORD)source + x);
            prs_rawbyte(&pc);
        } else {
            //if (lssize > 250) lssize = 250;
            prs_copy(&pc,lsoffset,lssize);
            x += (lssize - 1);
        }
    }
    prs_finish(&pc);
    return pc.dstptr - pc.dstptr_orig;
}

////////////////////////////////////////////////////////////////////////////////

u32 prs_decompress(void* source,void* dest) // 800F7CB0 through 800F7DE4 in mem 
{
    u32 r3,r5; // u32 r0,r6,r9; // 6 unnamed registers 
    u32 bitpos = 9; // 4 named registers 
    u8* sourceptr = (u8*)source;
    u8* sourceptr_orig = (u8*)source;
    u8* destptr = (u8*)dest;
    u8* destptr_orig = (u8*)dest;
    u8 currentbyte;
    int flag;
    int offset;
    u32 x,t; // 2 placed variables 

    //printf("\n> decompressing\n");
    currentbyte = sourceptr[0];
    sourceptr++;
    for (;;)
    {
        bitpos--;
        if (bitpos == 0)
        {
            currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            destptr[0] = sourceptr[0];
            //printf("> > > %08X->%08X byte\n",sourceptr - sourceptr_orig,destptr - destptr_orig);
            sourceptr++;
            destptr++;
            continue;
        }
        bitpos--;
        if (bitpos == 0)
        {
            currentbyte = sourceptr[0];
            bitpos = 8;
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            r3 = sourceptr[0] & 0xFF;
            //printf("> > > > > first: %02X - ",sourceptr[0]); system("PAUSE");
            offset = ((sourceptr[1] & 0xFF) << 8) | r3;
            //printf("> > > > > second: %02X - ",sourceptr[1]); system("PAUSE");
            sourceptr += 2;
            if (offset == 0) return (u32)(destptr - destptr_orig);
            r3 = r3 & 0x00000007;
            r5 = (offset >> 3) | 0xFFFFE000;
            if (r3 == 0)
            {
                flag = 0;
                r3 = sourceptr[0] & 0xFF;
                sourceptr++;
                r3++;
            } else r3 += 2;
            r5 += (u32)destptr;
            //printf("> > > %08X->%08X ldat %08X %08X %s\n",sourceptr - sourceptr_orig,destptr - destptr_orig,r5 - (u32)destptr,r3,flag ? "inline" : "extended");
        } else {
            r3 = 0;
            for (x = 0; x < 2; x++)
            {
                bitpos--;
                if (bitpos == 0)
                {
                    currentbyte = sourceptr[0];
                    bitpos = 8;
                    sourceptr++;
                }
                flag = currentbyte & 1;
                currentbyte = currentbyte >> 1;
                offset = r3 << 1;
                r3 = offset | flag;
            }
            offset = sourceptr[0] | 0xFFFFFF00;
            r3 += 2;
            sourceptr++;
            r5 = offset + (u32)destptr;
            //printf("> > > %08X->%08X sdat %08X %08X\n",sourceptr - sourceptr_orig,destptr - destptr_orig,r5 - (u32)destptr,r3);
        }
        if (r3 == 0) continue;
        t = r3;
        for (x = 0; x < t; x++)
        {
            destptr[0] = *(u8*)r5;
            r5++;
            r3++;
            destptr++;
        }
    }
}

u32 prs_decompress_size(void* source)
{
    u32 r3,r5; // u32 r0,r6,r9; // 6 unnamed registers 
    u32 bitpos = 9; // 4 named registers 
    u8* sourceptr = (u8*)source;
    u8* destptr = 0;
    u8* destptr_orig = 0;
    u8 currentbyte,lastbyte;
    int flag;
    int offset;
    u32 x,t; // 2 placed variables 

    //printf("> %08X -> %08X: begin\n",sourceptr,destptr);
    currentbyte = sourceptr[0];
    //printf("> [ ] %08X -> %02X: command stream\n",sourceptr,currentbyte);
    sourceptr++;
    for (;;)
    {
        bitpos--;
        if (bitpos == 0)
        {
            lastbyte = currentbyte = sourceptr[0];
            bitpos = 8;
            //printf("> [ ] %08X -> %02X: command stream\n",sourceptr,currentbyte);
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            //printf("> [1] %08X -> %08X: %02X\n",sourceptr,destptr,*sourceptr);
            sourceptr++;
            destptr++;
            continue;
        }
        //printf("> [0] extended\n");
        bitpos--;
        if (bitpos == 0)
        {
            lastbyte = currentbyte = sourceptr[0];
            bitpos = 8;
            //printf("> [ ] %08X -> %02X: command stream\n",sourceptr,currentbyte);
            sourceptr++;
        }
        flag = currentbyte & 1;
        currentbyte = currentbyte >> 1;
        if (flag)
        {
            r3 = sourceptr[0];
            offset = (sourceptr[1] << 8) | r3;
            sourceptr += 2;
            if (offset == 0) return (u32)(destptr - destptr_orig);
            r3 = r3 & 0x00000007;
            r5 = (offset >> 3) | 0xFFFFE000;
            if (r3 == 0)
            {
                r3 = sourceptr[0];
                sourceptr++;
                r3++;
            } else r3 += 2;
            r5 += (u32)destptr;
            //printf("> > [1] %08X -> %08X: block copy (%d)\n",r5,destptr,r3);
        } else {
            r3 = 0;
            for (x = 0; x < 2; x++)
            {
                bitpos--;
                if (bitpos == 0)
                {
                    lastbyte = currentbyte = sourceptr[0];
                    bitpos = 8;
                    //printf("> [ ] %08X -> %02X: command stream\n",sourceptr,currentbyte);
                    sourceptr++;
                }
                flag = currentbyte & 1;
                currentbyte = currentbyte >> 1;
                offset = r3 << 1;
                r3 = offset | flag;
            }
            offset = sourceptr[0] | 0xFFFFFF00;
            r3 += 2;
            sourceptr++;
            r5 = offset + (u32)destptr;
            //printf("> > [0] %08X -> %08X: block copy (%d)\n",r5,destptr,r3);
        }
        if (r3 == 0) continue;
        t = r3;
        //printf("> > [ ] copying %d bytes\n",t);
        for (x = 0; x < t; x++)
        {
            r5++;
            r3++;
            destptr++;
        }
    }
}

