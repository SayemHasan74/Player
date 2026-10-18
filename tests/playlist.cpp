#include "../src/playlist/FolderScanner.h"
#include "../src/playlist/NaturalSort.h"
#include "../src/data/RecentHistoryModel.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <iostream>
#include <cstdlib>
void check(bool value,const char *name){if(!value){std::cerr<<name<<'\n';std::exit(1);}}
int main(int argc,char **argv){QCoreApplication app(argc,argv);QTemporaryDir dir;
    for(const auto &name:{"Show S01E01.mkv","show S01E02.MKV","Show S01E05.mkv","Show S01E10.mkv","Show sample.mkv","Show trailer.mp4","notes.txt","Sampleton S01E03.mkv"}){QFile file(dir.path()+"/"+name);check(file.open(QIODevice::WriteOnly),"fixture");file.write("media");}
    check(NaturalSort::less("Episode 2","Episode 10"),"2 before 10");
    check(NaturalSort::less("S01E05","s01e10"),"season mixed case");
    check(NaturalSort::less("Episode 002","Episode 010"),"leading zeros");
    auto path=dir.path()+"/Show S01E05.mkv";auto files=FolderScanner::scan(path);
    check(files.size()==5,"ignore nonmedia and clear extras; keep Sampleton");
    check(files.indexOf(path)==3,"real random episode index");
    check(FolderScanner::neighbor(path,1).endsWith("S01E10.mkv"),"real next");
    check(FolderScanner::scan(path,{}).size()==7,"configurable ignore");
    check(FolderScanner::scan(dir.path()+"/Show sample.mkv").contains(dir.path()+"/Show sample.mkv"),"explicit extras stay playable");
    check(FolderScanner::scan("https://example.com/a.mkv").isEmpty(),"no network folder traversal");
    RecentHistoryModel model(true);model.replace({QVariantMap{{"path","a"}},QVariantMap{{"path","b"}}});QPersistentModelIndex first=model.index(0);model.replace({QVariantMap{{"path","a"},{"position",5}},QVariantMap{{"path","b"}}});check(first.isValid()&&first.data(RecentHistoryModel::RecordRole).toMap().value("position")==5,"progress updates retain delegates");model.replace({QVariantMap{{"path","b"}},QVariantMap{{"path","a"}}});check(!first.isValid(),"actual reorder resets list");
    std::cout<<"Natural sort / folder scanner / stable playlist model: 11 checks passed\n";
}
