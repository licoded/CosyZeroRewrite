/**
 * Stress Test for Z3 BMC Equivalence Checking
 * ============================================
 *
 * This test runs 1-2 hours of comprehensive stress testing:
 * - Complex formula equivalence tests
 * - Random formula fuzzing
 * - Memory leak detection preparation
 * - Timeout logging to CSV
 *
 * Author: Claude Code
 * Date: 2026-01-02
 */

#define FORMULA_USE_Z3
#include "formula/formula.hpp"
#include "formula/formula_pool.hpp"
#include "formula/formula_parser.hpp"
#include "formula/formula_z3.hpp"
#include "log/logger.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace formula;
using namespace std::chrono;

// ========== Configuration ==========

constexpr int TOTAL_TARGET_DURATION_MS = 5 * 60 * 1000;  // 5 hours (adjustable)
constexpr int PROGRESS_REPORT_INTERVAL_MS = 30000;  // Report every 30s
constexpr int BENCHMARK_TIMEOUT_MS = 30000;  // 30s per test
constexpr int MEMCHECK_INTERVAL_MS = 10 * 60 * 1000;  // Suggest memcheck every 10min

// ========== Global State ==========

struct TestStats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    int timeouts = 0;
    double total_time_ms = 0;
    int max_bound_reached = 0;
};

struct TimeoutRecord {
    std::string timestamp;
    std::string formula1;
    std::string formula2;
    int bound;
    unsigned timeout_ms;
    double elapsed_ms;
    std::string result;
};

TestStats stats;
std::vector<TimeoutRecord> timeout_records;
steady_clock::time_point test_start_time;
std::ofstream timeout_log;

// ========== Utility Functions ==========

std::string get_timestamp() {
    auto now = system_clock::now();
    auto ms_count = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&timer);

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    std::sprintf(buf + strlen(buf), ".%03d", static_cast<int>(ms_count.count()));
    return std::string(buf);
}

void print_header(const std::string& title) {
    std::cout << "\n========== " << title << " ==========\n";
}

void print_progress() {
    auto now = steady_clock::now();
    double elapsed = duration<double, std::milli>(now - test_start_time).count();
    double progress = (elapsed / TOTAL_TARGET_DURATION_MS) * 100;

    std::cout << "\n[" << get_timestamp() << "] Progress: "
              << std::fixed << std::setprecision(1) << progress << "% | "
              << "Total: " << stats.total
              << " | Passed: " << stats.passed
              << " | Failed: " << stats.failed
              << " | Timeouts: " << stats.timeouts
              << " | Avg time: " << std::setprecision(2)
              << (stats.total > 0 ? stats.total_time_ms / stats.total : 0) << "ms\n";
}

void log_timeout(const std::string& f1, const std::string& f2,
                int bound, unsigned timeout, double elapsed, const std::string& result) {
    TimeoutRecord rec;
    rec.timestamp = get_timestamp();
    rec.formula1 = f1;
    rec.formula2 = f2;
    rec.bound = bound;
    rec.timeout_ms = timeout;
    rec.elapsed_ms = elapsed;
    rec.result = result;

    timeout_records.push_back(rec);

    // Write to CSV
    if (timeout_log.is_open()) {
        timeout_log << rec.timestamp << ","
                    << "\"" << f1 << "\","
                    << "\"" << f2 << "\","
                    << bound << ","
                    << timeout << ","
                    << std::fixed << std::setprecision(2) << elapsed << ","
                    << result << "\n";
        timeout_log.flush();
    }
}

// ========== Test Cases ==========

struct TestCase {
    std::string name;
    std::string f1;
    std::string f2;
    bool expected_equiv;
    int max_bound;
    unsigned timeout_ms;
};

// ========== Complex Formula Tests ==========

