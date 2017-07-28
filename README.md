## Installing Twitcurl
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
    4. cp libtwitcurl.so.1.0 /usr/lib/libtwitcurl.so.1
    5. cd twitterClient, g++ twitterClient.cpp -ltwitcurl
    	* Note: if you get ld: cannot find -ltwitcurl linker error, then copying libtwitcurl.so as libtwitcurl.so.1.0 into /usr/lib/ directory
    	* If that still doesn't work run xcode-select --install in the terminal
3. Followed these [instructions](http://protomatic.blogspot.com/2013/01/tutorial-getting-tweets-into-cinder.html)
    1. make lib folder in finder and add both lib files (see directions)
	2. Added new group to project and in that group added the lib libtwitcurl.so.1.0 [source](https://stackoverflow.com/questions/16078512/undefined-symbols-for-architecture-x86-64-error-when-linking-opencv-in-xcode)

   
## Other Notes
 There is a file keys.txt which reads in the Twitter API oauth keys, for security reasons this is not included in the repo. You will need to create this file and have in this order:
 	
 	1. consumer key
 	2. consumer secret
 	3. token key
 	4. token secret

 Have each key on a separate line, be sure there are no added characters (ie don't number each key).

### Adding text file to project

1. Select your project in XCode
2. Click on Build Phases
3. Click on the '+' button
4. Click on New Copy Files Build Phase
5. Select Product Directory
6. Click the '+' to add your file
7. Click Add Other
8. Select your keys.txt file (this file can be wherever) and click Open
9. Check Copy items... and click Create folder references... and hit finish

[source](https://stackoverflow.com/questions/23438393/new-to-xcode-cant-open-files-in-c)