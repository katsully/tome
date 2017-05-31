##Installing Twitcurl
1. Cloned this [repo](https://github.com/swatkat/twitcurl)
2. Followed this [instructions](https://code.google.com/archive/p/twitcurl/wikis/WikiHowToUseTwitcurlLibrary.wiki)
 (sorta - here's how I did it) 
    1. Make sure you have GCC command line tools installed
    2. Cd into libtwitcurl
    3. run “make”
        * Got errors on running “make” unknown option -soname 
In the MakeFile I changed -soname to -install_name [source](https://stackoverflow.com/questions/4580789/ld-unknown-option-soname-on-os-x)
        * New error: ld: unknown option: -rpath-link=/usr/lib
Changed `LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib` to `LDFLAGS += -rpath $(STAGING_DIR)/usr/lib` [source](https://stackoverflow.com/questions/30825587/installing-twitcurl-on-os-x)
    4. cp libtwitcurl.so.1.0 /usr/lib/libtwitcurl.so
    5. cd twitterClient, g++ twitterClient.cpp -ltwitcurl
3. Followed these [instructions](http://protomatic.blogspot.com/2013/01/tutorial-getting-tweets-into-cinder.html)
    1. `#include AppBasic.h` becomes `#include "cinder/app/App.h"`
    2. `#include json/json.h` becomes `#include jsoncpp/json.h`
    3. In file twitcurl.h -> `#include “include/curl/curl.h”` becomes `#include “curl/curl.h”`
	4. Added new group to project and in that group added the lib libtwitcurl.so.1.0 [source](https://stackoverflow.com/questions/16078512/undefined-symbols-for-architecture-x86-64-error-when-linking-opencv-in-xcode)
   5. Comment out this line: `twit.setTwitterApiType(twitCurlTypes::eTwitCurlApiFormatJson);`
   6. Add `string resp;` before the if clause in setup
