#include <unistd.h>
#include <Application.h>
#include <InterfaceKit.h> 
#include <StorageKit.h>
#include <Roster.h>
#include <TrackerAddOn.h>
#include "TrackerScriptEditorWindow.h"

//..
#include <stdio.h>

class TrackerScript: public BApplication {
	
	BMessage refs;
	entry_ref ref;
	
  public:
	TrackerScript(): BApplication("application/x-vnd.Gargoyle-TrackerScript"){}
	
	virtual void ArgvReceived(int32 argc,char **argv)
	{
	
		if (argc < 2) return;
		
		while (--argc) {
			if (get_ref_for_path(argv[argc],&ref)==B_OK)
				refs.AddRef("refs",&ref);
		}
	}
	
	virtual void RefsReceived(BMessage *message) 
	{ 
		uint32 type; 
		int32 count; 
		
		message->GetInfo("refs", &type, &count); 
		if ( type != B_REF_TYPE ) 
			return; 
		
		for ( long i = --count; i >= 0; --i )
			if ( message->FindRef("refs", i, &ref) == B_OK )
				refs.AddRef("refs",&ref);
	}
	
	virtual void ReadyToRun()
	{
		
		if (refs.HasRef("refs")) 
		{
			get_ref_for_path(getcwd(NULL,0), &ref);
			process_refs(ref, &refs, NULL);
			PostMessage(B_QUIT_REQUESTED);
		} 
		else 
		{
			app_info info;
			GetAppInfo(&info);
			(new TrackerScriptEditorWindow(&info.ref))->Show();
		}
	}
	
	virtual void AboutRequested()
	{
		BAlert *alert=new BAlert(
			"",
			"TrackerScript\n\n"
			"This hybrid Application/Tracker addon executes a script (stored in "
			"attribute \"script\") with the selected files as arguments.  It "
			"optionally displays any output in a Terminal (set attribute \"terminal\" "
			"true).  When invoked as an Application with no arguments, it lets you "
			"edit these attributes.  (When invoked as an App with arguments or dropped "
			"files, it acts just like it does when invoked by the Tracker on those "
			"items.)\n"
			"\n"
			"Original developer:  Peter Folk (pfolk@uni.uiuc.edu) [Release 1.0.1 (05/14/1999)]"
			"\n\nPorted to Haiku: Andrea Anzani (andrea.anzani@gmail.com)\n"
			,
			"Got it!");
		alert->Go();
	}
};


int main(int,char **) 
{	TrackerScript().Run();
	return 0;
}
