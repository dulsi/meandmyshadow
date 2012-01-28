/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "Functions.h"
#include "GameState.h"
#include "Globals.h"
#include "TitleMenu.h"
#include "GUIListBox.h"
#include "InputManager.h"
#include <iostream>
#include <algorithm>
using namespace std;

/////////////////////////MAIN_MENU//////////////////////////////////

//Integer containing the highlighted/selected menu option.
static int highlight=0;

Menu::Menu(){
	highlight=0;
	animation=0;
	
	//Load the background and the title image.
	background=loadImage(getDataPath()+"gfx/menu/background.png");
	title=loadImage(getDataPath()+"gfx/menu/title.png");
	
	//Now render the five entries.
	SDL_Color black={0,0,0};
	entries[0]=TTF_RenderText_Blended(fontTitle,"Play",black);
	entries[1]=TTF_RenderText_Blended(fontTitle,"Options",black);
	entries[2]=TTF_RenderText_Blended(fontTitle,"Map Editor",black);
	entries[3]=TTF_RenderText_Blended(fontTitle,"Help",black);
	entries[4]=TTF_RenderText_Blended(fontTitle,"Exit",black);
	entries[5]=TTF_RenderText_Blended(fontTitle,">",black);
	entries[6]=TTF_RenderText_Blended(fontTitle,"<",black);
}

Menu::~Menu(){
	//We need to free the five text surfaceses.
	for(unsigned int i=0;i<7;i++)
		SDL_FreeSurface(entries[i]);
}


void Menu::handleEvents(){
	//Get the x and y location of the mouse.
	int x,y;
	SDL_GetMouseState(&x,&y);

	//Calculate which option is highlighted using the location of the mouse.
	//Only if mouse is 'doing something'
	if(event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN){
		if(x>=200&&x<600&&y>=200&&y<520){
			highlight=(y-136)/64;
		}
	}
	
	//Down/Up -arrows move highlight
	if(inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
		highlight++;
		if(highlight>=6)
			highlight=5;
	}
	if(inputMgr.isKeyDownEvent(INPUTMGR_UP)){
		highlight--;
		if(highlight<1)
			highlight=1;
	}
	
	//Check if there's a press event.
	if((event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) ||
		(event.type==SDL_KEYUP && (event.key.keysym.sym==SDLK_RETURN || event.key.keysym.sym==SDLK_KP_ENTER))){
		//We have one so check which selected/highlighted option needs to be done.
		switch(highlight){
		case 1:
			//Enter the levelSelect state.
			setNextState(STATE_LEVEL_SELECT);
			break;
		case 2:
			//Enter the options state.
			setNextState(STATE_OPTIONS);
			break;
		case 3:
			//Enter the levelEditor, but first set the level to a default leveledit map.
			levelName="";
			setNextState(STATE_LEVEL_EDITOR);
			break;
		case 4:
			//Enter the help state.
			setNextState(STATE_HELP);
			break;
		case 5:
			//We quit, so we enter the exit state.
			setNextState(STATE_EXIT);
			break;
		}
	}

	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
}

//Nothing to do here
void Menu::logic(){
	animation++;
	if(animation>10)
		animation=-10;
}


void Menu::render(){
	applySurface(0,0,background,screen,NULL);
	
	//Draw the title.
	applySurface(90,40,title,screen,NULL);
	
	//Draw the menu entries.
	for(unsigned int i=0;i<5;i++){
		applySurface((800-entries[i]->w)/2,200+64*i+(64-entries[i]->h)/2,entries[i],screen,NULL);
	}
	
	//Check if an option is selected/highlighted.
	if(highlight>0){
		//Draw the '>' sign, which is entry 5.
		int x=(800-entries[highlight-1]->w)/2-(25-abs(animation)/2)-entries[5]->w;
		int y=136+64*highlight+(64-entries[5]->h)/2;
		applySurface(x,y,entries[5],screen,NULL);
		
		//Draw the '<' sign, which is entry 6.
		x=(800-entries[highlight-1]->w)/2+entries[highlight-1]->w+(25-abs(animation)/2);
		y=136+64*highlight+(64-entries[6]->h)/2;
		applySurface(x,y,entries[6],screen,NULL);
	}
}


/////////////////////////HELP_MENU//////////////////////////////////
Help::Help():currentScreen(0){
	//Get a list of the files in the help folder.
	string folder="gfx/menu/help/";
	vector<string> v=enumAllFiles(getDataPath()+folder,"png");
	//Sort the files.
	sort(v.begin(),v.end());
	
	//Now loop the files and load them.
	for(unsigned int o=0;o<v.size();o++){
		//Load the image.
		SDL_Surface* image=loadImage(getDataPath()+folder+v[o]);
		
		//Check if the loading succeeded.
		if(image){
			//Add the image to the screens..
			screens.push_back(image);
		}
	}
	
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,GUIObjectNone,"");
	
	//Now we create the previous and next buttons.
	previous=new GUIObject(10,550,284,36,GUIObjectButton,"< Previous");
	previous->name="cmdPrevious";
	previous->eventCallback=this;
	GUIObjectRoot->childControls.push_back(previous);
	
	next=new GUIObject(506,550,284,36,GUIObjectButton,"Next >");
	next->name="cmdNext";
	next->eventCallback=this;
	GUIObjectRoot->childControls.push_back(next);
	
	//Create the exit button.
	GUIObject* obj=new GUIObject(10,10,184,36,GUIObjectButton,"Back");
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	//Finally update the buttons.
	updateButtons();
}

