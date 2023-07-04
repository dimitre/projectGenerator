


#include "visualStudioProject.h"
#include "Utils.h"

string visualStudioProject::LOG_NAME = "visualStudioProjectFile";


bool visualStudioProject::createProjectFile(){
//	alert("visualStudioProject::createProjectFile");

	solution	= projectDir / (projectName + ".sln");
	fs::path project 	= projectDir / (projectName + ".vcxproj");
	fs::path user 		= projectDir / (projectName + ".vcxproj.user");
	fs::path filters	= projectDir / (projectName + ".vcxproj.filters");

	fs::copy(templatePath / "emptyExample.vcxproj", 		project, fs::copy_options::overwrite_existing);
	fs::copy(templatePath / "emptyExample.vcxproj.user", 	user, fs::copy_options::overwrite_existing);
	fs::copy(templatePath / "emptyExample.sln", 			solution, fs::copy_options::overwrite_existing);
	fs::copy(templatePath / "emptyExample.vcxproj.filters", filters, fs::copy_options::overwrite_existing);

	fs::copy(templatePath / "icon.rc", projectDir / "icon.rc", fs::copy_options::overwrite_existing);

	pugi::xml_parse_result result = filterXmlDoc.load_file(filters.c_str());
	if (result.status==pugi::status_ok) {
		ofLogVerbose() << "loaded filter ";
	} else {
		ofLogVerbose() << "problem loading filter ";
	}

	findandreplaceInTexfile(solution, "emptyExample", projectName);
	findandreplaceInTexfile(user, "emptyExample", projectName);
	findandreplaceInTexfile(project, "emptyExample", projectName);

	fs::path relRoot = getOFRelPath(projectDir);

	if (!fs::equivalent(relRoot, "../../..")) {
		// let's make it windows friendly:
		string relRootWindows = convertStringToWindowsSeparator(relRoot.string());

		// sln has windows paths:
		findandreplaceInTexfile(solution, "..\\..\\..\\", relRootWindows);

		// vcx has unixy paths:
		//..\..\..\libs
		findandreplaceInTexfile(project, "..\\..\\..\\", relRoot.string());
	} else {
//		cout << "equivalent to default ../../.." << endl;
	}

	return true;
}


bool visualStudioProject::loadProjectFile(){
	fs::path projectPath { projectDir / (projectName + ".vcxproj") };
//	cout << "projectDir " << projectDir << endl;
//	cout << "projectName " << projectName << endl;
//	cout << "projectPath " << projectPath << endl;
	if (!fs::exists(projectPath)) {
		ofLogError(LOG_NAME) << "error loading " << projectPath << " doesn't exist";
		return false;
	}
	pugi::xml_parse_result result = doc.load_file(projectPath.c_str());
	bLoaded = result.status==pugi::status_ok;
	return bLoaded;
}


bool visualStudioProject::saveProjectFile(){
	if (!additionalvcxproj.empty()) {
		string additionalProjects;
//		string divider = "\r\n";
		string divider = "\n";
		for (auto & a : additionalvcxproj) {
			string name = a.filename().stem().string();
			string aString = a.string();
			std::replace (aString.begin(), aString.end(), '/', '\\');
			string uuid = generateUUID(name);
			additionalProjects +=
			"Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \""+name+"\", \""+aString+"\", \"{"+uuid+"}\"" +
			divider + "EndProject" + divider;
		}
//		string findString = "Global" + divider;
		string findString = "Global";
		
		additionalProjects += findString ;
		
		solution = solution.lexically_normal();
//		findandreplaceInTexfile(solution, findString, additionalProjects);

		std::ifstream file(solution);
		std::stringstream buffer;
		buffer << file.rdbuf();
		string str = buffer.str();
		file.close();

		std::size_t pos = str.find(findString);
		if (pos != std::string::npos) {
			str.replace(pos, findString.length(), additionalProjects);
		}

		std::ofstream myfile(solution);
		myfile << str;
		myfile.close();
		
		
//		cout << fs::current_path() << endl;
//		cout << fs::absolute(solution) << endl;
//		cout << solution.lexically_normal() << endl;
	}
	
	
	auto filters = projectDir / (projectName + ".vcxproj.filters");
	filterXmlDoc.save_file(filters.c_str());

	auto vcxFile = projectDir / (projectName + ".vcxproj");
	return doc.save_file(vcxFile.c_str());
	
	/*
	 PSEUDOCODE HERE
	 open sln project
	 find position of first "Global" word, put cursor behind
	 add one entry for each additional, fixing slashes, generating new uuid.
	 Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "openframeworksLib", "..\..\..\libs\openFrameworksCompiled\project\vs\openframeworksLib.vcxproj", "{5837595D-ACA9-485C-8E76-729040CE4B0B}"
	 EndProject
	*/
	

}

