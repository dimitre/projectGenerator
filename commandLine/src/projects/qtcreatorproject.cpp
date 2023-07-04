#include "qtcreatorproject.h"
#include "ofLog.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include <regex>

std::string QtCreatorProject::LOG_NAME = "QtCreatorProject";

QtCreatorProject::QtCreatorProject(std::string target)
	: baseProject(target){
}

bool QtCreatorProject::createProjectFile(){
	
	if (!fs::exists(projectDir)) {
		fs::create_directory(projectDir);
	}
//	ofDirectory dir(projectDir);
//	if(!dir.exists()) dir.create(true);

//	alert("project " + project.string());
	
	// FIXME: modularize the sequence of files to copy there.
	
	fs::path src = templatePath / "qtcreator.qbs";
	fs::path dst = projectDir / (projectName + ".qbs");

	if (!fs::exists(dst)) {
		try {
			fs::copy_file(src, dst);
		} catch(fs::filesystem_error& e) {
			ofLogError(LOG_NAME) << "error copying qbs template from " << src << " to " << dst;
			return false;
		}
		findandreplaceInTexfile(dst, "emptyExample", projectName);
	}
	
	vector < std::pair <fs::path, fs::path > > fromTo {
		{ "qtcreator.qbs", fs::path { projectName + ".qbs" } },
		{ "Makefile", "Makefile" },
		{ "config.make", "config.make" },
	};
	
	for (auto & p : fromTo) {
		fs::path src = templatePath / p.first;
		if(!fs::exists(dst)){
			fs::path dst = projectDir / p.second;
			
			alert("fromTo");
			cout << src << endl;
			cout << dst << endl;
			cout << "---" << endl;
//			try {
//				fs::copy_file(src, dst);
//			} catch(fs::filesystem_error& e) {
//				ofLogError(LOG_NAME) << "error copying Makefile template from " << src << " to " << dst;
//				return false;
//			}
		}
	}
	

//	dst = projectDir / "Makefile";
//	src = templatePath / "Makefile";
//	if(!fs::exists(dst)){
//		try {
//			fs::copy_file(src, dst);
//		} catch(fs::filesystem_error& e) {
//			ofLogError(LOG_NAME) << "error copying Makefile template from " << src << " to " << dst;
//			return false;
//		}
//	}
//
//	dst = projectDir / "config.make";
//	src = templatePath / "config.make";
//	if(!fs::exists(dst)){
//		try {
//			fs::copy_file(src, dst);
//		} catch(fs::filesystem_error& e) {
//			ofLogError(LOG_NAME) << "error copying config.make template from " << src << " to " << dst;
//			return false;
//		}
//	}


	// handle the relative roots.
	// FIXME: Move to baseproject?
	// FIXME: FS
	fs::path relRoot = getOFRelPath(projectDir);
	if (!fs::equivalent(relRoot, "../../..")) {
		std::string relPath2 = relRoot.string();
		
		// FIXME: this doesn't seem OK
		relPath2.erase(relPath2.end()-1);
		findandreplaceInTexfile(projectDir / "qtcreator.qbs", "../../..", relPath2);
		findandreplaceInTexfile(projectDir / "Makefile", "../../..", relPath2);
		findandreplaceInTexfile(projectDir / "config.make", "../../..", relPath2);
	}

	return true;
}

void QtCreatorProject::addSrc(std::string srcFile, std::string folder, baseProject::SrcType type){
	qbsProjectFiles.insert(srcFile);
}

bool QtCreatorProject::loadProjectFile(){

// FIXME: FS
//	ofFile project(projectDir / (projectName + ".qbs"), ofFile::ReadOnly,true);

	fs::path file { projectDir / (projectName + ".qbs") };
	if (!fs::exists(file)) {
		ofLogError(LOG_NAME) << "error loading" << file << "doesn't exist";
		return false;
	}
	
	std::ifstream project(file);
	auto ret = qbs.set(project);
	// parse files in current .qbs
	std::regex filesregex("files[ \t\r\n]*:[ \t\r\n]*\\[[ \t\r\n]*([\"'][^\\]\"']*[\"'][ \t\r\n]*,?[ \t\r\n]*)*\\]");
	std::smatch matches;
	std::string qbsStr = qbs.getText();
	if(std::regex_search(qbsStr, matches, filesregex)){
		std::string fullmatch = matches[0];
		originalFilesStr = fullmatch;
		while(std::regex_search(fullmatch, matches, std::regex("[ \t\r\n]*[\"']([^\\]\"']*)[\"'][ \t\r\n]*,?"))){
			qbsProjectFiles.insert(matches[1]);
			fullmatch = matches.suffix().str();
		}
	}

	// parse addons in current .qbs
	std::regex addonsregex("of\\.addons[ \t\r\n]*:[ \t\r\n]*\\[[ \t\r\n]*([\"'][^\\]\"']*[\"'][ \t\r\n]*,?[ \t\r\n]*)*\\]");
	if(std::regex_search(qbsStr, matches, addonsregex)){
		std::string fullmatch = matches[0];
		originalAddonsStr = fullmatch;
		while(std::regex_search(fullmatch, matches, std::regex("[ \t\r\n]*[\"']([^\\]\"']*)[\"'][ \t\r\n]*,?"))){
			bool alreadyExists=false;
			for(auto & a: addons){
				auto addonStr = a.isLocalAddon ? a.addonPath.string() : a.name;
				if(addonStr==matches[1]){
					alreadyExists = true;
					break;
				}
			}
			if(!alreadyExists){
				addAddon(matches[1].str());
				fullmatch = matches.suffix().str();
			}
		}
	}
	return ret;
}

bool QtCreatorProject::saveProjectFile(){
	auto qbsStr = qbs.getText();

	// create files str with all files
	std::string filesStr = "files: [\n";
	for(auto & f: qbsProjectFiles){
		filesStr += "            '" + f + "',\n";
	}
	filesStr += "        ]";

	// create addons str with all addons
	std::string addonsStr = "of.addons: [\n";
	for(auto & a: addons){
		auto addonStr = a.isLocalAddon ? a.addonPath.string() : a.name;
		addonsStr += "            '" + addonStr + "',\n";
	}
	addonsStr += "        ]";

	// if there were no addons str just append it to files
	if(originalAddonsStr!=""){
		ofStringReplace(qbsStr,originalAddonsStr,addonsStr);
	}else{
		filesStr += "\n\n        " + addonsStr;
	}
	if(originalFilesStr!=""){
		ofStringReplace(qbsStr,originalFilesStr,filesStr);
	}

	// save final project
	qbs.set(qbsStr);
	// FIXME: FS

	ofFile project(projectDir / (projectName + ".qbs"),ofFile::WriteOnly,true);
	project.writeFromBuffer(qbs);
	return true;
}

void QtCreatorProject::addAddon(ofAddon & addon){
	for(int i=0;i<(int)addons.size();i++){
		if(addons[i].name==addon.name) return;
	}

	addons.emplace_back(addon);
}
