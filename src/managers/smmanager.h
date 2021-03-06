/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.
 
    Copyright (c) 2011-2018 by Cao Jiayin - All rights reserved.
 
    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 
    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#pragma once

#include "utility/singleton.h"
#include <unordered_map>
#include "platform/sharedmemory/sharedmemory.h"

/////////////////////////////////////////////////////////////
// definition of shared memory manager
class	SMManager : public Singleton<SMManager>
{
public:
	// destructor
	~SMManager();

	// Initialize shared memory
	SharedMemory CreateSharedMemory(const string& sm_name, int length, unsigned type);

	// Release share memory resource
	void ReleaseSharedMemory(const string& sm_name);

	// Get Shared Memory
	SharedMemory GetSharedMemory(const string& sm_name);

private:
	// the map for shared memory
	std::unordered_map< string, PlatformSharedMemory > m_SharedMemory;

	// private constructor
	SMManager();

	friend class Singleton<SMManager>;
};
