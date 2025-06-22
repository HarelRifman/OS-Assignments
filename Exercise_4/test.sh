#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0

echo -e "${BLUE}=== News Broadcasting System Tester ===${NC}"
echo ""

print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC}: $2"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}✗ FAILED${NC}: $2"
        ((TESTS_FAILED++))
    fi
}

create_test_config() {
    local filename=$1
    local config_content=$2
    echo "$config_content" > "$filename"
}

# Test 1: Compilation
echo -e "${YELLOW}Test 1: Compilation${NC}"
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ -f "ex3.out" ]; then
    print_result 0 "Program compiles successfully"
else
    print_result 1 "Program failed to compile"
    exit 1
fi
echo ""

# Test 2: Single Producer, All Types
echo -e "${YELLOW}Test 2: Single Producer, All Types${NC}"
create_test_config "test1.txt" "PRODUCER 1
9
queue size = 5

Co-Editor queue size = 5"
timeout 10s ./ex3.out test1.txt > test1_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    # Check for correct format and DONE
    format_count=$(grep -E "^Producer 1 (SPORTS|NEWS|WEATHER) [0-9]+$" test1_output.txt | wc -l)
    done_count=$(grep -c "^DONE$" test1_output.txt)
    if [ $format_count -eq 9 ] && [ $done_count -eq 1 ]; then
        print_result 0 "Single producer, all types, correct output and DONE"
    else
        print_result 1 "Single producer, all types, output or DONE missing (format: $format_count/9, DONE: $done_count/1)"
    fi
else
    print_result 1 "Single producer, all types, program timeout or crash"
fi
echo ""

# Test 3: Multiple Producers, Mixed Loads
echo -e "${YELLOW}Test 3: Multiple Producers, Mixed Loads${NC}"
create_test_config "test2.txt" "PRODUCER 1
3
queue size = 2

PRODUCER 2
5
queue size = 3

PRODUCER 3
7
queue size = 4

Co-Editor queue size = 6"
timeout 15s ./ex3.out test2.txt > test2_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    p1=$(grep -c "^Producer 1 " test2_output.txt)
    p2=$(grep -c "^Producer 2 " test2_output.txt)
    p3=$(grep -c "^Producer 3 " test2_output.txt)
    done_count=$(grep -c "^DONE$" test2_output.txt)
    if [ $p1 -eq 3 ] && [ $p2 -eq 5 ] && [ $p3 -eq 7 ] && [ $done_count -eq 1 ]; then
        print_result 0 "Multiple producers, correct message counts and DONE"
    else
        print_result 1 "Multiple producers, incorrect counts (P1:$p1/3, P2:$p2/5, P3:$p3/7, DONE:$done_count/1)"
    fi
else
    print_result 1 "Multiple producers, program timeout or crash"
fi
echo ""

# Test 4: Queue Size Stress (Small Buffers)
echo -e "${YELLOW}Test 4: Queue Size Stress (Small Buffers)${NC}"
create_test_config "test3.txt" "PRODUCER 1
10
queue size = 1

PRODUCER 2
10
queue size = 1

Co-Editor queue size = 1"
timeout 20s ./ex3.out test3.txt > test3_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    p1=$(grep -c "^Producer 1 " test3_output.txt)
    p2=$(grep -c "^Producer 2 " test3_output.txt)
    done_count=$(grep -c "^DONE$" test3_output.txt)
    if [ $p1 -eq 10 ] && [ $p2 -eq 10 ] && [ $done_count -eq 1 ]; then
        print_result 0 "Small buffer stress test passed"
    else
        print_result 1 "Small buffer stress test failed (P1:$p1/10, P2:$p2/10, DONE:$done_count/1)"
    fi
else
    print_result 1 "Small buffer stress test, program timeout or crash"
fi
echo ""

# Test 5: Output Order (DONE after all messages)
echo -e "${YELLOW}Test 5: Output Order (DONE after all messages)${NC}"
create_test_config "test4.txt" "PRODUCER 1
5
queue size = 2

PRODUCER 2
5
queue size = 2

Co-Editor queue size = 2"
timeout 10s ./ex3.out test4.txt > test4_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    last_line=$(tail -n 1 test4_output.txt)
    if [ "$last_line" = "DONE" ]; then
        print_result 0 "DONE is printed last"
    else
        print_result 1 "DONE is not printed last"
    fi
else
    print_result 1 "Output order test, program timeout or crash"
fi
echo ""

# Test 6: Edge Case - Zero Products
echo -e "${YELLOW}Test 6: Edge Case - Zero Products${NC}"
create_test_config "test5.txt" "PRODUCER 1
0
queue size = 2

Co-Editor queue size = 2"
timeout 5s ./ex3.out test5.txt > test5_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    prod_count=$(grep -c "^Producer 1 " test5_output.txt)
    done_count=$(grep -c "^DONE$" test5_output.txt)
    if [ $prod_count -eq 0 ] && [ $done_count -eq 1 ]; then
        print_result 0 "Zero products handled correctly"
    else
        print_result 1 "Zero products test failed (Prod:$prod_count/0, DONE:$done_count/1)"
    fi
