#pragma once
#ifndef IO_H
#define IO_H

#include "config.h"
#include "Camera.h"

class IO {
public:
	static IO* inputInstance; 

	IO(std::shared_ptr<Camera> camera): worldCamera(camera) {
		inputInstance = this;
	}

	//Main mouse/key callbacks -> uses singleton pointer to execute callback
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (inputInstance) {
			inputInstance->onKey(window, key, scancode, action, mods);
		}
	}

	static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
		if (inputInstance) {
			inputInstance->onMouseMove(window, xpos, ypos);
		}
	}

	//Sets up standard movement and rotation(not yet) binds
	void setStandardBinds() {
		float moveSpeed = 0.01f; 

		//Set WASD binds 
		keyBindings[GLFW_KEY_W] = [this, moveSpeed, key = GLFW_KEY_W]() {
			worldCamera->move(key, moveSpeed);
		};

		keyBindings[GLFW_KEY_A] = [this, moveSpeed, key = GLFW_KEY_A]() {
			worldCamera->move(key, moveSpeed);
		};

		keyBindings[GLFW_KEY_D] = [this, moveSpeed, key = GLFW_KEY_D]() {
			worldCamera->move(key, moveSpeed);
		};

		keyBindings[GLFW_KEY_S] = [this, moveSpeed, key = GLFW_KEY_S]() {
			worldCamera->move(key, moveSpeed);
		};

		keyBindings[GLFW_KEY_SPACE] = [this, moveSpeed, key = GLFW_KEY_SPACE]() {
			worldCamera->move(key, moveSpeed);
		};

		keyBindings[GLFW_KEY_LEFT_CONTROL] = [this, moveSpeed, key = GLFW_KEY_LEFT_CONTROL]() {
			worldCamera->move(key, moveSpeed);
		};
	}

	//This function allows a user to bind a function to a specific key
	void bindKey(int key, std::function<void()> func) {
		keyBindings[key] = func;
	}

	void unbindKey(int key) {
		keyBindings.erase(key);
	}

	//This function checks that key exists in keyBindings and set its press state
	void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			auto bind = keyBindings.find(key);
			if (bind != keyBindings.end()) {
				isKeyPressed[key] = true;
			}
			else {
				std::cout << "Key not bound" << std::endl;
			}
		}
		else if (action == GLFW_RELEASE) {
			isKeyPressed[key] = false;
		}
	}

	void onMouseMove(GLFWwindow* window, double xpos, double ypos) {
		if (firstMouse) {
			lastX = xpos;
			lastY = ypos; 
			firstMouse = false; 
		}

		float xOffset = xpos - lastX; 
		float yOffset = ypos - lastY; 

		lastX = xpos; 
		lastY = ypos; 

		float sensitivity = 0.1f;

		xOffset *= sensitivity; 
		yOffset *= sensitivity;

		worldCamera->processMouse(xOffset, yOffset);
	}


	//This function checks each key's press state per frame, running it's bound function if pressed 
	void pollKeyBinds() {
		for (const auto& [key, pressed] : isKeyPressed) {
			if (pressed) {
				auto it = keyBindings.find(key);
				if (it != keyBindings.end()) {
					it->second();
				}
			}
		}
	}

private:
	std::unordered_map<int, std::function<void()>> keyBindings;
	std::unordered_map<int, bool> isKeyPressed; 

	bool firstMouse = true; 
	float lastX = 400, lastY = 300; 

	std::shared_ptr<Camera> worldCamera;
};


#endif