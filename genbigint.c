#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER // Visual Studio
#pragma warning(disable : 4996) // _CRT_SECURE_NO_WARNINGS
#endif


void usage()
{
	printf("Usage: genbigint <bits>\n"
		"  <bits> is a multiple of 64 (128, 192, 256, ...)\n");
}

void error(const char* msg1, const char* msg2)
{
	printf("Error: %s %s\n", msg1, msg2);
	exit(1);
}

void generate_asm_add(FILE* f, int bits)
{
	fprintf(f, "add%d:\n", bits);
	fprintf(f, "    mov rax, [rcx]\n");
	fprintf(f, "    add rax, [rdx]\n");
	fprintf(f, "    mov [r8], rax\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov rax, [rcx + %d]\n", i);
		fprintf(f, "    adc rax, [rdx + %d]\n", i);
		fprintf(f, "    mov [r8 + %d], rax\n\n", i);
	}

	fprintf(f, "    ret\n\n");
}

void generate_asm_sub(FILE* f, int bits)
{
	fprintf(f, "sub%d:\n", bits);
	fprintf(f, "    mov rax, [rcx]\n");
	fprintf(f, "    sub rax, [rdx]\n");
	fprintf(f, "    mov [r8], rax\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov rax, [rcx + %d]\n", i);
		fprintf(f, "    sbb rax, [rdx + %d]\n", i);
		fprintf(f, "    mov [r8 + %d], rax\n\n", i);
	}

	fprintf(f, "    ret\n\n");
}

void generate_asm_mul(FILE* f, int bits)
{
	fprintf(f, "mul%d_1:\n", bits);
	fprintf(f, "    push rbx\n\n");
	fprintf(f, "    mov rax, [rcx]\n");
	fprintf(f, "    mov rbx, rdx\n");
	fprintf(f, "    mul rbx\n");
	fprintf(f, "    mov [r8], rax\n");
	fprintf(f, "    mov [r8 + 8], rdx\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov rax, [rcx + %d]\n", i);
		fprintf(f, "    mul rbx\n");
		fprintf(f, "    add [r8 + %d], rax\n", i);
		fprintf(f, "    adc rdx, 0\n");
		if (i < bits / 8 - 8)
			fprintf(f, "    mov [r8 + %d], rdx\n\n", i + 8);
		else
			fprintf(f, "    mov rax, rdx\n\n");
	}

	fprintf(f, "    pop rbx\n");
	fprintf(f, "    ret\n\n");
}

void generate_asm_div(FILE* f, int bits)
{
	fprintf(f, "div%d_1:\n", bits);
	fprintf(f, "    push rbx\n\n");
	fprintf(f, "    mov rax, [rcx + %d]\n", bits / 8 - 8);
	fprintf(f, "    mov rbx, rdx\n");
	fprintf(f, "    xor rdx, rdx\n\n");

	fprintf(f, "    test    rax, rax\n");
	fprintf(f, "    js      signed_a\n\n");

	fprintf(f, "; unsigned a\n");
	fprintf(f, "    div    rbx\n");
	fprintf(f, "    mov    [r8 + %d], rax\n\n", bits / 8 - 8);

	for (int i = bits / 8 - 16; i > 0; i -= 8)
	{
		fprintf(f, "    mov    rax, [rcx + %d]\n", i);
		fprintf(f, "    div    rbx\n");
		fprintf(f, "    mov    [r8 + %d], rax\n\n", i);
	}

	fprintf(f, "    mov    rax, [rcx]\n");
	fprintf(f, "    div    rbx\n");
	fprintf(f, "    mov    [r8], rax\n\n");

	fprintf(f, "    mov    rax, rdx\n\n");

	fprintf(f, "    pop rbx\n");
	fprintf(f, "    ret\n\n");

	fprintf(f, "signed_a:\n");
	fprintf(f, "    neg    qword [rcx]\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    neg    qword [rcx + %d]\n", i);
	fprintf(f, "\n");

	fprintf(f, "    add    qword [rcx], 1\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    adc    qword [rcx + %d], 0\n", i);
	fprintf(f, "\n");

	for (int i = bits / 8 - 8; i > 0; i -= 8)
	{
		fprintf(f, "    mov    rax, [rcx + %d]\n", i);
		fprintf(f, "    div    rbx\n");
		fprintf(f, "    mov    [r8 + %d], rax\n\n", i);
	}

	fprintf(f, "    mov    rax, [rcx]\n");
	fprintf(f, "    div    rbx\n");
	fprintf(f, "    mov    [r8], rax\n\n");

	fprintf(f, "    neg    qword [rcx]\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    neg    qword [rcx + %d]\n", i);
	fprintf(f, "\n");

	fprintf(f, "    add    qword [rcx], 1\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    adc    qword [rcx + %d], 0\n", i);
	fprintf(f, "\n");

	fprintf(f, "    neg    qword [r8]\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    neg    qword [r8 + %d]\n", i);
	fprintf(f, "\n");

	fprintf(f, "    add    qword [r8], 1\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    adc    qword [r8 + %d], 0\n", i);
	fprintf(f, "\n");

	fprintf(f, "    mov    rax, rdx\n\n");

	fprintf(f, "    pop rbx\n");
	fprintf(f, "    ret\n\n");
}

void generate_asm_neg(FILE* f, int bits)
{
	fprintf(f, "neg%d:\n", bits);
	fprintf(f, "    not qword [rcx]\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    not qword [rcx + %d]\n", i);

	fprintf(f, "    add qword [rcx], 1\n");
	for (int i = 8; i < bits / 8; i += 8)
		fprintf(f, "    adc qword [rcx + %d], 0\n", i);

	fprintf(f, "    ret\n\n");
}

void generate_asm_shl(FILE* f, int bits)
{
	fprintf(f, "shl%d:\n", bits);
	fprintf(f, "    mov    r8, rcx\n");
	fprintf(f, "    mov    cl, dl\n\n");

	for (int i = bits / 8 - 8; i >= 8; i -= 8)
	{
		if (i - 8 != 0)
			fprintf(f, "    mov    rax, [r8 + %d]\n", i - 8);
		else
			fprintf(f, "    mov    rax, [r8]\n");
		fprintf(f, "    shld   [r8 + %d], rax, cl\n\n", i);
	}

	fprintf(f, "    shl    qword [r8], cl\n\n");

	fprintf(f, "    ret\n\n");
}

void generate_asm_shr(FILE* f, int bits)
{
	fprintf(f, "shr%d:\n", bits);
	fprintf(f, "    mov    r8, rcx\n");
	fprintf(f, "    mov    cl, dl\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov    rax, [r8 + %d]\n", i);
		if (i == 8)
			fprintf(f, "    shrd   [r8], rax, cl\n\n");
		else
			fprintf(f, "    shrd   [r8 + %d], rax, cl\n\n", i - 8);
	}

	fprintf(f, "    shr    qword [r8+%d], cl\n\n", bits / 8 - 8);

	fprintf(f, "    ret\n\n");
}

