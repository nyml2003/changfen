import argparse
import difflib
import os
import shutil
import subprocess
from typing import Any, Dict
import re
import datetime
import csv

def check_file(file1, file2, diff_file):
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        diff = difflib.unified_diff(
            list(map(lambda x: x.strip(), f1.readlines())),
            list(map(lambda x: x.strip(), f2.readlines())),
            fromfile=file1,
            tofile=file2,
        )
        diff_list = list(diff)

        if diff_file is not None:
            with open(diff_file, 'w') as f:
                f.writelines(diff_list)

        if len(diff_list) > 0:
            # compare float number with 1e-6 precision
            f1.seek(0)
            f2.seek(0)
            str1 = re.split(r'[ \n]', f1.read())
            str2 = re.split(r'[ \n]', f2.read())
            str1 = list(filter(lambda x: x != '', str1))
            str2 = list(filter(lambda x: x != '', str2))
            if (len(str1) != len(str2)):
                return False
            for i in range(len(str1)):
                if (str1[i] != str2[i]):
                    try:
                        float1 = float.fromhex(str1[i])
                        float2 = float.fromhex(str2[i])
                        if (abs(float1 - float2) > 1e-6):
                            return False
                    except:
                        return False

        return True

def execute(command, timeout) -> Dict[str, Any]:
    try:
        result = subprocess.run(command,
                                shell=True,
                                timeout=timeout,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True)

        return {
            'returncode': result.returncode,
            'stdout': result.stdout,
            'stderr': result.stderr,
        }

    except subprocess.TimeoutExpired:
        return {
            'returncode': None,
            'stdout': '',
            'stderr': 'TIMEOUT',
        }
    except Exception as e:
        return {
            'returncode': None,
            'stdout': '',
            'stderr': str(e),
        }

def log(logfile, command, exec_result):
    logfile.write(f'EXECUTE: {command}\n')
    logfile.write(f'STDOUT:\n')
    logfile.write(exec_result['stdout'])
    logfile.write(f'STDERR:\n')
    logfile.write(exec_result['stderr'])
    logfile.write(f'\n')

