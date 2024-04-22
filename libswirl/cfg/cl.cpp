/*
	This file is part of libswirl
	
	Command line parsing
	~yay~

	Nothing too interesting here, really
*/
#include "license/bsd"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "cfg/cfg.h"

wchar* trim_ws(wchar* str)
{
	if (str==0 || strlen(str)==0)
		return 0;

	while(*str)
	{
		if (!isspace(*str))
			break;
		str++;
	}

	size_t l=strlen(str);
	
	if (l==0)
		return 0;

	while(l>0)
	{
		if (!isspace(str[l-1]))
			break;
		str[l-1]=0;
		l--;
	}

	if (l==0)
		return 0;

	return str;
}

int setconfig(wchar** arg,int cl)
{
	int rv=0;
	for(;;)
	{
		if (cl<1)
		{
			printf("-config : invalid number of parameters, format is section:key=value\n");
			return rv;
		}
		wchar* sep=strstr(arg[1],":");
		if (sep==0)
		{
			printf("-config : invalid parameter %s, format is section:key=value\n",arg[1]);
			return rv;
		}
		wchar* value=strstr(sep+1,"=");
		if (value==0)
		{
			printf("-config : invalid parameter %s, format is section:key=value\n",arg[1]);
			return rv;
		}

		*sep++=0;
		*value++=0;

		wchar* sect=trim_ws(arg[1]);
		wchar* key=trim_ws(sep);
		value=trim_ws(value);

		if (sect==0 || key==0)
		{
			printf("-config : invalid parameter, format is section:key=value\n");
			return rv;
		}

		const wchar* constval = value;
		if (constval==0)
			constval="";
		printf("Virtual cfg %s:%s=%s\n",sect,key,value);

		cfgSetVirtual(sect,key,value);
		rv++;

		if (cl>=3 && stricmp(arg[2],",")==0)
		{
			cl-=2;
			arg+=2;
			rv++;
			continue;
		}
		else
			break;
	}
	return rv;
}

int showhelp(wchar** argv,int cl)
{
    printf("\nUsage:\n");
    printf("  Load emulator GUI: %s\n", argv[0]);
    printf("  Directly load disc image: %s image_path.[cdi/gdi]\n\n", argv[0]);
    
	printf("Available commands:\n");

    printf("  -config section:key=value [, ..]\n");
    printf("      Add a virtual config value unless a different value is written to em\n");
    printf("      Note: You can specify many settings in the xx:yy=zz , gg:hh=jj , ... format.\n");
    printf("            The spaces between the values and ',' are needed.\n");
    printf("  -portable:\n");
    printf("      Look for data and discs in the current directory\n");
    printf("  -help:\n");
    printf("      Show the help info that you're reading now\n\n");

	return 0;
}

bool ParseCommandLine(int argc,wchar* argv[])
{
	cfgSetVirtual("config", "image", "");
	int cl=argc-2;
	wchar** arg=argv+1;
	while(cl>=0)
	{
		if (stricmp(*arg,"-help")==0 || stricmp(*arg,"--help")==0)
		{
			showhelp(argv,cl);
			return true;
		}
		else if (stricmp(*arg,"-config")==0 || stricmp(*arg,"--config")==0)
		{
			int as=setconfig(arg,cl);
			cl-=as;
			arg+=as;
		}
		else if (stricmp(*arg,"-portable")==0 || stricmp(*arg,"--portable")==0)
		{
			clear_dirs();

			//portable mode uses relative paths to CWD
			set_user_config_dir(".");
			set_user_data_dir(".");
			add_system_config_dir(".");
			add_system_data_dir(".");
		}
		else
		{
			char* extension = strrchr(*arg, '.');

			if (extension
				&& (stricmp(extension, ".cdi") == 0 || stricmp(extension, ".chd") == 0
					|| stricmp(extension, ".gdi") == 0 || stricmp(extension, ".lst") == 0
					|| stricmp(extension, ".cue") == 0))
			{
				printf("Using '%s' as cd image\n", *arg);
				cfgSetVirtual("config", "image", *arg);
			}
			else if (extension && stricmp(extension, ".elf") == 0)
			{
				printf("Using '%s' as reios elf file\n", *arg);
				cfgSetVirtual("config", "reios.enabled", "1");
				cfgSetVirtual("reios", "ElfFile", *arg);
			}
			else if (stricmp(*arg, "nodisk") == 0)
			{
				printf("Starting without cd image\n");
				cfgSetVirtual("config", "image", "nodisk");
			}
			else
			{
#if DC_PLATFORM == DC_PLATFORM_NAOMI || DC_PLATFORM == DC_PLATFORM_ATOMISWAVE
				printf("Using '%s' as rom\n", *arg);
				cfgSetVirtual("config", "image", *arg);
#else
				printf("wtf %s is supposed to do ?\n",*arg);
#endif
			}
		}
		arg++;
		cl--;
	}
	printf("\n");
	return false;
}