void visualStudioProject::appendFilter(string folderName){
	fixSlashOrder(folderName);
	string uuid = generateUUID(folderName);
	string tag = "//ItemGroup[Filter]/Filter[@Include=\"" + folderName + "\"]";
	pugi::xpath_node_set set = filterXmlDoc.select_nodes(tag.c_str());
	if (set.size() > 0){
	//pugi::xml_node node = set[0].node();
	} else {

		pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[Filter]/Filter").node().parent();
		pugi::xml_node nodeAdded = node.append_child("Filter");
		nodeAdded.append_attribute("Include").set_value(folderName.c_str());
		pugi::xml_node nodeAdded2 = nodeAdded.append_child("UniqueIdentifier");

		 uuid.insert(8,"-");
		 uuid.insert(8+4+1,"-");
		 uuid.insert(8+4+4+2,"-");
		 uuid.insert(8+4+4+4+3,"-");

		//d8376475-7454-4a24-b08a-aac121d3ad6f

		string uuidAltered = "{" + uuid + "}";
		nodeAdded2.append_child(pugi::node_pcdata).set_value(uuidAltered.c_str());
	 }
}


void visualStudioProject::addSrc(string srcFile, string folder, SrcType type){
//	alert("addSrc " + srcFile, 35);
	string original = srcFile ;
	fixSlashOrder(folder);
	fixSlashOrder(srcFile);
	
	// I had an empty ClCompile field causing errors
	if (srcFile == "") {
		cout << "NOOOOO empty " << folder << " : " << original << endl;
		return;
	}

	std::vector < string > folderSubNames = ofSplitString(folder, "\\");
	string folderName = "";
	for (int i = 0; i < folderSubNames.size(); i++){
		if (i != 0) folderName += "\\";
		folderName += folderSubNames[i];
		appendFilter(folderName);
	}

	if(type==DEFAULT){
		if (ofIsStringInString(srcFile, ".h") || ofIsStringInString(srcFile, ".hpp") || ofIsStringInString(srcFile, ".inl")){
			appendValue(doc, "ClInclude", "Include", srcFile);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[ClInclude]").node();
			pugi::xml_node nodeAdded = node.append_child("ClInclude");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());

		} else if (ofIsStringInString(srcFile, ".glsl") || ofIsStringInString(srcFile, ".vert") || ofIsStringInString(srcFile, ".frag")) {
			// TODO: add to None but there's no None in the original template so this fails
			/*appendValue(doc, "None", "Include", srcFile);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[None]").node();
			pugi::xml_node nodeAdded = node.append_child("None");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());*/

		} else{
			appendValue(doc, "ClCompile", "Include", srcFile);

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
		}
	}else{
		switch(type){
		case CPP:{
			appendValue(doc, "ClCompile", "Include", srcFile);

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
			break;
		}
		case C:{
			pugi::xml_node node = appendValue(doc, "ClCompile", "Include", srcFile);

			if(!node.child("CompileAs")){
				pugi::xml_node compileAs = node.append_child("CompileAs");
				compileAs.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
				compileAs.set_value("Default");

				compileAs = node.append_child("CompileAs");
				compileAs.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|x64'");
				compileAs.set_value("Default");
			}

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
			break;
		}
		case HEADER:{
			appendValue(doc, "ClInclude", "Include", srcFile);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[ClInclude]").node();
			pugi::xml_node nodeAdded = node.append_child("ClInclude");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
			break;
		}
		case OBJC:{
			ofLogError() << "objective c type not supported on vs for " << srcFile;
			break;
		}
		default:{
			ofLogError() << "explicit source type " << type << " not supported yet on osx for " << srcFile;
			break;
		}
		}
	}
}