def test(executable_path: str, testcase_dir: str, output_dir: str,
         runtime_lib_dir: str, exec_timeout: int, test_ir: bool, output_std: bool):

    testcase_list = []

    def dfs(curr_dir: str):
        dir_list = sorted(os.listdir(curr_dir))

        for file_or_dir in dir_list:
            full_path = os.path.join(curr_dir, file_or_dir)

            if os.path.isfile(full_path) and full_path.endswith('.sy'):
                testcase_list.append(full_path.rsplit('.', 1)[0])

            elif os.path.isdir(full_path):
                dfs(full_path)

    dfs(testcase_dir)

    result_md = f"# Test Result\n "
    testcase_cnt = len(testcase_list)
    correct_cnt = 0
    result_md_table = f"| Testcase | Status | Time |\n"
    result_md_table += f"| -------- | ------ | ---- |\n"

    for testcase in testcase_list:
        start_test_time = datetime.datetime.now()
        basename: str = os.path.basename(testcase)

        in_path = f'{testcase}.in'

        std_out_path = f'{testcase}.out'
        if not os.path.isfile(in_path):
            in_path = None

        tokens_path = os.path.join(output_dir, f'{basename}.toks')
        ast_path = os.path.join(output_dir, f'{basename}.ast')
        ir_path = os.path.join(output_dir, f'{basename}.ll')
        opt_ir_path = os.path.join(output_dir, f'{basename}.opt.ll')
        asm_path = os.path.join(output_dir, f'{basename}.s')
        obj_from_ir_path = os.path.join(output_dir, f'{basename}.ir.o')
        out_path = os.path.join(output_dir, f'{basename}.out')
        exec_path = os.path.join(output_dir, f'{basename}')

        diff_path = os.path.join(output_dir, f'{basename}.diff')

        std_asm_from_ir_path = os.path.join(output_dir,
                                            f'{basename}.std_from_ir.s')

        std_ir_path = os.path.join(output_dir, f'{basename}.std.ll')
        std_asm_path = os.path.join(output_dir, f'{basename}.std.s')

        log_path = os.path.join(output_dir, f'{basename}.log')
        log_file = open(log_path, 'w')

        command = (f'{executable_path} {testcase}.sy '
                   f'--emit-ir {ir_path} '
                   f'--emit-opt-ir {opt_ir_path} '
                   f'-o {asm_path}')

        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        if exec_result['returncode'] is None:
            if exec_result['stderr'] == 'TIMEOUT':
                result_md_table += f"| `{testcase}` | ⚠️ sed TLE | {datetime.datetime.now() - start_test_time} |\n"
                print(f'[  ERROR  ] (sed TLE) {testcase}')
            else:
                result_md_table += f"| `{testcase}` | ⚠️ sed RE | {datetime.datetime.now() - start_test_time} |\n"
                print(f'[  ERROR  ] (sed RE) {testcase}')

            continue
        
        if test_ir:
            command = (f'llvm-link -opaque-pointers {opt_ir_path} '
                    f'{runtime_lib_dir}/libmylib.ll '
                    f'{runtime_lib_dir}/libsysy.ll '
                    f'-o {exec_path}')
        else:
            command = ('riscv64-linux-gnu-gcc '
                    f'-march=rv64gc {asm_path} '
                    f'-L{runtime_lib_dir} -lsysy '
                    f'-o {exec_path}')
        
        
        exec_result = execute(command, exec_timeout)
        log(log_file, command, exec_result)

        if exec_result['returncode'] is None:
            if exec_result['stderr'] == 'TIMEOUT':
                result_md_table += f"| `{testcase}` | ⚠️ sed TLE | {datetime.datetime.now() - start_test_time} |\n"
                print(f'[  ERROR  ] (sed TLE) {testcase}')
            else:
                result_md_table += f"| `{testcase}` | ⚠️ sed RE | {datetime.datetime.now() - start_test_time} |\n"
                print(f'[  ERROR  ] (sed RE) {testcase}')

            exit()
        elif exec_result['returncode'] != 0:
            result_md_table += f"| `{testcase}` | ⚠️ sed RE (syntax) | {datetime.datetime.now() - start_test_time}| \n"
            print(f'[  ERROR  ] (sed RE) (syntax) {testcase}')
            exit()
            

        if output_std:
            command = (f'clang -xc {testcase}.sy -include '
                    f'{runtime_lib_dir}/sylib.h '
                    f'-S -emit-llvm -o {std_ir_path} -O3')

            exec_result = execute(command, exec_timeout)
            log(log_file, command, exec_result)

            command = (f'llc -opaque-pointers -march=riscv64 {ir_path} '
                     f'-o {std_asm_from_ir_path}')

            exec_result = execute(command, exec_timeout)
            log(log_file, command, exec_result)

            if exec_result['returncode'] is None:
                result_md_table += f"| `{testcase}` | ⚠️ std ir->asm CE | {datetime.datetime.now() - start_test_time}| \n"
                print(f'[  ERROR  ] (std ir->asm CE) {testcase}')
                exit()

        if test_ir:
            command = (f'lli -opaque-pointers {exec_path} >{out_path}' if in_path is None
                     else f'lli -opaque-pointers {exec_path} <{in_path} >{out_path}')
        else:
            command = (f'qemu-riscv64 {exec_path} >{out_path}' if in_path is None
                    else f'qemu-riscv64 {exec_path} <{in_path} >{out_path}')
        
        

        exec_result = execute(command, exec_timeout)

        need_newline = False
        with open(out_path, 'r') as f:
            content = f.read()
            if len(content) > 0:
                if not content.endswith('\n'):
                    need_newline = True

        # add return code to the last line of out file
        with open(out_path, 'a+') as f:
            if need_newline:
                f.write('\n')
            f.write(str(exec_result['returncode']))
            f.write('\n')

        is_equal = check_file(out_path, std_out_path, diff_path)

        if exec_result['returncode'] is None:
            if exec_result['stderr'] == 'TIMEOUT':
                result_md_table += f'| `{testcase}` | ⏱️ TLE | {datetime.datetime.now() - start_test_time} |\n'
                print(f'[  ERROR  ] (TLE) {testcase}, check: {asm_path}')
            else:
                # SOS icon
                result_md_table += f'| `{testcase}` | 🆘 RE | {datetime.datetime.now() - start_test_time} |\n'
                print(f'[  ERROR  ] (RE) {testcase}, see: {log_path}')
        elif is_equal:
            correct_cnt += 1
            result_md_table += f'| `{testcase}` | ✅ AC | {datetime.datetime.now() - start_test_time}| \n'
            print(f'[ CORRECT ] (AC) {testcase}')
        else:
            result_md_table += f'| `{testcase}` | ❌ WA | {datetime.datetime.now() - start_test_time} \n'
            print(f'[  ERROR  ] (WA) {testcase}, see: {log_path}')

        log(log_file, command, exec_result)

    result_md += f'Passed {correct_cnt}/{testcase_cnt} testcases.\n\n'

    result_md += result_md_table

    with open(f'{output_dir}/../result.md', 'w') as f:
        f.write(result_md)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable_path', type=str, default='./test')
    parser.add_argument('--testcase_dir', type=str, default='./examples')
    parser.add_argument('--output_dir', type=str, default='./output')
    parser.add_argument('--runtime_lib_dir', type=str, default='./runtime-lib')
    parser.add_argument('--timeout', type=int, default=150)
    parser.add_argument('--test_ir', action='store_true', default=False)
    parser.add_argument('--output_std', action='store_true', default=False)
    args = parser.parse_args()
    return args

def main():
    args = parse_args()
    executable_path = args.executable_path
    testcase_dir = args.testcase_dir
    output_dir = args.output_dir
    runtime_lib_dir = args.runtime_lib_dir
    timeout = args.timeout
    test_ir = args.test_ir
    output_std = args.output_std
    if test_ir:
        print('[  INFO  ] Test LLVM IR')
    else:
        print('[  INFO  ] Test RISC-V assembly')
    
    if os.path.exists(output_dir):
            shutil.rmtree(output_dir)
    if not os.path.exists(output_dir):
            os.makedirs(output_dir)
    

    test(executable_path,
            testcase_dir,
            output_dir,
            runtime_lib_dir,
            timeout,
            test_ir,
            output_std)

if __name__ == '__main__':
    main()