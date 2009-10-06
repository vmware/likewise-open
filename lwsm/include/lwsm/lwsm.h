/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        lwsm.h
 *
 * Abstract:
 *
 *        Primary public header file
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWSM_H__
#define __LWSM_H__

/**
 * @file lwsm.h
 * @brief Likewise Service Manager API
 */

#include <lw/base.h>
#include <lwdef.h>

/**
 * @defgroup client Client API
 * @brief Client API
 *
 * This module contains the client-side API used to query and control services
 * in the Likewise Service Manager (lwsmd)
 */

/*@{*/

/**
 * @brief Opaque service handle
 *
 * A handle to a particular service that can be used to query or
 * perform operations upon it.
 */
typedef struct _LW_SERVICE_HANDLE *LW_SERVICE_HANDLE;

/**
 * @brief A pointer to an opaque service handle
 */
typedef struct _LW_SERVICE_HANDLE **PLW_SERVICE_HANDLE;

/**
 * @brief Status of a service
 *
 * Represents the status of a service (running, stopped, etc.)
 */
typedef enum _LW_SERVICE_STATUS
{
    /** @brief Service is running */
    LW_SERVICE_RUNNING = 0,
    /** @brief Service is stopped */
    LW_SERVICE_STOPPED = 1,
    /** @brief Service is starting */
    LW_SERVICE_STARTING = 2,
    /** @brief Service is stopping */
    LW_SERVICE_STOPPING = 3,
    /** @brief Service is paused */
    LW_SERVICE_PAUSED = 4,
    /** @brief Service is pining for the fjords */
    LW_SERVICE_DEAD = 5
} LW_SERVICE_STATUS;

/**
 * @brief A pointer to a service status value
 */
typedef LW_SERVICE_STATUS *PLW_SERVICE_STATUS;

/**
 * @brief Service type
 *
 * Represents the type of a service
 */
typedef enum _LW_SERVICE_TYPE
{
    /** Service is an executable */
    LW_SERVICE_EXECUTABLE = 0x0,
    /** Service is a module for a container */
    LW_SERVICE_MODULE = 0x1,
    /** Service is a driver */
    LW_SERVICE_DRIVER = 0x2
} LW_SERVICE_TYPE;

/**
 * @brief A pointer to a service type value
 */
typedef LW_SERVICE_TYPE *PLW_SERVICE_TYPE;

/**
 * @brief Service information
 *
 * Describes the basic information about a service,
 * such as its path, command line arguments, etc.
 */
typedef struct _LW_SERVICE_INFO
{
    /** @brief Service short name */
    LW_PWSTR pwszName;
    /** @brief Service description */
    LW_PWSTR pwszDescription;
    /** @brief Service type */
    LW_SERVICE_TYPE type;
    /** @brief Path to service executable or module */
    LW_PWSTR pwszPath;
    /** @brief Arguments to service when started */
    LW_PWSTR* ppwszArgs;
    /** @brief Names of services on which this service depends */
    LW_PWSTR* ppwszDependencies;
    /** @brief Is this service started on system startup? */
    LW_BOOL bStartupService;
} LW_SERVICE_INFO;

/**
 * @brief A pointer to a service info structure
 */
typedef LW_SERVICE_INFO *PLW_SERVICE_INFO;

/**
 * @brief A pointer to a constant service info structure
 */
typedef LW_SERVICE_INFO const * PCLW_SERVICE_INFO;

/**
 * @brief Acquire service handle
 *
 * Gets a handle to a known service by name
 *
 * @param[in] pwszServiceName the name of the service
 * @param[out] phHandle a service handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_NO_SUCH_SERVICE no service with the specified name exists
 */
DWORD
LwSmAcquireServiceHandle(
    LW_PCWSTR pwszServiceName,
    PLW_SERVICE_HANDLE phHandle
    );

/**
 * @brief Release a service handle
 *
 * Releases a handle previously acquired with #LwSmAcquireServiceHandle().
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmReleaseServiceHandle(
    LW_SERVICE_HANDLE hHandle
    );

/**
 * @brief Enumerate available services
 *
 * Returns a NULL-terminated list of strings containing the names
 * of all known services.  The list should be freed with
 * #LwSmFreeServiceNameList() when done.
 *
 * @param[out] pppwszServiceNames the returned list of services
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmEnumerateServices(
    PWSTR** pppwszServiceNames
    );

/**
 * @brief Free service name list
 *
 * Frees a service name list as returned by e.g. #LwSmEnumerateServices()
 *
 * @param[in,out] ppwszServiceNames the list of services
 */
