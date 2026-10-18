#include "AppSettings.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <cstdlib>
static void require(bool condition,const char *message) {if(!condition){std::cerr<<message<<'\n';std::exit(1);}}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);QTemporaryDir folder;require(folder.isValid(),"Temporary directory");
    qputenv("FLOW_SETTINGS_FILE",(folder.path()+"/settings.ini").toUtf8());
    {
        AppSettings settings;
        require(!settings.values().value("autoResume").toBool(),"Auto resume defaults off");
        require(settings.values().value("autoplayNext").toBool(),"Autoplay defaults on");
        settings.setValue("volume",200);require(settings.values().value("volume").toDouble()==150,"Volume clamp");
        settings.setValue("volume",73);settings.setValue("defaultSpeed",1.25);
        settings.setValue("subtitleLanguage","en,eng,ja,jpn");settings.setValue("highQuality",true);
        settings.setValue("unknown",123);require(!settings.values().contains("unknown"),"Reject unknown setting");
    } // Destructor flushes/join; writes occur on the settings worker.
    {
        AppSettings reloaded;
        require(reloaded.values().value("volume").toDouble()==73,"Volume survives restart");
        require(reloaded.values().value("defaultSpeed").toDouble()==1.25,"Speed survives restart");
        require(reloaded.values().value("highQuality").toBool(),"Quality survives restart");
        require(reloaded.values().value("subtitleLanguage")=="en,eng,ja,jpn","Languages survive restart");
    }
    std::cout<<"PASS: settings defaults, validation, worker flush and restart persistence\n";
}
