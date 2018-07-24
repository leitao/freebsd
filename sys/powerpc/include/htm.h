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

#ifndef	_MACHINE_HTM_H_
#define	_MACHINE_HTM_H_

/* Enable HTM for the very first time, through a HTM Facility Unavailable */
void enable_htm_thread(struct thread *td);

/* Save the HTM states prior to a process leaving the CPU */
void save_htm(struct thread *td);

/* Restore the HTM state before the process is scheduled */
void restore_htm(struct thread *td);

/* Check if HTM is enabled in the MSR */
bool htm_enabled(register_t msr);

/* Check if HTM is in transactional state*/
bool htm_transactional(register_t msr);

/* Check if HTM is in a active state*/
bool htm_active(register_t msr);

/* Check if HTM is in suspended state*/
bool htm_suspended(register_t msr);

/* enable MSR[HTM] on the current CPU */
void enable_htm_current_cpu(void);

void tabort(void);

#endif	/* _MACHINE_HTM_H_ */

