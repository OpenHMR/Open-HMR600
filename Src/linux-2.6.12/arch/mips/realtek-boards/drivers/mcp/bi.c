#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/mach-venus/mcp/bi.h>



///////////////////////////////////////////////////////////////////////////////////////////////

#ifdef do_div64_32
#undef do_div64_32
#endif


#ifdef do_div
#undef do_div
#endif

#define do_div64_32(res, high, low, base) ({ \
        unsigned long __quot32, __mod32; \
        unsigned long __cf, __tmp, __tmp2, __i; \
        \
        __asm__(".set   push\n\t" \
                ".set   noat\n\t" \
                ".set   noreorder\n\t" \
                "move   %2, $0\n\t" \
                "move   %3, $0\n\t" \
                "b      1f\n\t" \
                " li    %4, 0x21\n" \
                "0:\n\t" \
                "sll    $1, %0, 0x1\n\t" \
                "srl    %3, %0, 0x1f\n\t" \
                "or     %0, $1, %5\n\t" \
                "sll    %1, %1, 0x1\n\t" \
                "sll    %2, %2, 0x1\n" \
                "1:\n\t" \
                "bnez   %3, 2f\n\t" \
                " sltu  %5, %0, %z6\n\t" \
                "bnez   %5, 3f\n" \
                "2:\n\t" \
                " addiu %4, %4, -1\n\t" \
                "subu   %0, %0, %z6\n\t" \
                "addiu  %2, %2, 1\n" \
                "3:\n\t" \
                "bnez   %4, 0b\n\t" \
                " srl   %5, %1, 0x1f\n\t" \
                ".set   pop" \
                : "=&r" (__mod32), "=&r" (__tmp), \
                  "=&r" (__quot32), "=&r" (__cf), \
                  "=&r" (__i), "=&r" (__tmp2) \
                : "Jr" (base), "0" (high), "1" (low)); \
        \
        (res) = __quot32; \
        __mod32; })


#define do_div(n, base) ({ \
        unsigned long long __quot; \
        unsigned long __mod; \
        unsigned long long __div; \
        unsigned long __upper, __low, __high, __base; \
        \
        __div = (n); \
        __base = (base); \
        \
        __high = __div >> 32; \
        __low = __div; \
        __upper = __high; \
        \
        if (__high) \
                __asm__("divu   $0, %z2, %z3" \
                        : "=h" (__upper), "=l" (__high) \
                        : "Jr" (__high), "Jr" (__base) \
                        : "$0"); \
        \
        __mod = do_div64_32(__low, __upper, __low, __base); \
        \
        __quot = __high; \
        __quot = __quot << 32 | __low; \
        __quot; })


#ifdef BI_ALLOC_DEBUG

static int alloc_count = 0;

void *bi_osal_malloc(unsigned int NBYTES)
{
	void *p  = kmalloc(NBYTES, GFP_KERNEL);	
	alloc_count++;
    printk("[MCP][BI] mcpb_aloc mem %p, len=%d, cnt=%d\n", p ,NBYTES, alloc_count);     
	return p;
}

void bi_osal_free(void *p)
{	
    alloc_count--;
    printk("[MCP][BI] mcpb_free mem %p, cnt=%d\n", p, alloc_count);     
	kfree(p);
}

#else

    #define bi_osal_malloc(NBYTES)      kmalloc(NBYTES, GFP_KERNEL)
    #define bi_osal_free(p)             kfree(p)
#endif

#define bi_print    printk
///////////////////////////////////////////////////////////////////////////////////////////////






BI *init_BI(void)
{
	BI *p;
	
	p = (BI *) bi_osal_malloc(sizeof(BI));
	if (p == NULL) {
		bi_print("init_BI error!\n");
		return NULL;
	}
	
	memset(p, 0, sizeof(BI));
	p->m_nSign=1;
	p->m_nLength=1;

	return p;
}





void free_BI(BI* p)
{
    bi_osal_free(p);
}


BI *move(BI *A)
{	
	BI *p;
	
	p = init_BI();
	if (p == NULL)
		return NULL;

	memcpy(p, A, sizeof(BI));
	return p;
}

