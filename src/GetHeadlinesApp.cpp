#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/gl/TextureFont.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "jsoncpp/json.h"
#include "twitcurl.h"
#include "cinder/params/Params.h"
#include <fstream>
#include <regex>
#include "cinder/qtime/AvfWriter.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GetHeadlinesApp : public App {
  public:
	void setup() override;
    void keyDown(KeyEvent event) override;
	void update() override;
	void draw() override;
    std::string rtrim(std::string s);
    
    void getTweets();
    
    // background image
    gl::TextureRef mBackground;
    // star image
    gl::TextureRef mStars;
    bool mShowFlag;
    // logos
//    vector<gl::TextureRef> mLogos;
    
    //We'll parse our twitter content into these
    string tempKeyword;
    vector<string> mKeywords;
    vector<string> mAccounts;
    
    //For drawing our text
    Font mFont;
    gl::TextureFontRef	mTextureFont;
    
    params::InterfaceGlRef mParams;
    bool mShowParams;
    
    twitCurl twit;
    vector<map<string, int>> mTweets;
    bool mUseKeywords = true;
    
    // hold keys for Twitter API oauth
    vector<string> keys;
    
    int stripeHeight;
    int widthPos = 0;
    int widthPosOffset = 0;
    
    bool includeRTs;
    int tweetCount;
    int nyTweetCount;
    
    qtime::MovieWriterRef mMovieExporter;
    qtime::MovieWriter::Format format;
    // TODO - what should this number be?
    const int maxFrames = 10000;
};

void GetHeadlinesApp::setup()
{
    gl::clear(Color(0, 0, 0));
    gl::enableAlphaBlending(false);
    
    // read in variables
    Json::Value root;
    ifstream inputFile("accounts.json");
    
    if (inputFile.is_open()) {
        inputFile >> root;
    
        setFullScreen(root["fullscreen"].asBool());
        
        // load our flag image
        mBackground = gl::Texture::create( loadImage( loadAsset(root["backgroundImage"].asString()) ) );
        // load just the stars
        mStars = gl::Texture::create( loadImage( loadAsset(root["starsImage"].asString())));
    
        for(auto a: root["accounts"]){
            // load twitter handles
            mAccounts.push_back(a["name"].asString());
        }
        for(auto k: root["keywords"]){
            mKeywords.push_back(k.asString());
        }
        
        includeRTs = root["includeRTs"].asBool();
        tweetCount = root["tweetCount"].asInt();
        nyTweetCount = root["tweetCountNY1"].asInt();
        mShowParams = root["showParams"].asBool();
        mShowFlag = root["showFlag"].asBool();
    
        inputFile.close();
    }
    else cout << "Unable to open json file";
    
    stripeHeight = getWindowHeight()/13;

    // Create the interface and give it a name
    mParams = params::InterfaceGl::create("App parameters", vec2(200,200));
    
    // Set up some basic parameters
    mParams->addParam( "New Keyword", &tempKeyword ).updateFn( [this] { mKeywords.push_back(tempKeyword); getTweets();} );
    mParams->addParam("Filter", &mUseKeywords).updateFn( [this] {getTweets();});
    mParams->addParam("Show Params", &mShowParams).key("p");
    mParams->addButton("Update", [ & ]() { getTweets(); },	"key=u" );
    
    // Font used on news ticker for Fox
    mFont = Font( "Avenir", 36 );
    mTextureFont = gl::TextureFont::create( mFont );
    
    string line;
    ifstream myfile ("keys.txt");
    if (myfile.good())
    {
        while ( getline (myfile,line) )
        {
            keys.push_back(line);
        }

        myfile.close();
    }
    else cout << "Unable to open key file";
    
    twit.getOAuth().setConsumerKey(keys[0]);
    twit.getOAuth().setConsumerSecret(keys[1]);
    twit.getOAuth().setOAuthTokenKey(keys[2]);
    twit.getOAuth().setOAuthTokenSecret(keys[3]);
    
    getTweets();
    
    // quicktime setup
#if defined( CINDER_COCOA_TOUCH )
    format = qtime::MovieWrite::Format().codec( qtime::MovieWriter::PRO_RES_4444).fileType( qtime::MovieWriter::QUICK_TIME_MOVIE );
    mMovieExporter = qtime::MovieWriter::create( getDocumentsDirectory() / "test.mov", getWindowWidth(), getWindowHeight(), format );
#else
    fs::path path = getSaveFilePath();
    if( ! path.empty() ) {
//        auto format = qtime::MovieWriter::Format().codec( qtime::MovieWriter::H264 ).fileType( qtime::MovieWriter::QUICK_TIME_MOVIE )
//        .jpegQuality( 0.09f ).averageBitsPerSecond( 10000000 );
        format = qtime::MovieWriter::Format().codec( qtime::MovieWriter::PRO_RES_4444).fileType( qtime::MovieWriter::QUICK_TIME_MOVIE );
        mMovieExporter = qtime::MovieWriter::create( path, getWindowWidth(), getWindowHeight(), format );
    }
#endif
    gl::bindStockShader( gl::ShaderDef().color() );
}