void generate_asm_sar(FILE* f, int bits)
{
	fprintf(f, "sar%d:\n", bits);
	fprintf(f, "    mov    r8, rcx\n");
	fprintf(f, "    mov    cl, dl\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov    rax, [r8 + %d]\n", i);
		if (i == 8)
			fprintf(f, "    shrd   [r8], rax, cl\n\n");
		else
			fprintf(f, "    shrd   [r8 + %d], rax, cl\n\n", i - 8);
	}

	fprintf(f, "    sar    qword [r8+%d], cl\n\n", bits / 8 - 8);

	fprintf(f, "    ret\n\n");
}

void generate_asm_and(FILE* f, int bits)
{
	fprintf(f, "and%d:\n", bits);
	fprintf(f, "    mov rax, [rcx]\n");
	fprintf(f, "    and rax, [rdx]\n");
	fprintf(f, "    mov [r8], rax\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov rax, [rcx + %d]\n", i);
		fprintf(f, "    and rax, [rdx + %d]\n", i);
		fprintf(f, "    mov [r8 + %d], rax\n\n", i);
	}

	fprintf(f, "    ret\n\n");
}

void generate_asm_or(FILE* f, int bits)
{
	fprintf(f, "or%d:\n", bits);
	fprintf(f, "    mov rax, [rcx]\n");
	fprintf(f, "    or rax, [rdx]\n");
	fprintf(f, "    mov [r8], rax\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov rax, [rcx + %d]\n", i);
		fprintf(f, "    or rax, [rdx + %d]\n", i);
		fprintf(f, "    mov [r8 + %d], rax\n\n", i);
	}

	fprintf(f, "    ret\n\n");
}

void generate_asm_xor(FILE* f, int bits)
{
	fprintf(f, "xor%d:\n", bits);
	fprintf(f, "    mov rax, [rcx]\n");
	fprintf(f, "    xor rax, [rdx]\n");
	fprintf(f, "    mov [r8], rax\n\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    mov rax, [rcx + %d]\n", i);
		fprintf(f, "    xor rax, [rdx + %d]\n", i);
		fprintf(f, "    mov [r8 + %d], rax\n\n", i);
	}

	fprintf(f, "    ret\n\n");
}

void generate_asm_not(FILE* f, int bits)
{
	fprintf(f, "not%d:\n", bits);
	fprintf(f, "    not qword [rcx]\n");

	for (int i = 8; i < bits / 8; i += 8)
	{
		fprintf(f, "    not qword [rcx + %d]\n", i);
	}

	fprintf(f, "    ret\n\n");
}

void generate_asm(int bits)
{
	char s[128];
	sprintf(s, "int%d.asm", bits);

	FILE* f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);

	fprintf(f, "section .text\n\n");

	fprintf(f, "extern __cdecl\n\n");

	fprintf(f, "global add%d\n", bits);
	fprintf(f, "global sub%d\n", bits);
	fprintf(f, "global mul%d_1\n", bits);
	fprintf(f, "global div%d_1\n", bits);
	fprintf(f, "global neg%d\n", bits);
	fprintf(f, "global shl%d\n", bits);
	fprintf(f, "global shr%d\n", bits);
	fprintf(f, "global sar%d\n", bits);
	fprintf(f, "global and%d\n", bits);
	fprintf(f, "global or%d\n", bits);
	fprintf(f, "global xor%d\n", bits);
	fprintf(f, "global not%d\n", bits);

	generate_asm_add(f, bits);
	generate_asm_sub(f, bits);
	generate_asm_mul(f, bits);
	generate_asm_div(f, bits);
	generate_asm_neg(f, bits);
	generate_asm_shl(f, bits);
	generate_asm_shr(f, bits);
	generate_asm_sar(f, bits);
	generate_asm_and(f, bits);
	generate_asm_or(f, bits);
	generate_asm_xor(f, bits);
	generate_asm_not(f, bits);

	fclose(f);

}

void generate_c_h(int bits)
{
	char s[128];
	sprintf(s, "int%d.h", bits);

	FILE* f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);

	fprintf(f, "#ifndef INT%d_H\n", bits);
	fprintf(f, "#define INT%d_H\n\n", bits);

	fprintf(f, "#define INT%d_DIGITS %d\n\n", bits, bits / 64);

	fprintf(f, "typedef long long INT%d[INT%d_DIGITS];\n\n", bits, bits);

	fprintf(f, "static const INT%d INT%d_MIN = { ", bits, bits);
	for (int i = 1; i < bits / 64; i++)
	{
		fprintf(f, "0, ");
	}
	fprintf(f, "0x8000000000000000};\n");

	fprintf(f, "static const INT%d INT%d_MAX = { ", bits, bits);
	for (int i = 1; i < bits / 64; i++)
	{
		fprintf(f, "0xFFFFFFFFFFFFFFFF, ");
	}
	fprintf(f, " 0x7FFFFFFFFFFFFFFF };\n\n");



	fprintf(f, "/* a = i */\n");
	fprintf(f, "void assign%d(INT%d a, long long i);\n\n", bits, bits);

	fprintf(f, "/* b = a */\n");
	fprintf(f, "void copy%d(const INT%d a, INT%d b);\n\n", bits, bits, bits);

	fprintf(f, "/* return -1 if a < b, 0 if a == b, 1 if a > b */\n");
	fprintf(f, "int compare%d(const INT%d a, const INT%d b);\n\n", bits, bits, bits);

	fprintf(f, "/* return a pointer to the base 10 representation */\n");
	fprintf(f, "char* to_decimal_string%d(const INT%d a, char *buf, int size);\n\n", bits, bits);

	fprintf(f, "/* c = a + b */\n");
	fprintf(f, "void add%d(INT%d a, INT%d b, INT%d c);\n\n", bits, bits, bits, bits);

	fprintf(f, "/* c = a - b */\n");
	fprintf(f, "void sub%d(INT%d a, INT%d b, INT%d c);\n\n", bits, bits, bits, bits);

	fprintf(f, "/* c = a * i */\n");
	fprintf(f, "void mul%d_1(INT%d a, long long i, INT%d c);\n\n", bits, bits, bits);

	fprintf(f, "/* c = a * b */\n");
	fprintf(f, "void mul%d(INT%d a, INT%d b, INT%d c);\n\n", bits, bits, bits, bits);

	fprintf(f, "/* c = a / i (r = remainder) */\n");
	fprintf(f, "long long div%d_1(INT%d a, long long i, INT%d c);\n\n", bits, bits, bits);

	fprintf(f, "/* c = a / b (r = remainder) */\n");
	fprintf(f, "void div%d(INT%d a, INT%d b, INT%d c, INT%d r);\n\n", bits, bits, bits, bits, bits);

	fprintf(f, "/* a = -a */\n");
	fprintf(f, "void neg%d(INT%d a);\n\n", bits, bits);

	fprintf(f, "/* a = a << n */\n");
	fprintf(f, "void shl%d(INT%d a, int n);\n\n", bits, bits);

	fprintf(f, "/* a = a >> n */\n");
	fprintf(f, "void shr%d(INT%d a, int n);\n\n", bits, bits);

	fprintf(f, "/* a = a >> n (arithmetic) */\n");
	fprintf(f, "void sar%d(INT%d a, int n);\n\n", bits, bits);

	fprintf(f, "/* c = a & b */\n");
	fprintf(f, "void and%d(INT%d a, INT%d b, INT%d c);\n\n", bits, bits, bits, bits);

	fprintf(f, "/* c = a | b */\n");
	fprintf(f, "void or%d(INT%d a, INT%d b, INT%d c);\n\n", bits, bits, bits, bits);

	fprintf(f, "/* c = a ^ b */\n");
	fprintf(f, "void xor%d(INT%d a, INT%d b, INT%d c);\n\n", bits, bits, bits, bits);

	fprintf(f, "/* b = ~a */\n");
	fprintf(f, "void not%d(INT%d a, INT%d b);\n", bits, bits, bits);

	fprintf(f, "void parse%d(const char *s, INT%d a);\n\n", bits, bits);

	fprintf(f, "#endif\n");

	fclose(f);
}