/*
BI *set(char *str)
{
	int i;
	BI *p;
	
	p = init_BI();
	if (p == NULL)
		return NULL;


	return p;
}
*/
BI *move_p(unsigned long long b)
{
	int i;
	BI *p;
	
	p = init_BI();
	if (p == NULL)
		return NULL;

	if(b > 0xffffffff)
	{
		p->m_nLength=2;
		p->m_ulValue[1]=(unsigned int)(b>>32);
		p->m_ulValue[0]=(unsigned int)b;
	}
	else
	{
		p->m_nLength=1;
		p->m_ulValue[0]=(unsigned int)b;
	}
	
	for(i=p->m_nLength;i<BI_MAXLEN;i++)
		p->m_ulValue[i]=0;
	return p;
}

/*
 *  0: A = B
 *  1: A > B
 * -1: A < B
 *
 */
int Cmp(BI *A, BI *B)
{
	int i;
	
	if(A->m_nLength > B->m_nLength)return 1;
	if(A->m_nLength < B->m_nLength)return -1;
	for(i=A->m_nLength-1;i>=0;i--)
	{
		if(A->m_ulValue[i] > B->m_ulValue[i])return 1;
		if(A->m_ulValue[i] < B->m_ulValue[i])return -1;
	}
	return 0;
}

int isZero(BI *A)
{
	if ((A->m_nLength == 1) && (A->m_ulValue[0] == 0))
		return 1;
	else
		return 0;
}

void SetZero(BI *A)
{
	memset(A, 0, sizeof(BI));
	A->m_nSign=1;
	A->m_nLength=1;
}

/*
 * A = A + B
 *
 */
void Add(BI *A, BI *B)
{
	int i;
	unsigned int carry;
	unsigned long long sum;
	BI *p;
	
	if(A->m_nSign == B->m_nSign)
	{
		p = A;
		carry = 0;
		sum = 0;
		if(p->m_nLength < B->m_nLength)
			p->m_nLength = B->m_nLength;
		
		for(i=0;i<p->m_nLength;i++)
		{
			sum = B->m_ulValue[i];
			sum = sum + p->m_ulValue[i] + carry;
			p->m_ulValue[i] = (unsigned int)sum;
			carry = (sum > 0xffffffff ? 1 : 0);
		}
		if(p->m_nLength < BI_MAXLEN)
		{
			p->m_ulValue[p->m_nLength] = carry;
			p->m_nLength += carry;
		}
		//return p;
	}
	else{
		bi_print("Add: different sign\n");
		p = move(B);
		p->m_nSign = 1 - p->m_nSign;
		Sub(A, p);
		bi_osal_free(p);
		//return Sub(A, p);
	}
}

/*
 * A = A + b
 *
 */
void Add_p(BI *A, int b)
{
	int i;
	BI *p;
	unsigned long long sum;
	
	if((A->m_nSign*b)>=0)
	{
		//p = move(A);
		p = A;
		sum=b+p->m_ulValue[0];
		p->m_ulValue[0]=(unsigned int)sum;
		if(sum>0xffffffff)
		{
			i = 1;
			while(p->m_ulValue[i]==0xffffffff)
			{
				p->m_ulValue[i]=0;
				i++;
			}
			p->m_ulValue[i]++;
			if(i<BI_MAXLEN)
				p->m_nLength=i+1;
		}
		//return p;
	}
	else 
		Sub_p(A, -b);
		//return Sub_p(A, -b);
}

/*
 * A = A - B
 *
 */
