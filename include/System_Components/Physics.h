#pragma once
#ifndef PHYSICS_H
#define PHYSICS_H

#include "System_Components/ECS.h"

class Physics {
public: 
	Physics() {
		std::cout << "Constructing Physics Engine" << std::endl;
	};

	//Add components
	void addComponent(Component component) {
		trackedComponents.push_back(component);
	}

	//Update logic
	void updateComponents(float deltaTime) {
		std::cout << "Shit idk man some update logic eventually" << std::endl;
	};

	//Getters 
	std::vector<Component> getPhysicsComponents() { return trackedComponents; };
	
private: 
	std::vector<Component> trackedComponents;

};

#endif