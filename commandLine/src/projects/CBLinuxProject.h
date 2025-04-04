/*
 * CBLinuxProject.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */
#pragma once

#include "CBWinProject.h"
#include "LibraryBinary.h"

class CBLinuxProject: public CBWinProject {
public:
	CBLinuxProject(const std::string & target) : CBWinProject(target) {};

    bool createProjectFile() override;
    void addInclude(const fs::path & includeName) override{};
    void addLibrary(const LibraryBinary & lib)override{};

	static std::string LOG_NAME;
};
