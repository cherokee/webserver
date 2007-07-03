/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Cherokee
 *
 * Authors:
 *      Ross Smith II <cherokee at smithii.com>
 *
 *  Copyright (C) 2002-2005 OpenVPN Solutions LLC <info@openvpn.net>
 *  Copyright (C) 2007 Ross Smith II
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This code is designed to be built with the mingw compiler.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "win32_cservice.h"

/* bool definitions */
#define bool int
#define true 1
#define false 0

/* These are new for 2000/XP, so they aren't in the mingw headers yet */
#ifndef BELOW_NORMAL_PRIORITY_CLASS
# define BELOW_NORMAL_PRIORITY_CLASS 0x00004000
#endif
#ifndef ABOVE_NORMAL_PRIORITY_CLASS
# define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif

struct security_attributes
{
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;
};

/*
 * This event is initially created in the non-signaled
 * state.  It will transition to the signaled state when
 * we have received a terminate signal from the Service
 * Control Manager which will cause an asynchronous call
 * of ServiceStop below.
 */
#define EXIT_EVENT_NAME "cherokee_exit_1"

/*
 * Which registry key in HKLM should
 * we get config info from?
 */
#define REG_KEY "SOFTWARE\\Cherokee Project"

static HANDLE exit_event = NULL;

/* clear an object */
#define CLEAR(x) memset(&(x), 0, sizeof(x))

/* snprintf with guaranteed null termination */
#define mysnprintf(out, args...) \
{ \
	snprintf(out, sizeof(out), args); \
	out [sizeof (out) - 1] = '\0'; \
}

/*
 * Message handling
 */
#define M_INFO    (0)                                  // informational
#define M_SYSERR  (MSG_FLAGS_ERROR|MSG_FLAGS_SYS_CODE) // error + system code
#define M_ERR     (MSG_FLAGS_ERROR)                    // error

/* write error to event log */
#define MSG(flags, args...) \
{ \
	char x_msg[256]; \
	mysnprintf (x_msg, args); \
	AddToMessageLog ((flags), x_msg); \
}

/* get a registry string */
#define QUERY_REG_STRING(name, data) \
{ \
	len = sizeof (data); \
	status = RegQueryValueEx (Cherokee_key, name, NULL, &type, data, &len); \
	if (status != ERROR_SUCCESS || type != REG_SZ) \
	{ \
		SetLastError (status); \
		MSG (M_SYSERR, error_format_str, name); \
		RegCloseKey (Cherokee_key); \
		goto finish; \
	} \
}

/* get a registry string */
#define QUERY_REG_DWORD(name, data) \
{ \
	len = sizeof (DWORD); \
	status = RegQueryValueEx (Cherokee_key, name, NULL, &type, (LPBYTE)&data, &len); \
	if (status != ERROR_SUCCESS || type != REG_DWORD || len != sizeof (DWORD)) \
	{ \
		SetLastError (status); \
		MSG (M_SYSERR, error_format_dword, name); \
		RegCloseKey (Cherokee_key); \
		goto finish; \
	} \
}

bool
init_security_attributes_allow_all (struct security_attributes *obj)
{
	CLEAR (*obj);

	obj->sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	obj->sa.lpSecurityDescriptor = &obj->sd;
	obj->sa.bInheritHandle = TRUE;
	if (!InitializeSecurityDescriptor (&obj->sd, SECURITY_DESCRIPTOR_REVISION))
		return false;
	if (!SetSecurityDescriptorDacl (&obj->sd, TRUE, NULL, FALSE))
		return false;
	return true;
}

HANDLE
create_event (const char *name, bool allow_all, bool initial_state, bool manual_reset)
{
	if (allow_all)
	{
		struct security_attributes sa;
		if (!init_security_attributes_allow_all (&sa))
			return NULL;
		return CreateEvent (&sa.sa, (BOOL)manual_reset, (BOOL)initial_state, name);
	}
	else
		return CreateEvent (NULL, (BOOL)manual_reset, (BOOL)initial_state, name);
}

