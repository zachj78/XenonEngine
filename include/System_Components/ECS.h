#pragma once
#ifndef ECS_H
#define ECS_H

#include "Utils/config.h"

struct Transform {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

struct Component {
	std::string meshName;
	std::array<float, 2> velocity;
	Transform transform;
};

#endif