void visualStudioProject::addInclude(string includeName){
	fixSlashOrder(includeName);

	pugi::xpath_node_set source = doc.select_nodes("//ClCompile/AdditionalIncludeDirectories");
	for (pugi::xpath_node_set::const_iterator it = source.begin(); it != source.end(); ++it){
		pugi::xpath_node node = *it;
		string includes = node.node().first_child().value();
		std::vector < string > strings = ofSplitString(includes, ";");
		bool bAdd = true;
		for (int i = 0; i < (int)strings.size(); i++){
			if (strings[i].compare(includeName) == 0){
				bAdd = false;
			}
		}
		if (bAdd == true){
			strings.emplace_back(includeName);
			string includesNew = unsplitString(strings, ";");
			node.node().first_child().set_value(includesNew.c_str());
		}

	}
	//appendValue(doc, "Add", "directory", includeName);
}

void addLibraryPath(const pugi::xpath_node_set & nodes, string libFolder) {
	for (auto & node : nodes) {
		string includes = node.node().first_child().value();
		std::vector < string > strings = ofSplitString(includes, ";");
		bool bAdd = true;
		for (int i = 0; i < (int)strings.size(); i++) {
			if (strings[i].compare(libFolder) == 0) {
				bAdd = false;
			}
		}
		if (bAdd == true) {
			strings.emplace_back(libFolder);
			string libPathsNew = unsplitString(strings, ";");
			node.node().first_child().set_value(libPathsNew.c_str());
		}
	}
}

void addLibraryName(const pugi::xpath_node_set & nodes, string libName) {
	for (auto & node : nodes) {
		string includes = node.node().first_child().value();
		std::vector < string > strings = ofSplitString(includes, ";");
		bool bAdd = true;
		for (int i = 0; i < (int)strings.size(); i++) {
			if (strings[i].compare(libName) == 0) {
				bAdd = false;
			}
		}

		if (bAdd == true) {
			strings.emplace_back(libName);
			string libsNew = unsplitString(strings, ";");
			node.node().first_child().set_value(libsNew.c_str());
		}
	}
}

void visualStudioProject::addProps(fs::path propsFile){
//	alert ("visualStudioProject::addProps " + propsFile.string());
	string path = propsFile.string();
//	path.replace(path.find("/"), 1, "\\");


//	std::cout << ">>>>> visualStudioProject::addProps " << propsFile << std::endl;
	pugi::xpath_node_set items = doc.select_nodes("//ImportGroup");
	for (int i = 0; i < items.size(); i++) {
		pugi::xml_node additionalOptions;
//		cout << "adding path " << propsFile << endl;
//		cout << "adding path c_str " << propsFile.c_str() << endl;
//		cout << "adding path " << path << endl;
//		cout << "adding path c_str " << path.c_str() << endl;

		items[i].node().append_child("Import").append_attribute("Project").set_value(path.c_str());
//		items[i].node().append_child("Import").append_attribute("Project").set_value(propsFile);
//		cout << i << endl;
//		cout << items[i].node().name() << endl;
//		cout << items[i].node().value() << endl;
	}
//	auto check = doc.select_nodes("//ImportGroup/Import/Project");
}

