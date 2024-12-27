#include "httplib.h"
#include "include/NewtonianSimulator.hpp"
#include "include/BarnesHutSimulator.hpp"
#include "include/Config.hpp"
#include <memory>
#include <string>

using namespace GEngine;

std::unique_ptr<ISimulator> newtonianSimulator = std::make_unique<NewtonianSimulator>();
std::unique_ptr<ISimulator> barnesHutSimulator = std::make_unique<BarnesHutSimulator>();

void initializeSolarSystem(ISimulator& simulator) {
    simulator.clear();

    // 太阳
    simulator.addBody(std::make_shared<CelestialBody>(
        "Sun", 1.989e30, 696340000,
        Vector3D(0, 0, 0), Vector3D(0, 0, 0)
    ));

    // 水星
    simulator.addBody(std::make_shared<CelestialBody>(
        "Mercury", 3.285e23, 2439700,
        Vector3D(57.9e9, 0, 0), Vector3D(0, 47.87e3, 0)
    ));

    // 金星
    simulator.addBody(std::make_shared<CelestialBody>(
        "Venus", 4.867e24, 6051800,
        Vector3D(108.2e9, 0, 0), Vector3D(0, 35.02e3, 0)
    ));

    // 地球
    simulator.addBody(std::make_shared<CelestialBody>(
        "Earth", 5.972e24, 6371000,
        Vector3D(149.6e9, 0, 0), Vector3D(0, 29.78e3, 0)
    ));

    // 火星
    simulator.addBody(std::make_shared<CelestialBody>(
        "Mars", 6.39e23, 3389500,
        Vector3D(227.9e9, 0, 0), Vector3D(0, 24.077e3, 0)
    ));

    // 木星
    simulator.addBody(std::make_shared<CelestialBody>(
        "Jupiter", 1.898e27, 69911000,
        Vector3D(778.5e9, 0, 0), Vector3D(0, 13.07e3, 0)
    ));

    // 土星
    simulator.addBody(std::make_shared<CelestialBody>(
        "Saturn", 5.683e26, 58232000,
        Vector3D(1.434e12, 0, 0), Vector3D(0, 9.68e3, 0)
    ));

    // 天王星
    simulator.addBody(std::make_shared<CelestialBody>(
        "Uranus", 8.681e25, 25362000,
        Vector3D(2.871e12, 0, 0), Vector3D(0, 6.80e3, 0)
    ));

    simulator.addBody(std::make_shared<CelestialBody>(
        "Neptune", 1.024e26, 24622000,
        Vector3D(4.495e12, 0, 0), Vector3D(0, 5.43e3, 0)
    ));
}

