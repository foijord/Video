#include <boost/asio.hpp>

#include <ExamineMode.h>
#include <ShaderReload.h>

#include <Innovator/Nodes.h>
#include <Innovator/VideoNodes.h>
#include <Innovator/Window.h>
#include <Innovator/GroupNodes.h>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}
#include <iostream>


class AVException : public std::runtime_error {
public:
	AVException(int code) :
		runtime_error(av_make_error_string(this->error_string, AV_ERROR_MAX_STRING_SIZE, code))
	{}

	char error_string[AV_ERROR_MAX_STRING_SIZE];
};


#define THROW_ON_AV_ERROR(__function__) {	\
	int result = (__function__);			\
	if (result < 0) {						\
		throw AVException(result);			\
	}										\
}

int main(int argc, char* argv[])
{
	try {
		size_t verbose;
		std::string file;

		bpo::options_description help("Help options");
		help.add_options()
			("help", "produce help message");

		bpo::options_description generic("Generic options");
		generic.add_options()
			("verbose", bpo::value<size_t>(&verbose)->default_value(5), "set verbosity level [0-5]");

		bpo::options_description video("Video options");
		video.add_options()
			("file", bpo::value<std::string>(&file)->required(), "input media file (required)");

		bpo::options_description command_line_options;
		bpo::positional_options_description positional_options;

		positional_options.add("file", -1);

		command_line_options.add(help);
		command_line_options.add(generic);
		command_line_options.add(video);

		auto parsed_options = bpo::command_line_parser(argc, argv)
			.options(command_line_options)
			.positional(positional_options)
			.run();

		bpo::variables_map vm;
		bpo::store(parsed_options, vm);

		if (vm.count("help")) {
			std::stringstream message;
			message << command_line_options;
			throw std::runtime_error(message.str());
		}

		bpo::notify(vm);

		boost::log::add_common_attributes();

		switch (verbose) {
		case 0: boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::fatal); break;
		case 1: boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::error); break;
		case 2: boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning); break;
		case 3: boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info); break;
		case 4: boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug); break;
		case 5: boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace); break;
		default: std::cerr << "Invalid verbosity level: " << verbose << std::endl; break;
		}

		auto console_log = boost::log::add_console_log(
			std::cout,
			boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%");

		console_log->locked_backend()->auto_flush(true);

		BOOST_LOG_TRIVIAL(trace) << "file: " << file;

		AVFormatContext* av_format_context = nullptr;
		THROW_ON_AV_ERROR(avformat_open_input(&av_format_context, file.c_str(), nullptr, nullptr));
		THROW_ON_AV_ERROR(avformat_find_stream_info(av_format_context, nullptr));

		int video_stream_idx = av_find_best_stream(av_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
		if (video_stream_idx < 0) {
			const char* media_type_string = av_get_media_type_string(AVMEDIA_TYPE_VIDEO);
			throw std::runtime_error("could not find stream of type " + std::string(media_type_string));
		}

		auto video_stream = av_format_context->streams[video_stream_idx];

		auto av_packet = av_packet_alloc();
		if (!av_packet) {
			throw std::runtime_error("could not allocate av_packet");
		}

		//while (av_read_frame(av_format_context, av_packet) >= 0) {
		//	if (av_packet->stream_index == video_stream_idx) {
		//		BOOST_LOG_TRIVIAL(trace) << "reading frame";
		//	}
		//	av_packet_unref(av_packet);
		//}

		BOOST_LOG_TRIVIAL(trace) << "Media format: " << av_format_context->iformat->long_name << " (" << av_format_context->iformat->name << ")";

		VkExtent2D extent{
			.width = 1080,
			.height = 1080,
		};

		auto render_command = std::make_shared<RenderCommand>();
		render_command->children = {
			std::make_shared<SampleShading>(VK_TRUE, 0.9f),
			std::make_shared<RasterizationSamples>(VK_SAMPLE_COUNT_8_BIT),
			std::make_shared<VideoEncodeH264>()
		};

		auto framebuffer_object = std::make_shared<Separator>();
		framebuffer_object->children = {
			std::make_shared<ColorAttachmentReference>(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
			std::make_shared<DepthStencilAttachmentReference>(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
			std::make_shared<ResolveAttachmentReference>(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
			std::make_shared<ColorAttachmentDescription>(VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_8_BIT),
			std::make_shared<DepthStencilAttachmentDescription>(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_SAMPLE_COUNT_8_BIT),
			std::make_shared<ColorAttachmentDescription>(VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT),
			std::make_shared<SubpassDescription>(VK_PIPELINE_BIND_POINT_GRAPHICS),
			std::make_shared<Renderpass>(),

			std::make_shared<PipelineColorBlendAttachmentState>(),
			std::make_shared<ClearValue>(VkClearValue{.color = { 1.0f, 1.0f, 1.0f, 0.0f } }),
			std::make_shared<ColorAttachment>(VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_8_BIT),
			std::make_shared<ClearValue>(VkClearValue{.depthStencil = { 1.0f, 0 } }),
			std::make_shared<DepthStencilAttachment>(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_SAMPLE_COUNT_8_BIT),
			std::make_shared<ClearValue>(VkClearValue{.color = { 1.0f, 1.0f, 1.0f, 0.0f } }),
			std::make_shared<ColorAttachment>(VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT),
			std::make_shared<Framebuffer>(),

			render_command,
		};

		boost::asio::io_context io_context;
		auto work_guard = boost::asio::make_work_guard(io_context);

		ExamineMode mode;
		NativeWindow window(io_context, extent, framebuffer_object);
#ifdef DEBUG
		ShaderReload shader_reload(io_context, framebuffer_object);
#endif
		return window.show();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