else
    print_result 1 "Zero products test, program timeout or crash"
fi
echo ""
# Test 7: Large Number of Producers
echo -e "${YELLOW}Test 7: Large Number of Producers${NC}"
config=""
for i in $(seq 1 10); do
    config="${config}PRODUCER $i
3
queue size = 2

"
done
config="${config}Co-Editor queue size = 10"
create_test_config "test6.txt" "$config"
timeout 20s ./ex3.out test6.txt > test6_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    total=$(grep -E "^Producer [0-9]+ " test6_output.txt | wc -l)
    done_count=$(grep -c "^DONE$" test6_output.txt)
    if [ $total -eq 30 ] && [ $done_count -eq 1 ]; then
        print_result 0 "Large number of producers handled correctly"
    else
        print_result 1 "Large number of producers test failed (Total:$total/30, DONE:$done_count/1)"
    fi
else
    print_result 1 "Large number of producers, program timeout or crash"
fi
echo ""

# Test 8: Invalid Config File
echo -e "${YELLOW}Test 8: Invalid Config File${NC}"
timeout 5s ./ex3.out not_a_real_file.txt > test7_output.txt 2>&1
exit_code=$?
if [ $exit_code -ne 0 ]; then
    print_result 0 "Invalid config file handled with error"
else
    print_result 1 "Invalid config file should cause error"
fi
echo ""

# Test 9: No Argument
echo -e "${YELLOW}Test 9: No Argument${NC}"
timeout 5s ./ex3.out > test8_output.txt 2>&1
exit_code=$?
if [ $exit_code -ne 0 ]; then
    print_result 0 "No argument handled with error"
else
    print_result 1 "No argument should cause error"
fi
echo ""

# Test 10: Message Format Strictness
echo -e "${YELLOW}Test 10: Message Format Strictness${NC}"
create_test_config "test9.txt" "PRODUCER 1
5
queue size = 2

Co-Editor queue size = 2"
timeout 10s ./ex3.out test9.txt > test9_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    bad_lines=$(grep -vE "^Producer [0-9]+ (SPORTS|NEWS|WEATHER) [0-9]+$|^DONE$" test9_output.txt | wc -l)
    if [ $bad_lines -eq 0 ]; then
        print_result 0 "All output lines are in correct format"
    else
        print_result 1 "Some output lines are not in correct format ($bad_lines bad lines)"
    fi
else
    print_result 1 "Message format strictness test, program timeout or crash"
fi
echo ""

# Test 11: Performance (Many Messages)
echo -e "${YELLOW}Test 11: Performance (Many Messages)${NC}"
create_test_config "test10.txt" "PRODUCER 1
100
queue size = 10

PRODUCER 2
100
queue size = 10

PRODUCER 3
100
queue size = 10

Co-Editor queue size = 50"
start_time=$(date +%s)
timeout 60s ./ex3.out test10.txt > test10_output.txt 2>&1
exit_code=$?
end_time=$(date +%s)
if [ $exit_code -eq 0 ]; then
    total=$(grep -E "^Producer [0-9]+ " test10_output.txt | wc -l)
    done_count=$(grep -c "^DONE$" test10_output.txt)
    duration=$((end_time - start_time))
    if [ $total -eq 300 ] && [ $done_count -eq 1 ] && [ $duration -lt 60 ]; then
        print_result 0 "Performance test passed in $duration seconds"
    else
        print_result 1 "Performance test failed (Total:$total/300, DONE:$done_count/1, Time:$duration)"
    fi
else
    print_result 1 "Performance test, program timeout or crash"
fi
echo ""

# Test 12: Output Order (No Interleaving of DONE)
echo -e "${YELLOW}Test 12: Output Order (No Interleaving of DONE)${NC}"
create_test_config "test11.txt" "PRODUCER 1
2
queue size = 1

PRODUCER 2
2
queue size = 1

Co-Editor queue size = 1"
timeout 10s ./ex3.out test11.txt > test11_output.txt 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    done_lines=$(grep -n "^DONE$" test11_output.txt | cut -d: -f1)
    last_line=$(wc -l < test11_output.txt)
    if [ "$done_lines" = "$last_line" ]; then
        print_result 0 "DONE is the last line (no interleaving)"
    else
        print_result 1 "DONE is not the last line (interleaved output)"
    fi
else
    print_result 1 "Output order (no interleaving), program timeout or crash"
fi
echo ""

# Summary
echo -e "${BLUE}=== Test Summary ===${NC}"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo -e "Total Tests: $((TESTS_PASSED + TESTS_FAILED))"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed! 🎉${NC}"
else
    echo -e "${RED}Some tests failed. Check the output above for details.${NC}"
fi

# Cleanup
echo ""
echo -e "${YELLOW}Cleaning up test files...${NC}"
rm -f test*.txt test*_output.txt

exit $TESTS_FAILED