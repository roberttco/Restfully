//
// Created by guru on 9/9/20.
//
#define CATCH_CONFIG_FAST_COMPILE
#include <catch.hpp>

#include <cstring>
#include <stdio.h>
#include <iostream>
#include <map>

#include <Restfully.h>


TEST_CASE("parses 3 alpha terms", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/echo");
    Rest::Endpoint root(request);

    std::string hello_msg = "Greetings!";

    auto good = root / "api" / "dev" / "echo";
    auto bad = root / "api" / "dev" / 1;

    REQUIRE(good.status == 0);
    REQUIRE_FALSE(bad.status == 0);
}

TEST_CASE("parses a numeric path", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/1/echo");
    Rest::Endpoint root(request);
    auto good = root / "api" / "dev" / 1;
    REQUIRE(good.status == 0);
}

TEST_CASE("parses a direct numeric argument", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo");
    Rest::Endpoint root(request);
    int id = 0;
    auto good = root / "api" / "dev" / &id;
    REQUIRE(good.status == 0);
    REQUIRE(id == 6);
}

TEST_CASE("parses a direct string argument", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Endpoint root(request);
    int id = 0;
    std::string name = "jane";
    auto good = root / "api" / "dev" / &id / "echo" / &name;
    REQUIRE(good.status == 0);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
}

TEST_CASE("mismatched Uri does not fill direct arguments", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Endpoint root(request);
    int id = 0;
    std::string name = "jane";
    auto bad = root / "api" / "device" / &id / "echo" / &name;
    REQUIRE(bad.status != 0);
    REQUIRE(id != 6);
    REQUIRE(name == "jane");
}


TEST_CASE("parsing logic with two branches", "Parser") {
    int id = 0;
    std::string name;
    bool echo_greeted = false, exported_config = false;

    auto api_eval = [&id, &name, &echo_greeted, &exported_config](const char* req_uri) {
        Rest::UriRequest request(Rest::HttpGet, req_uri);
        Rest::Endpoint root(request);
        echo_greeted = false;
        exported_config = false;
        if (auto device_endpoint = root / "api" / "dev" / &id) {
            if (device_endpoint / "echo" / &name)
                echo_greeted = true;
            if (device_endpoint / "system" / "config")
                exported_config = true;
        }
    };

    api_eval("/api/dev/6/echo/the.brown.fox");
    REQUIRE(echo_greeted);
    REQUIRE_FALSE(exported_config);
    REQUIRE(id == 6);
    REQUIRE(name == "the.brown.fox");

    api_eval("/api/dev/8/system/config");
    REQUIRE_FALSE(echo_greeted);
    REQUIRE(exported_config);
    REQUIRE(id == 8);
}