int
close_if_open (HANDLE h)
{
	return (h != NULL) ? CloseHandle (h) : 1;
}

VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
	char append_string[2];
	char config_file[MAX_PATH];
	char exe_path[MAX_PATH];
	char log_path[MAX_PATH];
	char priority_string[64];
	char startup_options[MAX_PATH];
	char startup_directory[MAX_PATH];

	DWORD priority;
	bool append;

	ResetError ();

	if (!ReportStatusToSCMgr (SERVICE_START_PENDING, NO_ERROR, 3000))
	{
		MSG (M_ERR, "ReportStatusToSCMgr #1 failed");
		goto finish;
	}

	/*
	* Create our exit event
	*/
	exit_event = create_event (EXIT_EVENT_NAME, false, false, true);
	if (!exit_event)
	{
		MSG (M_ERR, "CreateEvent failed");
		goto finish;
	}

	/*
	* If exit event is already signaled, it means we were not
	* shut down properly.
	*/
	if (WaitForSingleObject (exit_event, 0) != WAIT_TIMEOUT)
	{
		MSG (M_ERR, "Exit event is already signaled -- we were not shut down properly");
		goto finish;
	}

	if (!ReportStatusToSCMgr (SERVICE_START_PENDING, NO_ERROR, 3000))
	{
		MSG (M_ERR, "ReportStatusToSCMgr #2 failed");
		goto finish;
	}

	/*
	* Read info from registry in key HKLM\SOFTWARE\Cherokee
	*/
	{
		HKEY Cherokee_key;
		LONG status;
		DWORD len;
		DWORD type;
		char error_string[256];

		static const char error_format_str[] =
		"Error querying registry key of type REG_SZ: HKLM\\" REG_KEY "\\%s";

		static const char error_format_dword[] =
		"Error querying registry key of type REG_DWORD: HKLM\\" REG_KEY "\\%s";

		status = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			REG_KEY,
			0,
			KEY_READ,
			&Cherokee_key);

		if (status != ERROR_SUCCESS)
		{
			SetLastError (status);
			MSG (M_SYSERR, "Registry key HKLM\\" REG_KEY " not found");
			goto finish;
		}

		/* get path to configuration directory */
		QUERY_REG_STRING ("config_file", config_file);

		/* get path to Cherokee.exe */
		QUERY_REG_STRING ("exe_path", exe_path);

		/* should we truncate or append to logfile? */
		QUERY_REG_STRING ("log_append", append_string);

		/* get path to log directory */
		QUERY_REG_STRING ("log_path", log_path);

		/* get priority for spawned Cherokee subprocesses */
		QUERY_REG_STRING ("priority", priority_string);

		/* get startup directory */
		QUERY_REG_STRING ("startup_directory", startup_directory);

		/* startup options, -r or -b */
		QUERY_REG_STRING ("startup_options", startup_options);

		RegCloseKey (Cherokee_key);
	}

	/* set process priority */
	priority = NORMAL_PRIORITY_CLASS;
	if (!strcasecmp (priority_string, "IDLE_PRIORITY_CLASS"))
		priority = IDLE_PRIORITY_CLASS;
	else if (!strcasecmp (priority_string, "BELOW_NORMAL_PRIORITY_CLASS"))
		priority = BELOW_NORMAL_PRIORITY_CLASS;
	else if (!strcasecmp (priority_string, "NORMAL_PRIORITY_CLASS"))
		priority = NORMAL_PRIORITY_CLASS;
	else if (!strcasecmp (priority_string, "ABOVE_NORMAL_PRIORITY_CLASS"))
		priority = ABOVE_NORMAL_PRIORITY_CLASS;
	else if (!strcasecmp (priority_string, "HIGH_PRIORITY_CLASS"))
		priority = HIGH_PRIORITY_CLASS;
	else
	{
		MSG (M_ERR, "Unknown priority name: %s", priority_string);
		priority = NORMAL_PRIORITY_CLASS;
	//      goto finish;
	}

	/* set log file append/truncate flag */
	append = false;
	if (append_string[0] == '0')
		append = false;
	else if (append_string[0] == '1')
		append = true;
	else
	{
		MSG (M_ERR, "Log file append flag (given as '%s') must be '0' or '1'", append_string);
		append = false;
		// goto finish;
	}

	/*
	* Instantiate an Cherokee process for each configuration
	* file found.
	*/
	{
		HANDLE log_handle = NULL;
		STARTUPINFO start_info;
		PROCESS_INFORMATION proc_info;
		struct security_attributes sa;
		char command_line[256];

		CLEAR (start_info);
		CLEAR (proc_info);
		CLEAR (sa);

		if (!ReportStatusToSCMgr (SERVICE_START_PENDING, NO_ERROR, 3000))
		{
			MSG (M_ERR, "ReportStatusToSCMgr #3 failed");
			goto finish;
		}

		/* construct command line */
		mysnprintf (command_line, "\"%s\" -C \"%s\" %s",
			exe_path,
			config_file,
			startup_options);

		/* Make security attributes struct for logfile handle so it can
		be inherited. */
		if (!init_security_attributes_allow_all (&sa))
		{
			MSG (M_SYSERR, "InitializeSecurityDescriptor start_cherokee failed");
			goto finish;
		}

		if (log_path && *log_path) {
			/* open logfile as stdout/stderr for soon-to-be-spawned subprocess */
			log_handle = CreateFile (log_path,
				GENERIC_WRITE,
				FILE_SHARE_READ,
				&sa.sa,
				append ? OPEN_ALWAYS : CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (log_handle == INVALID_HANDLE_VALUE)
			{
				MSG (M_SYSERR, "Cannot open logfile: %s", log_path);
				goto finish;
			}

			/* append to logfile? */
			if (append)
			{
				if (SetFilePointer (log_handle, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
				{
					MSG (M_SYSERR, "Cannot seek to end of logfile: %s", log_path);
					goto finish;
				}
			}
		}

		/* fill in STARTUPINFO struct */
		GetStartupInfo(&start_info);
		start_info.cb = sizeof(start_info);
		start_info.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
		start_info.wShowWindow = SW_HIDE;
		start_info.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
		start_info.hStdOutput = start_info.hStdError = log_handle;

		/* create an Cherokee process for one config file */
		if (!CreateProcess (
			exe_path,
			command_line,
			NULL,
			NULL,
			TRUE,
			priority | CREATE_NEW_CONSOLE,
			NULL,
			startup_directory,
			&start_info,
			&proc_info))
		{
			MSG (M_SYSERR, "CreateProcess failed, exe='%s' cmdline='%s'",
				exe_path,
				command_line);

			close_if_open (log_handle);
			goto finish;
		}

		/* close unneeded handles */
		Sleep (1000); /* try to prevent race if we close logfile
		handle before child process DUPs it */
		if (!CloseHandle (proc_info.hProcess)
			|| !CloseHandle (proc_info.hThread)
			|| !close_if_open (log_handle)
			)
		{
			MSG (M_SYSERR, "CloseHandle failed");
			goto finish;
		}
	}

	/* we are now fully started */
	if (!ReportStatusToSCMgr (SERVICE_RUNNING, NO_ERROR, 0))
	{
		MSG (M_ERR, "ReportStatusToSCMgr SERVICE_RUNNING failed");
		goto finish;
	}

	/* wait for our shutdown signal */
	if (WaitForSingleObject (exit_event, INFINITE) != WAIT_OBJECT_0)
	{
		MSG (M_ERR, "wait for shutdown signal failed");
	}

finish:
	ServiceStop ();
	close_if_open (exit_event);
}

VOID ServiceStop()
{
	if (exit_event)
		SetEvent(exit_event);
}