void Sub(BI *A, BI *B)
{
	int i;
	int cmp;
	int len = 0;
	int carry;
	BI *p;
	unsigned long long num = 0;
	unsigned int* s = NULL;
	unsigned int* d = NULL;

	if(A->m_nSign == B->m_nSign)
	{
		//p = move(A);
		p = A;
		cmp = Cmp(p, B); 
		if(cmp == 0)
		{
			SetZero(p);
			//p = move_p(0);
			return;
		}
		
		if(cmp > 0)	// p > B
		{
			s = p->m_ulValue;
			d = B->m_ulValue;
			len = p->m_nLength;
		}
		if(cmp < 0)	// p < B
		{
			s = B->m_ulValue;
			d = p->m_ulValue;
			len = B->m_nLength;
			p->m_nSign = 1 - p->m_nSign;
		}
		
		carry = 0;
		for(i=0;i<len;i++)
		{
			if((s[i]-carry) >= d[i])
			{
				p->m_ulValue[i] = s[i] - carry - d[i];
				carry = 0;
			}
			else	// borrow from upper digit
			{
				num = ((unsigned long long)0x1 << 32) + s[i];
				p->m_ulValue[i] = (unsigned int)(num - carry - d[i]);
				carry = 1;
			}
		}
		
		while(p->m_ulValue[len-1] == 0)
			len--;

		p->m_nLength = len;
		//return p;
	}
	else {
		bi_print("Sub: different sign\n");
		p = move(B);
		p->m_nSign = 1 - p->m_nSign;
		Add(A, p);
		bi_osal_free(p);
		//return Add(A, p);
	}

}

/*
 * A = A - b
 *
 */
void Sub_p(BI *A, int b)
{
	int i;
	BI *p;
	unsigned long long num;

	if((A->m_nSign*b)>=0)
	{
		//p = move(A);
		p = A;
		if(p->m_ulValue[0]>=(unsigned int)b)
		{
			p->m_ulValue[0] -= b;
			return;
		}
		// A < b and A has only one block
		if(p->m_nLength==1)
		{
			p->m_ulValue[0] =b - p->m_ulValue[0];
			p->m_nSign = 1 - p->m_nSign;
			return;
		}
		// A < b
		num = ((unsigned long long)0x1 << 32) + p->m_ulValue[0];
		p->m_ulValue[0] = (unsigned int)(num-b);
		i = 1;
		while(p->m_ulValue[i]==0)
		{
			p->m_ulValue[i]=0xffffffff;
			i++;
		}
		
		if(p->m_ulValue[i]==1)
			p->m_nLength--;
		
		p->m_ulValue[i]--;
		//return p;
	}
	else
		Add_p(A, -b);
		//return Add_p(A, -b);
}

/*
 * return A * B
 *
 */
BI *Mul(BI *A, BI *B)
{
	int i, j, k;
	BI *p, *q;
	unsigned long long mul;
	unsigned int carry;

	p = init_BI();
	q = init_BI();

	for(i=0;i<B->m_nLength;i++)
	{
		q->m_nLength = A->m_nLength;
		carry = 0;
		for(j=0;j<A->m_nLength;j++)
		{
			mul = A->m_ulValue[j];
			mul = mul * B->m_ulValue[i] + carry;
			q->m_ulValue[j] = (unsigned int)mul;
			carry = (unsigned int)(mul>>32);
		}
		if(carry && (q->m_nLength<BI_MAXLEN))
		{
			q->m_nLength++;
			q->m_ulValue[q->m_nLength-1] = carry;
		}
		
		if(q->m_nLength < BI_MAXLEN - i)
		{
			q->m_nLength += i;
			for(k=q->m_nLength-1;k>=i;k--)
				q->m_ulValue[k]=q->m_ulValue[k-i];
			for(k=0;k<i;k++)
				q->m_ulValue[k]=0;
		}
		Add(p, q);		
	}

	p->m_nSign = ((A->m_nSign + B->m_nSign == 1) ? 0 : 1);

	bi_osal_free(q);
	
	return p;
}

/*
 * A = A * b
 *
 */
BI *Mul_p(BI *A, int b)
{
	int i;
	unsigned int carry;
	BI *p;
	unsigned long long mul;
	
	//p = move(A);
	p = A;
	carry = 0;
	for(i=0;i<p->m_nLength;i++)
	{
		mul = p->m_ulValue[i];
		mul = mul * b + carry;
		p->m_ulValue[i] = (unsigned int)mul;
		carry = (unsigned int)((mul-p->m_ulValue[i])>>32);
	}
	
	if(carry&&(p->m_nLength<BI_MAXLEN))
	{
		p->m_nLength++;
		p->m_ulValue[p->m_nLength-1] = carry;
	}
	
	if(b<0)
		p->m_nSign = 1 - p->m_nSign;

	return p;
}

