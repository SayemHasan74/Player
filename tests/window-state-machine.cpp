#include "WindowStateMachine.h"
#include <iostream>
#include <random>
#include <cstdlib>
using Model=WindowStateMachine;
using Mode=Model::Mode;
static int checks=0;
static void require(bool value,const char *label) {++checks;if(!value){std::cerr<<"FAIL: "<<label<<'\n';std::exit(1);}}
int main() {
    const QRect windowed(123,87,960,540),full(-2560,0,2560,1440),mini(-460,1250,420,64);
    for(auto base:{Mode::Windowed,Mode::Maximized}) {
        for(auto first:{Mode::Fullscreen,Mode::Mini}) {
            Model model(windowed);model.setBaseMode(base);model.observe(windowed,"left",1.5);
            const auto original=model.current();
            model.toggle(first,full,mini);require(model.previous()&&*model.previous()==original,"explicit previous state");
            model.observe(QRect(-700,300,first==Mode::Mini?500:2560,first==Mode::Mini?64:1440),"right",2);
            model.toggle(first,full,mini);require(model.current()==original,"exact direct round trip including screen and DPR");
            require(model.depth()==0,"direct round trip history empty");
        }
    }
    {
        Model model(windowed);model.observe(windowed,"primary",1);
        model.toggle(Mode::Fullscreen,full,mini);const auto fullscreen=model.current();
        model.toggle(Mode::Mini,full,mini);model.observe(QRect(20,30,480,64),"secondary",1.5);const auto small=model.current();
        model.toggle(Mode::Fullscreen,full,mini);require(model.previous()&&*model.previous()==small,"mini fullscreen previous mini");
        model.exitFullscreen();require(model.current()==small,"escape restores mini geometry");
        model.toggle(Mode::Mini,full,mini);require(model.current()==fullscreen,"mini restores fullscreen origin");
        model.exitFullscreen();require(model.current().geometry==windowed,"nested returns original window geometry");
        model.toggle(Mode::Mini,full,mini);require(model.current().geometry==small.geometry,"mini independently remembers position and width");
    }
    for(unsigned seed=0;seed<100;++seed) {
        std::mt19937 random(seed);Model model(windowed);const auto original=model.current();
        for(int step=0;step<200;++step) {
            const auto before=model.current();const auto depth=model.depth();
            const auto target=random()%2?Mode::Fullscreen:Mode::Mini;
            const auto previous=model.previous()?*model.previous():original;
            model.toggle(target,full,mini);
            if(before.mode==target) {require(model.current()==previous,"random exact pop");require(model.depth()+1==depth,"random history pop");}
            else {require(model.previous()&&*model.previous()==before,"random previous snapshot");require(model.depth()==depth+1,"random history push");}
            require(model.current().geometry.isValid(),"random valid geometry");
        }
        while(model.depth()) {const auto expected=*model.previous();const auto mode=model.current().mode;model.toggle(mode,full,mini);require(model.current()==expected,"random unwind exact");}
        require(model.current()==original,"random complete restoration");
    }
    Model rapid(windowed);for(int i=0;i<20;++i)rapid.toggle(Mode::Fullscreen,full,mini);
    require(rapid.current().mode==Mode::Windowed&&rapid.current().geometry==windowed,"twenty rapid fullscreen transitions");
    rapid.exitFullscreen();require(rapid.depth()==0,"escape outside fullscreen no op");
    std::cout<<"PASS: "<<checks<<" checks; all mode round trips, nesting, geometry/DPI and 100 random sequences of 200 actions\n";
}