void visualStudioProject::addLibrary(const LibraryBinary & lib) {
	
	// TODO: in future change Library to FS
	auto libraryName = fs::path { lib.path };
//	fixSlashOrder(libraryName);

	// ok first, split path and library name.
//	size_t found = libraryName.find_last_of("\\");
//	string libFolder = libraryName.substr(0, found);
	auto libFolder = libraryName.parent_path();
	auto libName = libraryName.filename();

	// ---------| invariant: libExtension is `lib`

	// paths for libraries
	string linkPath;
	if (!lib.target.empty() && !lib.arch.empty()) {
		linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.target + "') and contains(@Condition,'" + lib.arch + "')]/Link/";
	}
	else if (!lib.target.empty()) {
		linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.target + "')]/Link/";
	}
	else if (!lib.arch.empty()) {
		linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.arch + "')]/Link/";
	}
	else {
		linkPath = "//ItemDefinitionGroup/Link/";
	}

	if (!libFolder.empty()) {
		pugi::xpath_node_set addlLibsDir = doc.select_nodes((linkPath + "AdditionalLibraryDirectories").c_str());
		addLibraryPath(addlLibsDir, libFolder.string());
	}

	pugi::xpath_node_set addlDeps = doc.select_nodes((linkPath + "AdditionalDependencies").c_str());
	addLibraryName(addlDeps, libName.string());

	ofLogVerbose() << "adding lib path " << libFolder;
	ofLogVerbose() << "adding lib " << libName;
}

