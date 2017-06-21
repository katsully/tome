#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include <iostream>
#include "cinder/gl/TextureFont.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "jsoncpp/json.h"
#include "twitcurl.h"

#include <fstream>

// won't always need this - just for debugging
#include <typeinfo>

using namespace ci;
using namespace ci::app;
using namespace std;

class GetHeadlinesApp : public App {
  public:
    //Optional for setting app size
    void prepareSettings(Settings* settings);
    
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
    
    //We'll parse our twitter content into these
    vector<string> temp;
    vector<string> words;
    
    //For drawing our text
    gl::TextureFont::DrawOptions fontOpts;
    gl::TextureFontRef font;
    
    twitCurl twit;
    vector<std::string> tweets;
    
    string keys[4];     // hold keys for Twitter API oauth
};

void GetHeadlinesApp::setup()
{
    gl::clear(Color(0, 0, 0));
    gl::enableAlphaBlending(false);
    
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
        twit.getLastWebResponse(resp);
        vector<string> names = {"realDonaldTrump", "FoxNews"};
        if(twit.userLookup(names, false)){
            twit.getLastWebResponse(resp);
            std::cout << typeid(resp).name() << '\n';
            std::ofstream file("filename.json");
            file << resp;
            file.close();
            
            Json::Reader reader;
            Json::Value root;
            std::ifstream inputFile("filename.json");
            inputFile >> root;
            for(auto s : root){
                std::string t = "@" + s["screen_name"].asString() + ":" + s["status"]["text"].asString();
                tweets.push_back(t);
            }
            inputFile.close();
        }
    }
    else
    {
        twit.getLastCurlError(resp);
        console() << resp << endl;
    }
    
}

void GetHeadlinesApp::prepareSettings(Settings* settings)
{
    settings->setWindowSize(1280, 720);
    cout << "def working1" << endl;
}

void GetHeadlinesApp::mouseDown( MouseEvent event )
{
}

void GetHeadlinesApp::update()
{
}

void GetHeadlinesApp::draw()
{
    gl::clear( Color( 0, 0, 0 ) );
    int counter = 25;
    for(std::string s : tweets) {
        gl::drawString(s, vec2(10, counter));
        counter+=25;
    }
}

CINDER_APP( GetHeadlinesApp, RendererGl )
