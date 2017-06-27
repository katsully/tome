#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include <iostream>
#include "cinder/gl/TextureFont.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "jsoncpp/json.h"
#include "twitcurl.h"
#include "cinder/params/Params.h"

#include <fstream>
#include <regex>

using namespace ci;
using namespace ci::app;
using namespace std;

class GetHeadlinesApp : public App {
  public:
    //Optional for setting app size
    void prepareSettings(Settings* settings);
    
	void setup() override;
	void update() override;
	void draw() override;
    
    void getTweets(string &resp);
    
    //We'll parse our twitter content into these
    vector<string> temp;
    vector<string> words;
    vector<string> names;
    
    //For drawing our text
    gl::TextureFont::DrawOptions fontOpts;
    gl::TextureFontRef mFont;
    
    // TODO ADD params
    // TODO add text box to search for particular term from networks we are already searching
    params::InterfaceGl	mParams;
    
    twitCurl twit;
    vector<std::string> tweets;
    
    string keys[4];     // hold keys for Twitter API oauth
};

void GetHeadlinesApp::prepareSettings(Settings* settings)
{
    settings->setWindowSize(1280, 720);
    cout << "def working1" << endl;
}

void GetHeadlinesApp::setup()
{
    setFullScreen(true);
    gl::clear(Color(0, 0, 0));
    gl::enableAlphaBlending(false);
    
    // TODO - change fonts
    mFont = gl::TextureFont::create(Font( "Times New Roman", 32 ));
    
    // TODO -  tv networks & donald trump
    names = {"realDonaldTrump", "FoxNews"};
    
    // TODO - list of keywords to futher curate tweets
    // TODO - get lots more tweets
    
    int count = 0;
    string line;
    ifstream myfile ("keys.txt");
    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            keys[count] = line;
            count++;
        }
        myfile.close();
    }
    else cout << "Unable to open file";
    
    twit.getOAuth().setConsumerKey(keys[0]);
    twit.getOAuth().setConsumerSecret(keys[1]);
    twit.getOAuth().setOAuthTokenKey(keys[2]);
    twit.getOAuth().setOAuthTokenSecret(keys[3]);
    
    string resp;
    
    if(twit.accountVerifyCredGet())
    {
        // TODO - make go through ea networks timeline instead?
        if(twit.userLookup(names, false)){
            twit.getLastWebResponse(resp);
            Json::Value root;
            Json::Reader json;
            bool parsed = json.parse(resp, root, false);
            
            if(!parsed) {
                console() << json.getFormattedErrorMessages() << endl;
            } else {
                const Json::Value results = root;
                for(auto s: root)
                {
                    // logo and then tweet
                    std::string t = s["status"]["text"].asString();
                    // TODO - this isn't perfect test with .@SenSchumer: "Senate Republican healthcare bill is a wolf in sheep's clothing, only this wolf has even sharper teeth than the House bill."
                    size_t end = t.find("http");
                    string editedTweet = t.substr(0, end);
                    cout << editedTweet << endl;
                    tweets.push_back(editedTweet);
                }
            }
        }
        // search with query
//        if(twit.search("Donald")){
//            twit.getLastWebResponse(resp);
//            cout << resp << endl;
//        } else {
//            cout << "DID NOT" << endl;
//            twit.getLastCurlError(resp);
//            console() << resp << endl;
//        }
    }
    else
    {
        twit.getLastCurlError(resp);
        console() << resp << endl;
    }
    
}

void GetHeadlinesApp::getTweets(string &resp)
{
    
}

void GetHeadlinesApp::update()
{
}

void GetHeadlinesApp::draw()
{
    // TODO - get flag image for reference
    gl::clear( Color( 0, 0, 0 ) );
    int counter = 25;
    
    // TODO - black text on white & white text on red
    // TODO - scrolling, ea line is one network
    // TODO - fit into height & width via andrew
    // TODO - send to Syphon
    // TODO - Syphon to isadora
    // TODO - trump put him on bottom
    for(std::string s : tweets) {
        gl::drawString(s, vec2(10, counter));
        counter+=25;
    }
}

// TODO - clickable app that can work on any comp
// TODO - app will need params that can be edited easily

CINDER_APP( GetHeadlinesApp, RendererGl )
