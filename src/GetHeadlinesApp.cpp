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
    void rtrim(std::string &s);
    void replaceAll(std::string& str, const std::string& from, const std::string& to);
    bool replace(std::string& str, const std::string& from, const std::string& to);
    
    void getTweets();
    
    // background image
    gl::TextureRef mBackground;
    // star image
    gl::TextureRef mStars;
    bool mShowFlag;
    // logos
//    vector<gl::TextureRef> mLogos;
    
    string tempKeyword;
    // this will contain which keywords we are searching for
    vector<string> mKeywords;
    // this will contain which accounts we query
    vector<string> mAccounts;
    
    //For drawing our text
    Font mFont;
    gl::TextureFontRef	mTextureFont;
    
    params::InterfaceGlRef mParams;
    bool mShowParams;
    
    twitCurl twit;
    vector<list<pair<string, int>>> mTweets;
    bool mUseKeywords = true;
    
    // hold keys for Twitter API oauth
    vector<string> keys;
    
    int stripeHeight;
    int widthPos = 0;
    int widthPosOffset = 0;
    
    bool includeRTs;
    int tweetCount;
    int nyTweetCount;
    bool mRandom;
    
    qtime::MovieWriterRef mMovieExporter;
    qtime::MovieWriter::Format format;
    const int maxFrames = 18000;     // 10 minutes
    
    // for offsets once you start moving tweets
    // TODO - initalize to all zeros
    int eraseOffsets[13];
};

void GetHeadlinesApp::setup()
{
    std::fill_n(eraseOffsets, 13, 0);
    
    gl::clear(Color(0, 0, 0));
    gl::enableAlphaBlending(false);
    
    // read in variables from json file
    Json::Value root;
    ifstream inputFile("accounts.json");
    
    if (inputFile.is_open()) {
        inputFile >> root;
        
        mShowFlag = root["showFlag"].asBool();
        includeRTs = root["includeRTs"].asBool();
        tweetCount = root["tweetCount"].asInt();
        nyTweetCount = root["tweetCountNY1"].asInt();
        mShowParams = root["showParams"].asBool();
    
        setFullScreen(false);
        
        //setFullScreen(root["fullscreen"].asBool());
        
        // load our flag images if we are showing the flag
        if(mShowFlag) {
            try {
            mBackground = gl::Texture::create( loadImage( loadAsset(root["backgroundImage"].asString()) ) );
            // load just the stars
            mStars = gl::Texture::create( loadImage( loadAsset(root["starsImage"].asString())));
            }
            catch( Exception &exc ) {
                cout << "cannot load flag images" << endl;
            }
        }
    
        // load twitter handles
        for(auto a: root["accounts"]){
            mAccounts.push_back(a["name"].asString());
        }
        // load all keywords
        for(auto k: root["keywords"]){
            mKeywords.push_back(k.asString());
        }

    
        inputFile.close();
    }
    else cout << "Unable to open json file";
    
    stripeHeight = (getWindowHeight()*.935)/13;

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
    format = qtime::MovieWrite::Format().codec( qtime::MovieWriter::PRO_RES_422).fileType( qtime::MovieWriter::QUICK_TIME_MOVIE ).setTimeScale(300);
    mMovieExporter = qtime::MovieWriter::create( getDocumentsDirectory() / "test.mov", getWindowWidth(), getWindowHeight(), format );
#else
    fs::path path = getSaveFilePath();
    // TODO - if green use h.264 codec
    if( ! path.empty() ) {
//        auto format = qtime::MovieWriter::Format().codec( qtime::MovieWriter::H264 ).fileType( qtime::MovieWriter::QUICK_TIME_MOVIE )
//        .jpegQuality( 0.09f ).averageBitsPerSecond( 10000000 );
        format = qtime::MovieWriter::Format().codec( qtime::MovieWriter::PRO_RES_4444).fileType( qtime::MovieWriter::QUICK_TIME_MOVIE ).setTimeScale(300);

        mMovieExporter = qtime::MovieWriter::create( path, getWindowWidth(), getWindowHeight(), format );
    }
#endif
    gl::bindStockShader( gl::ShaderDef().color() );
    
    gl::clear();
}

