/*
 * Copyright (C) 2011-2013 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me And My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Block.h"
#include "Functions.h"
#include "LevelEditor.h"
#include "StatisticsManager.h"
#include "SoundManager.h"
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
using namespace std;

Block::Block(Game* parent,int x,int y,int w,int h,int type):
	GameObject(parent),
	animation(0),
	flags(0),
	temp(0),
	dx(0),
	dy(0),
	movingPosTime(-1),
	speed(0),
	pushableCurrentStand(NULL),
	xVel(0), yVel(0),
	inAir(false), xVelBase(0), yVelBase(0),
	isDelete(false)
{
	//Make sure the type is set, if not init should be called somewhere else with this information.
	if(type>=0 && type<TYPE_MAX)
		init(x,y,w,h,type);
}

Block::Block(const Block& other)
	: GameObject(other)
	, ScriptProxyUserClass(other)
	, animation(other.animation)
	, flags(other.flags)
	, temp(other.temp)
	, dx(other.dx), dy(other.dy)
	, movingPos(other.movingPos)
	, movingPosTime(-1)
	, speed(other.speed)
	, pushableCurrentStand(NULL)
	, customAppearanceName(other.customAppearanceName)
	, appearance(other.appearance)
	, xVel(other.xVel), yVel(other.yVel)
	, inAir(other.inAir), xVelBase(other.xVelBase), yVelBase(other.yVelBase)
	, id(other.id), destination(other.destination), message(other.message)
	, isDelete(other.isDelete)
{
	auto se = parent->getScriptExecutor();
	if (se == NULL) return;

	lua_State *state = se->getLuaState();
	if (state == NULL) return;

	//Copy the compiledScripts.
	for (auto it = other.compiledScripts.begin(); it != other.compiledScripts.end(); ++it) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, it->second);
		compiledScripts[it->first] = luaL_ref(state, LUA_REGISTRYINDEX);
	}
}

Block::~Block() {
	if (pushableCurrentStand) {
		delete pushableCurrentStand;
	}

	if (auto se = parent->getScriptExecutor()) {
		if (lua_State *state = se->getLuaState()) {
			//Delete the compiledScripts.
			for (auto it = compiledScripts.begin(); it != compiledScripts.end(); ++it) {
				luaL_unref(state, LUA_REGISTRYINDEX, it->second);
			}
		}
	}
}

int Block::getPathMaxTime() {
	if (movingPosTime < 0) {
		movingPosTime = 0;
		for (const SDL_Rect& p : movingPos) {
			movingPosTime += p.w;
		}
	}

	return movingPosTime;
}

void Block::init(int x,int y,int w,int h,int type){
	//First set the location and size of the box.
	//The default size is 50x50.
	box.x=boxBase.x=x;
	box.y=boxBase.y=y;
	box.w=boxBase.w=w;
	box.h=boxBase.h=h;
	
	//Set the type.
	this->type=type;

	//Some types need type specific code.
	if(type==TYPE_START_PLAYER){
		//This is the player start so set the player here.
		//We center the player, the player is 23px wide.
		parent->player.setLocation(box.x+(box.w-23)/2,box.y);
		parent->player.fx=box.x+(box.w-23)/2;
		parent->player.fy=box.y;
	}else if(type==TYPE_START_SHADOW){
		//This is the shadow start so set the shadow here.
		//We center the shadow, the shadow is 23px wide.
		parent->shadow.setLocation(box.x+(box.w-23)/2,box.y);
		parent->shadow.fx=box.x+(box.w-23)/2;
		parent->shadow.fy=box.y;
	}

	if (pushableCurrentStand) pushableCurrentStand->clear();
	inAir=true;
	xVel=yVel=xVelBase=yVelBase=0;

	//And load the (default) appearance.
	objThemes.getBlock(type)->createInstance(&appearance);
}

void Block::show(SDL_Renderer& renderer){
	//Make sure we are visible.
	if ((flags & 0x80000000) != 0 && (stateID != STATE_LEVEL_EDITOR || dynamic_cast<LevelEditor*>(parent)->isPlayMode()))
		return;
	
	//Check if the block is visible.
	if(checkCollision(camera,box)==true || (stateID==STATE_LEVEL_EDITOR && checkCollision(camera,boxBase)==true)){
		//Some type of block needs additional state check.
		switch(type){
		case TYPE_CHECKPOINT:
			//Check if the checkpoint is last used.
			if(parent!=NULL && parent->objLastCheckPoint.get()==this){
				if(!temp) appearance.changeState("activated");
				temp=1;
			}else{
				if(temp) appearance.changeState("default");
				temp=0;
			}
			break;
		}

		//Always draw the base. (This is only supposed to show in editor.)
		appearance.drawState("base", renderer, boxBase.x - camera.x, boxBase.y - camera.y, boxBase.w, boxBase.h);

		//What we need to draw depends on the type of block.
		switch (type) {
		default:
			//Draw normal.
			appearance.draw(renderer, box.x - camera.x, box.y - camera.y, box.w, box.h);
			break;
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			//Draw conveyor belt.
			if (animation) {
				// FIXME: ad-hoc code. Should add a new animation type in theme system.
				const int a = animation / 10;
				const SDL_Rect r = { box.x - camera.x, box.y - camera.y, box.w, box.h };
				appearance.draw(renderer, box.x - camera.x - 50 + a, box.y - camera.y, box.w + 50, box.h, &r);
			} else {
				appearance.draw(renderer, box.x - camera.x, box.y - camera.y, box.w, box.h);
			}
			break;
		}

		//Some types need to draw something on top of the base/default.
		switch(type){
		case TYPE_BUTTON:
			if(flags&4){
				if(animation<5) animation++;
			}else{
				if(animation>0) animation--;
			}
			appearance.drawState("button", renderer, box.x - camera.x, box.y - camera.y - 5 + animation, box.w, box.h);
			break;
		}

		//Draw some stupid icons during edit mode.
		if (stateID == STATE_LEVEL_EDITOR) {
			auto bmGUI = static_cast<LevelEditor*>(parent)->getGuiTexture();
			if (!bmGUI) {
				return;
			}

			int x = box.x - camera.x + 2;

			//Scripted blocks
			if (!scripts.empty() || !compiledScripts.empty()) {
				const SDL_Rect r = { 0, 32, 16, 16 };
				const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
				SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
				x += 16;
			}

			//Invisible blocks
			if (flags & 0x80000000) {
				const SDL_Rect r = { 16, 48, 16, 16 };
				const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
				SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
				x += 16;
			}

			//Block with custom appearance
			if (!customAppearanceName.empty()) {
				const SDL_Rect r = { 48, 16, 16, 16 };
				const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
				SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
				x += 16;
			}
		}
	}
}

SDL_Rect Block::getBox(int boxType){
	SDL_Rect r={0,0,0,0};
	switch(boxType){
	case BoxType_Base:
		return boxBase;
	case BoxType_Previous:
		r.x=box.x-dx;
		r.y=box.y-dy;
		r.w=box.w;
		r.h=box.h;
		return r;
	case BoxType_Delta:
		r.x=dx;
		r.y=dy;
		return r;
	case BoxType_Velocity:
		r.x=xVel;
		r.y=yVel;
		//NOTE: In case of the pushable block we sometimes need to substract one from the vertical velocity.
		//The yVel is set to one when it's resting, but should be handled as zero in collision.
		if((type==TYPE_PUSHABLE || type==TYPE_SHADOW_PUSHABLE) && !inAir)
			r.y=0;
		return r;
	case BoxType_Current:
		return box;
	}
	return r;
}

void Block::moveTo(int x,int y){
	//The block has moved so calculate the delta.
	//NOTE: Every delta is summed since they all happened within one frame and for collision/movement we need the resulting delta.
	int delta=(x-box.x);
	dx+=delta;
	xVel+=delta;
	delta=(y-box.y);
	dy+=delta;
	yVel+=delta;

	//And set the new location.
	box.x=x;
	box.y=y;
}

void Block::growTo(int w,int h){
	//The block has changed size 
	//NOTE: Every delta is summed since they all happened within one frame and for collision/movement we need the resulting delta.
	int delta=(w-box.w);
	dx+=delta;
	xVel+=delta;
	delta=(h-box.h);
	dy+=delta;
	yVel+=delta;

	//And set the new location.
	box.w=w;
	box.h=h;
}

void Block::playAnimation(){
	switch(type){
	case TYPE_SWAP:
		appearance.changeState("activated");
		break;
	case TYPE_SWITCH:
		temp^=1;
		appearance.changeState(temp?"activated":"default");
		break;
	}
}

void Block::onEvent(int eventType){
	//Make sure we are visible, otherwise no events should be handled except for 'OnCreate'.
	if ((flags & 0x80000000) != 0 && eventType != GameObjectEvent_OnCreate)
		return;
	
	//Iterator used to check if the map contains certain entries.
	map<int,int>::iterator it;

	//Check if there's a script for the event.
	it=compiledScripts.find(eventType);
	if(it!=compiledScripts.end()){
		//There is a script so execute it and check return value.
		int ret=parent->getScriptExecutor()->executeScript(it->second,this);

		//Return value 1 means do default event process.
		//Other values are coming soon...
		if(ret!=1) return;
	}

	//Event handling.
	switch(eventType){
	case GameObjectEvent_PlayerWalkOn:
		switch(type){
		case TYPE_FRAGILE: case TYPE_SHADOW_FRAGILE:
			if ((flags & 0x3) < 3) {
				flags++;
				const int f = flags & 0x3;
				const char* s = (f <= 0) ? "default" : ((f == 1) ? "fragile1" : ((f == 2) ? "fragile2" : "fragile3"));
				appearance.changeState(s);

				//Statistics and achievement.
				if (f >= 3 && !(parent->player.isPlayFromRecord() || parent->interlevel)) {
					if (parent->player.getObjCurrentStand() == this) {
						statsMgr.playerFragileBlocksBroken++;
					} else if (parent->shadow.getObjCurrentStand() == this) {
						statsMgr.shadowFragileBlocksBroken++;
					} else {
						std::cerr << "ERROR: Fragile block breaks without player or shadow standing on it" << std::endl;
					}

					switch (statsMgr.playerFragileBlocksBroken + statsMgr.shadowFragileBlocksBroken) {
					case 1:
						statsMgr.newAchievement("fragile1");
						break;
					case 100:
						statsMgr.newAchievement("fragile100");
						break;
					}
				}
			}
			break;
		}
		break;
	case GameObjectEvent_PlayerIsOn:
		switch(type){
		case TYPE_BUTTON:
			temp=1;
			break;
		}
		break;
	case GameObjectEvent_OnPlayerInteraction:
		switch (type) {
		case TYPE_SWITCH:
			//Make sure that the id isn't emtpy.
			if (!id.empty()) {
				parent->broadcastObjectEvent(0x10000 | (flags & 3),
					-1, id.c_str());
			} else {
				cerr << "WARNING: invalid switch id!" << endl;
			}
			break;
		}
		break;
	case GameObjectEvent_OnToggle:
		switch(type){
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_MOVING_SHADOW_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			flags^=1;
			break;
		case TYPE_PORTAL:
			if ((flags & 0x1) == 0 || !appearance.changeState("automatic_activated")) {
				appearance.changeState("activated");
			}
			//Clear the broken flag since it is activated.
			flags &= ~0x40000000;
			break;
		case TYPE_COLLECTABLE:
			appearance.changeState("inactive");
			flags|=1;
			break;
		}
		break;
	case GameObjectEvent_OnSwitchOn:
		switch(type){
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_MOVING_SHADOW_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			flags&=~1;
			break;
		case TYPE_EXIT:
			appearance.changeState("default");
			break;
		}
		break;
	case GameObjectEvent_OnSwitchOff:
		switch(type){
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_MOVING_SHADOW_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			flags|=1;
			break;
		case TYPE_EXIT:
			appearance.changeState("closed");
			break;
		}
		break;
	}
}

int Block::queryProperties(int propertyType, bool isShadow){
	switch(propertyType){
	case GameObjectProperty_PlayerCanWalkOn:
		if (flags & 0x80000000) break;
		switch(type){
		case TYPE_BLOCK:
		case TYPE_MOVING_BLOCK:
		case TYPE_CONVEYOR_BELT:
		case TYPE_BUTTON:
			return 1;
		case TYPE_PUSHABLE:
			return (flags & 0x40000000) == 0 ? 1 : 0;
		case TYPE_SHADOW_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_SHADOW_CONVEYOR_BELT:
			return isShadow ? 1 : 0;
		case TYPE_SHADOW_PUSHABLE:
			return (isShadow && (flags & 0x40000000) == 0) ? 1 : 0;
		case TYPE_FRAGILE:
			if ((flags & 0x3) < 3) return 1;
			break;
		case TYPE_SHADOW_FRAGILE:
			if (isShadow && (flags & 0x3) < 3) return 1;
			break;
		}
		break;
	case GameObjectProperty_IsSpikes:
		if (flags & 0x80000000) break;
		switch(type){
		case TYPE_SPIKES: case TYPE_MOVING_SPIKES:
			return 1;
		case TYPE_SHADOW_SPIKES: case TYPE_MOVING_SHADOW_SPIKES:
			return isShadow ? 1 : 0;
		}
		break;
	case GameObjectProperty_Flags:
		return flags;
		break;
	default:
		break;
	}
	return 0;
}

void Block::getEditorData(std::vector<std::pair<std::string,std::string> >& obj){
	//Every block has an id.
	obj.push_back(pair<string,string>("id",id));

	//And visibility.
	obj.push_back(pair<string, string>("visible", (flags & 0x80000000) == 0 ? "1" : "0"));

	//And custom appearance.
	obj.push_back(pair<string, string>("appearance", customAppearanceName));

	//Block specific properties.
	switch(type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
	case TYPE_MOVING_SHADOW_SPIKES:
		{
			char s[64],s0[64];
			sprintf(s,"%d",(int)movingPos.size());
			obj.push_back(pair<string,string>("MovingPosCount",s));
			obj.push_back(pair<string,string>("activated",(flags&0x1)?"0":"1"));
			obj.push_back(pair<string,string>("loop",(flags&0x2)?"0":"1"));
			for(unsigned int i=0;i<movingPos.size();i++){
				sprintf(s0+1,"%u",i);
				sprintf(s,"%d",movingPos[i].x);
				s0[0]='x';
				obj.push_back(pair<string,string>(s0,s));
				sprintf(s,"%d",movingPos[i].y);
				s0[0]='y';
				obj.push_back(pair<string,string>(s0,s));
				sprintf(s,"%d",movingPos[i].w);
				s0[0]='t';
				obj.push_back(pair<string,string>(s0,s));
			}
		}
		break;
	case TYPE_CONVEYOR_BELT:
	case TYPE_SHADOW_CONVEYOR_BELT:
		{
			char s[64];
			obj.push_back(pair<string,string>("activated",(flags&0x1)?"0":"1"));
			sprintf(s,"%d",speed);
			obj.push_back(pair<string,string>("speed10",s));
		}
		break;
	case TYPE_PORTAL:
		obj.push_back(pair<string,string>("automatic",(flags&0x1)?"1":"0"));
		obj.push_back(pair<string,string>("destination",destination));
		//The message for a portal should not contain '\n'
		obj.push_back(pair<string, string>("message", message));
		break;
	case TYPE_BUTTON:
	case TYPE_SWITCH:
		{
			string s;
			switch(flags&0x3){
			case 1:
				s="on";
				break;
			case 2:
				s="off";
				break;
			default:
				s="toggle";
				break;
			}
			obj.push_back(pair<string,string>("behaviour",s));
			if (type == TYPE_SWITCH) {
				//The message for a switch should not contain '\n'
				obj.push_back(pair<string, string>("message", message));
			}
		}
		break;
	case TYPE_NOTIFICATION_BLOCK:
		//Change \n with the characters '\n'.
		obj.push_back(pair<string, string>("message", escapeNewline(message)));
		break;
	case TYPE_FRAGILE: case TYPE_SHADOW_FRAGILE:
		{
			char s[64];
			sprintf(s,"%d",flags&0x3);
			obj.push_back(pair<string,string>("state",s));
		}
		break;
	}
}

void Block::setEditorData(std::map<std::string,std::string>& obj){
	//Iterator used to check if the map contains certain entries.
	map<string,string>::iterator it;

	//Check if the data contains the appearance.
	it = obj.find("appearance");
	if (it != obj.end()) {
		std::string newAppearanceName;
		if (it->second.empty() || it->second == std::string(Game::blockName[type]) + "_Scenery") {
			//Use the default appearance. (Do nothing since newAppearanceName is already empty)
		} else {
			//Use the custom appearance.
			newAppearanceName = it->second;
		}

		if (newAppearanceName != customAppearanceName) {
			//Try to find the custom appearance.
			ThemeBlock *themeBlock = NULL;
			if (!newAppearanceName.empty()) {
				themeBlock = objThemes.getScenery(newAppearanceName);
				if (themeBlock == NULL) {
					std::cerr << "ERROR: failed to load custom appearance '" << newAppearanceName << "' for block " << Game::blockName[type] << std::endl;
				}
			} else {
				themeBlock = objThemes.getBlock(type);
				if (themeBlock == NULL) {
					std::cerr << "ERROR: failed to load default appearance for block " << Game::blockName[type] << std::endl;
				}
			}

			if (themeBlock) {
				//Update the custom appearance name.
				customAppearanceName = newAppearanceName;

				//Recreate the theme block instance.
				themeBlock->createInstance(&appearance);

				//Do some block specific stuff,
				//e.g. reset the state according to block type,
				//or load some missing part of block states from default appearance.
				switch (type) {
				case TYPE_FRAGILE: case TYPE_SHADOW_FRAGILE:
					{
						const int f = flags & 0x3;
						const char* s = (f == 0) ? "default" : ((f == 1) ? "fragile1" : ((f == 2) ? "fragile2" : "fragile3"));
						appearance.changeState(s);
					}
					break;
				case TYPE_BUTTON:
					if (appearance.blockStates.find("button") == appearance.blockStates.end()) {
						//Try to load the "button" state from default appearance
						ThemeBlockInstance defaultAppearance;
						objThemes.getBlock(type)->createInstance(&defaultAppearance);
						auto it = defaultAppearance.blockStates.find("button");
						if (it != defaultAppearance.blockStates.end() && it->second >= 0 && it->second < (int)defaultAppearance.states.size()) {
							appearance.states.push_back(defaultAppearance.states[it->second]);
							appearance.blockStates[it->first] = appearance.states.size() - 1;
						}
					}
					break;
				}
			}
		}
	}

	//Check if the data contains the id block.
	it=obj.find("id");
	if(it!=obj.end()){
		//Set the id of the block.
		id=obj["id"];
	}

	//Check if the data contains the visibility
	it = obj.find("visible");
	if (it != obj.end()) {
		//Set the visibility.
		const string& s = it->second;
		flags = (flags & ~0x80000000) | ((s == "true" || atoi(s.c_str())) ? 0 : 0x80000000);
	}

	//Block specific properties.
	switch(type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
	case TYPE_MOVING_SHADOW_SPIKES:
		{
			//Make sure that the editor data contains MovingPosCount.
			it=obj.find("MovingPosCount");
			if(it!=obj.end()){
				char s0[64];
				int m=atoi(obj["MovingPosCount"].c_str());
				movingPos.clear();
				for(int i=0;i<m;i++){
					SDL_Rect r={0,0,0,0};
					sprintf(s0+1,"%d",i);
					s0[0]='x';
					r.x=atoi(obj[s0].c_str());
					s0[0]='y';
					r.y=atoi(obj[s0].c_str());
					s0[0]='t';
					r.w=atoi(obj[s0].c_str());
					movingPos.push_back(r);
				}
			}

			//Check if the activated or disabled key is in the data.
			//NOTE: 'disabled' is obsolete in V0.5.
			it=obj.find("activated");
			if(it!=obj.end()){
				const string& s=it->second;
				flags&=~0x1;
				if(!(s=="true" || atoi(s.c_str()))) flags|=0x1;
			}else{
				it=obj.find("disabled");
				if(it!=obj.end()){
					const string& s=it->second;
					flags&=~0x1;
					if(s=="true" || atoi(s.c_str())) flags|=0x1;
				}
			}

			//Check if the loop key is in the data.
			it=obj.find("loop");
			if(it!=obj.end()){
				const string& s=it->second;
				flags |= 0x2;
				if (s == "true" || atoi(s.c_str())) flags &= ~0x2;
			}

		}
		break;
	case TYPE_CONVEYOR_BELT:
	case TYPE_SHADOW_CONVEYOR_BELT:
		{
			//Check if there's a speed key in the editor data.
			//NOTE: 'speed' is obsolete in V0.5.
			it=obj.find("speed10");
			if(it!=obj.end()){
				speed=atoi(it->second.c_str());
			}else{
				it = obj.find("speed");
				if (it != obj.end()){
					speed = atoi(it->second.c_str()) * 10;
				}
			}

			//Check if the activated or disabled key is in the data.
			//NOTE: 'disabled' is obsolete in V0.5.
			it=obj.find("activated");
			if(it!=obj.end()){
				const string& s=it->second;
				flags&=~0x1;
				if(!(s=="true" || atoi(s.c_str()))) flags|=0x1;
			}else{
				it=obj.find("disabled");
				if(it!=obj.end()){
					const string& s=it->second;
					flags&=~0x1;
					if(s=="true" || atoi(s.c_str())) flags|=0x1;
				}
			}
		}
		break;
	case TYPE_PORTAL:
		{
			bool updateAppearance = false;

			//Check if the destination key is in the data.
			it = obj.find("destination");
			if (it != obj.end()){
				destination = it->second;
				//Clear the broken flag since the destination has changed.
				flags &= ~0x40000000;
				updateAppearance = true;
			}

			//Check if the automatic key is in the data.
			it=obj.find("automatic");
			if(it!=obj.end()){
				const string& s=it->second;
				flags&=~0x1;
				if(s=="true" || atoi(s.c_str())) flags|=0x1;
				updateAppearance = true;
			}

			if (updateAppearance) {
				//Change appearance according to "automatic" property.
				if ((flags & 0x1) == 0 || !appearance.changeState("automatic")) {
					appearance.changeState("default");
				}
			}

			if ((it = obj.find("message")) != obj.end()) {
				//The message for a portal should not contain '\n'
				message = it->second;
			}
		}
		break;
	case TYPE_BUTTON:
	case TYPE_SWITCH:
		{
			//Check if the behaviour key is in the data.
			it=obj.find("behaviour");
			if(it!=obj.end()){
				const string& s=it->second;
				flags&=~0x3;
				if(s=="on" || s==_("On")) flags|=1;
				else if(s=="off" || s==_("Off")) flags|=2;
			}
			if (type == TYPE_SWITCH && (it = obj.find("message")) != obj.end()) {
				//The message for a switch should not contain '\n'
				message = it->second;
			}
		}
		break;
	case TYPE_NOTIFICATION_BLOCK:
		{
			//Check if the message key is in the data.
			it=obj.find("message");
			if(it!=obj.end()){
				//Change the characters '\n' to a real \n
				message = unescapeNewline(it->second);
			}
		}
		break;
	case TYPE_FRAGILE: case TYPE_SHADOW_FRAGILE:
		{
			//Check if the status is in the data.
			it=obj.find("state");
			if(it!=obj.end()){
				flags=(flags&~0x3)|(atoi(it->second.c_str())&0x3);

				const int f = flags & 0x3;
				const char* s=(f==0)?"default":((f==1)?"fragile1":((f==2)?"fragile2":"fragile3"));
				appearance.changeState(s);
			}
		}
		break;
	}
}

std::string Block::getEditorProperty(const std::string& property){
	//First get the complete editor data.
	vector<pair<string,string> > objMap;
	vector<pair<string,string> >::iterator it;
	getEditorData(objMap);

	//Loop through the entries.
	for(it=objMap.begin();it!=objMap.end();++it){
		if(it->first==property)
			return it->second;
	}

	//Nothing found.
	return "";
}

void Block::setEditorProperty(const std::string& property, const std::string& value){
	//Create a map to hold the property.
	std::map<std::string,std::string> editorData;
	editorData[property]=value;

	//And call the setEditorData method.
	setEditorData(editorData);
}

bool Block::loadFromNode(ImageManager&, SDL_Renderer&, TreeStorageNode* objNode){
	//Make sure there are enough parameters.
	if(objNode->value.size()<3)
		return false;

	//Load the type and location.
	int type = 0;
	{
		auto it = Game::blockNameMap.find(objNode->value[0]);
		if (it != Game::blockNameMap.end()) {
			type = it->second;
		} else {
			cerr << "WARNING: Unknown block type '" << objNode->value[0] << "'!" << endl;
		}
	}
	int x=atoi(objNode->value[1].c_str());
	int y=atoi(objNode->value[2].c_str());
	int w=50;
	int h=50;
	if(objNode->value.size()>3)
		w=atoi(objNode->value[3].c_str());
	if(objNode->value.size()>4)
		h=atoi(objNode->value[4].c_str());
	//Call the init method.
	init(x,y,w,h,type);

	//Loop through the attributes as editorProperties.
	map<string,string> obj;
	for(map<string,vector<string> >::iterator i=objNode->attributes.begin();i!=objNode->attributes.end();++i){
		if(i->second.size()>0) obj[i->first]=i->second[0];
	}
	setEditorData(obj);

	//Loop through the subNodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		//FIXME: Ugly variable naming.
		TreeStorageNode* obj=objNode->subNodes[i];
		if(obj==NULL) continue;

		//Check for a script block.
		if(obj->name=="script" && !obj->value.empty()){
			map<string,int>::iterator it=Game::gameObjectEventNameMap.find(obj->value[0]);
			if(it!=Game::gameObjectEventNameMap.end()){
				int eventType=it->second;
				const std::string& script=obj->attributes["script"][0];
				if(!script.empty()) scripts[eventType]=script;
			}
		}
	}
	
	return true;
}

void Block::move(){
	bool isPlayMode = stateID != STATE_LEVEL_EDITOR || dynamic_cast<LevelEditor*>(parent)->isPlayMode();

	//Make sure we are visible, if not return.
	if ((flags & 0x80000000) != 0 && isPlayMode)
		return;
	
	//First update the animation of the appearance.
	appearance.updateAnimation();
	
	//Block specific move code.
	switch(type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
	case TYPE_MOVING_SHADOW_SPIKES:
		//Only move block when we are in play mode.
		if (isPlayMode) {
			//Make sure the block is enabled, if so increase the time.
			if(!(flags&0x1)) temp++;
			int t=temp;
			SDL_Rect r0={0,0,0,0},r1;
			dx=0;
			dy=0;

			//Loop through the moving positions.
			for(unsigned int i=0;i<movingPos.size();i++){
				r1.x=movingPos[i].x;
				r1.y=movingPos[i].y;
				r1.w=movingPos[i].w;
				if(t==0&&r1.w==0){ // time == 0 means the block deactivates at this point automatically
					r1.w=1;
					flags|=0x1;
				}
				if(t>=0 && t<(int)r1.w){
					int newX=boxBase.x+(int)(float(r0.x)+(float(r1.x)-float(r0.x))*float(t)/float(r1.w));
					int newY=boxBase.y+(int)(float(r0.y)+(float(r1.y)-float(r0.y))*float(t)/float(r1.w));
					//Calculate the delta and velocity.
					xVel=dx=newX-box.x;
					yVel=dy=newY-box.y;
					//Set the new location of the moving block.
					box.x=newX;
					box.y=newY;
					return;
				} else if (t == (int)r1.w && i == movingPos.size() - 1) {
					//If the time is the time of the movingPosition then set it equal to the location.
					//We do this to prevent a slight edge between normal blocks and moving blocks.
					int newX=boxBase.x+r1.x;
					int newY=boxBase.y+r1.y;
					xVel=dx=newX-box.x;
					yVel=dy=newY-box.y;
					box.x=newX;
					box.y=newY;
					return;
				}
				t-=r1.w;
				r0.x=r1.x;
				r0.y=r1.y;
			}
			//Only reset the stuff when we're looping.
			if((flags & 0x2) == 0){
				//Set the time back to zero.
				temp=0;
				//Calculate the delta movement.
				if(!movingPos.empty() && movingPos.back().x==0 && movingPos.back().y==0){
					dx=boxBase.x-box.x;
					dy=boxBase.y-box.y;
				}
				//Set the movingblock back to it's initial location.
				box.x=boxBase.x;
				box.y=boxBase.y;
			} else {
				//Reached the end, but not looping
				xVel=yVel=dx=dy=0;
			}
		}
		break;
	case TYPE_BUTTON:
		//Only move block when we are in play mode.
		if (isPlayMode) {
			//Check the third bit of flags to see if temp changed.
			int new_flags=temp?4:0;
			if((flags^new_flags)&4){
				//The button has been pressed or unpressed so change the third bit on flags.
				flags=(flags&~4)|new_flags;

				if(parent && (new_flags || (flags&3)==0)){
					//Make sure that id isn't empty.
					if(!id.empty()){
						parent->broadcastObjectEvent(0x10000|(flags&3),-1,id.c_str());
					}else{
						cerr<<"WARNING: invalid button id!"<<endl;
					}
				}
			}
			temp=0;
		}
		break;
	case TYPE_CONVEYOR_BELT:
	case TYPE_SHADOW_CONVEYOR_BELT:
		//NOTE: we update conveyor belt animation even in edit mode.
		//Increase the conveyor belt animation.
		if((flags&1)==0){
			//Since now 1 speed = 0.1 pixel/s we need some more sophisticated calculation.
			int a = animation + speed, d = 0;
			if (a < 0) {
				//Add a delta value to make it positive
				d = (((-a) / 500) + 1) * 500;
			}

			//Set the velocity NOTE This isn't the actual velocity of the block, but the speed of the player/shadow standing on it.
			xVel = (a + d) / 10 - (animation + d) / 10;

			//Update animation value
			animation = (a + d) % 500;
			assert(animation >= 0);
		} else {
			//Clear the velocity NOTE This isn't the actual velocity of the block, but the speed of the player/shadow standing on it.
			xVel = 0;
		}
		break;
	case TYPE_PUSHABLE: case TYPE_SHADOW_PUSHABLE:
		//FIXME: You need to call pushableBlockCollisionResolveStep() and pushableBlockCollisionResolveEnd() manually.
		break;
	}
}

void Block::pushableBlockCollisionResolveStep(std::vector<Block*>& sortedLevelObjects, PushableBlockCollisionResolveFlags collisionResolveFlags) {
	if ((type != TYPE_PUSHABLE && type != TYPE_SHADOW_PUSHABLE) || (flags & 0xC0000000) != 0) return;

	bool isPlayMode = stateID != STATE_LEVEL_EDITOR || dynamic_cast<LevelEditor*>(parent)->isPlayMode();
	if (!isPlayMode) return;

	if (collisionResolveFlags & COLLISION_RESOLVE_X_INIT) {
		pushableLastX = box.x;
	}
	if (collisionResolveFlags & COLLISION_RESOLVE_Y_INIT) {
		pushableLastY = box.y;

		//Update the vertical velocity.
		if (inAir) {
			yVel += 1;

			//Cap fall speed to 13.
			if (yVel > 13)
				yVel = 13;
		}
	}

	//Now get the velocity and delta of the object(s) the block is standing on.
	if (pushableCurrentStand && !pushableCurrentStand->empty()) {
		int surfaceWidth = 0, surfaceVelocity = 0;

		if (collisionResolveFlags & COLLISION_RESOLVE_Y_MASK) {
			yVelBase = 0x7FFFFFFF;
		}

		for (auto ocs : *pushableCurrentStand) {
			SDL_Rect r = ocs->getBox();
			SDL_Rect v = ocs->getBox(BoxType_Velocity);
			SDL_Rect delta = ocs->getBox(BoxType_Delta);

			int w = std::min(r.x + r.w, box.x + box.w) - std::max(r.x, box.x);
			if (w <= 0) {
				// fprintf(stderr, "[pushableBlockCollisionResolveStep] ERROR: Block 0x%p is standing on 0x%p but there are no (%d) overlapping pixels\n", this, ocs, w);
				w = 1; // ???
			}

			switch (ocs->type) {
			case TYPE_CONVEYOR_BELT:
			case TYPE_SHADOW_CONVEYOR_BELT:
				//For conveyor belts the velocity is transfered.
				if (collisionResolveFlags & COLLISION_RESOLVE_X_MASK) {
					surfaceWidth += w;
					if (collisionResolveFlags & COLLISION_RESOLVE_X_INIT) {
						surfaceVelocity += v.x * w;
					}
				}
				break;
			case TYPE_PUSHABLE:
			case TYPE_SHADOW_PUSHABLE:
				//Pushable blocks may be moved several times during collision resolve steps.
				if (collisionResolveFlags & COLLISION_RESOLVE_X_MASK) {
					surfaceWidth += w;
					surfaceVelocity += delta.x * w;
				}
				break;
			default:
				//In other cases, such as, player on shadow, player on crate... the change in x position must be considered (but only once).
				if (collisionResolveFlags & COLLISION_RESOLVE_X_MASK) {
					surfaceWidth += w;
					if (collisionResolveFlags & COLLISION_RESOLVE_X_INIT) {
						surfaceVelocity += delta.x * w;
					}
				}
				break;
			}

			//NOTE: Only copy the velocity of the block when moving down.
			//Upwards is automatically resolved before the player is moved.
			if (collisionResolveFlags & COLLISION_RESOLVE_Y_MASK) {
				if (delta.y < yVelBase && delta.y >= 0) yVelBase = delta.y;
			}
		}

		if (collisionResolveFlags & COLLISION_RESOLVE_X_MASK) {
			if (surfaceWidth > 0 && surfaceVelocity != 0) {
				if (surfaceVelocity > 0) {
					xVelBase += (surfaceWidth / 2 + surfaceVelocity) / surfaceWidth;
				} else {
					xVelBase -= (surfaceWidth / 2 - surfaceVelocity) / surfaceWidth;
				}
			}
		}

		if (collisionResolveFlags & COLLISION_RESOLVE_Y_MASK) {
			if (yVelBase == 0x7FFFFFFF) yVelBase = 0;
		}
	}

	if (collisionResolveFlags & COLLISION_RESOLVE_Y_INIT) {
		//Remove all the objects the block is currently standing.
		if (pushableCurrentStand) {
			pushableCurrentStand->clear();
		} else {
			pushableCurrentStand = new std::vector<Block*>();
		}
	}

	//Store the location of the player.
	int lastX = box.x;
	int lastY = box.y;

	//An array that will hold all the GameObjects that are involved in the collision/movement.
	vector<Block*> objects;

	//All the blocks have moved so if there's collision with the player, the block moved into him.
	for (auto o : sortedLevelObjects) {
		//Make sure to only check visible blocks.
		if (o->flags & 0xC0000000)
			continue;
		//Make sure we aren't the block.
		if (o == this)
			continue;
		//NOTE: The spikes, pushable and shadow pushable are solid for the pushable,
		//also the shadow spikes are solid for shadow pushable.
		if (o->type == TYPE_SPIKES || o->type == TYPE_MOVING_SPIKES || o->type == TYPE_PUSHABLE || o->type == TYPE_SHADOW_PUSHABLE) {
		} else if (type == TYPE_SHADOW_PUSHABLE && (o->type == TYPE_SHADOW_SPIKES || o->type == TYPE_MOVING_SHADOW_SPIKES)) {
		} else {
			//Make sure the object is solid for the player.
			if (!o->queryProperties(GameObjectProperty_PlayerCanWalkOn, type == TYPE_SHADOW_PUSHABLE))
				continue;
		}

		//Check for collision.
		if (checkCollision(box, o->getBox()))
			objects.push_back(o);
	}

	//There was collision so try to resolve it.
	if (!objects.empty()) {
		//FIXME: When multiple moving blocks are overlapping the pushable can be "bounced" off depending on the block order.
		for (auto o : objects) {
			SDL_Rect r = o->getBox();
			SDL_Rect delta = o->getBox(BoxType_Delta);

			//Check on which side of the box the pushable is.
			//FIXME: std::min/max() are ad-hoc code to fix bugs!!! Originally they were only box.x/y
			if (collisionResolveFlags & COLLISION_RESOLVE_RIGHT) {
				if (delta.x > 0) {
					//Move the pushable right if necessary.
					if ((r.x + r.w) - std::max(box.x, pushableLastX) <= delta.x && box.x < r.x + r.w)
						box.x = r.x + r.w;
				}
			}
			if (collisionResolveFlags & COLLISION_RESOLVE_LEFT) {
				if (delta.x < 0) {
					//Move the pushable left if necessary.
					if ((std::min(box.x, pushableLastX) + box.w) - r.x <= -delta.x && box.x > r.x - box.w)
						box.x = r.x - box.w;
				}
			}
			if (collisionResolveFlags & COLLISION_RESOLVE_VERTICAL) {
				if (delta.y > 0) {
					//Move the pushable down if necessary.
					if ((r.y + r.h) - std::max(box.y, pushableLastY) <= delta.y && box.y < r.y + r.h)
						box.y = r.y + r.h;
				} else if (delta.y < 0) {
					//Move the pushable up if necessary.
					if ((std::min(box.y, pushableLastY) + box.h) - r.y <= -delta.y && box.y > r.y - box.h)
						box.y = r.y - box.h;
				}
			}
		}
	}

	//Reuse the objects array, this time for blocks the block moves into.
	objects.clear();

	//Determine the collision frame.
	SDL_Rect frame = { box.x, box.y, box.w, box.h };

	const int xVelTotal = xVel + xVelBase;
	const int yVelTotal = ((collisionResolveFlags & COLLISION_RESOLVE_Y_INIT) ? yVel : 0) + yVelBase;

	//Keep the horizontal movement of the block in mind.
	if (collisionResolveFlags & COLLISION_RESOLVE_X_MASK) {
		if (xVelTotal >= 0) {
			frame.w += xVelTotal;
		} else {
			frame.x += xVelTotal;
			frame.w -= xVelTotal;
		}
	}
	//And the vertical movement.
	if (collisionResolveFlags & COLLISION_RESOLVE_Y_MASK) {
		if (yVelTotal >= 0) {
			frame.h += yVelTotal;
		} else {
			frame.y += yVelTotal;
			frame.h -= yVelTotal;
		}
	}

	//Loop through the game objects.
	for (auto o : sortedLevelObjects) {
		//Make sure the object is visible.
		if (o->flags & 0xC0000000)
			continue;
		//Make sure we aren't the block.
		if (o == this)
			continue;
		//NOTE: The spikes, pushable and shadow pushable are solid for the pushable,
		//also the shadow spikes are solid for shadow pushable.
		if (o->type == TYPE_SPIKES || o->type == TYPE_MOVING_SPIKES || o->type == TYPE_PUSHABLE || o->type == TYPE_SHADOW_PUSHABLE) {
		} else if (type == TYPE_SHADOW_PUSHABLE && (o->type == TYPE_SHADOW_SPIKES || o->type == TYPE_MOVING_SHADOW_SPIKES)) {
		} else {
			//Make sure the object is solid for the player.
			if (!o->queryProperties(GameObjectProperty_PlayerCanWalkOn, type == TYPE_SHADOW_PUSHABLE))
				continue;
		}

		//Check if the block is inside the frame.
		if (checkCollision(frame, o->getBox()))
			objects.push_back(o);
	}

	//Horizontal pass.
	if ((collisionResolveFlags & COLLISION_RESOLVE_X_MASK) != 0 && xVelTotal != 0) {
		box.x += xVelTotal;

		const int x0 = box.x;

		for (auto o : objects) {
			SDL_Rect r = o->getBox();
			if (!checkCollision(box, r))
				continue;

			if (xVelTotal > 0) {
				//We came from the left so the right edge of the player must be less or equal than xVel+xVelBase.
				if ((box.x + box.w) - r.x <= xVelTotal)
					box.x = r.x - box.w;
			} else {
				//We came from the right so the left edge of the player must be greater or equal than xVel+xVelBase.
				if (box.x - (r.x + r.w) >= xVelTotal)
					box.x = r.x + r.w;
			}
		}

		if (collisionResolveFlags & COLLISION_RESOLVE_X_INIT) {
			const bool isPlayFromRecord = parent->player.isPlayFromRecord() || parent->interlevel;

			//Update player pushing distance statistics.
			if (parent->player.objCurrentPushing == this && parent->player.objCurrentPushing_pushVel != 0 && !isPlayFromRecord) {
				const int oldX = x0 - parent->player.objCurrentPushing_pushVel;
				if (parent->player.objCurrentPushing_pushVel > 0 && box.x > oldX) {
					statsMgr.playerPushingDistance += float(box.x - oldX) * 0.02f;
				} else if (parent->player.objCurrentPushing_pushVel < 0 && box.x < oldX) {
					statsMgr.playerPushingDistance += float(oldX - box.x) * 0.02f;
				}
			}

			//Update shadow pushing distance statistics.
			if (parent->shadow.objCurrentPushing == this && parent->shadow.objCurrentPushing_pushVel != 0 && !isPlayFromRecord) {
				const int oldX = x0 - parent->shadow.objCurrentPushing_pushVel;
				if (parent->shadow.objCurrentPushing_pushVel > 0 && box.x > oldX) {
					statsMgr.shadowPushingDistance += float(box.x - oldX) * 0.02f;
				} else if (parent->shadow.objCurrentPushing_pushVel < 0 && box.x < oldX) {
					statsMgr.shadowPushingDistance += float(oldX - box.x) * 0.02f;
				}
			}
		}
	}

	if (collisionResolveFlags & COLLISION_RESOLVE_Y_INIT) {
		inAir = true;

		//Vertical pass.
		if (yVelTotal != 0) {
			box.y += yVelTotal;
			for (auto o : objects) {
				SDL_Rect r = o->getBox();
				if (!checkCollision(box, r))
					continue;

				//Now check if we entered the block vertically.
				if (yVelTotal > 0) {
					//We came from the top so the bottom edge of the player must be less or equal than yVel+yVelBase.
					if ((box.y + box.h) - r.y <= yVelTotal) {
						//NOTE: lastStand is handled later since the player can stand on only one block at the time.

						//Check if there's already a lastStand.
						if (!pushableCurrentStand->empty()) {
							//Get any one of the lastStand.
							Block *lastStand = pushableCurrentStand->front();

							SDL_Rect r = o->getBox();
							SDL_Rect r2 = lastStand->getBox();

							//Check their y coordinates.
							if (r.y == r2.y) {
								pushableCurrentStand->push_back(o);
							} else if (r.y < r2.y) {
								pushableCurrentStand->clear();
								pushableCurrentStand->push_back(o);
							}
						} else {
							pushableCurrentStand->push_back(o);
						}
					}
				} else {
					//We came from the bottom so the upper edge of the player must be greater or equal than yVel+yVelBase.
					if (box.y - (r.y + r.h) >= yVelTotal) {
						box.y = r.y + r.h;
						yVel = 0; // ???
					}
				}
			}
		}

		if (!pushableCurrentStand->empty()) {
			inAir = false;
			yVel = 1;
			SDL_Rect r = pushableCurrentStand->front()->getBox();
			box.y = r.y - box.h;
		}
	}

	if (collisionResolveFlags & COLLISION_RESOLVE_X_MASK) {
		dx = box.x - lastX;
		xVel = 0;
		xVelBase = 0;
	}
	if (collisionResolveFlags & COLLISION_RESOLVE_Y_MASK) {
		dy = box.y - lastY;
	}
}

void Block::pushableBlockTopologicalSortVisit(std::vector<Block*>& pushableBlocks, std::vector<Block*>& sortedPushableBlocks, Block* currentBlock,
	bool(*hasEdge)(Block* block1, Block* block2))
{
	// This algorithm copied from Wikipedia "Topological sorting", section "Depth-first search".

	// Omit this check since it's already cheked in other places.
	assert(currentBlock->pushableTopologicalSortMark != PUSHABLE_TOPOLOGICAL_SORT_PERMANENT_MARK);

	// not a DAG error
	if (currentBlock->pushableTopologicalSortMark == PUSHABLE_TOPOLOGICAL_SORT_TEMPORARY_MARK) {
		fprintf(stderr, "[pushableBlockTopologicalSort] ERROR: Not a DAG! Current block: 0x%p (%d,%d,%d,%d)\n", currentBlock,
			currentBlock->box.x, currentBlock->box.y, currentBlock->box.w, currentBlock->box.h);
		return;
	}

	// mark currentBlock with a temporary mark
	currentBlock->pushableTopologicalSortMark = PUSHABLE_TOPOLOGICAL_SORT_TEMPORARY_MARK;

	for (auto o : pushableBlocks) {
		//Here we reverse the hasEdge() check compared with Wikipedia article, so that we can add currentBlock to the *tail* of the list.
		if (o != currentBlock
			&& o->pushableTopologicalSortMark != PUSHABLE_TOPOLOGICAL_SORT_PERMANENT_MARK
			&& hasEdge(o, currentBlock))
		{
			pushableBlockTopologicalSortVisit(pushableBlocks, sortedPushableBlocks, o, hasEdge);
		}
	}

	// mark currentBlock with a permanent mark
	currentBlock->pushableTopologicalSortMark = PUSHABLE_TOPOLOGICAL_SORT_PERMANENT_MARK;

	// add currentBlock to the *tail* of the list
	sortedPushableBlocks.push_back(currentBlock);
}


void Block::pushableBlockTopologicalSort(std::vector<Block*>& pushableBlocks, std::vector<Block*>& sortedPushableBlocks,
	bool(*hasEdge)(Block* block1, Block* block2))
{
	// This algorithm copied from Wikipedia "Topological sorting", section "Depth-first search".

	for (auto o : pushableBlocks) {
		o->pushableTopologicalSortMark = PUSHABLE_TOPOLOGICAL_SORT_NO_MARK;
	}

	sortedPushableBlocks.clear();

	for (auto o : pushableBlocks) {
		assert(o->pushableTopologicalSortMark != PUSHABLE_TOPOLOGICAL_SORT_TEMPORARY_MARK);
		if (o->pushableTopologicalSortMark == PUSHABLE_TOPOLOGICAL_SORT_NO_MARK) {
			pushableBlockTopologicalSortVisit(pushableBlocks, sortedPushableBlocks, o, hasEdge);
		}
	}

	assert(pushableBlocks.size() == sortedPushableBlocks.size());
}

void Block::pushableBlockCollisionResolve(std::vector<Block*>& nonPushableBlocks, std::vector<Block*>& pushableBlocks) {
	std::vector<Block*> sortedLevelObjects = nonPushableBlocks;
	sortedLevelObjects.insert(sortedLevelObjects.end(), pushableBlocks.begin(), pushableBlocks.end());

	//Sort pushable blocks by their position, which is an ad-hoc workaround for
	//<https://forum.freegamedev.net/viewtopic.php?f=48&t=8047#p77692>.

	//We sort by the bottom (reversed), then by the left
	std::stable_sort(sortedLevelObjects.begin(), sortedLevelObjects.end(),
		[](const Block* obj1, const Block* obj2)->bool
	{
		SDL_Rect r1 = const_cast<Block*>(obj1)->getBox(), r2 = const_cast<Block*>(obj2)->getBox();
		if (r1.y + r1.h > r2.y + r2.h) return true;
		else if (r1.y + r1.h < r2.y + r2.h) return false;
		else return r1.x < r2.x;
	});

	//Vertical movement.
	for (auto o : sortedLevelObjects) {
		o->pushableBlockCollisionResolveStep(sortedLevelObjects, Block::COLLISION_RESOLVE_Y_INIT_AND_VERTICAL);
	}

	std::vector<Block*> sortedPushableBlocks;

	//Do topological sort.
	pushableBlockTopologicalSort(pushableBlocks, sortedPushableBlocks, pushableBlockTopologicalSortCheckRight);
	sortedLevelObjects = nonPushableBlocks;
	sortedLevelObjects.insert(sortedLevelObjects.end(), sortedPushableBlocks.begin(), sortedPushableBlocks.end());

	//Horizontal movement (to the right).
	for (auto o : sortedLevelObjects) {
		o->pushableBlockCollisionResolveStep(sortedLevelObjects, Block::COLLISION_RESOLVE_X_INIT_AND_RIGHT);
	}

	//Do topological sort.
	pushableBlockTopologicalSort(pushableBlocks, sortedPushableBlocks, pushableBlockTopologicalSortCheckLeft);
	sortedLevelObjects = nonPushableBlocks;
	sortedLevelObjects.insert(sortedLevelObjects.end(), sortedPushableBlocks.begin(), sortedPushableBlocks.end());

	//Horizontal movement (to the left).
	for (auto o : sortedLevelObjects) {
		o->pushableBlockCollisionResolveStep(sortedLevelObjects, Block::COLLISION_RESOLVE_LEFT);
	}

	//End.
	for (auto o : sortedLevelObjects) {
		o->pushableBlockCollisionResolveEnd();
	}
}

bool Block::pushableBlockTopologicalSortCheckRight(Block* block1, Block* block2) {
	if (block2->pushableCurrentStand) {
		for (auto o : *(block2->pushableCurrentStand)) {
			if (o == block1) return true;
		}
	}
	return block1->box.y + block1->box.h > block2->box.y
		&& block2->box.y + block2->box.h > block1->box.y
		&& block1->box.x + block1->box.w < block2->box.x + block2->box.w;
}

bool Block::pushableBlockTopologicalSortCheckLeft(Block* block1, Block* block2) {
	if (block2->pushableCurrentStand) {
		for (auto o : *(block2->pushableCurrentStand)) {
			if (o == block1) return true;
		}
	}
	return block1->box.y + block1->box.h > block2->box.y
		&& block2->box.y + block2->box.h > block1->box.y
		&& block1->box.x > block2->box.x;
}

void Block::pushableBlockCollisionResolveEnd() {
	if ((type != TYPE_PUSHABLE && type != TYPE_SHADOW_PUSHABLE) || (flags & 0xC0000000) != 0) return;

	bool isPlayMode = stateID != STATE_LEVEL_EDITOR || dynamic_cast<LevelEditor*>(parent)->isPlayMode();
	if (!isPlayMode) return;

	//Reset some variables.
	dx = box.x - pushableLastX;
	dy = box.y - pushableLastY;
	xVel = 0;
	xVelBase = 0;

	//Finally check if the pushable is squashed.
	bool broken = false;

	if (box.y > parent->levelRect.y + parent->levelRect.h) {
		broken = true;
	} else {
		for (auto o : parent->levelObjects){
			//Make sure the object is visible.
			if (o->flags & 0xC0000000)
				continue;
			//Make sure we aren't the block.
			if (o == this)
				continue;
			//NOTE: The spikes, pushable and shadow pushable are solid for the pushable,
			//also the shadow spikes are solid for shadow pushable.
			if (o->type == TYPE_SPIKES || o->type == TYPE_MOVING_SPIKES || o->type == TYPE_PUSHABLE || o->type == TYPE_SHADOW_PUSHABLE) {
			} else if (type == TYPE_SHADOW_PUSHABLE && (o->type == TYPE_SHADOW_SPIKES || o->type == TYPE_MOVING_SHADOW_SPIKES)) {
			} else {
				//Make sure the object is solid for the player.
				if (!o->queryProperties(GameObjectProperty_PlayerCanWalkOn, type == TYPE_SHADOW_PUSHABLE))
					continue;
			}

			//Check if the block is inside the frame.
			if (checkCollision(box, o->getBox())) {
				broken = true;
				break;
			}
		}
	}

	if (broken) {
		//Squashed.
		appearance.changeState("broken");
		flags |= 0x40000000;

		//TODO: a proper sound effect.
		getSoundManager()->playSound("hit");

		//Statistics and achievement.
		if (!(parent->player.isPlayFromRecord() || parent->interlevel)) {
			statsMgr.pushableBlocksBroken++;

			switch (statsMgr.pushableBlocksBroken) {
			case 1:
				statsMgr.newAchievement("boxcrush1");
				break;
			case 100:
				statsMgr.newAchievement("boxcrush100");
				break;
			}
		}
	}
}

void Block::deleteMe() {
	if (isDelete) return;

	//Set the delete flag.
	isDelete = true;

	//Hide this block.
	flags |= 0x80000000;

	//Invalidate all references to this block.
	if (proxy->object == this) {
		proxy->object = NULL;
	}

	//Update totalCollectables, etc. when removing a collectable
	if (type == TYPE_COLLECTABLE) {
		if (flags & 0x1) parent->currentCollectables--;
		parent->totalCollectables--;
	}
}

void Block::breakTeleporter(bool broken) {
	if (type == TYPE_PORTAL) {
		if (broken) {
			appearance.changeState("broken");
			flags |= 0x40000000;
		} else {
			//Change appearance according to "automatic" property.
			if ((flags & 0x1) == 0 || !appearance.changeState("automatic")) {
				appearance.changeState("default");
			}
			flags &= ~0x40000000;
		}
	}
}
