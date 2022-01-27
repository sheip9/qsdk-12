/* Copyright (c) 2011-2012, 2017-2019 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PLATFORM_IPQ40XX_GPIO_H
#define __PLATFORM_IPQ40XX_GPIO_H

/* GPIO TLMM: Direction */
#ifndef GPIO_INPUT
#define GPIO_INPUT		0
#endif
#ifndef GPIO_OUTPUT
#define GPIO_OUTPUT		1
#endif

/* GPIO TLMM: Output value */
#define GPIO_OUT_LOW		0
#define GPIO_OUT_HIGH		1

/* GPIO TLMM: Pullup/Pulldown */
#define GPIO_NO_PULL		0
#define GPIO_PULL_DOWN		1
#define GPIO_PULL_UP    	2
#define GPIO_NOT_DEF    	3

/* GPIO TLMM: Drive Strength */
#define GPIO_2MA		0
#define GPIO_4MA		1
#define GPIO_6MA		2
#define GPIO_8MA		3
#define GPIO_10MA		4
#define GPIO_12MA		5
#define GPIO_14MA		6
#define GPIO_16MA		7

/* GPIO TLMM: Status */
#define GPIO_OE_ENABLE		1
#define GPIO_OE_DISABLE		0

/* GPIO VM */
#define GPIO_VM_ENABLE  1
#define GPIO_VM_DISABLE 0

/* GPIO OD */
#define GPIO_OD_ENABLE  1
#define GPIO_OD_DISABLE 0

/* GPIO PULLUP RES */
#define GPIO_PULL_RES0  0
#define GPIO_PULL_RES1  1
#define GPIO_PULL_RES2  2
#define GPIO_PULL_RES3  3

 
#define GSBI_1			1
#define GSBI_2			2
#define GSBI_4			4

#define BLSP_1			1

void gpio_config_i2c(uint8_t gsbi_id);
void gpio_config_uart_dm(uint8_t id);
void gpio_config_blsp_i2c(uint8_t blsp_id, uint8_t qup_id);

#endif