#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/GroupNodes.h>
#include <Innovator/Sensors.h>

#include <map>
#include <filesystem>


class ShaderReload {
public:
	ShaderReload(
		boost::asio::io_context& io_context,
		std::shared_ptr<Node> root) :
		root(std::move(root))
	{
		auto visit_group = [this](Group* node, State* state) { Group::visit(node, &this->shadervisitor, state); };
		auto visit_separator = [this](Separator* node, State* state) { Separator::visit(node, &this->shadervisitor, state); };

		this->shadervisitor.register_callback<Group>(visit_group);
		this->shadervisitor.register_callback<Separator>(visit_separator);
		this->shadervisitor.register_callback<RenderCommand>(visit_separator);
		this->shadervisitor.register_callback<FramebufferObject>(visit_separator);

		this->shadervisitor.register_callback<Shader>(
			[this](Shader* shader, State* state)
			{
				if (!this->last_write_times.contains(shader)) {
					this->last_write_times[shader] = std::filesystem::last_write_time(shader->filename);
				}

				auto last_write_time = std::filesystem::last_write_time(shader->filename);
				if (last_write_time != this->last_write_times.at(shader)) {
					this->last_write_times[shader] = last_write_time;
					try {
						shader->visit(&allocvisitor, state);
						this->root->visit(&pipelinevisitor, state);
						this->root->visit(&recordvisitor, state);
					}
					catch (std::exception& e) {
						std::cerr << e.what() << std::endl;
					}
				}
			});

		auto sensor = std::make_shared<TimerSensor>(
			io_context,
			[this]()
			{
				this->root->visit(&this->shadervisitor, &state);
			});
		sensor->schedule();
	}


	Visitor shadervisitor;
	boost::asio::io_context io_context;
	std::shared_ptr<Node> root;
	std::map<Shader*, std::filesystem::file_time_type> last_write_times;
};
