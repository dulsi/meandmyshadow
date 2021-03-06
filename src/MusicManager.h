/*
 * Copyright (C) 2012 Me and My Shadow
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

#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <string>
#include <map>
#include <vector>

struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;

//Class for loading and playing the music.
class MusicManager{
private:  
	//This structure is used to hold music information.
	struct Music{
		//Pointer to the actual music.
		Mix_Music* music;
		//Pointer to the loop music, if any.
		Mix_Music* loop;
		
		//String containing the name of the music.
		//This is the same name as in the musicCollection.
		std::string name;	
		
		//String containing the track name of the music.
		//This is the name given by the author.
		std::string trackName;
		//String containing the name of the author.
		std::string author;
		//String containing the license the music is released under.
		std::string license;
		
		//Integer containing the time where to start playing.
		int start;
		
		//The volume to play the music with.
		//Scale 0-128.
		int volume;
		
		//Integer containing the loopstart.
		int loopStart;
		//Integer containing the loopend.
		//NOTE: loopend doesn't work and is thus ignored.
		int loopEnd;
		
		//Integer used to keep track when which music was played.
		int lastTime;
	};
public:
	//Constructor.
	MusicManager();
	//Destructor.
	~MusicManager();
	
	//Destroys the music.
	void destroy();
	
	//Method that will either disable or enable music.
	//enable: Boolean if the musicManager should be enabled or not.
	void setEnabled(bool enable=true);
	
	//Method that will set the volume of the music.
	//NOTE: The set volume isn't presistent, only use this to update the volume after a change to the music setting.
	//volume: The new volume.
	void setVolume(int volume);
	
	//This method will load one music file and add it to the collection.
	//file: The filename of the music file.
	//list: The music list the music belongs to.
	//Returns: String containing the loaded music comma sperated, it's empty if it fails.
	std::string loadMusic(const std::string &file,const std::string &list="");
	
	//This method will load from a music list.
	//file: The filename of the music list file.
	//Returns: True if no error occurs.
	bool loadMusicList(const std::string &file);
	
	//This method will start playing a music file.
	//name: The name of the song.
	//fade: Boolean if it should fade the current one out or not.
	void playMusic(const std::string &name,bool fade=true);

	//Method for retrieving the current playing track.
	//Returns: the name of the music that is playing.
	std::string getCurrentMusic();
	
	//This method will pick music from the current music list.
	void pickMusic();
	
	//Method that will be called when a music stopped.
	void musicStopped();
	
	//Set the music list.
	//list: The name of the list.
	void setMusicList(const std::string &list);

	//Method for retrieving the current music list.
	//NOTE This returns the set music list, this doesn't mean the playing music IS from this list.
	//Returns: the name of the current music list.
	std::string getCurrentMusicList();

	//Method that will create credits text for the (loaded) music tracks.
	//NOTE: This is only used by the Credits screen.
	//Returns: A vector containing the lines of credits.
	std::vector<std::string> createCredits();
private:
	//Boolean if the MusicManager is enabled or not.
	//The default value is false meaning that the MusicManager has to be enabled before the music starts.
	bool enabled;
	
	//Integer that is used to keep track of the last played song.
	int lastTime;
	
	//Pointer to the music struct that is currently playing.
	Music* playing;
	
	//String containing the name of the music to play when the previous one stopped.
	//This means that it will be checked in the musicStopped method.
	std::string nextMusic;
	
	//String containing the name of the current music list.
	std::string currentList;
	
	//Map containing the music.
	//The key is the name of the music and the value is a pointer to the Mix_Music.
	std::map<std::string,Music*> musicCollection;
	
	//Map containing the music lists.
	//The key is the name of the list and the value is an array of music names.
	std::map<std::string,std::vector<std::string> > musicLists;
};

#endif