Help::~Help(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Help::updateButtons(){
	//Hide the previous button when the currentScreen is 0.
	if(currentScreen<=0)
		previous->visible=false;
	else
		previous->visible=true;
	
	//Hide the next button when currentScreen is equal to the number of screens.
	if(currentScreen>=int(screens.size()-1))
		next->visible=false;
	else
		next->visible=true;
}

void Help::handleEvents(){
	//Check if escape is pressed, if so return to the main menu.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}

	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
	
	//Check for the page up and page down buttons.
	if(event.type==SDL_KEYUP && (event.key.keysym.sym==SDLK_PAGEUP || event.key.keysym.sym==SDLK_RIGHT)){
		currentScreen++;
		
		//Check if the currentScreen isn't going above the max.
		if(currentScreen>=int(screens.size()))
			currentScreen=screens.size()-1;
		
		//Update the buttons.
		updateButtons();
	}
	if(event.type==SDL_KEYUP && (event.key.keysym.sym==SDLK_PAGEDOWN || event.key.keysym.sym==SDLK_LEFT)){
		currentScreen--;
		
		//Check if the currentScreen isn't going below zero.
		if(currentScreen<=0)
			currentScreen=0;
		
		//Update the buttons.
		updateButtons();
	}
}

//Nothing to do here.
void Help::logic(){}

void Help::render(){
	//Draw the current screen.
	applySurface(0,0,screens[currentScreen],screen,NULL);
	
	//Draw the page count text.
	char s[64];
	sprintf(s,"%d / %d",currentScreen+1,screens.size());
	
	SDL_Color black={0,0,0,0};
	SDL_Color white={255,255,255,255};
	SDL_Surface* bm=TTF_RenderText_Shaded(fontGUI,s,black,white);
	
	//Calculate the location, center horizontally and vertically relative to the top.
	SDL_Rect r;
	r.x=(SCREEN_WIDTH-bm->w)/2;
	r.y=560;
	
	//Draw the text and free the surface.
	SDL_BlitSurface(bm,NULL,screen,&r);
	SDL_FreeSurface(bm);
}

void Help::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdPrevious"){
			currentScreen--;
			
			//Check if the currentScreen isn't going below zero.
			if(currentScreen<0)
				currentScreen=0;
			
			//Update the buttons.
			updateButtons();
		}
		if(name=="cmdNext"){
			currentScreen++;
			
			//Check if the currentScreen isn't going above the max.
			if(currentScreen>=int(screens.size()))
				currentScreen=screens.size()-1;
			
			//Update the buttons.
			updateButtons();
		}
		if(name=="cmdBack"){
			setNextState(STATE_MENU);
		}
	}
}


/////////////////////////OPTIONS_MENU//////////////////////////////////

//Some varables for the options.
static bool sound,fullscreen,leveltheme,internet;
static string themeName;

static bool useProxy;
static string internetProxy;

