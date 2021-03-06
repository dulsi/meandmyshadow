/*
 * Copyright (C) 2011-2012 Me and My Shadow
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

#ifndef WIN32
#include <stdint.h>
#include <unistd.h>
#endif
#include "TreeStorageNode.h"
#include "MD5.h"
#include <string.h>
using namespace std;

TreeStorageNode::~TreeStorageNode(){
	//The deconstructor will just calls destroy().
	destroy();
}

void TreeStorageNode::destroy(){
	//Loop through the subnodes and delete them.
	for(unsigned int i=0;i<subNodes.size();i++){
		delete subNodes[i];
	}
	
	//Now clear some stuff.
	name.clear();
	value.clear();
	attributes.clear();
	subNodes.clear();
}

bool TreeStorageNode::setName(const std::string& name, const FilePosition& pos){
	this->name=name;
	return false;
}
void TreeStorageNode::getName(std::string& name){
	name=this->name;
}

bool TreeStorageNode::setValue(const std::vector<std::string>& value, const std::vector<FilePosition>& pos){
	this->value=value;
	return false;
}
void TreeStorageNode::getValue(std::vector<std::string>& value){
	value=this->value;
}

ITreeStorageBuilder* TreeStorageNode::newNode(){
	TreeStorageNode* obj=new TreeStorageNode;
	subNodes.push_back(obj);
	return obj;
}

bool TreeStorageNode::newAttribute(const std::string& name, const std::vector<std::string>& value, const FilePosition& namePos, const std::vector<FilePosition>& valuePos){
	//Put the attribute in the attributes map.
	attributes[name]=value;
	return false;
}

void* TreeStorageNode::getNextAttribute(void* pUserData,std::string& name,std::vector<std::string>& value){
	if(pUserData==NULL) objAttrIterator=attributes.begin();
	if(objAttrIterator!=attributes.end()){
		name=objAttrIterator->first;
		value=objAttrIterator->second;
		++objAttrIterator;
		return &objAttrIterator;
	}else{
		return NULL;
	}
}

void* TreeStorageNode::getNextNode(void* pUserData,ITreeStorageReader*& obj){
	intptr_t i=(intptr_t)pUserData;
	
	//Check if the pointer is in range of the subNodes vector.
	if(i<(int)subNodes.size()){
		obj=subNodes[i];
		return (void*)(i+1);
	}else{
		return NULL;
	}
}

static void md5AppendString(Md5& md5,const string& s){
	unsigned int sz=s.size();
	unsigned char c[4];
	c[0]=sz;
	c[1]=sz>>8;
	c[2]=sz>>16;
	c[3]=sz>>24;
	md5.update(c,4);

	if(sz>0) md5.update(s.c_str(),sz);
}

static void md5AppendVector(Md5& md5,const vector<string>& v){
	unsigned int sz=v.size();
	unsigned char c[4];
	c[0]=sz;
	c[1]=sz>>8;
	c[2]=sz>>16;
	c[3]=sz>>24;
	md5.update(c,4);

	for(unsigned int i=0;i<sz;i++){
		md5AppendString(md5,v[i]);
	}
}

static void md5AppendMap(Md5& md5,const map<string,vector<string> >& m){
	unsigned int sz=m.size();
	unsigned char c[4];
	c[0]=sz;
	c[1]=sz>>8;
	c[2]=sz>>16;
	c[3]=sz>>24;
	md5.update(c,4);

	for(map<string,vector<string> >::const_iterator it=m.begin();it!=m.end();++it){
		md5AppendString(md5,it->first);
		md5AppendVector(md5,it->second);
	}
}

unsigned char* TreeStorageNode::calcMD5(unsigned char* md){
	unsigned char digest[16];
	Md5 md5;

	md5.init();
	md5AppendString(md5,name);
	md5AppendVector(md5,value);
	md5AppendMap(md5,attributes);
	for(unsigned int i=0;i<subNodes.size();i++){
		TreeStorageNode *node=subNodes[i];
		if(node==NULL){
			memset(digest,0,16);
		}else{
			node->calcMD5(digest);
		}
		md5.update(digest,16);
	}

	return md5.final(md);
}