/*
 * return  A / B
 *
 */
BI *Div(BI *A, BI *B)
{
	int i, len;
	BI *p, *q, *r, *tmp;
	unsigned int carry;
	unsigned long long num, div;

	p = init_BI();
	q = move(A);
	r = init_BI();
	carry = 0;
	
	while(Cmp(q, B)>0)
	{
		// compare leading digit
		if(q->m_ulValue[q->m_nLength-1] > B->m_ulValue[B->m_nLength-1])
		{
			len = q->m_nLength - B->m_nLength;
			div = q->m_ulValue[q->m_nLength-1] / (B->m_ulValue[B->m_nLength-1] + 1);
		}
		else if(q->m_nLength > B->m_nLength)
		{
			len = q->m_nLength - B->m_nLength - 1;
			num = q->m_ulValue[q->m_nLength-1];
			num = (num<<32) + q->m_ulValue[q->m_nLength-2];
			if (B->m_ulValue[B->m_nLength-1]==0xffffffff)
                div = (num>>32);
			else 
				div = do_div(num, (B->m_ulValue[B->m_nLength-1] + 1));
		}
		else
		{
			Add_p(p, 1);
			break;
		}
		
		r = move_p(div);
		r->m_nLength += len;
		for(i=r->m_nLength-1;i>=len;i--)
			r->m_ulValue[i] = r->m_ulValue[i-len];
		for(i=0;i<len;i++)
			r->m_ulValue[i]=0;
		
		Add(p, r);
		tmp = Mul(r, B);
		bi_osal_free(r);
		r = tmp;
		Sub(q, r);
	}
	
	if(Cmp(q, B)==0)	// no remainder
		Add_p(p, 1);

	p->m_nSign = ((A->m_nSign+B->m_nSign==1) ? 0 : 1);
	bi_osal_free(q);
	bi_osal_free(r);
	return p;
}

/*
 * A = A / b
 *
 */
BI *Div_p(BI *A, int b)
{
	int i;
	unsigned int carry;
	BI *p;
	unsigned long long div, mul;
	
	//p = move(A);
	p = A;
	if(p->m_nLength==1)
	{
		p->m_ulValue[0] = p-> m_ulValue[0] / b;
		return p;
	}

	carry = 0;
	for(i=p->m_nLength-1;i>=0;i--)
	{
		div = carry;
		div = (div<<32) + p->m_ulValue[i];
		p->m_ulValue[i] = (unsigned int) do_div(div, b);
		mul = (do_div(div, b)) * b;
		carry = (unsigned int)(div - mul);
	}
	
	if(p->m_ulValue[p->m_nLength-1] == 0)
		p->m_nLength--;
	if(b < 0)
		p->m_nSign = 1 - p->m_nSign;
	return p;
}

/*
 * return A % B
 *
 */
BI *Mod(BI *A, BI *B)
{
	int i, len;
	BI* p = move(A);
	BI* q = NULL;
	BI* tmp = NULL;
	unsigned long long num, div;
			
	while(Cmp(p, B)>0)
	{		
		// compare leading digit
		if(p->m_ulValue[p->m_nLength-1] > B->m_ulValue[B->m_nLength-1])
		{
			len = p->m_nLength - B->m_nLength;
			div = p->m_ulValue[p->m_nLength-1] / (B->m_ulValue[B->m_nLength-1] + 1);	
		}
		else if(p->m_nLength > B->m_nLength)
		{
			len = p->m_nLength - B->m_nLength - 1;
			num = p->m_ulValue[p->m_nLength-1];			
			num = (num<<32) + p->m_ulValue[p->m_nLength-2];
			
			if(B->m_ulValue[B->m_nLength-1]==0xffffffff)
				div = (num>>32);
			else
            {			    
				div = do_div(num, (B->m_ulValue[B->m_nLength-1] + 1));
            }
		}
		else
		{			
			Sub(p, B);			
			break;
		}
				
		//------------------------------------
		tmp = move_p(div);
		q = Mul(tmp, B);		
		bi_osal_free(tmp);		
		
		q->m_nLength += len;
				
		for(i=q->m_nLength-1;i>=len;i--)
			q->m_ulValue[i]=q->m_ulValue[i-len];			
		
		for(i=0;i<len;i++)
			q->m_ulValue[i]=0;
				
		Sub(p, q);
		
        bi_osal_free(q);        
        //------------------------------------
	}
	
	if(Cmp(p, B)==0)		
		SetZero(p);
	    
	return p;
}

