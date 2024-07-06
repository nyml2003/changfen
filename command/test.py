import subprocess
import glob
import json
import os
import time

with open(f"{os.path.dirname(__file__)}/persist.json", "r") as config_file:
    config = json.load(config_file)

executable = config["executable"]
test_dirs = config["input"]


class TestResult:
    def __init__(self, input_file, stdout=None, stderr=None, err=None):
        self.input_file = input_file
        self.stdout = stdout
        self.stderr = stderr
        self.err = err
        self.index = None

    def flag_the_index(self, index):
        self.index = index
        return self


class TestResultManager:
    def __init__(self, file=os.path.dirname(__file__) + "/log.json"):
        self.file = file
        self.results = []

    def append(self, result):
        self.results.append(result.flag_the_index(len(self.results)))

    def __str__(self):
        return "\n".join(map(str, self.results))

    def save(self):
        with open(self.file, "w") as f:
            f.write(json.dumps([vars(result) for result in self.results], indent=4))


testResultManager = TestResultManager()

# 遍历输入文件
# input_files = glob.glob(f"{test_dir}/*.sy")
input_files = []
for test_dir in test_dirs:
    input_files += glob.glob(f"{test_dir}/*.sy")
for input_file in input_files:
    # 运行程序
    try:
        result = subprocess.run(
            [executable, input_file], capture_output=True, text=True, check=True
        )
        stdout = result.stdout.strip()
        stderr = result.stderr.strip()

        if stderr:
            testResultManager.append(
                TestResult(input_file=input_file, stdout=stdout, stderr=stderr)
            )
            continue
    except subprocess.CalledProcessError as e:
        testResultManager.append(
            TestResult(input_file=input_file, err=str(e.stderr), stdout=str(e.stdout))
        )
        continue

testResultManager.save()
