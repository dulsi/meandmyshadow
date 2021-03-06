/*
 * Copyright (C) 2012-2013 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Functions.h"
#include "GameState.h"
#include "Globals.h"
#include "GUIOverlay.h"
#include "InputManager.h"
#include "GUIObject.h"
#include "GUITextArea.h"
#include "StatisticsManager.h"

#include <assert.h>

using namespace std;

GUIOverlay::GUIOverlay(SDL_Renderer& renderer, GUIObject* root,bool dim)
	: dim(dim), keyboardNavigationMode(0)
{
	//First keep the pointer to the current GUIObjectRoot and currentState.
	parentState=currentState;
	tempGUIObjectRoot=GUIObjectRoot;

	//Now set the GUIObject root to the new root.
	currentState=this;
	GUIObjectRoot=root;
}

GUIOverlay::~GUIOverlay(){
	//We need to place everything back.
	currentState=parentState;
	parentState=NULL;

	//Delete the GUI if present.
	if(GUIObjectRoot)
		delete GUIObjectRoot;

	//Now put back the parent gui.
	GUIObjectRoot=tempGUIObjectRoot;
	tempGUIObjectRoot=NULL;
}

void GUIOverlay::enterLoop(ImageManager& imageManager, SDL_Renderer& renderer, bool skipByEscape, bool skipByReturn){
	//Keep the last resize event, this is to only process one.
	SDL_Event lastResize = {};

	while (GUIObjectRoot){
		while(SDL_PollEvent(&event)){
			//Check for a resize event.
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				lastResize = event;
				continue;
			}

			//Check if it's mouse event. If it's true then we quit the keyboard-only mode.
			if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
				isKeyboardOnly = false;
			}

			//Set the cursor type to the default one, the GUI can change that if needed.
			currentCursor = CURSOR_POINTER;

			//Let the input manager handle the events.
			inputMgr.updateState(true);

			//Let the currentState handle the events. (???)
			currentState->handleEvents(imageManager, renderer);

			//Also pass the events to the GUI.
			GUIObjectHandleEvents(imageManager, renderer, true);
			
			//Also check for the escape/return button.
			if ((skipByEscape && inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)) || (skipByReturn && inputMgr.isKeyDownEvent(INPUTMGR_SELECT))) {
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}

		//Process the resize event.
		if (lastResize.type == SDL_WINDOWEVENT){
			//TODO - used to be SDL_VIDEORESIZE
			// so this may trigger on more events than intended
			event = lastResize;
			onVideoResize(imageManager, renderer);

			//After resize we erase the event type
			//TODO - used to be SDL_NOEVENT
			lastResize.type = SDL_FIRSTEVENT;
		}

		//update input state (??)
		inputMgr.updateState(false);

		//Render the state.
		render(imageManager,renderer);
		//TODO: Shouldn't the gamestate take care of rendering the GUI?
		if (GUIObjectRoot) GUIObjectRoot->render(renderer);

		//draw new achievements (if any)
		statsMgr.render(imageManager, renderer);

		//display it
        flipScreen(renderer);
		SDL_Delay(1000/FPS);
	}

	//We broke out so clean up.
	delete this;
}

void GUIOverlay::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Experimental code for keyboard navigation.
	if (GUIObjectRoot && keyboardNavigationMode) {
		GUIObjectRoot->handleKeyboardNavigationEvents(imageManager, renderer, keyboardNavigationMode);
	}
}

//Nothing to do here
void GUIOverlay::logic(ImageManager&, SDL_Renderer&){
	//Check if the GUIObjectRoot (of the overlay) is deleted.
	if(!GUIObjectRoot)
		delete this;
}

void GUIOverlay::render(ImageManager& imageManager, SDL_Renderer& renderer) {
	//Render the parentState in full, including GUI
	{
		// backup some variables
		GameState *backupState = currentState;
		GUIObject *backupGUI = GUIObjectRoot;

		// in case of some code are accessing the global varaibles "currentState" and "GUIObjectRoot"
		currentState = parentState;
		GUIObjectRoot = tempGUIObjectRoot;

		currentState->render(imageManager, renderer);
		if (GUIObjectRoot) {
			GUIObjectRoot->render(renderer);
		}

		// sanity check - the currentState and GUIObjectRoot should be unchanged
		assert(currentState == parentState && GUIObjectRoot == tempGUIObjectRoot);

		// restore them
		currentState = backupState;
		GUIObjectRoot = backupGUI;
	}

	//Draw the overlay on top
	if(dim) {
		dimScreen(renderer);
	}
	// NO, the main loop renders the GUIObjectRoot again, so we shouldn't render here
	// if(GUIObjectRoot) GUIObjectRoot->render(renderer);
}

void GUIOverlay::resize(ImageManager& imageManager, SDL_Renderer& renderer){
	// backup some variables
	GameState *backupState = currentState;
	GUIObject *backupGUI = GUIObjectRoot;

	//We recenter the GUI.
	GUIObjectRoot->left=(SCREEN_WIDTH-GUIObjectRoot->width)/2;
	GUIObjectRoot->top=(SCREEN_HEIGHT-GUIObjectRoot->height)/2;

	// in case of some code are accessing the global varaibles "currentState" and "GUIObjectRoot"
	currentState = parentState;
	GUIObjectRoot = tempGUIObjectRoot;

	//Now let the parent state resize.
	currentState->resize(imageManager, renderer);

	//NOTE: After the resize it's likely that the GUIObjectRoot is new so we need to update our tempGUIObjectRoot pointer.
	tempGUIObjectRoot=GUIObjectRoot;

	// sanity check - the currentState should be unchanged
	assert(currentState == parentState);

	//And set the currentState back to ourself, GUIObjectRoot back to the overlay gui.
	currentState = backupState;
	GUIObjectRoot = backupGUI;
}

AddonOverlay::AddonOverlay(SDL_Renderer &renderer, GUIObject* root, GUIButton *okButton, GUIButton *cancelButton, GUITextArea *textArea, int keyboardNavigationMode)
	: GUIOverlay(renderer, root), okButton(okButton), cancelButton(cancelButton), textArea(textArea)
{
	this->keyboardNavigationMode = keyboardNavigationMode;
}

void AddonOverlay::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) {
	GUIOverlay::handleEvents(imageManager, renderer);

	//Do our own stuff.

	//Scroll the text area.
	if (textArea) {
		if (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(20, 0);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(-20, 0);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_UP)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(0, -1);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(0, 1);
		}
	}

	//Check return key.
	if (okButton && okButton->eventCallback && inputMgr.isKeyDownEvent(INPUTMGR_SELECT)){
		okButton->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, okButton->name, okButton, GUIEventClick);
		return;
	}

	//Check escape key.
	if (cancelButton && cancelButton->eventCallback && inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		cancelButton->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, cancelButton->name, cancelButton, GUIEventClick);
		return;
	}
}