void GetHeadlinesApp::getTweets()
{
    mTweets.clear();
    mRandom = false;
    string resp;
    
    // for pulling out hyperlinks
    regex reg("http\\S+");
    
    if(twit.accountVerifyCredGet())
    {
        for(string a: mAccounts) {
//            if(a == "RT_America"){
            // if NY1 change the tweet count to include more tweets
            if(a == "NY1") {
                tweetCount = nyTweetCount;
            }
            if(twit.timelineUserGet(true, includeRTs, tweetCount, a)) {
                cout << a << endl;
                list<pair<string,int>> temp;
                twit.getLastWebResponse(resp);
                Json::Value root;
                Json::Reader json;
                bool parsed = json.parse(resp, root, false);
                
                if(!parsed) {
                    console() << json.getFormattedErrorMessages() << endl;
                } else {
                    for(auto s: root)
                    {
                        std::string tweet = s["text"].asString();
                        // get rid of retweets (using false as a parameter doesn't get rid of quoted RTs)
                        if (tweet.substr(0,2) == "RT") {
                            continue;
                        }
                        
                        // only filter if using keywords
                        if(mUseKeywords) {
                            for(string k: mKeywords){
                                if (tweet.find(k) != std::string::npos) {
                                    // remove hyperlinks
                                    // TODO - remove duplicate tweets from same news source
                                    string editedTweet = regex_replace(tweet, reg, "");
                                    // remove weird period at beginning of some tweets
                                    if(editedTweet.at(0) == '.') {
                                        editedTweet = editedTweet.substr( 1, editedTweet.length() );
                                    }

                                    editedTweet = editedTweet.substr(0, editedTweet.find("[VIDEO]", 0));
                                    replaceAll(editedTweet, "&amp;", "&");
                                
                                    // go through and change all handles to actual names w/ twitter api
                                    string searchTweet = editedTweet;
                                    while (searchTweet.find('@') != std::string::npos) {
                                        searchTweet = searchTweet.substr(searchTweet.find("@"));
                                        int endIdx = 0;
                                        if(searchTweet.find(" ") != std::string::npos) {
                                            endIdx = searchTweet.find(" ")-1;
                                        } else {
                                            endIdx = searchTweet.length();
                                        }
                                        // If there is a ':' or other character after the handle
                                        // TODO - this should handle all non alpha numeric characters (excluding '_')
                                        if(searchTweet.substr(1,endIdx).find(":") != std::string::npos) {
                                            endIdx = searchTweet.find(":")-1;
                                        }
                                        if(searchTweet.substr(1,endIdx).find(".") != std::string::npos) {
                                            endIdx = searchTweet.find(".")-1;
                                        }
                                        if(searchTweet.substr(1,endIdx).find("'") != std::string::npos) {
                                            endIdx = searchTweet.find("'")-1;
                                        }
                                        if(searchTweet.substr(1,endIdx).find(")") != std::string::npos) {
                                            endIdx = searchTweet.find(")")-1;
                                        }
                                        if (twit.userGet(searchTweet.substr(1, endIdx)) ) {
                                            twit.getLastWebResponse(resp);
                                            Json::Value root;
                                            Json::Reader json;
                                            bool parsed = json.parse(resp, root, false);
                                            replaceAll(editedTweet, searchTweet.substr(0, endIdx+1), root["name"].asString());
                                        }
                                        searchTweet = searchTweet.substr(endIdx);
                                        
                                    }
                                    
                                    // TODO - can this be condensed?
                                    editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '\n'), editedTweet.end());
                                    editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '#'), editedTweet.end());
                                    // shortened urls
                                    editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '\246'), editedTweet.end());
                                    // shortened urls
                                    editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '\342'), editedTweet.end());
                                    // shortened urls
                                    editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '\200'), editedTweet.end());
                                    // remove $ because Cinder doesn't render that symbol (I filed an issue - https://github.com/cinder/Cinder/issues/1880)
                                    editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '$'), editedTweet.end());
                                    
                                    // clean up escape sequence (ie curly quotes, etc)
                                    replaceAll(editedTweet, "\230", "'");
                                    replaceAll(editedTweet, "\231", "'");
                                    replaceAll(editedTweet, "\223", "-");
                                    replaceAll(editedTweet, "\224", "-");
                                    replaceAll(editedTweet, "\234", "\"");
                                    replaceAll(editedTweet, "\235", "\"");
                                    // erase € because Cinder doesn't render it (I filed an issue - https://github.com/cinder/Cinder/issues/1880) )
                                    replaceAll(editedTweet, "\202\254", "");
                                    // remove registered mark
                                    replaceAll(editedTweet, "\302\256", "");
                                    
                                    rtrim(editedTweet);

                                    if(editedTweet.back() == ':' || editedTweet.back() == '.'){
                                        editedTweet.pop_back();
                                    }
                                  
                                    editedTweet += "...  ";
                                  
                                    float fontNameWidth = mTextureFont->measureString( editedTweet ).x;
                                    
                                    cout << editedTweet << endl;
                                    if(mRandom) {
                                        mTweets[Rand::randInt(0, mTweets.size())].push_back({editedTweet, fontNameWidth});
                                    } else {
                                        temp.push_back({editedTweet, fontNameWidth});
                                    }
                                    break;
                                }
                            }
                        } else {
                            // TODO - abstract most of the stuff in the if so it applies to both if and else
                            // remove hyperlinks
                            string editedTweet = regex_replace(tweet, reg, "");
                            editedTweet.erase(std::remove(editedTweet.begin(), editedTweet.end(), '\n'), editedTweet.end());
                            float fontNameWidth = mTextureFont->measureString( editedTweet+"..." ).x;
                            temp.push_back(make_pair(editedTweet, fontNameWidth));
                        }
                    }
                }
                if(!mRandom) {
                    mTweets.push_back(temp);
                }
            }
