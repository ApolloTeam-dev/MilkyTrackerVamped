/*
 *  fx/ParticleScene.h
 *
 *  Copyright 2009 Peter Barth
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PARTICLESCENE__H
#define PARTICLESCENE__H

#include "FXInterface.h"

class ParticleFX;
class TexturedGrid;

class ParticleScene : public FXInterface
{
private:
	float			time;
	ParticleFX		*particleFX;
	ParticleFX		*particleFun;
	TexturedGrid	*texturedGrid;

public:
	ParticleScene(int width, int height, int gridshift);
	~ParticleScene();

	void render(unsigned short* vscreen, unsigned int pitch);
	void update(float syncFrac);

};

#endif