TEST_CASE("matched Uri calls GET handler", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Endpoint root(request);
    bool hit = false;
    int id = 0;
    std::string name = "jane";
    auto good = root / "api" / "dev" / &id / "echo" / &name / Rest::GET([&]() { hit = true; return 200; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit);
}

TEST_CASE("matched Uri calls GET handler amongst PUT handler", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Endpoint root(request);
    bool hit = false;
    int id = 0;
    std::string name = "jane";
    auto good = root / "api" / "dev" / &id / "echo" / &name
            / Rest::PUT([&]() { hit = true; return 500; })
            / Rest::GET([&]() { hit = true; return 200; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit);
}

TEST_CASE("matched Uri calls PUT handler before GET handler", "Parser") {
    Rest::UriRequest request(Rest::HttpPut, "/api/dev/6/echo/john.doe");
    Rest::Endpoint root(request);
    bool hit = false;
    int id = 0;
    std::string name = "jane";
    auto good = root / "api" / "dev" / &id / "echo" / &name
                / Rest::PUT([&]() { hit = true; return 200; })
                / Rest::GET([&]() { hit = true; return 500; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit);
}

TEST_CASE("matched Uri calls PUT handler with UriRequest arg", "Parser") {
    Rest::UriRequest request(Rest::HttpPut, "/api/dev/6/echo/john.doe/and/jane");
    Rest::Endpoint root(request);
    int hit = 0;
    int id = 0;
    std::string name = "jane";
    auto good = root / "api" / "dev" / &id / "echo" / &name
                / Rest::PUT([&](const Rest::UriRequest& req) {
                    hit = req.words.size();
                    return 200;
                })
                / Rest::GET([&]() { hit = 1; return 500; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit == 7);
}

TEST_CASE("matched Uri calls PUT handler with Parser arg", "Parser") {
    Rest::UriRequest request(Rest::HttpPut, "/api/dev/6/echo/john.doe/and/jane");
    Rest::Endpoint root(request);
    int hit = 0;
    int id = 0;
    std::string name = "jane";
    auto good = root / "api" / "dev" / &id / "echo" / &name
                / Rest::PUT([&](const Rest::Endpoint& p) {
                    hit = p.ordinal();
                    return 200;
                })
                / Rest::GET([&]() { hit = 1; return 500; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit == 5);
}

class EchoService : public Rest::Endpoint::Delegate
{
public:
    std::string name;

    Rest::Endpoint delegate(Rest::Endpoint &p) override {
        return p / "echo" / &name / Rest::GET([this](const Rest::UriRequest& r) {
            std::cout << "hello " << name << std::endl;
            return 200;
        });
    }
};

TEST_CASE("match Uri using delegation", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe/and/jane");
    Rest::Endpoint root(request);
    int id = 0;
    EchoService echo;
    auto good = root / "api" / "dev" / &id / echo;

    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(echo.name == "john.doe");
}

TEST_CASE("matched Uri with Integer Argument", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Endpoint root(request);
    bool hit = false;
    std::string name = "jane";
    auto good = root / "api" / "dev" / Rest::Parameter::Integer("devid") / "echo" / &name / Rest::GET([&]() { hit = true; return 200; });
    auto devid = good["devid"];
    REQUIRE(good.status == 200);
    REQUIRE(devid.isInteger());
    REQUIRE(devid == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit);
}

TEST_CASE("matched Uri with String Argument", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Endpoint root(request);
    bool hit = false;
    int id = 0;
    auto good = root / "api" / "dev" / &id / "echo" / Rest::Parameter::String("name") / Rest::GET([&]() { hit = true; return 200; });
    auto name = good["name"];
    REQUIRE(good.status == 200);
    REQUIRE(name.isString());
    REQUIRE(name == "john.doe");
    REQUIRE(id == 6);
    REQUIRE(hit);
}

class Device
{
public:
    int id;
};

TEST_CASE("endpoint with object argument embedding", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/echo/john.doe");
    Rest::Endpoint root(request);
    int hit = 0;
    Device dev;
    dev.id = 5;
    auto good = root / "api" / "dev" / Rest::Argument::from("device", dev) / "echo" / Rest::Parameter::String("name") / Rest::GET([&](const Rest::Endpoint& p) {
        auto dev = p["device"];
        hit = dev.get<Device>().id;
        return 200;
    });
    auto name = good["name"];
    REQUIRE(good.status == 200);
    REQUIRE(name.isString());
    REQUIRE(name == "john.doe");
    REQUIRE(hit == dev.id);
}


#if 0
    // each parser function returns an iterator into the parsing, therefor parsing can be re-continued
    // the parser itself would be an iterator then right?
    // the parsing would stop though if UriRequest status becomes non-zero
    // the iterator evaluates to false if unexpected token or if UriRequest::status becomes non-zero
    parser
        .on("api")
        .on("dev")
        .on(1)      // const number
        .on("echo")

        .GET([&hello_msg]() { std::cout << hello_msg; })        // if its a GET request, then run this function
        .PUT(hello_msg);        // if its a PUT request, then run this function

    int devid;
    std::string varname;
    std::map<std::string, std::string> config;
    if(auto config_endpoint = parser >> "api" >> "dev" >> devid >> "configure") {
        config_endpoint >> "set" >> varname
                >> Rest::Get([&config, &varname]() { std::cout << config[varname]; })
                >> Rest::Put([&config, &varname]() { config[varname] = "post data"; })

    }


#endif