//            }
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
    // for recording to a mp4 file
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
        gl::clear(Color::black());
        gl::color(Color::white());
        gl::draw( mBackground, getWindowBounds() );
    } else{
        // TODO - does 0,0,0,0 work in syphon?
        gl::clear(Color(0,1,0));
    }

    int counter = 0;
    
    // TODO - send to Syphon
    // TODO - Syphon to isadora
    bool itHappened = false;
    for(vector<list<pair<string, int>>>::iterator iter1 = mTweets.begin(); iter1 != mTweets.end(); iter1++) {
        (counter >= 7) ? widthPos = 10 : widthPos = getWindowWidth() * .4 - 20;
            widthPos += eraseOffsets[counter];
        for(list<pair<string,int>>::iterator iter2 = iter1->begin(); iter2 != iter1->end();) {
            (counter%2==0) ? gl::color( Color::white() ) : gl::color( Color::black() );
            bool erased = false;
            // if tweet goes offscreen, send it to the back of the list
            if(iter2 == iter1->begin() && ((widthPos-widthPosOffset+20) * -1) > iter2->second) {
                eraseOffsets[counter] += iter2->second;
                widthPos += iter2->second;
                auto x = *iter2;
                iter2 = iter1->erase(iter2);
                erased = true;
                itHappened = true;
                iter1->insert(iter1->end(), x);
            }
            if(!erased) {
                mTextureFont->drawString(iter2->first, vec2(widthPos-widthPosOffset+20, counter*stripeHeight+60+(getWindowHeight()*.065)));
                widthPos+=iter2->second;
                ++iter2;
            }
        }
        counter++;
    }
    
    widthPosOffset+=2;
    
    if(mShowFlag) {
        // draw stars over tweets to create illusion that it's getting cut off
        gl::color(Color::white());
        Rectf drawRect( 0, 0, getWindowWidth()*.4, getWindowHeight()*.572);
        gl::draw(mStars, drawRect);
    }
    
    // Draw the interface
    if(mShowParams) { mParams->draw(); }
}

// trim from end
void GetHeadlinesApp::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

void GetHeadlinesApp::replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        size_t end_pos = start_pos + from.length();
        str.replace(start_pos, from.length(), to);
        
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

bool GetHeadlinesApp::replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

// TODO - put joke tweets back in randomly once looping is figured out and put NY1 back in
// TODO - clickable app that can work on any comp
// TODO - where should the file be saved? (same place as video assets, maybe a Google Drive folder
// TODO - executable doesn't work
// TODO - QA, ie there should always be at least two tweets from every network
// TODO - tweets need cleaning
// TODO - fps currently 26-29, can i make it better?
// TODO - write tweets into file, this becomes plan B in case api breaks

CINDER_APP( GetHeadlinesApp, RendererGl, [&](App::Settings *settings) {
    
    settings->setWindowSize(1920, 1080);
    // have the app run full screen in second monitor (if available)
    vector<DisplayRef> displays = Display::getDisplays();
    
    if (displays.size() > 1) {
        
        settings->setDisplay(displays[1]);
    }
})
