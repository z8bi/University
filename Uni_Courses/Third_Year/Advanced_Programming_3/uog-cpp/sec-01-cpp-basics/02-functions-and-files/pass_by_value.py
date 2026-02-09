def pass_by_value(x):
    x += 10
    return x

num = 5
result = pass_by_value(num)
print("Original number:", num)  # Output: Original number: 5
print("Result after function call:", result)  # Output: Result after function call:


def pass_by_list(lst):
    lst[0] += 10
    return lst

my_list = [5]
result_list = pass_by_list(my_list)
print("Original list:", my_list)  # Output: Original list: [15]
print("Result after function call:", result)  # Output: Result after function call: 15
