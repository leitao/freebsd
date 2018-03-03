/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (C) 2018 Breno Leitao
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/limits.h>

#include <machine/htm.h>
#include <machine/pcb.h>
#include <machine/psl.h>

static void enable_htm_current_cpu(void);
static void restore_htm_sprs(struct pcb *pcb);


/* Enable current HTM on current CPU MSR */
static void enable_htm_current_cpu() {
	register_t msr;

	msr = mfmsr();
	msr |= PSL_HTM;
	mtmsr(msr);
        isync();

}

/* Restore HTM Special registers from PCB */
static void restore_htm_sprs(struct pcb *pcb) {
	//KASSERT(mfmsr & PSL_HTM,
	//	"Trying to restore HTM SPRs without having MSR[HTM] enabled\n");
	printf("MSR is %llx\n", (long long) mfmsr());

	if ( (mfmsr() & PSL_HTM) == 0) {
		printf("PANIC\n");
	} else {
	mtspr(SPR_TFHAR, pcb->pcb_htm.tfhar);
	mtspr(SPR_TEXASR, pcb->pcb_htm.texasr);
	mtspr(SPR_TFIAR, pcb->pcb_htm.tfiar);

        isync();
	}
}

/* Enable HTM on thread on the first time */
void enable_htm_thread(struct thread *td) {
        struct  pcb *pcb;
        struct  trapframe *tf;

	printf("Enabling htm for thread td %s\n", td->td_name);

        pcb = td->td_pcb;
        tf = trapframe(td);

	/*
         * Save the thread's Altivec CPU number, and set the CPU's current
         * vector thread
         */
	td->td_pcb->pcb_htmcpu = PCPU_GET(cpuid);
        PCPU_SET(htmthread, td);

        /*
         * Enable the vector unit for when the thread returns from the
         * exception. If this is the first time the unit has been used by
         * the thread, initialise the vector registers and VSCR to 0, and
         * set the flag to indicate that the vector unit is in use.
         */
        tf->srr1 |= PSL_HTM;
        if (!(pcb->pcb_flags & PCB_HTM)) {
                memset(&pcb->pcb_htm, 0, sizeof pcb->pcb_htm);
                pcb->pcb_flags |= PCB_HTM;
        }

        /*
         * Enable HTM on MSR since we are going to access HTM SPRs
         */
	enable_htm_current_cpu();

	restore_htm_sprs(pcb);
}



