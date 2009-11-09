#include <StorageKit.h> 
#include <KernelKit.h>
#include <Alert.h>
#include <Roster.h>
#include <TrackerAddOn.h>

#include <stdio.h> 
#include <string.h> 
#include <unistd.h>
#include <errno.h>
#include <String.h>

#include <vector>

BString load_addon_data(bool*);

#define TERMINAL_SIGNATURE "application/x-vnd.Haiku-Terminal"

extern char **environ;

void process_refs(entry_ref dir_ref, BMessage *msg, void *)
{
	try {
		bool terminal;
		BString script = load_addon_data(&terminal);
		
		int32 refs; type_code type;
		msg->GetInfo("refs",&type,&refs);
		vector<BPath> paths(refs);
		
		// set up argv
		size_t argc=0;
		vector<const char *> argv(refs+6);
		BPath terminalPath;
		if (terminal) 
		{
			entry_ref terminal_ref;
			be_roster->FindApp(TERMINAL_SIGNATURE, &terminal_ref);
			terminalPath.SetTo(&terminal_ref);
			argv[argc++]=terminalPath.Path();			
		}		
			
		argv[argc++]="/bin/sh";
		argv[argc++]="-c";
		argv[argc++]=script.String();
		argv[argc++]="--";
		
		entry_ref ref;
		BEntry entry;
		for (size_t n=0; msg->FindRef("refs", n, &ref) == B_OK; n++) {
			entry.SetTo(&ref);
			entry.GetPath(&paths[n]);
			argv[argc++]=paths[n].Path();
		}
				
		// set the current directory/keep track of old
		BPath wd, pwd(getcwd(NULL,0));
		entry.SetTo(&dir_ref);
		entry.GetPath(&wd);
		chdir(wd.Path());

		// set up dummy stdin
		int oldin=dup(0);
		close(0);
		open("/dev/null",O_RDONLY);

		// load the command
		argv[argc]=NULL;
		thread_id tid=load_image(argc,argv.begin(),(const char**)environ);
		
		// restore stdin, pwd
		dup2(oldin,0);
		close(oldin);
		chdir(pwd.Path());
		
		if (tid<0) 
			throw strerror(tid);
		else // if all is well, proceed
			resume_thread(tid);
	
	} catch (const char *error) {
		BAlert *alert=new BAlert(
			"TrackerScript error", error, "Phoey!",
			0,0,	B_WIDTH_AS_USUAL,B_STOP_ALERT);
		alert->Go();
	}
}


BString load_addon_data(bool *terminal)
{
	// find our entry
	BString script;
	status_t ret;
	int32 cookie=0;
	image_info info;
	void *ptr;
	
	while (get_next_image_info(0,&cookie,&info) == B_OK) {
		ret=get_image_symbol(info.id, "process_refs",
			B_SYMBOL_TYPE_TEXT,&ptr);
		if (ret==B_OK && ptr==(void *)process_refs)
			break;
	}
	
	 BNode this_node(info.name);
	 ret=this_node.InitCheck();
	 if (ret!=B_OK) return NULL;
		
	time_t mtime;
	static time_t last_modified=0;
	this_node.GetModificationTime(&mtime);
	
	if (mtime>last_modified || last_modified==0) {
		attr_info info;
		status_t ret;
		
		// load the script
		ret=this_node.GetAttrInfo("script",&info);
		if (ret==B_OK && info.type==B_STRING_TYPE) {
			char* s = new char[info.size+1];
			this_node.ReadAttr("script", B_STRING_TYPE, 0, s, info.size);
			script.SetTo(s);
			delete[] s;
		} else script.SetTo("");
		
		// load whether we want output
		ret=this_node.GetAttrInfo("terminal",&info);
		if (ret==B_OK && info.type==B_INT32_TYPE) {
			int32 b;
			this_node.ReadAttr("terminal", B_INT32_TYPE, 0, &b,sizeof(b));
			*terminal=b;
		} else *terminal=false;
		
		last_modified=mtime;
	}
	
	return script;
}

