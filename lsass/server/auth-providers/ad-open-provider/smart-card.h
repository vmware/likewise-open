/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright 2013 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (C) VMware, Inc.  All rights reserved.
 *
 * Module Name:
 *
 *        smart_card.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Smart card handling functions (public header)
 *
 * Author: Dmitry Torokhov <dtor@vmware.com>
 */

#ifndef __AD_SMART_CARD_H__
#define __AD_SMART_CARD_H__

DWORD
AD_InitializeSmartCard(
    IN PLSA_AD_PROVIDER_STATE pState
    );

VOID
AD_FinishSmartCard(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_InspectSmartCard(
    IN PAD_PROVIDER_CONTEXT pContext,
    OUT PLSA_SECURITY_OBJECT* ppUserInfo,
    OUT PSTR* ppszSmartCardReader
    );

#endif /* __AD_SMART_CARD_H__ */
