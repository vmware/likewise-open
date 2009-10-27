/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Likewise.LMC.Services
{
    public class ServiceManagerApi
    {
        [Flags]
        public enum SERVICE_CONTROL : uint
        {
            STOP = 0x00000001,
            PAUSE = 0x00000002,
            CONTINUE = 0x00000003,
            INTERROGATE = 0x00000004,
            SHUTDOWN = 0x00000005,
            PARAMCHANGE = 0x00000006,
            NETBINDADD = 0x00000007,
            NETBINDREMOVE = 0x00000008,
            NETBINDENABLE = 0x00000009,
            NETBINDDISABLE = 0x0000000A,
            DEVICEEVENT = 0x0000000B,
            HARDWAREPROFILECHANGE = 0x0000000C,
            POWEREVENT = 0x0000000D,
            SESSIONCHANGE = 0x0000000E
        }

        public enum SERVICE_STATE : uint
        {
            SERVICE_STOPPED = 0x00000001,
            SERVICE_START_PENDING = 0x00000002,
            SERVICE_STOP_PENDING = 0x00000003,
            SERVICE_RUNNING = 0x00000004,
            SERVICE_CONTINUE_PENDING = 0x00000005,
            SERVICE_PAUSE_PENDING = 0x00000006,
            SERVICE_PAUSED = 0x00000007
        }

        [Flags]
        public enum SERVICE_ACCEPT : uint
        {
            STOP = 0x00000001,
            PAUSE_CONTINUE = 0x00000002,
            SHUTDOWN = 0x00000004,
            PARAMCHANGE = 0x00000008,
            NETBINDCHANGE = 0x00000010,
            HARDWAREPROFILECHANGE = 0x00000020,
            POWEREVENT = 0x00000040,
            SESSIONCHANGE = 0x00000080,
        }

        public enum SERVICE_CONFIGS
        {
            SERVICE_CONFIG_DESCRIPTION = 1,
            SERVICE_CONFIG_FAILURE_ACTIONS,
            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
            SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
            SERVICE_CONFIG_SERVICE_SID_INFO,
            SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO,
            SERVICE_CONFIG_PRESHUTDOWN_INFO,
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class SERVICE_FAILURE_ACTIONS
        {
            public int dwResetPeriod;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string lpRebootMsg;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string lpCommand;
            public int cActions;
            public IntPtr lpsaActions;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct SC_ACTION
        {
            public int Type;
            public int Delay;
        }

        public enum SC_ACTION_TYPE : int
        {
            SC_ACTION_NONE,
            SC_ACTION_RESTART,
            SC_ACTION_REBOOT,
            SC_ACTION_RUN_COMMAND
        }

        /**
        * @brief Service information
        *
        * Describes the basic information about a service,
        * such as its path, command line arguments, etc.
        */
        [StructLayout(LayoutKind.Sequential)]
        public struct LW_SERVICE_INFO
        {
            /** @brief Service short name */
            public string pwszName;
            /** @brief Service description */
            public string pwszDescription;
            /** @brief Service type */
            LW_SERVICE_TYPE type;
            /** @brief Path to service executable or module */
            public string pwszPath;
            /** @brief Arguments to service when started */
            public IntPtr ppwszArgs;
            /** @brief Names of services on which this service depends */
            IntPtr ppwszDependencies;
            /** @brief Is this service automatically started? */
            public bool bAutostart;
        }

        /**
        * @brief Service type
        *
        * Represents the type of a service
        */
        public enum LW_SERVICE_TYPE
        {
            /** Service is a legacy executable */
            LW_SERVICE_TYPE_LEGACY_EXECUTABLE = 0,
            /** Service is an executable that communicates with the service manager */
            LW_SERVICE_TYPE_EXECUTABLE = 1,
            /** Service is a module for a container */
            LW_SERVICE_TYPE_MODULE = 2,
            /** Service is a driver */
            LW_SERVICE_TYPE_DRIVER = 3
        }

        /**
        * @brief Service status
        *
        * Describes the runtime status of a service
        */
        public struct LW_SERVICE_STATUS
        {
            /** Brief Service state (stopped, running, etc.) */
            public LW_SERVICE_STATE state;
            /** Brief Service home */
            public LW_SERVICE_HOME home;
            /** Brief Process ID of service home */
            public  IntPtr pid;
        }

        /**
        * @brief State of a service
        *
        * Represents the state of a service (running, stopped, etc.)
        */
        public enum LW_SERVICE_STATE
        {
            /** @brief Service is running */
            LW_SERVICE_STATE_RUNNING = 0,
            /** @brief Service is stopped */
            LW_SERVICE_STATE_STOPPED = 1,
            /** @brief Service is starting */
            LW_SERVICE_STATE_STARTING = 2,
            /** @brief Service is stopping */
            LW_SERVICE_STATE_STOPPING = 3,
            /** @brief Service is paused */
            LW_SERVICE_STATE_PAUSED = 4,
            /** @brief Service is pining for the fjords */
            LW_SERVICE_STATE_DEAD = 5
        }

        /**
        * @brief Service home
        *
        * Denotes the location of a running service.
        */
        public enum LW_SERVICE_HOME
        {
            /** @brief Service is running in a standalone process */
            LW_SERVICE_HOME_STANDALONE,
            /** @brief Service is running in a service container */
            LW_SERVICE_HOME_CONTAINER,
            /** @brief Service is running in the IO manager */
            LW_SERVICE_HOME_IO_MANAGER,
            /** @brief Service is running directly in the service manager */
            LW_SERVICE_HOME_SERVICE_MANAGER
        }
    }    
}