Options::Options(){
	//Load the background image.
	background=loadImage(getDataPath()+"gfx/menu/background.png");
	//Render the title.
	SDL_Color black={0,0,0};
	title=TTF_RenderText_Blended(fontTitle,"Settings",black);
	
	//Set some default settings.
	sound=getSettings()->getBoolValue("sound");
	fullscreen=getSettings()->getBoolValue("fullscreen");
	themeName=processFileName(getSettings()->getValue("theme"));
	leveltheme=getSettings()->getBoolValue("leveltheme");
	internet=getSettings()->getBoolValue("internet");
	internetProxy=getSettings()->getValue("internet-proxy");
	useProxy=!internetProxy.empty();
	
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,GUIObjectNone);

	//Now we create GUIObjects for every option.
	GUIObject *obj=new GUIObject(150,150,240,36,GUIObjectCheckBox,"Sound",sound?1:0);
	obj->name="chkSound";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
		
	obj=new GUIObject(150,190,240,36,GUIObjectCheckBox,"Fullscreen",fullscreen?1:0);
	obj->name="chkFullscreen";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	obj=new GUIObject(150,230,240,36,GUIObjectLabel,"Theme:");
	obj->name="theme";
	GUIObjectRoot->childControls.push_back(obj);
	
	//Create the theme option gui element.
	theme=new GUISingleLineListBox(250,230,300,36);
	theme->name="lstTheme";
	vector<string> v=enumAllDirs(getUserPath()+"themes/");
	for(vector<string>::iterator i = v.begin(); i != v.end(); ++i){
		themeLocations[*i]=getUserPath()+"themes/"+*i;
	}
	vector<string> v2=enumAllDirs(getDataPath()+"themes/");
	for(vector<string>::iterator i = v2.begin(); i != v2.end(); ++i){
		themeLocations[*i]=getDataPath()+"themes/"+*i;
	}
	v.insert(v.end(), v2.begin(), v2.end());

	//Try to find the configured theme so we can display it.
	int value = -1;
	for(vector<string>::iterator i = v.begin(); i != v.end(); ++i){
		if(themeLocations[*i]==themeName) {
			value=i - v.begin();
		}
	}
	theme->item=v;
	if(value == -1)
		value=theme->item.size() - 1;
	theme->value=value;
	theme->eventCallback=this;
	GUIObjectRoot->childControls.push_back(theme);

	obj=new GUIObject(150,270,240,36,GUIObjectCheckBox,"Level themes",leveltheme?1:0);
	obj->name="chkLeveltheme";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	obj=new GUIObject(150,310,240,36,GUIObjectCheckBox,"Internet",internet?1:0);
	obj->name="chkInternet";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);

	//new: proxy settings
	obj=new GUIObject(150,350,240,36,GUIObjectCheckBox,"Internet proxy",useProxy?1:0);
	obj->name="chkProxy";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	obj=new GUIObject(350,350,300,36,GUIObjectTextBox,internetProxy.c_str());
	obj->name="txtProxy";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);

	//new: key settings
	obj=new GUIObject(150,390,240,36,GUIObjectButton,"Config Keys");
	obj->name="cmdKeys";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);

	obj=new GUIObject(100,520,284,36,GUIObjectButton,"Cancel");
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
		
	obj=new GUIObject(400,520,284,36,GUIObjectButton,"Save Changes");
	obj->name="cmdSave";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	restartLabel=new GUIObject(10,250,284,36,GUIObjectLabel,"You need to restart before the changes have effect.");
	restartLabel->name="restart";
	restartLabel->visible=false;
	GUIObjectRoot->childControls.push_back(restartLabel);
}

Options::~Options(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//And free the title image.
	SDL_FreeSurface(title);
}

void Options::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			setNextState(STATE_MENU);
		}
		else if(name=="cmdSave"){
			//Save is pressed thus save 
			getSettings()->setValue("sound",sound?"1":"0");
			if(!sound){
				Mix_HaltMusic();
			}else{
				Mix_PlayMusic(music,-1);
			}
			getSettings()->setValue("fullscreen",fullscreen?"1":"0");
			getSettings()->setValue("leveltheme",leveltheme?"1":"0");
			getSettings()->setValue("internet",internet?"1":"0");
			getSettings()->setValue("theme",themeName);
			if(!useProxy)
				internetProxy.clear();
			getSettings()->setValue("internet-proxy",internetProxy);

			//the keys
			inputMgr.saveConfig();
			
			saveSettings();
		}
		else if(name=="cmdKeys"){
			inputMgr.showConfig();
		}
		else if(name=="chkSound"){
			sound=obj->value?true:false;
		}
		else if(name=="chkFullscreen"){
			fullscreen=obj->value?true:false;
			
			//Check if we should set restart true or false.
			if(fullscreen==getSettings()->getBoolValue("fullscreen")){
				//Hide the restart text.
				restartLabel->visible=false;
			}else{
				//Set the restart text visible.
				restartLabel->visible=true;
			}
			  
		}
		else if(name=="chkLeveltheme"){
			leveltheme=obj->value?true:false;
		}
		else if(name=="chkInternet"){
			internet=obj->value?true:false;
		}
		else if(name=="chkProxy"){
			useProxy=obj->value?true:false;
		}
	}
	if(name=="lstTheme"){
		if(theme!=NULL && theme->value>=0 && theme->value<(int)theme->item.size()){
			//Check if the theme is installed in the data path.
			if(themeLocations[theme->item[theme->value]].find(getDataPath())!=string::npos){
				themeName="%DATA%/themes/"+fileNameFromPath(themeLocations[theme->item[theme->value]]);
			}else if(themeLocations[theme->item[theme->value]].find(getUserPath())!=string::npos){
				themeName="%USER%/themes/"+fileNameFromPath(themeLocations[theme->item[theme->value]]);
			}else{
				themeName=themeLocations[theme->item[theme->value]];
			}
		}
	}
	else if(name=="txtProxy"){
		internetProxy=obj->caption;
	}
}

void Options::handleEvents(){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
}

//Nothing to do here.
void Options::logic(){}

void Options::render(){
	//Render the background image.
	applySurface(0,0,background,screen,NULL);
	//Now render the title.
	applySurface((800-title->w)/2,40,title,screen,NULL);
	
	//NOTE: The rendering of the GUI is done in Main.
}
