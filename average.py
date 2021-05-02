f = open('simplelog', 'r')
simple_avg = 0.0
simple_max = 0.0
simple_min = 1.0
for num in f:
    temp = float(num)
    simple_avg += temp
    if temp > simple_max:
        simple_max = temp
    if temp < simple_min:
        simple_min = temp

f.close()

f = open('testlog', 'r')
test_avg = 0.0
test_max = 0.0
test_min = 1.0
for num in f:
    temp = float(num)
    test_avg += temp
    if temp > test_max:
        test_max = temp
    if temp < test_min:
        test_min = temp

f.close()

print('simple_test.c average: ' + str(simple_avg/15) + ' seconds')
print('fastest, slowest in simple: ' + str(simple_min) + ' ' + str(simple_max))
print('test_cases.c average: ' + str(test_avg/15) + ' seconds')
print('fastest, slowest in test: ' + str(test_min) + ' ' + str(test_max))