void generate_c(int bits)
{
	char s[128];
	FILE* f = NULL;
	int i, j, k;

	generate_c_h(bits);

	sprintf(s, "int%d.c", bits);
	f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);

	fprintf(f, "#include \"int%d.h\"\n\n", bits);

	fprintf(f, "void assign%d(INT%d a, long long i)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    a[0] = i;\n");
	for (i = 1; i < bits / 64; i++)
		fprintf(f, "    a[%d] = i < 0 ? -1 : 0;\n", i);
	fprintf(f, "}\n\n");

	fprintf(f, "void copy%d(const INT%d a, INT%d b)\n", bits, bits, bits);
	fprintf(f, "{\n");
	for (i = 0; i < bits / 64; i++)
		fprintf(f, "    b[%d] = a[%d];\n", i, i);
	fprintf(f, "}\n\n");

	fprintf(f, "int compare%d(const INT%d a, const INT%d b)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    long long sa = a[%d] & 0x8000000000000000;\n", bits / 64 - 1);
	fprintf(f, "    long long sb = b[%d] & 0x8000000000000000;\n\n", bits / 64 - 1);

	fprintf(f, "    if (sa == sb)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        /* same sign */\n");
	for (i = bits / 64 - 1; i >= 0; i--)
	{
		fprintf(f, "        if ((unsigned long long)a[%d] < (unsigned long long)b[%d]) return -1;\n", i, i);
		fprintf(f, "        if ((unsigned long long)a[%d] > (unsigned long long)b[%d]) return 1;\n", i, i);
	}

	fprintf(f, "        return 0;\n");

	fprintf(f, "    }\n\n");

	fprintf(f, "    if (sa == 0)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        /* positive a, negative b */\n");
	fprintf(f, "        return 1;\n");
	fprintf(f, "    }\n\n");

	fprintf(f, "    /* negative a, positive b */\n");
	fprintf(f, "    return -1;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "char* to_decimal_string%d(const INT%d a, char *buf, int size)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int i = size-1;\n");
	fprintf(f, "    int is_negative = (int)(a[%d] & 0x8000000000000000);\n", bits / 64 - 1);
	fprintf(f, "    INT%d b;\n", bits);
	fprintf(f, "    INT%d zero;\n", bits);
	fprintf(f, "    INT%d c;\n", bits);
	fprintf(f, "    long long r;\n\n");

	fprintf(f, "    copy%d(a, b);\n", bits);
	fprintf(f, "    assign%d(zero, 0);\n\n", bits);

	fprintf(f, "    if (is_negative)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        neg%d(b);\n", bits);
	fprintf(f, "    }\n\n");

	fprintf(f, "    buf[i--] = 0;\n");
	fprintf(f, "    while (compare%d(b, zero) != 0)\n", bits);
	fprintf(f, "    {\n");
	fprintf(f, "        r = div%d_1(b, 10, c);\n", bits);
	fprintf(f, "        buf[i--] = (char)r + '0';\n");
	fprintf(f, "        copy%d(c, b);\n", bits);
	fprintf(f, "    }\n\n");

	fprintf(f, "    if (is_negative)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        buf[i--] = '-';\n");
	fprintf(f, "    }\n\n");

	fprintf(f, "    return buf + i + 1;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "void mul%d(INT%d a, INT%d b, INT%d c)\n", bits, bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    INT%d aa, bb, x, y;\n", bits);
	fprintf(f, "    long long sa, sb;\n\n");

	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    sa = aa[%d] & 0x8000000000000000;\n", bits / 64 - 1);
	fprintf(f, "    sb = bb[%d] & 0x8000000000000000;\n", bits / 64 - 1);
	fprintf(f, "    if (sa) neg%d(aa);\n", bits);
	fprintf(f, "    if (sb) neg%d(bb);\n\n", bits);

	fprintf(f, "    mul%d_1(aa, bb[0], x);\n\n", bits);

	for (i = 1, k = 0; i < bits / 64; i++, k++)
	{
		fprintf(f, "    mul%d_1(aa, bb[%d], y);\n", bits, i);
		for (j = bits / 64 - 1; j >= i; j--)
		{
			fprintf(f, "    y[%d] = y[%d];\n", j, j - 1 - k);
		}
		for (; j >= 0; j--)
		{
			fprintf(f, "    y[%d] = 0;\n", j);
		}
		fprintf(f, "    add%d(x, y, c);\n", bits);
		if (i < bits / 64 - 1) fprintf(f, "    copy%d(c, x);\n\n", bits);
		else fprintf(f, "\n");
	}

	fprintf(f, "    if (sa != sb) neg%d(c);\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "void div%d_first_digit(INT%d a, INT%d b, INT%d c, INT%d r)\n", bits, bits, bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    INT%d bb;\n", bits);
	fprintf(f, "    long long x = 1;\n\n");

	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    assign%d(c, 1);\n\n", bits);

	fprintf(f, "    /* first, shift 64 bits at a time */\n");
	fprintf(f, "    while (bb[%d] == 0 && compare%d(a, bb) > 0)\n", bits / 64 - 1, bits);
	fprintf(f, "    {\n");
	for (i = bits / 64 - 1; i > 0; i--)
	{
		fprintf(f, "        bb[%d] = bb[%d];\n", i, i - 1);
		fprintf(f, "        c[%d] = c[%d];\n", i, i - 1);
	}
	fprintf(f, "        bb[0] = 0;\n");
	fprintf(f, "        c[0] = 0;\n");
	fprintf(f, "    }\n");

	fprintf(f, "    if (compare%d(a, bb) < 0)\n", bits);
	fprintf(f, "    {\n");
	for (i = 0; i < bits / 64 - 1; i++)
	{
		fprintf(f, "        bb[%d] = bb[%d];\n", i, i + 1);
		fprintf(f, "        c[%d] = c[%d];\n", i, i + 1);
	}
	fprintf(f, "        bb[%d] = 0;\n", bits / 64 - 1);
	fprintf(f, "        c[%d] = 0;\n", bits / 64 - 1);
	fprintf(f, "    }\n\n");

	fprintf(f, "    /* then, shift bit by bit */\n");
	fprintf(f, "    while ( ((bb[%d] & 0x4000000000000000) == 0) && (compare%d(a, bb) > 0) )\n", bits / 64 - 1, bits);
	fprintf(f, "    {\n");
	fprintf(f, "        shl%d(bb, 1);\n", bits);
	fprintf(f, "        shl%d(c, 1);\n", bits);
	fprintf(f, "    }\n");

	fprintf(f, "	if (compare%d(a, bb) < 0)\n", bits);
	fprintf(f, "    {\n");
	fprintf(f, "        shr%d(bb, 1);\n", bits);
	fprintf(f, "        shr%d(c, 1);\n", bits);
	fprintf(f, "    }\n\n");

	fprintf(f, "    /* subtract */\n");
	fprintf(f, "    sub%d(a, bb, r);\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "void div%d(INT%d a, INT%d b, INT%d c, INT%d d)\n", bits, bits, bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    long long sa = a[%d] & 0x8000000000000000;\n", bits / 64 - 1);
	fprintf(f, "    long long sb = b[%d] & 0x8000000000000000;\n", bits / 64 - 1);
	fprintf(f, "    long long sd;\n");
	fprintf(f, "    long long x;\n");
	fprintf(f, "    INT%d cc, dd, ccc;\n\n", bits);

	for (i = bits / 64 - 1; i > 0; i--)
		fprintf(f, "    c[%d] = 0;\n", i);

	fprintf(f, "    if (sa) neg%d(a);\n", bits);
	fprintf(f, "    if (sb) neg%d(b);\n\n", bits);

	fprintf(f, "    x = compare%d(a, b);\n\n", bits);

	fprintf(f, "    if (x < 0)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        c[0] = 0;\n");
	fprintf(f, "        copy%d(a, d);\n", bits);
	fprintf(f, "        if (sa != sb) neg%d(d);\n", bits);
	fprintf(f, "        if (sa) neg%d(a);\n", bits);
	fprintf(f, "        if (sb) neg%d(b);\n", bits);
	fprintf(f, "        return;\n");
	fprintf(f, "    }\n\n");

	fprintf(f, "    c[0] = 1;\n");

	fprintf(f, "    if (x == 0)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        if (sa != sb) neg%d(c);\n", bits);
	fprintf(f, "        if (sa) neg%d(a);\n", bits);
	fprintf(f, "        if (sb) neg%d(b);\n", bits);
	fprintf(f, "        assign%d(d, 0);\n", bits);
	fprintf(f, "        return;\n");
	fprintf(f, "    }\n\n");

	fprintf(f, "    div%d_first_digit(a, b, c, d);\n", bits);
	fprintf(f, "	if (compare%d(d, b) < 0)\n", bits);
	fprintf(f, "    {\n");
	fprintf(f, "        if (sa) neg%d(a);\n", bits);
	fprintf(f, "        if (sb) neg%d(b);\n", bits);
	fprintf(f, "        if (sa != sb)\n");
	fprintf(f, "        {\n");
	fprintf(f, "            neg%d(c);\n", bits);
	fprintf(f, "        }\n");
	fprintf(f, "        sd = d[%d] & 0x8000000000000000;\n", bits / 64 - 1);
	fprintf(f, "        if (sa != sd)\n");
	fprintf(f, "        {\n");
	fprintf(f, "            neg%d(d);\n", bits);
	fprintf(f, "        }\n");
	fprintf(f, "        return;\n");
	fprintf(f, "    }\n\n");

	fprintf(f, "    while (1)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        div%d_first_digit(d, b, cc, dd);\n", bits);
	fprintf(f, "        add%d(c, cc, ccc);\n", bits);
	fprintf(f, "        copy%d(ccc, c);\n", bits);
	fprintf(f, "        copy%d(dd, d);\n", bits);
	fprintf(f, "        if (compare%d(d, b) < 0)\n", bits);
	fprintf(f, "        {\n");
	fprintf(f, "            if (sa) neg%d(a);\n", bits);
	fprintf(f, "            if (sb) neg%d(b);\n", bits);
	fprintf(f, "            if (sa != sb)\n");
	fprintf(f, "            {\n");
	fprintf(f, "                neg%d(c);\n", bits);
	fprintf(f, "            }\n");
	fprintf(f, "            sd = d[%d] & 0x8000000000000000;\n", bits / 64 - 1);
	fprintf(f, "            if (sa != sd)\n");
	fprintf(f, "            {\n");
	fprintf(f, "                neg%d(d);\n", bits);
	fprintf(f, "            }\n");
	fprintf(f, "            return;\n");
	fprintf(f, "        }\n");
	fprintf(f, "    }\n");
	fprintf(f, "}\n\n");

	fprintf(f, "void parse%d(const char *s, INT%d a)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int i = 0;\n");
	fprintf(f, "    int is_negative = 0;\n");
	fprintf(f, "    INT%d b, c;\n", bits);
	fprintf(f, "    long long x;\n\n");

	fprintf(f, "    assign%d(a, 0);\n", bits);
	fprintf(f, "    assign%d(c, 0);\n\n", bits);

	fprintf(f, "    if (s[i] == '-')\n");
	fprintf(f, "    {\n");
	fprintf(f, "        is_negative = 1;\n");
	fprintf(f, "        i++;\n");
	fprintf(f, "    }\n\n");

	fprintf(f, "    while (s[i] >= '0' && s[i] <= '9')\n");
	fprintf(f, "    {\n");
	fprintf(f, "        x = s[i] - '0';\n");
	fprintf(f, "        assign%d(b, x);\n", bits);
	fprintf(f, "        add%d(c, b, a);\n", bits);
	fprintf(f, "        mul%d_1(a, 10, c);\n", bits);
	fprintf(f, "        i++;\n");
	fprintf(f, "    }\n\n");

	fprintf(f, "    if (is_negative)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        neg%d(a);\n", bits);
	fprintf(f, "    }\n");
	fprintf(f, "}\n\n");

	fclose(f);
}

