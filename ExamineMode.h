#pragma once

#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <Innovator/Visitor.h>
#include <Innovator/Nodes.h>
#include <Innovator/GroupNodes.h>

class ExamineMode {
public:
	ExamineMode()
	{
		eventvisitor.register_callback<ViewMatrix>(
			[this](ViewMatrix* camera, State* state)
			{
				if (eventvisitor.event.type() == typeid(MouseButtonEvent)) {
					auto event = std::any_cast<MouseButtonEvent>(eventvisitor.event);
					this->button = (event.action == GLFW_PRESS) ? event.button : -1;
				}

				if (eventvisitor.event.type() == typeid(MouseMoveEvent)) {
					auto event = std::any_cast<MouseMoveEvent>(eventvisitor.event);
					glm::dvec2 dx = this->position - event.position;
					this->position = event.position;

					auto& extent = ExtentElement.get(state);
					dx.x /= extent.width;
					dx.y /= extent.height;

					switch (this->button) {
					case GLFW_MOUSE_BUTTON_LEFT: camera->orbit(dx); break;
					case GLFW_MOUSE_BUTTON_MIDDLE: camera->pan(dx); break;
					case GLFW_MOUSE_BUTTON_RIGHT: camera->zoom(dx[1]); break;
					default: break;
					}
				}
			});
	}

	int button = -1;
	glm::dvec2 position{ 0 };
};