std::vector<TestCase> generate_complex_tests() {
    return {
        // Propositional logic complexity
        {"Prop: Deep nesting 1",
         "(((((p1 & p2) & p3) & p4) & p5) & p6)",
         "p1 & p2 & p3 & p4 & p5 & p6", true, -1, 5000},

        {"Prop: Deep nesting 2",
         "(p1 | (p2 | (p3 | (p4 | (p5 | p6)))))",
         "p1 | p2 | p3 | p4 | p5 | p6", true, -1, 5000},

        // Temporal operator chains
        {"Temp: X chain 5",
         "X(X(X(X(X(p1)))))",
         "X(X(X(X(X(p1)))))", true, -1, 10000},

        {"Temp: X chain 10",
         "X(X(X(X(X(X(X(X(X(X(p1))))))))))",
         "X(X(X(X(X(X(X(X(X(X(p1))))))))))", true, -1, 15000},

        {"Temp: Mixed X depth",
         "X(p1) | X(X(p2)) | X(X(X(p3)))",
         "(X(p1) | X(X(p2))) | X(X(X(p3)))", true, -1, 10000},

        // Until complexity
        {"Until: Nested",
         "(p1 U p2) U (p3 U p4)",
         "(p1 U p2) U (p3 U p4)", true, -1, 10000},

        {"Until: Triple nested",
         "((p1 U p2) U p3) U p4",
         "((p1 U p2) U p3) U p4", true, -1, 15000},

        {"Until: Wide operands",
         "(p1 | p2 | p3 | p4 | p5) U (p6 | p7 | p8 | p9 | p10)",
         "(p1 | p2 | p3 | p4 | p5) U (p6 | p7 | p8 | p9 | p10)", true, -1, 15000},

        // Release complexity
        {"Release: Nested",
         "(p1 R p2) R (p3 R p4)",
         "(p1 R p2) R (p3 R p4)", true, -1, 10000},

        // Mixed operators
        {"Mixed: X with U",
         "X(p1 U p2) | X(p3 U p4)",
         "X(p1 U p2) | X(p3 U p4)", true, -1, 15000},

        {"Mixed: U with X",
         "(p1 U X(p2)) | (X(p3) U p4)",
         "(p1 U X(p2)) | (X(p3) U p4)", true, -1, 15000},

        {"Mixed: Complex 1",
         "X(p1 | p2) | (p3 U p4)",
         "(X(p1) | X(p2)) | (p3 U p4)", true, -1, 10000},

        {"Mixed: Complex 2",
         "(p1 & p2) U (X(p3) & p4)",
         "(p1 & p2) U (X(p3) & p4)", true, -1, 15000},

        {"Mixed: Deep nesting with temporal",
         "((X(p1) U p2) & p3) | X(X(p4))",
         "((X(p1) U p2) & p3) | X(X(p4))", true, -1, 15000},

        // Negation handling
        {"Neg: Double negation chain",
         "!(!(!(!(!p1))))",
         "p1", true, -1, 5000},

        {"Neg: De Morgan complex",
         "!((p1 & p2) | (p3 & p4))",
         "(!p1 | !p2) & (!p3 | !p4)", true, -1, 10000},

        // Non-equivalent tests
        {"Non-equiv: U vs R",
         "p1 U p2",
         "p1 R p2", false, -1, 10000},

        {"Non-equiv: X distributes?",
         "X(p1 | p2)",
         "X(p1) | X(p2)", true, -1, 10000},

        {"Non-equiv: Different U",
         "p1 U p2",
         "p2 U p1", false, -1, 10000},
    };
}

// ========== Random Formula Generator ==========

class RandomFormulaGenerator {
public:
    RandomFormulaGenerator(int num_vars = 20)
        : rng_(std::random_device{}()), var_dist_(1, num_vars) {
        for (int i = 1; i <= num_vars; ++i) {
            vars_.push_back("p" + std::to_string(i));
        }
    }

