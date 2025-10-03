#!/bin/bash
# Run all prj1 tests

echo "================================"
echo "prj1 Test Suite Runner"
echo "================================"
echo ""

TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name=$1
    local test_binary=$2
    
    echo "Running $test_name..."
    if $test_binary; then
        echo "✓ $test_name PASSED"
        echo ""
        ((TESTS_PASSED++))
    else
        echo "✗ $test_name FAILED"
        echo ""
        ((TESTS_FAILED++))
    fi
}

# Build tests first
echo "Building tests..."
cd /workspaces/ZMQBridge/prj1/build
cmake .. && cmake --build . --parallel
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi
echo ""

# Run all tests
run_test "REQ/REP Pattern Test" "/workspaces/ZMQBridge/prj1/build/test_req_rep"
run_test "PUB/SUB Pattern Test" "/workspaces/ZMQBridge/prj1/build/test_pub_sub"
run_test "PUSH/PULL Pattern Test" "/workspaces/ZMQBridge/prj1/build/test_push_pull"
run_test "Edge Cases Test" "/workspaces/ZMQBridge/prj1/build/test_edge_cases"
run_test "Multi-threading Test" "/workspaces/ZMQBridge/prj1/build/test_multithreading"

# Summary
echo "================================"
echo "Test Summary"
echo "================================"
echo "Passed: $TESTS_PASSED"
echo "Failed: $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✓ All tests passed!"
    exit 0
else
    echo "✗ Some tests failed!"
    exit 1
fi
