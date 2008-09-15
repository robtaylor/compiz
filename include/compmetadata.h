#ifndef _COMPMETADATA_H
#define _COMPMETADATA_H

#include <vector>

#include <libxml/parser.h>

#include <compaction.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY (x)
#define MINTOSTRING(x) "<min>" TOSTRING (x) "</min>"
#define MAXTOSTRING(x) "<max>" TOSTRING (x) "</max>"
#define RESTOSTRING(min, max) MINTOSTRING (min) MAXTOSTRING (max)

class CompMetadata {
    public:
	struct OptionInfo {
	    const char           *name;
	    const char           *type;
	    const char           *data;
	    CompAction::CallBack initiate;
	    CompAction::CallBack terminate;
	};
    public:
	CompMetadata ();
        CompMetadata (CompString       plugin,
		      const OptionInfo *optionInfo = NULL,
		      unsigned int     nOptionInfo = 0);
	~CompMetadata ();

	std::vector<xmlDoc *> &doc ();

	bool addFromFile (CompString file);
	bool addFromString (CompString string);
	bool addFromIO (xmlInputReadCallback  ioread,
		        xmlInputCloseCallback ioclose,
		        void                  *ioctx);

	bool initOption (CompOption *option,
			 CompString name);

	bool initOptions (const OptionInfo   *info,
			  unsigned int       nOptions,
			  CompOption::Vector &options);



	CompString getShortPluginDescription ();

	CompString getLongPluginDescription ();

	CompString getShortOptionDescription (CompOption *option);

	CompString getLongOptionDescription (CompOption *option);
	
	CompString getStringFromPath (CompString path);

	static unsigned int readXmlChunk (const char   *src,
					  unsigned int *offset,
					  char         *buffer,
					  unsigned int length);

	static unsigned int
	readXmlChunkFromOptionInfo (const CompMetadata::OptionInfo *info,
				    unsigned int                   *offset,
				    char                           *buffer,
				    unsigned int                   length);

    private:
	CompString            mPath;
	std::vector<xmlDoc *> mDoc;
};

#endif