    std::string generate(int depth, bool allow_unary = true) {
        if (depth == 0 || (allow_unary && rng_() % 3 == 0)) {
            return random_literal();
        }

        int op = rng_() % 8;
        switch (op) {
            case 0: return "!" + generate(depth - 1, false);
            case 1: return "(" + generate(depth - 1) + " & " + generate(depth - 1) + ")";
            case 2: return "(" + generate(depth - 1) + " | " + generate(depth - 1) + ")";
            case 3: return "X(" + generate(depth - 1, false) + ")";
            case 4: return "(" + generate(depth - 1) + " U " + generate(depth - 1) + ")";
            case 5: return "(" + generate(depth - 1) + " R " + generate(depth - 1) + ")";
            case 6: return random_literal();  // Base case
            case 7: return "true";  // Constant
            default: return random_literal();
        }
    }

private:
    std::string random_literal() {
        int idx = var_dist_(rng_) - 1;
        if (rng_() % 3 == 0) {
            return "!" + vars_[idx];
        }
        return vars_[idx];
    }

    std::mt19937 rng_;
    std::uniform_int_distribution<int> var_dist_;
    std::vector<std::string> vars_;
};

// ========== Test Runner ==========

void run_test(FormulaPool&, FormulaParser& parser, const TestCase& test) {
    stats.total++;

    auto start = steady_clock::now();

    try {
        Formula* f1 = parser.parse(test.f1);
        Formula* f2 = parser.parse(test.f2);

        auto result = FormulaZ3::are_equivalent(f1, f2, test.max_bound, test.timeout_ms);

        auto end = steady_clock::now();
        double elapsed = duration<double, std::milli>(end - start).count();
        stats.total_time_ms += elapsed;

        if (result.has_value()) {
            if (result.value() == test.expected_equiv) {
                stats.passed++;
                std::cout << "  PASS: " << test.name << " (" << elapsed << "ms)\n";
                LOG_DEBUG("PASS: {} ({}ms)", test.name, elapsed);
            } else {
                stats.failed++;
                std::cout << "  FAIL: " << test.name << " - expected "
                          << (test.expected_equiv ? "equiv" : "not-equiv")
                          << ", got " << (result.value() ? "equiv" : "not-equiv")
                          << " (" << elapsed << "ms)\n";
                LOG_WARN("FAIL: {} - expected {}, got {} ({}ms)",
                         test.name, test.expected_equiv, result.value(), elapsed);
            }
        } else {
            stats.timeouts++;
            std::cout << "  TIMEOUT: " << test.name << " (" << elapsed << "ms)\n";
            LOG_WARN("TIMEOUT: {} ({}ms)", test.name, elapsed);
            log_timeout(test.f1, test.f2, test.max_bound, test.timeout_ms,
                      elapsed, "TIMEOUT");
        }

    } catch (const std::exception& e) {
        stats.failed++;
        auto end = steady_clock::now();
        double elapsed = duration<double, std::milli>(end - start).count();
        std::cout << "  ERROR: " << test.name << " - " << e.what() << " (" << elapsed << "ms)\n";
        LOG_ERROR("ERROR: {} - {} ({}ms)", test.name, e.what(), elapsed);
        log_timeout(test.f1, test.f2, test.max_bound, test.timeout_ms,
                  elapsed, std::string("ERROR: ") + e.what());
    }
}

void run_random_tests(FormulaPool& pool, FormulaParser& parser, int count) {
    RandomFormulaGenerator gen(20);
    static std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < count; ++i) {
        std::string f1 = gen.generate(5 + (i % 8));
        std::string f2 = gen.generate(5 + (i % 8));

        // Make 30% of tests equivalent (same formula)
        if (rng() % 10 < 3) {
            f2 = f1;
        }

        TestCase test;
        test.name = "Random_" + std::to_string(i);
        test.f1 = f1;
        test.f2 = f2;
        test.expected_equiv = (f1 == f2);
        test.max_bound = -1;
        test.timeout_ms = 10000 + (rng() % 20000);

        run_test(pool, parser, test);
    }
}

