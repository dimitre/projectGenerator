/*
 * Utils.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "pugixml.hpp"

#include "ofLog.h"
#include "ofSystemUtils.h"
#include "baseProject.h"
struct LibraryBinary;

namespace fs = of::filesystem;
using std::string;
using std::vector;
using std::cout;
using std::endl;

string generateUUID(string input);

fs::path getOFRoot();
void setOFRoot(const fs::path & path);

string convertStringToWindowsSeparator(string in);
vector<string> fileToStrings (const fs::path & file);

void findandreplace( string& tInput, string tFind, string tReplace );
void findandreplaceInTexfile (const fs::path & fileName, string tFind, string tReplace );

bool doesTagAndAttributeExist(pugi::xml_document & doc, string tag, string attribute, string newValue);
pugi::xml_node appendValue(pugi::xml_document & doc, string tag, string attribute, string newValue, bool addMultiple = false);



void getFoldersRecursively(const fs::path & path, std::vector < string > & folderNames, string platform);
void getFoldersRecursively(const fs::path & path, std::vector < fs::path > & folderNames, string platform);
void getFilesRecursively(const fs::path & path, std::vector < string > & fileNames);
void getFilesRecursively(const fs::path & path, std::vector < fs::path > & fileNames);
void getLibsRecursively(const fs::path & path, std::vector < string > & libFiles, std::vector < LibraryBinary > & libLibs, string platform = "", string arch = "", string target = "");
void getFrameworksRecursively(const fs::path & path, std::vector < string > & frameworks,  string platform = "" );
void getPropsRecursively(const fs::path & path, std::vector < fs::path > & props, const string & platform);
void getDllsRecursively(const fs::path & path, std::vector < string > & dlls, string platform);


void splitFromLast(string toSplit, string deliminator, string & first, string & second);
void splitFromFirst(string toSplit, string deliminator, string & first, string & second);

void fixSlashOrder(string & toFix);
string unsplitString (std::vector < string > strings, string deliminator );

//string getOFRelPath(const string & from);
//fs::path getOFRelPathFS(const fs::path & from);
fs::path getOFRelPath(const fs::path & from);

bool checkConfigExists();
bool askOFRoot();
string getOFRootFromConfig();

string getTargetString(ofTargetPlatform t);

std::unique_ptr<baseProject> getTargetProject(ofTargetPlatform targ);

template <class T>
inline bool isInVector(T item, std::vector<T> & vec){
	bool bIsInVector = false;
	for(int i=0;i<vec.size();i++){
		if(vec[i] == item){
			bIsInVector = true;
			break;
		}
	}
	return bIsInVector;
}

string colorText(const string & s, int color = 32);
void alert(string msg, int color=32);

bool ofIsPathInPath(const fs::path& basePath, const fs::path& subPath);


// fs::path subtract(const fs::path & base, const fs::path & sub) {

// }

#endif /* UTILS_H_ */
