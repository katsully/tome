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
	void setup() override;
	void update() override;
	void draw() override;
    
    void getTweets(string &resp);
    
    // background image
    gl::TextureRef mBackground;
    // star image
    gl::TextureRef mStars;
    // logos
    vector<gl::TextureRef> mLogos;
    
    //We'll parse our twitter content into these
    std::string tempKeyword;
    vector<std::string> mKeywords;
    vector<std::string> mAccounts;
    
    //For drawing our text
    Font mFont;
    gl::TextureFontRef	mTextureFont;
    
    params::InterfaceGlRef mParams;
    bool mShowParams = false;
    
    twitCurl twit;
    vector<vector<string>> mTweets;
    
    // hold keys for Twitter API oauth
    string keys[4];
    
    int stripeHeight;
    int widthPos = 0;
    int widthPosOffset = 0;
};

void GetHeadlinesApp::setup()
{
    setFullScreen(true);
    
    stripeHeight = getWindowHeight()/13;

    gl::clear(Color(0, 0, 0));
    gl::enableAlphaBlending(false);
    
    // load our flag image
    mBackground = gl::Texture::create( loadImage( loadAsset("flag.jpg") ) );
    // load just the stars
    mStars = gl::Texture::create( loadImage( loadAsset("flag2.jpg")));
    
    // load logos
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("fox.jpg"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("cnn.jpg"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("msnbc.jpg"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("abc.png"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("nbc.png"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("cbs.jpg"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("blaze.png"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("dailyshow.jpg"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("lastweek.png"))));
    mLogos.push_back(gl::Texture::create( loadImage( loadAsset("dt.jpg"))));
    
    // TODO -  tv networks & donald trump

    
    // read in accounts and keywords
    Json::Value root;
    std::ifstream inputFile("accounts.json");
    
    if (inputFile.is_open()) {
        inputFile >> root;
    
        for(auto a: root["accounts"]){
            mAccounts.push_back(a.asString());
        }
        for(auto k: root["keywords"]){
            mKeywords.push_back(k.asString());
        }
    
        inputFile.close();
    }
    else cout << "Unable to open json file";

    // Create the interface and give it a name
    mParams = params::InterfaceGl::create("App parameters", vec2(200,200));
    
    // Set up some basic parameters
    mParams->addParam( "New Keyword", &tempKeyword ).updateFn( [this] { mKeywords.push_back(tempKeyword);} );
    mParams->addParam("Show Params", &mShowParams).key("p");
    
    // Font used on news ticker for Fox
    mFont = Font( "Avenir", 36 );
    mTextureFont = gl::TextureFont::create( mFont );
    
    // TODO - shouldn't need a counter
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
    else cout << "Unable to open key file";
    
    twit.getOAuth().setConsumerKey(keys[0]);
    twit.getOAuth().setConsumerSecret(keys[1]);
    twit.getOAuth().setOAuthTokenKey(keys[2]);
    twit.getOAuth().setOAuthTokenSecret(keys[3]);
    
    string resp;
    
    if(twit.accountVerifyCredGet())
    {
        for(string a: mAccounts) {
            if(twit.timelineUserGet(true, false, 40, a)) {
                cout << a << endl;
                vector<string> temp;
                twit.getLastWebResponse(resp);
                Json::Value root;
                Json::Reader json;
                bool parsed = json.parse(resp, root, false);
                
                if(!parsed) {
                    console() << json.getFormattedErrorMessages() << endl;
                } else {
                    for(auto s: root)
                    {
                        std::string t = s["text"].asString();
                        // get rid of retweets
                        if (t.substr(0,2) == "RT") {
                            continue;
                        }
                        // TODO - add param to remove keyword query
                        
                        // TODO - this isn't perfect test with .@SenSchumer: "Senate Republican healthcare bill is a wolf in sheep's clothing, only this wolf has even sharper teeth than the House bill."
                        for(string k: mKeywords){
                            if (t.find(k) != std::string::npos) {
                                size_t end2 = t.find("http");
                                string editedTweet = t.substr(0, end2);
                                cout << editedTweet << endl;
                                temp.push_back(editedTweet);
                                break;
                            }
                        }
                    }
                }
                mTweets.push_back(temp);
            }
        }
    }
    else
    {
        twit.getLastCurlError(resp);
        console() << resp << endl;
    }
}

// TODO - make this respond to an update button in params
void GetHeadlinesApp::getTweets(string &resp)
{
}

void GetHeadlinesApp::update()
{
}

void GetHeadlinesApp::draw()
{
    gl::color(Color::white());
    gl::draw( mBackground, getWindowBounds() );
    int counter = 0;
    
    // TODO - fit into height & width via andrew
    // TODO - send to Syphon
    // TODO - Syphon to isadora
    // TODO - figure out how to calculate width of tweet (Sterling?)
    // TODO - bring in new logos & flag from Andew
    for(vector<string> s : mTweets) {
        (counter >= 7) ? widthPos = 10 : widthPos = getWindowWidth() / 2 - 20;
        for(string s1: s) {
            gl::color(Color::white());
            Rectf logoRect(widthPos-widthPosOffset, counter*stripeHeight+5, widthPos-widthPosOffset+50, counter*stripeHeight+stripeHeight);
            gl::draw(mLogos[counter], logoRect);
            (counter%2==0) ? gl::color( Color::white() ) : gl::color( Color::black() );
            mTextureFont->drawString(s1, vec2(widthPos-widthPosOffset+50, counter*stripeHeight+45));
            widthPos+=s1.length()*18;
        }
        counter++;
    }
    
    widthPosOffset+=2;
    gl::color(Color::white());
    Rectf drawRect( 0, 0, getWindowWidth()*.48, getWindowHeight()*.55 );
    gl::draw(mStars, drawRect);
    
    // Draw the interface
    if(mShowParams) { mParams->draw(); }
}

// TODO - clickable app that can work on any comp
// TODO - try with a quicktime block

CINDER_APP( GetHeadlinesApp, RendererGl, [&](App::Settings *settings) {
    
    // have the app run full screen in second monitor
    vector<DisplayRef> displays = Display::getDisplays();
    
    if (displays.size() > 1) {
        
        settings->setDisplay(displays[1]);
    }
})