int main() {
    httplib::Server svr;

    auto setCorsHeaders = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    svr.Options("/(.*)", [&setCorsHeaders](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.set_content("", "text/plain");
    });

    initializeSolarSystem(*newtonianSimulator);
    initializeSolarSystem(*barnesHutSimulator);

    svr.Get("/api/system-state", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
        auto& simulator = useBarnesHut ? *barnesHutSimulator : *newtonianSimulator;
        
        nlohmann::json state = simulator.getSystemState();
        res.set_content(state.dump(), "application/json");
    });

    svr.Post("/api/simulate", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        try {
            bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
            auto& simulator = useBarnesHut ? *barnesHutSimulator : *newtonianSimulator;
            auto& config = SimulationConfig::getInstance();

            // 解析请求体中的时间控制参数
            if (!req.body.empty()) {
                auto params = nlohmann::json::parse(req.body);
                if (params.contains("timeDirection")) {
                    config.timeDirectionForward = (params["timeDirection"] == "forward");
                }
                if (params.contains("timeStep")) {
                    config.timeStep = params["timeStep"].get<double>();
                }
            }

            simulator.step();
            
            nlohmann::json state = simulator.getSystemState();
            res.set_content(state.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(
                nlohmann::json({{"error", e.what()}}).dump(),
                "application/json"
            );
            res.status = 400;
        }
    });

    svr.Post("/api/jump-time", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        try {
            bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
            auto& simulator = useBarnesHut ? *barnesHutSimulator : *newtonianSimulator;
            auto& config = SimulationConfig::getInstance();

            auto params = nlohmann::json::parse(req.body);
            if (!params.contains("days")) {
                throw std::runtime_error("Missing 'days' parameter");
            }

            double days = params["days"].get<double>();
            int steps = static_cast<int>((days * 24 * 3600) / config.timeStep);

            // 执行多步模���
            for (int i = 0; i < steps; ++i) {
                simulator.step();
            }

            nlohmann::json state = simulator.getSystemState();
            res.set_content(state.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(
                nlohmann::json({{"error", e.what()}}).dump(),
                "application/json"
            );
            res.status = 400;
        }
    });

    svr.Post("/api/reset", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
        auto& simulator = useBarnesHut ? *barnesHutSimulator : *newtonianSimulator;

        simulator.reset();
        initializeSolarSystem(simulator);
        
        nlohmann::json state = simulator.getSystemState();
        res.set_content(state.dump(), "application/json");
    });

    svr.Post("/api/configure", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        try {
            auto config = nlohmann::json::parse(req.body);
            
            newtonianSimulator->configure(config);
            barnesHutSimulator->configure(config);
            
            res.set_content("{\"status\": \"success\"}", "application/json");
        } catch (const std::exception& e) {
            res.set_content(
                nlohmann::json({{"error", e.what()}}).dump(),
                "application/json"
            );
            res.status = 400;
        }
    });

    svr.Get("/api/export-config", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        nlohmann::json config;
        
        config["simulationConfig"] = SimulationConfig::getInstance().toJson();
        
        bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
        auto& simulator = useBarnesHut ? *barnesHutSimulator : *newtonianSimulator;
        config["bodies"] = simulator.getSystemState();
        
        res.set_content(config.dump(2), "application/json");
    });

    svr.Post("/api/import-config", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        try {
            auto config = nlohmann::json::parse(req.body);
            
            if (config.contains("simulationConfig")) {
                SimulationConfig::getInstance().loadFromJson(config["simulationConfig"]);
            }
            
            if (config.contains("bodies")) {
                bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
                auto& simulator = useBarnesHut ? *barnesHutSimulator : *newtonianSimulator;
                
                simulator.clear();
                for (const auto& bodyData : config["bodies"]) {
                    simulator.addBody(CelestialBody::fromJson(bodyData));
                }
            }
            
            res.set_content("{\"status\": \"success\"}", "application/json");
        } catch (const std::exception& e) {
            res.set_content(
                nlohmann::json({{"error", e.what()}}).dump(),
                "application/json"
            );
            res.status = 400;
        }
    });

    // 获取引力场数据的API
    svr.Get("/api/gravitational-field", [&setCorsHeaders](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        try {
            bool useBarnesHut = req.has_param("algorithm") && req.get_param_value("algorithm") == "barnes-hut";
            auto& simulator = useBarnesHut ? *barnesHutSimulator : *newtonianSimulator;

            // 获取查询参数
            double centerX = req.has_param("centerX") ? std::stod(req.get_param_value("centerX")) : 0.0;
            double centerY = req.has_param("centerY") ? std::stod(req.get_param_value("centerY")) : 0.0;
            double centerZ = req.has_param("centerZ") ? std::stod(req.get_param_value("centerZ")) : 0.0;
            double size = req.has_param("size") ? std::stod(req.get_param_value("size")) : 1e12;
            int resolution = req.has_param("resolution") ? std::stoi(req.get_param_value("resolution")) : 10;

            // 计算引力场数据
            Vector3D center(centerX, centerY, centerZ);
            nlohmann::json fieldData = simulator.getGravitationalFieldData(center, size, resolution);
            
            res.set_content(fieldData.dump(), "application/json");
        } catch (const std::exception& e) {
            res.set_content(
                nlohmann::json({{"error", e.what()}}).dump(),
                "application/json"
            );
            res.status = 400;
        }
    });

    svr.listen("localhost", 8081);
    return 0;
}
