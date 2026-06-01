#include "doctest.h"
#include "vioavr/core/hc05.hpp"

using namespace vioavr::core;

static void feed_at(Hc05& hc, const char* cmd) {
    while (*cmd)
        hc.rx_byte(static_cast<u8>(*cmd++));
}

static bool read_response(Hc05& hc, char* buf, u16 max_sz = 64) {
    u16 pos = 0;
    while (hc.has_tx_byte() && pos < max_sz - 1)
        buf[pos++] = static_cast<char>(hc.read_tx_byte());
    buf[pos] = '\0';
    return pos > 0;
}

TEST_CASE("HC-05 AT: basic echo") {
    Hc05 hc;
    feed_at(hc, "AT\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
}

TEST_CASE("HC-05 AT: name") {
    Hc05 hc;
    REQUIRE(std::strcmp(hc.name(), "HC-05") == 0);
    feed_at(hc, "AT+NAME?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+NAME:HC-05") != nullptr);
    feed_at(hc, "AT+NAME=MyDevice\r\n");
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(std::strcmp(hc.name(), "MyDevice") == 0);
    feed_at(hc, "AT+NAME?\r\n");
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+NAME:MyDevice") != nullptr);
}

TEST_CASE("HC-05 AT: pin code") {
    Hc05 hc;
    REQUIRE(std::strcmp(hc.pin_code(), "1234") == 0);
    feed_at(hc, "AT+PSWD?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+PSWD:1234") != nullptr);
    feed_at(hc, "AT+PSWD=4321\r\n");
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(std::strcmp(hc.pin_code(), "4321") == 0);
}

TEST_CASE("HC-05 AT: uart baud") {
    Hc05 hc;
    CHECK(hc.baud() == 9600);
    feed_at(hc, "AT+UART?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+UART:9600") != nullptr);
    feed_at(hc, "AT+UART=115200,0,0\r\n");
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(hc.baud() == 115200);
}

TEST_CASE("HC-05 AT: role") {
    Hc05 hc;
    CHECK(hc.role() == false);
    feed_at(hc, "AT+ROLE?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+ROLE:0") != nullptr);
    feed_at(hc, "AT+ROLE=1\r\n");
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(hc.role() == true);
}

TEST_CASE("HC-05 AT: state") {
    Hc05 hc;
    CHECK(hc.state() == Hc05::State::AT_MODE);
    feed_at(hc, "AT+STATE?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+STATE:INITIALIZED") != nullptr);
}

TEST_CASE("HC-05 AT: address") {
    Hc05 hc;
    feed_at(hc, "AT+ADDR?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+ADDR:") != nullptr);
}

TEST_CASE("HC-05 AT: version") {
    Hc05 hc;
    feed_at(hc, "AT+VERSION?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+VERSION:2.0") != nullptr);
}

TEST_CASE("HC-05 AT: cmode") {
    Hc05 hc;
    CHECK(hc.cmode() == 1);
    feed_at(hc, "AT+CMODE?\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "+CMODE:1") != nullptr);
    feed_at(hc, "AT+CMODE=0\r\n");
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(hc.cmode() == 0);
}

TEST_CASE("HC-05 connection lifecycle") {
    Hc05 master;
    Hc05 slave;
    master.set_role(true);
    slave.set_role(false);
    master.pair(&slave);
    slave.pair(&master);
    char resp[64] = {};
    feed_at(master, "AT+INIT\r\n");
    CHECK(read_response(master, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(master.state() == Hc05::State::INIT);
    feed_at(master, "AT+CONN\r\n");
    CHECK(read_response(master, resp));
    CHECK(std::strstr(resp, "CONNECT") != nullptr);
    CHECK(master.state() == Hc05::State::CONNECTED);
    CHECK(slave.state() == Hc05::State::CONNECTED);
    feed_at(master, "Hello");
    CHECK(slave.has_injected_data());
    char data[16] = {};
    u16 pos = 0;
    while (slave.has_injected_data() && pos < 15)
        data[pos++] = static_cast<char>(slave.read_injected_byte());
    data[pos] = '\0';
    CHECK(std::strcmp(data, "Hello") == 0);
    slave.rx_byte('+');
    slave.rx_byte('+');
    slave.rx_byte('+');
    CHECK(slave.state() == Hc05::State::AT_MODE);
    char slave_resp[64] = {};
    CHECK(read_response(slave, slave_resp));
    CHECK(std::strstr(slave_resp, "OK") != nullptr);
}

TEST_CASE("HC-05 at disconnect") {
    Hc05 master;
    Hc05 slave;
    master.set_role(true);
    master.pair(&slave);
    slave.pair(&master);
    feed_at(master, "AT+INIT\r\n");
    { char r[64] = {}; read_response(master, r, 64); }
    feed_at(master, "AT+CONN\r\n");
    { char r[64] = {}; read_response(master, r, 64); }
    CHECK(master.state() == Hc05::State::CONNECTED);
    CHECK(slave.state() == Hc05::State::CONNECTED);
    // Must escape to AT mode first, then disconnect
    master.rx_byte('+');
    master.rx_byte('+');
    master.rx_byte('+');
    { char r[64] = {}; read_response(master, r, 64); } // OK
    CHECK(master.state() == Hc05::State::AT_MODE);
    feed_at(master, "AT+DISC\r\n");
    char resp[64] = {};
    CHECK(read_response(master, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(master.state() == Hc05::State::INIT);
    CHECK(slave.state() == Hc05::State::INIT);
}

TEST_CASE("HC-05 external data injection") {
    Hc05 hc;
    u8 data[] = "ExternalHello";
    hc.inject_external_data(data, 13);
    CHECK(hc.has_external_data());
    char buf[32] = {};
    u16 pos = 0;
    while (hc.has_external_data() && pos < 31)
        buf[pos++] = static_cast<char>(hc.read_injected_byte());
    buf[pos] = '\0';
    CHECK(std::strcmp(buf, "ExternalHello") == 0);
}

TEST_CASE("HC-05 at reset") {
    Hc05 hc;
    hc.set_name("TestName");
    hc.set_baud(115200);
    feed_at(hc, "AT+RESET\r\n");
    char resp[64] = {};
    CHECK(read_response(hc, resp));
    CHECK(std::strstr(resp, "OK") != nullptr);
    CHECK(std::strcmp(hc.name(), "TestName") == 0);
}

TEST_CASE("HC-05 data mode forwarding both directions") {
    Hc05 a;
    Hc05 b;
    a.set_role(true);
    b.set_role(false);
    a.pair(&b);
    b.pair(&a);
    feed_at(a, "AT+INIT\r\n");
    { char r[64] = {}; read_response(a, r, 64); }
    feed_at(a, "AT+CONN\r\n");
    { char r[64] = {}; read_response(a, r, 64); }
    a.rx_byte('A');
    CHECK(b.has_injected_data());
    CHECK(b.read_injected_byte() == 'A');
    b.rx_byte('B');
    CHECK(a.has_injected_data());
    CHECK(a.read_injected_byte() == 'B');
}