/*
 * A = A % b
 *
 */
unsigned int Mod_p(BI *A, int b)
{
	int i;
	unsigned int carry;
	unsigned long long div;
	unsigned long long temp;
	
	if(A->m_nLength==1)
		return(A->m_ulValue[0]%b);

	carry = 0;
	for(i=A->m_nLength-1;i>=0;i--)
	{
		div = carry * ((unsigned long long)0x1 << 32) + A->m_ulValue[i];
		temp = do_div(div, b);
		carry = (unsigned int)(div - (temp * b));
	}
	return carry;
}

BI *Exp_Mod(BI *base, BI *exp, BI *mod)
{
	BI* x = move_p(1);
	BI* y = move(base);
	BI* z = move(exp);
	BI* tmp = NULL;
	
	
	while ((z->m_nLength != 1) || (z->m_ulValue[0])) {
		//bi_print("=> ");
		if (z->m_ulValue[0] & 1) {
			//bi_print("1, ");
			Sub_p(z, 1);
			//bi_print("3, ");
			tmp = Mul(x, y);
			bi_osal_free(x);
			x = tmp;
			//bi_print("5\n");
			tmp = Mod(x, mod);
			bi_osal_free(x);
			x = tmp;
		}
		else {
			//bi_print("2, ");
			z = Div_p(z, 2);
			//bi_print("4, ");
			tmp = Mul(y, y);
			bi_osal_free(y);
			y = tmp;
			//bi_print("6\n");
			tmp = Mod(y, mod);
			bi_osal_free(y);
			y = tmp;
		}
	}
	
	bi_osal_free(y);
	bi_osal_free(z);
	return x;
}

BI *InPutFromStr(char *str, const unsigned int system)
{
	int i;
	int c = 0;
	int len;
	BI *p;
	
	p = init_BI();
	if (p == NULL)
		return NULL;
	
	len = strlen(str);
	for (i=0; i<len; i++) {
		p = Mul_p(p, system);
		
		if (system == DEC)
			c=str[i]-48;
		else if (system == HEX) {
			if (str[i] >= 97)		// 'a' ~ 'f'
				c = (str[i] - 97) + 10;
			else if (str[i] >= 65)		// 'A' ~ 'F'
				c = (str[i] - 65) + 10;
			else				// '0' ~ '9'
				c = str[i] - 48;
		}
		Add_p(p, c);
	}
	
	return p;
}

BI *InPutFromAddr(unsigned char *addr, int len)
{
#if 0    
	int i, space, count;
	BI *p;
	
	p = init_BI();
	if (p == NULL)
		return NULL;

	count = len / sizeof(unsigned int);
	space = len % sizeof(unsigned int);
	if (space)
		count++;

	if (count > BI_MAXLEN)
		return NULL;		// overflow

	p->m_nLength = count;
/*
	i = count - 1;
	if (space) {
		// highest element may be not full
		p->m_ulValue[i] = (*(unsigned int *)addr) >> (sizeof(unsigned int) - space) * 8;
		addr += space;
		i--;
	}

	while (i >= 0) {
		//p->m_ulValue[i] = *(unsigned int *)addr;
		//addr += 4;
		p->m_ulValue[i] = *addr << 24;
		addr++;
		p->m_ulValue[i] |= *addr << 16;
		addr++;
		p->m_ulValue[i] |= *addr << 8;
		addr++;
		p->m_ulValue[i] |= *addr;
		addr++;
		i--;
	}
*/	
	i = 0;
	if (space) {
		// highest element may be not full
		p->m_ulValue[i] = (*(unsigned int *)addr) >> (sizeof(unsigned int) - space) * 8;
		addr += space;
		i--;
	}

	while (i < count) {
		p->m_ulValue[i] = *(unsigned int *)addr;
		addr += 4;
		//p->m_ulValue[i] = *addr << 24;
		//addr++;
		//p->m_ulValue[i] |= *addr << 16;
		//addr++;
		//p->m_ulValue[i] |= *addr << 8;
		//addr++;
		//p->m_ulValue[i] |= *addr;
		//addr++;
		i++;
	}

	return p;
#else
    char str[512];    
    int i;
    int j=0;
    
    if (len > 128)
        return NULL;
        
    for (i=0; i <len; i++)
    {
        str[j++] = '0' + (addr[i] >> 4);
        str[j++] = '0' + (addr[i] & 0xF);
    }
    
    str[j] = '\0';
    
    return InPutFromStr(str, HEX);
          
#endif	
}