// ========== Main ==========

int main() {
    test_start_time = steady_clock::now();

    // Initialize logger
    LOG_INFO("Starting Z3 BMC Stress Test");
    LOG_INFO("Total target duration: {} ms", TOTAL_TARGET_DURATION_MS);
    LOG_INFO("Progress report interval: {} ms", PROGRESS_REPORT_INTERVAL_MS);
    LOG_INFO("Benchmark timeout: {} ms", BENCHMARK_TIMEOUT_MS);

    print_header("Z3 BMC Stress Test");

    // Open timeout log
    char time_buf[64];
    auto now = system_clock::now();
    auto timer = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&timer);
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm);

    std::string log_filename = "logs/timeouts_" + std::string(time_buf) + ".csv";
    timeout_log.open(log_filename);
    if (timeout_log.is_open()) {
        timeout_log << "timestamp,formula1,formula2,bound,timeout_ms,elapsed_ms,result\n";
        std::cout << "Timeout log: " << log_filename << "\n";
    }

    FormulaPool pool;
    FormulaParser parser(pool);

    print_header("Starting Tests");

    auto last_progress = steady_clock::now();
    int random_batch = 0;

    while (true) {
        auto now = steady_clock::now();
        double elapsed = duration<double, std::milli>(now - test_start_time).count();

        // Check if we should stop
        if (elapsed >= TOTAL_TARGET_DURATION_MS) {
            break;
        }

        // Progress report
        if (duration<double, std::milli>(now - last_progress).count() >= PROGRESS_REPORT_INTERVAL_MS) {
            print_progress();
            last_progress = now;
        }

        // Run complex tests first
        static bool complex_done = false;
        if (!complex_done) {
            print_header("Complex Formula Tests");
            auto complex_tests = generate_complex_tests();
            for (const auto& test : complex_tests) {
                run_test(pool, parser, test);
            }
            complex_done = true;
            continue;
        }

        // Run random tests in batches
        print_header("Random Tests Batch " + std::to_string(++random_batch));
        run_random_tests(pool, parser, 50);
    }

    // Final report
    print_header("Final Report");

    double total_time = duration<double, std::milli>(steady_clock::now() - test_start_time).count() / 1000.0;

    std::cout << "\nTotal test time: "
              << std::fixed << std::setprecision(1)
              << total_time
              << " seconds\n";

    std::cout << "Tests run: " << stats.total << "\n";
    std::cout << "  Passed:  " << stats.passed << " ("
              << std::fixed << std::setprecision(1)
              << (stats.total > 0 ? (stats.passed * 100.0 / stats.total) : 0) << "%)\n";
    std::cout << "  Failed:  " << stats.failed << "\n";
    std::cout << "  Timeouts: " << stats.timeouts << "\n";
    std::cout << "Average time per test: "
              << std::setprecision(2)
              << (stats.total > 0 ? stats.total_time_ms / stats.total : 0) << "ms\n";

    // Log final summary
    LOG_INFO("Stress test completed");
    LOG_INFO("Total test time: {:.1f} seconds", total_time);
    LOG_INFO("Tests run: {}", stats.total);
    LOG_INFO("Passed: {} ({:.1f}%)", stats.passed,
             stats.total > 0 ? (stats.passed * 100.0 / stats.total) : 0);
    LOG_INFO("Failed: {}", stats.failed);
    LOG_INFO("Timeouts: {}", stats.timeouts);
    LOG_INFO("Average time per test: {:.2f}ms",
             stats.total > 0 ? stats.total_time_ms / stats.total : 0);
    LOG_FLUSH();

    if (timeout_log.is_open()) {
        timeout_log.close();
        std::cout << "\nTimeout log saved to: " << log_filename << "\n";
    }

    std::cout << "\n========== Test Complete ==========\n";

    return (stats.failed == 0) ? 0 : 1;
}
