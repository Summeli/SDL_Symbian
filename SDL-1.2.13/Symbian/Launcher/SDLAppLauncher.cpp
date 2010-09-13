#include <SDLAppLauncher.rsg>
#include <apgcli.h>
#include <eikdll.h>
#include <apgtask.h>
#include <eikapp.h>
#include <eikenv.h>
#include "SDLLauncher.hrh"
class CSDLAppLauncher : public CEikApplication {
public:
	CSDLAppLauncher();
	~CSDLAppLauncher();

	CApaDocument *CreateDocumentL();

	TUid AppDllUid() const
	{
		return TUid::Uid(KSDLAppLauncherUid);
	}
};


#include <eikdoc.h>

class CSDLAppLauncherDoc : public CEikDocument {
public:
	CSDLAppLauncherDoc(CEikApplication &aApplicaiton);
	~CSDLAppLauncherDoc();

	CEikAppUi *CreateAppUiL();
	void ConstructL();
};

#include <eikappui.h>
class CSDLAppLauncherUi;
class CScummWatcher : public CActive {
public:
	CScummWatcher();
	~CScummWatcher();

	void DoCancel();
	void RunL();
	CSDLAppLauncherUi *iAppUi;
};

class CSDLAppLauncherUi : public CEikAppUi {
public:
	CSDLAppLauncherUi();
	~CSDLAppLauncherUi();

	void ConstructL();
	void HandleCommandL(TInt aCommand);
	void HandleForegroundEventL(TBool aForeground);
	void BringUpEmulatorL();

private:
	TThreadId iThreadId;
	TInt iExeWgId;
	RThread iThreadWatch;
	CScummWatcher *iWatcher;
};

EXPORT_C CApaApplication *NewApplication() {
	return (new CSDLAppLauncher);
}

CSDLAppLauncher::CSDLAppLauncher() {
}

CSDLAppLauncher::~CSDLAppLauncher() {
}

CApaDocument *CSDLAppLauncher::CreateDocumentL() {
	return new (ELeave)CSDLAppLauncherDoc(*this);
}


CSDLAppLauncherDoc::CSDLAppLauncherDoc(CEikApplication &aApp) : CEikDocument(aApp) {
}

CSDLAppLauncherDoc::~CSDLAppLauncherDoc() {
}

CEikAppUi *CSDLAppLauncherDoc::CreateAppUiL() {
	return new (ELeave)CSDLAppLauncherUi;
}

void CSDLAppLauncherUi::HandleForegroundEventL(TBool aForeground) {
	if(aForeground) {
		BringUpEmulatorL();
	}
}

CSDLAppLauncherUi::CSDLAppLauncherUi() {
}

CSDLAppLauncherUi::~CSDLAppLauncherUi() {
	if(iWatcher) {
		iThreadWatch.LogonCancel(iWatcher->iStatus);
		iWatcher->Cancel();
	}

	delete iWatcher;

	iThreadWatch.Close();
}

void CSDLAppLauncherUi::ConstructL() {
	BaseConstructL();
	TBuf<128> startFile;
	startFile = iEikonEnv->EikAppUi()->Application()->AppFullName();
	TParse parser;
	parser.Set(startFile,NULL,NULL);

	startFile = parser.DriveAndPath();
	startFile.Append(parser.Name());
#ifndef __WINS__
	startFile.Append( _L(".exe"));
#else
	startFile.Append( _L(".dll"));
#endif
	CApaCommandLine *cmdLine = CApaCommandLine::NewLC(startFile);
	RApaLsSession lsSession;

	lsSession.Connect();
	CleanupClosePushL(lsSession);
	TInt error = lsSession.StartApp(*cmdLine, iThreadId);

	CleanupStack::PopAndDestroy();//close lsSession
	CleanupStack::PopAndDestroy(cmdLine);

	User::After(500000);// Let the application start

	TApaTaskList taskList(iEikonEnv->WsSession());

	TApaTask myTask = taskList.FindApp(TUid::Uid(KSDLAppLauncherUid));
	myTask.SendToBackground();

	TApaTask exeTask = taskList.FindByPos(0);

	iExeWgId=exeTask.WgId();
	exeTask.BringToForeground();

	if(iExeWgId == myTask.WgId()) { // Should n't be the same
		Exit();
	}
	if(iThreadWatch.Open(iThreadId) == KErrNone) {
		iWatcher = new (ELeave)CScummWatcher;
		iWatcher->iAppUi = this;
		iThreadWatch.Logon(iWatcher->iStatus);
	}
}

CScummWatcher::CScummWatcher() : CActive(EPriorityStandard) {
	CActiveScheduler::Add(this);

	iStatus = KRequestPending;
	SetActive();
}

CScummWatcher::~CScummWatcher() {
}

void CScummWatcher::DoCancel() {
}

void CScummWatcher::RunL() {
	iAppUi->HandleCommandL(EEikCmdExit);
}

void CSDLAppLauncherUi::BringUpEmulatorL() {
	RThread thread;

	if(thread.Open(iThreadId) == KErrNone) {
		thread.Close();
		TApaTask apaTask(iEikonEnv->WsSession());
		apaTask.SetWgId(iExeWgId);
		apaTask.BringToForeground();
	} else {
		iExeWgId = -1;
		Exit();
	}
}

void CSDLAppLauncherUi::HandleCommandL(TInt aCommand) {
	switch(aCommand) {
	case EEikCmdExit:
		{
			RThread thread;
			if(thread.Open(iThreadId) == KErrNone) {
				thread.Terminate(0);
				thread.Close();
			}
			Exit();
		}
		break;
	}
}

GLDEF_C  TInt E32Dll(TDllReason) {
	return KErrNone;
}
