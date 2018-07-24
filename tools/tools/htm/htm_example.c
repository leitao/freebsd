/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2018 Breno Leitao
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/* HTM testsuite for powerpc64. It contains specific assembly code that is not
 * portable
 */

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

#if !defined(__powerpc64__)
#error "This is a powerpc64-only testsuite"
#endif

#define SPR_TEXASR_FC           0xFE00000000000000
#define SPR_TEXASR_ABORT        0x100000000
#define SPR_TEXASR_SUSP         0x80000000
#define SPR_TEXASR_HV           0x20000000
#define SPR_TEXASR_PR           0x10000000
#define SPR_TEXASR_FS           0x8000000
#define SPR_TEXASR_EXACT        0x4000000
#define SPR_TEXASR_ROT          0x2000000
#define SPR_TEXASR_TL           0xFFF;

/* Sleep time (in seconds) before reading TEXASR SPR. Test to guarantee that
 * the process is scheduled in and out.
 */
#define SLEEP_TIME		5

uint64_t mftexasr(void);
int force_abort_kernel(void);
int force_abort_pr(void);
int check_texasr_kernel(void);
int check_texasr_pr(void);
int check_texasr_succeed(void);
int transaction_succeed(void);

uint64_t mftexasr(void)
{
	uint64_t val;

	/* SPR_TEXASR = 0x82 */
	asm volatile ("mfspr	%0, 0x82;"
		      : "=r" (val));

#ifdef DEBUG
	printf("> TEXASR = 0x%lx\n", val);
#endif

	return (val);
}

int check_texasr_kernel(void)
{
        int ret = 0;
	uint64_t texasr;

	/* Testing to guarantee that the process is scheduled in and out */
        sleep(SLEEP_TIME);
	texasr = mftexasr();

        if ((texasr & SPR_TEXASR_PR) == SPR_TEXASR_PR) {
                printf("!! Tested failed: [TEXASR] Transaction was not supposed to fail in userspace\n");
		printf("\tTEXASR = %lx\n", texasr);

                ret++;
        }

        if ((texasr & SPR_TEXASR_ABORT) != SPR_TEXASR_ABORT) {
                printf("!! Tested failed: [TEXASR] Transaction was supposed to be abort\n");
		printf("\tTEXASR = %lx\n", texasr);

                ret++;
        };

        return (ret);
}


int check_texasr_pr(void)
{
	int ret = 0;
	uint64_t texasr;

	sleep(SLEEP_TIME);
	texasr = mftexasr();

	if ((texasr & SPR_TEXASR_PR) != SPR_TEXASR_PR) {
		printf("!! Tested failed: [TEXASR] Transaction was not supposed to fail in kernel space\n");
		printf("\tTEXASR = %lx\n", texasr);

		ret++;
	};

	if ((texasr & SPR_TEXASR_ABORT) != SPR_TEXASR_ABORT) {
		printf("!! Tested failed: [TEXASR] Transaction was supposed to be forced to aborted\n");
		printf("\tTEXASR = %lx\n", texasr);

		ret++;
	}

	return (ret);
}

int force_abort_kernel(void)
{
	int ret;

	printf("Testing a transaction failure (abort) in kernel space\n");

	asm volatile goto (
		"tbegin.	;"
		"beq 	%[fail]	;"
		"trap		;"
		"1:     bdnz 1b	;"
		"tend.		;"
		:
		:
		:
		: fail
	);

	printf("!! Tested failed.Unexpected transaction commmit\n");

	return (1);

fail:
	ret = check_texasr_kernel();
	if (ret == 0){
		printf(":: Tested passed\n");
		return 0;
	}

	return ret;
}

int force_abort_pr(void)
{
	int ret;

	printf("Testing transaction failure (tabort) in userspace (PR)\n");

	asm volatile goto (
		"tbegin.		;"
		"beq		%[fail]	;"
		"tabort. 	0	;"
		"tend.			;"
		:
		:
		:
		: fail
	);

	printf("!! Tested Failed. Unexpected transaction commmit.\n");

	return (1);

	/* Failure handler. Will beset at TFHAR */
fail:
	ret = check_texasr_pr();
	if (ret == 0){
		printf(":: Tested passed\n");
		return 0;
	}

	return (ret);
}

int check_texasr_succeed(void)
{
	uint64_t texasr;

	sleep(SLEEP_TIME);
	texasr = mftexasr();

	if (texasr != 0) {
		printf("! Texasr should be set to zero. TEXASR = %lx\n", texasr);

		return (1);
	}

	return (0);
}

int transaction_succeed(void)
{
	int ret = 0;
	uint64_t gpr3;

	printf("Testing Transaction commit\n");

	asm volatile goto (
		"tbegin.		;"
		"beq 	%[fail]		;"
		"li	3, 0xEF		;"
		"addis	3, 3, 0xBE	;"
		"tend.			;"
		:
		:
		:
		: fail
	);

	/* Test if r3 contains 0xbe00ef */
	asm volatile ("mr %0, 3"
		: "=r" (gpr3)
		:
	);
	if (gpr3 != 0xbe00ef) {
		printf("Transaction committed, but GPR3 is invalid (%lx)\n",
			gpr3);
		ret = 1;
	}

#ifdef DEBUG
	printf("GPR3 set properly (%lx)\n", gpr3);
#endif

	ret += check_texasr_succeed();
	if (ret == 0){
		printf(":: Tested passed\n");
		return 0;
	} else {
		printf(":: Tested failed\n");
		return (ret);
	}


	/* Transaction Failure handler. Should not execute after here */
fail:
	printf("!! Tested failed. Transaction not expected to fail\n");

	return (check_texasr_succeed() + 1);
}

int main(void)
{
	int ret;

	if (mftexasr() != 0) {
		printf("[TEXASR] Failure: Texasr is initially set to non zero value:"
		        "%lx\n", mftexasr());
		return (-1);
	}

	ret = force_abort_pr();
	ret += force_abort_kernel();
	ret += transaction_succeed();

	if (ret == 0)
		printf("\nALL Tested Passed\n");

	return (ret);
}