void visualStudioProject::addCFLAG(string cflag, LibType libType){
	pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	for(int i=0;i<items.size();i++){
		pugi::xml_node additionalOptions;
		bool found=false;
		string condition(items[i].node().attribute("Condition").value());
		if (libType == RELEASE_LIB && condition.find("Release") != string::npos) {
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}else if(libType==DEBUG_LIB && condition.find("Debug") != string::npos){
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		if(!found) continue;
		if(!additionalOptions){
			items[i].node().child("ClCompile").append_child("AdditionalOptions").append_child(pugi::node_pcdata).set_value(cflag.c_str());
		}else{
			additionalOptions.set_value((string(additionalOptions.value()) + " " + cflag).c_str());
		}
	}
}

void visualStudioProject::addCPPFLAG(string cppflag, LibType libType){
	pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	for(int i=0;i<items.size();i++){
		pugi::xml_node additionalOptions;
		bool found=false;
		string condition(items[i].node().attribute("Condition").value());
		if(libType==RELEASE_LIB && condition.find("Debug") != string::npos){
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}else if(libType==DEBUG_LIB && condition.find("Release") != string::npos){
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		if(!found) continue;
		if(!additionalOptions){
			items[i].node().child("ClCompile").append_child("AdditionalOptions").append_child(pugi::node_pcdata).set_value(cppflag.c_str());
		}else{
			additionalOptions.set_value((string(additionalOptions.value()) + " " + cppflag).c_str());
		}
	}
}

void visualStudioProject::addDefine(string define, LibType libType) {
	pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	for (int i = 0; i<items.size(); i++) {
		pugi::xml_node additionalOptions;
		bool found = false;
		string condition(items[i].node().attribute("Condition").value());
		if (libType == RELEASE_LIB && condition.find("Debug") != string::npos) {
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		else if (libType == DEBUG_LIB && condition.find("Release") != string::npos) {
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		if (!found) continue;
		if (!additionalOptions) {
			items[i].node().child("ClCompile").append_child("PreprocessorDefinitions").append_child(pugi::node_pcdata).set_value(define.c_str());
		}
		else {
			additionalOptions.set_value((string(additionalOptions.value()) + " " + define).c_str());
		}
	}
}

void visualStudioProject::addAddon(ofAddon & addon) {
	alert ("visualStudioProject::addAddon " + addon.name);
	
//	fs::path additionalFolder = addon.addonPath / (addon.name + "Lib");
//	if (fs::exists(additionalFolder)) {
//		alert("Additional! " + additionalFolder.string(), 34);
//		for (const auto & entry : fs::directory_iterator(additionalFolder)) {
//			auto f = entry.path();
//			alert(f.string());
//			if (f.extension() == ".vcxproj") {
//				additionalvcxproj.emplace_back(f);
//				alert("VCSPROJ Exists " + f.string(), 35);
//			}
//		}
//	}
//	
	
	for (auto & a : addons) {
		if (a.name == addon.name) return;
	}

	for (auto & d : addon.dependencies) {
		baseProject::addAddon(d);
	}

//	cout << "-x-x-x-" << endl;
//	for (auto & s : addon.srcFiles) {
//		cout << s << endl;
//	}
//	cout << "-x-x-x-" << endl;
	
	ofLogNotice() << "adding addon: " << addon.name;
	addons.push_back(addon);

	for (auto & e : addon.includePaths) {
		ofLogVerbose() << "adding addon include path: " << e;
		addInclude(e);
	}

	// temporary removal
	for (auto & props : addon.propsFiles) {
		ofLogVerbose() << "adding addon props: " << props;
		addProps(props);
	}

	for(auto & lib: addon.libs){
		ofLogVerbose() << "adding addon libs: " << lib.path;
		addLibrary(lib);
	}

	
	for (auto & s : addon.srcFiles) {
		ofLogVerbose() << "adding addon srcFiles: " << s;
		if ( addon.filesToFolders[s] == "" ) {
			addon.filesToFolders[s] = "other";
		}
		
//		cout << "addSrc s=" << s << " : " << addon.filesToFolders[s] << endl;
		addSrc(s,addon.filesToFolders[s]);
	}


	for(int i=0;i<(int)addon.csrcFiles.size(); i++){
		ofLogVerbose() << "adding addon c srcFiles: " << addon.csrcFiles[i];
		if(addon.filesToFolders[addon.csrcFiles[i]]=="") addon.filesToFolders[addon.csrcFiles[i]]="other";
		addSrc(addon.csrcFiles[i],addon.filesToFolders[addon.csrcFiles[i]],C);
	}

	for(int i=0;i<(int)addon.cppsrcFiles.size(); i++){
		ofLogVerbose() << "adding addon cpp srcFiles: " << addon.cppsrcFiles[i];
		if(addon.filesToFolders[addon.cppsrcFiles[i]]=="") addon.filesToFolders[addon.cppsrcFiles[i]]="other";
		addSrc(addon.cppsrcFiles[i],addon.filesToFolders[addon.cppsrcFiles[i]],C);
	}

	for(int i=0;i<(int)addon.headersrcFiles.size(); i++){
		ofLogVerbose() << "adding addon header srcFiles: " << addon.headersrcFiles[i];
		if(addon.filesToFolders[addon.headersrcFiles[i]]=="") addon.filesToFolders[addon.headersrcFiles[i]]="other";
		addSrc(addon.headersrcFiles[i],addon.filesToFolders[addon.headersrcFiles[i]],C);
	}

	for(int i=0;i<(int)addon.objcsrcFiles.size(); i++){
		ofLogVerbose() << "adding addon objc srcFiles: " << addon.objcsrcFiles[i];
		if(addon.filesToFolders[addon.objcsrcFiles[i]]=="") addon.filesToFolders[addon.objcsrcFiles[i]]="other";
		addSrc(addon.objcsrcFiles[i],addon.filesToFolders[addon.objcsrcFiles[i]],C);
	}

	for (auto & d : addon.dllsToCopy) {
		ofLogVerbose() << "adding addon dlls to bin: " << d;
		fs::path from = addon.addonPath / d;
		fs::path to = projectDir / "bin" / from.filename();
//		alert("copy from to " + from.string() + " : " + to.string());
		fs::copy_file(from, to, fs::copy_options::overwrite_existing);
	}

	for(int i=0;i<(int)addon.cflags.size();i++){
		ofLogVerbose() << "adding addon cflags: " << addon.cflags[i];
		addCFLAG(addon.cflags[i],RELEASE_LIB);
		addCFLAG(addon.cflags[i],DEBUG_LIB);
	}

	for(int i=0;i<(int)addon.cppflags.size();i++){
		ofLogVerbose() << "adding addon cppflags: " << addon.cppflags[i];
		addCPPFLAG(addon.cppflags[i],RELEASE_LIB);
		addCPPFLAG(addon.cppflags[i],DEBUG_LIB);
	}

	for (int i = 0; i<(int)addon.defines.size(); i++) {
		ofLogVerbose() << "adding addon define: " << addon.defines[i];
		addDefine(addon.defines[i], RELEASE_LIB);
		addDefine(addon.defines[i], DEBUG_LIB);
	}
}