char *OutPutToStr(BI *A, const unsigned int system)
{
	unsigned int i, j;
	unsigned int str_size;
	unsigned int raw_len;
	BI *p;
	char ch;
	char *str, *out;
	
	if (isZero(A))
		return "0";
	
	// an unsigned int variable approximately equals to 10 decimal digit
	str_size = 10 * A->m_nLength + 1;
	str = (char *)bi_osal_malloc(str_size);

	if (str == NULL)
		return NULL;

	memset(str, 0, str_size);
	p = move(A);
	i = 0;
	while(p->m_ulValue[p->m_nLength-1] > 0)
	{
		ch = Mod_p(p, system);	// to ascii
		
		if(system == DEC)
			ch += 48;
		else if (system == HEX)
			ch += (ch < 10 ? 48 : 55);
			
		str[i++] = ch;
		//bi_print("%d ", ch);
		p = Div_p(p, system);
	}
	bi_osal_free(p);
	raw_len = i + 1;
	//bi_print("\n");
	
	if (i <= 1)	// only one digit
		return str;

	// reverse string order
	i--;
	j = 0;
	while (i > j) {
		ch = str[i];
		str[i] = str[j];
		str[j] = ch;
		i--;
		j++;
	}

	// strip padding (format: 1FF...F00)
	i = j = 0;
	if (*str == '1') {
		i++;
		j++;
		while (*(str+i) == 'F') {
			if (j == 1)	j++;
			i++;
		}
		
		if ((*(str+i) == '0') && (*(str+i+1) == '0')) {
			j++;
			i+=2;
		}
	}
	
	//bi_print("%d padding characters, raw_len:%d\n", i, raw_len);
	if (j == 3) {
		// make sure that it have walked through all padding characters
		out = (char *)bi_osal_malloc(raw_len - i);
		memset(out, 0, raw_len - i);
		strcpy(out, str+i);
		bi_osal_free(str);
	}
	else
		out = str;
	
	return out;
}

void OutPutToAddr(BI *A, unsigned char *addr, char reverse)
{
	int i;

	if (reverse) {
		for (i = A->m_nLength - 1; i >= 0; i--) {
			*addr = (A->m_ulValue[i] >> 24) & 0xff;
			addr++;
			*addr = (A->m_ulValue[i] >> 16) & 0xff;
			addr++;
			*addr = (A->m_ulValue[i] >> 8) & 0xff;
			addr++;
			*addr = (A->m_ulValue[i] >> 0) & 0xff;
			addr++;
		}
	}
	else {
		for (i = 0; i < A->m_nLength; i++, addr+=4)
			*(unsigned int *)addr = A->m_ulValue[i];
	}
}


void dump_bi(BI *A)
{
	int i;

	if (A == NULL) {
		bi_print("null");
		return;
	}
	
	bi_print("%s%x [", (A->m_nSign == 1 ? "+" : "-"), A->m_nLength);
	    
	for (i=A->m_nLength-1; i>=0; i--)
		bi_print("%8X, ", A->m_ulValue[i]);
	
	bi_print("]\n");
}

