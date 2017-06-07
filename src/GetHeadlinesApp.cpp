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
