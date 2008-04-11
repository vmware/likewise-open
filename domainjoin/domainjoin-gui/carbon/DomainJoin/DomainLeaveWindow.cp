/*
 *  DomainLeaveWindow.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainLeaveWindow.h"
#include "DomainJoinInterface.h"
#include "DomainJoinException.h"

const int DomainLeaveWindow::COMPUTER_NAME_ID = 304;
const int DomainLeaveWindow::DOMAIN_NAME_ID   = 306;
const int DomainLeaveWindow::LEAVE_ID         = 308;
const int DomainLeaveWindow::CLOSE_ID         = 309;

const int DomainLeaveWindow::LEAVE_CMD_ID     = 'leav';
const int DomainLeaveWindow::CLOSE_CMD_ID     = 'not!';
		
//--------------------------------------------------------------------------------------------
Boolean
DomainLeaveWindow::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
        case CLOSE_CMD_ID:
			this->Close();
			break;
        			
		case LEAVE_CMD_ID:
		    HandleLeaveDomain();
			break;
			
        default:
            return false;
    }
	
	return true;
}

void
DomainLeaveWindow::SetComputerName(const std::string& name)
{
    OSStatus err = SetLabelControlString(COMPUTER_NAME_ID, name);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set computer name in control");
	   throw DomainJoinException(errMsg);
	}
}

void
DomainLeaveWindow::SetDomainName(const std::string& name)
{
    OSStatus err = SetLabelControlString(DOMAIN_NAME_ID, name);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set domain name in control");
	   throw DomainJoinException(errMsg);
	}
}

std::string
DomainLeaveWindow::GetComputerName()
{
	std::string result;
    OSStatus err = GetLabelControlString(COMPUTER_NAME_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get computer name from control");
	   throw DomainJoinException(errMsg);
	}
	
	return result;
}

std::string
DomainLeaveWindow::GetDomainName()
{
    std::string result;
    OSStatus err = GetLabelControlString(DOMAIN_NAME_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get domain name from control");
	   throw DomainJoinException(errMsg);
	}
	
	return result;
}

bool
DomainLeaveWindow::ConfirmLeave(const std::string& domainName)
{
    AlertStdCFStringAlertParamRec params;
	DialogRef dialog;
	OSStatus err = noErr;
	DialogItemIndex itemHit;
	CFStringRef msgStrRef = NULL;
	
	GetStandardAlertDefaultParams(&params, kStdCFStringAlertVersionOne);
	
	params.movable = true;
	params.defaultText = CFSTR("Yes");
	params.cancelText = CFSTR("No");
	params.otherText = NULL;
	params.defaultButton = kAlertStdAlertCancelButton;
	params.position = kWindowCenterOnParentWindow;
	
	msgStrRef = CFStringCreateWithFormat(NULL, NULL, CFSTR("Are you sure you want to leave the %s domain?"), domainName.c_str());
	
	err = CreateStandardAlert(kAlertStopAlert,
	                          CFSTR("Likewise Enterprise"),
							  msgStrRef,
							  &params,
							  &dialog);
	if (err == noErr)
	{
	   err = RunStandardAlert(dialog, NULL, &itemHit);
	   if (err != noErr)
	   {
	      throw DomainJoinException("Failed to display an alert", err);
	   }
	}
	else
	{
	   throw DomainJoinException("Failed to create dialog", err);
	}
	
	if (msgStrRef)
	{
	   CFRelease(msgStrRef);
	}
	
	return itemHit != 2;
}

void
DomainLeaveWindow::HandleLeaveDomain()
{
	try
	{
	    std::string computerName = GetComputerName();
		std::string domainName = GetDomainName();
		
		if (!ConfirmLeave(domainName))
		   return;
		
		setuid(0);
		
		DomainJoinInterface::LeaveDomain();
		PostApplicationEvent(MAIN_MENU_JOIN_OR_LEAVE_ID);
	}
	catch(DomainJoinException& dje)
    {
        SInt16 outItemHit;
        char msgStr[256];
        sprintf(msgStr, "An Error occurred when joining Active Directory. Error code [%.8x]", dje.GetErrorCode());
        CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
		CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
        StandardAlert(kAlertStopAlert,
                      "\pDomain Join Error",
                      (StringPtr)msgStr,
                      NULL,
                      &outItemHit);
    }
    catch(...)
    {
        SInt16 outItemHit;
        StandardAlert(kAlertStopAlert,
                      "\pUnexpected error",
                      "\pAn unexpected error occurred when joining the Active Directory domain. Please report this to Likewise Technical Support at support@likewisesoftware.com",
                      NULL,
                      &outItemHit);
    }
}

void
DomainLeaveWindow::Close()
{
    QuitApplicationEventLoop();
    TWindow::Close();
}