VOID
LwSmFreeServiceNameList(
    PWSTR* ppwszServiceNames
    );

/**
 * @brief Add new service
 *
 * Adds a new service to the service manager described by
 * the provided service info structure.
 *
 * @param[in] pServiceInfo a service info structure describing the new service
 */
DWORD
LwSmAddService(
    PCLW_SERVICE_INFO pServiceInfo
    );

/**
 * @brief Remove an existing service
 *
 * Removes an existing service with the given name from the service manager.
 *
 * @param[in] pwszName the name of the service
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_NO_SUCH_SERVICE a service with the specified name did not exist
 */
DWORD
LwSmRemoveService(
    LW_PCWSTR pwszName
    );

/**
 * @brief Start a service
 *
 * Starts the service represented by the given service handle.  If the service
 * is already started, this function trivially succeeds.  If the service is not
 * started, it will attempt to start it and wait for it finish starting.
 * If the service is in the process of starting, it will wait for it to finish
 * starting.
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_SERVICE_UNRESPONSIVE the service did not respond to requests to start
 * @retval LW_ERROR_SERVICE_DEPENDENCY_UNMET the service depends on another service which is not running
 * @retval LW_ERROR_INVALID_SERVICE_TRANSITION the service cannot be started in its present state
 */
DWORD
LwSmStartService(
    LW_SERVICE_HANDLE hHandle
    );
/**
 * @brief Stop a service
 *
 * Stops the service represented by the given service handle.  If the service
 * is already stopped, this function trivially succeeds.  If the service is not
 * stopped, it will attempt to stop it and wait for it finish stopping.
 * If the service is in the process of stopping, it will wait for it to finish
 * stopping.
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_SERVICE_UNRESPONSIVE the service did not respond to requests to stop
 * @retval LW_ERROR_DEPENDENT_SERVICE_STILL_RUNNING the service cannot be stopped as another running service depends on it
 * @retval LW_ERROR_INVALID_SERVICE_TRANSITION the service cannot be started in its present state
 */
DWORD
LwSmStopService(
    LW_SERVICE_HANDLE hHandle
    );
   
/**
 * @brief Get service status
 *
 * Gets the current status of the service represented by the given service handle.
 * 
 * @param[in] hHandle the service handle
 * @param[out] pStatus the status of the service
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmGetServiceStatus(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_STATUS pStatus
    );

/**
 * @brief Refresh service
 *
 * Refreshes the service represented by the given service handle, which typically
 * entails it reloading its configuration.
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmRefreshService(
    LW_SERVICE_HANDLE hHandle
    );

/**
 * @brief Get service info
 *
 * Gets the service info structore of the service represented by the given
 * service handle.  The structure should be freed with #LwSmFreeServiceInfo()
 * when done.
 * 
 * @param[in] hHandle the service handle
 * @param[out] ppInfo the info structure for the service
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmGetServiceInfo(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_INFO* ppInfo
    );

/**
 * @brief Get recursive dependency list
 *
 * Gets a list of all recursive dependencies of the service represented
 * by the given service handle -- that is, all services the given service
 * depends on directly or indirectly.  The entries in the list will be in the
 * order in which they would need to be started in order to start the
 * given service.  The list should be freed with #LwSmFreeServiceNameList()
 * when done.
 *
 * @param[in] hHandle the service handle
 * @param[out] pppwszServiceList the service dependency list
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmGetServiceDependencyClosure(
    LW_SERVICE_HANDLE hHandle,
    PWSTR** pppwszServiceList
    );

/**
 * @brief Get recursive reverse dependency list
 *
 * Gets a list of all recursive reverse dependencies of the service represented
 * by the given service handle -- that is, all services which depend on the
 * given service directly or indirectly.  The entries in the list will be in the
 * order in which they would need to be stopped in order to stop the
 * given service.  The list should be freed with #LwSmFreeServiceNameList()
 * when done.
 *
 * @param[in] hHandle the service handle
 * @param[out] pppwszServiceList the service reverse dependency list
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmGetServiceReverseDependencyClosure(
    LW_SERVICE_HANDLE hHandle,
    PWSTR** pppwszServiceList
    );

/**
 * @brief Free service info structure
 *
 * Frees a service info structure as returned by e.g.
 * #LwSmGetServiceInfo()
 *
 * @param[in,out] pInfo the service info structure to free
 */
VOID
LwSmFreeServiceInfo(
    PLW_SERVICE_INFO pInfo
    );

/*@}*/

#endif
