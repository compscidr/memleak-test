#include <future>
#include <malloc.h>
#include <grpcpp/server_builder.h>
#include <iostream>
#include <test.grpc.pb.h>
#include <test.pb.h>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::sinks::ansicolor_stdout_sink_mt> make_color_console_sink() {
    // TODO(jayv) multi-thread safe sink, we can revisit if all core components work single-threaded
    auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>(spdlog::color_mode::automatic);
    if (color_sink->should_color()) {
        color_sink->set_color(spdlog::level::level_enum::trace, "\u001b[36m");     // cyan
        color_sink->set_color(spdlog::level::level_enum::debug, "\u001b[34m");     // blue
        color_sink->set_color(spdlog::level::level_enum::info, "\u001b[32m");      // green
        color_sink->set_color(spdlog::level::level_enum::warn, "\u001b[33m");      // yellow
        color_sink->set_color(spdlog::level::level_enum::err, "\u001b[31m");       // red
        color_sink->set_color(spdlog::level::level_enum::critical, "\u001b[35m");  // magenta
        color_sink->set_pattern("[%H:%M:%S.%e] [thread-%t] [%4n] [%^%8l%$] %v");
    } else {
        color_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread-%t] [%4n] [%8l] %v");
    }
    return color_sink;
}

std::shared_ptr<spdlog::logger> make_logger(const std::string& name) {
    static auto color_sink = make_color_console_sink();
    static auto level = spdlog::level::debug;
    auto the_logger = std::make_shared<spdlog::logger>(name, color_sink);
    the_logger->set_level(level);
    the_logger->info("Logger {} initialized with level: {}", name, spdlog::level::to_string_view(level));
    return the_logger;
}

std::shared_ptr<spdlog::logger> logger = make_logger("test");

class IoServiceImpl final : public IOService::Service {
public:
    grpc::Status GetIO(grpc::ServerContext* context, const google::protobuf::Empty* request, IOPointList* response) override {
        logger->debug("IN HERE");
        auto info = mallinfo();
        logger->debug("USED HEAP UMS: {}, USED HEAP UORD: {}", info.usmblks, info.uordblks);
        context->TryCancel();
        return grpc::Status::OK;
        /*
        abort();
        return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");*/
    }
};

int main() {
    IoServiceImpl ioService;
    grpc::ServerBuilder builder;
    std::string hostname("0.0.0.0:1111");

    builder.AddListeningPort(hostname, grpc::InsecureServerCredentials());
    builder.RegisterService(&ioService);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << hostname << std::endl;
    server->Wait();
    return 0;
}
