#include "doctest.h"
#include "signal_tracer.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    return std::string(std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>());
}

TEST_CASE("VCD format basics")
{
    vcd::SignalTracer tracer("10 ns");
    tracer.add_signal("clk");
    tracer.add_signal("data", 4);

    tracer.record("clk", 0, false);
    tracer.record("data", 0, (uint64_t)0xA);

    tracer.record("clk", 1, true);
    tracer.record("clk", 2, false);

    tracer.record("data", 3, (uint64_t)0x5);

    REQUIRE(tracer.dump("/tmp/vcd_test.vcd"));

    std::string vcd = read_file("/tmp/vcd_test.vcd");

    CHECK(vcd.find("$timescale 10 ns $end") != std::string::npos);
    CHECK(vcd.find("$var wire 1 ! clk $end") != std::string::npos);
    CHECK(vcd.find("$var wire 4 \" data $end") != std::string::npos);

    // dumpvars: first recorded values
    CHECK(vcd.find("0!") != std::string::npos);     // clk=0
    CHECK(vcd.find("b1010 \"") != std::string::npos); // data=0xA

    // transitions
    CHECK(vcd.find("#1\n1!") != std::string::npos);  // clk↑ @1
    CHECK(vcd.find("#2\n0!") != std::string::npos);  // clk↓ @2
    CHECK(vcd.find("#3\nb0101 \"") != std::string::npos); // data→0x5 @3

    // No transition at time 0 (covered by dumpvars)
    CHECK(vcd.find("#0") == std::string::npos);
}

TEST_CASE("VCD deduplicates identical consecutive values")
{
    vcd::SignalTracer tracer;
    tracer.add_signal("x");

    tracer.record("x", 0, false);
    tracer.record("x", 5, false);  // same — should be suppressed
    tracer.record("x", 10, true); // change
    tracer.record("x", 20, true); // same — suppressed

    REQUIRE(tracer.dump("/tmp/vcd_dedup.vcd"));
    std::string vcd = read_file("/tmp/vcd_dedup.vcd");

    CHECK(vcd.find("#5") == std::string::npos);  // suppressed
    CHECK(vcd.find("#20") == std::string::npos); // suppressed
    CHECK(vcd.find("#10\n1!") != std::string::npos);  // only transition
}

TEST_CASE("VCD signal not recorded — initial value is x")
{
    vcd::SignalTracer tracer;
    tracer.add_signal("z");

    REQUIRE(tracer.dump("/tmp/vcd_z.vcd"));
    std::string vcd = read_file("/tmp/vcd_z.vcd");

    // First signal gets VCD ID '!' (ASCII 33); unknown initial → "x!"
    CHECK(vcd.find("x!") != std::string::npos);
}

TEST_CASE("VCD clears correctly")
{
    vcd::SignalTracer tracer;
    tracer.add_signal("a");
    tracer.record("a", 0, true);
    tracer.clear();

    REQUIRE(tracer.dump("/tmp/vcd_clear.vcd"));
    std::string vcd = read_file("/tmp/vcd_clear.vcd");

    // After clear, signal should be unknown ("x!") again
    CHECK(vcd.find("x!") != std::string::npos);
    CHECK(vcd.find("0!") == std::string::npos);  // not from before clear
}

TEST_CASE("VCD multi-cycle PWM waveform round-trip")
{
    vcd::SignalTracer tracer;
    tracer.add_signal("pwm");

    // 50% duty, period 10
    for (int t = 0; t < 30; ++t) {
        tracer.record("pwm", t, (t % 10) < 5);
    }

    REQUIRE(tracer.dump("/tmp/vcd_pwm.vcd"));
    std::string vcd = read_file("/tmp/vcd_pwm.vcd");

    // dumpvars: pwm=1 at time 0 (first value is true since 0%10 < 5)
    CHECK(vcd.find("1!") != std::string::npos);

    // Transitions every 5 cycles
    CHECK(vcd.find("#5\n0!") != std::string::npos);   // falling edge
    CHECK(vcd.find("#10\n1!") != std::string::npos);  // rising edge
    CHECK(vcd.find("#15\n0!") != std::string::npos);
    CHECK(vcd.find("#20\n1!") != std::string::npos);
    CHECK(vcd.find("#25\n0!") != std::string::npos);

    // Only 6 events (first value by dumpvars + 5 transitions)
    // Each event is 2 lines (#N\n<V><id>\n), events section = 10 lines
    // Count occurrences of '#' — should be 5 (no #0)
    int hash_count = 0;
    for (size_t pos = 0; (pos = vcd.find('#', pos)) != std::string::npos; ++pos)
        hash_count++;
    CHECK(hash_count == 5);
}
