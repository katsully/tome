#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include <iostream>
#include "cinder/gl/TextureFont.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "jsoncpp/json.h"
#include "twitcurl.h"

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
};

void GetHeadlinesApp::setup()
{
    gl::clear(Color(0, 0, 0));
    gl::enableAlphaBlending(false);
//    font = gl::TextureFont::create(Font(loadFile("acmesa.TTF"), 16));
    cout << "def working2" << endl;

    //Optional, i'm locked behind a corporate firewall, send help!
//    twit.setProxyServerIp(std::string("ip.ip.ip.ip"));
//    twit.setProxyServerPort(std::string("port"));
    
    //Obviously we'll replace these strings
    twit.getOAuth().setConsumerKey(std::string("R2lam0vepQjqSouiljoThU4fi"));
twit.getOAuth().setConsumerSecret(std::string("FbhZRfFNF3mLmnwWW1GHSdJUuZgMIkAFeErrn3x6aMeQvzmtER"));
    twit.getOAuth().setOAuthTokenKey(std::string("382795211-5EvULyAAxAcVUMBdKuVtMLJMq3uRgb4jdmZZqOVX"));
    twit.getOAuth().setOAuthTokenSecret(std::string("9K0wDoz9vWFr8vSFsFkxrwytf1aBoBJANFGlEZx2oqkPg"));
    
    //We like Json, he's a cool guy, but we could've used XML too, FYI.
    // not working
//    twit.setTwitterApiType(twitCurlTypes::eTwitCurlApiFormatJson);
    
    string resp;
    
    if(twit.accountVerifyCredGet())
    {
        twit.getLastWebResponse(resp);
        console() << resp << std::endl;
        if(twit.search(string("itp")))
        {
            twit.getLastWebResponse(resp);
            
            Json::Value root;
            Json::Reader json;
            bool parsed = json.parse(resp, root, false);
            
            if(!parsed)
            {
                console() << json.getFormattedErrorMessages() << endl;
            }
            else
            {
                const Json::Value results = root["results"];
                for(int i=0;i<results.size();++i)
                {
                    temp.clear();
                    const string content = results[i]["text"].asString();
                    temp = split(content, ' ');
                    words.insert(words.end(), temp.begin(), temp.end());
                }
            }
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
}

CINDER_APP( GetHeadlinesApp, RendererGl )