void generate_cpp(int bits)
{
	char s[128];
	FILE* f = NULL;

	sprintf(s, "int%d.hpp", bits);
	f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);

	fprintf(f, "#pragma once\n\n");

	fprintf(f, "#include <iostream>\n\n");

	fprintf(f, "class int%d\n", bits);
	fprintf(f, "{\n");
	fprintf(f, "public:\n");
	fprintf(f, "    static const int%d min;\n", bits);
	fprintf(f, "    static const int%d max;\n", bits);
	fprintf(f, "    static const int%d zero;\n", bits);
	fprintf(f, "    static const int%d one;\n", bits);
	fprintf(f, "    static const int%d minus_one;\n\n", bits);

	fprintf(f, "    int%d(long long i = 0);\n", bits);
	fprintf(f, "    int%d(long long a[%d]);\n\n", bits, bits / 64);

	fprintf(f, "    int%d operator+(const int%d& other) const;\n", bits, bits);
	fprintf(f, "    int%d operator-(const int%d& other) const;\n", bits, bits);
	fprintf(f, "    int%d operator*(const int%d& other) const;\n", bits, bits);
	fprintf(f, "    int%d operator/(const int%d& other) const;\n", bits, bits);
	fprintf(f, "    int%d operator%%(const int%d& other) const;\n\n", bits, bits);

	fprintf(f, "    bool operator==(const int%d& other) const;\n", bits);
	fprintf(f, "    bool operator!=(const int%d& other) const;\n", bits);
	fprintf(f, "    bool operator<(const int%d& other) const;\n", bits);
	fprintf(f, "    bool operator>(const int%d& other) const;\n", bits);
	fprintf(f, "    bool operator<=(const int%d& other) const;\n", bits);

	fprintf(f, "    bool operator>=(const int%d& other) const;\n\n", bits);

	fprintf(f, "    int%d operator-() const;\n", bits);
	fprintf(f, "    int%d operator~() const;\n\n", bits);

	fprintf(f, "    int%d operator&(const int%d& other) const;\n", bits, bits);
	fprintf(f, "    int%d operator|(const int%d& other) const;\n", bits, bits);
	fprintf(f, "    int%d operator^(const int%d& other) const;\n\n", bits, bits);

	fprintf(f, "    int%d operator<<(int shift) const;\n", bits);
	fprintf(f, "    int%d operator>>(int shift) const;\n\n", bits);

	fprintf(f, "    int%d& operator+=(const int%d& other);\n", bits, bits);
	fprintf(f, "    int%d& operator-=(const int%d& other);\n", bits, bits);
	fprintf(f, "    int%d& operator*=(const int%d& other);\n", bits, bits);
	fprintf(f, "    int%d& operator/=(const int%d& other);\n", bits, bits);
	fprintf(f, "    int%d& operator%%=(const int%d& other);\n\n", bits, bits);

	fprintf(f, "    int%d& operator&=(const int%d& other);\n", bits, bits);
	fprintf(f, "    int%d& operator|=(const int%d& other);\n", bits, bits);
	fprintf(f, "    int%d& operator^=(const int%d& other);\n\n", bits, bits);

	fprintf(f, "    int%d& operator<<=(int shift);\n", bits);
	fprintf(f, "    int%d& operator>>=(int shift);\n\n", bits);

	fprintf(f, "    int%d& operator++();\n", bits);
	fprintf(f, "    int%d operator++(int);\n", bits);
	fprintf(f, "    int%d& operator--();\n", bits);
	fprintf(f, "    int%d operator--(int);\n\n", bits);

	fprintf(f, "    operator bool() const;\n\n");

	fprintf(f, "    bool is_negative() const;\n\n");

	fprintf(f, "    friend std::ostream& operator<<(std::ostream& os, const int%d& i);\n", bits);
	fprintf(f, "    std::string to_decimal_string() const;\n\n");

	fprintf(f, "private:\n");
	fprintf(f, "    long long value[%d];\n", bits / 64);
	fprintf(f, "};\n");

	sprintf(s, "int%d.cpp", bits);
	f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);

	fprintf(f, "#include \"int%d.h\"\n\n", bits);
	fprintf(f, "#include \"int%d.hpp\"\n\n", bits);
	fprintf(f, "#include <iostream>\n\n");

	fprintf(f, "const int%d int%d::min = INT%d_MIN;\n", bits, bits, bits);
	fprintf(f, "const int%d int%d::max = INT%d_MAX;\n", bits, bits, bits);
	fprintf(f, "const int%d int%d::zero = 0;\n", bits, bits);
	fprintf(f, "const int%d int%d::one = 1;\n", bits, bits);
	fprintf(f, "const int%d int%d::minus_one = -1;\n\n", bits, bits);

	fprintf(f, "int%d::int%d(long long i)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    assign%d(value, i);\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "int%d::int%d(long long a[%d])\n", bits, bits, bits / 64);
	fprintf(f, "{\n");
	fprintf(f, "    for (int i = 0; i < %d; i++)\n", bits / 64);
	fprintf(f, "    {\n");
	fprintf(f, "        value[i] = a[i];\n");
	fprintf(f, "    }\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator+(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    add%d(value, other.value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator-(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    sub%d(value, other.value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator*(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    mul%d(value, other.value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator/(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    int%d r;\n", bits);
	fprintf(f, "    div%d(value, other.value, result.value, r.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator%%(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d r;\n", bits);
	fprintf(f, "    div%d(value, other.value, r.value, value);\n", bits);
	fprintf(f, "    return r;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "bool int%d::operator==(const int%d& other) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    return compare%d(value, other.value) == 0;\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "bool int%d::operator!=(const int%d& other) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    return compare%d(value, other.value) != 0;\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "bool int%d::operator<(const int%d& other) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    return compare%d(value, other.value) < 0;\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "bool int%d::operator>(const int%d& other) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    return compare%d(value, other.value) > 0;\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "bool int%d::operator<=(const int%d& other) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    return compare%d(value, other.value) <= 0;\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "bool int%d::operator>=(const int%d& other) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    return compare%d(value, other.value) >= 0;\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator-() const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    neg%d(value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator~() const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    not%d(value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator&(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    and%d(value, other.value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator|(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    or%d(value, other.value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator^(const int%d& other) const\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    xor%d(value, other.value, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator<<(int shift) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    shl%d(value, shift, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator>>(int shift) const\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result;\n", bits);
	fprintf(f, "    shr%d(value, shift, result.value);\n", bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator+=(const int%d& other)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    add%d(value, other.value, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator-=(const int%d& other)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    sub%d(value, other.value, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator*=(const int%d& other)\n", bits, bits, bits);

	fprintf(f, "{\n");
	fprintf(f, "    mul%d(value, other.value, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator/=(const int%d& other)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d r;\n", bits);
	fprintf(f, "    div%d(value, other.value, value, r.value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator%%=(const int%d& other)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d r;\n", bits);
	fprintf(f, "    div%d(value, other.value, r.value, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator&=(const int%d& other)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    and%d(value, other.value, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator|=(const int%d& other)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    or%d(value, other.value, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator^=(const int%d& other)\n", bits, bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    xor%d(value, other.value, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator<<=(int shift)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    shl%d(value, shift, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator>>=(int shift)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    shr%d(value, shift, value);\n", bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator++()\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    add%d(value, INT%d_ONE.value, value);\n", bits, bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator++(int)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result(*this);\n", bits);
	fprintf(f, "    add%d(value, INT%d_ONE.value, value);\n", bits, bits);
	fprintf(f, "    return result;\n");

	fprintf(f, "}\n\n");

	fprintf(f, "int%d& int%d::operator--()\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    sub%d(value, INT%d_ONE.value, value);\n", bits, bits);
	fprintf(f, "    return *this;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d int%d::operator--(int)\n", bits, bits);
	fprintf(f, "{\n");
	fprintf(f, "    int%d result(*this);\n", bits);
	fprintf(f, "    sub%d(value, INT%d_ONE.value, value);\n", bits, bits);
	fprintf(f, "    return result;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int%d::operator bool() const\n", bits);
	fprintf(f, "{\n");
	fprintf(f, "    return compare%d(value, INT%d_ZERO.value) != 0;\n", bits, bits);
	fprintf(f, "}\n\n");

	fprintf(f, "bool int%d::is_negative() const\n", bits);
	fprintf(f, "{\n");
	fprintf(f, "    return is_negative%d(value);\n", bits);
	fprintf(f, "}\n\n");

	fprintf(f, "std::ostream& operator<<(std::ostream& os, const int%d& i)\n", bits);
	fprintf(f, "{\n");
	fprintf(f, "    os << i.to_decimal_string();\n");
	fprintf(f, "    return os;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "std::string int%d::to_decimal_string() const\n", bits);
	fprintf(f, "{\n");
	fprintf(f, "    char buf[%d / 3 + 1], *s;\n", bits);
	fprintf(f, "    return ::to_decimal_string(value, buf, sizeof(buf));\n");
	fprintf(f, "}\n\n");

	fclose(f);
}

void generate_test_c(int bits)
{
	char s[128];
	FILE* f = NULL;

	sprintf(s, "test_int%d.c", bits);
	f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);

	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include \"int%d.h\"\n\n", bits);

	fprintf(f, "void fibonacci()\n");
	fprintf(f, "{\n");
	fprintf(f, "    INT%d a, b, c;\n", bits);
	fprintf(f, "    char buf[%d / 3 + 1], *s;\n\n", bits);
	fprintf(f, "    assign%d(a, 0);\n", bits);
	fprintf(f, "    assign%d(b, 1);\n\n", bits);
	fprintf(f, "    printf(\"Fibonacci sequence using INT%d\\n\");\n\n", bits);
	fprintf(f, "    for (int i = 0; compare%d(a, b) <= 0; i++)\n", bits);
	fprintf(f, "    {\n");
	fprintf(f, "        s = to_decimal_string%d(b, buf, sizeof(buf));\n", bits);
	fprintf(f, "        printf(\"%%d: %%s\\n\", i, s);\n\n");
	fprintf(f, "        add%d(a, b, c);\n", bits);
	fprintf(f, "        copy%d(b, a);\n", bits);
	fprintf(f, "        copy%d(c, b);\n", bits);
	fprintf(f, "    }\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int main()\n");
	fprintf(f, "{\n");

	fprintf(f, "    INT%d a, b, c, r, aa, bb, cc, rr;\n", bits);
	fprintf(f, "    INT%d zero, one, minus_one;\n\n", bits);

	fprintf(f, "    assign%d(zero, 0);\n", bits);
	fprintf(f, "    assign%d(one, 1);\n", bits);
	fprintf(f, "    assign%d(minus_one, -1);\n\n", bits);

	/* test add */

	fprintf(f, "    /* test add */\n\n");

	fprintf(f, "    assign%d(a, 1);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 2);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    add%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, 3);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, 1);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -2);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    add%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, -1);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, -2);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 1);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    add%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, -1);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MAX, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 1);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    add%d(a, b, c);\n", bits);
	fprintf(f, "    copy%d(INT%d_MIN, cc);\n", bits, bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MIN, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -1);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    add%d(a, b, c);\n", bits);
	fprintf(f, "    copy%d(INT%d_MAX, cc);\n", bits, bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MIN, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    copy%d(INT%d_MIN, b);\n", bits, bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    add%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, 0);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MAX, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    copy%d(INT%d_MAX, b);\n", bits, bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    add%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, -2);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	/* test sub */

	fprintf(f, "    /* test sub */\n\n");

	fprintf(f, "    assign%d(a, 1);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 2);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    sub%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, -1);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MIN, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 1);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    sub%d(a, b, c);\n", bits);
	fprintf(f, "    copy%d(INT%d_MAX, cc);\n", bits, bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MAX, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -1);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    sub%d(a, b, c);\n", bits);
	fprintf(f, "    copy%d(INT%d_MIN, cc);\n", bits, bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MIN, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    copy%d(INT%d_MIN, b);\n", bits, bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    sub%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, 0);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MAX, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    copy%d(INT%d_MAX, b);\n", bits, bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    sub%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, 0);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	/* test mul */

	fprintf(f, "    /* test mul */\n\n");

	fprintf(f, "    assign%d(a, 2);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 3);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    mul%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, 6);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, 2);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -3);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    mul%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, -6);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, -2);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 3);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    mul%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, -6);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, -2);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -3);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    mul%d(a, b, c);\n", bits);
	fprintf(f, "    assign%d(cc, 6);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MAX, a);\n", bits, bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -1);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    mul%d(a, b, c);\n", bits);
	fprintf(f, "    sub%d(c, one, cc);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(cc, INT%d_MIN) == 0);\n\n", bits, bits);


	/* test div */

	fprintf(f, "    /* test div */\n");

	fprintf(f, "    assign%d(a, 10);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 6);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    assign%d(cc, 1);\n", bits);
	fprintf(f, "    assign%d(rr, 4);\n", bits);
	fprintf(f, "    div%d(a, b, c, r);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(r, rr) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, 10);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -6);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    assign%d(cc, -1);\n", bits);
	fprintf(f, "    assign%d(rr, 4);\n", bits);
	fprintf(f, "    div%d(a, b, c, r);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(r, rr) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, -10);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, 6);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    assign%d(cc, -1);\n", bits);
	fprintf(f, "    assign%d(rr, -4);\n", bits);
	fprintf(f, "    div%d(a, b, c, r);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(r, rr) == 0);\n\n", bits);

	fprintf(f, "    assign%d(a, -10);\n", bits);
	fprintf(f, "    copy%d(a, aa);\n", bits);
	fprintf(f, "    assign%d(b, -6);\n", bits);
	fprintf(f, "    copy%d(b, bb);\n", bits);
	fprintf(f, "    assign%d(cc, 1);\n", bits);
	fprintf(f, "    assign%d(rr, -4);\n", bits);
	fprintf(f, "    div%d(a, b, c, r);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(b, bb) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(c, cc) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(r, rr) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MAX, a);\n", bits, bits);
	fprintf(f, "    copy%d(INT%d_MAX, b);\n", bits, bits);
	fprintf(f, "    div%d(a, b, c, r);\n", bits);
	fprintf(f, "    assert(compare%d(c, one) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(r, zero) == 0);\n\n", bits);

	fprintf(f, "    copy%d(INT%d_MAX, a);\n", bits, bits);
	fprintf(f, "    assign%d(b, 2);\n", bits);
	fprintf(f, "    div%d(a, b, c, r);\n", bits);
	fprintf(f, "    shl%d(c, 1);\n", bits);
	fprintf(f, "    add%d(c, one, cc);\n", bits);
	fprintf(f, "    assert(compare%d(a, cc) == 0);\n", bits);
	fprintf(f, "    assert(compare%d(r, one) == 0);\n\n", bits);

	fprintf(f, "    parse%d(\"0\", a);\n", bits);
	fprintf(f, "    assert(compare%d(a, zero) == 0);\n\n", bits);

	fprintf(f, "    parse%d(\"1\", a);\n", bits);
	fprintf(f, "    assert(compare%d(a, one) == 0);\n\n", bits);

	fprintf(f, "    parse%d(\"-1\", a);\n", bits);
	fprintf(f, "    assert(compare%d(a, minus_one) == 0);\n\n", bits);

	fprintf(f, "    parse%d(\"123\", a);\n", bits);
	fprintf(f, "    assign%d(aa, 123);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n\n", bits);

	fprintf(f, "    parse%d(\"-123\", a);\n", bits);
	fprintf(f, "    assign%d(aa, -123);\n", bits);
	fprintf(f, "    assert(compare%d(a, aa) == 0);\n\n", bits);

	fprintf(f, "    printf(\"All tests passed.\\n\");\n\n");

	fprintf(f, "    fibonacci();\n\n");

	fprintf(f, "    return 0;\n");
	fprintf(f, "}\n");

	fclose(f);
}

void generate_test_cpp(int bits)
{
	char s[128];
	FILE* f = NULL;

	sprintf(s, "test_int%d.cpp", bits);
	f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);

	fprintf(f, "#include \"int%d.hpp\"\n", bits);
	fprintf(f, "#include <assert.h>\n");

	fprintf(f, "void fibonacci()\n");
	fprintf(f, "{\n");
	fprintf(f, "    int%d a = 0, b = 1, c;\n\n", bits);
	
	fprintf(f, "    printf(\"Fibonacci sequence using int%d\\n\");\n\n", bits);
	fprintf(f, "    for (int i = 0; a <= b; i++)\n");
	fprintf(f, "    {\n");
	fprintf(f, "        std::cout << i << \": \" << b.to_decimal_string() << std::endl;");
	fprintf(f, "        c = a + b;\n");
	fprintf(f, "        a = b;\n");
	fprintf(f, "        b = c;\n");
	fprintf(f, "    }\n");
	fprintf(f, "}\n\n");

	fprintf(f, "int main()\n");
	fprintf(f, "{\n");

	fprintf(f, "    int%d a, b, c, r, aa, bb, cc, rr;\n", bits);

	/* test add */

	fprintf(f, "    // test add\n\n");

	fprintf(f, "    a = 1;\n");
	fprintf(f, "    b = 2;\n");
	fprintf(f, "    c = a + b;\n");
	fprintf(f, "    assert(a == 1);\n");
	fprintf(f, "    assert(b == 2);\n");
	fprintf(f, "    assert(c == 3);\n\n");

	fprintf(f, "    a = 1;\n");
	fprintf(f, "    b = -2;\n");
	fprintf(f, "    c = a + b;\n");
	fprintf(f, "    assert(a == 1);\n");
	fprintf(f, "    assert(b == -2);\n");
	fprintf(f, "    assert(c == -1);\n\n");

	fprintf(f, "    a = -2;\n");
	fprintf(f, "    b = 1;\n");
	fprintf(f, "    c = a + b;\n");
	fprintf(f, "    assert(a == -2);\n");
	fprintf(f, "    assert(b == 1);\n");
	fprintf(f, "    assert(c == -1);\n\n");

	fprintf(f, "    a = int%d::max;\n", bits);
	fprintf(f, "    b = 1;\n");
	fprintf(f, "    c = a + b;\n");
	fprintf(f, "    assert(a == int%d::max);\n", bits);
	fprintf(f, "    assert(b == 1);\n");
	fprintf(f, "    assert(c == int%d::min);\n\n", bits);

	fprintf(f, "    a = int%d::min;\n", bits);
	fprintf(f, "    b = -1;\n");
	fprintf(f, "    c = a + b;\n");
	fprintf(f, "    assert(a == int%d::min);\n", bits);
	fprintf(f, "    assert(b == -1);\n");
	fprintf(f, "    assert(c == int%d::max);\n\n", bits);


	/* test sub */

	fprintf(f, "    // test sub\n\n");

	fprintf(f, "    a = 1;\n");
	fprintf(f, "    b = 2;\n");
	fprintf(f, "    c = a - b;\n");
	fprintf(f, "    assert(a == 1);\n");
	fprintf(f, "    assert(b == 2);\n");
	fprintf(f, "    assert(c == -1);\n\n");

	fprintf(f, "    a = int%d::min;\n", bits);
	fprintf(f, "    b = 1;\n");
	fprintf(f, "    c = a - b;\n");
	fprintf(f, "    assert(a == int%d::min);\n", bits);
	fprintf(f, "    assert(b == 1);\n");
	fprintf(f, "    assert(c == int%d::max);\n\n", bits);

	fprintf(f, "    a = int%d::max;\n", bits);
	fprintf(f, "    b = -1;\n");
	fprintf(f, "    c = a - b;\n");
	fprintf(f, "    assert(a == int%d::min);\n", bits);

	/* test mul */

	fprintf(f, "    // test mul\n\n");

	fprintf(f, "    a = 2;\n");
	fprintf(f, "    b = 3;\n");
	fprintf(f, "    c = a * b;\n");
	fprintf(f, "    assert(a == 2);\n");
	fprintf(f, "    assert(b == 3);\n");
	fprintf(f, "    assert(c == 6);\n\n");

	fprintf(f, "    a = 2;\n");
	fprintf(f, "    b = -3;\n");
	fprintf(f, "    c = a * b;\n");
	fprintf(f, "    assert(a == 2);\n");
	fprintf(f, "    assert(b == -3);\n");
	fprintf(f, "    assert(c == -6);\n\n");

	fprintf(f, "    a = -2;\n");
	fprintf(f, "    b = 3;\n");
	fprintf(f, "    c = a * b;\n");
	fprintf(f, "    assert(a == -2);\n");
	fprintf(f, "    assert(b == 3);\n");
	fprintf(f, "    assert(c == -6);\n\n");

	fprintf(f, "    a = -2;\n");
	fprintf(f, "    b = -3;\n");
	fprintf(f, "    c = a * b;\n");
	fprintf(f, "    assert(a == -2);\n");
	fprintf(f, "    assert(b == -3);\n");
	fprintf(f, "    assert(c == 6);\n\n");

	fprintf(f, "    a = int%d::max;\n", bits);
	fprintf(f, "    b = -1;\n");
	fprintf(f, "    c = a * b;\n");
	fprintf(f, "    assert(a == int%d::max);\n", bits);
	fprintf(f, "    assert(b == -1);\n");
	fprintf(f, "    assert(c == int%d::min);\n", bits);

	/* test div */

	fprintf(f, "    // test div\n");

	fprintf(f, "    a = 10;\n");
	fprintf(f, "    b = 6;\n");
	fprintf(f, "    c = a / b;\n");
	fprintf(f, "    r = a %% b;\n");
	fprintf(f, "    assert(a == 10);\n");
	fprintf(f, "    assert(b == 6);\n");
	fprintf(f, "    assert(c == 1);\n");
	fprintf(f, "    assert(r == 4);\n\n");

	fprintf(f, "    a = 10;\n");
	fprintf(f, "    b = -6;\n");
	fprintf(f, "    c = a / b;\n");
	fprintf(f, "    r = a %% b;\n");
	fprintf(f, "    assert(a == 10);\n");
	fprintf(f, "    assert(b == -6);\n");
	fprintf(f, "    assert(c == -1);\n");
	fprintf(f, "    assert(r == 4);\n\n");

	fprintf(f, "    a = -10;\n");
	fprintf(f, "    b = 6;\n");
	fprintf(f, "    c = a / b;\n");
	fprintf(f, "    r = a %% b;\n");
	fprintf(f, "    assert(a == -10);\n");
	fprintf(f, "    assert(b == 6);\n");
	fprintf(f, "    assert(c == -1);\n");
	fprintf(f, "    assert(r == -4);\n\n");

	fprintf(f, "    a = -10;\n");
	fprintf(f, "    b = -6;\n");
	fprintf(f, "    c = a / b;\n");
	fprintf(f, "    r = a %% b;\n");
	fprintf(f, "    assert(a == -10);\n");
	fprintf(f, "    assert(b == -6);\n");
	fprintf(f, "    assert(c == 1);\n");
	fprintf(f, "    assert(r == -4);\n\n");

	fprintf(f, "    a = int%d::max;\n", bits);
	fprintf(f, "    b = int%d::max;\n", bits);
	fprintf(f, "    c = a / b;\n");
	fprintf(f, "    r = a %% b;\n");
	fprintf(f, "    assert(c == 1);\n\n");
	fprintf(f, "    assert(r == 0);\n\n");

	fprintf(f, "    printf(\"All tests passed.\\n\");\n\n");

	fprintf(f, "    fibonacci();\n\n");

	fprintf(f, "    return 0;\n");
	fprintf(f, "}\n");

	fclose(f);
}

void generate_project_custom_build(FILE* f, int bits)
{
	char s[128];
	sprintf(s, "int%d", bits);

	fprintf(f, "    <CustomBuild Include=\"%s.asm\">\n", s);
	fprintf(f, "      <FileType>Document</FileType>\n");
	fprintf(f, "      <Command>nasm -fwin64 -o%s.obj %s.asm</Command>\n", s, s);
	fprintf(f, "      <Outputs>%s.obj</Outputs>\n", s);
	fprintf(f, "      <OutputItemType>Object</OutputItemType>\n"
		"      <BuildInParallel>true</BuildInParallel>\n"
		"    </CustomBuild>\n");
}

void generate_project(char c, int bits)
{
	char s[128];
	FILE* f = NULL;

	if (c == 'c')
		sprintf(s, "test_int%d_c.vcxproj", bits);
	else
		sprintf(s, "test_int%d_cpp.vcxproj", bits);
	f = fopen(s, "w");
	if (f == NULL)
		error("Cannot open file", s);
	fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
		"  <ItemGroup Label=\"ProjectConfigurations\">\n"
		"    <ProjectConfiguration Include=\"Release|x64\">\n"
		"      <Configuration>Release</Configuration>\n"
		"      <Platform>x64</Platform>\n"
		"    </ProjectConfiguration>\n"
		"    <ProjectConfiguration Include=\"Debug|x64\">\n"
		"      <Configuration>Debug</Configuration>\n"
		"      <Platform>x64</Platform>\n"
		"    </ProjectConfiguration>\n"
		"  </ItemGroup>\n"
		"  <ItemGroup>\n");

	generate_project_custom_build(f, bits);

	fprintf(f, "  </ItemGroup>\n"
		"  <ItemGroup>\n"
		"    <ClCompile Include=\"int%d.c\"/>\n");
	if (c == 'c')
		fprintf(f, "    <ClCompile Include=\"test_int%d.c\"/>\n", bits);
	else
		fprintf(f, "    <ClCompile Include=\"test_int%d.cpp\"/>\n", bits);
	fprintf(f, "  </ItemGroup>\n"
		"  <ItemGroup>\n"
		"    <ClInclude Include=\"int%d.h\"/>\n"
		"  </ItemGroup>\n", bits, bits, bits);

	fprintf(f, "%s",
		"  <PropertyGroup Label=\"Globals\">\n"
		"    <VCProjectVersion>17.0</VCProjectVersion>\n"
		"    <Keyword>Win32Proj</Keyword>\n"
		"    <ProjectGuid>{5fde6895-ddd7-4173-9d15-d609a12293cc}</ProjectGuid>\n"
		"    <RootNamespace>testint256</RootNamespace>\n"
		"    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\n"
		"  </PropertyGroup>\n"
		"  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n"
		"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"Configuration\">\n"
		"    <ConfigurationType>Application</ConfigurationType>\n"
		"    <UseDebugLibraries>false</UseDebugLibraries>\n"
		"    <PlatformToolset>v143</PlatformToolset>\n"
		"    <WholeProgramOptimization>true</WholeProgramOptimization>\n"
		"    <CharacterSet>Unicode</CharacterSet>\n"
		"  </PropertyGroup>\n"
		"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"Configuration\">\n"
		"    <ConfigurationType>Application</ConfigurationType>\n"
		"    <UseDebugLibraries>true</UseDebugLibraries>\n"
		"    <PlatformToolset>v143</PlatformToolset>\n"
		"    <WholeProgramOptimization>false</WholeProgramOptimization>\n"
		"    <CharacterSet>Unicode</CharacterSet>\n"
		"  </PropertyGroup>\n"
		"  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n"
		"  <ImportGroup Label=\"ExtensionSettings\" />\n"
		"  <ImportGroup Label=\"Shared\" />\n"
		"  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n"
		"    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n"
		"  </ImportGroup>\n"
		"  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n"
		"    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n"
		"  </ImportGroup>\n"
		"  <PropertyGroup Label=\"UserMacros\" />\n"
		"  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n"
		"    <ClCompile>\n"
		"      <WarningLevel>Level3</WarningLevel>\n"
		"      <FunctionLevelLinking>true</FunctionLevelLinking>\n"
		"      <IntrinsicFunctions>true</IntrinsicFunctions>\n"
		"      <SDLCheck>true</SDLCheck>\n"
		"      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n"
		"      <ConformanceMode>true</ConformanceMode>\n"
		"    </ClCompile>\n"
		"    <Link>\n"
		"      <SubSystem>Console</SubSystem>\n"
		"      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n"
		"      <OptimizeReferences>true</OptimizeReferences>\n"
		"      <GenerateDebugInformation>true</GenerateDebugInformation>\n"
		"    </Link>\n"
		"  </ItemDefinitionGroup>\n"
		"  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n"
		"    <ClCompile>\n"
		"      <WarningLevel>Level3</WarningLevel>\n"
		"      <SDLCheck>true</SDLCheck>\n"
		"      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n"
		"      <ConformanceMode>true</ConformanceMode>\n"
		"    </ClCompile>\n"
		"    <Link>\n"
		"      <SubSystem>Console</SubSystem>\n"
		"      <GenerateDebugInformation>true</GenerateDebugInformation>\n"
		"    </Link>\n"
		"  </ItemDefinitionGroup>\n"
		"  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n"
		"  <ImportGroup Label=\"ExtensionTargets\">\n"
		"  </ImportGroup>\n"
		"</Project>\n");

	fclose(f);
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		usage();
		return 1;
	}

	int bits = atoi(argv[1]);
	if (bits < 128 || bits % 64 != 0)
	{
		usage();
		return 2;
	}

	generate_asm(bits);
	generate_c(bits);
	generate_cpp(bits);

	generate_test_c(bits);
	generate_project('c', bits);

	generate_test_cpp(bits);
	generate_project('x', bits);

	return 0;
}
