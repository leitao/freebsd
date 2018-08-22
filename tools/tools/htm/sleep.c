#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

#define	SPR_TEXASR		0x083
#define SPR_TEXASR_PR		0x10000000
#define SPR_TEXASR_HV		0x20000000
#define SPR_TEXASR_ABORT	0x80000000

uint64_t counter = 0x0ff000000;

uint64_t mftexasr() {
	uint64_t val;
	asm volatile ("mfspr %0, 0x82;"
		      : "=r" (val));
	return val;
}

int check_texasr_kernel(uint64_t texasr) {
        int ret = 0;

	/* Testing to guarantee that the process is scheduled in and out */
        sleep(5);
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


int check_texasr_pr(uint64_t texasr) {
	int ret = 0;

	/* Testing to guarantee that the process is scheduled in and out */
	sleep(5);
	if ((texasr & SPR_TEXASR_PR) != SPR_TEXASR_PR) {	
		printf("!! Tested failed: [TEXASR] Transaction was not supposed to fail in kernel space\n");
		printf("\tTEXASR = %lx\n", texasr);
		ret++;
	};

	if ((texasr & SPR_TEXASR_ABORT) == SPR_TEXASR_ABORT) {	
		printf("!! Tested failed: [TEXASR] Transaction was not supposed to be forced to aborted\n");
		ret++;
		printf("\tTEXASR = %lx\n", texasr);
	} 
	return ret;
}

int force_abort_kernel(){
	uint64_t texasr;
	int ret;

	printf("Testing a transaction failure (abort) in kernel space\n");

	asm goto (
		"tbegin.	;"
		"beq 	%[fail];"
		"trap;"
		"1:     bdnz 1b ;"
		"tend.	;"
		:
		:[counter] "r" (counter)
		:
		: fail
	);

	printf("!! Tested failed.Unexpected transaction commmit\n");
	return (+1);
fail:
	texasr = mftexasr();
	ret =  check_texasr_kernel(texasr);
	if (ret == 0){
		printf(":: Tested passed\n");
		return 0;
	}
	return ret;
}

int force_abort_pr(){
	uint64_t texasr;
	int ret;

	texasr = mftexasr();
	printf("Testing transaction failure in userspace (PR)\n");

	asm goto (
		"tbegin.	;"
		"beq 	%[fail];"
		"tabort. 0;"
		"tend.	;"
		:
		:[counter] "r" (counter)
		:
		: fail
	);

	printf("!! Tested Failed. Unexpected transaction commmit\n");
	return (+1);
fail:
	texasr = mftexasr();
	ret =  check_texasr_pr(texasr);
	if (ret == 0){
		printf(":: Tested passed\n");
		return 0;
	}
	return ret;
}

int check_texasr_succeed(uint64_t texasr){
	if (texasr != 0) {
		printf("Texasr should be set to zero. TEXASR = %lx\n", texasr);
		return 1;
	}
	return 0;
}

int transaction_succeed(){
	uint64_t texasr;
	int ret;

	printf("Testing Transaction commit\n");

	asm goto (
		"tbegin.	;"
		"beq 	%[fail];"
		"li	3, 0;"
		"tend.	;"
		:
		:[counter] "r" (counter)
		:
		: fail
	);

	texasr = mftexasr();
	ret = check_texasr_succeed(texasr);
	if (ret == 0){
		printf(":: Tested passed\n");
		return 0;
	}
	return ret;
fail:
	printf("!! Tested failed. Transaction not expected to fail\n");

	texasr = mftexasr();

	return check_texasr_succeed(texasr) + 1;
}

int main(){
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

	return ret;
}