void GetHeadlinesApp::getTweets()
{
    mTweets.clear();
    string resp;
    
    // for pulling out hyperlinks
    regex reg("http\\S+");
    
    if(twit.accountVerifyCredGet())
    {
        for(string a: mAccounts) {
            // if NY1 change the tweet count to include more tweets
            if(a == "NY1") {
                tweetCount = nyTweetCount;
            }
            if(twit.timelineUserGet(true, includeRTs, tweetCount, a)) {
//                cout << a << endl;
                map<string,int> temp;
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
                        // get rid of retweets (using false as a parameter doesn't get rid of quoted RTs)
                        if (t.substr(0,2) == "RT") {
                            continue;
                        }
                        
                        // only filter if using keywords
                        if(mUseKeywords) {
                            for(string k: mKeywords){
                                if (t.find(k) != std::string::npos) {
                                    string editedTweet = regex_replace(t, reg, "");
                                    editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '\n'), editedTweet.end());
                                    float fontNameWidth = mTextureFont->measureString( editedTweet+"..." ).x;
//                                    cout << editedTweet << endl;
                                    temp.insert(make_pair(editedTweet, fontNameWidth));
                                    break;
                                }
                            }
                        } else {
                            size_t end2 = t.find("http");
                            string editedTweet = rtrim(t.substr(0, end2));
                            editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '\n'), editedTweet.end());
                            float fontNameWidth = mTextureFont->measureString( editedTweet+"..." ).x;
                            temp.insert(make_pair(editedTweet, fontNameWidth));
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

void GetHeadlinesApp::keyDown(KeyEvent event)
{
    // toggle on and off using keywords
    if (event.getChar() == 'k') {
        mUseKeywords = !mUseKeywords;
        getTweets();
    }
}

void GetHeadlinesApp::update()
{
    if( mMovieExporter && getElapsedFrames() > 1 && getElapsedFrames() < maxFrames )
        mMovieExporter->addFrame( copyWindowSurface() );
    else if( mMovieExporter && getElapsedFrames() >= maxFrames ) {
        mMovieExporter->finish();
        mMovieExporter.reset();
    }
}

void GetHeadlinesApp::draw()
{
    if(mShowFlag) {
        gl::color(Color::white());
        gl::draw( mBackground, getWindowBounds() );
    } else{
        // TODO- green background or 0,0,0,0?
        gl::clear(Color(0,1,0));
    }
    int counter = 0;
    
    // TODO - send to Syphon
    // TODO - Syphon to isadora
    for(vector<map<string, int> >::iterator iter1 = mTweets.begin();
        iter1 != mTweets.end();
        iter1++) {
        (counter >= 7) ? widthPos = 10 : widthPos = getWindowWidth() * .4 - 20;
        for(map<string,int>::iterator iter2 = iter1->begin(); iter2 != iter1->end(); ++iter2) {
            (counter%2==0) ? gl::color( Color::white() ) : gl::color( Color::black() );
            // TODO - tweets should loop
//            if(widthPos-widthPosOffset+15 + iter2->second < 0 ) {
//                widthPos =
//            }
            mTextureFont->drawString(iter2->first+"...", vec2(widthPos-widthPosOffset+15, counter*stripeHeight+45));
            widthPos+=iter2->second;
        }
        counter++;
    }
    
    widthPosOffset+=2;
    
    if(mShowFlag) {
        // draw stars over tweets to create illusion that it's getting cut off
        gl::color(Color::white());
        Rectf drawRect( -1, -1, getWindowWidth()*.4, getWindowHeight()*.54 );
        gl::draw(mStars, drawRect);
    }
    
    // Draw the interface
    if(mShowParams) { mParams->draw(); }
}

// trim from end
std::string GetHeadlinesApp::rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

// TODO - clickable app that can work on any comp
// TODO - where should the file be saved? (same place as video assets, maybe a Google Drive folder
// TODO - executable doesn't work
// TODO - QA, ie there should always be at least two tweets from every network
// TODO - tweets need cleaning
// TODO - fps currently 26-29, can i make it better?

CINDER_APP( GetHeadlinesApp, RendererGl, [&](App::Settings *settings) {
    
    // have the app run full screen in second monitor (if available)
    vector<DisplayRef> displays = Display::getDisplays();
    
    if (displays.size() > 1) {
        
        settings->setDisplay(displays[1]);
    }
})
