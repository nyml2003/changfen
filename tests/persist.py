import argparse
import json
parser = argparse.ArgumentParser(description='test')
parser.add_argument('--executable', required=True, help='编译产物的路径')
# 测试用例所在路径，一般会包含input和expected_output两个部分
parser.add_argument('--input', nargs='+', required=True, help='测试用例所在路径')
# python的输出，用来存放处理过的测试结果
parser.add_argument('--expected', required=False, help='输出文件所在路径')
# 本文件所在的路径
parser.add_argument('--pwd', required=True, help='本文件所在的路径')
# 测试所在的路径
parser.add_argument('--test', required=True, help='测试所在的路径')

result = parser.parse_args().__dict__

# 将编译产物复制到测试用例目录下，并更名为test
import os
import shutil
if os.path.exists(f'{result["test"]}/test'):
    os.remove(f'{result["test"]}/test')
shutil.copy(result['executable'], f'{result["test"]}/test')
    

